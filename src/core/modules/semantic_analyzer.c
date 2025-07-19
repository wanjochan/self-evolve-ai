/**
 * semantic_analyzer.c - C99Bin Semantic Analyzer Module
 * 
 * T1.1.3: 语义分析器开发 - 专注于setjmp/longjmp语义检查
 * 基于现有架构的高效实现
 */

#include "pipeline_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// 语义分析错误类型
typedef enum {
    SEMANTIC_ERROR_TYPE_MISMATCH,
    SEMANTIC_ERROR_UNDEFINED_VAR,
    SEMANTIC_ERROR_SETJMP_USAGE,
    SEMANTIC_ERROR_LONGJMP_USAGE,
    SEMANTIC_ERROR_JMP_BUF_DECL,
    SEMANTIC_ERROR_FUNCTION_CALL,
    SEMANTIC_ERROR_INVALID_ASSIGNMENT
} SemanticErrorType;

// 语义分析器状态
typedef struct {
    ASTNode* current_function;
    bool has_setjmp;
    bool has_longjmp;
    char* jmp_buf_var;
    int error_count;
    char error_messages[100][256];
} SemanticContext;

// 符号表条目
typedef struct SymbolEntry {
    char* name;
    ASTNodeType type;
    bool is_jmp_buf;
    struct SymbolEntry* next;
} SymbolEntry;

// 符号表
typedef struct {
    SymbolEntry* symbols;
} SymbolTable;

// 语义分析器接口
bool semantic_analyze(ASTNode* ast, SemanticContext* context);
bool validate_setjmp_longjmp_usage(ASTNode* ast, SemanticContext* context);
bool check_jmp_buf_variables(ASTNode* ast, SymbolTable* table);
bool validate_function_calls(ASTNode* ast, SemanticContext* context);

// 实现语义分析主入口
bool semantic_analyze(ASTNode* ast, SemanticContext* context) {
    if (!ast || !context) return false;
    
    // 初始化语义分析上下文
    context->current_function = NULL;
    context->has_setjmp = false;
    context->has_longjmp = false;
    context->jmp_buf_var = NULL;
    context->error_count = 0;
    
    printf("🔍 Starting semantic analysis...\n");
    
    // 创建符号表
    SymbolTable symbol_table = {0};
    
    // 步骤1: 检查jmp_buf变量声明
    if (!check_jmp_buf_variables(ast, &symbol_table)) {
        snprintf(context->error_messages[context->error_count++], 256,
                "Error: Invalid jmp_buf variable declarations");
        return false;
    }
    
    // 步骤2: 验证setjmp/longjmp使用模式
    if (!validate_setjmp_longjmp_usage(ast, context)) {
        snprintf(context->error_messages[context->error_count++], 256,
                "Error: Invalid setjmp/longjmp usage pattern");
        return false;
    }
    
    // 步骤3: 验证函数调用语义
    if (!validate_function_calls(ast, context)) {
        snprintf(context->error_messages[context->error_count++], 256,
                "Error: Invalid function call semantics");
        return false;
    }
    
    printf("✅ Semantic analysis completed successfully\n");
    printf("   - setjmp usage: %s\n", context->has_setjmp ? "found" : "none");
    printf("   - longjmp usage: %s\n", context->has_longjmp ? "found" : "none");
    printf("   - jmp_buf variable: %s\n", context->jmp_buf_var ? context->jmp_buf_var : "none");
    
    return true;
}

// 检查jmp_buf变量声明的语义正确性
bool check_jmp_buf_variables(ASTNode* ast, SymbolTable* table) {
    if (!ast) return true;
    
    // 检查变量声明
    if (ast->type == ASTC_VAR_DECL) {
        // 检查是否是jmp_buf类型
        if (ast->data.var_decl.type && 
            ast->data.var_decl.type->type == ASTC_TYPE_INT) { // 暂时映射的jmp_buf
            
            // 添加到符号表
            SymbolEntry* entry = malloc(sizeof(SymbolEntry));
            entry->name = strdup(ast->data.var_decl.name);
            entry->type = ASTC_TYPE_INT;
            entry->is_jmp_buf = true; // 标记为jmp_buf
            entry->next = table->symbols;
            table->symbols = entry;
            
            printf("📝 Found jmp_buf variable: %s\n", entry->name);
        }
    }
    
    // 递归检查子节点
    // 简化实现：这里应该遍历所有子节点
    return true;
}

// 验证setjmp/longjmp使用模式的语义正确性
bool validate_setjmp_longjmp_usage(ASTNode* ast, SemanticContext* context) {
    if (!ast) return true;
    
    // 检查函数调用
    if (ast->type == ASTC_CALL_EXPR) {
        if (ast->data.call_expr.callee && 
            ast->data.call_expr.callee->type == ASTC_EXPR_IDENTIFIER) {
            
            const char* func_name = ast->data.call_expr.callee->data.identifier.name;
            
            if (strcmp(func_name, "setjmp") == 0) {
                context->has_setjmp = true;
                printf("🎯 Found setjmp call\n");
                
                // 验证setjmp参数
                if (ast->data.call_expr.arg_count != 1) {
                    snprintf(context->error_messages[context->error_count++], 256,
                            "Error: setjmp requires exactly one argument");
                    return false;
                }
                
                // 检查参数是否是jmp_buf变量
                ASTNode* arg = ast->data.call_expr.args[0];
                if (arg && arg->type == ASTC_EXPR_IDENTIFIER) {
                    context->jmp_buf_var = strdup(arg->data.identifier.name);
                    printf("   - Using jmp_buf variable: %s\n", context->jmp_buf_var);
                }
            }
            
            if (strcmp(func_name, "longjmp") == 0) {
                context->has_longjmp = true;
                printf("🎯 Found longjmp call\n");
                
                // 验证longjmp参数
                if (ast->data.call_expr.arg_count != 2) {
                    snprintf(context->error_messages[context->error_count++], 256,
                            "Error: longjmp requires exactly two arguments");
                    return false;
                }
                
                // 检查第一个参数是否与setjmp使用同一个jmp_buf
                ASTNode* buf_arg = ast->data.call_expr.args[0];
                if (buf_arg && buf_arg->type == ASTC_EXPR_IDENTIFIER) {
                    if (context->jmp_buf_var && 
                        strcmp(buf_arg->data.identifier.name, context->jmp_buf_var) != 0) {
                        snprintf(context->error_messages[context->error_count++], 256,
                                "Warning: longjmp uses different jmp_buf than setjmp");
                    }
                }
            }
        }
    }
    
    // 递归检查子节点 (简化)
    return true;
}

// 验证函数调用的语义正确性
bool validate_function_calls(ASTNode* ast, SemanticContext* context) {
    if (!ast) return true;
    
    if (ast->type == ASTC_CALL_EXPR) {
        // 验证函数调用的基本语义
        if (!ast->data.call_expr.callee) {
            snprintf(context->error_messages[context->error_count++], 256,
                    "Error: Function call missing callee");
            return false;
        }
        
        // 特殊处理标记的libc调用
        if (ast->data.call_expr.is_libc_call) {
            printf("🔧 Validating special libc call\n");
            // 这里可以添加更多特殊验证逻辑
        }
    }
    
    return true;
}

// 语义分析报告生成
void semantic_generate_report(SemanticContext* context) {
    printf("\n=== Semantic Analysis Report ===\n");
    printf("Errors found: %d\n", context->error_count);
    
    for (int i = 0; i < context->error_count; i++) {
        printf("❌ %s\n", context->error_messages[i]);
    }
    
    if (context->error_count == 0) {
        printf("✅ No semantic errors found!\n");
    }
    
    printf("================================\n\n");
}

// 清理语义分析器资源
void semantic_cleanup(SemanticContext* context) {
    if (context->jmp_buf_var) {
        free(context->jmp_buf_var);
        context->jmp_buf_var = NULL;
    }
}