/*
 * Performance Detector Header - Stage 2 AI性能瓶颈检测系统
 * T1.3: 性能瓶颈检测器接口定义
 */

#ifndef PERFORMANCE_DETECTOR_H
#define PERFORMANCE_DETECTOR_H

#include <time.h>

// 性能热点统计结果
typedef struct PerformanceHotspots {
    int total_bottlenecks;
    int critical_bottlenecks;     // 严重程度 >= 8
    int high_bottlenecks;         // 严重程度 >= 6
    double total_impact;          // 总性能影响
    char* worst_file;             // 最差文件
    int worst_file_issues;        // 最差文件问题数
} PerformanceHotspots;

// 主要接口函数
int performance_detector_run(void);                           // 运行性能瓶颈检测
int performance_detector_export_json(const char* output_file); // 导出JSON报告

// 性能瓶颈类别常量
#define BOTTLENECK_ALGORITHM       "ALGORITHM_COMPLEXITY"
#define BOTTLENECK_MEMORY         "MEMORY_MANAGEMENT"
#define BOTTLENECK_IO             "IO_OPERATIONS"
#define BOTTLENECK_CACHE          "CACHE_PERFORMANCE"
#define BOTTLENECK_CONCURRENCY    "CONCURRENCY"
#define BOTTLENECK_COMPILER       "COMPILER_SPECIFIC"
#define BOTTLENECK_STRING         "STRING_PROCESSING"

// 严重程度常量
#define SEVERITY_CRITICAL    9
#define SEVERITY_HIGH       7
#define SEVERITY_MEDIUM     5
#define SEVERITY_LOW        3

#endif // PERFORMANCE_DETECTOR_H