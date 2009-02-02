	;; 
	;; boot.s - kernel start location + multiboot header
	;; -*- asm-mode -*-
	
	;; constants for mboot descriptor structure
	MBOOT_PAGE_ALIGN	equ	1<<0		; load kernel at a page boundary
	MBOOT_MEM_INFO		equ	1 << 1		; loader should provide kernel with memory info
	MBOOT_HEADER_MAGIC	equ	0x1BADB002 ; magic
	MBOOT_HEADER_FLAGS	equ	MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
	MBOOT_CHECKSUM		equ	-(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

	;;  nasm directives
	[BITS 32]		; 32-bit instructions

	[GLOBAL mboot]	; export mboot section for grub + C code to find
	[EXTERN code]	; text section is defined externally
	[EXTERN bss]	; ditto bss
	[EXTERN end]	; ditto ditto last section

	;; mboot header structure
mboot:
	dd MBOOT_HEADER_MAGIC 	; header
	dd MBOOT_HEADER_FLAGS
	dd MBOOT_CHECKSUM

	dd mboot		; mboot descriptor address
	dd code	 		; start of .text section
	dd bss 			; end of .data section
	dd end			; end of kernel
	dd start 		; kernel entry point

	
	[GLOBAL start] 		; kernel entry point
	[EXTERN main] 		; main is defined in C

start:
	push esp 		; save the initial stack pointer
	push ebx 		; bootloader puts a ptr to mboot information structure in ebx

	;; kick off kernel loading
	cli			; disable interrupts
	call main 		; call kernel main()
	jmp $ 			; enter an infinite loop to keep the cpu from marching off the end

	