#ifndef _MULTIBOOT_H_
#define _MULTIBOOT_H_

#include "common.h"

#define MULTIBOOT_FLAG_MEM     0x001
#define MULTIBOOT_FLAG_DEVICE  0x002
#define MULTIBOOT_FLAG_CMDLINE 0x004
#define MULTIBOOT_FLAG_MODS    0x008
#define MULTIBOOT_FLAG_AOUT    0x010
#define MULTIBOOT_FLAG_ELF     0x020
#define MULTIBOOT_FLAG_MMAP    0x040
#define MULTIBOOT_FLAG_CONFIG  0x080
#define MULTIBOOT_FLAG_LOADER  0x100
#define MULTIBOOT_FLAG_APM     0x200
#define MULTIBOOT_FLAG_VBE     0x400


struct multiboot_module {
  u32int mod_start;
  u32int mod_end;
  char *string;
  u32int reserved;
} __attribute__((packed));

typedef struct multiboot_module multiboot_module_t;


struct multiboot_memory_map_entry {
  u32int size;
  u64int base_addr;
// You can also use: unsigned long long int base_addr; if supported.
  u64int length;
// You can also use: unsigned long long int length; if supported.
  u32int type;
} __attribute__((packed));
typedef struct multiboot_memory_map_entry multiboot_mmap_entry_t;


struct multiboot {
  u32int flags;
  u32int mem_lower;
  u32int mem_upper;
  // boot device
  u8int drive_number;
  u8int part_number;
  u8int subpart_number;
  u8int unused;
  char *cmdline;
  u32int mods_count;
  multiboot_module_t *mods_addr;
  u32int num;
  u32int size;
  u32int addr;
  u32int shndx;
  u32int mmap_length;
  multiboot_mmap_entry_t *mmap_addr;
  // this information is useless, so forget it.
  // cylinders, heads, and sectors?!
  u32int drives_length;
  u32int drives_addr;
  u32int config_table;
  char *boot_loader_name;
  u32int apm_table;
  u32int vbe_control_info;
  u32int vbe_mode_info;
  u32int vbe_mode;
  u32int vbe_interface_seg;
  u32int vbe_interface_off;
  u32int vbe_interface_len;
}  __attribute__((packed));

typedef struct multiboot_header multiboot_header_t; 
#endif
