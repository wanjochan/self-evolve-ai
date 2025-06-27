/**
 * platform_minimal.c - 最小化平台抽象层实现
 * 
 * 只包含evolver0_loader需要的内存管理功能，避免网络依赖
 */

#include "platform.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>

#ifndef _WIN32
#include <time.h>
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

// ===============================================
// 内存管理功能 (evolver0_loader需要的核心功能)
// ===============================================

// 分配可执行内存
void* platform_alloc_executable(size_t size) {
#ifdef _WIN32
    // Windows实现：使用VirtualAlloc
    void* ptr = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!ptr) {
        fprintf(stderr, "Error: VirtualAlloc failed to allocate %zu bytes\n", size);
        return NULL;
    }
    return ptr;
#else
    // Linux/Unix实现：使用mmap
    void* ptr = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, 
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        fprintf(stderr, "Error: mmap failed to allocate %zu bytes\n", size);
        return NULL;
    }
    return ptr;
#endif
}

// 释放可执行内存
void platform_free_executable(void* ptr, size_t size) {
    if (!ptr) return;

#ifdef _WIN32
    // Windows实现：使用VirtualFree
    // 注意：Windows的VirtualFree不需要size参数
    (void)size; // 避免未使用参数警告
    if (!VirtualFree(ptr, 0, MEM_RELEASE)) {
        fprintf(stderr, "Warning: VirtualFree failed\n");
    }
#else
    // Linux/Unix实现：使用munmap
    if (munmap(ptr, size) != 0) {
        fprintf(stderr, "Warning: munmap failed\n");
    }
#endif
}

// ===============================================
// 平台检测功能 (简化版本)
// ===============================================

// 判断当前系统是否为Windows
int platform_is_windows(void) {
#ifdef _WIN32
    return 1;
#else
    return 0;
#endif
}

// ===============================================
// 文件系统功能 (基础版本)
// ===============================================

// 检查文件是否存在
int platform_file_exists(const char* path) {
    if (!path) return 0;
    
    FILE* file = fopen(path, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

// 获取文件大小
long platform_file_size(const char* path) {
    if (!path) return -1;
    
    FILE* file = fopen(path, "rb");
    if (!file) return -1;
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fclose(file);
    
    return size;
}

// ===============================================
// 时间功能 (基础版本)
// ===============================================

// 获取当前时间戳 (毫秒)
uint64_t platform_get_timestamp() {
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    
    // 转换为64位整数 (100纳秒单位)
    uint64_t time = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    
    // 转换为毫秒 (从1601年1月1日开始)
    time = (time - 116444736000000000ULL) / 10000;
    
    return time;
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#endif
}

// ===============================================
// 调试和日志功能
// ===============================================

// 输出调试信息
void platform_debug_print(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    printf("[DEBUG] ");
    vprintf(format, args);
    printf("\n");
    
    va_end(args);
}

// 输出错误信息
void platform_error_print(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    fprintf(stderr, "[ERROR] ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    
    va_end(args);
}
