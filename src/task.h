#ifndef _TASK_H_
#define _TASK_H_

#include "common.h"
#include "paging.h"

typedef s32int pid_t;

// represents a process
typedef struct task {
  pid_t id; // process id
  u32int esp, ebp; // stack and base ptrs
  u32int eip; // instruction pointer
  page_directory_t *page_directory;
  u32int state; // task state - runnable, sleeping, etc
  u32int quantum; // number of ticks remaining to the process
  struct task *next; // next task on the list
} task_t;

void initialize_tasking();

void switch_task();

pid_t fork();

// copy the current process's stack to a new location and update esp/ebp
void move_stack(void *new_stack_start, u32int size);

pid_t getpid(); 

#endif
