/**
 * minimal_end_to_end_test.c - 最小端到端编译流程测试
 */

#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Minimal End-to-End Test\n");
    
    // 测试基本算术
    int a = 10;
    int b = 20;
    int sum = a + b;
    
    printf("Testing arithmetic\n");
    printf("Sum result: ");
    
    if (sum == 30) {
        printf("PASSED\n");
    } else {
        printf("FAILED\n");
    }
    
    // 测试动态内存
    printf("Testing memory\n");
    int* ptr = (int*)malloc(4);
    if (ptr != NULL) {
        printf("Memory allocation: PASSED\n");
        free(ptr);
    } else {
        printf("Memory allocation: FAILED\n");
    }
    
    printf("Test completed\n");
    return 0;
}
