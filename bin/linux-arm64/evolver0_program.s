# Cross-platform assembly for linux-arm64
.text
.globl _start
.globl main
main:
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    mov w0, #42
    ldp x29, x30, [sp], #16
    ret
