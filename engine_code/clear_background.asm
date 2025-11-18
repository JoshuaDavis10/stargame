global FILL_PIXELS_ASM

section .text

; rdi is pixel_buffer
; rsi is number of bytes to write
; rdx is the color data (512 bits)

FILL_PIXELS_ASM:
	xor rax, rax ; pixel index
	align 64
.loop:
	vmovdqu ymm0, [rdx]
	vmovdqu [rdi + rax], ymm0
	add rax, 32
	cmp rax, rsi
	jb .loop
	ret
