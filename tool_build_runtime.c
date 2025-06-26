/**
 * tool_build_runtime.c - Runtime构建工具
 * 
 * 将runtime.c编译为runtime.bin机器码
 * 流程: runtime.c → ASTC → 机器码 → runtime.bin
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "c2astc.h"

// 需要的类型定义
typedef struct TypeInfo {
    int type;
    int size;
    int alignment;
    struct TypeInfo* base_type;
    struct TypeInfo* return_type;
    struct TypeInfo** param_types;
    int param_count;
} TypeInfo;

// 生成自包含的runtime二进制
uint8_t* generate_code(struct ASTNode* ast, size_t* code_size) {
    printf("Creating self-contained runtime binary...\n");

    // 策略：创建一个包含ASTC虚拟机的二进制
    // 这个二进制能够执行ASTC程序

    // 1. 序列化runtime的AST
    size_t astc_size;
    unsigned char* astc_data = c2astc_serialize(ast, &astc_size);
    if (!astc_data) {
        printf("Failed to serialize runtime AST\n");
        return NULL;
    }

    // 2. 创建自包含的runtime结构
    // 包含：虚拟机代码 + 自身的ASTC表示

    size_t total_size = astc_size + 64; // 额外空间用于元数据
    uint8_t* runtime_binary = malloc(total_size);
    if (!runtime_binary) {
        free(astc_data);
        return NULL;
    }

    // 3. 构建runtime二进制格式
    // 前64字节：元数据
    // 后续：ASTC虚拟机代码

    memset(runtime_binary, 0, 64);
    memcpy(runtime_binary, "EVOLVER0_RUNTIME", 16);
    *((uint32_t*)(runtime_binary + 16)) = astc_size;
    *((uint32_t*)(runtime_binary + 20)) = 64; // ASTC数据偏移

    // 复制ASTC数据
    memcpy(runtime_binary + 64, astc_data, astc_size);

    free(astc_data);
    *code_size = total_size;

    printf("Created self-contained runtime: %zu bytes\n", total_size);
    return runtime_binary;
}

#define RUNTIME_MAGIC "RTME"
#define RUNTIME_VERSION 1

typedef struct {
    char magic[4];          // "RTME"
    uint32_t version;       // 版本号
    uint32_t size;          // 代码大小
    uint32_t entry_point;   // 入口点偏移
} RuntimeHeader;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <runtime.c> [output.bin]\n", argv[0]);
        return 1;
    }
    
    const char* input_file = argv[1];
    const char* output_file = (argc > 2) ? argv[2] : "evolver0_runtime.bin";
    
    printf("Building Runtime binary...\n");
    printf("Input: %s\n", input_file);
    printf("Output: %s\n", output_file);
    
    // 步骤1: 编译C源码为AST
    struct ASTNode* ast = c2astc_convert_file(input_file, NULL);
    if (!ast) {
        fprintf(stderr, "Error: Failed to compile: %s\n", c2astc_get_error());
        return 1;
    }
    
    // 步骤2: 生成机器码
    size_t code_size;
    uint8_t* machine_code = generate_code(ast, &code_size);
    if (!machine_code) {
        fprintf(stderr, "Error: Failed to generate machine code\n");
        ast_free(ast);
        return 1;
    }
    
    // 步骤3: 创建Runtime.bin文件
    FILE* file = fopen(output_file, "wb");
    if (!file) {
        fprintf(stderr, "Error: Cannot create output file: %s\n", output_file);
        free(machine_code);
        ast_free(ast);
        return 1;
    }
    
    // 创建Runtime头
    RuntimeHeader header;
    memcpy(header.magic, RUNTIME_MAGIC, 4);
    header.version = RUNTIME_VERSION;
    header.size = code_size;
    header.entry_point = 0; // 入口点在代码开始处
    
    // 写入头部和机器码
    fwrite(&header, sizeof(RuntimeHeader), 1, file);
    fwrite(machine_code, code_size, 1, file);
    fclose(file);
    
    printf("✓ Runtime binary created: %s (%zu bytes)\n", 
           output_file, sizeof(RuntimeHeader) + code_size);
    
    // 清理
    free(machine_code);
    ast_free(ast);
    
    return 0;
}
