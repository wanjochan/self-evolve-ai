# 独立C编译器生成的汇编代码
.text
.globl _start
_start:
main:
    mov $42, %eax
    mov $1, %ebx
    int $0x80
