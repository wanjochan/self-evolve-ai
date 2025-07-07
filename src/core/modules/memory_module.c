/**
 * memory_module.c - 内存管理模块
 * 
 * 基于新的模块系统实现的内存管理模块。
 * 作为示例，展示如何使用新的模块系统创建模块。
 */

#include "../module.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

// ===============================================
// 内部常量和配置
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

// 内存API结构
typedef struct {
    void* (*alloc)(size_t size);
    void (*free)(void* ptr);
    void* (*realloc)(void* ptr, size_t size);
    void* (*calloc)(size_t count, size_t size);
    void* (*alloc_pool)(size_t size, MemoryPoolType pool);
    void (*get_stats)(MemoryPoolStats stats[MEMORY_POOL_COUNT]);
    void (*print_stats)(void);
} MemoryAPI;

// ===============================================
// 内部状态
// ===============================================

// 内存池链表头
static MemoryBlock* memory_pools[MEMORY_POOL_COUNT] = {NULL};

// 内存池统计
static MemoryPoolStats pool_stats[MEMORY_POOL_COUNT] = {{0}};

// 是否已初始化
static bool initialized = false;

// ===============================================
// 内部函数
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

// ===============================================
// 内存管理函数
// ===============================================

// 函数声明
static void* memory_alloc_pool(size_t size, MemoryPoolType pool);
static void* memory_alloc(size_t size);
static void memory_free(void* ptr);
static void* memory_realloc(void* ptr, size_t size);
static void* memory_calloc(size_t count, size_t size);
static void memory_get_stats(MemoryPoolStats stats[MEMORY_POOL_COUNT]);
static void memory_print_stats(void);

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

// 从特定内存池分配内存
static void* memory_alloc_pool(size_t size, MemoryPoolType pool) {
    if (pool >= MEMORY_POOL_COUNT) {
        pool = MEMORY_POOL_GENERAL;
    }
    
    // 分配内存块
    MemoryBlock* block = malloc(sizeof(MemoryBlock) + size);
    if (!block) return NULL;
    
    block->size = size;
    block->pool = pool;
    block->next = NULL;
    block->prev = NULL;
    
    // 添加到内存池
    add_to_pool(block, pool);
    
    return block->data;
}

// 获取内存池统计
static void memory_get_stats(MemoryPoolStats stats[MEMORY_POOL_COUNT]) {
    if (!stats) return;
    memcpy(stats, pool_stats, sizeof(pool_stats));
}

// 打印内存池统计
static void memory_print_stats(void) {
    printf("Memory Pool Statistics:\n");
    printf("Pool                 | Total      | Current    | Peak       | Allocs     | Frees\n");
    printf("--------------------+------------+------------+------------+------------+------------\n");
    
    const char* pool_names[] = {
        "General",
        "Bytecode",
        "JIT",
        "Modules",
        "Temporary",
        "C99 AST",
        "C99 Symbols",
        "C99 Strings"
    };
    
    for (int i = 0; i < MEMORY_POOL_COUNT; i++) {
        printf("%-20s | %10zu | %10zu | %10zu | %10zu | %10zu\n",
               pool_names[i],
               pool_stats[i].total_allocated,
               pool_stats[i].current_usage,
               pool_stats[i].peak_usage,
               pool_stats[i].allocation_count,
               pool_stats[i].free_count);
    }
    
    // 计算总计
    size_t total_allocated = 0;
    size_t current_usage = 0;
    size_t peak_usage = 0;
    size_t allocation_count = 0;
    size_t free_count = 0;
    
    for (int i = 0; i < MEMORY_POOL_COUNT; i++) {
        total_allocated += pool_stats[i].total_allocated;
        current_usage += pool_stats[i].current_usage;
        peak_usage += pool_stats[i].peak_usage;
        allocation_count += pool_stats[i].allocation_count;
        free_count += pool_stats[i].free_count;
    }
    
    printf("--------------------+------------+------------+------------+------------+------------\n");
    printf("%-20s | %10zu | %10zu | %10zu | %10zu | %10zu\n",
           "Total",
           total_allocated,
           current_usage,
           peak_usage,
           allocation_count,
           free_count);
}

// ===============================================
// 模块API
// ===============================================

// 内存API实例
static MemoryAPI memory_api = {
    .alloc = memory_alloc,
    .free = memory_free,
    .realloc = memory_realloc,
    .calloc = memory_calloc,
    .alloc_pool = memory_alloc_pool,
    .get_stats = memory_get_stats,
    .print_stats = memory_print_stats
};

// 获取内存API
static MemoryAPI* get_memory_api(void) {
    return &memory_api;
}

// ===============================================
// 符号表
// ===============================================

static struct {
    const char* name;
    void* symbol;
} memory_symbols[] = {
    {"memory_alloc", memory_alloc},
    {"memory_free", memory_free},
    {"memory_realloc", memory_realloc},
    {"memory_calloc", memory_calloc},
    {"memory_alloc_pool", memory_alloc_pool},
    {"memory_get_stats", memory_get_stats},
    {"memory_print_stats", memory_print_stats},
    {"get_memory_api", get_memory_api},
    {NULL, NULL}
};

// ===============================================
// 模块接口实现
// ===============================================

// 初始化模块
static int memory_init(void) {
    if (initialized) {
        return 0;  // 已初始化
    }
    
    // 初始化内存池
    for (int i = 0; i < MEMORY_POOL_COUNT; i++) {
        memory_pools[i] = NULL;
        memset(&pool_stats[i], 0, sizeof(MemoryPoolStats));
    }
    
    initialized = true;
    printf("Memory module initialized\n");
    
    return 0;
}

// 清理模块
static void memory_cleanup(void) {
    if (!initialized) {
        return;  // 未初始化
    }
    
    // 释放所有内存块
    for (int i = 0; i < MEMORY_POOL_COUNT; i++) {
        MemoryBlock* block = memory_pools[i];
        while (block) {
            MemoryBlock* next = block->next;
            free(block);
            block = next;
        }
        memory_pools[i] = NULL;
    }
    
    // 打印最终统计
    memory_print_stats();
    
    initialized = false;
    printf("Memory module cleaned up\n");
}

// 解析符号
static void* memory_resolve(const char* symbol) {
    if (!symbol) {
        return NULL;
    }
    
    for (int i = 0; memory_symbols[i].name; i++) {
        if (strcmp(memory_symbols[i].name, symbol) == 0) {
            return memory_symbols[i].symbol;
        }
    }
    
    return NULL;
}

// ===============================================
// 模块定义
// ===============================================

// 内存模块定义
Module module_memory = {
    .name = "memory",
    .state = MODULE_UNLOADED,
    .error = NULL,
    .init = memory_init,
    .cleanup = memory_cleanup,
    .resolve = memory_resolve
};

// 注意：不再需要REGISTER_MODULE，使用动态加载机制