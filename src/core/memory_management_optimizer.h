#ifndef MEMORY_MANAGEMENT_OPTIMIZER_H
#define MEMORY_MANAGEMENT_OPTIMIZER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>

// T3.3 内存管理优化器
// 目标: 内存使用效率提升15%，减少碎片化

#ifdef __cplusplus
extern "C" {
#endif

// 内存池类型
typedef enum {
    MEMORY_POOL_SMALL,      // 小块内存 (<=64字节)
    MEMORY_POOL_MEDIUM,     // 中块内存 (65-512字节)
    MEMORY_POOL_LARGE,      // 大块内存 (513-4096字节)
    MEMORY_POOL_HUGE,       // 超大块内存 (>4096字节)
    MEMORY_POOL_TEMP,       // 临时内存
    MEMORY_POOL_COUNT
} MemoryPoolType;

// 内存块大小类别
#define SMALL_BLOCK_SIZE    64
#define MEDIUM_BLOCK_SIZE   512
#define LARGE_BLOCK_SIZE    4096
#define HUGE_BLOCK_THRESHOLD 4096

// 内存对齐
#define MEMORY_ALIGNMENT    16
#define CACHE_LINE_SIZE     64

// 内存池配置
typedef struct {
    bool enable_memory_pools;        // 启用内存池
    bool enable_alignment_opt;       // 启用对齐优化
    bool enable_fragmentation_mgmt;  // 启用碎片管理
    bool enable_cache_friendly;      // 启用缓存友好分配
    bool enable_statistics;          // 启用统计信息
    
    size_t small_pool_size;          // 小块内存池大小
    size_t medium_pool_size;         // 中块内存池大小
    size_t large_pool_size;          // 大块内存池大小
    size_t temp_pool_size;           // 临时内存池大小
    
    double fragmentation_threshold;  // 碎片化阈值
    int defrag_frequency;            // 碎片整理频率
} MemoryOptimizerConfig;

// 内存块头
typedef struct MemoryBlockHeader {
    size_t size;                     // 块大小
    MemoryPoolType pool_type;        // 所属内存池
    uint32_t magic;                  // 魔数验证
    bool is_free;                    // 是否空闲
    struct MemoryBlockHeader* next;  // 下一个块
    struct MemoryBlockHeader* prev;  // 前一个块
} MemoryBlockHeader;

// 内存池
typedef struct {
    void* memory;                    // 内存区域
    size_t size;                     // 总大小
    size_t used;                     // 已使用大小
    size_t free;                     // 空闲大小
    MemoryBlockHeader* free_list;    // 空闲块链表
    int block_count;                 // 块数量
    int free_count;                  // 空闲块数量
} MemoryPool;

// 内存统计
typedef struct {
    uint64_t total_allocations;      // 总分配次数
    uint64_t total_frees;            // 总释放次数
    uint64_t total_allocated_bytes;  // 总分配字节数
    uint64_t total_freed_bytes;      // 总释放字节数
    uint64_t current_usage;          // 当前使用量
    uint64_t peak_usage;             // 峰值使用量
    
    uint64_t pool_hits;              // 内存池命中次数
    uint64_t pool_misses;            // 内存池未命中次数
    uint64_t fragmentation_events;   // 碎片化事件次数
    uint64_t defrag_operations;      // 碎片整理操作次数
    
    double avg_allocation_size;      // 平均分配大小
    double fragmentation_ratio;      // 碎片化比率
    double pool_hit_rate;            // 内存池命中率
    
    time_t last_update;              // 最后更新时间
} MemoryOptimizerStats;

// 优化的内存管理器
typedef struct {
    MemoryOptimizerConfig config;    // 配置
    MemoryOptimizerStats stats;      // 统计信息
    
    MemoryPool pools[MEMORY_POOL_COUNT]; // 内存池数组
    
    bool is_initialized;             // 是否已初始化
    time_t init_time;                // 初始化时间
    
    // 碎片管理
    int defrag_counter;              // 碎片整理计数器
    double last_fragmentation_ratio; // 上次碎片化比率
    
    // 线程安全 (简化版本)
    bool thread_safe;                // 是否线程安全
} MemoryOptimizer;

// 全局内存优化器实例
extern MemoryOptimizer g_memory_optimizer;

// 初始化和清理
int memory_optimizer_init(const MemoryOptimizerConfig* config);
void memory_optimizer_cleanup(void);
bool memory_optimizer_is_initialized(void);

// 配置管理
MemoryOptimizerConfig memory_optimizer_get_default_config(void);
int memory_optimizer_set_config(const MemoryOptimizerConfig* config);
MemoryOptimizerConfig memory_optimizer_get_config(void);

// 优化的内存分配接口
void* memory_optimizer_malloc(size_t size);
void* memory_optimizer_calloc(size_t count, size_t size);
void* memory_optimizer_realloc(void* ptr, size_t size);
void memory_optimizer_free(void* ptr);

// 内存池分配
void* memory_optimizer_pool_alloc(MemoryPoolType pool_type, size_t size);
void memory_optimizer_pool_free(void* ptr);

// 对齐分配
void* memory_optimizer_aligned_alloc(size_t alignment, size_t size);
void memory_optimizer_aligned_free(void* ptr);

// 临时内存分配
void* memory_optimizer_temp_alloc(size_t size);
void memory_optimizer_temp_free_all(void);

// 内存池管理
int memory_optimizer_init_pool(MemoryPoolType pool_type, size_t size);
void memory_optimizer_cleanup_pool(MemoryPoolType pool_type);
int memory_optimizer_resize_pool(MemoryPoolType pool_type, size_t new_size);

// 碎片管理
int memory_optimizer_defragment(MemoryPoolType pool_type);
int memory_optimizer_defragment_all(void);
double memory_optimizer_get_fragmentation_ratio(MemoryPoolType pool_type);
bool memory_optimizer_needs_defragmentation(MemoryPoolType pool_type);

// 统计和监控
MemoryOptimizerStats memory_optimizer_get_stats(void);
void memory_optimizer_reset_stats(void);
void memory_optimizer_print_stats(void);
void memory_optimizer_print_pool_info(MemoryPoolType pool_type);
int memory_optimizer_export_stats(const char* filename);

// 性能分析
double memory_optimizer_get_pool_hit_rate(void);
double memory_optimizer_get_avg_allocation_size(void);
size_t memory_optimizer_get_current_usage(void);
size_t memory_optimizer_get_peak_usage(void);

// 内存检查和验证
bool memory_optimizer_validate_heap(void);
bool memory_optimizer_check_leaks(void);
int memory_optimizer_find_corruption(void);

// 实用工具
MemoryPoolType memory_optimizer_get_pool_type(size_t size);
size_t memory_optimizer_align_size(size_t size, size_t alignment);
bool memory_optimizer_is_aligned(void* ptr, size_t alignment);

// 内部函数
static inline size_t align_size(size_t size) {
    return (size + MEMORY_ALIGNMENT - 1) & ~(MEMORY_ALIGNMENT - 1);
}

static inline bool is_power_of_two(size_t n) {
    return n && !(n & (n - 1));
}

static inline size_t next_power_of_two(size_t n) {
    if (n <= 1) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    if (sizeof(size_t) > 4) {
        n |= n >> 32;
    }
    return n + 1;
}

// 魔数定义
#define MEMORY_MAGIC_ALLOCATED  0xDEADBEEF
#define MEMORY_MAGIC_FREE       0xFEEDFACE
#define MEMORY_MAGIC_CORRUPTED  0xBADC0FFE

// 错误码
#define MEMORY_SUCCESS          0
#define MEMORY_ERROR_INIT       -1
#define MEMORY_ERROR_CONFIG     -2
#define MEMORY_ERROR_ALLOC      -3
#define MEMORY_ERROR_FREE       -4
#define MEMORY_ERROR_CORRUPT    -5
#define MEMORY_ERROR_LEAK       -6

#ifdef __cplusplus
}
#endif

#endif // MEMORY_MANAGEMENT_OPTIMIZER_H
