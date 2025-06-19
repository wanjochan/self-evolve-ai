.global _start
.text
_start:
    mov $60, %rax    # exit系统调用号
    mov $42, %rdi    # 退出码
    syscall