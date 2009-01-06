#include "common.h"
#include "descriptor_tables.h"

#define LIMIT 0xFFFFFFFF

// prototypes for functions implemented in asm
extern void gdt_flush(u32int);

static void init_gdt();
static void gdt_set_gate(s32int num, u32int base, u32int limit, u8int access, u8int gran);

// global structures for the cpu
gdt_entry_t gdt_entries[5];
gdt_ptr_t gdt_ptr = {0};

void init_descriptor_tables()
{
  init_gdt();
}

static void init_gdt()
{
  gdt_ptr.limit = (sizeof(gdt_entry_t)*5)-1;
  // forgive me
  gdt_ptr.base = (u32int)&gdt_entries;

  gdt_set_gate(0,0,0,0,0); //null segment
  gdt_set_gate(1,0,LIMIT,0x9A,0xCF); // kernel code segment
  gdt_set_gate(2,0,LIMIT,0x92,0xCF); // kernel data segment
  gdt_set_gate(3,0,LIMIT,0xFA,0xCF); // user code segment
  gdt_set_gate(4,0,LIMIT,0xF2,0xCF); // user data segment

  gdt_flush((u32int)&gdt_ptr);
}

static void gdt_set_gate(s32int num, u32int base, u32int limit, u8int access, u8int gran)
{
  gdt_entry_t* entry = gdt_entries+num;
  
  entry->base_low = (base & 0xFFFF);
  entry->base_middle = (base >> 16) & 0xFF;
  entry->base_high = (base >> 24) & 0xFF;
 
  entry->limit_low = (limit & 0xFFFF);
  entry->granularity = (limit >> 16) & 0x0F;

  entry->granularity |= gran & 0xF0;
  entry->access = access;
}
