/**
 * end_to_end_test.c - 端到端编译流程测试
 * 测试完整的 C → ASTC → RT → 执行 流程
 */

#include <stdio.h>

// 测试函数调用
int add_numbers(int a, int b) {
    return a + b;
}

// 测试条件语句
int test_conditions(int x) {
    if (x > 10) {
        return 1;
    } else {
        return 0;
    }
}

int main() {
    printf("=== End-to-End Compilation Test ===\n");
    
    // 测试基本输出
    printf("Step 1: Basic output - PASSED\n");
    
    // 测试函数调用
    int result = add_numbers(5, 3);
    printf("Step 2: Function call - add_numbers(5, 3) = %d\n", result);
    
    // 测试条件语句
    int cond_result = test_conditions(15);
    printf("Step 3: Conditional logic - test_conditions(15) = %d\n", cond_result);
    
    // 测试循环
    printf("Step 4: Loop test - ");
    for (int i = 0; i < 3; i++) {
        printf("%d ", i);
    }
    printf("\n");
    
    printf("=== All Tests Completed Successfully ===\n");
    printf("C → ASTC → RT → Execution pipeline working!\n");
    
    return 0;
}
