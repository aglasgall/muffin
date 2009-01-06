#ifndef _MONITOR_H_
#define _MONITOR_H_

#include "common.h"

/* write a single character to the screen */
void monitor_put(char c);

/* clear the screen */
void monitor_clear();

/* write a null-terminated string to the screen */

void monitor_write (char* c);

#endif
