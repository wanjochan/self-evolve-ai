#include "compiler_codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 内部函数声明
static void codegen_append(CodeGenerator* cg, const char* str);
static int codegen_generate_statement(CodeGenerator* cg, ASTNode* stmt_node);

// 初始化代码生成器
void codegen_init(CodeGenerator* cg) {
    cg->buffer_size = 16384; // 初始缓冲区大小 16KB
    cg->buffer = (char*)malloc(cg->buffer_size);
    if (!cg->buffer) {
        fprintf(stderr, "Failed to allocate buffer for code generator\n");
        exit(1);
    }
    cg->buffer[0] = '\0';
    cg->buffer_offset = 0;
    cg->label_count = 0;
}

// 释放代码生成器资源
void codegen_free(CodeGenerator* cg) {
    if (cg->buffer) {
        free(cg->buffer);
    }
}

// 生成整个翻译单元的汇编代码
int codegen_generate_translation_unit(CodeGenerator* cg, ASTNode* unit_node) {
    if (!unit_node || unit_node->type != ASTC_TRANSLATION_UNIT) {
        return -1;
    }

    for (int i = 0; i < unit_node->data.translation_unit.declaration_count; i++) {
        ASTNode* decl = unit_node->data.translation_unit.declarations[i];
        if (decl->type == ASTC_FUNC_DECL) {
            if (codegen_generate_function(cg, decl) != 0) {
                return -1;
            }
        }
    }

    return 0;
}

// 生成单个函数的汇编代码
int codegen_generate_function(CodeGenerator* cg, ASTNode* func_node) {
    if (!func_node || func_node->type != ASTC_FUNC_DECL) {
        return -1;
    }

    char temp_buffer[256];

    // 函数标签
    sprintf(temp_buffer, "\n%s:\n", func_node->data.func_decl.name);
    codegen_append(cg, temp_buffer);

    // 函数序言
    codegen_append(cg, "    push rbp\n");
    codegen_append(cg, "    mov rbp, rsp\n");

    // 为局部变量分配栈空间 (TODO: 计算实际需要的空间)
    codegen_append(cg, "    sub rsp, 16\n"); 

    // 生成函数体代码
    if (func_node->data.func_decl.has_body) {
        if (codegen_generate_statement(cg, func_node->data.func_decl.body) != 0) {
            return -1;
        }
    }

    // 函数尾声
    codegen_append(cg, "    mov rsp, rbp\n");
    codegen_append(cg, "    pop rbp\n");
    codegen_append(cg, "    ret\n");

    return 0;
}

// 生成语句的汇编代码
static int codegen_generate_statement(CodeGenerator* cg, ASTNode* stmt_node) {
    if (!stmt_node) return -1;

    switch (stmt_node->type) {
        case ASTC_COMPOUND_STMT:
            for (int i = 0; i < stmt_node->data.compound_stmt.statement_count; i++) {
                if (codegen_generate_statement(cg, stmt_node->data.compound_stmt.statements[i]) != 0) {
                    return -1;
                }
            }
            break;
        
        // TODO: 实现其他语句类型的代码生成

        default:
            // 其他语句暂不处理
            break;
    }
    return 0;
}

// 向缓冲区追加字符串
static void codegen_append(CodeGenerator* cg, const char* str) {
    size_t str_len = strlen(str);
    if (cg->buffer_offset + str_len + 1 > cg->buffer_size) {
        cg->buffer_size = (cg->buffer_offset + str_len + 1) * 2;
        cg->buffer = (char*)realloc(cg->buffer, cg->buffer_size);
        if (!cg->buffer) {
            fprintf(stderr, "Failed to reallocate buffer for code generator\n");
            exit(1);
        }
    }
    strcat(cg->buffer, str);
    cg->buffer_offset += str_len;
}

// 公开版本的codegen_append
void codegen_append_public(CodeGenerator* cg, const char* str) {
    codegen_append(cg, str);
}
