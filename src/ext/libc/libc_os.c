/**
 * libc_os.c - 操作系统开发专用的自实现libc
 * 
 * 完全独立的标准库实现，不依赖任何外部库
 * 专为操作系统内核和系统级程序设计
 */

#include <stddef.h>
#include <stdint.h>

// ===============================================
// 基础内存操作
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

// ===============================================
// 字符串操作
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

// ===============================================
// 数字转换
// ===============================================

int atoi(const char* str) {
    int result = 0;
    int sign = 1;
    size_t i = 0;
    
    // 跳过空白字符
    while (str[i] == ' ' || str[i] == '\t') {
        i++;
    }
    
    // 处理符号
    if (str[i] == '-') {
        sign = -1;
        i++;
    } else if (str[i] == '+') {
        i++;
    }
    
    // 转换数字
    while (str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;
    }
    
    return result * sign;
}

// 简化的sprintf实现（仅支持基本格式）
int sprintf(char* buffer, const char* format, ...) {
    // 这是一个极简实现，仅用于演示
    // 实际的sprintf需要复杂的格式解析
    
    size_t i = 0, j = 0;
    while (format[i] != '\0') {
        if (format[i] == '%' && format[i+1] == 's') {
            // 简单的字符串替换（需要va_list支持）
            buffer[j++] = 'S';
            buffer[j++] = 'T';
            buffer[j++] = 'R';
            i += 2;
        } else if (format[i] == '%' && format[i+1] == 'd') {
            // 简单的数字替换
            buffer[j++] = 'N';
            buffer[j++] = 'U';
            buffer[j++] = 'M';
            i += 2;
        } else {
            buffer[j++] = format[i++];
        }
    }
    buffer[j] = '\0';
    
    return j;
}

// ===============================================
// 系统级内存管理
// ===============================================

// 简化的内存分配器（用于操作系统开发）
static char memory_pool[64 * 1024]; // 64KB内存池
static size_t memory_offset = 0;

void* malloc(size_t size) {
    // 简单的线性分配器
    if (memory_offset + size > sizeof(memory_pool)) {
        return NULL; // 内存不足
    }
    
    void* ptr = &memory_pool[memory_offset];
    memory_offset += size;
    
    // 对齐到8字节边界
    memory_offset = (memory_offset + 7) & ~7;
    
    return ptr;
}

void free(void* ptr) {
    // 简化实现：不实际释放内存
    // 实际的操作系统需要更复杂的内存管理
    (void)ptr; // 避免未使用警告
}

// ===============================================
// 系统调用接口（操作系统专用）
// ===============================================

// 系统输出（直接写入控制台）
int puts(const char* str) {
    // 在实际操作系统中，这里会调用内核的输出函数
    // 这里只是一个占位符实现
    size_t len = strlen(str);
    
    // 模拟系统调用
    for (size_t i = 0; i < len; i++) {
        // 在实际实现中，这里会调用内核的字符输出函数
        // 例如：kernel_putchar(str[i]);
    }
    
    return len;
}

int printf(const char* format, ...) {
    // 简化的printf实现
    // 在实际操作系统中需要完整的格式化支持
    return puts(format);
}

// ===============================================
// 操作系统专用函数
// ===============================================

// 内存池重置（用于操作系统内存管理）
void os_memory_reset(void) {
    memory_offset = 0;
    memset(memory_pool, 0, sizeof(memory_pool));
}

// 获取内存使用统计
size_t os_memory_used(void) {
    return memory_offset;
}

size_t os_memory_available(void) {
    return sizeof(memory_pool) - memory_offset;
}

// 系统初始化函数
void libc_os_init(void) {
    os_memory_reset();
}

// 导出符号表（用于.rt模块）
typedef struct {
    const char* name;
    void* function;
} OSLibcExport;

static const OSLibcExport os_libc_exports[] = {
    {"memset", memset},
    {"memcpy", memcpy},
    {"memcmp", memcmp},
    {"strlen", strlen},
    {"strcpy", strcpy},
    {"strcat", strcat},
    {"strcmp", strcmp},
    {"atoi", atoi},
    {"sprintf", sprintf},
    {"malloc", malloc},
    {"free", free},
    {"puts", puts},
    {"printf", printf},
    {"os_memory_reset", os_memory_reset},
    {"os_memory_used", os_memory_used},
    {"os_memory_available", os_memory_available},
    {"libc_os_init", libc_os_init},
    {NULL, NULL} // 结束标记
};

// 获取导出函数数量
int get_os_libc_export_count(void) {
    int count = 0;
    while (os_libc_exports[count].name != NULL) {
        count++;
    }
    return count;
}

// 主入口函数（用于测试）
int main(void) {
    libc_os_init();
    
    printf("OS libc initialized\n");
    printf("Available functions: %d\n", get_os_libc_export_count());
    printf("Memory pool size: %zu bytes\n", sizeof(memory_pool));
    
    // 测试基本功能
    char buffer[100];
    strcpy(buffer, "Hello, OS libc!");
    printf("Test string: %s\n", buffer);
    
    return 0;
}
