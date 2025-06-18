#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

int main() {
    printf("===================================\n");
    printf("TCC 交叉编译测试程序\n");
    printf("===================================\n\n");

    // 检测架构
    #ifdef __i386__
    printf("架构: x86 (32位)\n");
    #elif defined(__x86_64__)
    printf("架构: x86_64 (64位)\n");
    #elif defined(__arm__)
    printf("架构: ARM (32位)\n");
    #elif defined(__aarch64__)
    printf("架构: ARM64 (64位)\n");
    #else
    printf("架构: 未知\n");
    #endif

    // 检测操作系统
    #ifdef _WIN32
    printf("操作系统: Windows\n");
    #elif defined(__linux__)
    printf("操作系统: Linux\n");
    #elif defined(__APPLE__)
    printf("操作系统: macOS\n");
    #else
    printf("操作系统: 未知\n");
    #endif

    // 检测编译器
    #ifdef __TINYC__
    printf("编译器: Tiny C Compiler (TCC)\n");
    #elif defined(__GNUC__)
    printf("编译器: GCC\n");
    #elif defined(_MSC_VER)
    printf("编译器: Microsoft Visual C++\n");
    #else
    printf("编译器: 未知\n");
    #endif

    // 检测二进制格式
    #ifdef _WIN32
    printf("二进制格式: PE (Windows)\n");
    #elif defined(__linux__)
    printf("二进制格式: ELF (Linux)\n");
    #elif defined(__APPLE__)
    printf("二进制格式: Mach-O (macOS)\n");
    #else
    printf("二进制格式: 未知\n");
    #endif

    // 显示指针大小
    printf("指针大小: %zu 字节\n", sizeof(void*));

    // 显示整数类型大小
    printf("int 大小: %zu 字节\n", sizeof(int));
    printf("long 大小: %zu 字节\n", sizeof(long));
    printf("long long 大小: %zu 字节\n", sizeof(long long));

    printf("\n===================================\n");
    printf("测试完成！\n");
    printf("===================================\n");

    return 0;
} 