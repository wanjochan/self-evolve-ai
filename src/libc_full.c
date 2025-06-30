/**
 * libc_full.c - 完整libc实现
 * 
 * 包含完整的标准库函数，用于通用应用程序
 * 提供最大的兼容性和功能
 */

#include <stddef.h>
#include <stdint.h>

// ===============================================
// 完整内存操作
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

int memcmp(const void* ptr1, const void* ptr2, size_t count) {
    const unsigned char* p1 = (const unsigned char*)ptr1;
    const unsigned char* p2 = (const unsigned char*)ptr2;
    
    for (size_t i = 0; i < count; i++) {
        if (p1[i] < p2[i]) return -1;
        if (p1[i] > p2[i]) return 1;
    }
    
    return 0;
}

void* memmove(void* dest, const void* src, size_t count) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    
    if (d < s) {
        for (size_t i = 0; i < count; i++) {
            d[i] = s[i];
        }
    } else {
        for (size_t i = count; i > 0; i--) {
            d[i-1] = s[i-1];
        }
    }
    
    return dest;
}

// ===============================================
// 完整字符串操作
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

char* strncpy(char* dest, const char* src, size_t count) {
    size_t i = 0;
    while (i < count && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    while (i < count) {
        dest[i] = '\0';
        i++;
    }
    return dest;
}

char* strcat(char* dest, const char* src) {
    size_t dest_len = strlen(dest);
    size_t i = 0;
    
    while (src[i] != '\0') {
        dest[dest_len + i] = src[i];
        i++;
    }
    dest[dest_len + i] = '\0';
    
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

int strncmp(const char* str1, const char* str2, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (str1[i] == '\0' || str2[i] == '\0') {
            if (str1[i] == str2[i]) return 0;
            return (str1[i] < str2[i]) ? -1 : 1;
        }
        if (str1[i] != str2[i]) {
            return (str1[i] < str2[i]) ? -1 : 1;
        }
    }
    return 0;
}

char* strchr(const char* str, int character) {
    while (*str != '\0') {
        if (*str == character) {
            return (char*)str;
        }
        str++;
    }
    return NULL;
}

// ===============================================
// 数学函数
// ===============================================

int abs(int value) {
    return (value < 0) ? -value : value;
}

long labs(long value) {
    return (value < 0) ? -value : value;
}

// ===============================================
// 转换函数
// ===============================================

int atoi(const char* str) {
    int result = 0;
    int sign = 1;
    size_t i = 0;
    
    while (str[i] == ' ' || str[i] == '\t') {
        i++;
    }
    
    if (str[i] == '-') {
        sign = -1;
        i++;
    } else if (str[i] == '+') {
        i++;
    }
    
    while (str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;
    }
    
    return result * sign;
}

long atol(const char* str) {
    long result = 0;
    int sign = 1;
    size_t i = 0;
    
    while (str[i] == ' ' || str[i] == '\t') {
        i++;
    }
    
    if (str[i] == '-') {
        sign = -1;
        i++;
    } else if (str[i] == '+') {
        i++;
    }
    
    while (str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;
    }
    
    return result * sign;
}

// ===============================================
// 字符分类函数
// ===============================================

int isdigit(int c) {
    return (c >= '0' && c <= '9');
}

int isalpha(int c) {
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

int isalnum(int c) {
    return isalpha(c) || isdigit(c);
}

int isspace(int c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v');
}

int tolower(int c) {
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    return c;
}

int toupper(int c) {
    if (c >= 'a' && c <= 'z') {
        return c - ('a' - 'A');
    }
    return c;
}

// ===============================================
// 版本信息
// ===============================================

const char* libc_version(void) {
    return "libc_full v1.0";
}

int libc_function_count(void) {
    return 25; // 所有实现的函数数量
}

// ===============================================
// 模块初始化
// ===============================================

int libc_full_init(void) {
    return 0; // 成功
}

void libc_full_cleanup(void) {
    // 完整版本无需特殊清理
}

// ===============================================
// 主入口（用于测试）
// ===============================================

int main(void) {
    libc_full_init();
    
    // 测试各种功能
    char buffer[100];
    strcpy(buffer, "Full libc test");
    
    if (strcmp(buffer, "Full libc test") == 0 && 
        strlen(buffer) == 14 &&
        isalpha('A') &&
        isdigit('5') &&
        atoi("123") == 123) {
        return 0; // 测试通过
    }
    
    return 1; // 测试失败
}
