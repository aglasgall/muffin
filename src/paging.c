#include "paging.h"
#include "monitor.h"
#include "kheap.h"
#include "multiboot.h"

#define PAGE_SIZE 0x1000

static u32int *frames = 0;
// set = mmio space, unset = real memory
static u32int *memory_map = 0;
static u32int nframes = 0;

page_directory_t* current_directory = 0;
page_directory_t* kernel_directory = 0;

// defined in kheap.c
extern u32int placement_address;
extern heap_t *kheap;

// defined in main.c
extern struct multiboot *mboot_header;

// defined in process.s
void copy_page_physical(u32int src, u32int dest);

#define INDEX_FROM_BIT(a) ((a)/(8*4))
#define OFFSET_FROM_BIT(a) ((a)%(8*4))


static void set_frame_in_map(u32int frame_addr,u32int *map)
{
  u32int frame = frame_addr/PAGE_SIZE;
  u32int idx = INDEX_FROM_BIT(frame);
  u32int off = OFFSET_FROM_BIT(frame);
  map[idx] |= (0x1 << off);
}

// set a bit in the frames bitset
static void set_frame(u32int frame_addr)
{
  set_frame_in_map(frame_addr,frames);
}

static void clear_frame_in_map(u32int frame_addr,u32int *map)
{
   u32int frame = frame_addr/PAGE_SIZE;
   u32int idx = INDEX_FROM_BIT(frame);
   u32int off = OFFSET_FROM_BIT(frame);
   map[idx] &= ~(0x1 << off);
}

// Static function to clear a bit in the frames bitset
static void clear_frame(u32int frame_addr)
{
  clear_frame_in_map(frame_addr,frames);
}


static u32int test_frame_in_map(u32int frame_addr,u32int *map)
{
   u32int frame = frame_addr/PAGE_SIZE;
   u32int idx = INDEX_FROM_BIT(frame);
   u32int off = OFFSET_FROM_BIT(frame);
   return (map[idx] & (0x1 << off));
}

// Static function to test if a bit is set.
static u32int test_frame(u32int frame_addr)
{
  return test_frame_in_map(frame_addr,frames);
}

// Static function to find the first free frame.
static u32int first_frame()
{
   u32int i, j;
   for (i = 0; i < INDEX_FROM_BIT(nframes); i++)
   {
       if (frames[i] != 0xFFFFFFFF) // nothing free, exit early.
       {
           // at least one bit is free here.
           for (j = 0; j < 32; j++)
           {
               u32int toTest = 0x1 << j;
               if ( !(frames[i]&toTest) && !(memory_map[i]&toTest))
               {
                   return i*4*8+j;
               }
           }
       }
   }
   return -1;
} 


void alloc_frame(page_t *page, int is_kernel, int is_writeable)
{
  if(page->frame) {
    return; // page already has an allocated frame
  }
  u32int idx = first_frame();
  if(idx == (u32int)-1) {
    PANIC("No free frames!");
  }

  set_frame(idx*PAGE_SIZE); // mark frame as in use
  page->present = 1; // mark page as present
  page->rw = (is_writeable)?1:0;
  page->user = (is_kernel)?0:1;
  page->frame = idx;
}

void free_frame(page_t *page)
{
  u32int frame;
  if(!(frame=page->frame)) {
    // page wasn't actually allocated
    return;
  }
  clear_frame(frame);
  page->frame = 0;
}

static page_table_t *clone_table(page_table_t *src, u32int *physAddr)
{
  // allocate a new table
  page_table_t *table = (page_table_t*)kmalloc_ap(sizeof(page_table_t),physAddr);
  memset(table,0,sizeof(page_table_t));
  
  int i = 0;
  for(i = 0; i < 1024; ++i) {
    // ignore unmapped pages (THIS WILL CHANGE)
    if(!src->pages[i].frame) {
      continue;
    }

    alloc_frame(&table->pages[i], 0, 0);

    if (src->pages[i].present) table->pages[i].present = 1;
    if (src->pages[i].rw) table->pages[i].rw = 1;
    if (src->pages[i].user) table->pages[i].user = 1;
    if (src->pages[i].accessed) table->pages[i].accessed = 1;
    if (src->pages[i].dirty) table->pages[i].dirty = 1;

    // copy the data in the physical page
    copy_page_physical(src->pages[i].frame*PAGE_SIZE, table->pages[i].frame*PAGE_SIZE);
  }
  return table;
}

page_directory_t *clone_directory(page_directory_t *src)
{
  u32int phys = 0;
  // make a new directory and get its physical address
  page_directory_t *dir = (page_directory_t*)kmalloc_ap(sizeof(page_directory_t), &phys);
  memset(dir,0,sizeof(page_directory_t));

  // get the offset of tablesPhysical from the start of dir
  u32int offset = (u32int)dir->tablesPhysical - (u32int)dir;
  dir->physicalAddr = phys+offset;

  // copy the page tables
  int i = 0;
  for(i = 0; i < 1024; ++i) {
    // skip empty tables
    if(!src->tables[i]) {
      continue;
    }
    // link instead of copying the kernel pages
    // (copy the address. we're in the kernel, so we can use the same ptr)
    if(kernel_directory->tables[i] == src->tables[i]) {
      dir->tables[i] = src->tables[i];
      dir->tablesPhysical[i] = src->tablesPhysical[i];
    } else {
      // copy table
      u32int phys = 0;
      dir->tables[i] = clone_table(src->tables[i], &phys);
      // as in get_page
      dir->tablesPhysical[i] = phys | 0x07;
    }
  }
  return dir;
}

void initialize_paging()
{
  // we have memory up to 7FF0000
  u32int mem_end_page = 0x7FEF000;                        
  // let's take a look at what the memory map we're given looks like
  multiboot_mmap_entry_t *entry = mboot_header->mmap_addr;
  multiboot_mmap_entry_t *next_entry = 0;
  u32int mmap_entries = mboot_header->mmap_length/sizeof(multiboot_mmap_entry_t);
  monitor_write_hex(mmap_entries);
  monitor_write(" memory map entries\n");

  monitor_write("Memory map from ");
  monitor_write_hex((u32int)(mboot_header->mmap_addr));
  monitor_write(" to ");
  monitor_write_hex((u32int)(mboot_header->mmap_addr) + mboot_header->mmap_length);
  monitor_write("\n");

  //  while(entry < mboot_header->mmap_addr + mboot_header->mmap_length) {
  for(u32int i = 0; i < mmap_entries; ++i) {

    next_entry = (multiboot_mmap_entry_t*)(mboot_header->mmap_addr+i);
    entry = next_entry;

    monitor_write("Memory area from ");
    //    monitor_write_hex(area->base_addr_high);
    monitor_write_hex(entry->base_addr);
    //    monitor_write("\n");
    monitor_write("\tto ");
    //    monitor_write_hex(area->length_high);
    monitor_write_hex(entry->base_addr + entry->length);
    monitor_write(" type ");
    monitor_write_hex(entry->type);
    monitor_write("\n");
    nframes += entry->length / PAGE_SIZE;
  }
  monitor_write_hex(nframes);
  monitor_write(" page frames\n");
  //  PANIC("finished dumping memory map");
    // number of page frames == memory / page frame size
  //  nframes = mem_end_page / PAGE_SIZE;
  // allocate the page frame bitmap
  frames = (u32int*)kmalloc(INDEX_FROM_BIT(nframes));
  // allocate the memory map
  memory_map =  (u32int*)kmalloc(INDEX_FROM_BIT(nframes));
  memset(frames,0,INDEX_FROM_BIT(nframes));
  memset(memory_map,0,INDEX_FROM_BIT(nframes));

  // allocate a page directory
  kernel_directory = (page_directory_t*)kmalloc_a(sizeof(page_directory_t));
  memset(kernel_directory,0,sizeof(page_directory_t));
  kernel_directory->physicalAddr = (u32int)kernel_directory->tablesPhysical;

  // map pages for kernel heap
  u32int i = 0;
  page_t *page = 0;
  for(i = KHEAP_START; i < KHEAP_START+KHEAP_INITIAL_SIZE; i += PAGE_SIZE) {
    page = get_page(i,1,kernel_directory);
  } 

  // walk the multiboot memory map again to fence off MMIO space
  for(i = 0; i < mmap_entries; ++i) {
    entry = (multiboot_mmap_entry_t*)(mboot_header->mmap_addr+i);
      if(entry->type != 0x1) {
	for(u32int j = entry->base_addr; j < entry->length; j += PAGE_SIZE) {
	  u32int frame = j / PAGE_SIZE;
	  set_frame_in_map(j,memory_map);
	}
      }
  }
  
  // identity map all of the memory we've allocated so far
  // so we can use it once we've enabled paging
  i = 0;
  while(i <= placement_address+0x1000) {
    page = get_page(i,1,kernel_directory);
    alloc_frame(page,0,0);
    i += PAGE_SIZE;
  }

  // now allocate the pages we mapped earlier for the heap
  for(i = KHEAP_START; i < KHEAP_START+KHEAP_INITIAL_SIZE; i += PAGE_SIZE) {
    page = get_page(i,1,kernel_directory);
    alloc_frame(page,0,0);
  }

  // register page fault handler
  register_interrupt_handler(14,page_fault);
  // load page diretectory into cr3 and enable paging bit in cr0
  switch_page_directory(kernel_directory);

  // we've enabled paging, now create a heap!
  kheap = create_heap(KHEAP_START, KHEAP_START + KHEAP_INITIAL_SIZE, 0xCFFFF000, 0, 0);
  monitor_write("About to enable paging\n");
  current_directory = clone_directory(kernel_directory);
  switch_page_directory(current_directory);

  monitor_write("Paging enabled!\n");
}

void switch_page_directory(page_directory_t *dir)
{
  u32int cr0 = 0;
  // disable paging
  current_directory = dir;
  // load page directory pointer into cr3

  asm volatile("mov %0, %%cr3"::"r"(dir->physicalAddr));

  asm volatile("mov %%cr0, %0": "=r"(cr0));
  cr0 |= 0x80000000; // turn paging bit on
  asm volatile ("mov %0, %%cr0":: "r"(cr0));
}

page_t *get_page(u32int address, int make, page_directory_t *dir)
{
  address /= PAGE_SIZE; // turn address into index in bitmap
  // find page table holding index
  u32int table_idx = address / 1024;
  if(dir->tables[table_idx]) { // if the table already exists
    return &dir->tables[table_idx]->pages[address%1024];
  } else if(make) {
    u32int tmp = 0;
    dir->tables[table_idx] = (page_table_t*)kmalloc_ap(sizeof(page_table_t),&tmp);
    memset(dir->tables[table_idx], 0, sizeof(page_table_t));
    dir->tablesPhysical[table_idx] = tmp | 0x7; // PRESENT, RW, USER
    return &dir->tables[table_idx]->pages[address%1024];
  } else {
    return 0;
  }
}

void page_fault(registers_t regs)
{

  // faulting address is in cr2
  u32int faulting_address = 0;
  asm volatile("mov %%cr2, %0" : "=r"(faulting_address));
  
  int present = !(regs.err_code & 0x1); // page not present?
  int rw = regs.err_code & 0x2;         // write = 0, read = 1
  int us = regs.err_code & 0x4;         // user mode?
  int reserved = regs.err_code & 0x8;   // reserved bits overwritten?
  int id = regs.err_code & 0x10;        // caused by an instruction fetch?

  // for now, just output an error message
  monitor_write("Page fault! ( ");
  if(present) {
    monitor_write("present ");
  }
  if(rw) {
    monitor_write("read-only ");
  }
  if(us) {
    monitor_write("user mode ");
  }
  if(reserved) {
    monitor_write("reserved ");
  }
  monitor_write(") at ");
  monitor_write_hex(faulting_address);
  monitor_write("\n");
  PANIC("Page fault!");
}

