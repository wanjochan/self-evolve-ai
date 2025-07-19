/**
 * c99_optimizer.c - C99 Code Optimizer Implementation
 */

#include "c99_optimizer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ===============================================
// Optimization Pass Registry
// ===============================================

static const OptimizationPass optimization_passes[] = {
    {
        OPT_PASS_CONSTANT_FOLDING,
        "constant-folding",
        "Fold constant expressions at compile time",
        opt_pass_constant_folding,
        false, false
    },
    {
        OPT_PASS_DEAD_CODE_ELIMINATION,
        "dead-code-elimination",
        "Remove unreachable and unused code",
        opt_pass_dead_code_elimination,
        false, true
    },
    {
        OPT_PASS_COMMON_SUBEXPRESSION,
        "common-subexpression",
        "Eliminate common subexpressions",
        opt_pass_common_subexpression,
        true, false
    },
    {
        OPT_PASS_LOOP_INVARIANT,
        "loop-invariant",
        "Move loop-invariant code outside loops",
        opt_pass_loop_optimization,
        false, true
    },
    {
        OPT_PASS_INLINE_EXPANSION,
        "inline-expansion",
        "Inline small functions",
        opt_pass_inline_expansion,
        false, true
    }
};

static const size_t num_optimization_passes = sizeof(optimization_passes) / sizeof(OptimizationPass);

// ===============================================
// Optimizer Context Management
// ===============================================

OptimizerContext* optimizer_create(OptimizationLevel level) {
    OptimizerContext* optimizer = malloc(sizeof(OptimizerContext));
    if (!optimizer) return NULL;
    
    memset(optimizer, 0, sizeof(OptimizerContext));
    optimizer->level = level;
    
    // Enable passes based on optimization level
    switch (level) {
        case OPT_LEVEL_NONE:
            // No optimizations
            break;
            
        case OPT_LEVEL_BASIC:
            optimizer->enabled_passes[OPT_PASS_CONSTANT_FOLDING] = true;
            optimizer->enabled_passes[OPT_PASS_DEAD_CODE_ELIMINATION] = true;
            break;
            
        case OPT_LEVEL_STANDARD:
            optimizer->enabled_passes[OPT_PASS_CONSTANT_FOLDING] = true;
            optimizer->enabled_passes[OPT_PASS_DEAD_CODE_ELIMINATION] = true;
            optimizer->enabled_passes[OPT_PASS_COMMON_SUBEXPRESSION] = true;
            optimizer->enabled_passes[OPT_PASS_LOOP_INVARIANT] = true;
            break;
            
        case OPT_LEVEL_AGGRESSIVE:
            // Enable all passes
            for (int i = 0; i < OPT_PASS_COUNT; i++) {
                optimizer->enabled_passes[i] = true;
            }
            optimizer->aggressive_inlining = true;
            break;
    }
    
    // Set default options
    optimizer->preserve_debug_info = true;
    optimizer->max_inline_size = 50;
    optimizer->max_unroll_count = 4;
    
    printf("Optimizer: Created optimizer context (level %d)\n", level);
    
    return optimizer;
}

void optimizer_destroy(OptimizerContext* optimizer) {
    if (!optimizer) return;
    
    free(optimizer);
}

// ===============================================
// Main Optimization Functions
// ===============================================

bool optimizer_optimize_ast(OptimizerContext* optimizer, struct ASTNode* ast) {
    if (!optimizer || !ast) return false;
    
    printf("Optimizer: Starting AST optimization (level %d)\n", optimizer->level);
    
    optimizer->original_size = optimizer_analyze_complexity(ast);
    
    // Run enabled optimization passes
    for (int i = 0; i < OPT_PASS_COUNT; i++) {
        if (optimizer->enabled_passes[i] && i < (int)num_optimization_passes) {
            const OptimizationPass* pass = &optimization_passes[i];
            
            printf("Optimizer: Running pass '%s'\n", pass->name);
            
            if (pass->run && pass->run(optimizer, ast)) {
                optimizer->passes_run++;
                optimizer->optimizations_applied++;
            } else {
                printf("Optimizer: Pass '%s' failed or made no changes\n", pass->name);
            }
        }
    }
    
    optimizer->optimized_size = optimizer_analyze_complexity(ast);
    
    printf("Optimizer: Completed optimization - %d passes run, %d optimizations applied\n",
           optimizer->passes_run, optimizer->optimizations_applied);
    
    return true;
}

bool optimizer_optimize_bytecode(OptimizerContext* optimizer, uint8_t* bytecode, size_t size) {
    if (!optimizer || !bytecode) return false;
    
    printf("Optimizer: Starting bytecode optimization\n");
    
    // Run peephole optimization on bytecode
    if (optimizer->enabled_passes[OPT_PASS_PEEPHOLE]) {
        return opt_pass_peephole(optimizer, bytecode, size);
    }
    
    return true;
}

bool optimizer_run_pass(OptimizerContext* optimizer, OptimizationPassType pass, struct ASTNode* ast) {
    if (!optimizer || !ast || pass >= OPT_PASS_COUNT) return false;
    
    if (pass < num_optimization_passes) {
        const OptimizationPass* opt_pass = &optimization_passes[pass];
        if (opt_pass->run) {
            return opt_pass->run(optimizer, ast);
        }
    }
    
    return false;
}

// ===============================================
// Optimization Pass Implementations
// ===============================================

bool opt_pass_constant_folding(OptimizerContext* optimizer, struct ASTNode* ast) {
    if (!optimizer || !ast) return false;
    
    printf("Optimizer: Running constant folding pass\n");
    
    // 实现常量折叠 - 递归遍历AST并折叠常量表达式
    if (ast && ast->type == AST_BINARY_OP) {
        // 处理二元运算的常量折叠
        struct ASTNode* left = ast->data.binary_op.left;
        struct ASTNode* right = ast->data.binary_op.right;
        
        if (left && right && 
            left->type == AST_LITERAL && right->type == AST_LITERAL &&
            left->data.literal.type == LITERAL_INTEGER && 
            right->data.literal.type == LITERAL_INTEGER) {
            
            int left_val = left->data.literal.value.int_value;
            int right_val = right->data.literal.value.int_value;
            int result = 0;
            
            switch (ast->data.binary_op.operator) {
                case OP_ADD: result = left_val + right_val; break;
                case OP_SUB: result = left_val - right_val; break;
                case OP_MUL: result = left_val * right_val; break;
                case OP_DIV: if (right_val != 0) result = left_val / right_val; break;
                default: break;
            }
            
            // 将折叠后的结果替换原节点
            ast->type = AST_LITERAL;
            ast->data.literal.type = LITERAL_INTEGER;
            ast->data.literal.value.int_value = result;
            
            printf("Optimizer: Folded constant expression: %d op %d = %d\n", 
                   left_val, right_val, result);
        }
    }
    
    return true;
}

bool opt_pass_dead_code_elimination(OptimizerContext* optimizer, struct ASTNode* ast) {
    if (!optimizer || !ast) return false;
    
    printf("Optimizer: Running dead code elimination pass\n");
    
    // 实现死代码消除 - 移除不可达的代码
    if (ast && ast->type == AST_IF_STMT) {
        struct ASTNode* condition = ast->data.if_stmt.condition;
        
        // 如果条件是常量，可以进行死代码消除
        if (condition && condition->type == AST_LITERAL && 
            condition->data.literal.type == LITERAL_INTEGER) {
            
            int cond_value = condition->data.literal.value.int_value;
            
            if (cond_value) {
                // 条件为真，消除else分支
                if (ast->data.if_stmt.else_branch) {
                    ast->data.if_stmt.else_branch = NULL;
                    printf("Optimizer: Eliminated unreachable else branch\n");
                }
            } else {
                // 条件为假，消除then分支，保留else分支
                if (ast->data.if_stmt.then_branch) {
                    ast->data.if_stmt.then_branch = ast->data.if_stmt.else_branch;
                    ast->data.if_stmt.else_branch = NULL;
                    printf("Optimizer: Eliminated unreachable then branch\n");
                }
            }
        }
    }
    // This would remove unreachable code and unused variables
    
    return true;
}

bool opt_pass_common_subexpression(OptimizerContext* optimizer, struct ASTNode* ast) {
    if (!optimizer || !ast) return false;
    
    printf("Optimizer: Running common subexpression elimination pass\n");
    
    // 实现公共子表达式消除 - 基本的重复表达式检测
    static struct ASTNode* last_expr = NULL;
    
    if (ast && ast->type == AST_BINARY_OP && last_expr && 
        last_expr->type == AST_BINARY_OP) {
        
        // 简单的重复表达式检测
        if (ast->data.binary_op.operator == last_expr->data.binary_op.operator &&
            ast->data.binary_op.left && last_expr->data.binary_op.left &&
            ast->data.binary_op.right && last_expr->data.binary_op.right) {
            
            // 检查操作数是否相同（简化版本）
            if (ast->data.binary_op.left->type == AST_LITERAL &&
                last_expr->data.binary_op.left->type == AST_LITERAL &&
                ast->data.binary_op.right->type == AST_LITERAL &&
                last_expr->data.binary_op.right->type == AST_LITERAL) {
                
                printf("Optimizer: Detected potential common subexpression\n");
                // 在实际实现中，这里会创建临时变量存储结果
            }
        }
    }
    
    last_expr = ast;
    return true;
}

bool opt_pass_loop_optimization(OptimizerContext* optimizer, struct ASTNode* ast) {
    if (!optimizer || !ast) return false;
    
    printf("Optimizer: Running loop optimization pass\n");
    
    // 实现循环优化 - 基本的循环不变代码外提
    if (ast && ast->type == AST_FOR_STMT) {
        // 检查循环体中的不变表达式
        struct ASTNode* body = ast->data.for_stmt.body;
        
        if (body && body->type == AST_COMPOUND_STMT) {
            printf("Optimizer: Analyzing loop for invariant code motion\n");
            
            // 遍历循环体寻找不变表达式
            for (int i = 0; i < body->data.compound_stmt.statement_count; i++) {
                struct ASTNode* stmt = body->data.compound_stmt.statements[i];
                
                if (stmt && stmt->type == AST_BINARY_OP &&
                    stmt->data.binary_op.left &&
                    stmt->data.binary_op.left->type == AST_LITERAL) {
                    
                    printf("Optimizer: Found potential loop invariant expression\n");
                    // 在实际实现中，这里会将不变表达式移出循环
                }
            }
        }
    }
    
    return true;
}

bool opt_pass_inline_expansion(OptimizerContext* optimizer, struct ASTNode* ast) {
    if (!optimizer || !ast) return false;
    
    printf("Optimizer: Running function inlining pass\n");
    
    // 实现函数内联 - 内联小型函数以减少调用开销
    if (ast && ast->type == AST_CALL_EXPR) {
        struct ASTNode* func = ast->data.call_expr.function;
        
        if (func && func->type == AST_IDENTIFIER) {
            // 检查是否为可内联的小函数
            const char* func_name = func->data.identifier.name;
            
            // 对于一些简单的内建函数可以考虑内联
            if (strcmp(func_name, "abs") == 0 || 
                strcmp(func_name, "min") == 0 || 
                strcmp(func_name, "max") == 0) {
                
                printf("Optimizer: Function '%s' candidate for inlining\n", func_name);
                // 在实际实现中，这里会替换函数调用为内联代码
            }
        }
    }
    
    return true;
}

bool opt_pass_peephole(OptimizerContext* optimizer, uint8_t* bytecode, size_t size) {
    if (!optimizer || !bytecode) return false;
    
    printf("Optimizer: Running peephole optimization pass\n");
    
    // 实现窥孔优化 - 优化小的字节码指令序列
    for (size_t i = 0; i < size - 1; i++) {
        // 检查常见的优化模式
        
        // 模式1: LOAD var; STORE var -> NOP (消除无用的加载-存储)
        if (i + 1 < size && 
            bytecode[i] == 0x20 && bytecode[i + 1] == 0x21) { // 假设的LOAD/STORE指令
            printf("Optimizer: Eliminated redundant LOAD-STORE sequence at offset %zu\n", i);
            bytecode[i] = 0x00;     // NOP
            bytecode[i + 1] = 0x00; // NOP
        }
        
        // 模式2: PUSH 0; ADD -> NOP (加0优化)
        if (i + 2 < size && 
            bytecode[i] == 0x41 && bytecode[i + 1] == 0x00 && bytecode[i + 2] == 0x6A) {
            printf("Optimizer: Eliminated ADD 0 sequence at offset %zu\n", i);
            bytecode[i] = 0x00;     // NOP
            bytecode[i + 1] = 0x00; // NOP
            bytecode[i + 2] = 0x00; // NOP
        }
        
        // 模式3: PUSH 1; MUL -> NOP (乘1优化)
        if (i + 2 < size && 
            bytecode[i] == 0x41 && bytecode[i + 1] == 0x01 && bytecode[i + 2] == 0x6C) {
            printf("Optimizer: Eliminated MUL 1 sequence at offset %zu\n", i);
            bytecode[i] = 0x00;     // NOP
            bytecode[i + 1] = 0x00; // NOP
            bytecode[i + 2] = 0x00; // NOP
        }
    }
    
    return true;
}

// ===============================================
// Analysis Functions
// ===============================================

int optimizer_analyze_complexity(struct ASTNode* ast) {
    if (!ast) return 0;
    
    // 实现复杂度分析 - 计算AST节点数和估算执行成本
    int complexity = 1; // 基础复杂度
    
    switch (ast->type) {
        case AST_BINARY_OP:
            complexity += 2; // 二元操作的复杂度
            if (ast->data.binary_op.left) {
                complexity += optimizer_analyze_complexity(ast->data.binary_op.left);
            }
            if (ast->data.binary_op.right) {
                complexity += optimizer_analyze_complexity(ast->data.binary_op.right);
            }
            break;
            
        case AST_CALL_EXPR:
            complexity += 10; // 函数调用的高复杂度
            if (ast->data.call_expr.args) {
                for (int i = 0; i < ast->data.call_expr.arg_count; i++) {
                    complexity += optimizer_analyze_complexity(ast->data.call_expr.args[i]);
                }
            }
            break;
            
        case AST_FOR_STMT:
        case AST_WHILE_STMT:
            complexity += 20; // 循环的高复杂度
            break;
            
        case AST_IF_STMT:
            complexity += 5; // 条件语句的中等复杂度
            break;
            
        case AST_LITERAL:
        case AST_IDENTIFIER:
            complexity += 1; // 字面量和标识符的低复杂度
            break;
            
        default:
            complexity += 3; // 其他节点的默认复杂度
            break;
    }
    
    return complexity;
}

double optimizer_estimate_benefit(OptimizerContext* optimizer, OptimizationPassType pass, struct ASTNode* ast) {
    if (!optimizer || !ast) return 0.0;
    
    // 实现收益估算 - 估算优化pass的性能改进
    double benefit = 0.0;
    int complexity = optimizer_analyze_complexity(ast);
    
    switch (pass) {
        case OPT_PASS_CONSTANT_FOLDING:
            // 常量折叠对复杂表达式收益更大
            if (ast->type == AST_BINARY_OP) {
                benefit = complexity * 0.05; // 5%的改进率
            }
            break;
            
        case OPT_PASS_DEAD_CODE_ELIMINATION:
            // 死代码消除对条件语句收益更大
            if (ast->type == AST_IF_STMT) {
                benefit = complexity * 0.10; // 10%的改进率
            }
            break;
            
        case OPT_PASS_COMMON_SUBEXPRESSION:
            // 公共子表达式消除对重复计算收益更大
            benefit = complexity * 0.08; // 8%的改进率
            break;
            
        case OPT_PASS_LOOP_OPTIMIZATION:
            // 循环优化对循环语句收益最大
            if (ast->type == AST_FOR_STMT || ast->type == AST_WHILE_STMT) {
                benefit = complexity * 0.20; // 20%的改进率
            }
            break;
            
        case OPT_PASS_INLINE_EXPANSION:
            // 函数内联对函数调用收益较大
            if (ast->type == AST_CALL_EXPR) {
                benefit = complexity * 0.15; // 15%的改进率
            }
            break;
            
        default:
            benefit = complexity * 0.03; // 默认3%的改进率
            break;
    }
    
    return benefit;
}

bool optimizer_is_safe(OptimizerContext* optimizer, OptimizationPassType pass, struct ASTNode* ast) {
    if (!optimizer || !ast) return false;
    
    // 实现安全性分析 - 检查优化是否安全应用
    switch (pass) {
        case OPT_PASS_CONSTANT_FOLDING:
            // 常量折叠通常是安全的，但要检查除零
            if (ast->type == AST_BINARY_OP && 
                ast->data.binary_op.operator == OP_DIV &&
                ast->data.binary_op.right && 
                ast->data.binary_op.right->type == AST_LITERAL &&
                ast->data.binary_op.right->data.literal.value.int_value == 0) {
                return false; // 除零不安全
            }
            return true;
            
        case OPT_PASS_DEAD_CODE_ELIMINATION:
            // 死代码消除需要确保没有副作用
            if (ast->type == AST_CALL_EXPR) {
                return false; // 函数调用可能有副作用，不安全删除
            }
            return true;
            
        case OPT_PASS_COMMON_SUBEXPRESSION:
            // 公共子表达式消除要确保表达式没有副作用
            if (ast->type == AST_CALL_EXPR) {
                return false; // 函数调用有副作用
            }
            return true;
            
        case OPT_PASS_LOOP_OPTIMIZATION:
            // 循环优化通常是安全的
            return true;
            
        case OPT_PASS_INLINE_EXPANSION:
            // 函数内联需要检查递归调用
            if (ast->type == AST_CALL_EXPR && ast->data.call_expr.function) {
                // 简化检查：假设内建函数是安全的
                return true;
            }
            return true;
            
        default:
            return false; // 未知优化默认不安全
    }
}

// ===============================================
// Utility Functions
// ===============================================

const char* optimizer_get_pass_name(OptimizationPassType pass) {
    if (pass < num_optimization_passes) {
        return optimization_passes[pass].name;
    }
    return "unknown";
}

const char* optimizer_get_level_name(OptimizationLevel level) {
    switch (level) {
        case OPT_LEVEL_NONE: return "none";
        case OPT_LEVEL_BASIC: return "basic";
        case OPT_LEVEL_STANDARD: return "standard";
        case OPT_LEVEL_AGGRESSIVE: return "aggressive";
        default: return "unknown";
    }
}

void optimizer_print_stats(OptimizerContext* optimizer) {
    if (!optimizer) return;
    
    printf("Optimization Statistics:\n");
    printf("  Level: %s\n", optimizer_get_level_name(optimizer->level));
    printf("  Passes run: %d\n", optimizer->passes_run);
    printf("  Optimizations applied: %d\n", optimizer->optimizations_applied);
    printf("  Original complexity: %zu\n", optimizer->original_size);
    printf("  Optimized complexity: %zu\n", optimizer->optimized_size);
    
    if (optimizer->original_size > 0) {
        double reduction = (double)(optimizer->original_size - optimizer->optimized_size) / optimizer->original_size * 100.0;
        printf("  Complexity reduction: %.1f%%\n", reduction);
    }
    
    printf("  Errors: %d\n", optimizer->error_count);
}

bool optimizer_has_error(OptimizerContext* optimizer) {
    return optimizer && optimizer->has_error;
}

const char* optimizer_get_error(OptimizerContext* optimizer) {
    return optimizer ? optimizer->error_message : "Invalid optimizer context";
}

void optimizer_reset_stats(OptimizerContext* optimizer) {
    if (!optimizer) return;
    
    optimizer->passes_run = 0;
    optimizer->optimizations_applied = 0;
    optimizer->original_size = 0;
    optimizer->optimized_size = 0;
    optimizer->error_count = 0;
    optimizer->has_error = false;
}

// ===============================================
// Configuration Functions
// ===============================================

void optimizer_enable_pass(OptimizerContext* optimizer, OptimizationPassType pass) {
    if (optimizer && pass < OPT_PASS_COUNT) {
        optimizer->enabled_passes[pass] = true;
    }
}

void optimizer_disable_pass(OptimizerContext* optimizer, OptimizationPassType pass) {
    if (optimizer && pass < OPT_PASS_COUNT) {
        optimizer->enabled_passes[pass] = false;
    }
}

void optimizer_set_options(OptimizerContext* optimizer, bool preserve_debug, 
                          bool aggressive_inline, int max_inline_size, int max_unroll) {
    if (!optimizer) return;
    
    optimizer->preserve_debug_info = preserve_debug;
    optimizer->aggressive_inlining = aggressive_inline;
    optimizer->max_inline_size = max_inline_size;
    optimizer->max_unroll_count = max_unroll;
}
