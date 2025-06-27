# Cross-platform assembly for windows-x64
.intel_syntax noprefix
.text
.globl main
main:
    push rbp
    mov rbp, rsp
    mov rax, 42
    pop rbp
    ret
