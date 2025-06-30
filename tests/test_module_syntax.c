// 测试模块语法解析功能
#include <stdio.h>

// 模块声明
module math_utils;

// 导入系统模块
import libc from "libc.native";

// 导出函数
export add_numbers;
export multiply_numbers;

// 实现导出的函数
int add_numbers(int a, int b) {
    return a + b;
}

int multiply_numbers(int a, int b) {
    return a * b;
}

int main() {
    printf("Testing module syntax parsing...\n");
    
    int result1 = add_numbers(5, 3);
    int result2 = multiply_numbers(4, 6);
    
    printf("5 + 3 = %d\n", result1);
    printf("4 * 6 = %d\n", result2);
    
    printf("Module syntax test completed!\n");
    return 0;
}
