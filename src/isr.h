#ifndef _REGISTERS_H_
#define _REGISTERS_H_

#include "common.h"

typedef struct registers {
  u32int ds; // data segment
  u32int edi, esi, ebp, esp, ebx, edx, ecx, eax; // pushed by pusha
  u32int int_no, err_code; // interrupt # and error code
  u32int eip, cs, eflags, useresp, ss; // pushed by cpu in response to interrupt
} registers_t;

#endif

