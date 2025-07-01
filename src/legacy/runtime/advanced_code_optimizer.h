/**
 * advanced_code_optimizer.h - 高级代码优化器
 * 
 * 提升JIT编译器生成的机器码质量和性能
 */

#ifndef ADVANCED_CODE_OPTIMIZER_H
#define ADVANCED_CODE_OPTIMIZER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// 优化级别和策略
// ===============================================

typedef enum {
    OPT_LEVEL_NONE = 0,         // 无优化
    OPT_LEVEL_BASIC = 1,        // 基础优化
    OPT_LEVEL_STANDARD = 2,     // 标准优化
    OPT_LEVEL_AGGRESSIVE = 3,   // 激进优化
    OPT_LEVEL_EXTREME = 4       // 极限优化
} OptimizationLevel;

typedef enum {
    OPT_STRATEGY_SIZE = 1,      // 大小优化
    OPT_STRATEGY_SPEED = 2,     // 速度优化
    OPT_STRATEGY_BALANCED = 3,  // 平衡优化
    OPT_STRATEGY_POWER = 4,     // 功耗优化
    OPT_STRATEGY_DEBUG = 5      // 调试友好优化
} OptimizationStrategy;

// ===============================================
// 代码分析和优化上下文
// ===============================================

typedef struct BasicBlock {
    uint32_t id;
    uint8_t* code;              // 基本块代码
    size_t code_size;           // 代码大小
    uint32_t* predecessors;     // 前驱块
    uint32_t predecessor_count;
    uint32_t* successors;       // 后继块
    uint32_t successor_count;
    
    // 分析信息
    bool is_loop_header;        // 是否为循环头
    bool is_hot_path;           // 是否为热路径
    uint32_t execution_count;   // 执行次数估计
    
    // 优化标记
    bool optimized;
    uint32_t optimization_flags;
} BasicBlock;

typedef struct ControlFlowGraph {
    BasicBlock* blocks;
    uint32_t block_count;
    uint32_t block_capacity;
    uint32_t entry_block;
    uint32_t* exit_blocks;
    uint32_t exit_block_count;
} ControlFlowGraph;

typedef struct CodeOptimizer {
    OptimizationLevel level;
    OptimizationStrategy strategy;
    
    // 控制流图
    ControlFlowGraph* cfg;
    
    // 优化选项
    bool enable_constant_folding;      // 常量折叠
    bool enable_dead_code_elimination; // 死代码消除
    bool enable_common_subexpression;  // 公共子表达式消除
    bool enable_loop_optimization;     // 循环优化
    bool enable_inlining;              // 函数内联
    bool enable_vectorization;         // 向量化
    bool enable_register_allocation;   // 寄存器分配
    bool enable_instruction_scheduling; // 指令调度
    bool enable_branch_prediction;     // 分支预测优化
    bool enable_cache_optimization;    // 缓存优化
    
    // 分析结果
    uint32_t* live_variables;          // 活跃变量
    uint32_t live_var_count;
    uint32_t* def_use_chains;          // 定义-使用链
    uint32_t def_use_count;
    
    // 优化统计
    uint32_t optimizations_applied;
    uint32_t instructions_eliminated;
    uint32_t constants_folded;
    uint32_t loops_optimized;
    uint32_t functions_inlined;
    
    // 性能估计
    uint32_t estimated_cycles_before;
    uint32_t estimated_cycles_after;
    float performance_improvement;
    
} CodeOptimizer;

// ===============================================
// 高级优化API
// ===============================================

/**
 * 创建代码优化器
 */
CodeOptimizer* code_optimizer_create(OptimizationLevel level, OptimizationStrategy strategy);

/**
 * 释放代码优化器
 */
void code_optimizer_free(CodeOptimizer* optimizer);

/**
 * 优化代码
 */
int code_optimizer_optimize(CodeOptimizer* optimizer, uint8_t* code, size_t* code_size);

/**
 * 构建控制流图
 */
int code_optimizer_build_cfg(CodeOptimizer* optimizer, uint8_t* code, size_t code_size);

/**
 * 数据流分析
 */
int code_optimizer_dataflow_analysis(CodeOptimizer* optimizer);

// ===============================================
// 具体优化实现
// ===============================================

/**
 * 常量折叠优化
 */
int code_optimizer_constant_folding(CodeOptimizer* optimizer);

/**
 * 死代码消除
 */
int code_optimizer_dead_code_elimination(CodeOptimizer* optimizer);

/**
 * 公共子表达式消除
 */
int code_optimizer_common_subexpression_elimination(CodeOptimizer* optimizer);

/**
 * 循环优化
 */
int code_optimizer_loop_optimization(CodeOptimizer* optimizer);

/**
 * 函数内联
 */
int code_optimizer_function_inlining(CodeOptimizer* optimizer);

/**
 * 指令调度
 */
int code_optimizer_instruction_scheduling(CodeOptimizer* optimizer);

/**
 * 寄存器分配优化
 */
int code_optimizer_register_allocation(CodeOptimizer* optimizer);

/**
 * 分支预测优化
 */
int code_optimizer_branch_prediction(CodeOptimizer* optimizer);

/**
 * 向量化优化
 */
int code_optimizer_vectorization(CodeOptimizer* optimizer);

/**
 * 缓存优化
 */
int code_optimizer_cache_optimization(CodeOptimizer* optimizer);

// ===============================================
// 性能分析和预测
// ===============================================

/**
 * 估计代码性能
 */
uint32_t code_optimizer_estimate_performance(CodeOptimizer* optimizer, uint8_t* code, size_t code_size);

/**
 * 分析热点路径
 */
int code_optimizer_analyze_hot_paths(CodeOptimizer* optimizer);

/**
 * 预测分支行为
 */
int code_optimizer_predict_branches(CodeOptimizer* optimizer);

/**
 * 分析内存访问模式
 */
int code_optimizer_analyze_memory_patterns(CodeOptimizer* optimizer);

// ===============================================
// 架构特定优化
// ===============================================

/**
 * x86_64特定优化
 */
int code_optimizer_x86_64_specific(CodeOptimizer* optimizer);

/**
 * ARM64特定优化
 */
int code_optimizer_arm64_specific(CodeOptimizer* optimizer);

/**
 * RISC-V特定优化
 */
int code_optimizer_riscv_specific(CodeOptimizer* optimizer);

// ===============================================
// 优化验证和测试
// ===============================================

/**
 * 验证优化正确性
 */
bool code_optimizer_verify_correctness(CodeOptimizer* optimizer, 
                                      uint8_t* original_code, size_t original_size,
                                      uint8_t* optimized_code, size_t optimized_size);

/**
 * 性能基准测试
 */
int code_optimizer_benchmark(CodeOptimizer* optimizer, 
                           uint8_t* code, size_t code_size,
                           uint32_t* cycles_before, uint32_t* cycles_after);

/**
 * 生成优化报告
 */
void code_optimizer_generate_report(CodeOptimizer* optimizer, const char* filename);

// ===============================================
// 调试和可视化
// ===============================================

/**
 * 打印控制流图
 */
void code_optimizer_print_cfg(CodeOptimizer* optimizer);

/**
 * 打印优化统计
 */
void code_optimizer_print_stats(CodeOptimizer* optimizer);

/**
 * 可视化优化过程
 */
int code_optimizer_visualize_optimization(CodeOptimizer* optimizer, const char* output_file);

// ===============================================
// 配置和调优
// ===============================================

/**
 * 设置优化选项
 */
void code_optimizer_set_options(CodeOptimizer* optimizer, 
                               bool constant_folding,
                               bool dead_code_elimination,
                               bool common_subexpression,
                               bool loop_optimization,
                               bool inlining,
                               bool vectorization);

/**
 * 自动调优优化参数
 */
int code_optimizer_auto_tune(CodeOptimizer* optimizer, 
                            uint8_t* test_code, size_t test_size,
                            uint32_t iterations);

/**
 * 获取推荐的优化设置
 */
void code_optimizer_get_recommended_settings(OptimizationLevel level,
                                            OptimizationStrategy strategy,
                                            CodeOptimizer* optimizer);

// ===============================================
// 优化质量评估
// ===============================================

typedef struct OptimizationQuality {
    float code_size_reduction;      // 代码大小减少百分比
    float performance_improvement;  // 性能提升百分比
    float compilation_time;         // 编译时间(秒)
    uint32_t optimizations_applied; // 应用的优化数量
    uint32_t instructions_eliminated; // 消除的指令数
    float optimization_efficiency;   // 优化效率
} OptimizationQuality;

/**
 * 评估优化质量
 */
void code_optimizer_evaluate_quality(CodeOptimizer* optimizer,
                                    uint8_t* original_code, size_t original_size,
                                    uint8_t* optimized_code, size_t optimized_size,
                                    OptimizationQuality* quality);

/**
 * 比较不同优化策略
 */
int code_optimizer_compare_strategies(uint8_t* code, size_t code_size,
                                     OptimizationQuality* results,
                                     uint32_t strategy_count);

#ifdef __cplusplus
}
#endif

#endif // ADVANCED_CODE_OPTIMIZER_H
