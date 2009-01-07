#include "monitor.h"
#include "descriptor_tables.h"
#include "timer.h"

int main(struct multiboot *mboot_ptr)
{
  // initialization code goes here
  
  init_descriptor_tables();

  monitor_clear();

  initialize_paging();
  monitor_write("Hello, paged world!\n");
  monitor_write_hex(0xDEADBEEF);
  monitor_put('\n');

  u32int *ptr = (u32int*)0xA0000000;
  u32int do_page_fault = *ptr;

  return 0xDEADBABA;
}

