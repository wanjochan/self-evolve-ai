/*
 * Code Quality Assessor Header - Stage 2 代码质量评估系统
 * T3.2: 代码质量评估系统接口定义
 */

#ifndef CODE_QUALITY_ASSESSOR_H
#define CODE_QUALITY_ASSESSOR_H

#include <time.h>

// 代码质量评估结果
typedef struct CodeQualityAssessmentMetrics {
    double overall_quality_score;      // 总体质量评分
    double maintainability_score;      // 可维护性评分
    double performance_score;          // 性能评分
    double security_score;             // 安全性评分
    double reliability_score;          // 可靠性评分
    double modularity_score;           // 模块化评分
    double code_clarity_score;         // 代码清晰度评分
    double architecture_score;         // 架构质量评分
} CodeQualityAssessmentMetrics;

// 主要接口函数
int code_quality_assessor_run(void);                           // 运行代码质量评估
int code_quality_assessor_export_json(const char* output_file); // 导出JSON报告

// 质量维度常量
#define QUALITY_MAINTAINABILITY     "可维护性"
#define QUALITY_PERFORMANCE         "性能"
#define QUALITY_SECURITY            "安全性"
#define QUALITY_RELIABILITY         "可靠性"
#define QUALITY_MODULARITY          "模块化"
#define QUALITY_CODE_CLARITY        "代码清晰度"
#define QUALITY_ARCHITECTURE        "架构质量"

// 质量等级常量
#define QUALITY_GRADE_EXCELLENT     "优秀 (A)"
#define QUALITY_GRADE_GOOD          "良好 (B)"
#define QUALITY_GRADE_AVERAGE       "中等 (C)"
#define QUALITY_GRADE_PASS          "及格 (D)"
#define QUALITY_GRADE_FAIL          "不及格 (F)"

#endif // CODE_QUALITY_ASSESSOR_H