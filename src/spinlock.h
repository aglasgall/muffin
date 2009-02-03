#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_
#include "common.h"

typedef u32int spinlock_t;

void spinlock_init(spinlock_t *lock);

u32int spinlock_take(spinlock_t *lock);
void spinlock_release(spinlock_t *lock);

#endif
