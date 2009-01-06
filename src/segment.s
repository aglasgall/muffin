	[BITS 32]		; 32-bit instructions
	[GLOBAL gdt_flush] 	; make gdt_flush callable from C

gdt_flush:
	mov eax, [esp+4]	; pointer to new gdt is in esp+4
	lgdt [eax]		; load new gdt

	mov ax, 0x10		; kernel data segment is the third descriptor
	mov ds, ax		; load all data segment selectors
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	jmp 0x08:.flush		; code segment is at gdtr+8
.flush:
	ret