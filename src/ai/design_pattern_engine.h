/*
 * Design Pattern Engine Header - Stage 2 AI设计模式识别系统
 * T1.2: 设计模式识别引擎接口定义
 */

#ifndef DESIGN_PATTERN_ENGINE_H
#define DESIGN_PATTERN_ENGINE_H

#include <time.h>

// 架构质量分析结果
typedef struct ArchitectureQuality {
    int total_patterns;
    int design_quality_score;    // 设计质量评分 1-100
    int maintainability_score;   // 可维护性评分 1-100
    int extensibility_score;     // 可扩展性评分 1-100
} ArchitectureQuality;

// 主要接口函数
int design_pattern_engine_run(void);                           // 运行设计模式分析
int design_pattern_engine_export_json(const char* output_file); // 导出JSON报告

// 模式类型常量
#define PATTERN_TYPE_CREATIONAL   "creational"
#define PATTERN_TYPE_STRUCTURAL   "structural"
#define PATTERN_TYPE_BEHAVIORAL   "behavioral"
#define PATTERN_TYPE_STAGE1       "stage1_specific"

// 置信度阈值
#define CONFIDENCE_HIGH     85
#define CONFIDENCE_MEDIUM   70
#define CONFIDENCE_LOW      50

#endif // DESIGN_PATTERN_ENGINE_H