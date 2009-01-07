#ifndef _MONITOR_H_
#define _MONITOR_H_

#include "common.h"

#define PANIC(message) panic(message,__FILE__,__LINE__)

/* write a single character to the screen */
void monitor_put(char c);

/* clear the screen */
void monitor_clear();

/* write a null-terminated string to the screen */

void monitor_write (char* c);

void monitor_write_hex(u32int num);

void panic(char* message, char *file, int line);

#endif
