SOURCES=boot.o main.o monitor.o common.o

CFLAGS= -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Wall -std=gnu99 -g -O0
LDFLAGS=-Tlink.ld -melf_i386
ASFLAGS=-felf

all: $(SOURCES) link

clean:
	rm *.o kernel

link:
	ld $(LDFLAGS) -o kernel $(SOURCES)

.s.o:
	nasm $(ASFLAGS) $<
