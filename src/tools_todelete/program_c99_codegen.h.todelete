#ifndef PROGRAM_C99_CODEGEN_H
#define PROGRAM_C99_CODEGEN_H

#include "astc.h"

// 代码生成器结构
typedef struct {
    char* buffer;           // 汇编代码缓冲区
    size_t buffer_size;     // 缓冲区大小
    size_t buffer_offset;   // 当前写入位置
    int label_count;        // 标签计数器，用于生成唯一的标签
} CodeGenerator;

// 初始化代码生成器
void codegen_init(CodeGenerator* cg);

// 释放代码生成器资源
void codegen_free(CodeGenerator* cg);

// 生成整个翻译单元的汇编代码
int codegen_generate_translation_unit(CodeGenerator* cg, ASTNode* unit_node);

// 生成单个函数的汇编代码
int codegen_generate_function(CodeGenerator* cg, ASTNode* func_node);

#endif // PROGRAM_C99_CODEGEN_H
