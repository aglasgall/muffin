#include "timer.h"
#include "isr.h"
#include "monitor.h"
#include "common.h"

#define PIT_FREQ 1193180
#define TIMER_COMMAND 0x43
#define TIMER_DATA 0x40

u32int tick = 0;

static void timer_callback(registers_t regs)
{
  tick++;
  monitor_write("Tick: ");
  monitor_write_hex(tick);
  monitor_put('\n');
}

void init_timer(u32int frequency)
{
  // register timer handler
  register_interrupt_handler(IRQ0, timer_callback);

  // nb that frequency must fit into 16 bits
  u32int divisor = PIT_FREQ / frequency;

  // tell the timer we're going to set our desired frequency
  outb(TIMER_COMMAND, 0x36);

  // split divisor into low and high bytes
  u8int l = (u8int)(divisor & 0xFF);
  u8int h = (u8int)((divisor >> 8) & 0xFF);
  
  // send desired frequency
  outb(TIMER_DATA, l);
  outb(TIMER_DATA,h);
}

