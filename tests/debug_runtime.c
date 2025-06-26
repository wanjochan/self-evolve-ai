/**
 * debug_runtime.c - 调试Runtime执行
 * 测试Runtime是否能正确执行简单的ASTC程序
 */

#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Debug: Runtime test starting\n");
    
    int result = 42;
    printf("Debug: Setting result to %d\n", result);
    
    printf("Debug: Runtime test completed\n");
    return result;
}
