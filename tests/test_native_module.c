/**
 * test_native_module.c - 测试原生模块构建的简单程序
 */

#include <stdio.h>

// 导出函数
int test_function(int a, int b) {
    return a + b;
}

// 导出变量
int test_variable = 42;

// 导出常量
const char* test_constant = "Hello from native module";

// 主函数
int main(void) {
    printf("Test native module\n");
    printf("Function result: %d\n", test_function(10, 20));
    printf("Variable value: %d\n", test_variable);
    printf("Constant value: %s\n", test_constant);
    return 0;
}
