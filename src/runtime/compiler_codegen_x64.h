#ifndef CODEGENX64_64_H
#define CODEGENX64_64_H

#include "core_astc.h"

// 生成单个函数的x86-64汇编代码
char* generate_function_asm(ASTNode* func_node);

#endif // CODEGENX64_64_H
