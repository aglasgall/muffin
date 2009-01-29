#ifndef _MONITOR_H_
#define _MONITOR_H_

#include "common.h"

#define PANIC(message) panic(message,__FILE__,__LINE__)
#define ASSERT(b) ((b) ? ((void)0) : panic_assert(__FILE__,__LINE__,#b))

/* write a single character to the screen */
void monitor_put(char c);

/* clear the screen */
void monitor_clear();

/* write a null-terminated string to the screen */

void monitor_write (char* c);

void monitor_write_hex(u32int num);

void panic(const char* message, const char *file, u32int line);

void panic_assert(const char* file, u32int line, const char* desc);

#endif
