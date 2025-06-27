# Cross-platform assembly for macos-x64
.text
.globl _main
_main:
    push rbp
    mov rbp, rsp
    mov rax, 42
    pop rbp
    ret
