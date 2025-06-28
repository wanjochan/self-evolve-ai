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

// x64函数序言和尾声
void x64_emit_function_prologue(CodeGen* gen);
void x64_emit_function_epilogue(CodeGen* gen);

#endif // CODEGENX64_64_H
