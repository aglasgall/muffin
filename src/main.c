#include "monitor.h"

int main(struct multiboot *mboot_ptr)
{
  // initialization code goes here

   monitor_clear();
   monitor_write("Hello, world!");

  return 0xDEADBABA;
}

