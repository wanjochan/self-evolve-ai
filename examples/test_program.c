/**
 * test_program.c - 测试程序
 * 
 * 一个更复杂的测试程序，用于验证三层架构的功能。
 */

#include <stdio.h>

// 简单的数学函数
int add(int a, int b) {
    return a + b;
}

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main(int argc, char* argv[]) {
    printf("ASTC Test Program\n");
    printf("================\n");
    
    // 测试基本运算
    int x = 10;
    int y = 20;
    int sum = add(x, y);
    
    printf("Basic arithmetic test:\n");
    printf("  %d + %d = %d\n", x, y, sum);
    
    // 测试递归函数
    int n = 5;
    int fact = factorial(n);
    printf("Factorial test:\n");
    printf("  %d! = %d\n", n, fact);
    
    // 测试命令行参数
    printf("Command line arguments:\n");
    printf("  argc = %d\n", argc);
    for (int i = 0; i < argc; i++) {
        printf("  argv[%d] = %s\n", i, argv[i]);
    }
    
    printf("\nThree-layer architecture test completed successfully!\n");
    return 0;
}
