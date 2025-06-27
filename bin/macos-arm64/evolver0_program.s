# Cross-platform assembly for macos-arm64
.text
.globl _main
_main:
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    mov w0, #42
    ldp x29, x30, [sp], #16
    ret
