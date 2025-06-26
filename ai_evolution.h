/**
 * ai_evolution.h - AI驱动进化算法架构
 * 
 * 这个模块实现了AI驱动的代码进化算法，包括：
 * 1. 代码分析和性能评估
 * 2. 优化策略选择和应用
 * 3. 自动改进机制
 * 4. 进化历史跟踪
 * 
 * 版本：1.0.0
 * 日期：2025-06-27
 */

#ifndef AI_EVOLUTION_H
#define AI_EVOLUTION_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// ===============================================
// 核心数据结构
// ===============================================

// 代码性能指标
typedef struct {
    double execution_time;      // 执行时间（秒）
    size_t memory_usage;        // 内存使用量（字节）
    size_t code_size;          // 代码大小（字节）
    int error_count;           // 错误数量
    double cpu_utilization;    // CPU利用率
    double success_rate;       // 成功率
} PerformanceMetrics;

// 优化策略类型
typedef enum {
    OPT_PERFORMANCE,    // 性能优化
    OPT_MEMORY,         // 内存优化
    OPT_SIZE,           // 代码大小优化
    OPT_RELIABILITY,    // 可靠性优化
    OPT_MAINTAINABILITY // 可维护性优化
} OptimizationStrategy;

// 进化操作类型
typedef enum {
    EVO_MUTATION,       // 变异
    EVO_CROSSOVER,      // 交叉
    EVO_SELECTION,      // 选择
    EVO_ADAPTATION      // 适应
} EvolutionOperation;

// 代码变体
typedef struct CodeVariant {
    char* source_code;          // 源代码
    char* astc_code;           // ASTC代码
    PerformanceMetrics metrics; // 性能指标
    double fitness_score;       // 适应度分数
    int generation;            // 代数
    time_t created_time;       // 创建时间
    struct CodeVariant* parent; // 父代码
    struct CodeVariant* next;   // 链表下一个
} CodeVariant;

// AI进化引擎
typedef struct {
    CodeVariant* population;    // 代码种群
    size_t population_size;     // 种群大小
    int current_generation;     // 当前代数
    double mutation_rate;       // 变异率
    double crossover_rate;      // 交叉率
    OptimizationStrategy strategy; // 当前优化策略
    
    // 学习参数
    double learning_rate;       // 学习率
    double exploration_rate;    // 探索率
    
    // 历史记录
    PerformanceMetrics* history; // 性能历史
    size_t history_size;        // 历史大小
    size_t history_capacity;    // 历史容量
} AIEvolutionEngine;

// ===============================================
// 核心函数接口
// ===============================================

// 初始化AI进化引擎
bool ai_evolution_init(AIEvolutionEngine* engine, size_t population_size);

// 清理AI进化引擎
void ai_evolution_cleanup(AIEvolutionEngine* engine);

// 添加代码变体到种群
bool ai_evolution_add_variant(AIEvolutionEngine* engine, const char* source_code);

// 评估代码变体的性能
PerformanceMetrics ai_evolution_evaluate(AIEvolutionEngine* engine, CodeVariant* variant);

// 计算适应度分数
double ai_evolution_calculate_fitness(const PerformanceMetrics* metrics, OptimizationStrategy strategy);

// 执行一代进化
bool ai_evolution_evolve_generation(AIEvolutionEngine* engine);

// 选择最优代码变体
CodeVariant* ai_evolution_select_best(AIEvolutionEngine* engine);

// 应用变异操作
CodeVariant* ai_evolution_mutate(AIEvolutionEngine* engine, CodeVariant* parent);

// 应用交叉操作
CodeVariant* ai_evolution_crossover(AIEvolutionEngine* engine, CodeVariant* parent1, CodeVariant* parent2);

// 自适应调整参数
void ai_evolution_adapt_parameters(AIEvolutionEngine* engine);

// 分析进化趋势
bool ai_evolution_analyze_trends(AIEvolutionEngine* engine);

// 生成优化建议
char* ai_evolution_generate_suggestions(AIEvolutionEngine* engine, CodeVariant* variant);

// ===============================================
// 辅助函数
// ===============================================

// 创建代码变体
CodeVariant* ai_evolution_create_variant(const char* source_code);

// 释放代码变体
void ai_evolution_free_variant(CodeVariant* variant);

// 复制代码变体
CodeVariant* ai_evolution_clone_variant(const CodeVariant* original);

// 比较两个代码变体
int ai_evolution_compare_variants(const CodeVariant* a, const CodeVariant* b);

// 保存进化历史
bool ai_evolution_save_history(AIEvolutionEngine* engine, const char* filename);

// 加载进化历史
bool ai_evolution_load_history(AIEvolutionEngine* engine, const char* filename);

// 打印进化统计信息
void ai_evolution_print_stats(AIEvolutionEngine* engine);

// ===============================================
// 配置常量
// ===============================================

#define AI_EVO_DEFAULT_POPULATION_SIZE  10
#define AI_EVO_DEFAULT_MUTATION_RATE    0.1
#define AI_EVO_DEFAULT_CROSSOVER_RATE   0.7
#define AI_EVO_DEFAULT_LEARNING_RATE    0.01
#define AI_EVO_DEFAULT_EXPLORATION_RATE 0.2
#define AI_EVO_MAX_GENERATIONS          1000
#define AI_EVO_FITNESS_THRESHOLD        0.95

#endif // AI_EVOLUTION_H
