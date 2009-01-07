#include "common.h"

/* output `value' to `port' */
void outb(u16int port, u8int value)
{
  asm volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}


/* read a byte from `port' and return it */
u8int inb(u16int port)
{
  u8int ret = 0;
  asm volatile("inb %1, %0" : "=a" (ret) : "dN" (port));
  return ret;
}

/* read a word from `port' and return it */
u16int inw(u16int port)
{
  u16int ret = 0;
  asm volatile("inw %1, %0" : "=a" (ret) : "dN" (port));
  return ret;
}

void *memset(void *s, int c, size_t count)
{
  char *ptr = s;
  
  while (count--) {
    *ptr++ = c;
  }
  return s;
}
