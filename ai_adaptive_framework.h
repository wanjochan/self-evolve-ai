/**
 * ai_adaptive_framework.h - AI适应性进化框架
 * 
 * 这个模块实现了完整的AI适应性进化框架，整合了：
 * 1. AI进化算法 (ai_evolution.h)
 * 2. AI学习机制 (ai_learning.h)  
 * 3. AI优化算法 (ai_optimizer.h)
 * 4. 环境感知和自适应调整
 * 5. 多目标优化和决策
 * 
 * 版本：1.0.0
 * 日期：2025-06-27
 */

#ifndef AI_ADAPTIVE_FRAMEWORK_H
#define AI_ADAPTIVE_FRAMEWORK_H

#include "ai_evolution.h"
#include "ai_learning.h"
#include "ai_optimizer.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// ===============================================
// 环境和上下文
// ===============================================

// 环境类型
typedef enum {
    ENV_DEVELOPMENT,     // 开发环境
    ENV_TESTING,         // 测试环境
    ENV_PRODUCTION,      // 生产环境
    ENV_RESEARCH,        // 研究环境
    ENV_EMBEDDED         // 嵌入式环境
} EnvironmentType;

// 资源约束
typedef struct {
    size_t max_memory;      // 最大内存限制
    double max_cpu_time;    // 最大CPU时间
    size_t max_code_size;   // 最大代码大小
    int max_complexity;     // 最大复杂度
    bool real_time_required; // 是否需要实时性
} ResourceConstraints;

// 环境上下文
typedef struct {
    EnvironmentType type;           // 环境类型
    ResourceConstraints constraints; // 资源约束
    double performance_weight;      // 性能权重
    double memory_weight;          // 内存权重
    double reliability_weight;     // 可靠性权重
    double maintainability_weight; // 可维护性权重
    time_t last_update;           // 最后更新时间
} EnvironmentContext;

// 适应性策略
typedef enum {
    ADAPT_CONSERVATIVE,  // 保守策略
    ADAPT_BALANCED,      // 平衡策略
    ADAPT_AGGRESSIVE,    // 激进策略
    ADAPT_CUSTOM         // 自定义策略
} AdaptationStrategy;

// 进化目标
typedef struct {
    double target_performance;     // 目标性能
    double target_memory_usage;    // 目标内存使用
    double target_reliability;    // 目标可靠性
    double target_maintainability; // 目标可维护性
    double tolerance;              // 容忍度
} EvolutionGoals;

// ===============================================
// 适应性进化框架
// ===============================================

// AI适应性进化框架
typedef struct {
    // 核心AI组件
    AIEvolutionEngine* evolution;   // 进化引擎
    AILearningEngine* learning;     // 学习引擎
    AIOptimizerEngine* optimizer;   // 优化引擎
    
    // 环境和上下文
    EnvironmentContext environment; // 环境上下文
    AdaptationStrategy strategy;    // 适应策略
    EvolutionGoals goals;          // 进化目标
    
    // 适应性参数
    double adaptation_rate;         // 适应率
    double exploration_factor;      // 探索因子
    double exploitation_factor;     // 利用因子
    int adaptation_interval;        // 适应间隔（秒）
    
    // 状态和统计
    int total_adaptations;         // 总适应次数
    int successful_adaptations;    // 成功适应次数
    double overall_improvement;    // 总体改进
    time_t last_adaptation;       // 最后适应时间
    
    // 历史记录
    PerformanceMetrics* adaptation_history; // 适应历史
    size_t history_size;                   // 历史大小
    size_t history_capacity;               // 历史容量
} AIAdaptiveFramework;

// ===============================================
// 核心函数接口
// ===============================================

// 初始化适应性框架
bool ai_adaptive_init(AIAdaptiveFramework* framework);

// 清理适应性框架
void ai_adaptive_cleanup(AIAdaptiveFramework* framework);

// 设置环境上下文
bool ai_adaptive_set_environment(AIAdaptiveFramework* framework, 
                                 const EnvironmentContext* context);

// 设置进化目标
bool ai_adaptive_set_goals(AIAdaptiveFramework* framework, 
                          const EvolutionGoals* goals);

// 执行适应性进化
bool ai_adaptive_evolve(AIAdaptiveFramework* framework, const char* code);

// 自动适应环境变化
bool ai_adaptive_auto_adapt(AIAdaptiveFramework* framework);

// 评估当前状态
double ai_adaptive_evaluate_state(AIAdaptiveFramework* framework);

// 生成适应性建议
char* ai_adaptive_generate_recommendations(AIAdaptiveFramework* framework, 
                                          const char* code);

// ===============================================
// 决策和策略
// ===============================================

// 多目标决策
OptimizationType ai_adaptive_decide_optimization_type(AIAdaptiveFramework* framework,
                                                     const PerformanceMetrics* current);

// 调整适应策略
void ai_adaptive_adjust_strategy(AIAdaptiveFramework* framework, 
                                const PerformanceMetrics* feedback);

// 平衡多个目标
double ai_adaptive_balance_objectives(AIAdaptiveFramework* framework,
                                     const PerformanceMetrics* metrics);

// 预测适应效果
double ai_adaptive_predict_adaptation_effect(AIAdaptiveFramework* framework,
                                            const char* code,
                                            OptimizationType type);

// ===============================================
// 环境感知
// ===============================================

// 检测环境变化
bool ai_adaptive_detect_environment_change(AIAdaptiveFramework* framework);

// 分析资源使用情况
ResourceConstraints ai_adaptive_analyze_resource_usage(AIAdaptiveFramework* framework);

// 监控性能指标
PerformanceMetrics ai_adaptive_monitor_performance(AIAdaptiveFramework* framework,
                                                  const char* code);

// 评估环境适应性
double ai_adaptive_evaluate_environment_fitness(AIAdaptiveFramework* framework);

// ===============================================
// 学习和记忆
// ===============================================

// 记录适应历史
bool ai_adaptive_record_adaptation(AIAdaptiveFramework* framework,
                                  const PerformanceMetrics* before,
                                  const PerformanceMetrics* after,
                                  OptimizationType type);

// 从历史中学习
bool ai_adaptive_learn_from_history(AIAdaptiveFramework* framework);

// 更新适应模型
bool ai_adaptive_update_model(AIAdaptiveFramework* framework,
                             const PerformanceMetrics* feedback);

// 预测未来趋势
PerformanceMetrics ai_adaptive_predict_future_trends(AIAdaptiveFramework* framework);

// ===============================================
// 辅助函数
// ===============================================

// 创建环境上下文
EnvironmentContext ai_adaptive_create_environment(EnvironmentType type,
                                                 const ResourceConstraints* constraints);

// 创建进化目标
EvolutionGoals ai_adaptive_create_goals(double performance, double memory,
                                       double reliability, double maintainability);

// 计算适应性分数
double ai_adaptive_calculate_fitness_score(const PerformanceMetrics* metrics,
                                          const EvolutionGoals* goals);

// 打印适应性统计
void ai_adaptive_print_stats(AIAdaptiveFramework* framework);

// 保存适应性状态
bool ai_adaptive_save_state(AIAdaptiveFramework* framework, const char* filename);

// 加载适应性状态
bool ai_adaptive_load_state(AIAdaptiveFramework* framework, const char* filename);

// ===============================================
// 配置常量
// ===============================================

#define AI_ADAPTIVE_DEFAULT_ADAPTATION_RATE    0.1
#define AI_ADAPTIVE_DEFAULT_EXPLORATION_FACTOR 0.3
#define AI_ADAPTIVE_DEFAULT_EXPLOITATION_FACTOR 0.7
#define AI_ADAPTIVE_DEFAULT_INTERVAL           60  // 60秒
#define AI_ADAPTIVE_MAX_HISTORY                1000
#define AI_ADAPTIVE_MIN_IMPROVEMENT_THRESHOLD  0.05

#endif // AI_ADAPTIVE_FRAMEWORK_H
