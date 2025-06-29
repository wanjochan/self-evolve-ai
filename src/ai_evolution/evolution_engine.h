#ifndef EVOLUTION_ENGINE_H
#define EVOLUTION_ENGINE_H

/**
 * evolution_engine.h - AI自主进化引擎
 * 
 * 这是项目的核心：实现AI自主分析、修改和优化自己代码的能力
 * 使用已建立的自举工具链进行编译和测试
 */

#include "code_analyzer.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// 进化引擎状态
// ===============================================

typedef enum {
    EVOLUTION_IDLE,             // 空闲状态
    EVOLUTION_ANALYZING,        // 分析阶段
    EVOLUTION_GENERATING,       // 生成阶段
    EVOLUTION_TESTING,          // 测试阶段
    EVOLUTION_VALIDATING,       // 验证阶段
    EVOLUTION_DEPLOYING,        // 部署阶段
    EVOLUTION_ERROR             // 错误状态
} EvolutionState;

typedef struct {
    EvolutionState state;
    int generation;             // 当前代数
    int successful_mutations;   // 成功变异数
    int failed_mutations;       // 失败变异数
    float fitness_score;        // 适应度评分
    char* current_target;       // 当前进化目标
    char* last_error;          // 最后错误信息
} EvolutionStatus;

// ===============================================
// 进化目标定义
// ===============================================

typedef enum {
    TARGET_COMPILER_PERFORMANCE,    // 编译器性能优化
    TARGET_RUNTIME_EFFICIENCY,      // 运行时效率优化
    TARGET_CODE_QUALITY,           // 代码质量改进
    TARGET_MEMORY_OPTIMIZATION,    // 内存使用优化
    TARGET_SECURITY_ENHANCEMENT,   // 安全性增强
    TARGET_FEATURE_ADDITION,       // 功能增加
    TARGET_BUG_FIXING              // 错误修复
} EvolutionTarget;

typedef struct {
    EvolutionTarget target;
    char* description;
    char* target_files[10];     // 目标文件列表
    int target_file_count;
    float priority;             // 优先级 0.0-1.0
    bool is_critical;           // 是否关键
} EvolutionGoal;

// ===============================================
// 核心进化接口
// ===============================================

/**
 * 初始化AI进化引擎
 */
int evolution_engine_init(void);

/**
 * 开始自主进化过程
 */
int evolution_start_autonomous_mode(void);

/**
 * 停止进化过程
 */
int evolution_stop(void);

/**
 * 获取当前进化状态
 */
EvolutionStatus* evolution_get_status(void);

/**
 * 设置进化目标
 */
int evolution_set_goals(EvolutionGoal* goals, int count);

/**
 * 执行单次进化迭代
 */
int evolution_iterate(void);

// ===============================================
// 自主分析和改进
// ===============================================

/**
 * 分析整个项目并识别改进机会
 */
int evolution_analyze_project(const char* project_path);

/**
 * 生成改进版本的代码
 */
char* evolution_generate_improved_code(const char* file_path);

/**
 * 使用自举工具链编译测试
 */
bool evolution_compile_and_test(const char* source_file, const char* output_file);

/**
 * 验证改进效果
 */
bool evolution_validate_improvement(const char* original_file, const char* improved_file);

/**
 * 部署改进版本
 */
int evolution_deploy_improvement(const char* improved_file, const char* target_file);

// ===============================================
// 智能决策系统
// ===============================================

/**
 * 评估改进的风险
 */
float evolution_assess_risk(const char* original_code, const char* improved_code);

/**
 * 选择最佳改进方案
 */
CodeImprovement* evolution_select_best_improvements(CodeImprovement* candidates, int count, int* selected_count);

/**
 * 预测改进效果
 */
float evolution_predict_improvement_impact(CodeImprovement* improvement);

/**
 * 学习从历史改进
 */
void evolution_learn_from_history(void);

// ===============================================
// 自举工具链集成
// ===============================================

/**
 * 使用c2astc编译源文件
 */
bool evolution_compile_with_c2astc(const char* source_file, const char* astc_file);

/**
 * 使用runtime执行ASTC文件
 */
bool evolution_execute_with_runtime(const char* astc_file);

/**
 * 运行完整的编译测试流程
 */
bool evolution_run_full_test_suite(void);

/**
 * 验证自举能力
 */
bool evolution_verify_self_hosting(void);

// ===============================================
// 进化历史和学习
// ===============================================

typedef struct {
    char* timestamp;
    EvolutionTarget target;
    char* file_modified;
    char* improvement_description;
    float fitness_before;
    float fitness_after;
    bool was_successful;
    char* error_message;
} EvolutionRecord;

/**
 * 记录进化历史
 */
void evolution_record_attempt(EvolutionRecord* record);

/**
 * 获取进化历史
 */
EvolutionRecord* evolution_get_history(int* count);

/**
 * 分析成功模式
 */
void evolution_analyze_success_patterns(void);

/**
 * 调整进化策略
 */
void evolution_adapt_strategy(void);

// ===============================================
// 安全和回滚机制
// ===============================================

/**
 * 创建代码备份
 */
bool evolution_create_backup(const char* file_path);

/**
 * 回滚到上一个版本
 */
bool evolution_rollback_changes(const char* file_path);

/**
 * 验证系统完整性
 */
bool evolution_verify_system_integrity(void);

/**
 * 紧急停止进化
 */
void evolution_emergency_stop(void);

// ===============================================
// 监控和报告
// ===============================================

/**
 * 生成进化报告
 */
char* evolution_generate_report(void);

/**
 * 监控系统性能
 */
void evolution_monitor_performance(void);

/**
 * 检测异常行为
 */
bool evolution_detect_anomalies(void);

/**
 * 发送进化通知
 */
void evolution_notify_progress(const char* message);

// ===============================================
// 资源清理
// ===============================================

/**
 * 清理进化引擎
 */
void evolution_engine_cleanup(void);

/**
 * 释放进化状态
 */
void evolution_free_status(EvolutionStatus* status);

/**
 * 释放进化记录
 */
void evolution_free_history(EvolutionRecord* records, int count);

#ifdef __cplusplus
}
#endif

#endif // EVOLUTION_ENGINE_H
