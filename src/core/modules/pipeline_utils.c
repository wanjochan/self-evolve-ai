/**
 * pipeline_utils.c - Pipeline Utility Functions
 * 
 * 提供pipeline子模块共享的工具函数实现
 */

#include "pipeline_common.h"

// ===============================================
// Token管理实现
// ===============================================

Token* create_token(TokenType type, const char* value, int line, int column) {
    Token* token = malloc(sizeof(Token));
    if (!token) return NULL;
    
    token->type = type;
    token->value = value ? strdup(value) : NULL;
    token->line = line;
    token->column = column;
    
    return token;
}

void free_token(Token* token) {
    if (!token) return;
    if (token->value) free(token->value);
    free(token);
}

void free_token_array(Token** tokens, int count) {
    if (!tokens) return;
    for (int i = 0; i < count; i++) {
        free_token(tokens[i]);
    }
    free(tokens);
}

// ===============================================
// 代码生成器工具实现
// ===============================================

void init_codegen(CodeGenerator* cg) {
    if (!cg) return;
    cg->buffer_size = 4096;
    cg->buffer = malloc(cg->buffer_size);
    if (cg->buffer) {
        cg->buffer[0] = '\0';
    }
    cg->buffer_offset = 0;
    cg->label_count = 0;
}

void free_codegen(CodeGenerator* cg) {
    if (cg && cg->buffer) {
        free(cg->buffer);
        cg->buffer = NULL;
    }
}

void codegen_append(CodeGenerator* cg, const char* code) {
    if (!cg || !code || !cg->buffer) return;
    
    size_t len = strlen(code);
    if (cg->buffer_offset + len >= cg->buffer_size) {
        cg->buffer_size *= 2;
        cg->buffer = realloc(cg->buffer, cg->buffer_size);
        if (!cg->buffer) return;
    }
    
    strcpy(cg->buffer + cg->buffer_offset, code);
    cg->buffer_offset += len;
}

void codegen_append_format(CodeGenerator* cg, const char* format, ...) {
    if (!cg || !format) return;
    
    char temp_buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(temp_buffer, sizeof(temp_buffer), format, args);
    va_end(args);
    
    codegen_append(cg, temp_buffer);
}

// ===============================================
// 错误处理工具实现
// ===============================================

void set_pipeline_error(char* error_buffer, size_t buffer_size, const char* format, ...) {
    if (!error_buffer || !format) return;
    
    va_list args;
    va_start(args, format);
    vsnprintf(error_buffer, buffer_size, format, args);
    va_end(args);
}

// ===============================================
// AST工具函数实现
// ===============================================

bool is_constant_expression(const ASTNode* expr) {
    if (!expr) return false;

    switch (expr->type) {
        case ASTC_EXPR_CONSTANT:
            return true;
        case ASTC_BINARY_OP:
            return is_constant_expression(expr->data.binary_op.left) &&
                   is_constant_expression(expr->data.binary_op.right);
        case ASTC_UNARY_OP:
            return is_constant_expression(expr->data.unary_op.operand);
        default:
            return false;
    }
}

bool has_side_effects(const ASTNode* node) {
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
                    return has_side_effects(node->data.binary_op.left) ||
                           has_side_effects(node->data.binary_op.right);
            }
        case ASTC_UNARY_OP:
            return has_side_effects(node->data.unary_op.operand);
        default:
            return true; // 保守处理：假设有副作用
    }
}

ASTNode* copy_ast_node(const ASTNode* node) {
    if (!node) return NULL;
    
    // 简化实现：只复制基本信息
    ASTNode* copy = ast_create_node(node->type, node->line, node->column);
    if (!copy) return NULL;
    
    // 根据节点类型复制数据
    switch (node->type) {
        case ASTC_EXPR_CONSTANT:
            copy->data.constant.type = node->data.constant.type;
            copy->data.constant.int_val = node->data.constant.int_val;
            break;
        case ASTC_EXPR_IDENTIFIER:
            if (node->data.identifier.name) {
                copy->data.identifier.name = strdup(node->data.identifier.name);
            }
            break;
        // 其他类型可以根据需要添加
        default:
            break;
    }
    
    return copy;
}