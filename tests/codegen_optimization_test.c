/**
 * codegen_optimization_test.c - 测试代码生成优化
 */

#include <stdio.h>

int main() {
    printf("Testing code generation optimization...\n");
    
    // 测试常量运算（应该被常量折叠优化）
    int a = 5 + 3;
    printf("Constant folding test: 5 + 3 = %d\n", a);
    
    // 测试变量运算
    int b = 10;
    int c = 20;
    int d = b + c;
    printf("Variable operation test: %d + %d = %d\n", b, c, d);
    
    printf("Code generation optimization test complete.\n");
    return 0;
}
