/**
 * ai_optimizer.h - AI代码优化算法
 * 
 * 这个模块实现了AI驱动的自动代码优化算法，包括：
 * 1. 性能优化（循环优化、算法优化）
 * 2. 内存优化（内存分配优化、缓存优化）
 * 3. 架构改进（代码结构优化、模块化改进）
 * 4. 智能重构（代码简化、可读性提升）
 * 
 * 版本：1.0.0
 * 日期：2025-06-27
 */

#ifndef AI_OPTIMIZER_H
#define AI_OPTIMIZER_H

#include "ai_evolution.h"
#include "ai_learning.h"
#include <stdint.h>
#include <stdbool.h>

// ===============================================
// 优化类型和策略
// ===============================================

// 优化类型
typedef enum {
    OPT_TYPE_PERFORMANCE,    // 性能优化
    OPT_TYPE_MEMORY,         // 内存优化
    OPT_TYPE_SIZE,           // 代码大小优化
    OPT_TYPE_READABILITY,    // 可读性优化
    OPT_TYPE_MAINTAINABILITY // 可维护性优化
} OptimizationType;

// 优化技术
typedef enum {
    OPT_TECH_LOOP_UNROLL,    // 循环展开
    OPT_TECH_LOOP_FUSION,    // 循环融合
    OPT_TECH_CONST_FOLD,     // 常量折叠
    OPT_TECH_DEAD_CODE,      // 死代码消除
    OPT_TECH_INLINE,         // 函数内联
    OPT_TECH_VECTORIZE,      // 向量化
    OPT_TECH_CACHE_OPT,      // 缓存优化
    OPT_TECH_MEM_POOL,       // 内存池
    OPT_TECH_ALGORITHM,      // 算法替换
    OPT_TECH_DATA_STRUCT     // 数据结构优化
} OptimizationTechnique;

// 优化规则
typedef struct OptimizationRule {
    char* name;                    // 规则名称
    char* description;             // 规则描述
    char* pattern;                 // 匹配模式
    char* replacement;             // 替换模式
    OptimizationType type;         // 优化类型
    OptimizationTechnique technique; // 优化技术
    double expected_improvement;   // 预期改进
    double confidence;             // 置信度
    int usage_count;              // 使用次数
    struct OptimizationRule* next; // 链表下一个
} OptimizationRule;

// 优化建议
typedef struct {
    char* suggestion;              // 建议内容
    char* optimized_code;          // 优化后代码
    OptimizationType type;         // 优化类型
    double improvement_estimate;   // 改进估计
    double confidence;             // 置信度
    char* explanation;             // 解释说明
} OptimizationSuggestion;

// AI优化引擎
typedef struct {
    OptimizationRule* rules;       // 优化规则库
    AILearningEngine* learning;    // 学习引擎
    
    // 优化参数
    double min_improvement_threshold; // 最小改进阈值
    double confidence_threshold;      // 置信度阈值
    bool aggressive_optimization;     // 激进优化模式
    
    // 统计信息
    int total_optimizations;       // 总优化次数
    int successful_optimizations;  // 成功优化次数
    double total_improvement;      // 总改进量
} AIOptimizerEngine;

// ===============================================
// 核心函数接口
// ===============================================

// 初始化AI优化引擎
bool ai_optimizer_init(AIOptimizerEngine* engine, AILearningEngine* learning);

// 清理AI优化引擎
void ai_optimizer_cleanup(AIOptimizerEngine* engine);

// 分析代码并生成优化建议
OptimizationSuggestion* ai_optimizer_analyze_code(AIOptimizerEngine* engine, 
                                                  const char* code);

// 应用优化建议
char* ai_optimizer_apply_optimization(AIOptimizerEngine* engine, 
                                     const char* code, 
                                     const OptimizationSuggestion* suggestion);

// 自动优化代码
char* ai_optimizer_auto_optimize(AIOptimizerEngine* engine, 
                                const char* code, 
                                OptimizationType type);

// 评估优化效果
double ai_optimizer_evaluate_optimization(AIOptimizerEngine* engine,
                                         const char* original_code,
                                         const char* optimized_code);

// ===============================================
// 具体优化算法
// ===============================================

// 性能优化
char* ai_optimizer_optimize_performance(AIOptimizerEngine* engine, const char* code);

// 内存优化
char* ai_optimizer_optimize_memory(AIOptimizerEngine* engine, const char* code);

// 代码大小优化
char* ai_optimizer_optimize_size(AIOptimizerEngine* engine, const char* code);

// 可读性优化
char* ai_optimizer_optimize_readability(AIOptimizerEngine* engine, const char* code);

// 循环优化
char* ai_optimizer_optimize_loops(const char* code);

// 算法优化
char* ai_optimizer_optimize_algorithms(const char* code);

// 内存分配优化
char* ai_optimizer_optimize_memory_allocation(const char* code);

// 函数优化
char* ai_optimizer_optimize_functions(const char* code);

// ===============================================
// 模式识别和匹配
// ===============================================

// 识别优化机会
bool ai_optimizer_identify_opportunities(AIOptimizerEngine* engine, const char* code);

// 匹配优化规则
OptimizationRule* ai_optimizer_match_rules(AIOptimizerEngine* engine, const char* code);

// 分析代码复杂度
int ai_optimizer_analyze_complexity(const char* code);

// 检测性能瓶颈
char* ai_optimizer_detect_bottlenecks(const char* code);

// ===============================================
// 规则管理
// ===============================================

// 添加优化规则
bool ai_optimizer_add_rule(AIOptimizerEngine* engine, const OptimizationRule* rule);

// 移除优化规则
bool ai_optimizer_remove_rule(AIOptimizerEngine* engine, const char* rule_name);

// 更新规则统计
void ai_optimizer_update_rule_stats(OptimizationRule* rule, double improvement);

// 学习新的优化模式
bool ai_optimizer_learn_pattern(AIOptimizerEngine* engine, 
                               const char* original, 
                               const char* optimized, 
                               double improvement);

// ===============================================
// 辅助函数
// ===============================================

// 创建优化规则
OptimizationRule* ai_optimizer_create_rule(const char* name, 
                                          const char* pattern, 
                                          const char* replacement,
                                          OptimizationType type,
                                          OptimizationTechnique technique);

// 释放优化规则
void ai_optimizer_free_rule(OptimizationRule* rule);

// 创建优化建议
OptimizationSuggestion* ai_optimizer_create_suggestion(const char* suggestion,
                                                      const char* optimized_code,
                                                      OptimizationType type,
                                                      double improvement,
                                                      const char* explanation);

// 释放优化建议
void ai_optimizer_free_suggestion(OptimizationSuggestion* suggestion);

// 打印优化统计
void ai_optimizer_print_stats(AIOptimizerEngine* engine);

// ===============================================
// 配置常量
// ===============================================

#define AI_OPT_MIN_IMPROVEMENT_THRESHOLD  0.05
#define AI_OPT_CONFIDENCE_THRESHOLD       0.6
#define AI_OPT_MAX_RULES                  1000
#define AI_OPT_MAX_SUGGESTIONS            10

#endif // AI_OPTIMIZER_H
