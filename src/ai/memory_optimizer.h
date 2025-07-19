/*
 * Memory Optimizer Header - Stage 2 AI内存管理优化系统
 * T2.2: 内存管理优化AI接口定义
 */

#ifndef MEMORY_OPTIMIZER_H
#define MEMORY_OPTIMIZER_H

#include <time.h>

// 内存优化统计结果
typedef struct MemoryOptimizationStats {
    int total_optimizations;
    int high_priority_optimizations;
    double total_memory_savings;
    int leak_fixes;
    int security_fixes;
    int pool_optimizations;
    int smart_memory_improvements;
} MemoryOptimizationStats;

// 主要接口函数
int memory_optimizer_run(void);                           // 运行内存优化分析
int memory_optimizer_export_json(const char* output_file); // 导出JSON报告

// 内存优化类型常量
#define MEMORY_LEAK_PREVENTION      "LEAK_PREVENTION"
#define MEMORY_SECURITY_FIX         "SECURITY_FIX"
#define MEMORY_POOL                 "MEMORY_POOL"
#define MEMORY_FRAGMENTATION        "FRAGMENTATION_REDUCTION"
#define MEMORY_CACHE_OPTIMIZATION   "CACHE_OPTIMIZATION"
#define MEMORY_SMART_MANAGEMENT     "SMART_MEMORY"
#define MEMORY_COPY_OPTIMIZATION    "COPY_OPTIMIZATION"
#define MEMORY_COMPILER_SPECIFIC    "COMPILER_SPECIFIC"
#define MEMORY_GARBAGE_COLLECTION   "GARBAGE_COLLECTION"

// 优先级常量
#define MEMORY_PRIORITY_CRITICAL    9
#define MEMORY_PRIORITY_HIGH        7
#define MEMORY_PRIORITY_MEDIUM      5
#define MEMORY_PRIORITY_LOW         3

#endif // MEMORY_OPTIMIZER_H