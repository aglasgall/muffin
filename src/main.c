#include "monitor.h"

int main(struct multiboot *mboot_ptr)
{
  // initialization code goes here

   monitor_clear();
   monitor_write("Hello, world!");
   monitor_put('\n');
   monitor_write_hex(0xDEADBEEF);
  return 0xDEADBABA;
}

