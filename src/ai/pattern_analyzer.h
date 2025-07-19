/*
 * Pattern Analyzer Header - Stage 2 AI模式识别系统
 * T1.1: 代码模式分析器接口定义
 */

#ifndef PATTERN_ANALYZER_H
#define PATTERN_ANALYZER_H

#include <time.h>

// 分析统计结构
typedef struct AnalysisStats {
    int total_files;           // 分析的文件总数
    int total_lines;           // 代码总行数
    int total_patterns;        // 发现的模式总数
    int high_priority_issues;  // 高优先级问题数
    int medium_priority_issues;// 中优先级问题数
    int low_priority_issues;   // 低优先级问题数
} AnalysisStats;

// 主要接口函数
int pattern_analyzer_run(void);                          // 运行模式分析
AnalysisStats* pattern_analyzer_get_stats(void);         // 获取分析统计
int pattern_analyzer_export_json(const char* output_file); // 导出JSON报告

// 模式类别常量
#define PATTERN_CATEGORY_PERFORMANCE   "performance"
#define PATTERN_CATEGORY_DESIGN        "design_pattern"
#define PATTERN_CATEGORY_QUALITY       "quality"
#define PATTERN_CATEGORY_SECURITY      "security"

// 优先级常量
#define PATTERN_PRIORITY_HIGH    1
#define PATTERN_PRIORITY_MEDIUM  2
#define PATTERN_PRIORITY_LOW     3

#endif // PATTERN_ANALYZER_H