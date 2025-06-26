/**
 * ai_learning.h - AI自我学习机制
 *
 * 这个模块实现了AI系统的自我学习能力，包括：
 * 1. 性能监控和数据收集
 * 2. 错误分析和模式识别
 * 3. 改进建议生成
 * 4. 知识库管理和更新
 *
 * 版本：1.0.0
 * 日期：2025-06-27
 */

#ifndef AI_LEARNING_H
#define AI_LEARNING_H

#include "ai_evolution.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// ===============================================
// 学习数据结构
// ===============================================

// 执行记录
typedef struct {
    char* code_snippet;        // 代码片段
    PerformanceMetrics metrics; // 性能指标
    int error_code;            // 错误代码
    char* error_message;       // 错误信息
    time_t timestamp;          // 时间戳
    double improvement_score;   // 改进分数
} ExecutionRecord;

// 错误模式
typedef struct ErrorPattern {
    char* pattern_name;        // 模式名称
    char* description;         // 描述
    char* code_pattern;        // 代码模式
    char* solution;            // 解决方案
    int occurrence_count;      // 出现次数
    double confidence;         // 置信度
    struct ErrorPattern* next; // 链表下一个
} ErrorPattern;

// 性能模式
typedef struct PerformancePattern {
    char* pattern_name;        // 模式名称
    char* code_pattern;        // 代码模式
    double avg_improvement;    // 平均改进
    int usage_count;           // 使用次数
    double success_rate;       // 成功率
    struct PerformancePattern* next;
} PerformancePattern;

// 知识库
typedef struct {
    ErrorPattern* error_patterns;       // 错误模式
    PerformancePattern* perf_patterns;  // 性能模式
    ExecutionRecord* records;           // 执行记录
    size_t record_count;               // 记录数量
    size_t record_capacity;            // 记录容量
    
    // 学习统计
    int total_executions;              // 总执行次数
    int successful_optimizations;     // 成功优化次数
    double overall_improvement;        // 总体改进
    time_t last_update;               // 最后更新时间
} KnowledgeBase;

// AI学习引擎
typedef struct {
    KnowledgeBase knowledge;           // 知识库
    double learning_threshold;         // 学习阈值
    double confidence_threshold;       // 置信度阈值
    int min_pattern_occurrences;      // 最小模式出现次数
    
    // 学习参数
    double pattern_decay_rate;         // 模式衰减率
    double adaptation_rate;            // 适应率
    bool auto_update_enabled;          // 自动更新启用
} AILearningEngine;

// ===============================================
// 核心函数接口
// ===============================================

// 初始化AI学习引擎
bool ai_learning_init(AILearningEngine* engine);

// 清理AI学习引擎
void ai_learning_cleanup(AILearningEngine* engine);

// 记录执行结果
bool ai_learning_record_execution(AILearningEngine* engine, const char* code, 
                                  const PerformanceMetrics* metrics, 
                                  int error_code, const char* error_msg);

// 分析错误模式
bool ai_learning_analyze_errors(AILearningEngine* engine);

// 分析性能模式
bool ai_learning_analyze_performance(AILearningEngine* engine);

// 生成改进建议
char* ai_learning_generate_suggestions(AILearningEngine* engine, const char* code);

// 学习新模式
bool ai_learning_learn_pattern(AILearningEngine* engine, const char* pattern_type, 
                              const char* pattern, const char* solution);

// 更新知识库
bool ai_learning_update_knowledge(AILearningEngine* engine);

// 预测性能改进
double ai_learning_predict_improvement(AILearningEngine* engine, const char* code, 
                                      const char* optimization);

// 评估优化效果
double ai_learning_evaluate_optimization(AILearningEngine* engine, 
                                        const char* original_code, 
                                        const char* optimized_code);

// ===============================================
// 模式识别函数
// ===============================================

// 识别代码模式
char* ai_learning_identify_code_pattern(const char* code);

// 匹配错误模式
ErrorPattern* ai_learning_match_error_pattern(AILearningEngine* engine, 
                                             const char* error_msg);

// 匹配性能模式
PerformancePattern* ai_learning_match_performance_pattern(AILearningEngine* engine, 
                                                         const char* code);

// 计算模式相似度
double ai_learning_calculate_pattern_similarity(const char* pattern1, const char* pattern2);

// ===============================================
// 知识库管理
// ===============================================

// 保存知识库
bool ai_learning_save_knowledge(AILearningEngine* engine, const char* filename);

// 加载知识库
bool ai_learning_load_knowledge(AILearningEngine* engine, const char* filename);

// 合并知识库
bool ai_learning_merge_knowledge(AILearningEngine* engine, const KnowledgeBase* other);

// 清理过期模式
void ai_learning_cleanup_patterns(AILearningEngine* engine);

// 打印学习统计
void ai_learning_print_stats(AILearningEngine* engine);

// ===============================================
// 辅助函数
// ===============================================

// 创建执行记录
ExecutionRecord* ai_learning_create_record(const char* code, 
                                          const PerformanceMetrics* metrics,
                                          int error_code, const char* error_msg);

// 释放执行记录
void ai_learning_free_record(ExecutionRecord* record);

// 创建错误模式
ErrorPattern* ai_learning_create_error_pattern(const char* name, const char* pattern, 
                                              const char* solution);

// 释放错误模式
void ai_learning_free_error_pattern(ErrorPattern* pattern);

// 创建性能模式
PerformancePattern* ai_learning_create_perf_pattern(const char* name, const char* pattern);

// 释放性能模式
void ai_learning_free_perf_pattern(PerformancePattern* pattern);

// ===============================================
// 配置常量
// ===============================================

#define AI_LEARNING_DEFAULT_THRESHOLD       0.1
#define AI_LEARNING_DEFAULT_CONFIDENCE      0.7
#define AI_LEARNING_MIN_PATTERN_COUNT       3
#define AI_LEARNING_PATTERN_DECAY_RATE      0.95
#define AI_LEARNING_ADAPTATION_RATE         0.05
#define AI_LEARNING_MAX_RECORDS             10000
#define AI_LEARNING_MAX_PATTERNS            1000

#endif // AI_LEARNING_H
