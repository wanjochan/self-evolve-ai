/**
 * platform.c - 平台抽象层实现
 * 
 * 根据不同平台提供统一接口的具体实现
 */

#include "platform.h"
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

// 判断当前系统是否为Windows
int platform_is_windows(void) {
#ifdef _WIN32
    return 1;
#else
    return 0;
#endif
}

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
    // POSIX实现：使用mmap
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
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    // POSIX实现：使用munmap
    munmap(ptr, size);
#endif
} 