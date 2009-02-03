SOURCES=boot.o main.o monitor.o common.o interrupt.o descriptor_tables.o segment.o isr.o timer.o kheap.o paging.o ordered_array.o process.o task.o spinlock.o locks.o

CFLAGS= -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Wall -std=gnu99 -g -O0
LDFLAGS=-Tlink.ld -melf_i386
ASFLAGS=-felf

all: $(SOURCES) link

clean:
	rm -f *.o kernel

link:
	ld $(LDFLAGS) -o kernel $(SOURCES)

.s.o:
	nasm $(ASFLAGS) $<

run:
	kvm -fda floppy.img

debug:
	kvm -fda floppy.img -s -S -monitor stdio
