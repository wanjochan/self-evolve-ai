/**
 * libc_minimal.c - 最小化libc实现
 * 
 * 仅包含最基本的函数，用于资源受限的环境
 * 适用于嵌入式系统和微内核
 */

#include <stddef.h>

// ===============================================
// 最小化内存操作
// ===============================================

void* memset(void* dest, int value, size_t count) {
    unsigned char* d = (unsigned char*)dest;
    unsigned char val = (unsigned char)value;
    
    for (size_t i = 0; i < count; i++) {
        d[i] = val;
    }
    
    return dest;
}

void* memcpy(void* dest, const void* src, size_t count) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    
    for (size_t i = 0; i < count; i++) {
        d[i] = s[i];
    }
    
    return dest;
}

// ===============================================
// 最小化字符串操作
// ===============================================

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

char* strcpy(char* dest, const char* src) {
    size_t i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

int strcmp(const char* str1, const char* str2) {
    size_t i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {
        if (str1[i] < str2[i]) return -1;
        if (str1[i] > str2[i]) return 1;
        i++;
    }
    
    if (str1[i] == '\0' && str2[i] == '\0') return 0;
    if (str1[i] == '\0') return -1;
    return 1;
}

// ===============================================
// 版本信息
// ===============================================

const char* libc_version(void) {
    return "libc_minimal v1.0";
}

int libc_function_count(void) {
    return 6; // memset, memcpy, strlen, strcpy, strcmp, libc_version
}

// ===============================================
// 模块初始化
// ===============================================

int libc_minimal_init(void) {
    return 0; // 成功
}

void libc_minimal_cleanup(void) {
    // 最小化版本无需清理
}

// ===============================================
// 主入口（用于测试）
// ===============================================

int main(void) {
    libc_minimal_init();
    
    // 简单测试
    char buffer[50];
    strcpy(buffer, "Minimal libc test");
    
    if (strcmp(buffer, "Minimal libc test") == 0) {
        return 0; // 测试通过
    }
    
    return 1; // 测试失败
}
