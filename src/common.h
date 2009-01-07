#ifndef _COMMON_H_
#define _COMMON_H_

/* of course, these are for 32-bit x86. */
typedef unsigned   int u32int;
typedef            int s32int;
typedef unsigned short u16int;
typedef          short s16int;
typedef unsigned char  u8int;
typedef          char  s8int;

typedef u32int size_t;

void outb(u16int port, u8int value);
u8int inb(u16int port);
u16int inw(u16int port);

void *memset(void *s, int c, size_t count);

#endif


