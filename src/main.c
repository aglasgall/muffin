#include "monitor.h"
#include "descriptor_tables.h"
#include "timer.h"
#include "kheap.h"
#include "paging.h"
#include "multiboot.h"
#include "task.h"

u32int initial_esp = 0;
struct multiboot *mboot_header = 0;

int main(struct multiboot *mboot_ptr, u32int initial_stack)
{
  // initialization code goes here
  initial_esp = initial_stack;
  mboot_header = mboot_ptr;
  init_descriptor_tables();

  monitor_init();
  monitor_clear();
  u32int a = kmalloc(8);
  initialize_paging();
  monitor_write("Hello, paged world!\n");
  monitor_write_hex(0xDEADBEEF);
  monitor_put('\n');
  
  monitor_write("testing heap allocation now\n");

  u32int b = kmalloc(8);
  u32int c = kmalloc(8);
  monitor_write("a: ");
  monitor_write_hex(a);
  monitor_write(", b: ");
  monitor_write_hex(b);
  monitor_write("\nc: ");
  monitor_write_hex(c);
  
  kfree(c);
  kfree(b);
  u32int d = kmalloc(12);
  monitor_write(", d: ");
  monitor_write_hex(d); 
  monitor_write("\n");
  // start multitasking
  initialize_tasking();
  asm volatile("sti");
   init_timer(50);
  pid_t ret = fork();
  pid_t my_pid = getpid();
  monitor_lock();
  monitor_write("fork() returned: ");
  monitor_write_hex(ret);
  switch_task();
  monitor_write(", and getpid() returned ");
  monitor_write_hex(my_pid);
  monitor_write("\n============================================================================\n");
  monitor_release();
  /* while(1) { */
  /*   monitor_lock(); */
  /*   monitor_write("Hello from process "); */
  /*   monitor_write_hex(my_pid); */
  /*   switch_task(); */
  /*   monitor_write(" !\n"); */
  /*   monitor_release(); */
  /* } */
  /* u32int *ptr = (u32int*)0xA0000000; */
  /* u32int do_page_fault = *ptr; */

  return 0xDEADBABA;
}

