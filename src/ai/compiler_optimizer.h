/*
 * Compiler Optimizer Header - Stage 2 AI编译器优化系统
 * T2.1: 编译器优化AI接口定义
 */

#ifndef COMPILER_OPTIMIZER_H
#define COMPILER_OPTIMIZER_H

#include <time.h>

// 性能分析结果
typedef struct PerformanceMetrics {
    int bottlenecks_found;           // 发现的性能瓶颈数量
    int optimizations_recommended;   // 推荐的优化方案数量
    double total_potential_improvement; // 总体潜在性能提升百分比
    int high_priority_optimizations;    // 高优先级优化项目数量
} PerformanceMetrics;

// 主要接口函数
int compiler_optimizer_run(void);                           // 运行编译器优化分析
int compiler_optimizer_export_json(const char* output_file); // 导出JSON报告

// 优化类别常量
#define OPTIMIZATION_COMPILATION   "compilation"
#define OPTIMIZATION_CODEGEN      "codegen"
#define OPTIMIZATION_CACHE        "cache"
#define OPTIMIZATION_STAGE1       "stage1_specific"

// 优先级常量
#define PRIORITY_CRITICAL    9
#define PRIORITY_HIGH       7
#define PRIORITY_MEDIUM     5
#define PRIORITY_LOW        3

// ROI阈值
#define ROI_EXCELLENT    5.0
#define ROI_GOOD        2.0
#define ROI_ACCEPTABLE  1.0

#endif // COMPILER_OPTIMIZER_H