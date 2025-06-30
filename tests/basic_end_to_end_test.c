/**
 * basic_end_to_end_test.c - 基础端到端编译流程测试
 */

#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("=== Basic End-to-End Test ===\n");
    printf("Testing basic C functionality\n\n");
    
    // 测试基本算术
    printf("=== Testing Arithmetic ===\n");
    int a = 10;
    int b = 20;
    int sum = a + b;
    printf("a = %d, b = %d\n", a, b);
    printf("sum = %d\n", sum);
    
    if (sum == 30) {
        printf("Arithmetic test: PASSED\n");
    } else {
        printf("Arithmetic test: FAILED\n");
    }
    printf("\n");
    
    // 测试数组
    printf("=== Testing Arrays ===\n");
    int numbers[3];
    numbers[0] = 1;
    numbers[1] = 2;
    numbers[2] = 3;
    
    printf("Array elements: ");
    for (int i = 0; i < 3; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    
    int array_sum = numbers[0] + numbers[1] + numbers[2];
    printf("Array sum: %d\n", array_sum);
    
    if (array_sum == 6) {
        printf("Array test: PASSED\n");
    } else {
        printf("Array test: FAILED\n");
    }
    printf("\n");
    
    // 测试动态内存
    printf("=== Testing Dynamic Memory ===\n");
    int* ptr = (int*)malloc(sizeof(int));
    if (ptr != NULL) {
        *ptr = 42;
        printf("Dynamic value: %d\n", *ptr);
        free(ptr);
        printf("Memory test: PASSED\n");
    } else {
        printf("Memory test: FAILED\n");
    }
    printf("\n");
    
    // 测试控制流
    printf("=== Testing Control Flow ===\n");
    int x = 15;
    if (x > 10) {
        printf("x is greater than 10\n");
        printf("Control flow test: PASSED\n");
    } else {
        printf("Control flow test: FAILED\n");
    }
    printf("\n");
    
    // 测试循环
    printf("=== Testing Loops ===\n");
    int loop_sum = 0;
    for (int i = 1; i <= 5; i++) {
        loop_sum = loop_sum + i;
    }
    printf("Loop sum (1+2+3+4+5): %d\n", loop_sum);
    
    if (loop_sum == 15) {
        printf("Loop test: PASSED\n");
    } else {
        printf("Loop test: FAILED\n");
    }
    printf("\n");
    
    printf("=== Test Summary ===\n");
    printf("Basic end-to-end compilation test completed\n");
    printf("If you see this message, the toolchain is working!\n");
    
    return 0;
}
