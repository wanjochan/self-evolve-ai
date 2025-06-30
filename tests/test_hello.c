#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main() {
    printf("Hello, Self-Evolve AI!\n");
    printf("Testing C99 compiler features:\n");

    // 测试基本数据类型
    int a = 42;
    float b = 3.14f;
    double c = 2.718281828;
    char str[] = "Test string";

    printf("Integer: %d\n", a);
    printf("Float: %.2f\n", b);
    printf("Double: %.6f\n", c);
    printf("String: %s\n", str);

    // 测试数学函数
    double sqrt_result = sqrt(16.0);
    printf("sqrt(16) = %.2f\n", sqrt_result);

    // 测试字符串函数
    int len = strlen(str);
    printf("String length: %d\n", len);

    // 测试内存分配
    int* ptr = (int*)malloc(sizeof(int) * 5);
    if (ptr) {
        for (int i = 0; i < 5; i++) {
            ptr[i] = i * i;
            printf("ptr[%d] = %d\n", i, ptr[i]);
        }
        free(ptr);
    }

    // 测试控制流
    for (int i = 0; i < 3; i++) {
        if (i % 2 == 0) {
            printf("Even: %d\n", i);
        } else {
            printf("Odd: %d\n", i);
        }
    }

    printf("All tests completed successfully!\n");
    return 0;
}
