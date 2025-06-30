#include "codegen_arm64.h"
#include "astc2native.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ===============================================
// ARM64架构特定的机器码生成函数
// ===============================================

void arm64_emit_nop(CodeGen* gen) {
    // ARM64 NOP instruction: 0xD503201F
    emit_byte(gen, 0x1F);
    emit_byte(gen, 0x20);
    emit_byte(gen, 0x03);
    emit_byte(gen, 0xD5);
}

void arm64_emit_halt_with_return_value(CodeGen* gen) {
    // ARM64 function epilogue with return value
    // mov w0, w1 (move return value to w0)
    emit_byte(gen, 0xE0);
    emit_byte(gen, 0x03);
    emit_byte(gen, 0x01);
    emit_byte(gen, 0x2A);
    
    // ret (return)
    emit_byte(gen, 0xC0);
    emit_byte(gen, 0x03);
    emit_byte(gen, 0x5F);
    emit_byte(gen, 0xD6);
}

void arm64_emit_const_i32(CodeGen* gen, uint32_t value) {
    // mov w1, #immediate (load 32-bit immediate to w1)
    // This is a simplified implementation for small values
    if (value <= 0xFFFF) {
        uint16_t imm16 = (uint16_t)value;
        uint32_t instruction = 0x52800001 | ((uint32_t)imm16 << 5);
        
        emit_byte(gen, instruction & 0xFF);
        emit_byte(gen, (instruction >> 8) & 0xFF);
        emit_byte(gen, (instruction >> 16) & 0xFF);
        emit_byte(gen, (instruction >> 24) & 0xFF);
    } else {
        // For larger values, use movz + movk sequence
        // movz w1, #(value & 0xFFFF)
        uint32_t movz = 0x52800001 | ((value & 0xFFFF) << 5);
        emit_byte(gen, movz & 0xFF);
        emit_byte(gen, (movz >> 8) & 0xFF);
        emit_byte(gen, (movz >> 16) & 0xFF);
        emit_byte(gen, (movz >> 24) & 0xFF);
        
        // movk w1, #(value >> 16), lsl #16
        if (value >> 16) {
            uint32_t movk = 0x72A00001 | (((value >> 16) & 0xFFFF) << 5);
            emit_byte(gen, movk & 0xFF);
            emit_byte(gen, (movk >> 8) & 0xFF);
            emit_byte(gen, (movk >> 16) & 0xFF);
            emit_byte(gen, (movk >> 24) & 0xFF);
        }
    }
}

void arm64_emit_binary_op_add(CodeGen* gen) {
    // Simplified: add w1, w1, w2 (w1 = w1 + w2)
    emit_byte(gen, 0x21);
    emit_byte(gen, 0x00);
    emit_byte(gen, 0x02);
    emit_byte(gen, 0x0B);
}

void arm64_emit_binary_op_sub(CodeGen* gen) {
    // Simplified: sub w1, w1, w2 (w1 = w1 - w2)
    emit_byte(gen, 0x21);
    emit_byte(gen, 0x00);
    emit_byte(gen, 0x02);
    emit_byte(gen, 0x4B);
}

void arm64_emit_binary_op_mul(CodeGen* gen) {
    // Simplified: mul w1, w1, w2 (w1 = w1 * w2)
    emit_byte(gen, 0x21);
    emit_byte(gen, 0x7C);
    emit_byte(gen, 0x02);
    emit_byte(gen, 0x1B);
}

void arm64_emit_libc_call(CodeGen* gen, uint16_t func_id, uint16_t arg_count) {
    // Simplified implementation: simulate libc calls
    switch (func_id) {
        case 0x30: // printf
            // mov w0, #25 (return 25 characters printed)
            arm64_emit_const_i32(gen, 25);
            break;
        case 0x50: // malloc
            // mov w0, #0x1000 (return 4KB address)
            arm64_emit_const_i32(gen, 0x1000);
            break;
        default:
            // mov w0, #0 (default return 0)
            arm64_emit_const_i32(gen, 0);
            break;
    }
}

void arm64_emit_user_call(CodeGen* gen) {
    // Simplified user function call
    // mov w0, #42 (return 42)
    arm64_emit_const_i32(gen, 42);
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
    // ARM64 division: udiv w1, w1, w2
    emit_byte(gen, 0x21);
    emit_byte(gen, 0x08);
    emit_byte(gen, 0xC2);
    emit_byte(gen, 0x1A);
}

// Function prologue/epilogue
void arm64_emit_function_prologue(CodeGen* gen) {
    // stp x29, x30, [sp, #-16]! (save frame pointer and link register)
    emit_byte(gen, 0xFD);
    emit_byte(gen, 0x7B);
    emit_byte(gen, 0xBF);
    emit_byte(gen, 0xA9);
    
    // mov x29, sp (set up frame pointer)
    emit_byte(gen, 0xFD);
    emit_byte(gen, 0x03);
    emit_byte(gen, 0x00);
    emit_byte(gen, 0x91);
}

void arm64_emit_function_epilogue(CodeGen* gen) {
    // ldp x29, x30, [sp], #16 (restore frame pointer and link register)
    emit_byte(gen, 0xFD);
    emit_byte(gen, 0x7B);
    emit_byte(gen, 0xC1);
    emit_byte(gen, 0xA8);
    
    // ret (return)
    emit_byte(gen, 0xC0);
    emit_byte(gen, 0x03);
    emit_byte(gen, 0x5F);
    emit_byte(gen, 0xD6);
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
