#include "memory_management_optimizer.h"
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

// 全局内存优化器实例
MemoryOptimizer g_memory_optimizer = {0};

// 获取高精度时间
static double get_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// 获取默认配置
MemoryOptimizerConfig memory_optimizer_get_default_config(void) {
    MemoryOptimizerConfig config = {
        .enable_memory_pools = true,
        .enable_alignment_opt = true,
        .enable_fragmentation_mgmt = true,
        .enable_cache_friendly = true,
        .enable_statistics = true,
        
        .small_pool_size = 64 * 1024,    // 64KB
        .medium_pool_size = 256 * 1024,  // 256KB
        .large_pool_size = 1024 * 1024,  // 1MB
        .temp_pool_size = 128 * 1024,    // 128KB
        
        .fragmentation_threshold = 0.3,  // 30%碎片化阈值
        .defrag_frequency = 100          // 每100次分配检查一次
    };
    return config;
}

// 初始化内存池
static int init_memory_pool(MemoryPool* pool, size_t size) {
    if (!pool || size == 0) return MEMORY_ERROR_CONFIG;
    
    pool->memory = malloc(size);
    if (!pool->memory) return MEMORY_ERROR_ALLOC;
    
    pool->size = size;
    pool->used = 0;
    pool->free = size;
    pool->free_list = NULL;
    pool->block_count = 0;
    pool->free_count = 0;
    
    return MEMORY_SUCCESS;
}

// 清理内存池
static void cleanup_memory_pool(MemoryPool* pool) {
    if (!pool) return;
    
    if (pool->memory) {
        free(pool->memory);
        pool->memory = NULL;
    }
    
    pool->size = 0;
    pool->used = 0;
    pool->free = 0;
    pool->free_list = NULL;
    pool->block_count = 0;
    pool->free_count = 0;
}

// 初始化内存优化器
int memory_optimizer_init(const MemoryOptimizerConfig* config) {
    if (g_memory_optimizer.is_initialized) {
        return MEMORY_SUCCESS;  // 已经初始化
    }
    
    // 使用默认配置或提供的配置
    if (config) {
        g_memory_optimizer.config = *config;
    } else {
        g_memory_optimizer.config = memory_optimizer_get_default_config();
    }
    
    // 初始化统计信息
    memset(&g_memory_optimizer.stats, 0, sizeof(MemoryOptimizerStats));
    g_memory_optimizer.stats.last_update = time(NULL);
    
    // 初始化内存池
    if (g_memory_optimizer.config.enable_memory_pools) {
        size_t pool_sizes[MEMORY_POOL_COUNT] = {
            g_memory_optimizer.config.small_pool_size,
            g_memory_optimizer.config.medium_pool_size,
            g_memory_optimizer.config.large_pool_size,
            0, // HUGE池动态分配
            g_memory_optimizer.config.temp_pool_size
        };
        
        for (int i = 0; i < MEMORY_POOL_COUNT; i++) {
            if (pool_sizes[i] > 0) {
                if (init_memory_pool(&g_memory_optimizer.pools[i], pool_sizes[i]) != MEMORY_SUCCESS) {
                    // 清理已初始化的池
                    for (int j = 0; j < i; j++) {
                        cleanup_memory_pool(&g_memory_optimizer.pools[j]);
                    }
                    return MEMORY_ERROR_INIT;
                }
            }
        }
    }
    
    g_memory_optimizer.is_initialized = true;
    g_memory_optimizer.init_time = time(NULL);
    g_memory_optimizer.defrag_counter = 0;
    g_memory_optimizer.last_fragmentation_ratio = 0.0;
    g_memory_optimizer.thread_safe = false; // 简化版本
    
    printf("Memory Optimizer: 初始化完成\n");
    printf("  内存池: %s\n", g_memory_optimizer.config.enable_memory_pools ? "启用" : "禁用");
    printf("  对齐优化: %s\n", g_memory_optimizer.config.enable_alignment_opt ? "启用" : "禁用");
    printf("  碎片管理: %s\n", g_memory_optimizer.config.enable_fragmentation_mgmt ? "启用" : "禁用");
    printf("  缓存友好: %s\n", g_memory_optimizer.config.enable_cache_friendly ? "启用" : "禁用");
    printf("  小块池: %zu KB\n", g_memory_optimizer.config.small_pool_size / 1024);
    printf("  中块池: %zu KB\n", g_memory_optimizer.config.medium_pool_size / 1024);
    printf("  大块池: %zu KB\n", g_memory_optimizer.config.large_pool_size / 1024);
    
    return MEMORY_SUCCESS;
}

// 清理内存优化器
void memory_optimizer_cleanup(void) {
    if (!g_memory_optimizer.is_initialized) {
        return;
    }
    
    // 清理所有内存池
    for (int i = 0; i < MEMORY_POOL_COUNT; i++) {
        cleanup_memory_pool(&g_memory_optimizer.pools[i]);
    }
    
    g_memory_optimizer.is_initialized = false;
    printf("Memory Optimizer: 清理完成\n");
}

// 检查是否已初始化
bool memory_optimizer_is_initialized(void) {
    return g_memory_optimizer.is_initialized;
}

// 获取内存池类型
MemoryPoolType memory_optimizer_get_pool_type(size_t size) {
    if (size <= SMALL_BLOCK_SIZE) {
        return MEMORY_POOL_SMALL;
    } else if (size <= MEDIUM_BLOCK_SIZE) {
        return MEMORY_POOL_MEDIUM;
    } else if (size <= LARGE_BLOCK_SIZE) {
        return MEMORY_POOL_LARGE;
    } else {
        return MEMORY_POOL_HUGE;
    }
}

// 从内存池分配内存
static void* pool_alloc(MemoryPool* pool, size_t size) {
    if (!pool || !pool->memory || size == 0) {
        return NULL;
    }
    
    // 对齐大小
    size_t aligned_size = align_size(size);
    
    // 检查是否有足够空间
    if (pool->used + aligned_size + sizeof(MemoryBlockHeader) > pool->size) {
        return NULL;
    }
    
    // 在池中分配内存
    MemoryBlockHeader* header = (MemoryBlockHeader*)((char*)pool->memory + pool->used);
    header->size = aligned_size;
    header->pool_type = MEMORY_POOL_SMALL; // 简化处理
    header->magic = MEMORY_MAGIC_ALLOCATED;
    header->is_free = false;
    header->next = NULL;
    header->prev = NULL;
    
    pool->used += aligned_size + sizeof(MemoryBlockHeader);
    pool->free = pool->size - pool->used;
    pool->block_count++;
    
    return (char*)header + sizeof(MemoryBlockHeader);
}

// 优化的内存分配
void* memory_optimizer_malloc(size_t size) {
    if (!g_memory_optimizer.is_initialized) {
        if (memory_optimizer_init(NULL) != MEMORY_SUCCESS) {
            return malloc(size); // 回退到系统malloc
        }
    }
    
    if (size == 0) return NULL;
    
    void* ptr = NULL;
    
    // 更新统计信息
    g_memory_optimizer.stats.total_allocations++;
    g_memory_optimizer.stats.total_allocated_bytes += size;
    
    if (g_memory_optimizer.config.enable_memory_pools) {
        MemoryPoolType pool_type = memory_optimizer_get_pool_type(size);
        
        if (pool_type != MEMORY_POOL_HUGE) {
            ptr = pool_alloc(&g_memory_optimizer.pools[pool_type], size);
            if (ptr) {
                g_memory_optimizer.stats.pool_hits++;
                g_memory_optimizer.stats.current_usage += size;
                if (g_memory_optimizer.stats.current_usage > g_memory_optimizer.stats.peak_usage) {
                    g_memory_optimizer.stats.peak_usage = g_memory_optimizer.stats.current_usage;
                }
                return ptr;
            }
        }
        
        g_memory_optimizer.stats.pool_misses++;
    }
    
    // 回退到系统分配
    if (g_memory_optimizer.config.enable_alignment_opt) {
        size_t aligned_size = align_size(size);
        ptr = malloc(aligned_size + sizeof(MemoryBlockHeader));
        if (ptr) {
            MemoryBlockHeader* header = (MemoryBlockHeader*)ptr;
            header->size = aligned_size;
            header->pool_type = MEMORY_POOL_HUGE;
            header->magic = MEMORY_MAGIC_ALLOCATED;
            header->is_free = false;
            header->next = NULL;
            header->prev = NULL;
            
            g_memory_optimizer.stats.current_usage += aligned_size;
            if (g_memory_optimizer.stats.current_usage > g_memory_optimizer.stats.peak_usage) {
                g_memory_optimizer.stats.peak_usage = g_memory_optimizer.stats.current_usage;
            }
            
            return (char*)ptr + sizeof(MemoryBlockHeader);
        }
    } else {
        ptr = malloc(size);
        if (ptr) {
            g_memory_optimizer.stats.current_usage += size;
            if (g_memory_optimizer.stats.current_usage > g_memory_optimizer.stats.peak_usage) {
                g_memory_optimizer.stats.peak_usage = g_memory_optimizer.stats.current_usage;
            }
        }
    }
    
    return ptr;
}

// 优化的内存释放
void memory_optimizer_free(void* ptr) {
    if (!ptr) return;
    
    if (!g_memory_optimizer.is_initialized) {
        free(ptr);
        return;
    }
    
    g_memory_optimizer.stats.total_frees++;
    
    // 检查是否是对齐分配的内存
    if (g_memory_optimizer.config.enable_alignment_opt) {
        MemoryBlockHeader* header = (MemoryBlockHeader*)((char*)ptr - sizeof(MemoryBlockHeader));
        
        // 验证魔数
        if (header->magic == MEMORY_MAGIC_ALLOCATED) {
            g_memory_optimizer.stats.total_freed_bytes += header->size;
            g_memory_optimizer.stats.current_usage -= header->size;
            
            header->magic = MEMORY_MAGIC_FREE;
            header->is_free = true;
            
            // 对于HUGE池，直接释放
            if (header->pool_type == MEMORY_POOL_HUGE) {
                free(header);
                return;
            }
            
            // 对于其他池，标记为空闲（简化处理）
            return;
        }
    }
    
    // 回退到系统释放
    free(ptr);
}

// 获取统计信息
MemoryOptimizerStats memory_optimizer_get_stats(void) {
    if (!g_memory_optimizer.is_initialized) {
        MemoryOptimizerStats empty_stats = {0};
        return empty_stats;
    }
    
    // 更新计算字段
    if (g_memory_optimizer.stats.total_allocations > 0) {
        g_memory_optimizer.stats.avg_allocation_size = 
            (double)g_memory_optimizer.stats.total_allocated_bytes / g_memory_optimizer.stats.total_allocations;
    }
    
    uint64_t total_pool_accesses = g_memory_optimizer.stats.pool_hits + g_memory_optimizer.stats.pool_misses;
    if (total_pool_accesses > 0) {
        g_memory_optimizer.stats.pool_hit_rate = 
            (double)g_memory_optimizer.stats.pool_hits / total_pool_accesses;
    }
    
    g_memory_optimizer.stats.last_update = time(NULL);
    return g_memory_optimizer.stats;
}

// 打印统计信息
void memory_optimizer_print_stats(void) {
    if (!g_memory_optimizer.is_initialized) {
        printf("Memory Optimizer: 未初始化\n");
        return;
    }
    
    MemoryOptimizerStats stats = memory_optimizer_get_stats();
    
    printf("=== 内存管理优化器统计信息 ===\n");
    printf("总分配次数: %lu\n", stats.total_allocations);
    printf("总释放次数: %lu\n", stats.total_frees);
    printf("总分配字节: %lu\n", stats.total_allocated_bytes);
    printf("总释放字节: %lu\n", stats.total_freed_bytes);
    printf("当前使用量: %lu 字节 (%.2f KB)\n", stats.current_usage, stats.current_usage / 1024.0);
    printf("峰值使用量: %lu 字节 (%.2f KB)\n", stats.peak_usage, stats.peak_usage / 1024.0);
    printf("内存池命中: %lu\n", stats.pool_hits);
    printf("内存池未命中: %lu\n", stats.pool_misses);
    printf("内存池命中率: %.2f%%\n", stats.pool_hit_rate * 100);
    printf("平均分配大小: %.2f 字节\n", stats.avg_allocation_size);
    printf("碎片化比率: %.2f%%\n", stats.fragmentation_ratio * 100);
    printf("运行时间: %ld 秒\n", time(NULL) - g_memory_optimizer.init_time);
    printf("=============================\n");
}

// 重置统计信息
void memory_optimizer_reset_stats(void) {
    if (!g_memory_optimizer.is_initialized) {
        return;
    }
    
    memset(&g_memory_optimizer.stats, 0, sizeof(MemoryOptimizerStats));
    g_memory_optimizer.stats.last_update = time(NULL);
    printf("Memory Optimizer: 统计信息已重置\n");
}

// 获取内存池命中率
double memory_optimizer_get_pool_hit_rate(void) {
    if (!g_memory_optimizer.is_initialized) {
        return 0.0;
    }
    
    uint64_t total = g_memory_optimizer.stats.pool_hits + g_memory_optimizer.stats.pool_misses;
    if (total == 0) {
        return 0.0;
    }
    
    return (double)g_memory_optimizer.stats.pool_hits / total;
}

// 获取当前内存使用量
size_t memory_optimizer_get_current_usage(void) {
    if (!g_memory_optimizer.is_initialized) {
        return 0;
    }
    
    return g_memory_optimizer.stats.current_usage;
}

// 获取峰值内存使用量
size_t memory_optimizer_get_peak_usage(void) {
    if (!g_memory_optimizer.is_initialized) {
        return 0;
    }
    
    return g_memory_optimizer.stats.peak_usage;
}
