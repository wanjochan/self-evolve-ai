#include "performance_analysis_tool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <math.h>

// 全局性能分析器实例
PerformanceAnalyzer g_performance_analyzer = {0};

// 获取高精度时间
double performance_analyzer_get_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// 获取默认配置
PerformanceAnalysisConfig performance_analyzer_get_default_config(void) {
    PerformanceAnalysisConfig config = {
        .enabled_analysis_types = PERF_ANALYSIS_ALL,
        .enable_real_time_analysis = true,
        .enable_bottleneck_detection = true,
        .enable_optimization_suggestions = true,
        .enable_comparative_analysis = true,
        
        .cpu_threshold = 0.8,           // 80% CPU使用率阈值
        .memory_threshold = 0.9,        // 90% 内存使用率阈值
        .cache_miss_threshold = 0.1,    // 10% 缓存未命中率阈值
        .branch_miss_threshold = 0.05,  // 5% 分支预测失败率阈值
        
        .sampling_interval_ms = 100,    // 100ms采样间隔
        .analysis_window_size = 100,    // 100个样本的分析窗口
        .max_bottlenecks = 10,          // 最多检测10个瓶颈
        
        .output_file = NULL,
        .enable_json_output = true,
        .enable_csv_output = true
    };
    return config;
}

// 初始化性能分析器
int performance_analyzer_init(const PerformanceAnalysisConfig* config) {
    if (g_performance_analyzer.is_initialized) {
        return 0; // 已经初始化
    }
    
    // 使用默认配置或提供的配置
    if (config) {
        g_performance_analyzer.config = *config;
    } else {
        g_performance_analyzer.config = performance_analyzer_get_default_config();
    }
    
    // 初始化指标历史缓冲区
    g_performance_analyzer.metrics_history_size = g_performance_analyzer.config.analysis_window_size;
    g_performance_analyzer.metrics_history = malloc(
        g_performance_analyzer.metrics_history_size * sizeof(PerformanceMetrics));
    
    if (!g_performance_analyzer.metrics_history) {
        return -1;
    }
    
    memset(g_performance_analyzer.metrics_history, 0, 
           g_performance_analyzer.metrics_history_size * sizeof(PerformanceMetrics));
    
    g_performance_analyzer.metrics_history_count = 0;
    g_performance_analyzer.is_initialized = true;
    g_performance_analyzer.is_analyzing = false;
    g_performance_analyzer.next_session_id = 1;
    g_performance_analyzer.init_time = time(NULL);
    
    // 初始化统计信息
    g_performance_analyzer.total_sessions = 0;
    g_performance_analyzer.total_bottlenecks_detected = 0;
    g_performance_analyzer.total_analysis_time_ns = 0;
    
    printf("Performance Analyzer: 初始化完成\n");
    printf("  分析类型: 0x%X\n", g_performance_analyzer.config.enabled_analysis_types);
    printf("  实时分析: %s\n", g_performance_analyzer.config.enable_real_time_analysis ? "启用" : "禁用");
    printf("  瓶颈检测: %s\n", g_performance_analyzer.config.enable_bottleneck_detection ? "启用" : "禁用");
    printf("  采样间隔: %d ms\n", g_performance_analyzer.config.sampling_interval_ms);
    printf("  分析窗口: %d 样本\n", g_performance_analyzer.config.analysis_window_size);
    
    return 0;
}

// 清理性能分析器
void performance_analyzer_cleanup(void) {
    if (!g_performance_analyzer.is_initialized) {
        return;
    }
    
    // 停止实时分析
    if (g_performance_analyzer.is_analyzing) {
        performance_analyzer_stop_real_time_analysis();
    }
    
    // 清理当前会话
    if (g_performance_analyzer.current_session) {
        // 清理瓶颈链表
        PerformanceBottleneck* current = g_performance_analyzer.current_session->bottlenecks;
        while (current) {
            PerformanceBottleneck* next = current->next;
            free(current);
            current = next;
        }
        
        free(g_performance_analyzer.current_session);
        g_performance_analyzer.current_session = NULL;
    }
    
    // 清理指标历史
    if (g_performance_analyzer.metrics_history) {
        free(g_performance_analyzer.metrics_history);
        g_performance_analyzer.metrics_history = NULL;
    }
    
    g_performance_analyzer.is_initialized = false;
    printf("Performance Analyzer: 清理完成\n");
}

// 检查是否已初始化
bool performance_analyzer_is_initialized(void) {
    return g_performance_analyzer.is_initialized;
}

// 收集系统性能指标
int performance_analyzer_collect_metrics(PerformanceMetrics* metrics) {
    if (!metrics) return -1;
    
    memset(metrics, 0, sizeof(PerformanceMetrics));
    
    // 获取当前时间戳
    struct timeval tv;
    gettimeofday(&tv, NULL);
    metrics->timestamp_ns = tv.tv_sec * 1000000000ULL + tv.tv_usec * 1000ULL;
    
    // 获取系统资源使用情况
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        // CPU时间 (用户态 + 内核态)
        metrics->execution_time_ns = (usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) * 1000000000ULL +
                                   (usage.ru_utime.tv_usec + usage.ru_stime.tv_usec) * 1000ULL;
        
        // 内存使用 (RSS)
        metrics->memory_usage_bytes = usage.ru_maxrss * 1024; // Linux: KB, macOS: bytes
        
        // 页面错误
        metrics->cache_misses = usage.ru_majflt; // 主要页面错误
        metrics->cache_hits = usage.ru_minflt;   // 次要页面错误
        
        // 上下文切换
        metrics->context_switches = usage.ru_nvcsw + usage.ru_nivcsw;
    }
    
    // 计算CPU利用率 (简化版本)
    static double last_cpu_time = 0;
    static double last_wall_time = 0;
    
    double current_wall_time = performance_analyzer_get_time();
    double current_cpu_time = metrics->execution_time_ns / 1000000000.0;
    
    if (last_wall_time > 0) {
        double wall_delta = current_wall_time - last_wall_time;
        double cpu_delta = current_cpu_time - last_cpu_time;
        
        if (wall_delta > 0) {
            metrics->cpu_utilization = cpu_delta / wall_delta;
            if (metrics->cpu_utilization > 1.0) metrics->cpu_utilization = 1.0;
            if (metrics->cpu_utilization < 0.0) metrics->cpu_utilization = 0.0;
        }
    }
    
    last_cpu_time = current_cpu_time;
    last_wall_time = current_wall_time;
    
    // 计算缓存命中率
    uint64_t total_cache_accesses = metrics->cache_hits + metrics->cache_misses;
    if (total_cache_accesses > 0) {
        metrics->cache_hit_ratio = (double)metrics->cache_hits / total_cache_accesses;
    }
    
    // 模拟其他指标 (在实际实现中应该从系统获取)
    metrics->instruction_count = metrics->execution_time_ns / 1000; // 简化估算
    metrics->cycle_count = metrics->instruction_count * 2; // 假设平均2个周期/指令
    
    if (metrics->cycle_count > 0) {
        metrics->instructions_per_cycle = (double)metrics->instruction_count / metrics->cycle_count;
    }
    
    // 分支预测 (模拟数据)
    metrics->branch_predictions = metrics->instruction_count / 10; // 假设10%是分支指令
    metrics->branch_mispredictions = metrics->branch_predictions / 20; // 假设5%分支预测失败
    
    if (metrics->branch_predictions > 0) {
        metrics->branch_prediction_accuracy = 1.0 - 
            (double)metrics->branch_mispredictions / metrics->branch_predictions;
    }
    
    return 0;
}

// 添加指标样本
int performance_analyzer_add_metrics_sample(const PerformanceMetrics* metrics) {
    if (!g_performance_analyzer.is_initialized || !metrics) {
        return -1;
    }
    
    // 循环缓冲区
    int index = g_performance_analyzer.metrics_history_count % g_performance_analyzer.metrics_history_size;
    g_performance_analyzer.metrics_history[index] = *metrics;
    g_performance_analyzer.metrics_history_count++;
    
    return 0;
}

// 开始分析会话
uint32_t performance_analyzer_start_session(const char* session_name, uint32_t analysis_types) {
    if (!g_performance_analyzer.is_initialized) {
        if (performance_analyzer_init(NULL) != 0) {
            return 0;
        }
    }
    
    // 结束当前会话
    if (g_performance_analyzer.current_session) {
        performance_analyzer_end_session(g_performance_analyzer.current_session->session_id);
    }
    
    // 创建新会话
    PerformanceAnalysisSession* session = malloc(sizeof(PerformanceAnalysisSession));
    if (!session) {
        return 0;
    }
    
    memset(session, 0, sizeof(PerformanceAnalysisSession));
    
    session->session_id = g_performance_analyzer.next_session_id++;
    session->start_time = time(NULL);
    session->analysis_types = analysis_types;
    session->bottlenecks = NULL;
    session->bottleneck_count = 0;
    
    if (session_name) {
        strncpy(session->session_name, session_name, sizeof(session->session_name) - 1);
    } else {
        snprintf(session->session_name, sizeof(session->session_name), "Session_%u", session->session_id);
    }
    
    // 收集基线指标
    performance_analyzer_collect_metrics(&session->baseline_metrics);
    
    g_performance_analyzer.current_session = session;
    g_performance_analyzer.total_sessions++;
    
    printf("Performance Analysis Session Started: %s (ID: %u)\n", 
           session->session_name, session->session_id);
    
    return session->session_id;
}

// 结束分析会话
int performance_analyzer_end_session(uint32_t session_id) {
    if (!g_performance_analyzer.current_session || 
        g_performance_analyzer.current_session->session_id != session_id) {
        return -1;
    }
    
    PerformanceAnalysisSession* session = g_performance_analyzer.current_session;
    session->end_time = time(NULL);
    
    // 收集最终指标
    performance_analyzer_collect_metrics(&session->current_metrics);
    
    // 执行瓶颈检测
    if (g_performance_analyzer.config.enable_bottleneck_detection) {
        performance_analyzer_detect_bottlenecks();
    }
    
    // 计算性能评分
    session->overall_performance_score = 
        performance_analyzer_calculate_performance_score(&session->current_metrics);
    session->cpu_efficiency = 
        performance_analyzer_calculate_cpu_efficiency(&session->current_metrics);
    session->memory_efficiency = 
        performance_analyzer_calculate_memory_efficiency(&session->current_metrics);
    session->cache_efficiency = 
        performance_analyzer_calculate_cache_efficiency(&session->current_metrics);
    
    // 生成分析摘要
    snprintf(session->analysis_summary, sizeof(session->analysis_summary),
             "Session Duration: %ld seconds, Performance Score: %.2f, "
             "CPU Efficiency: %.2f%%, Memory Efficiency: %.2f%%, Cache Efficiency: %.2f%%",
             session->end_time - session->start_time,
             session->overall_performance_score,
             session->cpu_efficiency * 100,
             session->memory_efficiency * 100,
             session->cache_efficiency * 100);
    
    printf("Performance Analysis Session Ended: %s\n", session->session_name);
    printf("  Duration: %ld seconds\n", session->end_time - session->start_time);
    printf("  Performance Score: %.2f\n", session->overall_performance_score);
    printf("  Bottlenecks Detected: %d\n", session->bottleneck_count);
    
    // 注意: 这里不释放session，因为可能需要后续查询
    // 在实际应用中，应该有会话管理机制

    return 0;
}

// 瓶颈检测
int performance_analyzer_detect_bottlenecks(void) {
    if (!g_performance_analyzer.is_initialized || !g_performance_analyzer.current_session) {
        return -1;
    }

    PerformanceAnalysisSession* session = g_performance_analyzer.current_session;
    PerformanceMetrics* metrics = &session->current_metrics;

    // 清理现有瓶颈
    PerformanceBottleneck* current = session->bottlenecks;
    while (current) {
        PerformanceBottleneck* next = current->next;
        free(current);
        current = next;
    }
    session->bottlenecks = NULL;
    session->bottleneck_count = 0;

    // CPU瓶颈检测
    if (metrics->cpu_utilization > g_performance_analyzer.config.cpu_threshold) {
        PerformanceBottleneck* bottleneck = malloc(sizeof(PerformanceBottleneck));
        if (bottleneck) {
            memset(bottleneck, 0, sizeof(PerformanceBottleneck));
            bottleneck->type = BOTTLENECK_CPU_BOUND;
            bottleneck->severity = metrics->cpu_utilization;
            bottleneck->impact_percentage = (metrics->cpu_utilization - g_performance_analyzer.config.cpu_threshold) * 100;

            snprintf(bottleneck->description, sizeof(bottleneck->description),
                    "High CPU utilization: %.1f%%", metrics->cpu_utilization * 100);
            snprintf(bottleneck->suggestion, sizeof(bottleneck->suggestion),
                    "Consider optimizing CPU-intensive operations, using parallel processing, or reducing computational complexity");

            bottleneck->related_metrics = *metrics;
            bottleneck->next = session->bottlenecks;
            session->bottlenecks = bottleneck;
            session->bottleneck_count++;
        }
    }

    // 内存瓶颈检测
    if (metrics->memory_usage_bytes > 0) {
        // 简化的内存阈值检测 (假设系统有8GB内存)
        size_t estimated_total_memory = 8ULL * 1024 * 1024 * 1024;
        double memory_ratio = (double)metrics->memory_usage_bytes / estimated_total_memory;

        if (memory_ratio > g_performance_analyzer.config.memory_threshold) {
            PerformanceBottleneck* bottleneck = malloc(sizeof(PerformanceBottleneck));
            if (bottleneck) {
                memset(bottleneck, 0, sizeof(PerformanceBottleneck));
                bottleneck->type = BOTTLENECK_MEMORY_BOUND;
                bottleneck->severity = memory_ratio;
                bottleneck->impact_percentage = (memory_ratio - g_performance_analyzer.config.memory_threshold) * 100;

                snprintf(bottleneck->description, sizeof(bottleneck->description),
                        "High memory usage: %.1f MB", metrics->memory_usage_bytes / (1024.0 * 1024.0));
                snprintf(bottleneck->suggestion, sizeof(bottleneck->suggestion),
                        "Consider optimizing memory usage, implementing memory pooling, or reducing memory allocations");

                bottleneck->related_metrics = *metrics;
                bottleneck->next = session->bottlenecks;
                session->bottlenecks = bottleneck;
                session->bottleneck_count++;
            }
        }
    }

    // 缓存未命中瓶颈检测
    if (metrics->cache_hit_ratio < (1.0 - g_performance_analyzer.config.cache_miss_threshold)) {
        PerformanceBottleneck* bottleneck = malloc(sizeof(PerformanceBottleneck));
        if (bottleneck) {
            memset(bottleneck, 0, sizeof(PerformanceBottleneck));
            bottleneck->type = BOTTLENECK_CACHE_MISS;
            bottleneck->severity = 1.0 - metrics->cache_hit_ratio;
            bottleneck->impact_percentage = (g_performance_analyzer.config.cache_miss_threshold - (1.0 - metrics->cache_hit_ratio)) * 100;

            snprintf(bottleneck->description, sizeof(bottleneck->description),
                    "Low cache hit ratio: %.1f%%", metrics->cache_hit_ratio * 100);
            snprintf(bottleneck->suggestion, sizeof(bottleneck->suggestion),
                    "Consider improving data locality, using cache-friendly algorithms, or optimizing memory access patterns");

            bottleneck->related_metrics = *metrics;
            bottleneck->next = session->bottlenecks;
            session->bottlenecks = bottleneck;
            session->bottleneck_count++;
        }
    }

    // 分支预测失败瓶颈检测
    if (metrics->branch_prediction_accuracy < (1.0 - g_performance_analyzer.config.branch_miss_threshold)) {
        PerformanceBottleneck* bottleneck = malloc(sizeof(PerformanceBottleneck));
        if (bottleneck) {
            memset(bottleneck, 0, sizeof(PerformanceBottleneck));
            bottleneck->type = BOTTLENECK_BRANCH_MISS;
            bottleneck->severity = 1.0 - metrics->branch_prediction_accuracy;
            bottleneck->impact_percentage = (g_performance_analyzer.config.branch_miss_threshold - (1.0 - metrics->branch_prediction_accuracy)) * 100;

            snprintf(bottleneck->description, sizeof(bottleneck->description),
                    "Low branch prediction accuracy: %.1f%%", metrics->branch_prediction_accuracy * 100);
            snprintf(bottleneck->suggestion, sizeof(bottleneck->suggestion),
                    "Consider reducing conditional branches, using branch-free algorithms, or improving branch predictability");

            bottleneck->related_metrics = *metrics;
            bottleneck->next = session->bottlenecks;
            session->bottlenecks = bottleneck;
            session->bottleneck_count++;
        }
    }

    g_performance_analyzer.total_bottlenecks_detected += session->bottleneck_count;

    // 调用回调函数
    if (g_performance_analyzer.bottleneck_callback) {
        PerformanceBottleneck* bottleneck = session->bottlenecks;
        while (bottleneck) {
            g_performance_analyzer.bottleneck_callback(bottleneck, g_performance_analyzer.callback_user_data);
            bottleneck = bottleneck->next;
        }
    }

    return session->bottleneck_count;
}

// 计算性能评分
double performance_analyzer_calculate_performance_score(const PerformanceMetrics* metrics) {
    if (!metrics) return 0.0;

    double cpu_score = 1.0 - metrics->cpu_utilization;
    double cache_score = metrics->cache_hit_ratio;
    double branch_score = metrics->branch_prediction_accuracy;
    double ipc_score = metrics->instructions_per_cycle / 4.0; // 假设理想IPC为4

    if (ipc_score > 1.0) ipc_score = 1.0;

    // 加权平均
    double overall_score = (cpu_score * 0.3 + cache_score * 0.3 + branch_score * 0.2 + ipc_score * 0.2);

    return overall_score;
}

// 计算CPU效率
double performance_analyzer_calculate_cpu_efficiency(const PerformanceMetrics* metrics) {
    if (!metrics || metrics->cycle_count == 0) return 0.0;

    return metrics->instructions_per_cycle / 4.0; // 假设理想IPC为4
}

// 计算内存效率
double performance_analyzer_calculate_memory_efficiency(const PerformanceMetrics* metrics) {
    if (!metrics) return 0.0;

    // 简化的内存效率计算 (基于缓存命中率)
    return metrics->cache_hit_ratio;
}

// 计算缓存效率
double performance_analyzer_calculate_cache_efficiency(const PerformanceMetrics* metrics) {
    if (!metrics) return 0.0;

    return metrics->cache_hit_ratio;
}

// 瓶颈类型转字符串
const char* performance_analyzer_bottleneck_type_to_string(BottleneckType type) {
    switch (type) {
        case BOTTLENECK_CPU_BOUND: return "CPU_BOUND";
        case BOTTLENECK_MEMORY_BOUND: return "MEMORY_BOUND";
        case BOTTLENECK_IO_BOUND: return "IO_BOUND";
        case BOTTLENECK_CACHE_MISS: return "CACHE_MISS";
        case BOTTLENECK_BRANCH_MISS: return "BRANCH_MISS";
        case BOTTLENECK_JIT_COMPILE: return "JIT_COMPILE";
        case BOTTLENECK_MODULE_LOAD: return "MODULE_LOAD";
        case BOTTLENECK_LOCK_CONTENTION: return "LOCK_CONTENTION";
        case BOTTLENECK_UNKNOWN: return "UNKNOWN";
        default: return "INVALID";
    }
}

// 获取瓶颈列表
PerformanceBottleneck* performance_analyzer_get_bottlenecks(void) {
    if (!g_performance_analyzer.current_session) {
        return NULL;
    }

    return g_performance_analyzer.current_session->bottlenecks;
}

// 获取瓶颈数量
int performance_analyzer_get_bottleneck_count(void) {
    if (!g_performance_analyzer.current_session) {
        return 0;
    }

    return g_performance_analyzer.current_session->bottleneck_count;
}

// 启动实时分析
int performance_analyzer_start_real_time_analysis(void) {
    if (!g_performance_analyzer.is_initialized) {
        return -1;
    }

    g_performance_analyzer.is_analyzing = true;
    printf("Real-time performance analysis started\n");
    return 0;
}

// 停止实时分析
int performance_analyzer_stop_real_time_analysis(void) {
    if (!g_performance_analyzer.is_initialized) {
        return -1;
    }

    g_performance_analyzer.is_analyzing = false;
    printf("Real-time performance analysis stopped\n");
    return 0;
}

// 检查实时分析是否活跃
bool performance_analyzer_is_real_time_active(void) {
    return g_performance_analyzer.is_analyzing;
}

// 设置瓶颈回调
void performance_analyzer_set_bottleneck_callback(void (*callback)(const PerformanceBottleneck*, void*), void* user_data) {
    if (!g_performance_analyzer.is_initialized) return;

    g_performance_analyzer.bottleneck_callback = callback;
    g_performance_analyzer.callback_user_data = user_data;
}

// 获取当前指标
int performance_analyzer_get_current_metrics(PerformanceMetrics* metrics) {
    if (!metrics) return -1;

    return performance_analyzer_collect_metrics(metrics);
}

// 获取会话
PerformanceAnalysisSession* performance_analyzer_get_session(uint32_t session_id) {
    if (g_performance_analyzer.current_session &&
        g_performance_analyzer.current_session->session_id == session_id) {
        return g_performance_analyzer.current_session;
    }

    return NULL;
}

// 获取配置
PerformanceAnalysisConfig performance_analyzer_get_config(void) {
    return g_performance_analyzer.config;
}

// 设置配置
int performance_analyzer_set_config(const PerformanceAnalysisConfig* config) {
    if (!config) return -1;

    g_performance_analyzer.config = *config;
    return 0;
}
