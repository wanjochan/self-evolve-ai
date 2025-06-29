#ifndef CODE_ANALYZER_H
#define CODE_ANALYZER_H

/**
 * code_analyzer.h - AI驱动的代码分析器
 * 
 * 这个模块实现AI自主分析代码的能力，识别改进机会
 * 为代码进化提供智能决策支持
 */

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// 代码分析结果结构
// ===============================================

typedef enum {
    IMPROVEMENT_PERFORMANCE,    // 性能优化
    IMPROVEMENT_MEMORY,         // 内存优化
    IMPROVEMENT_READABILITY,    // 可读性改进
    IMPROVEMENT_MAINTAINABILITY,// 可维护性改进
    IMPROVEMENT_SECURITY,       // 安全性改进
    IMPROVEMENT_FUNCTIONALITY   // 功能增强
} ImprovementType;

typedef struct {
    ImprovementType type;
    char* description;
    char* file_path;
    int line_number;
    int confidence_score;       // 0-100的置信度
    char* suggested_fix;
} CodeImprovement;

typedef struct {
    char* file_path;
    size_t file_size;
    int complexity_score;       // 代码复杂度评分
    int quality_score;          // 代码质量评分
    int performance_score;      // 性能评分
    CodeImprovement* improvements;
    int improvement_count;
} CodeAnalysisResult;

// ===============================================
// AI代码分析接口
// ===============================================

/**
 * 初始化AI代码分析器
 */
int ai_analyzer_init(void);

/**
 * 分析单个源文件
 */
CodeAnalysisResult* ai_analyze_file(const char* file_path);

/**
 * 分析整个项目
 */
CodeAnalysisResult* ai_analyze_project(const char* project_path);

/**
 * 生成改进建议
 */
CodeImprovement* ai_generate_improvements(const char* source_code, int* count);

/**
 * 评估代码质量
 */
int ai_evaluate_code_quality(const char* source_code);

/**
 * 计算代码复杂度
 */
int ai_calculate_complexity(const char* source_code);

/**
 * 评估代码性能
 */
int ai_evaluate_performance(const char* source_code);

/**
 * 检测性能瓶颈
 */
CodeImprovement* ai_detect_performance_issues(const char* source_code, int* count);

/**
 * 检测内存泄漏风险
 */
CodeImprovement* ai_detect_memory_issues(const char* source_code, int* count);

/**
 * 检测安全漏洞
 */
CodeImprovement* ai_detect_security_issues(const char* source_code, int* count);

// ===============================================
// 代码模式识别
// ===============================================

typedef enum {
    PATTERN_LOOP_OPTIMIZATION,      // 循环优化机会
    PATTERN_FUNCTION_INLINING,      // 函数内联机会
    PATTERN_MEMORY_POOLING,         // 内存池化机会
    PATTERN_CACHE_OPTIMIZATION,     // 缓存优化机会
    PATTERN_ALGORITHM_IMPROVEMENT,  // 算法改进机会
    PATTERN_DATA_STRUCTURE_OPT      // 数据结构优化机会
} CodePattern;

typedef struct {
    CodePattern pattern;
    char* location;
    char* description;
    int impact_score;           // 影响评分
} PatternMatch;

/**
 * 识别代码模式
 */
PatternMatch* ai_identify_patterns(const char* source_code, int* count);

/**
 * 建议重构方案
 */
char* ai_suggest_refactoring(const char* source_code, CodePattern pattern);

// ===============================================
// 自动代码生成
// ===============================================

/**
 * 生成优化版本的代码
 */
char* ai_generate_optimized_code(const char* original_code, CodeImprovement* improvements, int count);

/**
 * 生成测试用例
 */
char* ai_generate_test_cases(const char* function_code);

/**
 * 生成文档
 */
char* ai_generate_documentation(const char* source_code);

// ===============================================
// 进化策略
// ===============================================

typedef enum {
    EVOLUTION_INCREMENTAL,      // 渐进式进化
    EVOLUTION_AGGRESSIVE,       // 激进式进化
    EVOLUTION_CONSERVATIVE,     // 保守式进化
    EVOLUTION_EXPERIMENTAL      // 实验性进化
} EvolutionStrategy;

typedef struct {
    EvolutionStrategy strategy;
    float mutation_rate;        // 变异率
    float selection_pressure;   // 选择压力
    int population_size;        // 种群大小
    int max_generations;        // 最大代数
} EvolutionConfig;

/**
 * 配置进化策略
 */
void ai_configure_evolution(EvolutionConfig* config);

/**
 * 执行代码进化
 */
char* ai_evolve_code(const char* source_code, EvolutionConfig* config);

// ===============================================
// 资源清理
// ===============================================

/**
 * 释放分析结果
 */
void ai_free_analysis_result(CodeAnalysisResult* result);

/**
 * 释放改进建议
 */
void ai_free_improvements(CodeImprovement* improvements, int count);

/**
 * 释放模式匹配结果
 */
void ai_free_pattern_matches(PatternMatch* matches, int count);

/**
 * 清理AI分析器
 */
void ai_analyzer_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // CODE_ANALYZER_H
