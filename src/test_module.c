/**
 * test_module.c - 简单的测试模块
 * 
 * 这是一个简单的测试模块，用于测试build_native_module工具。
 */

#include <stdio.h>

// 测试函数
int test_function(int a, int b) {
    return a + b;
}

// 测试变量
int test_variable = 42;

// 测试常量
const int test_constant = 100;

// 模块入口点
int module_main(int argc, char* argv[]) {
    printf("测试模块已加载\n");
    printf("test_function(10, 20) = %d\n", test_function(10, 20));
    printf("test_variable = %d\n", test_variable);
    printf("test_constant = %d\n", test_constant);
    return 0;
} 