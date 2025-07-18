#ifndef ASTC_EXECUTION_OPTIMIZER_H
#define ASTC_EXECUTION_OPTIMIZER_H

#include "astc.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// VM状态枚举
typedef enum {
    VM_STATE_READY,
    VM_STATE_RUNNING,
    VM_STATE_STOPPED,
    VM_STATE_ERROR
} VMState;

// T3.2 ASTC字节码执行优化器
// 目标: 字节码执行性能提升25%以上

#ifdef __cplusplus
extern "C" {
#endif

// 优化配置
typedef struct {
    bool enable_jump_table;          // 启用跳转表分发
    bool enable_instruction_fusion;  // 启用指令融合
    bool enable_register_allocation; // 启用寄存器分配
    bool enable_hot_spot_detection;  // 启用热点检测
    bool enable_jit_compilation;     // 启用JIT编译
    bool enable_instruction_cache;   // 启用指令缓存
    
    int hot_spot_threshold;          // 热点检测阈值
    int instruction_cache_size;      // 指令缓存大小
    int register_count;              // 虚拟寄存器数量
    double jit_threshold;            // JIT编译阈值
} ASTCExecutionOptimizerConfig;

// 执行统计
typedef struct {
    uint64_t total_instructions;     // 总指令数
    uint64_t total_executions;       // 总执行次数
    uint64_t cache_hits;             // 缓存命中次数
    uint64_t cache_misses;           // 缓存未命中次数
    uint64_t jit_compilations;       // JIT编译次数
    uint64_t hot_spots_detected;     // 检测到的热点数
    
    double total_execution_time;     // 总执行时间
    double avg_execution_time;       // 平均执行时间
    double jit_compile_time;         // JIT编译时间
    double cache_hit_time;           // 缓存命中平均时间
    
    time_t last_update;              // 最后更新时间
} ASTCExecutionStats;

// 指令缓存条目
typedef struct InstructionCacheEntry {
    uint64_t hash;                   // 指令序列哈希
    ASTCInstruction* instructions;   // 缓存的指令序列
    int instruction_count;           // 指令数量
    void* optimized_code;            // 优化后的代码
    size_t code_size;                // 代码大小
    int access_count;                // 访问次数
    time_t last_access;              // 最后访问时间
    struct InstructionCacheEntry* next; // 链表下一项
} InstructionCacheEntry;

// 热点检测条目
typedef struct HotSpotEntry {
    uint64_t pc_start;               // 热点起始PC
    uint64_t pc_end;                 // 热点结束PC
    int execution_count;             // 执行次数
    double total_time;               // 总执行时间
    bool is_compiled;                // 是否已编译
    void* compiled_code;             // 编译后的代码
    size_t code_size;                // 代码大小
    struct HotSpotEntry* next;       // 链表下一项
} HotSpotEntry;

// 优化的VM上下文
typedef struct {
    // 基础VM状态
    VMState state;
    ASTCBytecodeProgram* program;
    size_t program_counter;
    
    // 优化的栈和寄存器
    uint64_t* stack;
    size_t stack_size;
    size_t stack_pointer;
    uint64_t* registers;             // 扩展寄存器组
    int register_count;
    
    // 优化器配置和统计
    ASTCExecutionOptimizerConfig config;
    ASTCExecutionStats stats;
    
    // 指令缓存系统
    InstructionCacheEntry* instruction_cache[256]; // 指令缓存哈希表
    int cache_entry_count;
    
    // 热点检测系统
    HotSpotEntry* hot_spots[64];     // 热点哈希表
    int hot_spot_count;
    
    // JIT编译系统
    void* jit_context;               // JIT编译器上下文
    bool jit_enabled;                // JIT是否启用
    
    // 跳转表 (用于优化指令分发)
    void* jump_table[256];           // 指令跳转表
    
    // 错误处理
    char error_message[256];
    
    // 性能计时
    double execution_start_time;
    
    // 状态管理
    bool is_initialized;
    time_t init_time;
} OptimizedVMContext;

// 全局优化器实例
extern OptimizedVMContext g_astc_optimizer;

// 初始化和清理
int astc_optimizer_init(const ASTCExecutionOptimizerConfig* config);
void astc_optimizer_cleanup(void);
bool astc_optimizer_is_initialized(void);

// 配置管理
int astc_optimizer_set_config(const ASTCExecutionOptimizerConfig* config);
ASTCExecutionOptimizerConfig astc_optimizer_get_config(void);
ASTCExecutionOptimizerConfig astc_optimizer_get_default_config(void);

// 优化执行接口
int astc_optimizer_execute_program(ASTCBytecodeProgram* program);
int astc_optimizer_execute_bytecode(const uint8_t* bytecode, size_t size);
int astc_optimizer_execute_with_context(OptimizedVMContext* ctx, ASTCBytecodeProgram* program);

// 指令分发优化
typedef void (*InstructionHandler)(OptimizedVMContext* ctx, const ASTCInstruction* instr);
void astc_optimizer_init_jump_table(OptimizedVMContext* ctx);
void astc_optimizer_dispatch_instruction(OptimizedVMContext* ctx, const ASTCInstruction* instr);

// 指令缓存管理
int astc_optimizer_cache_instruction_sequence(OptimizedVMContext* ctx, 
                                             ASTCInstruction* instructions, 
                                             int count);
InstructionCacheEntry* astc_optimizer_lookup_cached_sequence(OptimizedVMContext* ctx, 
                                                           uint64_t hash);
int astc_optimizer_clear_instruction_cache(OptimizedVMContext* ctx);

// 热点检测和JIT编译
int astc_optimizer_detect_hot_spots(OptimizedVMContext* ctx);
int astc_optimizer_compile_hot_spot(OptimizedVMContext* ctx, HotSpotEntry* hot_spot);
int astc_optimizer_execute_compiled_code(OptimizedVMContext* ctx, void* code, size_t size);

// 寄存器分配优化
int astc_optimizer_allocate_registers(OptimizedVMContext* ctx, ASTCBytecodeProgram* program);
int astc_optimizer_optimize_stack_usage(OptimizedVMContext* ctx);

// 指令融合优化
int astc_optimizer_fuse_instructions(OptimizedVMContext* ctx, 
                                   ASTCInstruction* instructions, 
                                   int count);
bool astc_optimizer_can_fuse_instructions(const ASTCInstruction* instr1, 
                                        const ASTCInstruction* instr2);

// 性能统计
ASTCExecutionStats astc_optimizer_get_stats(void);
void astc_optimizer_reset_stats(void);
double astc_optimizer_get_cache_hit_rate(void);
double astc_optimizer_get_avg_execution_time(void);
int astc_optimizer_get_hot_spot_count(void);

// 优化分析
int astc_optimizer_analyze_performance(void);
int astc_optimizer_suggest_optimizations(void);
int astc_optimizer_auto_tune(void);

// 调试和监控
void astc_optimizer_print_stats(void);
void astc_optimizer_print_cache_info(void);
void astc_optimizer_print_hot_spots(void);
int astc_optimizer_export_stats(const char* filename);

// 实用工具
uint64_t astc_optimizer_hash_instruction_sequence(ASTCInstruction* instructions, int count);
double astc_optimizer_get_time(void);
int astc_optimizer_compare_performance(const ASTCExecutionStats* before, 
                                     const ASTCExecutionStats* after);

// 优化的指令处理函数
void astc_handle_i32_const(OptimizedVMContext* ctx, const ASTCInstruction* instr);
void astc_handle_i64_const(OptimizedVMContext* ctx, const ASTCInstruction* instr);
void astc_handle_i32_add(OptimizedVMContext* ctx, const ASTCInstruction* instr);
void astc_handle_i32_sub(OptimizedVMContext* ctx, const ASTCInstruction* instr);
void astc_handle_i32_mul(OptimizedVMContext* ctx, const ASTCInstruction* instr);
void astc_handle_local_get(OptimizedVMContext* ctx, const ASTCInstruction* instr);
void astc_handle_local_set(OptimizedVMContext* ctx, const ASTCInstruction* instr);
void astc_handle_return(OptimizedVMContext* ctx, const ASTCInstruction* instr);
void astc_handle_drop(OptimizedVMContext* ctx, const ASTCInstruction* instr);
void astc_handle_nop(OptimizedVMContext* ctx, const ASTCInstruction* instr);

// 内联栈操作 (优化版本)
static inline bool astc_optimizer_push(OptimizedVMContext* ctx, uint64_t value) {
    if (ctx->stack_pointer >= ctx->stack_size) {
        return false;
    }
    ctx->stack[ctx->stack_pointer++] = value;
    return true;
}

static inline bool astc_optimizer_pop(OptimizedVMContext* ctx, uint64_t* value) {
    if (ctx->stack_pointer == 0) {
        return false;
    }
    *value = ctx->stack[--ctx->stack_pointer];
    return true;
}

static inline uint64_t astc_optimizer_peek(OptimizedVMContext* ctx) {
    if (ctx->stack_pointer == 0) return 0;
    return ctx->stack[ctx->stack_pointer - 1];
}

#ifdef __cplusplus
}
#endif

#endif // ASTC_EXECUTION_OPTIMIZER_H
