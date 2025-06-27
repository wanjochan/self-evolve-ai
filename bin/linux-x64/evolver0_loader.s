# Cross-platform assembly for linux-x64
.text
.globl _start
.globl main
_start:
    call main
    mov rdi, rax
    mov rax, 60
    syscall
main:
    push rbp
    mov rbp, rsp
    mov rax, 42
    pop rbp
    ret
