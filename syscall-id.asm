;nasm -f elf64 syscall-id.asm 
;ld -m elf_x86_64 -s -o syscall-id syscall-id.o 
; runs /bin/id

section .text
    global _start

_start:

    xor rdx, rdx
    push rdx
    mov rax, 0x64692f6e69622f
    push rax
    mov rdi, rsp
    push rdx
    push rdi
    mov rsi, rsp
    xor rax, rax
    mov al, 0x3b
    syscall
