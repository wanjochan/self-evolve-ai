#include "x64_codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 简单的汇编代码生成器
char* generate_function_asm(ASTNode* func_node) {
    if (!func_node || func_node->type != ASTC_FUNC_DECL) {
        return NULL;
    }

    // 为汇编代码分配缓冲区
    char* asm_code = (char*)malloc(4096);
    if (!asm_code) return NULL;
    asm_code[0] = '\0';

    // 函数序言
    strcat(asm_code, "push rbp\n");
    strcat(asm_code, "mov rbp, rsp\n");

    // 假设函数体是一个简单的 return 语句
    if (func_node->data.func_decl.has_body && 
        func_node->data.func_decl.body->type == ASTC_COMPOUND_STMT &&
        func_node->data.func_decl.body->data.compound_stmt.statement_count > 0) {

        ASTNode* stmt = func_node->data.func_decl.body->data.compound_stmt.statements[0];
        if (stmt->type == ASTC_RETURN_STMT && stmt->data.return_stmt.value->type == ASTC_EXPR_CONSTANT) {
            int ret_val = stmt->data.return_stmt.value->data.constant.int_val;
            char buffer[64];
            sprintf(buffer, "mov eax, %d\n", ret_val);
            strcat(asm_code, buffer);
        }
    }

    // 函数尾声
    strcat(asm_code, "pop rbp\n");
    strcat(asm_code, "ret\n");

    return asm_code;
}

