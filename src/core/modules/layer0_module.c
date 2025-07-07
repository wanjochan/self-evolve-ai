/**
 * layer0_module.c - Layer 0 Foundation Module
 * 
 * 基础层模块，合并了memory、utils、std和libdl的核心功能
 * 为其他模块提供基础服务，是整个模块系统的基础层
 */

#include "../module.h"
#include "../astc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <math.h>

// Platform-specific includes
#ifdef _WIN32
#include <windows.h>
#include <malloc.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#endif

// ===============================================
// 模块信息
// ===============================================

#define MODULE_NAME "layer0"
#define MODULE_VERSION "1.0.0"
#define MODULE_DESCRIPTION "Layer 0 Foundation Module (memory+utils+std+libdl)"

// ===============================================
// 内存管理部分 (from memory_module.c)
// ===============================================

// 内存池类型
typedef enum {
    MEMORY_POOL_GENERAL,    // 通用内存
    MEMORY_POOL_BYTECODE,   // 字节码存储
    MEMORY_POOL_JIT,        // JIT编译代码
    MEMORY_POOL_MODULES,    // 模块数据
    MEMORY_POOL_TEMP,       // 临时数据
    MEMORY_POOL_C99_AST,    // C99 AST nodes
    MEMORY_POOL_C99_SYMBOLS,// C99 symbol table
    MEMORY_POOL_C99_STRINGS,// C99 string literals
    MEMORY_POOL_COUNT       // 内存池数量
} MemoryPoolType;

// 内存块头
typedef struct MemoryBlock {
    size_t size;                  // 块大小
    MemoryPoolType pool;          // 所属内存池
    struct MemoryBlock* next;     // 链表下一项
    struct MemoryBlock* prev;     // 链表前一项
    unsigned char data[];         // 实际数据
} MemoryBlock;

// 内存池统计
typedef struct {
    size_t total_allocated;       // 总分配字节数
    size_t current_usage;         // 当前使用字节数
    size_t peak_usage;            // 峰值使用字节数
    size_t allocation_count;      // 分配次数
    size_t free_count;            // 释放次数
} MemoryPoolStats;

// 内存池链表头
static MemoryBlock* memory_pools[MEMORY_POOL_COUNT] = {NULL};

// 内存池统计
static MemoryPoolStats pool_stats[MEMORY_POOL_COUNT] = {{0}};

// ===============================================
// 架构检测部分 (from utils_module.c)
// ===============================================

typedef enum {
    ARCH_UNKNOWN = 0,
    ARCH_X86_32,
    ARCH_X86_64,
    ARCH_ARM32,
    ARCH_ARM64
} DetectedArchitecture;

typedef enum {
    PLATFORM_WINDOWS,
    PLATFORM_LINUX,
    PLATFORM_MACOS,
    PLATFORM_UNKNOWN
} RuntimePlatform;

// ===============================================
// 动态加载部分 (libdl functionality)
// ===============================================

typedef struct {
    void* handle;
    char* path;
    int ref_count;
} DynamicLibrary;

static DynamicLibrary* loaded_libraries = NULL;
static int library_count = 0;
static int library_capacity = 0;

// ===============================================
// 内存管理函数实现
// ===============================================

// 获取内存块
static MemoryBlock* get_block(void* ptr) {
    if (!ptr) return NULL;
    return (MemoryBlock*)((unsigned char*)ptr - sizeof(MemoryBlock));
}

// 添加到内存池
static void add_to_pool(MemoryBlock* block, MemoryPoolType pool) {
    if (!block) return;
    
    block->pool = pool;
    block->next = memory_pools[pool];
    block->prev = NULL;
    
    if (memory_pools[pool]) {
        memory_pools[pool]->prev = block;
    }
    
    memory_pools[pool] = block;
    
    // 更新统计
    pool_stats[pool].total_allocated += block->size;
    pool_stats[pool].current_usage += block->size;
    pool_stats[pool].allocation_count++;
    
    if (pool_stats[pool].current_usage > pool_stats[pool].peak_usage) {
        pool_stats[pool].peak_usage = pool_stats[pool].current_usage;
    }
}

// 从内存池移除
static void remove_from_pool(MemoryBlock* block) {
    if (!block) return;
    
    MemoryPoolType pool = block->pool;
    
    if (block->prev) {
        block->prev->next = block->next;
    } else {
        memory_pools[pool] = block->next;
    }
    
    if (block->next) {
        block->next->prev = block->prev;
    }
    
    // 更新统计
    pool_stats[pool].current_usage -= block->size;
    pool_stats[pool].free_count++;
}

// 从特定内存池分配内存
static void* memory_alloc_pool(size_t size, MemoryPoolType pool) {
    if (pool >= MEMORY_POOL_COUNT) {
        pool = MEMORY_POOL_GENERAL;
    }
    
    // 分配内存块
    MemoryBlock* block = malloc(sizeof(MemoryBlock) + size);
    if (!block) return NULL;
    
    block->size = size;
    add_to_pool(block, pool);
    
    return block->data;
}

// 分配内存
static void* memory_alloc(size_t size) {
    return memory_alloc_pool(size, MEMORY_POOL_GENERAL);
}

// 释放内存
static void memory_free(void* ptr) {
    if (!ptr) return;
    
    MemoryBlock* block = get_block(ptr);
    remove_from_pool(block);
    free(block);
}

// 重新分配内存
static void* memory_realloc(void* ptr, size_t size) {
    if (!ptr) return memory_alloc(size);
    if (size == 0) {
        memory_free(ptr);
        return NULL;
    }
    
    MemoryBlock* old_block = get_block(ptr);
    MemoryPoolType pool = old_block->pool;
    
    // 分配新块
    void* new_ptr = memory_alloc_pool(size, pool);
    if (!new_ptr) return NULL;
    
    // 复制数据
    size_t copy_size = old_block->size < size ? old_block->size : size;
    memcpy(new_ptr, ptr, copy_size);
    
    // 释放旧块
    memory_free(ptr);
    
    return new_ptr;
}

// 分配并清零内存
static void* memory_calloc(size_t count, size_t size) {
    size_t total = count * size;
    void* ptr = memory_alloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

// ===============================================
// 架构检测函数实现
// ===============================================

// 检测当前平台
static RuntimePlatform detect_platform(void) {
    if (getenv("WINDIR") != NULL || getenv("windir") != NULL) {
        return PLATFORM_WINDOWS;
    }

    FILE* test_file = fopen("/System/Library/CoreServices/SystemVersion.plist", "r");
    if (test_file) {
        fclose(test_file);
        return PLATFORM_MACOS;
    }

    test_file = fopen("/proc/version", "r");
    if (test_file) {
        fclose(test_file);
        return PLATFORM_LINUX;
    }

    return PLATFORM_UNKNOWN;
}

// 检测当前架构
static DetectedArchitecture detect_architecture(void) {
    RuntimePlatform platform = detect_platform();

    if (platform == PLATFORM_WINDOWS) {
        if (getenv("ProgramFiles(x86)") != NULL) {
            const char* processor_arch = getenv("PROCESSOR_ARCHITECTURE");
            const char* processor_arch_w6432 = getenv("PROCESSOR_ARCHITEW6432");

            if (processor_arch_w6432 && strstr(processor_arch_w6432, "AMD64")) {
                return ARCH_X86_64;
            } else if (processor_arch && strstr(processor_arch, "AMD64")) {
                return ARCH_X86_64;
            } else if (processor_arch && strstr(processor_arch, "ARM64")) {
                return ARCH_ARM64;
            } else if (processor_arch && strstr(processor_arch, "ARM")) {
                return ARCH_ARM32;
            }
            return ARCH_X86_64;
        } else {
            const char* processor_arch = getenv("PROCESSOR_ARCHITECTURE");
            if (processor_arch && strstr(processor_arch, "x86")) {
                return ARCH_X86_32;
            } else if (processor_arch && strstr(processor_arch, "ARM")) {
                return ARCH_ARM32;
            }
            return ARCH_X86_32;
        }
    } else {
        FILE* cpuinfo = fopen("/proc/cpuinfo", "r");
        if (cpuinfo) {
            char line[256];
            while (fgets(line, sizeof(line), cpuinfo)) {
                if (strstr(line, "x86_64") || strstr(line, "amd64")) {
                    fclose(cpuinfo);
                    return ARCH_X86_64;
                } else if (strstr(line, "aarch64") || strstr(line, "arm64")) {
                    fclose(cpuinfo);
                    return ARCH_ARM64;
                } else if (strstr(line, "i386") || strstr(line, "i686")) {
                    fclose(cpuinfo);
                    return ARCH_X86_32;
                } else if (strstr(line, "arm")) {
                    fclose(cpuinfo);
                    return ARCH_ARM32;
                }
            }
            fclose(cpuinfo);
        }

        if (sizeof(void*) == 8) {
            return ARCH_X86_64;
        } else {
            return ARCH_X86_32;
        }
    }

    return ARCH_UNKNOWN;
}

// 获取架构名称
static const char* get_architecture_name(DetectedArchitecture arch) {
    switch (arch) {
        case ARCH_X86_64: return "x64";
        case ARCH_X86_32: return "x86";
        case ARCH_ARM64: return "arm64";
        case ARCH_ARM32: return "arm32";
        default: return "unknown";
    }
}

// 获取架构位数
static int get_architecture_bits(DetectedArchitecture arch) {
    switch (arch) {
        case ARCH_X86_64: return 64;
        case ARCH_X86_32: return 32;
        case ARCH_ARM64: return 64;
        case ARCH_ARM32: return 32;
        default: return 0;
    }
}

// ===============================================
// 工具函数实现
// ===============================================

// 安全字符串复制
static void safe_strncpy(char* dest, const char* src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) return;
    
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
}

// 安全格式化字符串
static int safe_snprintf(char* buffer, size_t size, const char* format, ...) {
    if (!buffer || !format || size == 0) return -1;
    
    va_list args;
    va_start(args, format);
    int result = vsnprintf(buffer, size, format, args);
    va_end(args);
    
    return result;
}

// 文件是否存在
static int file_exists(const char* path) {
    if (!path) return 0;
    FILE* file = fopen(path, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

// 获取文件大小
static long get_file_size(const char* path) {
    if (!path) return -1;
    FILE* file = fopen(path, "rb");
    if (!file) return -1;
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fclose(file);
    
    return size;
}

// ===============================================
// 动态加载函数实现
// ===============================================

// 加载动态库
static void* dlopen_wrapper(const char* filename, int flag) {
#ifdef _WIN32
    (void)flag; // 忽略flag参数
    return LoadLibrary(filename);
#else
    return dlopen(filename, flag);
#endif
}

// 获取符号地址
static void* dlsym_wrapper(void* handle, const char* symbol) {
#ifdef _WIN32
    return GetProcAddress((HMODULE)handle, symbol);
#else
    return dlsym(handle, symbol);
#endif
}

// 关闭动态库
static int dlclose_wrapper(void* handle) {
#ifdef _WIN32
    return FreeLibrary((HMODULE)handle) ? 0 : -1;
#else
    return dlclose(handle);
#endif
}

// 获取错误信息
static char* dlerror_wrapper(void) {
#ifdef _WIN32
    static char error_msg[256];
    DWORD error = GetLastError();
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, error_msg, sizeof(error_msg), NULL);
    return error_msg;
#else
    return dlerror();
#endif
}

// ===============================================
// 标准库函数实现
// ===============================================

// 标准库的printf实现
static int std_printf(const char* format, ...) {
    if (!format) return 0;
    
    va_list args;
    va_start(args, format);
    int result = vprintf(format, args);
    va_end(args);
    
    return result;
}

// 标准库的sprintf实现
static int std_sprintf(char* str, const char* format, ...) {
    if (!str || !format) return 0;
    
    va_list args;
    va_start(args, format);
    int result = vsprintf(str, format, args);
    va_end(args);
    
    return result;
}

// 标准库的strlen实现
static size_t std_strlen(const char* str) {
    return str ? strlen(str) : 0;
}

// 标准库的strcpy实现
static char* std_strcpy(char* dest, const char* src) {
    return (dest && src) ? strcpy(dest, src) : dest;
}

// 标准库的strcmp实现
static int std_strcmp(const char* str1, const char* str2) {
    if (!str1 || !str2) return 0;
    return strcmp(str1, str2);
}

// ===============================================
// 模块符号表
// ===============================================

static struct {
    const char* name;
    void* symbol;
} layer0_symbols[] = {
    // 内存管理
    {"memory_alloc", memory_alloc},
    {"memory_free", memory_free},
    {"memory_realloc", memory_realloc},
    {"memory_calloc", memory_calloc},
    {"memory_alloc_pool", memory_alloc_pool},
    
    // 架构检测
    {"detect_architecture", detect_architecture},
    {"detect_platform", detect_platform},
    {"get_architecture_name", get_architecture_name},
    {"get_architecture_bits", get_architecture_bits},
    
    // 工具函数
    {"safe_strncpy", safe_strncpy},
    {"safe_snprintf", safe_snprintf},
    {"file_exists", file_exists},
    {"get_file_size", get_file_size},
    
    // 动态加载
    {"dlopen", dlopen_wrapper},
    {"dlsym", dlsym_wrapper},
    {"dlclose", dlclose_wrapper},
    {"dlerror", dlerror_wrapper},
    
    // 标准库函数
    {"printf", std_printf},
    {"sprintf", std_sprintf},
    {"strlen", std_strlen},
    {"strcpy", std_strcpy},
    {"strcmp", std_strcmp},
    {"malloc", memory_alloc},
    {"free", memory_free},
    {"realloc", memory_realloc},
    {"calloc", memory_calloc},
    
    {NULL, NULL}
};

// ===============================================
// 模块初始化和清理
// ===============================================

static int layer0_init(void) {
    printf("Layer0 Module: Initializing foundation module...\n");
    
    // 初始化内存池
    for (int i = 0; i < MEMORY_POOL_COUNT; i++) {
        memory_pools[i] = NULL;
        memset(&pool_stats[i], 0, sizeof(MemoryPoolStats));
    }
    
    // 检测系统架构
    DetectedArchitecture arch = detect_architecture();
    RuntimePlatform platform = detect_platform();
    
    printf("Layer0 Module: Detected architecture: %s (%d-bit)\n", 
           get_architecture_name(arch), get_architecture_bits(arch));
    printf("Layer0 Module: Detected platform: %d\n", platform);
    
    return 0;
}

static void layer0_cleanup(void) {
    printf("Layer0 Module: Cleaning up foundation module...\n");
    
    // 释放所有内存池
    for (int i = 0; i < MEMORY_POOL_COUNT; i++) {
        MemoryBlock* block = memory_pools[i];
        while (block) {
            MemoryBlock* next = block->next;
            free(block);
            block = next;
        }
        memory_pools[i] = NULL;
    }
    
    // 清理动态库
    if (loaded_libraries) {
        for (int i = 0; i < library_count; i++) {
            if (loaded_libraries[i].handle) {
                dlclose_wrapper(loaded_libraries[i].handle);
            }
            if (loaded_libraries[i].path) {
                free(loaded_libraries[i].path);
            }
        }
        free(loaded_libraries);
        loaded_libraries = NULL;
        library_count = 0;
        library_capacity = 0;
    }
}

// ===============================================
// 模块接口实现
// ===============================================

// 解析符号
static void* layer0_resolve(const char* symbol) {
    if (!symbol) return NULL;
    
    for (int i = 0; layer0_symbols[i].name; i++) {
        if (strcmp(layer0_symbols[i].name, symbol) == 0) {
            return layer0_symbols[i].symbol;
        }
    }
    
    return NULL;
}

// ===============================================
// 模块定义
// ===============================================

// Layer0模块定义
Module module_layer0 = {
    .name = "layer0",
    .state = MODULE_UNLOADED,
    .error = NULL,
    .init = layer0_init,
    .cleanup = layer0_cleanup,
    .resolve = layer0_resolve
}; 