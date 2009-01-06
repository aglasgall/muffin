#include "monitor.h"
#include "descriptor_tables.h"

int main(struct multiboot *mboot_ptr)
{
  // initialization code goes here
  
  init_descriptor_tables();

  monitor_clear();
  monitor_write("Hello, world!\m");
  monitor_write_hex(0xDEADBEEF);
  monitor_put('\n');
  asm volatile ("int $0x3");
  asm volatile ("int $0x4");
  return 0xDEADBABA;
}

