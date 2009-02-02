	[BITS 32]
	[GLOBAL memcpy]

memcpy:
	push ebx

	mov edi, [esp+4] 	; dest is 1st parameter
	mov esi, [esp+8] 	; src is 2nd parameter
	mov ecx, [esp+12] 	; length is 3rd parameter

	rep movsb 		; do the block move

	mov eax, [esp+4] 	; return = dest
	pop ebx
	ret
	