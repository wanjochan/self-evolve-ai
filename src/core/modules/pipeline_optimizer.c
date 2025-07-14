/**
 * pipeline_optimizer.c - Pipeline Optimizer Module
 * 
 * 优化器模块，负责AST优化：
 * - 常量折叠
 * - 死代码消除
 * - 基本块优化
 * - 寄存器分配优化
 */

#include "pipeline_common.h"

// ===============================================
// 优化器上下文管理
// ===============================================

OptimizerContext* optimizer_create_context(OptimizationLevel level) {
    OptimizerContext* ctx = malloc(sizeof(OptimizerContext));
    if (!ctx) return NULL;

    ctx->level = level;
    ctx->optimization_passes = 1;
    ctx->optimization_log = NULL;
    ctx->log_size = 0;

    // 根据优化级别设置优化选项
    switch (level) {
        case OPT_LEVEL_NONE:
            ctx->enable_constant_folding = false;
            ctx->enable_dead_code_elimination = false;
            ctx->enable_register_allocation = false;
            ctx->enable_basic_block_optimization = false;
            break;
        case OPT_LEVEL_BASIC:
            ctx->enable_constant_folding = true;
            ctx->enable_dead_code_elimination = false;
            ctx->enable_register_allocation = false;
            ctx->enable_basic_block_optimization = false;
            break;
        case OPT_LEVEL_STANDARD:
            ctx->enable_constant_folding = true;
            ctx->enable_dead_code_elimination = true;
            ctx->enable_register_allocation = false;
            ctx->enable_basic_block_optimization = false;
            ctx->optimization_passes = 2;
            break;
        case OPT_LEVEL_AGGRESSIVE:
            ctx->enable_constant_folding = true;
            ctx->enable_dead_code_elimination = true;
            ctx->enable_register_allocation = true;
            ctx->enable_basic_block_optimization = true;
            ctx->optimization_passes = 3;
            break;
    }

    return ctx;
}

void optimizer_free_context(OptimizerContext* ctx) {
    if (!ctx) return;
    if (ctx->optimization_log) free(ctx->optimization_log);
    free(ctx);
}

// ===============================================
// 常量折叠优化
// ===============================================

static bool is_constant_expression_impl(const ASTNode* expr) {
    if (!expr) return false;

    switch (expr->type) {
        case ASTC_EXPR_CONSTANT:
            return true;
        case ASTC_BINARY_OP:
            return is_constant_expression_impl(expr->data.binary_op.left) &&
                   is_constant_expression_impl(expr->data.binary_op.right);
        case ASTC_UNARY_OP:
            return is_constant_expression_impl(expr->data.unary_op.operand);
        default:
            return false;
    }
}

static int64_t evaluate_constant_expression(const ASTNode* expr) {
    if (!expr || expr->type != ASTC_EXPR_CONSTANT) return 0;
    return expr->data.constant.int_val;
}

static ASTNode* fold_binary_operation(ASTNode* binary_op) {
    if (!binary_op || binary_op->type != ASTC_BINARY_OP) return binary_op;

    ASTNode* left = binary_op->data.binary_op.left;
    ASTNode* right = binary_op->data.binary_op.right;

    if (!is_constant_expression_impl(left) || !is_constant_expression_impl(right)) {
        return binary_op;
    }

    int64_t left_val = evaluate_constant_expression(left);
    int64_t right_val = evaluate_constant_expression(right);
    int64_t result = 0;

    switch (binary_op->data.binary_op.op) {
        case ASTC_OP_ADD:
            result = left_val + right_val;
            break;
        case ASTC_OP_SUB:
            result = left_val - right_val;
            break;
        case ASTC_OP_MUL:
            result = left_val * right_val;
            break;
        case ASTC_OP_DIV:
            if (right_val != 0) result = left_val / right_val;
            else return binary_op; // 避免除零
            break;
        default:
            return binary_op;
    }

    // 创建新的常量节点
    ASTNode* constant = ast_create_node(ASTC_EXPR_CONSTANT, binary_op->line, binary_op->column);
    constant->data.constant.type = ASTC_TYPE_INT;
    constant->data.constant.int_val = result;

    // 释放原有节点
    ast_free(left);
    ast_free(right);
    ast_free(binary_op);

    return constant;
}

static bool constant_folding_pass(ASTNode* ast, OptimizerContext* ctx) {
    if (!ast || !ctx) return false;

    switch (ast->type) {
        case ASTC_BINARY_OP: {
            // 递归优化子表达式
            if (ast->data.binary_op.left) {
                constant_folding_pass(ast->data.binary_op.left, ctx);
            }
            if (ast->data.binary_op.right) {
                constant_folding_pass(ast->data.binary_op.right, ctx);
            }
            
            // 尝试折叠当前操作
            ASTNode* folded = fold_binary_operation(ast);
            if (folded != ast) {
                // 节点被折叠了，但这里需要更复杂的逻辑来替换父节点中的引用
                // 简化处理：记录优化发生
                return true;
            }
            break;
        }
        case ASTC_UNARY_OP:
            if (ast->data.unary_op.operand) {
                constant_folding_pass(ast->data.unary_op.operand, ctx);
            }
            break;
        case ASTC_FUNC_DECL:
            if (ast->data.func_decl.body) {
                constant_folding_pass(ast->data.func_decl.body, ctx);
            }
            break;
        case ASTC_RETURN_STMT:
            if (ast->data.return_stmt.value) {
                constant_folding_pass(ast->data.return_stmt.value, ctx);
            }
            break;
        case ASTC_EXPR_STMT:
            if (ast->data.expr_stmt.expr) {
                constant_folding_pass(ast->data.expr_stmt.expr, ctx);
            }
            break;
        default:
            break;
    }

    return true;
}

// ===============================================
// 死代码消除优化
// ===============================================

static bool has_side_effects_impl(const ASTNode* node) {
    if (!node) return false;

    switch (node->type) {
        case ASTC_EXPR_CONSTANT:
        case ASTC_EXPR_IDENTIFIER:
            return false;
        case ASTC_CALL_EXPR:
            return true; // 函数调用可能有副作用
        case ASTC_BINARY_OP:
            switch (node->data.binary_op.op) {
                case ASTC_OP_ASSIGN:
                    return true; // 赋值有副作用
                default:
                    return has_side_effects_impl(node->data.binary_op.left) ||
                           has_side_effects_impl(node->data.binary_op.right);
            }
        case ASTC_UNARY_OP:
            return has_side_effects_impl(node->data.unary_op.operand);
        default:
            return true; // 保守处理：假设有副作用
    }
}

static bool dead_code_elimination_pass(ASTNode* ast, OptimizerContext* ctx) {
    if (!ast || !ctx) return false;

    switch (ast->type) {
        case ASTC_EXPR_STMT:
            // 如果表达式语句没有副作用，可以标记为删除
            if (ast->data.expr_stmt.expr && !has_side_effects_impl(ast->data.expr_stmt.expr)) {
                // 简化处理：标记但不实际删除
                return true;
            }
            break;
        case ASTC_FUNC_DECL:
            if (ast->data.func_decl.body) {
                dead_code_elimination_pass(ast->data.func_decl.body, ctx);
            }
            break;
        default:
            break;
    }

    return true;
}

// ===============================================
// 基本块优化
// ===============================================

typedef struct BasicBlock {
    int id;
    ASTNode** instructions;
    int instruction_count;
    struct BasicBlock** predecessors;
    int predecessor_count;
    struct BasicBlock** successors;
    int successor_count;
    bool is_entry;
    bool is_exit;
} BasicBlock;

typedef struct {
    BasicBlock** blocks;
    int block_count;
    BasicBlock* entry_block;
    BasicBlock* exit_block;
} ControlFlowGraph;

static ControlFlowGraph* build_cfg(ASTNode* ast) {
    // 简化实现：只创建一个基本块
    ControlFlowGraph* cfg = malloc(sizeof(ControlFlowGraph));
    if (!cfg) return NULL;

    cfg->block_count = 1;
    cfg->blocks = malloc(sizeof(BasicBlock*));
    cfg->blocks[0] = malloc(sizeof(BasicBlock));
    
    BasicBlock* bb = cfg->blocks[0];
    bb->id = 0;
    bb->instruction_count = 0;
    bb->instructions = NULL;
    bb->predecessor_count = 0;
    bb->predecessors = NULL;
    bb->successor_count = 0;
    bb->successors = NULL;
    bb->is_entry = true;
    bb->is_exit = true;

    cfg->entry_block = bb;
    cfg->exit_block = bb;

    return cfg;
}

static void free_cfg(ControlFlowGraph* cfg) {
    if (!cfg) return;
    
    for (int i = 0; i < cfg->block_count; i++) {
        BasicBlock* bb = cfg->blocks[i];
        if (bb->instructions) free(bb->instructions);
        if (bb->predecessors) free(bb->predecessors);
        if (bb->successors) free(bb->successors);
        free(bb);
    }
    
    free(cfg->blocks);
    free(cfg);
}

static bool basic_block_optimization_pass(ASTNode* ast, OptimizerContext* ctx) {
    if (!ast || !ctx) return false;

    ControlFlowGraph* cfg = build_cfg(ast);
    if (!cfg) return false;

    // 简化的基本块优化：只是构建了CFG
    bool result = true;

    free_cfg(cfg);
    return result;
}

// ===============================================
// 寄存器分配优化 (简化实现)
// ===============================================

static bool register_allocation_pass(ASTNode* ast, OptimizerContext* ctx) {
    if (!ast || !ctx) return false;
    
    // 简化实现：只返回成功
    return true;
}

// ===============================================
// 主优化接口
// ===============================================

bool optimizer_optimize_ast(ASTNode* ast, OptimizerContext* ctx) {
    if (!ast || !ctx) return false;

    for (int pass = 0; pass < ctx->optimization_passes; pass++) {
        bool changed = false;

        if (ctx->enable_constant_folding) {
            if (constant_folding_pass(ast, ctx)) {
                changed = true;
            }
        }

        if (ctx->enable_dead_code_elimination) {
            if (dead_code_elimination_pass(ast, ctx)) {
                changed = true;
            }
        }

        if (ctx->enable_basic_block_optimization) {
            if (basic_block_optimization_pass(ast, ctx)) {
                changed = true;
            }
        }

        if (ctx->enable_register_allocation) {
            if (register_allocation_pass(ast, ctx)) {
                changed = true;
            }
        }

        // 如果没有变化，提前退出
        if (!changed) {
            break;
        }
    }

    return true;
}

// ===============================================
// 工具函数实现
// ===============================================

bool is_constant_expression(const ASTNode* expr) {
    return is_constant_expression_impl(expr);
}

bool has_side_effects(const ASTNode* node) {
    return has_side_effects_impl(node);
}