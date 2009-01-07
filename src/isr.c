#include "common.h"
#include "isr.h"
#include "monitor.h"

isr_t interrupt_handlers[256] = {0};

void isr_handler(registers_t regs)
{
  if(interrupt_handlers[regs.int_no]) {
    isr_t handler = interrupt_handlers[regs.int_no];
    handler(regs);
  } else {
    monitor_write("received interrupt: ");
    monitor_write_hex(regs.int_no);
    monitor_put('\n');
  }
}

void irq_handler(registers_t regs)
{
  // Send an end-of-interrupt signal to the PICs
  // if interrupt came from the slave PIC...
  if (regs.int_no >= 40) {
    // reset slave
    outb(0xA0, 0x20);
  }
  // reset master
  outb(0x20, 0x20);

  if(interrupt_handlers[regs.int_no]) {
    isr_t handler = interrupt_handlers[regs.int_no];
    handler(regs);
  }

}

void register_interrupt_handler(u8int n, isr_t handler)
{
  interrupt_handlers[n] = handler;
}
