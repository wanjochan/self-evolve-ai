/**
 * enhanced_jit_compiler.h - 增强的JIT编译器
 * 
 * 提升代码生成质量和性能的JIT编译器
 */

#ifndef ENHANCED_JIT_COMPILER_H
#define ENHANCED_JIT_COMPILER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "compiler_astc2rt.h"

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// 增强的代码生成器
// ===============================================

typedef struct {
    uint8_t* code;              // 机器码缓冲区
    size_t code_size;           // 当前代码大小
    size_t code_capacity;       // 缓冲区容量
    TargetArch target_arch;     // 目标架构
    
    // 优化选项
    bool enable_optimizations;  // 启用优化
    bool enable_register_allocation; // 启用寄存器分配
    bool enable_dead_code_elimination; // 启用死代码消除
    bool enable_constant_folding; // 启用常量折叠
    
    // 寄存器分配
    uint32_t register_usage[16]; // 寄存器使用情况
    uint32_t next_virtual_reg;   // 下一个虚拟寄存器
    
    // 跳转标签管理
    uint32_t* jump_labels;       // 跳转标签地址
    uint32_t jump_label_count;   // 标签数量
    uint32_t jump_label_capacity; // 标签容量
    
    // 函数调用栈
    uint32_t stack_offset;       // 当前栈偏移
    uint32_t max_stack_size;     // 最大栈大小
    
    // 性能统计
    uint32_t instructions_compiled; // 编译的指令数
    uint32_t optimizations_applied; // 应用的优化数
    uint64_t compilation_time_us;   // 编译时间(微秒)
} EnhancedCodeGen;

// ===============================================
// JIT编译器优化级别
// ===============================================

typedef enum {
    JIT_OPT_NONE = 0,           // 无优化
    JIT_OPT_BASIC = 1,          // 基础优化
    JIT_OPT_AGGRESSIVE = 2,     // 激进优化
    JIT_OPT_SIZE = 3,           // 大小优化
    JIT_OPT_SPEED = 4           // 速度优化
} JitOptLevel;

typedef struct {
    JitOptLevel opt_level;      // 优化级别
    bool inline_functions;      // 内联函数
    bool unroll_loops;          // 循环展开
    bool vectorize;             // 向量化
    bool profile_guided;        // 配置文件引导优化
    uint32_t max_inline_size;   // 最大内联大小
    uint32_t max_unroll_count;  // 最大展开次数
} JitOptOptions;

// ===============================================
// 增强的JIT编译器API
// ===============================================

/**
 * 创建增强的代码生成器
 */
EnhancedCodeGen* enhanced_codegen_create(TargetArch arch, JitOptOptions* options);

/**
 * 释放增强的代码生成器
 */
void enhanced_codegen_free(EnhancedCodeGen* gen);

/**
 * 编译ASTC到优化的机器码
 */
int enhanced_compile_astc_to_machine_code(uint8_t* astc_data, size_t astc_size, 
                                         EnhancedCodeGen* gen);

/**
 * 编译单个ASTC指令到优化的机器码
 */
int enhanced_compile_instruction(EnhancedCodeGen* gen, uint8_t opcode, 
                                uint8_t* operands, size_t operand_len);

/**
 * 应用代码优化
 */
int enhanced_apply_optimizations(EnhancedCodeGen* gen);

/**
 * 寄存器分配
 */
int enhanced_allocate_registers(EnhancedCodeGen* gen);

/**
 * 死代码消除
 */
int enhanced_eliminate_dead_code(EnhancedCodeGen* gen);

/**
 * 常量折叠
 */
int enhanced_fold_constants(EnhancedCodeGen* gen);

/**
 * 循环优化
 */
int enhanced_optimize_loops(EnhancedCodeGen* gen);

/**
 * 函数内联
 */
int enhanced_inline_functions(EnhancedCodeGen* gen);

// ===============================================
// 架构特定的优化代码生成
// ===============================================

/**
 * x64架构优化代码生成
 */
int enhanced_emit_x64_optimized(EnhancedCodeGen* gen, uint8_t opcode, 
                               uint8_t* operands, size_t operand_len);

/**
 * ARM64架构优化代码生成
 */
int enhanced_emit_arm64_optimized(EnhancedCodeGen* gen, uint8_t opcode, 
                                 uint8_t* operands, size_t operand_len);

/**
 * 通用架构优化代码生成
 */
int enhanced_emit_generic_optimized(EnhancedCodeGen* gen, uint8_t opcode, 
                                   uint8_t* operands, size_t operand_len);

// ===============================================
// 性能分析和调试
// ===============================================

/**
 * 获取编译统计信息
 */
typedef struct {
    uint32_t total_instructions;
    uint32_t optimized_instructions;
    uint32_t register_spills;
    uint32_t function_calls;
    uint32_t memory_accesses;
    uint64_t compilation_time_us;
    size_t code_size_before_opt;
    size_t code_size_after_opt;
    float optimization_ratio;
} JitCompilationStats;

void enhanced_get_compilation_stats(EnhancedCodeGen* gen, JitCompilationStats* stats);

/**
 * 打印编译统计信息
 */
void enhanced_print_compilation_stats(EnhancedCodeGen* gen);

/**
 * 生成优化报告
 */
void enhanced_generate_optimization_report(EnhancedCodeGen* gen, const char* filename);

/**
 * 验证生成的机器码
 */
bool enhanced_validate_machine_code(EnhancedCodeGen* gen);

// ===============================================
// 运行时性能监控
// ===============================================

/**
 * 运行时性能计数器
 */
typedef struct {
    uint64_t instructions_executed;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t branch_predictions;
    uint64_t branch_mispredictions;
    uint64_t execution_time_us;
} RuntimePerfCounters;

/**
 * 启用性能监控
 */
int enhanced_enable_perf_monitoring(EnhancedCodeGen* gen);

/**
 * 获取运行时性能数据
 */
void enhanced_get_runtime_perf(RuntimePerfCounters* counters);

/**
 * 基于性能数据重新优化
 */
int enhanced_reoptimize_based_on_profile(EnhancedCodeGen* gen, RuntimePerfCounters* counters);

// ===============================================
// 多线程JIT编译支持
// ===============================================

/**
 * 并行编译ASTC函数
 */
int enhanced_parallel_compile_functions(uint8_t* astc_data, size_t astc_size,
                                       EnhancedCodeGen** generators, int num_threads);

/**
 * 线程安全的代码生成
 */
int enhanced_thread_safe_emit(EnhancedCodeGen* gen, uint8_t* code, size_t size);

// ===============================================
// 缓存和持久化
// ===============================================

/**
 * 保存编译缓存
 */
int enhanced_save_compilation_cache(EnhancedCodeGen* gen, const char* cache_file);

/**
 * 加载编译缓存
 */
int enhanced_load_compilation_cache(EnhancedCodeGen* gen, const char* cache_file);

/**
 * 清理编译缓存
 */
void enhanced_cleanup_compilation_cache(void);

// ===============================================
// 默认配置
// ===============================================

/**
 * 获取默认优化选项
 */
JitOptOptions enhanced_get_default_opt_options(void);

/**
 * 获取性能优化选项
 */
JitOptOptions enhanced_get_performance_opt_options(void);

/**
 * 获取大小优化选项
 */
JitOptOptions enhanced_get_size_opt_options(void);

#ifdef __cplusplus
}
#endif

#endif // ENHANCED_JIT_COMPILER_H
