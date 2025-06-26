/**
 * test_self_bootstrap.c - 测试真正的自举编译功能
 * 
 * 这个测试验证evolver0是否能够：
 * 1. 编译任意C程序（不仅仅是返回固定值）
 * 2. 实现真正的自举编译
 * 3. 生成可工作的evolver1
 */

#include <stdio.h>
#include <stdlib.h>

// 简单的测试函数
int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    printf("=== 自举编译测试程序 ===\n");
    
    // 测试递归函数
    int fib_result = fibonacci(5);  // 应该返回5
    printf("fibonacci(5) = %d\n", fib_result);
    
    int fact_result = factorial(4); // 应该返回24
    printf("factorial(4) = %d\n", fact_result);
    
    // 测试循环
    int sum = 0;
    for (int i = 1; i <= 5; i++) {
        sum += i;
    }
    printf("sum(1..5) = %d\n", sum); // 应该返回15
    
    // 测试条件语句
    int max = (fib_result > fact_result) ? fib_result : fact_result;
    printf("max(fib, fact) = %d\n", max);
    
    // 返回一个可验证的结果
    return fib_result + fact_result + sum + max; // 5 + 24 + 15 + 24 = 68
}
