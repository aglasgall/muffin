#ifndef _KHEAP_H_
#define _KHEAP_H_

#include "common.h"
#include "ordered_array.h"

#define KHEAP_START         0xC0000000
#define KHEAP_INITIAL_SIZE  0x100000
#define HEAP_INDEX_SIZE   0x20000
#define HEAP_MAGIC        0x123890AB
#define HEAP_MIN_SIZE     0x70000

/* size information for a hole/block */
typedef struct header_struct {
  u32int magic; // magic number for corruption prevention
  u8int is_hole; // 1 if hole, 0 if block
  u32int size; // size of block, including footer
} header_t;

typedef struct footer_struct {
  u32int magic; // same as header_t
  header_t *header; // pointer to hole/block header
} footer_t;

typedef struct heap_struct {
  ordered_array_t index;
  u32int start_address; // start of heap
  u32int end_address; // end of currently allocated space, may be expanded to...
  u32int max_address; // max address heap can be expanded to
  u8int supervisor; // map pages from this heap as kernel?
  u8int readonly; // map pages from this heap as read-only?
} heap_t;

// create a new heap. start and end must be page aligned.
heap_t *create_heap(u32int start, u32int end, u32int max, u8int supervisor, u8int readonly);

// allocate a contiguous region memory of memory, page-aligned if page_align = 1

void *alloc(u32int size, u8int page_align, heap_t *heap);

// free a block allocated with alloc
void free(void *p, heap_t *heap);

u32int kmalloc_a(u32int sz);  // page aligned.
u32int kmalloc_p(u32int sz, u32int *phys); // returns a physical address.
u32int kmalloc_ap(u32int sz, u32int *phys); // page aligned and returns a physical address.
u32int kmalloc(u32int sz); // vanilla (normal).
void kfree(void *p);


#endif
