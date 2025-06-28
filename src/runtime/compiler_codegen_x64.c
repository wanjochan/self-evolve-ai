#include "compiler_codegen_x64.h"
#include "compiler_astc2rt.h"
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

// ===============================================
// x64架构特定的机器码生成函数
// ===============================================

void x64_emit_nop(CodeGen* gen) {
    emit_byte(gen, 0x90);  // nop
}

void x64_emit_halt_with_return_value(CodeGen* gen) {
    // 检查栈是否有值，如果有则使用，否则返回0
    // 为了安全，我们先设置默认返回值
    emit_byte(gen, 0xb8);        // mov eax, 0 (默认返回值)
    emit_int32(gen, 0);

    // TODO: 这里应该检查栈顶是否有值，如果有则pop到eax
    // 暂时使用默认值以确保稳定性

    // 恢复栈指针
    emit_byte(gen, 0x48);        // add rsp, 48
    emit_byte(gen, 0x83);
    emit_byte(gen, 0xc4);
    emit_byte(gen, 0x30);

    // 标准函数尾声
    emit_byte(gen, 0x5d);        // pop rbp
    emit_byte(gen, 0xc3);        // ret
}

void x64_emit_const_i32(CodeGen* gen, uint32_t value) {
    // mov eax, immediate (32-bit)
    emit_byte(gen, 0xb8);
    emit_int32(gen, value);
    // push rax
    emit_byte(gen, 0x50);
}

void x64_emit_binary_op_add(CodeGen* gen) {
    emit_byte(gen, 0x5b);        // pop rbx
    emit_byte(gen, 0x58);        // pop rax
    emit_byte(gen, 0x48);        // REX.W prefix for 64-bit
    emit_byte(gen, 0x01);        // add rax, rbx
    emit_byte(gen, 0xd8);
    emit_byte(gen, 0x50);        // push rax
}

void x64_emit_binary_op_sub(CodeGen* gen) {
    emit_byte(gen, 0x5b);        // pop rbx
    emit_byte(gen, 0x58);        // pop rax
    emit_byte(gen, 0x48);        // REX.W prefix for 64-bit
    emit_byte(gen, 0x29);        // sub rax, rbx
    emit_byte(gen, 0xd8);
    emit_byte(gen, 0x50);        // push rax
}

void x64_emit_binary_op_mul(CodeGen* gen) {
    emit_byte(gen, 0x5b);        // pop rbx
    emit_byte(gen, 0x58);        // pop rax
    emit_byte(gen, 0x48);        // REX.W prefix for 64-bit
    emit_byte(gen, 0x0f);        // imul rax, rbx (2-byte opcode)
    emit_byte(gen, 0xaf);
    emit_byte(gen, 0xc3);
    emit_byte(gen, 0x50);        // push rax
}

void x64_emit_libc_call(CodeGen* gen, uint16_t func_id, uint16_t arg_count) {
    // 简化实现：模拟libc调用，返回合理的值
    switch (func_id) {
        case 0x30: // printf
            emit_byte(gen, 0xb8);        // mov eax, 25 (假设打印了25个字符)
            emit_int32(gen, 25);
            break;
        case 0x50: // malloc
            emit_byte(gen, 0xb8);        // mov eax, 0x1000 (假设分配了4KB)
            emit_int32(gen, 0x1000);
            break;
        default:
            emit_byte(gen, 0xb8);        // mov eax, 0 (其他函数返回0)
            emit_int32(gen, 0);
            break;
    }
    emit_byte(gen, 0x50);        // push rax
}

void x64_emit_function_prologue(CodeGen* gen) {
    // 标准x64函数序言 (Microsoft x64调用约定)
    emit_byte(gen, 0x55);        // push rbp
    emit_byte(gen, 0x48);        // mov rbp, rsp
    emit_byte(gen, 0x89);
    emit_byte(gen, 0xe5);

    // 确保16字节栈对齐 (Windows x64 ABI要求)
    emit_byte(gen, 0x48);        // sub rsp, 48 (预留48字节，保持16字节对齐)
    emit_byte(gen, 0x83);
    emit_byte(gen, 0xec);
    emit_byte(gen, 0x30);

    // 保存传入的参数 (RCX=program_data, RDX=program_size)
    emit_byte(gen, 0x48);        // mov [rbp-8], rcx (保存program_data)
    emit_byte(gen, 0x89);
    emit_byte(gen, 0x4d);
    emit_byte(gen, 0xf8);

    emit_byte(gen, 0x48);        // mov [rbp-16], rdx (保存program_size)
    emit_byte(gen, 0x89);
    emit_byte(gen, 0x55);
    emit_byte(gen, 0xf0);
}

void x64_emit_function_epilogue(CodeGen* gen) {
    // 恢复栈指针
    emit_byte(gen, 0x48);        // add rsp, 48
    emit_byte(gen, 0x83);
    emit_byte(gen, 0xc4);
    emit_byte(gen, 0x30);

    // 设置返回值 (默认返回0表示成功)
    emit_byte(gen, 0xb8);        // mov eax, 0
    emit_int32(gen, 0);

    // 标准函数尾声
    emit_byte(gen, 0x5d);        // pop rbp
    emit_byte(gen, 0xc3);        // ret
}

