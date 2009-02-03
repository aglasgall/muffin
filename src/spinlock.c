#include "spinlock.h"
#include "task.h"
#include "monitor.h"
extern volatile task_t *current_task;

void spinlock_init(spinlock_t *lock)
{
  *lock = 1;
}

/*u32int spinlock_try_take(spinlock_t *lock)
{
  asm volatile ("xor %eax, %eax");
  asm volatile ("mov %0, %%edx" :: "c"(lock));
  asm volatile ("xchg %eax, (%edx)");
  }*/
extern u32int spinlock_try_take(spinlock_t *lock);

void spinlock_acquire(spinlock_t *lock)
{
  u32int lock_acquired = 0;
  while(!(lock_acquired = spinlock_try_take(lock))) {
    if(current_task) {
      switch_task();
    } else {
      // spin
    }
  }
}

/* void spinlock_release(spinlock_t *lock) */
/* { */
/*   asm volatile("mov $0x1, %eax"); */
/*   asm volatile("mov %%eax, (%0)" : "=r"(lock)); */
/* } */
