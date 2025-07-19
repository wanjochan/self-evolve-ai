/*
 * AI Learning Framework Header - Stage 2 AI学习框架系统
 * T3.1: AI学习框架接口定义
 */

#ifndef AI_LEARNING_FRAMEWORK_H
#define AI_LEARNING_FRAMEWORK_H

#include <time.h>

// AI学习统计结果
typedef struct AILearningMetrics {
    int total_patterns_learned;
    int total_experiences_accumulated;
    int total_recommendations_generated;
    double overall_learning_score;
    double pattern_recognition_accuracy;
    double recommendation_success_rate;
    int knowledge_base_size;
} AILearningMetrics;

// 主要接口函数
int ai_learning_framework_run(void);                           // 运行AI学习框架
int ai_learning_framework_export_json(const char* output_file); // 导出JSON报告

// 学习模式类别常量
#define LEARNING_DESIGN_PATTERN      "DESIGN_PATTERN"
#define LEARNING_CODE_SMELL          "CODE_SMELL"
#define LEARNING_PERFORMANCE_PATTERN "PERFORMANCE_PATTERN"
#define LEARNING_ARCHITECTURE_PATTERN "ARCHITECTURE_PATTERN"
#define LEARNING_OPTIMIZATION_PATTERN "OPTIMIZATION_PATTERN"

// 推荐类型常量
#define RECOMMENDATION_ARCHITECTURE  "架构改进"
#define RECOMMENDATION_PERFORMANCE   "性能优化"
#define RECOMMENDATION_CODE_QUALITY  "代码质量"
#define RECOMMENDATION_REFACTORING   "重构建议"

#endif // AI_LEARNING_FRAMEWORK_H