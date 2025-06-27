#ifndef X64_CODEGEN_H
#define X64_CODEGEN_H

#include "astc.h"

// 生成单个函数的x86-64汇编代码
char* generate_function_asm(ASTNode* func_node);

#endif //X64_CODEGEN_H
