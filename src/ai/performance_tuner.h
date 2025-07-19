/*
 * Performance Tuner Header - Stage 2 AI性能调优系统
 * T2.4: 性能调优AI接口定义
 */

#ifndef PERFORMANCE_TUNER_H
#define PERFORMANCE_TUNER_H

#include <time.h>

// 性能调优统计结果
typedef struct PerformanceTuningMetrics {
    int total_tunings;
    int high_priority_tunings;
    double overall_performance_gain;   // 整体性能提升
    int cpu_optimizations;
    int memory_optimizations;
    int io_optimizations;
    int compiler_optimizations;
    int concurrency_optimizations;
    int system_optimizations;
    double estimated_speedup;          // 预期加速比
} PerformanceTuningMetrics;

// 主要接口函数
int performance_tuner_run(void);                           // 运行性能调优分析
int performance_tuner_export_json(const char* output_file); // 导出JSON报告

// 调优类别常量
#define TUNING_CPU_OPTIMIZATION         "CPU_OPTIMIZATION"
#define TUNING_MEMORY_OPTIMIZATION      "MEMORY_OPTIMIZATION"
#define TUNING_IO_OPTIMIZATION          "IO_OPTIMIZATION"
#define TUNING_COMPILER_OPTIMIZATION    "COMPILER_OPTIMIZATION"
#define TUNING_CONCURRENCY_OPTIMIZATION "CONCURRENCY_OPTIMIZATION"
#define TUNING_SYSTEM_OPTIMIZATION      "SYSTEM_OPTIMIZATION"

// 优先级常量
#define TUNING_PRIORITY_CRITICAL        10
#define TUNING_PRIORITY_HIGH            8
#define TUNING_PRIORITY_MEDIUM          6
#define TUNING_PRIORITY_LOW             4

#endif // PERFORMANCE_TUNER_H