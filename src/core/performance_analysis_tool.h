#ifndef PERFORMANCE_ANALYSIS_TOOL_H
#define PERFORMANCE_ANALYSIS_TOOL_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <stdio.h>

// T4.2 性能分析工具
// 目标: 能够准确识别性能瓶颈和优化点

#ifdef __cplusplus
extern "C" {
#endif

// 性能分析类型
typedef enum {
    PERF_ANALYSIS_CPU = 0x0001,
    PERF_ANALYSIS_MEMORY = 0x0002,
    PERF_ANALYSIS_IO = 0x0004,
    PERF_ANALYSIS_NETWORK = 0x0008,
    PERF_ANALYSIS_CACHE = 0x0010,
    PERF_ANALYSIS_BRANCH = 0x0020,
    PERF_ANALYSIS_JIT = 0x0040,
    PERF_ANALYSIS_MODULE = 0x0080,
    PERF_ANALYSIS_ALL = 0xFFFF
} PerformanceAnalysisType;

// 性能瓶颈类型
typedef enum {
    BOTTLENECK_CPU_BOUND,
    BOTTLENECK_MEMORY_BOUND,
    BOTTLENECK_IO_BOUND,
    BOTTLENECK_CACHE_MISS,
    BOTTLENECK_BRANCH_MISS,
    BOTTLENECK_JIT_COMPILE,
    BOTTLENECK_MODULE_LOAD,
    BOTTLENECK_LOCK_CONTENTION,
    BOTTLENECK_UNKNOWN
} BottleneckType;

// 性能指标
typedef struct {
    // CPU指标
    double cpu_utilization;
    uint64_t instruction_count;
    uint64_t cycle_count;
    double instructions_per_cycle;
    uint64_t context_switches;
    
    // 内存指标
    size_t memory_usage_bytes;
    size_t peak_memory_usage;
    uint64_t memory_allocations;
    uint64_t memory_deallocations;
    double memory_fragmentation_ratio;
    size_t cache_usage;
    
    // 缓存指标
    uint64_t cache_hits;
    uint64_t cache_misses;
    double cache_hit_ratio;
    uint64_t cache_evictions;
    
    // 分支预测指标
    uint64_t branch_predictions;
    uint64_t branch_mispredictions;
    double branch_prediction_accuracy;
    
    // I/O指标
    uint64_t disk_reads;
    uint64_t disk_writes;
    uint64_t disk_read_bytes;
    uint64_t disk_write_bytes;
    double disk_utilization;
    
    // 网络指标
    uint64_t network_packets_sent;
    uint64_t network_packets_received;
    uint64_t network_bytes_sent;
    uint64_t network_bytes_received;
    
    // JIT编译指标
    uint64_t jit_compilations;
    uint64_t jit_compilation_time_ns;
    uint64_t jit_code_size;
    double jit_compilation_ratio;
    
    // 模块加载指标
    uint64_t module_loads;
    uint64_t module_load_time_ns;
    uint64_t module_unloads;
    
    // 时间指标
    uint64_t timestamp_ns;
    uint64_t execution_time_ns;
    uint64_t idle_time_ns;
} PerformanceMetrics;

// 性能瓶颈信息
typedef struct PerformanceBottleneck {
    BottleneckType type;
    double severity;              // 严重程度 (0.0-1.0)
    double impact_percentage;     // 对性能的影响百分比
    char description[256];        // 瓶颈描述
    char suggestion[512];         // 优化建议
    
    // 相关指标
    PerformanceMetrics related_metrics;
    
    // 位置信息
    const char* function_name;
    const char* file_name;
    int line_number;
    
    struct PerformanceBottleneck* next;
} PerformanceBottleneck;

// 性能分析会话
typedef struct {
    uint32_t session_id;
    time_t start_time;
    time_t end_time;
    uint32_t analysis_types;      // 分析类型掩码
    
    PerformanceMetrics baseline_metrics;
    PerformanceMetrics current_metrics;
    
    PerformanceBottleneck* bottlenecks;
    int bottleneck_count;
    
    // 分析结果
    double overall_performance_score;
    double cpu_efficiency;
    double memory_efficiency;
    double io_efficiency;
    double cache_efficiency;
    
    char session_name[128];
    char analysis_summary[1024];
} PerformanceAnalysisSession;

// 性能分析配置
typedef struct {
    uint32_t enabled_analysis_types;
    bool enable_real_time_analysis;
    bool enable_bottleneck_detection;
    bool enable_optimization_suggestions;
    bool enable_comparative_analysis;
    
    double cpu_threshold;         // CPU使用率阈值
    double memory_threshold;      // 内存使用率阈值
    double cache_miss_threshold;  // 缓存未命中率阈值
    double branch_miss_threshold; // 分支预测失败率阈值
    
    int sampling_interval_ms;     // 采样间隔
    int analysis_window_size;     // 分析窗口大小
    int max_bottlenecks;          // 最大瓶颈数量
    
    const char* output_file;      // 输出文件
    bool enable_json_output;      // 启用JSON输出
    bool enable_csv_output;       // 启用CSV输出
} PerformanceAnalysisConfig;

// 性能分析器
typedef struct {
    PerformanceAnalysisConfig config;
    PerformanceAnalysisSession* current_session;
    
    PerformanceMetrics* metrics_history;
    int metrics_history_size;
    int metrics_history_count;
    
    bool is_initialized;
    bool is_analyzing;
    
    uint32_t next_session_id;
    time_t init_time;
    
    // 统计信息
    uint64_t total_sessions;
    uint64_t total_bottlenecks_detected;
    uint64_t total_analysis_time_ns;
    
    // 回调函数
    void (*bottleneck_callback)(const PerformanceBottleneck* bottleneck, void* user_data);
    void* callback_user_data;
} PerformanceAnalyzer;

// 全局性能分析器实例
extern PerformanceAnalyzer g_performance_analyzer;

// 初始化和清理
int performance_analyzer_init(const PerformanceAnalysisConfig* config);
void performance_analyzer_cleanup(void);
bool performance_analyzer_is_initialized(void);

// 配置管理
PerformanceAnalysisConfig performance_analyzer_get_default_config(void);
int performance_analyzer_set_config(const PerformanceAnalysisConfig* config);
PerformanceAnalysisConfig performance_analyzer_get_config(void);

// 分析会话管理
uint32_t performance_analyzer_start_session(const char* session_name, uint32_t analysis_types);
int performance_analyzer_end_session(uint32_t session_id);
PerformanceAnalysisSession* performance_analyzer_get_session(uint32_t session_id);

// 性能指标收集
int performance_analyzer_collect_metrics(PerformanceMetrics* metrics);
int performance_analyzer_add_metrics_sample(const PerformanceMetrics* metrics);
int performance_analyzer_get_current_metrics(PerformanceMetrics* metrics);

// 瓶颈检测和分析
int performance_analyzer_detect_bottlenecks(void);
PerformanceBottleneck* performance_analyzer_get_bottlenecks(void);
int performance_analyzer_get_bottleneck_count(void);

// 性能分析
double performance_analyzer_calculate_performance_score(const PerformanceMetrics* metrics);
double performance_analyzer_calculate_cpu_efficiency(const PerformanceMetrics* metrics);
double performance_analyzer_calculate_memory_efficiency(const PerformanceMetrics* metrics);
double performance_analyzer_calculate_cache_efficiency(const PerformanceMetrics* metrics);

// 优化建议
int performance_analyzer_generate_suggestions(void);
const char* performance_analyzer_get_optimization_suggestion(BottleneckType bottleneck_type);

// 比较分析
int performance_analyzer_compare_sessions(uint32_t session1_id, uint32_t session2_id);
int performance_analyzer_compare_metrics(const PerformanceMetrics* before, const PerformanceMetrics* after);

// 报告生成
int performance_analyzer_generate_report(uint32_t session_id, const char* output_file);
int performance_analyzer_export_metrics_csv(const char* filename);
int performance_analyzer_export_metrics_json(const char* filename);

// 实时分析
int performance_analyzer_start_real_time_analysis(void);
int performance_analyzer_stop_real_time_analysis(void);
bool performance_analyzer_is_real_time_active(void);

// 回调管理
void performance_analyzer_set_bottleneck_callback(void (*callback)(const PerformanceBottleneck*, void*), void* user_data);

// 实用工具
const char* performance_analyzer_bottleneck_type_to_string(BottleneckType type);
BottleneckType performance_analyzer_string_to_bottleneck_type(const char* str);
double performance_analyzer_get_time(void);

// 系统性能监控
int performance_analyzer_get_system_cpu_usage(double* cpu_usage);
int performance_analyzer_get_system_memory_usage(size_t* memory_usage, size_t* total_memory);
int performance_analyzer_get_system_io_stats(uint64_t* reads, uint64_t* writes);

// 进程性能监控
int performance_analyzer_get_process_cpu_usage(double* cpu_usage);
int performance_analyzer_get_process_memory_usage(size_t* memory_usage);
int performance_analyzer_get_process_io_stats(uint64_t* reads, uint64_t* writes);

// 性能分析宏
#define PERF_ANALYSIS_START(name, types) \
    performance_analyzer_start_session(name, types)

#define PERF_ANALYSIS_END(session_id) \
    performance_analyzer_end_session(session_id)

#define PERF_ANALYSIS_SAMPLE() \
    do { \
        PerformanceMetrics _metrics; \
        if (performance_analyzer_collect_metrics(&_metrics) == 0) { \
            performance_analyzer_add_metrics_sample(&_metrics); \
        } \
    } while(0)

// 性能计时宏
#define PERF_TIMER_DECLARE(name) \
    double _perf_timer_##name##_start = performance_analyzer_get_time()

#define PERF_TIMER_END(name) \
    do { \
        double _elapsed = performance_analyzer_get_time() - _perf_timer_##name##_start; \
        printf("Performance Timer [%s]: %.6f seconds\n", #name, _elapsed); \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif // PERFORMANCE_ANALYSIS_TOOL_H
