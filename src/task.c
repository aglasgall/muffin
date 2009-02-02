#include "paging.h"
#include "monitor.h"
#include "task.h"
#include "kheap.h"

#define PAGE_SIZE 0x1000
#define PTR_SIZE 4

#define KERNEL_STACK 0xE0000000
#define KERNEL_STACK_SIZE 0x2000

#define TASK_SWITCHED_COOKIE 0xDEADBEEF
// imported from main.c
extern u32int initial_esp;

/// imported from process.s
extern u32int read_eip();
extern void do_switch_task(u32int eip, u32int esp, u32int ebp, u32int cr3);
// current running task
volatile task_t *current_task;

// head of the task list
volatile task_t *ready_queue;

// imported from paging.c
extern page_directory_t *kernel_directory;
extern page_directory_t *current_directory;

pid_t next_pid = 1;

void initialize_tasking() 
{
  // disable interrupts
  asm volatile("cli");
  
  // move the kernel stack so we know where it is
  move_stack((void*)KERNEL_STACK, KERNEL_STACK_SIZE);
  
  // initialize the first task (kernel)
  current_task = ready_queue = (task_t*)kmalloc(sizeof(task_t));
  memset((void*)current_task,0,sizeof(current_task));
  current_task->id = next_pid++;
  current_task->esp = current_task->ebp = 0;
  current_task->eip = 0;
  current_task->page_directory = current_directory;
  current_task->next = 0;

  // we've moved the stack, re-enable interrupts
  asm volatile("sti");
}

pid_t fork()
{
  asm volatile("cli");

  task_t *parent_task = (task_t*)current_task;
  
  page_directory_t *directory = clone_directory(current_directory);
  
  task_t *new_task = (task_t*)kmalloc(sizeof(task_t));
  memset(new_task,0,sizeof(new_task));
  
  new_task->id = next_pid++;
  new_task->esp = new_task->ebp = 0;
  new_task->eip = 0;
  new_task->page_directory = directory;
  new_task->next = 0;


  // walk the task list to find the end and add the task
  task_t *ptr = (task_t*)ready_queue;
  while(ptr->next) {
    ptr = ptr->next;
  }
  ptr->next = new_task;

  // where the new task should start
  pid_t eip = read_eip();
  
  // if we're in the parent
  if(current_task == parent_task) {
    u32int esp = 0; 
    asm volatile("mov %%esp, %0" : "=r"(esp));
    u32int ebp = 0; 
    asm volatile("mov %%ebp, %0" : "=r"(ebp));
    new_task->esp = esp;
    new_task->ebp = ebp;
    new_task->eip = eip;
    // we're all done, re-enable interrupts
    asm volatile("sti");
    return new_task->id;
  } else {
    // it's afterwards, and we're in the child now
    // we're the child
    return 0;
  }
}

void switch_task()
{
  // just return if tasking isn't on yet
  if(!current_task) {
    return;
  }
  // read stack and base pointers for saving later
  u32int esp, ebp, eip = 0;
  asm volatile("mov %%esp, %0" : "=r"(esp));
  asm volatile("mov %%ebp, %0" : "=r"(ebp)); 

  eip = read_eip();
  
  // if it's after the task switch and we're in a different task now
  if(eip == TASK_SWITCHED_COOKIE) {
    return;
  }
  // if we got here, we're still before the task switch
  current_task->eip = eip;
  current_task->esp = esp;
  current_task->ebp = ebp;

  // get the next task to run
  current_task = current_task->next;
  if(!current_task) {
    current_task = ready_queue;
  }
  eip = current_task->eip;
  esp = current_task->esp;
  ebp = current_task->ebp;

  current_directory = current_task->page_directory;
  // okay, now do the actual task switch
  do_switch_task(eip, esp, ebp, current_directory->physicalAddr);
}

pid_t getpid() {  
  return current_task->id;
}

void move_stack(void *new_stack_start, u32int size)
{
  u32int i = 0;
  // allocate some space for the new stack
  for(i = (u32int)new_stack_start;
      i >= ((u32int)new_stack_start-size);
      i -= PAGE_SIZE) {

    alloc_frame(get_page(i,1,current_directory),0,1);
  }
  // flush tlb
  u32int pd_addr = 0;
  asm volatile("mov %%cr3, %0" : "=r"(pd_addr));
  asm volatile("mov %0, %%cr3" : : "r"(pd_addr));
  
  // Old ESP and EBP, read from registers.
  u32int old_stack_pointer = 0; 
  asm volatile("mov %%esp, %0" : "=r" (old_stack_pointer));
  u32int old_base_pointer = 0;  
  asm volatile("mov %%ebp, %0" : "=r" (old_base_pointer)); 

  u32int offset = (u32int)new_stack_start - initial_esp;

  u32int new_stack_pointer = old_stack_pointer + offset;
  u32int new_base_pointer = old_base_pointer + offset;

  // copy the stack
  memcpy((void*)new_stack_pointer, 
	 (void*)old_stack_pointer, initial_esp-old_stack_pointer);

  // update pointers on the stack into the stack
  for(i = (u32int)new_stack_start; 
      i > (u32int)new_stack_start-size; i -= PTR_SIZE) {
    
    u32int tmp = *(u32int*)i;
    // if the value is in stack range...
    if((old_stack_pointer < tmp) && (tmp < initial_esp)) {
      tmp += offset;
      u32int *tmp2 = (u32int*)i;
      *tmp2 = tmp;
    }
  }
  
  // change stacks
  asm volatile("mov %0, %%esp" : : "r" (new_stack_pointer));
  asm volatile("mov %0, %%ebp" : : "r" (new_base_pointer));
}
