#include "codegen_x64.h"
#include "memory.h"
#include <stdio.h>
#include <string.h>

// x64 instruction encoding helpers
static void emit_rex(CodeGen* gen, bool w, bool r, bool x, bool b) {
    uint8_t rex = 0x40;
    if (w) rex |= 0x08;
    if (r) rex |= 0x04;
    if (x) rex |= 0x02;
    if (b) rex |= 0x01;
    emit_byte(gen, rex);
}

void x64_emit_nop(CodeGen* gen) {
    emit_byte(gen, 0x90); // NOP
}

void x64_emit_halt_with_return_value(CodeGen* gen) {
    // mov eax, 0
    emit_byte(gen, 0xb8);
    emit_int32(gen, 0);
    
    // ret
    emit_byte(gen, 0xc3);
}

void x64_emit_const_i32(CodeGen* gen, uint32_t value) {
    // mov eax, imm32
    emit_byte(gen, 0xb8);
    emit_int32(gen, value);
}

void x64_emit_binary_op_add(CodeGen* gen) {
    // pop rdx
    emit_byte(gen, 0x5a);
    
    // pop rax
    emit_byte(gen, 0x58);
    
    // add rax, rdx
    emit_rex(gen, true, false, false, false);
    emit_byte(gen, 0x01);
    emit_byte(gen, 0xd0);
    
    // push rax
    emit_byte(gen, 0x50);
}

void x64_emit_binary_op_sub(CodeGen* gen) {
    // pop rdx
    emit_byte(gen, 0x5a);
    
    // pop rax
    emit_byte(gen, 0x58);
    
    // sub rax, rdx
    emit_rex(gen, true, false, false, false);
    emit_byte(gen, 0x29);
    emit_byte(gen, 0xd0);
    
    // push rax
    emit_byte(gen, 0x50);
}

void x64_emit_binary_op_mul(CodeGen* gen) {
    // pop rdx
    emit_byte(gen, 0x5a);
    
    // pop rax
    emit_byte(gen, 0x58);
    
    // mul rdx
    emit_rex(gen, true, false, false, false);
    emit_byte(gen, 0x0f);
    emit_byte(gen, 0xaf);
    emit_byte(gen, 0xc2);
    
    // push rax
    emit_byte(gen, 0x50);
}

void x64_emit_libc_call(CodeGen* gen, uint16_t func_id, uint16_t arg_count) {
    // Save argument count
    emit_byte(gen, 0x6a); // push imm8
    emit_byte(gen, arg_count);
    
    // Save function ID
    emit_byte(gen, 0x68); // push imm32
    emit_int32(gen, func_id);
    
    // Call libc dispatcher
    emit_byte(gen, 0xe8); // call rel32
    emit_int32(gen, 0); // Placeholder for relative address
}

void x64_emit_user_call(CodeGen* gen) {
    // pop rax (function address)
    emit_byte(gen, 0x58);
    
    // call rax
    emit_byte(gen, 0xff);
    emit_byte(gen, 0xd0);
}

void x64_emit_add(CodeGen* gen) {
    // pop rdx
    emit_byte(gen, 0x5a);
    
    // pop rax
    emit_byte(gen, 0x58);
    
    // add rax, rdx
    emit_rex(gen, true, false, false, false);
    emit_byte(gen, 0x01);
    emit_byte(gen, 0xd0);
    
    // push rax
    emit_byte(gen, 0x50);
}

void x64_emit_sub(CodeGen* gen) {
    // pop rdx
    emit_byte(gen, 0x5a);
    
    // pop rax
    emit_byte(gen, 0x58);
    
    // sub rax, rdx
    emit_rex(gen, true, false, false, false);
    emit_byte(gen, 0x29);
    emit_byte(gen, 0xd0);
    
    // push rax
    emit_byte(gen, 0x50);
}

void x64_emit_mul(CodeGen* gen) {
    // pop rdx
    emit_byte(gen, 0x5a);
    
    // pop rax
    emit_byte(gen, 0x58);
    
    // imul rax, rdx
    emit_rex(gen, true, false, false, false);
    emit_byte(gen, 0x0f);
    emit_byte(gen, 0xaf);
    emit_byte(gen, 0xc2);
    
    // push rax
    emit_byte(gen, 0x50);
}

void x64_emit_div(CodeGen* gen) {
    // pop rcx (divisor)
    emit_byte(gen, 0x59);
    
    // pop rax (dividend)
    emit_byte(gen, 0x58);
    
    // xor rdx, rdx (clear rdx for division)
    emit_rex(gen, true, false, false, false);
    emit_byte(gen, 0x31);
    emit_byte(gen, 0xd2);
    
    // div rcx
    emit_rex(gen, true, false, false, false);
    emit_byte(gen, 0xf7);
    emit_byte(gen, 0xf1);
    
    // push rax (quotient)
    emit_byte(gen, 0x50);
}

void x64_emit_less_than(CodeGen* gen) {
    // pop rdx (second operand)
    emit_byte(gen, 0x5a);
    
    // pop rax (first operand)
    emit_byte(gen, 0x58);
    
    // cmp rax, rdx
    emit_rex(gen, true, false, false, false);
    emit_byte(gen, 0x39);
    emit_byte(gen, 0xd0);
    
    // setl al
    emit_byte(gen, 0x0f);
    emit_byte(gen, 0x9c);
    emit_byte(gen, 0xc0);
    
    // movzx rax, al
    emit_rex(gen, true, false, false, false);
    emit_byte(gen, 0x0f);
    emit_byte(gen, 0xb6);
    emit_byte(gen, 0xc0);
    
    // push rax
    emit_byte(gen, 0x50);
}

void x64_emit_equal(CodeGen* gen) {
    // pop rdx (second operand)
    emit_byte(gen, 0x5a);
    
    // pop rax (first operand)
    emit_byte(gen, 0x58);
    
    // cmp rax, rdx
    emit_rex(gen, true, false, false, false);
    emit_byte(gen, 0x39);
    emit_byte(gen, 0xd0);
    
    // sete al
    emit_byte(gen, 0x0f);
    emit_byte(gen, 0x94);
    emit_byte(gen, 0xc0);
    
    // movzx rax, al
    emit_rex(gen, true, false, false, false);
    emit_byte(gen, 0x0f);
    emit_byte(gen, 0xb6);
    emit_byte(gen, 0xc0);
    
    // push rax
    emit_byte(gen, 0x50);
}

void x64_emit_function_prologue(CodeGen* gen) {
    // push rbp
    emit_byte(gen, 0x55);
    
    // mov rbp, rsp
    emit_rex(gen, true, false, false, false);
    emit_byte(gen, 0x89);
    emit_byte(gen, 0xe5);
    
    // sub rsp, 32 (reserve shadow space for Win64 ABI)
    emit_rex(gen, true, false, false, false);
    emit_byte(gen, 0x83);
    emit_byte(gen, 0xec);
    emit_byte(gen, 0x20);
}

void x64_emit_function_epilogue(CodeGen* gen) {
    // mov rsp, rbp
    emit_rex(gen, true, false, false, false);
    emit_byte(gen, 0x89);
    emit_byte(gen, 0xec);
    
    // pop rbp
    emit_byte(gen, 0x5d);
    
    // ret
    emit_byte(gen, 0xc3);
}

char* generate_function_asm(ASTNode* func_node) {
    if (!func_node || func_node->type != ASTC_FUNC_DECL) {
        return NULL;
    }
    
    // 分配一个初始的字符串缓冲区
    size_t capacity = 1024;
    char* asm_code = memory_alloc(capacity, MEMORY_POOL_GENERAL);
    if (!asm_code) return NULL;
    
    size_t length = 0;
    
    // 生成函数标签
    length += snprintf(asm_code + length, capacity - length,
                      "%s:\n", func_node->data.func_decl.name);
    
    // 生成函数序言
    length += snprintf(asm_code + length, capacity - length,
                      "    push rbp\n"
                      "    mov rbp, rsp\n"
                      "    sub rsp, 32\n");
    
    // 生成函数体
    if (func_node->data.func_decl.body) {
        // TODO: 实现函数体的汇编生成
        length += snprintf(asm_code + length, capacity - length,
                         "    ; Function body implementation\n");
    }
    
    // 生成函数尾声
    length += snprintf(asm_code + length, capacity - length,
                      "    mov rsp, rbp\n"
                      "    pop rbp\n"
                      "    ret\n");
    
    return asm_code;
}

