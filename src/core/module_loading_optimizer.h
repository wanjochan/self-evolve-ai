#ifndef MODULE_LOADING_OPTIMIZER_H
#define MODULE_LOADING_OPTIMIZER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// T3.1 模块加载性能优化器
// 目标: 模块加载时间减少30%，内存占用优化20%

#ifdef __cplusplus
extern "C" {
#endif

// 优化配置
typedef struct {
    bool enable_preloading;          // 启用预加载
    bool enable_lazy_loading;        // 启用延迟加载
    bool enable_symbol_cache;        // 启用符号缓存
    bool enable_memory_pool;         // 启用内存池
    bool enable_compression;         // 启用压缩存储
    
    int preload_thread_count;        // 预加载线程数
    int symbol_cache_size;           // 符号缓存大小
    int memory_pool_size;            // 内存池大小
    double cache_hit_threshold;      // 缓存命中率阈值
} ModuleLoadingOptimizerConfig;

// 性能统计
typedef struct {
    uint64_t total_loads;            // 总加载次数
    uint64_t cache_hits;             // 缓存命中次数
    uint64_t cache_misses;           // 缓存未命中次数
    uint64_t preload_hits;           // 预加载命中次数
    
    double total_load_time;          // 总加载时间
    double avg_load_time;            // 平均加载时间
    double cache_hit_time;           // 缓存命中平均时间
    double cache_miss_time;          // 缓存未命中平均时间
    
    uint64_t memory_allocated;       // 已分配内存
    uint64_t memory_peak;            // 内存峰值
    uint64_t memory_saved;           // 节省的内存
    
    time_t last_update;              // 最后更新时间
} ModuleLoadingStats;

// 预加载条目
typedef struct PreloadEntry {
    char* module_name;               // 模块名称
    char* module_path;               // 模块路径
    void* cached_data;               // 缓存的数据
    size_t data_size;                // 数据大小
    time_t load_time;                // 加载时间
    int access_count;                // 访问次数
    double priority;                 // 优先级
    struct PreloadEntry* next;       // 链表下一项
} PreloadEntry;

// 符号缓存条目
typedef struct SymbolCacheEntry {
    char* symbol_name;               // 符号名称
    void* symbol_addr;               // 符号地址
    char* module_name;               // 所属模块
    uint64_t hash;                   // 符号哈希
    time_t last_access;              // 最后访问时间
    int access_count;                // 访问次数
    struct SymbolCacheEntry* next;   // 哈希链表
} SymbolCacheEntry;

// 内存池块
typedef struct MemoryPoolBlock {
    void* memory;                    // 内存块
    size_t size;                     // 块大小
    size_t used;                     // 已使用大小
    bool is_free;                    // 是否空闲
    struct MemoryPoolBlock* next;    // 下一个块
} MemoryPoolBlock;

// 模块加载优化器
typedef struct {
    ModuleLoadingOptimizerConfig config;  // 配置
    ModuleLoadingStats stats;             // 统计信息
    
    // 预加载系统
    PreloadEntry* preload_cache[256];     // 预加载缓存哈希表
    int preload_count;                    // 预加载条目数
    
    // 符号缓存系统
    SymbolCacheEntry* symbol_cache[1024]; // 符号缓存哈希表
    int symbol_count;                     // 符号数量
    
    // 内存池系统
    MemoryPoolBlock* memory_pool;         // 内存池
    size_t pool_total_size;               // 池总大小
    size_t pool_used_size;                // 池已用大小
    
    // 状态管理
    bool is_initialized;                  // 是否已初始化
    bool is_optimizing;                   // 是否正在优化
    time_t init_time;                     // 初始化时间
} ModuleLoadingOptimizer;

// 全局优化器实例
extern ModuleLoadingOptimizer g_module_optimizer;

// 初始化和清理
int module_optimizer_init(const ModuleLoadingOptimizerConfig* config);
void module_optimizer_cleanup(void);
bool module_optimizer_is_initialized(void);

// 配置管理
int module_optimizer_set_config(const ModuleLoadingOptimizerConfig* config);
ModuleLoadingOptimizerConfig module_optimizer_get_config(void);
ModuleLoadingOptimizerConfig module_optimizer_get_default_config(void);

// 优化接口
void* module_optimizer_load(const char* module_name);
int module_optimizer_unload(const char* module_name);
void* module_optimizer_resolve_symbol(const char* module_name, const char* symbol_name);

// 预加载管理
int module_optimizer_preload_module(const char* module_name);
int module_optimizer_preload_modules(const char* module_names[], int count);
int module_optimizer_start_preload_thread(void);
void module_optimizer_stop_preload_thread(void);

// 符号缓存管理
int module_optimizer_cache_symbol(const char* module_name, const char* symbol_name, void* symbol_addr);
void* module_optimizer_lookup_symbol(const char* symbol_name);
int module_optimizer_clear_symbol_cache(void);

// 内存池管理
void* module_optimizer_alloc(size_t size);
void module_optimizer_free(void* ptr);
int module_optimizer_compact_memory_pool(void);

// 性能统计
ModuleLoadingStats module_optimizer_get_stats(void);
void module_optimizer_reset_stats(void);
double module_optimizer_get_cache_hit_rate(void);
double module_optimizer_get_avg_load_time(void);
uint64_t module_optimizer_get_memory_usage(void);

// 优化分析
int module_optimizer_analyze_performance(void);
int module_optimizer_suggest_optimizations(void);
int module_optimizer_auto_tune(void);

// 调试和监控
void module_optimizer_print_stats(void);
void module_optimizer_print_cache_info(void);
int module_optimizer_export_stats(const char* filename);

// 实用工具
uint64_t module_optimizer_hash_string(const char* str);
double module_optimizer_get_time(void);
int module_optimizer_compare_performance(const ModuleLoadingStats* before, const ModuleLoadingStats* after);

#ifdef __cplusplus
}
#endif

#endif // MODULE_LOADING_OPTIMIZER_H
