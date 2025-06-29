/**
 * dynamic_runtime_selector.h - 动态Runtime选择器
 * 
 * 根据程序需求和系统环境自动选择最合适的Runtime版本
 */

#ifndef DYNAMIC_RUNTIME_SELECTOR_H
#define DYNAMIC_RUNTIME_SELECTOR_H

#include <stdint.h>
#include <stdbool.h>
#include "rt_format_standard.h"

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// 程序需求分析
// ===============================================

typedef struct {
    uint32_t min_memory;                 // 最小内存需求 (字节)
    uint32_t min_stack_size;             // 最小栈大小
    uint32_t min_heap_size;              // 最小堆大小
    bool needs_floating_point;           // 是否需要浮点运算
    bool needs_threading;                // 是否需要多线程
    bool needs_file_io;                  // 是否需要文件I/O
    bool needs_network;                  // 是否需要网络功能
    bool needs_graphics;                 // 是否需要图形功能
    uint32_t libc_functions_used;        // 使用的libc函数数量
    uint32_t optimization_preference;    // 优化偏好 (0=大小, 1=速度, 2=平衡)
} ProgramRequirements;

typedef struct {
    char name[64];                       // Runtime名称
    char version[16];                    // 版本号
    char filename[256];                  // 文件路径
    RTArchitecture architecture;         // 支持的架构
    RTOperatingSystem os;                // 支持的操作系统
    RTABI abi;                          // ABI约定
    
    uint32_t file_size;                  // 文件大小
    uint32_t memory_footprint;           // 内存占用
    uint32_t startup_time;               // 启动时间 (微秒)
    uint32_t execution_speed;            // 执行速度评分 (1-100)
    
    // 功能支持
    bool supports_floating_point;
    bool supports_threading;
    bool supports_file_io;
    bool supports_network;
    bool supports_graphics;
    uint32_t max_libc_functions;
    
    // 资源限制
    uint32_t max_memory;
    uint32_t max_stack_size;
    uint32_t max_heap_size;
    
    bool available;                      // 是否可用
    uint32_t compatibility_score;        // 兼容性评分
} RuntimeInfo;

typedef struct {
    RuntimeInfo* runtimes;               // Runtime列表
    uint32_t runtime_count;              // Runtime数量
    uint32_t capacity;                   // 容量
    
    RTArchitecture current_arch;         // 当前架构
    RTOperatingSystem current_os;        // 当前操作系统
    RTABI current_abi;                   // 当前ABI
    
    uint32_t available_memory;           // 可用内存
    uint32_t cpu_cores;                  // CPU核心数
    bool has_fpu;                        // 是否有浮点单元
} RuntimeSelector;

// ===============================================
// 选择策略
// ===============================================

typedef enum {
    STRATEGY_FASTEST,                    // 最快执行速度
    STRATEGY_SMALLEST,                   // 最小文件大小
    STRATEGY_BALANCED,                   // 平衡性能和大小
    STRATEGY_MEMORY_EFFICIENT,          // 内存效率优先
    STRATEGY_COMPATIBILITY,              // 兼容性优先
    STRATEGY_CUSTOM                      // 自定义策略
} SelectionStrategy;

typedef struct {
    SelectionStrategy strategy;
    uint32_t speed_weight;               // 速度权重 (0-100)
    uint32_t size_weight;                // 大小权重 (0-100)
    uint32_t memory_weight;              // 内存权重 (0-100)
    uint32_t compatibility_weight;       // 兼容性权重 (0-100)
} SelectionCriteria;

// ===============================================
// 函数声明
// ===============================================

/**
 * 初始化Runtime选择器
 */
RuntimeSelector* runtime_selector_init(void);

/**
 * 释放Runtime选择器
 */
void runtime_selector_free(RuntimeSelector* selector);

/**
 * 扫描并注册可用的Runtime
 */
int runtime_selector_scan_runtimes(RuntimeSelector* selector, const char* runtime_dir);

/**
 * 手动注册Runtime
 */
int runtime_selector_register_runtime(RuntimeSelector* selector, const RuntimeInfo* runtime);

/**
 * 分析程序需求
 */
ProgramRequirements runtime_analyze_program(const char* program_file);

/**
 * 根据需求选择最佳Runtime
 */
const RuntimeInfo* runtime_select_best(RuntimeSelector* selector, 
                                      const ProgramRequirements* requirements,
                                      const SelectionCriteria* criteria);

/**
 * 计算Runtime兼容性评分
 */
uint32_t runtime_calculate_compatibility(const RuntimeInfo* runtime, 
                                        const ProgramRequirements* requirements);

/**
 * 计算Runtime性能评分
 */
uint32_t runtime_calculate_performance_score(const RuntimeInfo* runtime,
                                            const SelectionCriteria* criteria);

/**
 * 验证Runtime可用性
 */
bool runtime_verify_availability(const RuntimeInfo* runtime);

/**
 * 获取默认选择策略
 */
SelectionCriteria runtime_get_default_criteria(SelectionStrategy strategy);

/**
 * 列出所有可用的Runtime
 */
void runtime_list_available(const RuntimeSelector* selector);

/**
 * 获取Runtime详细信息
 */
int runtime_get_info(const char* runtime_file, RuntimeInfo* info);

/**
 * 自动选择并加载Runtime
 */
int runtime_auto_select_and_load(const char* program_file,
                                 const char* runtime_dir,
                                 SelectionStrategy strategy);

/**
 * 缓存Runtime选择结果
 */
int runtime_cache_selection(const char* program_file, const RuntimeInfo* runtime);

/**
 * 从缓存获取Runtime选择
 */
const RuntimeInfo* runtime_get_cached_selection(const char* program_file);

#ifdef __cplusplus
}
#endif

#endif // DYNAMIC_RUNTIME_SELECTOR_H
