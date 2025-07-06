#include "codegen_arm64.h"
#include "astc2native.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ===============================================
// ARM64架构特定的机器码生成函数
// ===============================================

// ARM64 instruction encoding helpers
static void emit_uint32(CodeGen* gen, uint32_t value) {
    emit_byte(gen, value & 0xFF);
    emit_byte(gen, (value >> 8) & 0xFF);
    emit_byte(gen, (value >> 16) & 0xFF);
    emit_byte(gen, (value >> 24) & 0xFF);
}

void arm64_emit_nop(CodeGen* gen) {
    // NOP (d503201f)
    emit_uint32(gen, 0xd503201f);
}

void arm64_emit_halt_with_return_value(CodeGen* gen) {
    // mov x0, #0
    emit_uint32(gen, 0xd2800000);
    
    // ret
    emit_uint32(gen, 0xd65f03c0);
}

void arm64_emit_const_i32(CodeGen* gen, uint32_t value) {
    // mov x0, #value
    if (value <= 0xFFF) {
        // Can use immediate
        emit_uint32(gen, 0xd2800000 | (value << 5));
    } else {
        // Need to load in parts
        // movz x0, #(value & 0xFFFF)
        emit_uint32(gen, 0xd2800000 | ((value & 0xFFFF) << 5));
        
        if (value > 0xFFFF) {
            // movk x0, #((value >> 16) & 0xFFFF), lsl #16
            emit_uint32(gen, 0xf2a00000 | (((value >> 16) & 0xFFFF) << 5));
        }
    }
    
    // str x0, [sp, #-16]!
    emit_uint32(gen, 0xf81f0fe0);
}

void arm64_emit_binary_op_add(CodeGen* gen) {
    // ldr x1, [sp], #16
    emit_uint32(gen, 0xf84107e1);
    
    // ldr x0, [sp], #16
    emit_uint32(gen, 0xf84107e0);
    
    // add x0, x0, x1
    emit_uint32(gen, 0x8b000020);
    
    // str x0, [sp, #-16]!
    emit_uint32(gen, 0xf81f0fe0);
}

void arm64_emit_binary_op_sub(CodeGen* gen) {
    // ldr x1, [sp], #16
    emit_uint32(gen, 0xf84107e1);
    
    // ldr x0, [sp], #16
    emit_uint32(gen, 0xf84107e0);
    
    // sub x0, x0, x1
    emit_uint32(gen, 0xcb000020);
    
    // str x0, [sp, #-16]!
    emit_uint32(gen, 0xf81f0fe0);
}

void arm64_emit_binary_op_mul(CodeGen* gen) {
    // ldr x1, [sp], #16
    emit_uint32(gen, 0xf84107e1);
    
    // ldr x0, [sp], #16
    emit_uint32(gen, 0xf84107e0);
    
    // mul x0, x0, x1
    emit_uint32(gen, 0x9b007c20);
    
    // str x0, [sp, #-16]!
    emit_uint32(gen, 0xf81f0fe0);
}

void arm64_emit_libc_call(CodeGen* gen, uint16_t func_id, uint16_t arg_count) {
    // Save argument count
    // mov w1, #arg_count
    emit_uint32(gen, 0x52800000 | (arg_count & 0xFFFF));
    
    // Save function ID
    // mov w0, #func_id
    emit_uint32(gen, 0x52800000 | (func_id & 0xFFFF));
    
    // bl libc_dispatcher
    emit_uint32(gen, 0x94000000); // Placeholder for relative address
}

void arm64_emit_user_call(CodeGen* gen) {
    // ldr x16, [sp], #16
    emit_uint32(gen, 0xf84107f0);
    
    // blr x16
    emit_uint32(gen, 0xd63f0200);
}

// Arithmetic operations
void arm64_emit_add(CodeGen* gen) {
    arm64_emit_binary_op_add(gen);
}

void arm64_emit_sub(CodeGen* gen) {
    arm64_emit_binary_op_sub(gen);
}

void arm64_emit_mul(CodeGen* gen) {
    arm64_emit_binary_op_mul(gen);
}

void arm64_emit_div(CodeGen* gen) {
    // ldr x1, [sp], #16
    emit_uint32(gen, 0xf84107e1);
    
    // ldr x0, [sp], #16
    emit_uint32(gen, 0xf84107e0);
    
    // sdiv x0, x0, x1
    emit_uint32(gen, 0x9ac00c20);
    
    // str x0, [sp, #-16]!
    emit_uint32(gen, 0xf81f0fe0);
}

void arm64_emit_less_than(CodeGen* gen) {
    // ldr x1, [sp], #16
    emit_uint32(gen, 0xf84107e1);
    
    // ldr x0, [sp], #16
    emit_uint32(gen, 0xf84107e0);
    
    // cmp x0, x1
    emit_uint32(gen, 0xeb01001f);
    
    // cset x0, lt
    emit_uint32(gen, 0x9a9f17e0);
    
    // str x0, [sp, #-16]!
    emit_uint32(gen, 0xf81f0fe0);
}

void arm64_emit_equal(CodeGen* gen) {
    // ldr x1, [sp], #16
    emit_uint32(gen, 0xf84107e1);
    
    // ldr x0, [sp], #16
    emit_uint32(gen, 0xf84107e0);
    
    // cmp x0, x1
    emit_uint32(gen, 0xeb01001f);
    
    // cset x0, eq
    emit_uint32(gen, 0x9a9f07e0);
    
    // str x0, [sp, #-16]!
    emit_uint32(gen, 0xf81f0fe0);
}

// Function prologue/epilogue
void arm64_emit_function_prologue(CodeGen* gen) {
    // stp x29, x30, [sp, #-16]!
    emit_uint32(gen, 0xa9bf7bfd);
    
    // mov x29, sp
    emit_uint32(gen, 0x910003fd);
    
    // sub sp, sp, #32
    emit_uint32(gen, 0xd10083ff);
}

void arm64_emit_function_epilogue(CodeGen* gen) {
    // mov sp, x29
    emit_uint32(gen, 0x910003bf);
    
    // ldp x29, x30, [sp], #16
    emit_uint32(gen, 0xa8c17bfd);
    
    // ret
    emit_uint32(gen, 0xd65f03c0);
}

// ARM64 specific instruction encoding helpers
void arm64_emit_mov_immediate(CodeGen* gen, uint8_t reg, uint32_t value) {
    // mov w{reg}, #value
    uint32_t instruction = 0x52800000 | reg | ((value & 0xFFFF) << 5);
    emit_byte(gen, instruction & 0xFF);
    emit_byte(gen, (instruction >> 8) & 0xFF);
    emit_byte(gen, (instruction >> 16) & 0xFF);
    emit_byte(gen, (instruction >> 24) & 0xFF);
}

void arm64_emit_add_registers(CodeGen* gen, uint8_t dst, uint8_t src1, uint8_t src2) {
    // add w{dst}, w{src1}, w{src2}
    uint32_t instruction = 0x0B000000 | dst | (src1 << 5) | (src2 << 16);
    emit_byte(gen, instruction & 0xFF);
    emit_byte(gen, (instruction >> 8) & 0xFF);
    emit_byte(gen, (instruction >> 16) & 0xFF);
    emit_byte(gen, (instruction >> 24) & 0xFF);
}

void arm64_emit_sub_registers(CodeGen* gen, uint8_t dst, uint8_t src1, uint8_t src2) {
    // sub w{dst}, w{src1}, w{src2}
    uint32_t instruction = 0x4B000000 | dst | (src1 << 5) | (src2 << 16);
    emit_byte(gen, instruction & 0xFF);
    emit_byte(gen, (instruction >> 8) & 0xFF);
    emit_byte(gen, (instruction >> 16) & 0xFF);
    emit_byte(gen, (instruction >> 24) & 0xFF);
}

void arm64_emit_mul_registers(CodeGen* gen, uint8_t dst, uint8_t src1, uint8_t src2) {
    // mul w{dst}, w{src1}, w{src2}
    uint32_t instruction = 0x1B007C00 | dst | (src1 << 5) | (src2 << 16);
    emit_byte(gen, instruction & 0xFF);
    emit_byte(gen, (instruction >> 8) & 0xFF);
    emit_byte(gen, (instruction >> 16) & 0xFF);
    emit_byte(gen, (instruction >> 24) & 0xFF);
}

void arm64_emit_branch_link(CodeGen* gen, uint32_t target) {
    // bl target (branch with link)
    uint32_t instruction = 0x94000000 | ((target >> 2) & 0x3FFFFFF);
    emit_byte(gen, instruction & 0xFF);
    emit_byte(gen, (instruction >> 8) & 0xFF);
    emit_byte(gen, (instruction >> 16) & 0xFF);
    emit_byte(gen, (instruction >> 24) & 0xFF);
}

void arm64_emit_return(CodeGen* gen) {
    // ret
    emit_byte(gen, 0xC0);
    emit_byte(gen, 0x03);
    emit_byte(gen, 0x5F);
    emit_byte(gen, 0xD6);
}

char* generate_arm64_function_asm(ASTNode* func_node) {
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
                      "    stp x29, x30, [sp, #-16]!\n"
                      "    mov x29, sp\n"
                      "    sub sp, sp, #32\n");
    
    // 生成函数体
    if (func_node->data.func_decl.body) {
        // TODO: 实现函数体的汇编生成
        length += snprintf(asm_code + length, capacity - length,
                         "    ; Function body implementation\n");
    }
    
    // 生成函数尾声
    length += snprintf(asm_code + length, capacity - length,
                      "    mov sp, x29\n"
                      "    ldp x29, x30, [sp], #16\n"
                      "    ret\n");
    
    return asm_code;
}
