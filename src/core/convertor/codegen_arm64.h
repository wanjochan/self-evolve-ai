#ifndef CODEGEN_ARM64_H
#define CODEGEN_ARM64_H

#include "../astc.h"
#include "astc2native.h"

// ARM64架构特定的机器码生成函数
void arm64_emit_nop(CodeGen* gen);
void arm64_emit_halt_with_return_value(CodeGen* gen);
void arm64_emit_const_i32(CodeGen* gen, uint32_t value);
void arm64_emit_binary_op_add(CodeGen* gen);
void arm64_emit_binary_op_sub(CodeGen* gen);
void arm64_emit_binary_op_mul(CodeGen* gen);
void arm64_emit_libc_call(CodeGen* gen, uint16_t func_id, uint16_t arg_count);
void arm64_emit_user_call(CodeGen* gen);

// Arithmetic operations
void arm64_emit_add(CodeGen* gen);
void arm64_emit_sub(CodeGen* gen);
void arm64_emit_mul(CodeGen* gen);
void arm64_emit_div(CodeGen* gen);

// Function prologue/epilogue
void arm64_emit_function_prologue(CodeGen* gen);
void arm64_emit_function_epilogue(CodeGen* gen);

// ARM64 specific instruction encoding helpers
void arm64_emit_mov_immediate(CodeGen* gen, uint8_t reg, uint32_t value);
void arm64_emit_add_registers(CodeGen* gen, uint8_t dst, uint8_t src1, uint8_t src2);
void arm64_emit_sub_registers(CodeGen* gen, uint8_t dst, uint8_t src1, uint8_t src2);
void arm64_emit_mul_registers(CodeGen* gen, uint8_t dst, uint8_t src1, uint8_t src2);
void arm64_emit_branch_link(CodeGen* gen, uint32_t target);
void arm64_emit_return(CodeGen* gen);

#endif // CODEGEN_ARM64_H
