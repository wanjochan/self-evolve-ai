/**
 * memory.c - Core Memory Management System Implementation
 */

#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 全局内存统计
static MemoryStats g_memory_stats = {0};

// 内存池状态
static bool g_initialized = false;

int memory_init(void) {
    if (g_initialized) {
        return 0;
    }
    
    memset(&g_memory_stats, 0, sizeof(MemoryStats));
    g_initialized = true;
    
    return 0;
}

void memory_cleanup(void) {
    if (!g_initialized) {
        return;
    }
    
    g_initialized = false;
}

void* memory_alloc(size_t size, MemoryPoolType pool) {
    if (!g_initialized || pool >= MEMORY_POOL_COUNT) {
        return NULL;
    }
    
    void* ptr = malloc(size);
    if (ptr) {
        g_memory_stats.total_allocated += size;
        g_memory_stats.current_usage += size;
        g_memory_stats.allocation_count++;
        g_memory_stats.pool_usage[pool] += size;
        
        if (g_memory_stats.current_usage > g_memory_stats.peak_usage) {
            g_memory_stats.peak_usage = g_memory_stats.current_usage;
        }
    }
    
    return ptr;
}

void memory_free(void* ptr) {
    if (!g_initialized || !ptr) {
        return;
    }
    
    free(ptr);
    g_memory_stats.free_count++;
}

void* memory_calloc(size_t count, size_t size, MemoryPoolType pool) {
    size_t total_size = count * size;
    void* ptr = memory_alloc(total_size, pool);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

void* memory_realloc(void* ptr, size_t new_size, MemoryPoolType pool) {
    if (!g_initialized) {
        return NULL;
    }
    
    void* new_ptr = realloc(ptr, new_size);
    if (new_ptr) {
        g_memory_stats.total_allocated += new_size;
        g_memory_stats.current_usage += new_size;
        g_memory_stats.pool_usage[pool] += new_size;
        
        if (g_memory_stats.current_usage > g_memory_stats.peak_usage) {
            g_memory_stats.peak_usage = g_memory_stats.current_usage;
        }
    }
    
    return new_ptr;
}

char* memory_strdup(const char* str, MemoryPoolType pool) {
    if (!str) {
        return NULL;
    }
    
    size_t len = strlen(str) + 1;
    char* dup = (char*)memory_alloc(len, pool);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
} 