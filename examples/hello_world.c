/**
 * hello_world.c - 简单的Hello World程序
 * 
 * 这是一个用于测试三层架构的简单C程序。
 * 将被编译为ASTC字节码，然后通过simple_loader执行。
 */

#include <stdio.h>

int main(void) {
    printf("Hello, World from ASTC!\n");
    printf("This program is running in the three-layer architecture:\n");
    printf("  Layer 1: simple_loader\n");
    printf("  Layer 2: vm_*.native\n");
    printf("  Layer 3: hello_world.astc (this program)\n");
    return 0;
}
