global FILL_PIXELS_ASM

section .text

FILL_PIXELS_ASM:
	xor rax, rax ; pixel index
.loop:
	mov[rdi + rax*4], edx ; move color into buffer
	inc rax
	cmp rax, rsi
	jb .loop
	ret
