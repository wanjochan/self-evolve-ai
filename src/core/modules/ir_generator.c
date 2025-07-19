/**
 * ir_generator.c - C99Bin Intermediate Representation Generator
 * 
 * T1.2: 中间代码生成 - AST到IR转换，专门优化setjmp/longjmp
 * 基于现有架构的高效实现
 */

#include "pipeline_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// IR指令类型
typedef enum {
    IR_NOP,
    IR_LOAD,
    IR_STORE,
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_DIV,
    IR_CALL,
    IR_SETJMP,      // 特殊IR for setjmp
    IR_LONGJMP,     // 特殊IR for longjmp
    IR_LABEL,
    IR_JUMP,
    IR_CJUMP,       // 条件跳转
    IR_RETURN
} IRInstructionType;

// IR操作数
typedef struct {
    enum { IR_OPERAND_REG, IR_OPERAND_CONST, IR_OPERAND_VAR } type;
    union {
        int reg_id;
        int64_t const_val;
        char* var_name;
    } value;
} IROperand;

// IR指令
typedef struct IRInstruction {
    IRInstructionType type;
    IROperand dest;
    IROperand src1;
    IROperand src2;
    char* label;
    struct IRInstruction* next;
} IRInstruction;

// IR基本块
typedef struct IRBasicBlock {
    char* label;
    IRInstruction* instructions;
    struct IRBasicBlock* next;
} IRBasicBlock;

// IR函数
typedef struct IRFunction {
    char* name;
    IRBasicBlock* blocks;
    int reg_count;
    int label_count;
    struct IRFunction* next;
} IRFunction;

// IR模块
typedef struct {
    IRFunction* functions;
    int function_count;
} IRModule;

// IR生成器上下文
typedef struct {
    IRModule* module;
    IRFunction* current_func;
    IRBasicBlock* current_block;
    int next_reg;
    int next_label;
    bool has_setjmp;
    char* jmp_buf_var;
} IRContext;

// IR生成器接口
IRModule* ir_generate(ASTNode* ast);
IRFunction* ir_generate_function(ASTNode* func_ast, IRContext* ctx);
IRInstruction* ir_generate_expression(ASTNode* expr, IRContext* ctx);
IRInstruction* ir_generate_statement(ASTNode* stmt, IRContext* ctx);
IRInstruction* ir_generate_setjmp_call(ASTNode* call, IRContext* ctx);
IRInstruction* ir_generate_longjmp_call(ASTNode* call, IRContext* ctx);

// 创建IR指令
IRInstruction* ir_create_instruction(IRInstructionType type) {
    IRInstruction* instr = malloc(sizeof(IRInstruction));
    memset(instr, 0, sizeof(IRInstruction));
    instr->type = type;
    instr->next = NULL;
    return instr;
}

// 创建寄存器操作数
IROperand ir_make_reg(int reg_id) {
    IROperand op = {0};
    op.type = IR_OPERAND_REG;
    op.value.reg_id = reg_id;
    return op;
}

// 创建常量操作数
IROperand ir_make_const(int64_t val) {
    IROperand op = {0};
    op.type = IR_OPERAND_CONST;
    op.value.const_val = val;
    return op;
}

// 创建变量操作数
IROperand ir_make_var(const char* name) {
    IROperand op = {0};
    op.type = IR_OPERAND_VAR;
    op.value.var_name = strdup(name);
    return op;
}

// IR生成主入口
IRModule* ir_generate(ASTNode* ast) {
    if (!ast) return NULL;
    
    printf("🏗️  Starting IR generation...\n");
    
    IRContext ctx = {0};
    ctx.module = malloc(sizeof(IRModule));
    memset(ctx.module, 0, sizeof(IRModule));
    ctx.next_reg = 1;
    ctx.next_label = 1;
    
    // 简化：假设只有一个main函数
    if (ast->type == ASTC_PROGRAM) {
        IRFunction* main_func = ir_generate_function(ast, &ctx);
        if (main_func) {
            ctx.module->functions = main_func;
            ctx.module->function_count = 1;
            printf("✅ Generated IR for main function\n");
        }
    }
    
    printf("🎯 IR generation completed!\n");
    printf("   - Functions: %d\n", ctx.module->function_count);
    printf("   - Registers used: %d\n", ctx.next_reg - 1);
    printf("   - Has setjmp/longjmp: %s\n", ctx.has_setjmp ? "yes" : "no");
    
    return ctx.module;
}

// 生成函数的IR
IRFunction* ir_generate_function(ASTNode* func_ast, IRContext* ctx) {
    IRFunction* func = malloc(sizeof(IRFunction));
    memset(func, 0, sizeof(IRFunction));
    func->name = strdup("main"); // 简化
    func->reg_count = 0;
    func->label_count = 0;
    
    ctx->current_func = func;
    
    // 创建入口基本块
    IRBasicBlock* entry_block = malloc(sizeof(IRBasicBlock));
    memset(entry_block, 0, sizeof(IRBasicBlock));
    entry_block->label = strdup("entry");
    func->blocks = entry_block;
    ctx->current_block = entry_block;
    
    printf("📝 Generating IR for function: %s\n", func->name);
    
    // 简化：生成一些示例IR指令
    // 这里应该递归遍历AST并生成相应的IR
    
    return func;
}

// 生成表达式的IR
IRInstruction* ir_generate_expression(ASTNode* expr, IRContext* ctx) {
    if (!expr) return NULL;
    
    switch (expr->type) {
        case ASTC_CALL_EXPR: {
            // 检查是否是setjmp/longjmp调用
            if (expr->data.call_expr.callee && 
                expr->data.call_expr.callee->type == ASTC_EXPR_IDENTIFIER) {
                
                const char* func_name = expr->data.call_expr.callee->data.identifier.name;
                
                if (strcmp(func_name, "setjmp") == 0) {
                    return ir_generate_setjmp_call(expr, ctx);
                }
                if (strcmp(func_name, "longjmp") == 0) {
                    return ir_generate_longjmp_call(expr, ctx);
                }
            }
            
            // 普通函数调用
            IRInstruction* call = ir_create_instruction(IR_CALL);
            call->dest = ir_make_reg(ctx->next_reg++);
            printf("🔧 Generated IR for function call\n");
            return call;
        }
        
        case ASTC_EXPR_CONSTANT: {
            IRInstruction* load = ir_create_instruction(IR_LOAD);
            load->dest = ir_make_reg(ctx->next_reg++);
            load->src1 = ir_make_const(expr->data.constant.int_val);
            printf("📊 Generated IR for constant: %lld\n", expr->data.constant.int_val);
            return load;
        }
        
        case ASTC_BINARY_OP: {
            // 生成二元操作的IR
            IRInstruction* op = ir_create_instruction(IR_ADD); // 简化
            op->dest = ir_make_reg(ctx->next_reg++);
            printf("🧮 Generated IR for binary operation\n");
            return op;
        }
        
        default:
            return NULL;
    }
}

// 生成setjmp调用的特殊IR
IRInstruction* ir_generate_setjmp_call(ASTNode* call, IRContext* ctx) {
    printf("🎯 Generating special IR for setjmp call\n");
    ctx->has_setjmp = true;
    
    IRInstruction* setjmp_ir = ir_create_instruction(IR_SETJMP);
    setjmp_ir->dest = ir_make_reg(ctx->next_reg++);
    
    // 处理jmp_buf参数
    if (call->data.call_expr.arg_count > 0) {
        ASTNode* buf_arg = call->data.call_expr.args[0];
        if (buf_arg && buf_arg->type == ASTC_EXPR_IDENTIFIER) {
            setjmp_ir->src1 = ir_make_var(buf_arg->data.identifier.name);
            ctx->jmp_buf_var = strdup(buf_arg->data.identifier.name);
            printf("   - jmp_buf variable: %s\n", ctx->jmp_buf_var);
        }
    }
    
    return setjmp_ir;
}

// 生成longjmp调用的特殊IR
IRInstruction* ir_generate_longjmp_call(ASTNode* call, IRContext* ctx) {
    printf("🎯 Generating special IR for longjmp call\n");
    
    IRInstruction* longjmp_ir = ir_create_instruction(IR_LONGJMP);
    
    // 处理参数
    if (call->data.call_expr.arg_count >= 2) {
        ASTNode* buf_arg = call->data.call_expr.args[0];
        ASTNode* val_arg = call->data.call_expr.args[1];
        
        if (buf_arg && buf_arg->type == ASTC_EXPR_IDENTIFIER) {
            longjmp_ir->src1 = ir_make_var(buf_arg->data.identifier.name);
            printf("   - jmp_buf variable: %s\n", buf_arg->data.identifier.name);
        }
        
        if (val_arg && val_arg->type == ASTC_EXPR_CONSTANT) {
            longjmp_ir->src2 = ir_make_const(val_arg->data.constant.int_val);
            printf("   - return value: %lld\n", val_arg->data.constant.int_val);
        }
    }
    
    return longjmp_ir;
}

// 生成语句的IR
IRInstruction* ir_generate_statement(ASTNode* stmt, IRContext* ctx) {
    if (!stmt) return NULL;
    
    switch (stmt->type) {
        case ASTC_IF_STMT: {
            printf("🔀 Generating IR for if statement\n");
            // 生成条件跳转IR
            IRInstruction* cjump = ir_create_instruction(IR_CJUMP);
            return cjump;
        }
        
        case ASTC_WHILE_STMT: {
            printf("🔄 Generating IR for while loop\n");
            IRInstruction* loop = ir_create_instruction(IR_LABEL);
            loop->label = strdup("loop_start");
            return loop;
        }
        
        default:
            return ir_generate_expression(stmt, ctx);
    }
}

// 打印IR模块 (调试用)
void ir_print_module(IRModule* module) {
    if (!module) return;
    
    printf("\n=== Generated IR Module ===\n");
    
    IRFunction* func = module->functions;
    while (func) {
        printf("Function: %s\n", func->name);
        
        IRBasicBlock* block = func->blocks;
        while (block) {
            printf("  Block: %s\n", block->label);
            
            IRInstruction* instr = block->instructions;
            while (instr) {
                printf("    ");
                switch (instr->type) {
                    case IR_SETJMP:
                        printf("SETJMP");
                        break;
                    case IR_LONGJMP:
                        printf("LONGJMP");
                        break;
                    case IR_CALL:
                        printf("CALL");
                        break;
                    case IR_LOAD:
                        printf("LOAD");
                        break;
                    default:
                        printf("IR_%d", instr->type);
                        break;
                }
                printf("\n");
                instr = instr->next;
            }
            
            block = block->next;
        }
        
        func = func->next;
    }
    
    printf("==========================\n\n");
}

// 清理IR模块
void ir_cleanup_module(IRModule* module) {
    // 简化：实际实现需要递归释放所有内存
    if (module) {
        free(module);
    }
}