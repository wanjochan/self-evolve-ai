/**
 * simplified_end_to_end_test.c - 简化的端到端编译流程测试
 * 
 * 测试核心C功能的编译和执行，验证工具链稳定性
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 测试基本算术
int test_arithmetic() {
    printf("=== Testing Arithmetic ===\n");
    
    int a = 10;
    int b = 20;
    int sum = a + b;
    int diff = b - a;
    int product = a * b;
    int quotient = b / a;
    
    printf("a = %d, b = %d\n", a, b);
    printf("sum = %d\n", sum);
    printf("diff = %d\n", diff);
    printf("product = %d\n", product);
    printf("quotient = %d\n", quotient);
    
    return (sum == 30 && diff == 10 && product == 200 && quotient == 2) ? 1 : 0;
}

// 测试数组
int test_arrays() {
    printf("=== Testing Arrays ===\n");
    
    int numbers[5];
    numbers[0] = 1;
    numbers[1] = 2;
    numbers[2] = 3;
    numbers[3] = 4;
    numbers[4] = 5;
    
    printf("Array elements: ");
    int sum = 0;
    for (int i = 0; i < 5; i++) {
        printf("%d ", numbers[i]);
        sum = sum + numbers[i];
    }
    printf("\n");
    printf("Sum: %d\n", sum);
    
    return (sum == 15) ? 1 : 0;
}

// 测试字符串
int test_strings() {
    printf("=== Testing Strings ===\n");
    
    char message[32];
    message[0] = 'H';
    message[1] = 'e';
    message[2] = 'l';
    message[3] = 'l';
    message[4] = 'o';
    message[5] = '\0';
    
    printf("Message: %s\n", message);
    int len = strlen(message);
    printf("Length: %d\n", len);
    
    return (len == 5) ? 1 : 0;
}

// 测试控制流
int test_control_flow() {
    printf("=== Testing Control Flow ===\n");
    
    int result = 0;
    
    // 测试if-else
    int x = 42;
    if (x > 50) {
        result = 1;
    } else if (x > 30) {
        result = 2;
    } else {
        result = 3;
    }
    printf("If-else result: %d\n", result);
    
    // 测试for循环
    int count = 0;
    for (int i = 1; i <= 5; i++) {
        count = count + i;
    }
    printf("For loop sum: %d\n", count);
    
    // 测试while循环
    int j = 0;
    int while_sum = 0;
    while (j < 3) {
        while_sum = while_sum + j;
        j = j + 1;
    }
    printf("While loop sum: %d\n", while_sum);
    
    return (result == 2 && count == 15 && while_sum == 3) ? 1 : 0;
}

// 测试动态内存
int test_dynamic_memory() {
    printf("=== Testing Dynamic Memory ===\n");
    
    int* ptr = (int*)malloc(sizeof(int) * 5);
    if (ptr == NULL) {
        printf("Memory allocation failed\n");
        return 0;
    }
    
    // 初始化数组
    for (int i = 0; i < 5; i++) {
        ptr[i] = i * 2;
    }
    
    printf("Dynamic array: ");
    int sum = 0;
    for (int i = 0; i < 5; i++) {
        printf("%d ", ptr[i]);
        sum = sum + ptr[i];
    }
    printf("\n");
    printf("Sum: %d\n", sum);
    
    free(ptr);
    printf("Memory freed\n");
    
    return (sum == 20) ? 1 : 0;
}

// 测试函数调用
int add_numbers(int x, int y) {
    return x + y;
}

int test_function_calls() {
    printf("=== Testing Function Calls ===\n");
    
    int result1 = add_numbers(10, 20);
    int result2 = add_numbers(5, 15);
    
    printf("add_numbers(10, 20) = %d\n", result1);
    printf("add_numbers(5, 15) = %d\n", result2);
    
    return (result1 == 30 && result2 == 20) ? 1 : 0;
}

// 主函数
int main() {
    printf("=== Simplified End-to-End Test ===\n");
    printf("Testing core C functionality compilation and execution\n\n");
    
    int tests_passed = 0;
    int total_tests = 6;
    
    if (test_arithmetic()) {
        printf("✅ Arithmetic test: PASSED\n");
        tests_passed = tests_passed + 1;
    } else {
        printf("❌ Arithmetic test: FAILED\n");
    }
    printf("\n");
    
    if (test_arrays()) {
        printf("✅ Arrays test: PASSED\n");
        tests_passed = tests_passed + 1;
    } else {
        printf("❌ Arrays test: FAILED\n");
    }
    printf("\n");
    
    if (test_strings()) {
        printf("✅ Strings test: PASSED\n");
        tests_passed = tests_passed + 1;
    } else {
        printf("❌ Strings test: FAILED\n");
    }
    printf("\n");
    
    if (test_control_flow()) {
        printf("✅ Control flow test: PASSED\n");
        tests_passed = tests_passed + 1;
    } else {
        printf("❌ Control flow test: FAILED\n");
    }
    printf("\n");
    
    if (test_dynamic_memory()) {
        printf("✅ Dynamic memory test: PASSED\n");
        tests_passed = tests_passed + 1;
    } else {
        printf("❌ Dynamic memory test: FAILED\n");
    }
    printf("\n");
    
    if (test_function_calls()) {
        printf("✅ Function calls test: PASSED\n");
        tests_passed = tests_passed + 1;
    } else {
        printf("❌ Function calls test: FAILED\n");
    }
    printf("\n");
    
    printf("=== Test Results ===\n");
    printf("Tests passed: %d/%d\n", tests_passed, total_tests);
    
    if (tests_passed == total_tests) {
        printf("🎉 All tests passed! End-to-end compilation successful!\n");
        return 0;
    } else {
        printf("⚠️ Some tests failed. Toolchain needs improvement.\n");
        return 1;
    }
}
