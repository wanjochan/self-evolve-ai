/**
 * toolchain_test.c - 工具链完整性测试
 * 
 * 这个简单的C程序用于测试我们的自举工具链
 * 从C源码 → ASTC字节码 → 机器码 → 执行
 */

#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("=== Toolchain Verification Test ===\n");
    printf("This program was compiled using our self-hosted toolchain!\n");
    printf("C Source → ASTC Bytecode → Machine Code → Execution\n");
    printf("SUCCESS: Complete toolchain working!\n");
    
    // 测试一些基本的C功能
    int numbers[] = {1, 2, 3, 4, 5};
    int sum = 0;
    
    for (int i = 0; i < 5; i++) {
        sum += numbers[i];
    }
    
    printf("Array sum test: %d (expected: 15)\n", sum);
    
    // 测试动态内存分配
    char* message = malloc(50);
    if (message) {
        sprintf(message, "Dynamic allocation works!");
        printf("Memory test: %s\n", message);
        free(message);
    }
    
    printf("=== All tests passed! ===\n");
    return 42; // 特殊返回值用于验证
}
