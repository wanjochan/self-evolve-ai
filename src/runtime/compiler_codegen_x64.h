#ifndef CODEGENX64_64_H
#define CODEGENX64_64_H

#include "core_astc.h"
#include "compiler_astc2rt.h"

// 生成单个函数的x86-64汇编代码
char* generate_function_asm(ASTNode* func_node);

// x64架构特定的机器码生成函数
void x64_emit_nop(CodeGen* gen);
void x64_emit_halt_with_return_value(CodeGen* gen);
void x64_emit_const_i32(CodeGen* gen, uint32_t value);
void x64_emit_binary_op_add(CodeGen* gen);
void x64_emit_binary_op_sub(CodeGen* gen);
void x64_emit_binary_op_mul(CodeGen* gen);
void x64_emit_libc_call(CodeGen* gen, uint16_t func_id, uint16_t arg_count);
void x64_emit_user_call(CodeGen* gen);

// Arithmetic operations
void x64_emit_add(CodeGen* gen);
void x64_emit_sub(CodeGen* gen);
void x64_emit_mul(CodeGen* gen);
void x64_emit_div(CodeGen* gen);

// Comparison operations
void x64_emit_less_than(CodeGen* gen);
void x64_emit_equal(CodeGen* gen);

// x64函数序言和尾声
void x64_emit_function_prologue(CodeGen* gen);
void x64_emit_function_epilogue(CodeGen* gen);

#endif // CODEGENX64_64_H
