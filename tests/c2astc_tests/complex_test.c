/**
 * complex_test.c - 复杂测试用例
 * 用于测试c2astc模块的控制流解析功能
 */

// 包含标准库
#include <stdio.h>
#include <stdlib.h>

// 函数声明
int factorial(int n);
void print_array(int arr[], int size);
int fibonacci(int n);

/**
 * 主函数
 */
int main() {
    // 变量声明和初始化
    int i, j;
    int sum = 0;
    int numbers[5] = {1, 2, 3, 4, 5};
    
    // for循环
    for (i = 0; i < 5; i++) {
        sum += numbers[i];
    }
    printf("Sum: %d\n", sum);
    
    // while循环
    i = 0;
    while (i < 5) {
        printf("%d ", numbers[i]);
        i++;
    }
    printf("\n");
    
    // if-else语句
    if (sum > 10) {
        printf("Sum is greater than 10\n");
    } else {
        printf("Sum is less than or equal to 10\n");
    }
    
    // 嵌套循环
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            printf("(%d, %d) ", i, j);
        }
        printf("\n");
    }
    
    // 函数调用
    printf("Factorial of 5: %d\n", factorial(5));
    printf("Fibonacci of 10: %d\n", fibonacci(10));
    
    // 数组操作
    print_array(numbers, 5);
    
    return 0;
}

/**
 * 计算阶乘
 */
int factorial(int n) {
    if (n <= 1) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

/**
 * 打印数组
 */
void print_array(int arr[], int size) {
    int i;
    printf("Array: ");
    for (i = 0; i < size; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

/**
 * 计算斐波那契数列
 */
int fibonacci(int n) {
    if (n <= 0) {
        return 0;
    } else if (n == 1) {
        return 1;
    } else {
        int a = 0, b = 1, c, i;
        for (i = 2; i <= n; i++) {
            c = a + b;
            a = b;
            b = c;
        }
        return b;
    }
} 