#ifndef MODULE_STABILITY_H
#define MODULE_STABILITY_H

#include <stdint.h>
#include <stdbool.h>

/**
 * 模块系统稳定性增强
 * 
 * 提供模块加载、符号解析、内存管理和错误处理的稳定性改进
 */

// 模块健康状态
typedef enum {
    MODULE_HEALTH_UNKNOWN = 0,
    MODULE_HEALTH_HEALTHY = 1,
    MODULE_HEALTH_WARNING = 2,
    MODULE_HEALTH_ERROR = 3,
    MODULE_HEALTH_CRITICAL = 4
} ModuleHealthStatus;

// 模块统计信息
typedef struct {
    uint64_t load_count;        // 加载次数
    uint64_t unload_count;      // 卸载次数
    uint64_t symbol_resolve_count; // 符号解析次数
    uint64_t error_count;       // 错误次数
    uint64_t memory_usage;      // 内存使用量
    double last_load_time;      // 最后加载时间
    ModuleHealthStatus health;  // 健康状态
} ModuleStats;

// 模块缓存项
typedef struct ModuleCacheEntry {
    char* module_name;
    void* handle;
    ModuleStats stats;
    bool is_loaded;
    struct ModuleCacheEntry* next;
} ModuleCacheEntry;

// 模块系统配置
typedef struct {
    uint32_t max_cached_modules;    // 最大缓存模块数
    uint32_t max_load_retries;      // 最大加载重试次数
    uint32_t health_check_interval; // 健康检查间隔(秒)
    bool enable_auto_recovery;      // 启用自动恢复
    bool enable_memory_monitoring;  // 启用内存监控
} ModuleSystemConfig;

// 全局模块系统状态
typedef struct {
    ModuleCacheEntry* cache_head;
    ModuleSystemConfig config;
    uint64_t total_modules_loaded;
    uint64_t total_errors;
    bool is_initialized;
} ModuleSystemState;

// 初始化和清理
int module_stability_init(const ModuleSystemConfig* config);
void module_stability_cleanup(void);

// 模块管理
void* stable_module_load(const char* module_name);
int stable_module_unload(const char* module_name);
void* stable_module_resolve(const char* module_name, const char* symbol_name);

// 健康监控
ModuleHealthStatus module_get_health(const char* module_name);
ModuleStats* module_get_stats(const char* module_name);
int module_run_health_check(const char* module_name);

// 错误处理和恢复
int module_attempt_recovery(const char* module_name);
void module_clear_error_state(const char* module_name);

// 内存管理
uint64_t module_get_memory_usage(const char* module_name);
int module_optimize_memory(void);

// 缓存管理
int module_cache_cleanup(void);
int module_cache_preload(const char** module_names, int count);

// 统计和诊断
void module_print_system_stats(void);
void module_print_module_stats(const char* module_name);
int module_export_diagnostics(const char* filename);

// 配置管理
int module_update_config(const ModuleSystemConfig* new_config);
ModuleSystemConfig* module_get_config(void);

// 默认配置
static const ModuleSystemConfig DEFAULT_MODULE_CONFIG = {
    .max_cached_modules = 32,
    .max_load_retries = 3,
    .health_check_interval = 60,
    .enable_auto_recovery = true,
    .enable_memory_monitoring = true
};

// 错误代码
#define MODULE_SUCCESS              0
#define MODULE_ERROR_INIT_FAILED   -1
#define MODULE_ERROR_NOT_FOUND     -2
#define MODULE_ERROR_LOAD_FAILED   -3
#define MODULE_ERROR_SYMBOL_NOT_FOUND -4
#define MODULE_ERROR_MEMORY_ERROR  -5
#define MODULE_ERROR_INVALID_PARAM -6
#define MODULE_ERROR_SYSTEM_ERROR  -7

// 日志级别
typedef enum {
    MODULE_LOG_DEBUG = 0,
    MODULE_LOG_INFO = 1,
    MODULE_LOG_WARNING = 2,
    MODULE_LOG_ERROR = 3,
    MODULE_LOG_CRITICAL = 4
} ModuleLogLevel;

// 日志函数
void module_log(ModuleLogLevel level, const char* format, ...);

// 性能监控
typedef struct {
    double load_time;
    double resolve_time;
    uint64_t memory_peak;
    uint64_t cache_hits;
    uint64_t cache_misses;
} ModulePerformanceMetrics;

ModulePerformanceMetrics* module_get_performance_metrics(const char* module_name);
void module_reset_performance_metrics(const char* module_name);

// 安全检查
int module_verify_integrity(const char* module_name);
int module_check_permissions(const char* module_name);

// 并发安全（为未来扩展预留）
int module_lock_system(void);
int module_unlock_system(void);
int module_lock_module(const char* module_name);
int module_unlock_module(const char* module_name);

#endif // MODULE_STABILITY_H
