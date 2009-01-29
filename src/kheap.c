#include "kheap.h"
#include "paging.h"
#include "monitor.h"

#define PAGE_SIZE 0x1000

#define PAGE_ALIGNED(addr) ((addr)%PAGE_SIZE == 0)

// defined in the linker script
extern u32int end;
u32int placement_address = (u32int)&end;
heap_t *kheap = 0;;

// defined in paging.c
extern page_directory_t *kernel_directory;

static s32int find_smallest_hole(u32int size, u8int page_align, heap_t *heap)
{
  u32int iterator = 0;
  while (iterator < heap->index.size) {
    header_t *header = (header_t*) lookup_ordered_array(iterator,&heap->index);
    // if caller request page-aligned memory
    if(page_align > 0) {
      u32int location = (u32int)header;
      s32int offset = 0;
      if(((location+sizeof(header_t)) & 0xFFFFF000) != 0) {
	offset = PAGE_SIZE - (location+sizeof(header_t))%PAGE_SIZE;
      }
      s32int hole_size = (s32int)header->size - offset;
      // if we can fit in this hole with the required padding...
      if(hole_size >= (s32int)size) {
	break;
      }
    } else if(header->size >= size) {
      break;
    }
    iterator++;
  }

  if(iterator == heap->index.size) {
    return -1; // got to the end, no dice
  } else {
    return iterator;
  } 
}

static s8int header_t_less_than(void *a, void *b)
{
  return (((header_t*)a)->size < ((header_t*)b)->size)?1:0;
}

heap_t *create_heap(u32int start, u32int end_addr, u32int max, u8int supervisor, u8int readonly)
{
  heap_t *heap = (heap_t*)kmalloc(sizeof(heap_t));

  // we require page-aligned start and end addresses
  ASSERT(PAGE_ALIGNED(start));
  ASSERT(PAGE_ALIGNED(end_addr));

  // initialize the heap's index
  heap->index = place_ordered_array((void*)start, HEAP_INDEX_SIZE, &header_t_less_than);

  // shift start address forward to the about where we can start allocating from
  start += sizeof(type_t)*HEAP_INDEX_SIZE;
  
  // make sure new start is page-aligned
  if((start & 0xFFFFF000) != 0) {
    // round up to nearest page boundary
    start &= 0xFFFFF000;
    start += PAGE_SIZE;
  }
  
  heap->start_address = start;
  heap->end_address = end_addr;
  heap->max_address = max;
  heap->supervisor = supervisor;
  heap->readonly = readonly;

  // create one huge hole to start with
  header_t *hole = (header_t*)start;
  hole->size = end_addr-start;
  hole->magic = HEAP_MAGIC;
  hole->is_hole = 1;

  footer_t *footer = (footer_t*)((u32int)hole + hole->size -sizeof(footer_t));
  footer->magic = HEAP_MAGIC;
  footer->header = hole;
  insert_ordered_array((void*)hole, &heap->index);

  return heap;
}


static void expand(u32int new_size, heap_t *heap)
{
  // sanity check the request
  ASSERT(new_size > (heap->end_address - heap->start_address));
  
  // we want to expand to the nearest page boundary
  if((new_size & 0xFFFFF000) != 0) {
    new_size &= 0xFFFFF000;
    new_size += PAGE_SIZE;
  }

  ASSERT((heap->start_address+new_size) <= heap->max_address);
  
  // allocate pages and page frames for the new chunks of heap
  u32int old_size = heap->end_address - heap->start_address;
  u32int i = old_size;
  while(i < new_size) {
    alloc_frame(get_page(heap->start_address+i,1,kernel_directory),
		(heap->supervisor)?1:0,(heap->readonly)?0:1);
    i += PAGE_SIZE;
  }
  heap->end_address = heap->start_address+new_size;
}

static u32int contract(u32int new_size, heap_t *heap)
{
  ASSERT(new_size < (heap->end_address - heap->start_address));
  
  // get nearest page boundary
  if(new_size & PAGE_SIZE) {
    new_size &= PAGE_SIZE;
    new_size += 0x1000;
  }
  
  // don't shrink below the minimum
  if(new_size < HEAP_MIN_SIZE) {
    new_size = HEAP_MIN_SIZE;
  }
  
  u32int old_size = heap->end_address - heap->start_address;
  u32int i = old_size - PAGE_SIZE;
  while(new_size < i) {
    free_frame(get_page(heap->start_address+i,0,kernel_directory));
    i -= PAGE_SIZE;
  }
  heap->end_address = heap->start_address + new_size;
  return new_size;
}

void *alloc(u32int size, u8int page_align, heap_t *heap)
{
  // allocation == requested size + size of header and footer
  u32int new_size = size + sizeof(header_t) + sizeof(footer_t);

  // find a hole that fits
  s32int iterator = find_smallest_hole(new_size, page_align, heap);
  
  if (iterator == -1) { // if no hole big enough exists...
    // we're going to need to grow the heap, so save some data
    u32int old_length = heap->end_address - heap->start_address;
    u32int old_end_address = heap->end_address;
    expand(old_length+new_size, heap);
    u32int new_length = heap->end_address-heap->start_address;

    // find the last header (we'll be using it and the new space)
    iterator = 0;
    u32int idx = -1;
    u32int value = 0;

    while(iterator < heap->index.size) {
      u32int tmp = (u32int)lookup_ordered_array(iterator,&heap->index);
      if(tmp > value) {
	value = tmp;
	idx = iterator;
      }
      iterator++;
    }
    // if we didn't find any headers, make one in the new space
    if(idx == -1) {
      header_t *header = (header_t*)old_end_address;
      header->magic = HEAP_MAGIC;
      header->size = new_length-old_length;
      header->is_hole = 1;
      
      footer_t *footer = (footer_t*)(old_end_address+header->size - sizeof(footer_t));
      footer->magic = HEAP_MAGIC;
      footer->header = header;
      insert_ordered_array((void*)header, &heap->index);
    } else {
      // adjust last header to point into new space
      header_t *header = lookup_ordered_array(idx, &heap->index);
      header->size += new_length - old_length;
      //rewrite footer;
      footer_t *footer = (footer_t*)((u32int)header + header->size - sizeof(footer_t));
      footer->header = header;
      footer->magic = HEAP_MAGIC;
    }
    // we have enough space now! recur.
    return alloc(size, page_align, heap);
  }
  
  header_t *orig_hole_header = (header_t *)lookup_ordered_array(iterator, &heap->index);
  u32int orig_hole_pos = (u32int)orig_hole_header;
  u32int orig_hole_size = orig_hole_header->size;
  
  // should we split the hole into two parts (is the hole huge?)
  if(orig_hole_size - new_size < sizeof(header_t) + sizeof(footer_t)) {
    // it's not, increase the requested size so we don't split it
    size += orig_hole_size - new_size;
    new_size = orig_hole_size;
  }
  
  // page align data if necessary and make a new hole out of the space lost by doing so
  if(page_align && orig_hole_pos&0xFFFFF000) {
    u32int offset = (orig_hole_pos & 0xFFF) - sizeof(header_t);
    u32int new_location = orig_hole_pos + PAGE_SIZE - offset;
    header_t *hole_header = (header_t*)orig_hole_pos;
    hole_header->size = PAGE_SIZE - offset;
    hole_header->magic = HEAP_MAGIC;
    hole_header->is_hole = 1;
    footer_t *hole_footer = (footer_t*)((u32int)new_location - sizeof(footer_t));
    hole_footer->magic = HEAP_MAGIC;
    hole_footer->header = hole_header;
    orig_hole_pos = new_location;
    orig_hole_size = orig_hole_size - hole_header->size;
  } else {
    // remove the hole from the index
    remove_ordered_array(iterator,&heap->index);
  }
  // overwrite original block header
  header_t *block_header = (header_t *)orig_hole_pos;
  block_header->magic = HEAP_MAGIC;
  block_header->is_hole = 0;
  block_header->size = new_size;
  // and footer
  footer_t *block_footer = (footer_t*)(orig_hole_pos + sizeof(header_t) + size);
  block_footer->magic = HEAP_MAGIC;
  block_footer->header = block_header;

  // if we need to split the hole, do it here
  if(orig_hole_size - new_size > 0) {
    header_t *hole_header = (header_t*)(orig_hole_pos+sizeof(header_t) + size + sizeof(footer_t));
    hole_header->magic = HEAP_MAGIC;
    hole_header->is_hole = 1;
    hole_header->size = orig_hole_size - new_size;
    footer_t *hole_footer = (footer_t*)(orig_hole_pos + orig_hole_size - sizeof(footer_t));
    if((u32int)hole_footer < heap->end_address) {
      hole_footer->magic = HEAP_MAGIC;
      hole_footer->header = hole_header;
    }
    insert_ordered_array((void*)hole_header,&heap->index);
  }

  // and we're done!
  return (void*)((u32int)block_header+sizeof(header_t));
}

void free(void *p, heap_t *heap)
{
  // if the pointer's null do nothing
  if(!p) {
    return;
  }

  // get the pointer's header and footer
  header_t *header = (header_t*)((u32int)p - sizeof(header_t));
  footer_t *footer = (footer_t*)((u32int)header + header->size - sizeof(footer_t));
  
  // sanity check
  ASSERT(header->magic == HEAP_MAGIC);
  ASSERT(footer->magic == HEAP_MAGIC);

  // make the block a hole again
  header->is_hole = 1;

  // presumably we want to add this hole back into the index by default
  char do_add = 1;

  // if the thing to the left in the index is a hole, unify this hole with it
  // (to prevent fragmentation)
  footer_t *test_footer = (footer_t*)((u32int)header-sizeof(footer_t));
  if(test_footer->magic == HEAP_MAGIC &&
     test_footer->header->is_hole == 1) {
    
    u32int old_size = header->size;
    header = test_footer->header;
    footer->header = header;
    header->size += old_size;
    do_add = 0;
  }

  // if the thing to the right of us is a header, unify it with this hole
  header_t *test_header = (header_t*)((u32int)footer + sizeof(footer_t));
  if(test_header->magic == HEAP_MAGIC &&
     test_header->is_hole == 1) {
    // increase our hole's size
    header->size += test_header->size;
    // rewrite footer
    test_footer = (footer_t*)((u32int)test_header+test_header->size - sizeof(footer_t));
    footer = test_footer;
    footer->header = header;

    // remove old hole from index
    u32int iterator = 0;
    while((iterator < heap->index.size) &&
	  (lookup_ordered_array(iterator, &heap->index) != (void*)test_header)) {
      iterator++;
    }
    ASSERT(iterator < heap->index.size);
    remove_ordered_array(iterator, &heap->index);
  }

  // if the footer is at the end of the heap, we can contract the heap
  if((u32int)footer+sizeof(footer_t) == heap->end_address) {
    u32int old_length = heap->end_address-heap->start_address;
    u32int new_length = contract((u32int)header-heap->start_address, heap);
    // will this block still exist after resizing?
    if(header->size - (old_length - new_length) > 0) {
      // yes, contract the block and rewrite the footer
      header->size -= (old_length - new_length);
      footer = (footer_t*)((u32int)header + header->size - sizeof(footer_t));
      footer->magic = HEAP_MAGIC;
      footer->header = header;
    } else {
      // no, delete the block entirely
      u32int iterator = 0;
      while((iterator <  heap->index.size) &&
	    (lookup_ordered_array(iterator,&heap->index) != (void*)header)) {
	iterator++;
      }
      
      if(iterator < heap->index.size) {
	do_add = 0;
	remove_ordered_array(iterator, &heap->index);
      }
    }
  }
  if(do_add) {
    insert_ordered_array((void*)header, &heap->index);
  }
}

static u32int kmalloc_int(u32int sz, int align,u32int *phys)
{
  if (kheap != 0) {
    void *addr = alloc(sz, (u8int)align, kheap);
    if (phys != 0) {
      page_t *page = get_page((u32int)addr, 0, kernel_directory);
      *phys = page->frame*0x1000 + ((u32int)addr&0xFFF);
    }
    return (u32int)addr;
  } else {
    if (align == 1 && (placement_address & 0xFFFFF000)) // If the address is not already page-aligned
      {
	// Align it.
	placement_address &= 0xFFFFF000;
	placement_address += 0x1000;
      }

    if(phys) {
      *phys = placement_address;
    }

    u32int tmp = placement_address;
    placement_address += sz;

    return tmp;
  }
}

u32int kmalloc_a(u32int sz)
{
  return kmalloc_int(sz,1,0);
}

u32int kmalloc_p(u32int sz, u32int *phys)
{
  return kmalloc_int(sz,0,phys);
}

u32int kmalloc_ap(u32int sz, u32int *phys)
{
  return kmalloc_int(sz,1,phys);
}

u32int kmalloc(u32int sz)
{
  return kmalloc_int(sz,0,0);
}

void kfree(void *p)
{
  free(p,kheap);
}
