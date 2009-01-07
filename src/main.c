#include "monitor.h"
#include "descriptor_tables.h"
#include "timer.h"

int main(struct multiboot *mboot_ptr)
{
  // initialization code goes here
  
  init_descriptor_tables();

  monitor_clear();
  monitor_write("Hello, world!\n");
  monitor_write_hex(0xDEADBEEF);
  monitor_put('\n');
  asm volatile ("int $0x3");
  asm volatile ("int $0x4");
  init_timer(50);
  asm volatile ("sti");

  return 0xDEADBABA;
}

