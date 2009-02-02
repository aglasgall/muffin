#ifndef _PAGING_H_
#define _PAGING_H_

#include "common.h"
#include "isr.h"

typedef struct page {
  u32int present:1;  // page present in memory?
  u32int rw:1;       // read-only if 0, rw if 1
  u32int user:1;     // kernel if 0, user if 1
  u32int accessed:1; // page accessed since last refresh?
  u32int dirty:1;    // page written to since last refresh?
  u32int unused:7;   // unused and reserved bits
  u32int frame:20;   // page frame number (shifted right 12 bits), 
                     // since only the top 20 matter
} page_t;

typedef struct page_table {
  page_t pages[1024];
} page_table_t;


typedef struct page_directory {

  // pointers to page tables
  page_table_t *tables[1024];

  // physical addresses of page tables
  u32int tablesPhysical[1024];

  // physical address of tablesPhysical
  u32int physicalAddr;
} page_directory_t;


// set up environment and page tables and enable paging
void initialize_paging();


// load a page directory into CR3
void switch_page_directory(page_directory_t *new);

// clone a page directory
page_directory_t *clone_directory(page_directory_t *src);

// get a pointer to the page `address' lies in, creating the page's table
//  if make == 1 and the table doesn't exist yet
page_t *get_page(u32int address, int make, page_directory_t *dir);
// map in and free a page
void alloc_frame(page_t *page, int is_kernel, int is_writeable);
void free_frame(page_t *page);
// page fault handler
void page_fault(registers_t regs);

#endif
