	[BITS 32]
	[GLOBAL copy_page_physical]
	[GLOBAL read_eip]
	[GLOBAL do_switch_task]
	
copy_page_physical:    
	push ebx 		; C calling convention demands ebx be preserved
	pushf 			; save previous interrupt flag state

	cli 			; disable interrupts

	mov esi, [esp+12] 	; source address
	mov edi, [esp+16] 	; destination address

	mov edx, cr0 		; get control register...
	and edx, 0x7fffffff 	; ...disable paging...
	mov cr0, edx 		; ... and store it back

	mov ecx, 1024 		; 1024 words * 4 bytes a word = 1 page (4096)

	rep movsd

	mov ecx, cr0 		; re-enable paging
	or edx, 0x80000000
	mov cr0, edx
	popf 			; re enable interrupts if they were enabled
	pop ebx 		; restore ebx
	ret

	
read_eip:
	mov eax, [esp]
	ret

do_switch_task:
	cli
	pop ecx 		; throw away return address
	pop ecx 		; first parameter is the new eip
	pop esi 		; second parameter is new esp, store in esi temporarily
	pop edi 		; third is new ebp, store in edi temporarily
	pop edx 		; fourth is new cr3, but we can't pop
	mov esp, esi
	mov ebp, edi
	mov cr3, edx 		; directly into that
	mov eax, 0xdeadbeef 	; magic cookie value
	sti
	jmp ecx
	