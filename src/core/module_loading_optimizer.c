#include "module_loading_optimizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>

// 全局优化器实例
ModuleLoadingOptimizer g_module_optimizer = {0};

// 预加载线程
static pthread_t preload_thread;
static bool preload_thread_running = false;

// 获取高精度时间
double module_optimizer_get_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// 字符串哈希函数 (djb2算法)
uint64_t module_optimizer_hash_string(const char* str) {
    if (!str) return 0;
    
    uint64_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

// 获取默认配置
ModuleLoadingOptimizerConfig module_optimizer_get_default_config(void) {
    ModuleLoadingOptimizerConfig config = {
        .enable_preloading = true,
        .enable_lazy_loading = true,
        .enable_symbol_cache = true,
        .enable_memory_pool = true,
        .enable_compression = false,  // 暂时禁用压缩
        
        .preload_thread_count = 2,
        .symbol_cache_size = 1024,
        .memory_pool_size = 1024 * 1024,  // 1MB
        .cache_hit_threshold = 0.8
    };
    return config;
}

// 初始化优化器
int module_optimizer_init(const ModuleLoadingOptimizerConfig* config) {
    if (g_module_optimizer.is_initialized) {
        return 0;  // 已经初始化
    }
    
    // 使用默认配置或提供的配置
    if (config) {
        g_module_optimizer.config = *config;
    } else {
        g_module_optimizer.config = module_optimizer_get_default_config();
    }
    
    // 初始化统计信息
    memset(&g_module_optimizer.stats, 0, sizeof(ModuleLoadingStats));
    g_module_optimizer.stats.last_update = time(NULL);
    
    // 初始化预加载缓存
    memset(g_module_optimizer.preload_cache, 0, sizeof(g_module_optimizer.preload_cache));
    g_module_optimizer.preload_count = 0;
    
    // 初始化符号缓存
    memset(g_module_optimizer.symbol_cache, 0, sizeof(g_module_optimizer.symbol_cache));
    g_module_optimizer.symbol_count = 0;
    
    // 初始化内存池
    if (g_module_optimizer.config.enable_memory_pool) {
        g_module_optimizer.memory_pool = malloc(sizeof(MemoryPoolBlock));
        if (g_module_optimizer.memory_pool) {
            g_module_optimizer.memory_pool->memory = malloc(g_module_optimizer.config.memory_pool_size);
            g_module_optimizer.memory_pool->size = g_module_optimizer.config.memory_pool_size;
            g_module_optimizer.memory_pool->used = 0;
            g_module_optimizer.memory_pool->is_free = true;
            g_module_optimizer.memory_pool->next = NULL;
            
            g_module_optimizer.pool_total_size = g_module_optimizer.config.memory_pool_size;
            g_module_optimizer.pool_used_size = 0;
        }
    }
    
    g_module_optimizer.is_initialized = true;
    g_module_optimizer.is_optimizing = false;
    g_module_optimizer.init_time = time(NULL);
    
    printf("Module Optimizer: 初始化完成\n");
    printf("  预加载: %s\n", g_module_optimizer.config.enable_preloading ? "启用" : "禁用");
    printf("  符号缓存: %s\n", g_module_optimizer.config.enable_symbol_cache ? "启用" : "禁用");
    printf("  内存池: %s (%d KB)\n", 
           g_module_optimizer.config.enable_memory_pool ? "启用" : "禁用",
           g_module_optimizer.config.memory_pool_size / 1024);
    
    return 0;
}

// 清理优化器
void module_optimizer_cleanup(void) {
    if (!g_module_optimizer.is_initialized) {
        return;
    }
    
    // 停止预加载线程
    if (preload_thread_running) {
        module_optimizer_stop_preload_thread();
    }
    
    // 清理预加载缓存
    for (int i = 0; i < 256; i++) {
        PreloadEntry* entry = g_module_optimizer.preload_cache[i];
        while (entry) {
            PreloadEntry* next = entry->next;
            free(entry->module_name);
            free(entry->module_path);
            free(entry->cached_data);
            free(entry);
            entry = next;
        }
    }
    
    // 清理符号缓存
    for (int i = 0; i < 1024; i++) {
        SymbolCacheEntry* entry = g_module_optimizer.symbol_cache[i];
        while (entry) {
            SymbolCacheEntry* next = entry->next;
            free(entry->symbol_name);
            free(entry->module_name);
            free(entry);
            entry = next;
        }
    }
    
    // 清理内存池
    MemoryPoolBlock* block = g_module_optimizer.memory_pool;
    while (block) {
        MemoryPoolBlock* next = block->next;
        free(block->memory);
        free(block);
        block = next;
    }
    
    g_module_optimizer.is_initialized = false;
    printf("Module Optimizer: 清理完成\n");
}

// 检查是否已初始化
bool module_optimizer_is_initialized(void) {
    return g_module_optimizer.is_initialized;
}

// 符号缓存查找
void* module_optimizer_lookup_symbol(const char* symbol_name) {
    if (!g_module_optimizer.is_initialized || !g_module_optimizer.config.enable_symbol_cache) {
        return NULL;
    }
    
    uint64_t hash = module_optimizer_hash_string(symbol_name);
    int bucket = hash % 1024;
    
    SymbolCacheEntry* entry = g_module_optimizer.symbol_cache[bucket];
    while (entry) {
        if (strcmp(entry->symbol_name, symbol_name) == 0) {
            entry->access_count++;
            entry->last_access = time(NULL);
            g_module_optimizer.stats.cache_hits++;
            return entry->symbol_addr;
        }
        entry = entry->next;
    }
    
    g_module_optimizer.stats.cache_misses++;
    return NULL;
}

// 缓存符号
int module_optimizer_cache_symbol(const char* module_name, const char* symbol_name, void* symbol_addr) {
    if (!g_module_optimizer.is_initialized || !g_module_optimizer.config.enable_symbol_cache) {
        return -1;
    }
    
    uint64_t hash = module_optimizer_hash_string(symbol_name);
    int bucket = hash % 1024;
    
    // 检查是否已存在
    SymbolCacheEntry* existing = g_module_optimizer.symbol_cache[bucket];
    while (existing) {
        if (strcmp(existing->symbol_name, symbol_name) == 0) {
            existing->symbol_addr = symbol_addr;  // 更新地址
            return 0;
        }
        existing = existing->next;
    }
    
    // 创建新条目
    SymbolCacheEntry* entry = malloc(sizeof(SymbolCacheEntry));
    if (!entry) return -1;
    
    entry->symbol_name = strdup(symbol_name);
    entry->module_name = strdup(module_name);
    entry->symbol_addr = symbol_addr;
    entry->hash = hash;
    entry->last_access = time(NULL);
    entry->access_count = 1;
    entry->next = g_module_optimizer.symbol_cache[bucket];
    
    g_module_optimizer.symbol_cache[bucket] = entry;
    g_module_optimizer.symbol_count++;
    
    return 0;
}

// 内存池分配
void* module_optimizer_alloc(size_t size) {
    if (!g_module_optimizer.is_initialized || !g_module_optimizer.config.enable_memory_pool) {
        return malloc(size);
    }
    
    // 对齐到8字节边界
    size = (size + 7) & ~7;
    
    MemoryPoolBlock* block = g_module_optimizer.memory_pool;
    while (block) {
        if (block->is_free && (block->size - block->used) >= size) {
            void* ptr = (char*)block->memory + block->used;
            block->used += size;
            
            if (block->used >= block->size) {
                block->is_free = false;
            }
            
            g_module_optimizer.pool_used_size += size;
            g_module_optimizer.stats.memory_allocated += size;
            
            if (g_module_optimizer.pool_used_size > g_module_optimizer.stats.memory_peak) {
                g_module_optimizer.stats.memory_peak = g_module_optimizer.pool_used_size;
            }
            
            return ptr;
        }
        block = block->next;
    }
    
    // 内存池不足，使用系统分配
    return malloc(size);
}

// 内存池释放 (简化版本，实际应该实现更复杂的释放逻辑)
void module_optimizer_free(void* ptr) {
    if (!ptr) return;
    
    if (!g_module_optimizer.is_initialized || !g_module_optimizer.config.enable_memory_pool) {
        free(ptr);
        return;
    }
    
    // 简化版本：不实际释放内存池中的内存，只是标记
    // 实际实现应该跟踪分配的块并正确释放
    // 这里只是为了演示概念
}

// 获取统计信息
ModuleLoadingStats module_optimizer_get_stats(void) {
    if (!g_module_optimizer.is_initialized) {
        ModuleLoadingStats empty_stats = {0};
        return empty_stats;
    }
    
    // 更新平均加载时间
    if (g_module_optimizer.stats.total_loads > 0) {
        g_module_optimizer.stats.avg_load_time = 
            g_module_optimizer.stats.total_load_time / g_module_optimizer.stats.total_loads;
    }
    
    g_module_optimizer.stats.last_update = time(NULL);
    return g_module_optimizer.stats;
}

// 获取缓存命中率
double module_optimizer_get_cache_hit_rate(void) {
    if (!g_module_optimizer.is_initialized) {
        return 0.0;
    }
    
    uint64_t total_accesses = g_module_optimizer.stats.cache_hits + g_module_optimizer.stats.cache_misses;
    if (total_accesses == 0) {
        return 0.0;
    }
    
    return (double)g_module_optimizer.stats.cache_hits / total_accesses;
}

// 打印统计信息
void module_optimizer_print_stats(void) {
    if (!g_module_optimizer.is_initialized) {
        printf("Module Optimizer: 未初始化\n");
        return;
    }
    
    ModuleLoadingStats stats = module_optimizer_get_stats();
    
    printf("=== 模块加载优化器统计信息 ===\n");
    printf("总加载次数: %lu\n", stats.total_loads);
    printf("缓存命中: %lu\n", stats.cache_hits);
    printf("缓存未命中: %lu\n", stats.cache_misses);
    printf("缓存命中率: %.2f%%\n", module_optimizer_get_cache_hit_rate() * 100);
    printf("平均加载时间: %.6f 秒\n", stats.avg_load_time);
    printf("内存使用: %lu KB\n", g_module_optimizer.pool_used_size / 1024);
    printf("内存峰值: %lu KB\n", stats.memory_peak / 1024);
    printf("符号缓存数量: %d\n", g_module_optimizer.symbol_count);
    printf("预加载条目: %d\n", g_module_optimizer.preload_count);
    printf("运行时间: %ld 秒\n", time(NULL) - g_module_optimizer.init_time);
    printf("=============================\n");
}

// 重置统计信息
void module_optimizer_reset_stats(void) {
    if (!g_module_optimizer.is_initialized) {
        return;
    }
    
    memset(&g_module_optimizer.stats, 0, sizeof(ModuleLoadingStats));
    g_module_optimizer.stats.last_update = time(NULL);
    printf("Module Optimizer: 统计信息已重置\n");
}

// 获取内存使用量
uint64_t module_optimizer_get_memory_usage(void) {
    if (!g_module_optimizer.is_initialized) {
        return 0;
    }

    return g_module_optimizer.pool_used_size;
}

// 性能比较
int module_optimizer_compare_performance(const ModuleLoadingStats* before, const ModuleLoadingStats* after) {
    if (!before || !after) {
        return -1;
    }

    printf("=== 性能优化对比 ===\n");

    // 加载时间对比
    if (before->avg_load_time > 0 && after->avg_load_time > 0) {
        double time_improvement = (before->avg_load_time - after->avg_load_time) / before->avg_load_time * 100;
        printf("平均加载时间: %.6f -> %.6f 秒 (改进: %.1f%%)\n",
               before->avg_load_time, after->avg_load_time, time_improvement);
    }

    // 缓存命中率对比
    double before_hit_rate = 0.0, after_hit_rate = 0.0;
    uint64_t before_total = before->cache_hits + before->cache_misses;
    uint64_t after_total = after->cache_hits + after->cache_misses;

    if (before_total > 0) before_hit_rate = (double)before->cache_hits / before_total;
    if (after_total > 0) after_hit_rate = (double)after->cache_hits / after_total;

    printf("缓存命中率: %.1f%% -> %.1f%% (改进: %.1f%%)\n",
           before_hit_rate * 100, after_hit_rate * 100,
           (after_hit_rate - before_hit_rate) * 100);

    // 内存使用对比
    if (before->memory_peak > 0 && after->memory_peak > 0) {
        double memory_improvement = (double)(before->memory_peak - after->memory_peak) / before->memory_peak * 100;
        printf("内存峰值: %lu -> %lu KB (改进: %.1f%%)\n",
               before->memory_peak / 1024, after->memory_peak / 1024, memory_improvement);
    }

    printf("==================\n");
    return 0;
}

// 停止预加载线程
void module_optimizer_stop_preload_thread(void) {
    if (preload_thread_running) {
        preload_thread_running = false;
        // 等待线程结束
        pthread_join(preload_thread, NULL);
    }
}
