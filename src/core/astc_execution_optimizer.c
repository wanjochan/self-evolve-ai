#include "astc_execution_optimizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

// 全局优化器实例
OptimizedVMContext g_astc_optimizer = {0};

// 获取高精度时间
double astc_optimizer_get_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// 指令序列哈希函数
uint64_t astc_optimizer_hash_instruction_sequence(ASTCInstruction* instructions, int count) {
    if (!instructions || count <= 0) return 0;
    
    uint64_t hash = 5381;
    for (int i = 0; i < count; i++) {
        hash = ((hash << 5) + hash) + (uint64_t)instructions[i].opcode;
        hash = ((hash << 5) + hash) + (uint64_t)instructions[i].operand.i64;
    }
    return hash;
}

// 获取默认配置
ASTCExecutionOptimizerConfig astc_optimizer_get_default_config(void) {
    ASTCExecutionOptimizerConfig config = {
        .enable_jump_table = true,
        .enable_instruction_fusion = true,
        .enable_register_allocation = true,
        .enable_hot_spot_detection = true,
        .enable_jit_compilation = false,  // 暂时禁用JIT
        .enable_instruction_cache = true,
        
        .hot_spot_threshold = 100,
        .instruction_cache_size = 256,
        .register_count = 32,
        .jit_threshold = 0.8
    };
    return config;
}

// 初始化优化器
int astc_optimizer_init(const ASTCExecutionOptimizerConfig* config) {
    if (g_astc_optimizer.is_initialized) {
        return 0;  // 已经初始化
    }
    
    // 使用默认配置或提供的配置
    if (config) {
        g_astc_optimizer.config = *config;
    } else {
        g_astc_optimizer.config = astc_optimizer_get_default_config();
    }
    
    // 初始化统计信息
    memset(&g_astc_optimizer.stats, 0, sizeof(ASTCExecutionStats));
    g_astc_optimizer.stats.last_update = time(NULL);
    
    // 初始化栈
    g_astc_optimizer.stack_size = 2048;  // 增大栈大小
    g_astc_optimizer.stack = malloc(g_astc_optimizer.stack_size * sizeof(uint64_t));
    g_astc_optimizer.stack_pointer = 0;
    
    // 初始化寄存器
    g_astc_optimizer.register_count = g_astc_optimizer.config.register_count;
    g_astc_optimizer.registers = malloc(g_astc_optimizer.register_count * sizeof(uint64_t));
    memset(g_astc_optimizer.registers, 0, g_astc_optimizer.register_count * sizeof(uint64_t));
    
    // 初始化指令缓存
    memset(g_astc_optimizer.instruction_cache, 0, sizeof(g_astc_optimizer.instruction_cache));
    g_astc_optimizer.cache_entry_count = 0;
    
    // 初始化热点检测
    memset(g_astc_optimizer.hot_spots, 0, sizeof(g_astc_optimizer.hot_spots));
    g_astc_optimizer.hot_spot_count = 0;
    
    // 初始化跳转表
    if (g_astc_optimizer.config.enable_jump_table) {
        astc_optimizer_init_jump_table(&g_astc_optimizer);
    }
    
    g_astc_optimizer.state = VM_STATE_READY;
    g_astc_optimizer.is_initialized = true;
    g_astc_optimizer.init_time = time(NULL);
    
    printf("ASTC Optimizer: 初始化完成\n");
    printf("  跳转表分发: %s\n", g_astc_optimizer.config.enable_jump_table ? "启用" : "禁用");
    printf("  指令融合: %s\n", g_astc_optimizer.config.enable_instruction_fusion ? "启用" : "禁用");
    printf("  寄存器分配: %s\n", g_astc_optimizer.config.enable_register_allocation ? "启用" : "禁用");
    printf("  热点检测: %s\n", g_astc_optimizer.config.enable_hot_spot_detection ? "启用" : "禁用");
    printf("  指令缓存: %s (%d 条目)\n", 
           g_astc_optimizer.config.enable_instruction_cache ? "启用" : "禁用",
           g_astc_optimizer.config.instruction_cache_size);
    printf("  虚拟寄存器: %d 个\n", g_astc_optimizer.register_count);
    
    return 0;
}

// 清理优化器
void astc_optimizer_cleanup(void) {
    if (!g_astc_optimizer.is_initialized) {
        return;
    }
    
    // 清理栈和寄存器
    free(g_astc_optimizer.stack);
    free(g_astc_optimizer.registers);
    
    // 清理指令缓存
    for (int i = 0; i < 256; i++) {
        InstructionCacheEntry* entry = g_astc_optimizer.instruction_cache[i];
        while (entry) {
            InstructionCacheEntry* next = entry->next;
            free(entry->instructions);
            free(entry->optimized_code);
            free(entry);
            entry = next;
        }
    }
    
    // 清理热点
    for (int i = 0; i < 64; i++) {
        HotSpotEntry* entry = g_astc_optimizer.hot_spots[i];
        while (entry) {
            HotSpotEntry* next = entry->next;
            free(entry->compiled_code);
            free(entry);
            entry = next;
        }
    }
    
    g_astc_optimizer.is_initialized = false;
    printf("ASTC Optimizer: 清理完成\n");
}

// 检查是否已初始化
bool astc_optimizer_is_initialized(void) {
    return g_astc_optimizer.is_initialized;
}

// 初始化跳转表
void astc_optimizer_init_jump_table(OptimizedVMContext* ctx) {
    if (!ctx) return;
    
    // 初始化跳转表为NULL
    memset(ctx->jump_table, 0, sizeof(ctx->jump_table));
    
    // 设置常用指令的处理函数
    ctx->jump_table[AST_I32_CONST] = (void*)astc_handle_i32_const;
    ctx->jump_table[AST_I64_CONST] = (void*)astc_handle_i64_const;
    ctx->jump_table[AST_I32_ADD] = (void*)astc_handle_i32_add;
    ctx->jump_table[AST_I32_SUB] = (void*)astc_handle_i32_sub;
    ctx->jump_table[AST_I32_MUL] = (void*)astc_handle_i32_mul;
    ctx->jump_table[AST_LOCAL_GET] = (void*)astc_handle_local_get;
    ctx->jump_table[AST_LOCAL_SET] = (void*)astc_handle_local_set;
    ctx->jump_table[AST_RETURN] = (void*)astc_handle_return;
    ctx->jump_table[AST_DROP] = (void*)astc_handle_drop;
    ctx->jump_table[AST_NOP] = (void*)astc_handle_nop;
}

// 优化的指令分发
void astc_optimizer_dispatch_instruction(OptimizedVMContext* ctx, const ASTCInstruction* instr) {
    if (!ctx || !instr) return;
    
    if (ctx->config.enable_jump_table && instr->opcode < 256) {
        InstructionHandler handler = (InstructionHandler)ctx->jump_table[instr->opcode];
        if (handler) {
            handler(ctx, instr);
            return;
        }
    }
    
    // 回退到switch分发
    switch (instr->opcode) {
        case AST_I32_CONST:
            astc_handle_i32_const(ctx, instr);
            break;
        case AST_I64_CONST:
            astc_handle_i64_const(ctx, instr);
            break;
        case AST_I32_ADD:
            astc_handle_i32_add(ctx, instr);
            break;
        case AST_I32_SUB:
            astc_handle_i32_sub(ctx, instr);
            break;
        case AST_I32_MUL:
            astc_handle_i32_mul(ctx, instr);
            break;
        case AST_LOCAL_GET:
            astc_handle_local_get(ctx, instr);
            break;
        case AST_LOCAL_SET:
            astc_handle_local_set(ctx, instr);
            break;
        case AST_RETURN:
            astc_handle_return(ctx, instr);
            break;
        case AST_DROP:
            astc_handle_drop(ctx, instr);
            break;
        case AST_NOP:
            astc_handle_nop(ctx, instr);
            break;
        default:
            snprintf(ctx->error_message, sizeof(ctx->error_message), 
                    "Unsupported opcode: %d", instr->opcode);
            ctx->state = VM_STATE_ERROR;
            break;
    }
}

// 优化的程序执行
int astc_optimizer_execute_program(ASTCBytecodeProgram* program) {
    if (!g_astc_optimizer.is_initialized) {
        if (astc_optimizer_init(NULL) != 0) {
            return -1;
        }
    }
    
    return astc_optimizer_execute_with_context(&g_astc_optimizer, program);
}

// 使用上下文执行程序
int astc_optimizer_execute_with_context(OptimizedVMContext* ctx, ASTCBytecodeProgram* program) {
    if (!ctx || !program) return -1;
    
    // 验证程序格式
    if (memcmp(program->magic, "ASTC", 4) != 0) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), "Invalid ASTC magic");
        return -1;
    }
    
    ctx->program = program;
    ctx->program_counter = program->entry_point;
    ctx->state = VM_STATE_RUNNING;
    ctx->execution_start_time = astc_optimizer_get_time();
    
    // 主执行循环
    while (ctx->state == VM_STATE_RUNNING && 
           ctx->program_counter < program->instruction_count) {
        
        ASTCInstruction* instr = &program->instructions[ctx->program_counter];
        
        // 更新统计
        ctx->stats.total_instructions++;
        
        // 热点检测
        if (ctx->config.enable_hot_spot_detection) {
            // 简化的热点检测逻辑
            uint64_t pc_hash = ctx->program_counter % 64;
            HotSpotEntry* hot_spot = ctx->hot_spots[pc_hash];
            if (hot_spot && hot_spot->pc_start == ctx->program_counter) {
                hot_spot->execution_count++;
            }
        }
        
        // 执行指令
        astc_optimizer_dispatch_instruction(ctx, instr);
        
        if (ctx->state == VM_STATE_ERROR) {
            return -1;
        }
        
        ctx->program_counter++;
    }
    
    // 更新执行统计
    double execution_time = astc_optimizer_get_time() - ctx->execution_start_time;
    ctx->stats.total_executions++;
    ctx->stats.total_execution_time += execution_time;
    ctx->stats.avg_execution_time = ctx->stats.total_execution_time / ctx->stats.total_executions;
    ctx->stats.last_update = time(NULL);
    
    if (ctx->state == VM_STATE_RUNNING) {
        ctx->state = VM_STATE_STOPPED;
    }
    
    return ctx->state == VM_STATE_STOPPED ? 0 : -1;
}

// 获取统计信息
ASTCExecutionStats astc_optimizer_get_stats(void) {
    if (!g_astc_optimizer.is_initialized) {
        ASTCExecutionStats empty_stats = {0};
        return empty_stats;
    }
    
    return g_astc_optimizer.stats;
}

// 获取缓存命中率
double astc_optimizer_get_cache_hit_rate(void) {
    if (!g_astc_optimizer.is_initialized) {
        return 0.0;
    }
    
    uint64_t total_accesses = g_astc_optimizer.stats.cache_hits + g_astc_optimizer.stats.cache_misses;
    if (total_accesses == 0) {
        return 0.0;
    }
    
    return (double)g_astc_optimizer.stats.cache_hits / total_accesses;
}

// 打印统计信息
void astc_optimizer_print_stats(void) {
    if (!g_astc_optimizer.is_initialized) {
        printf("ASTC Optimizer: 未初始化\n");
        return;
    }
    
    ASTCExecutionStats stats = astc_optimizer_get_stats();
    
    printf("=== ASTC执行优化器统计信息 ===\n");
    printf("总指令数: %lu\n", stats.total_instructions);
    printf("总执行次数: %lu\n", stats.total_executions);
    printf("缓存命中: %lu\n", stats.cache_hits);
    printf("缓存未命中: %lu\n", stats.cache_misses);
    printf("缓存命中率: %.2f%%\n", astc_optimizer_get_cache_hit_rate() * 100);
    printf("平均执行时间: %.9f 秒\n", stats.avg_execution_time);
    printf("JIT编译次数: %lu\n", stats.jit_compilations);
    printf("热点检测数: %lu\n", stats.hot_spots_detected);
    printf("运行时间: %ld 秒\n", time(NULL) - g_astc_optimizer.init_time);
    printf("=============================\n");
}

// 重置统计信息
void astc_optimizer_reset_stats(void) {
    if (!g_astc_optimizer.is_initialized) {
        return;
    }
    
    memset(&g_astc_optimizer.stats, 0, sizeof(ASTCExecutionStats));
    g_astc_optimizer.stats.last_update = time(NULL);
    printf("ASTC Optimizer: 统计信息已重置\n");
}

// 性能比较
int astc_optimizer_compare_performance(const ASTCExecutionStats* before, 
                                     const ASTCExecutionStats* after) {
    if (!before || !after) {
        return -1;
    }
    
    printf("=== ASTC执行优化对比 ===\n");
    
    // 执行时间对比
    if (before->avg_execution_time > 0 && after->avg_execution_time > 0) {
        double time_improvement = (before->avg_execution_time - after->avg_execution_time) 
                                / before->avg_execution_time * 100;
        printf("平均执行时间: %.9f -> %.9f 秒 (改进: %.1f%%)\n", 
               before->avg_execution_time, after->avg_execution_time, time_improvement);
    }
    
    // 指令执行速度对比
    if (before->total_execution_time > 0 && after->total_execution_time > 0) {
        double before_ips = before->total_instructions / before->total_execution_time;
        double after_ips = after->total_instructions / after->total_execution_time;
        double ips_improvement = (after_ips - before_ips) / before_ips * 100;
        printf("指令执行速度: %.0f -> %.0f 指令/秒 (改进: %.1f%%)\n", 
               before_ips, after_ips, ips_improvement);
    }
    
    // 缓存命中率对比
    uint64_t before_total = before->cache_hits + before->cache_misses;
    uint64_t after_total = after->cache_hits + after->cache_misses;
    
    if (before_total > 0 && after_total > 0) {
        double before_hit_rate = (double)before->cache_hits / before_total;
        double after_hit_rate = (double)after->cache_hits / after_total;
        printf("缓存命中率: %.1f%% -> %.1f%% (改进: %.1f%%)\n", 
               before_hit_rate * 100, after_hit_rate * 100, 
               (after_hit_rate - before_hit_rate) * 100);
    }
    
    printf("==================\n");
    return 0;
}

// 优化的指令处理函数实现

// I32常量指令
void astc_handle_i32_const(OptimizedVMContext* ctx, const ASTCInstruction* instr) {
    if (!astc_optimizer_push(ctx, (uint64_t)instr->operand.i32)) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), "Stack overflow");
        ctx->state = VM_STATE_ERROR;
    }
}

// I64常量指令
void astc_handle_i64_const(OptimizedVMContext* ctx, const ASTCInstruction* instr) {
    if (!astc_optimizer_push(ctx, instr->operand.i64)) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), "Stack overflow");
        ctx->state = VM_STATE_ERROR;
    }
}

// I32加法指令 (优化版本)
void astc_handle_i32_add(OptimizedVMContext* ctx, const ASTCInstruction* instr) {
    uint64_t b, a;
    if (!astc_optimizer_pop(ctx, &b) || !astc_optimizer_pop(ctx, &a)) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), "Stack underflow");
        ctx->state = VM_STATE_ERROR;
        return;
    }

    // 执行32位加法
    uint32_t result = (uint32_t)a + (uint32_t)b;
    if (!astc_optimizer_push(ctx, (uint64_t)result)) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), "Stack overflow");
        ctx->state = VM_STATE_ERROR;
    }
}

// I32减法指令
void astc_handle_i32_sub(OptimizedVMContext* ctx, const ASTCInstruction* instr) {
    uint64_t b, a;
    if (!astc_optimizer_pop(ctx, &b) || !astc_optimizer_pop(ctx, &a)) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), "Stack underflow");
        ctx->state = VM_STATE_ERROR;
        return;
    }

    uint32_t result = (uint32_t)a - (uint32_t)b;
    if (!astc_optimizer_push(ctx, (uint64_t)result)) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), "Stack overflow");
        ctx->state = VM_STATE_ERROR;
    }
}

// I32乘法指令
void astc_handle_i32_mul(OptimizedVMContext* ctx, const ASTCInstruction* instr) {
    uint64_t b, a;
    if (!astc_optimizer_pop(ctx, &b) || !astc_optimizer_pop(ctx, &a)) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), "Stack underflow");
        ctx->state = VM_STATE_ERROR;
        return;
    }

    uint32_t result = (uint32_t)a * (uint32_t)b;
    if (!astc_optimizer_push(ctx, (uint64_t)result)) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), "Stack overflow");
        ctx->state = VM_STATE_ERROR;
    }
}

// 本地变量获取指令 (优化版本 - 使用寄存器)
void astc_handle_local_get(OptimizedVMContext* ctx, const ASTCInstruction* instr) {
    uint32_t index = instr->operand.index;

    // 优化：优先使用寄存器
    if (ctx->config.enable_register_allocation && index < ctx->register_count) {
        if (!astc_optimizer_push(ctx, ctx->registers[index])) {
            snprintf(ctx->error_message, sizeof(ctx->error_message), "Stack overflow");
            ctx->state = VM_STATE_ERROR;
        }
        return;
    }

    // 回退到栈操作
    if (index >= ctx->stack_size) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), "Local index out of bounds");
        ctx->state = VM_STATE_ERROR;
        return;
    }

    if (!astc_optimizer_push(ctx, ctx->stack[index])) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), "Stack overflow");
        ctx->state = VM_STATE_ERROR;
    }
}

// 本地变量设置指令 (优化版本 - 使用寄存器)
void astc_handle_local_set(OptimizedVMContext* ctx, const ASTCInstruction* instr) {
    uint32_t index = instr->operand.index;
    uint64_t value;

    if (!astc_optimizer_pop(ctx, &value)) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), "Stack underflow");
        ctx->state = VM_STATE_ERROR;
        return;
    }

    // 优化：优先使用寄存器
    if (ctx->config.enable_register_allocation && index < ctx->register_count) {
        ctx->registers[index] = value;
        return;
    }

    // 回退到栈操作
    if (index >= ctx->stack_size) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), "Local index out of bounds");
        ctx->state = VM_STATE_ERROR;
        return;
    }

    ctx->stack[index] = value;
}

// 返回指令
void astc_handle_return(OptimizedVMContext* ctx, const ASTCInstruction* instr) {
    ctx->state = VM_STATE_STOPPED;
}

// 丢弃栈顶指令
void astc_handle_drop(OptimizedVMContext* ctx, const ASTCInstruction* instr) {
    uint64_t value;
    if (!astc_optimizer_pop(ctx, &value)) {
        snprintf(ctx->error_message, sizeof(ctx->error_message), "Stack underflow");
        ctx->state = VM_STATE_ERROR;
    }
}

// 空操作指令
void astc_handle_nop(OptimizedVMContext* ctx, const ASTCInstruction* instr) {
    // 什么都不做
}
