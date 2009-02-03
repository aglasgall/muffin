	[BITS 32]
	[GLOBAL spinlock_try_take]
	[GLOBAL spinlock_release]
spinlock_try_take:
	xor eax, eax
	mov edx, [esp+4]
	lock xchg eax, [edx]
	ret
	
spinlock_release:
	mov eax, 0x1
	mov edx, [esp+4]
	mov [edx], eax
	ret