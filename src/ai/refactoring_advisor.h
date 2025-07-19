/*
 * Refactoring Advisor Header - Stage 2 AI重构机会识别系统
 * T1.4: 重构机会识别器接口定义
 */

#ifndef REFACTORING_ADVISOR_H
#define REFACTORING_ADVISOR_H

#include <time.h>

// 代码质量统计结果
typedef struct CodeQualityMetrics {
    int total_smells;
    int high_severity_smells;
    int function_level_issues;
    int duplication_issues;
    int naming_issues;
    int comment_issues;
    int error_handling_issues;
    double overall_quality_score;
    double technical_debt_hours;
} CodeQualityMetrics;

// 主要接口函数
int refactoring_advisor_run(void);                           // 运行重构机会识别
int refactoring_advisor_export_json(const char* output_file); // 导出JSON报告

// 代码异味类别常量
#define SMELL_FUNCTION_LEVEL     "FUNCTION_LEVEL"
#define SMELL_CLASS_LEVEL        "CLASS_LEVEL"
#define SMELL_DUPLICATION        "DUPLICATION"
#define SMELL_NAMING             "NAMING"
#define SMELL_COMMENTS           "COMMENTS"
#define SMELL_ERROR_HANDLING     "ERROR_HANDLING"
#define SMELL_COMPILER_SPECIFIC  "COMPILER_SPECIFIC"

// 严重程度常量
#define SMELL_SEVERITY_CRITICAL  9
#define SMELL_SEVERITY_HIGH      7
#define SMELL_SEVERITY_MEDIUM    5
#define SMELL_SEVERITY_LOW       3

#endif // REFACTORING_ADVISOR_H