/**
 * tool_build_program.c - Program ASTC构建工具
 * 
 * 将C源码编译成ASTC格式，符合三层架构的Program层要求
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "c2astc.h"

#define ASTC_MAGIC "ASTC"
#define ASTC_VERSION 1

typedef struct {
    char magic[4];          // "ASTC"
    uint32_t version;       // 版本号
    uint32_t size;          // 数据大小
    uint32_t entry_point;   // 入口点
} ASTCHeader;

static void print_usage(const char* program_name) {
    printf("Program ASTC Builder - Three-Layer Architecture\n");
    printf("Usage: %s <input.c> [output.astc]\n", program_name);
    printf("Converts C source code to ASTC format for the Program layer\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char* input_file = argv[1];
    const char* output_file = (argc > 2) ? argv[2] : "evolver0_program.astc";
    
    printf("Building Program ASTC for three-layer architecture...\n");
    printf("Input: %s\n", input_file);
    printf("Output: %s\n", output_file);
    
    // 步骤1: 使用c2astc编译C源码
    printf("Step 1: Compiling C source to ASTC...\n");
    
    struct ASTNode* ast = c2astc_convert_file(input_file, NULL);
    if (!ast) {
        fprintf(stderr, "Error: Failed to compile C source: %s\n", c2astc_get_error());
        return 1;
    }
    
    printf("✓ C source compiled successfully\n");
    
    // 步骤2: 序列化ASTC
    printf("Step 2: Serializing ASTC...\n");
    
    size_t astc_data_size;
    unsigned char* astc_data = c2astc_serialize(ast, &astc_data_size);
    if (!astc_data) {
        fprintf(stderr, "Error: Failed to serialize ASTC: %s\n", c2astc_get_error());
        ast_free(ast);
        return 1;
    }
    
    printf("✓ ASTC serialized: %zu bytes\n", astc_data_size);
    
    // 步骤3: 创建ASTC文件
    printf("Step 3: Creating ASTC file...\n");
    
    FILE* file = fopen(output_file, "wb");
    if (!file) {
        fprintf(stderr, "Error: Cannot create output file: %s\n", output_file);
        free(astc_data);
        ast_free(ast);
        return 1;
    }
    
    // 创建ASTC头
    ASTCHeader header;
    memcpy(header.magic, ASTC_MAGIC, 4);
    header.version = ASTC_VERSION;
    header.size = astc_data_size;
    header.entry_point = 0;  // 主函数入口点
    
    // 写入头部
    fwrite(&header, sizeof(ASTCHeader), 1, file);
    
    // 写入ASTC数据
    fwrite(astc_data, astc_data_size, 1, file);
    
    fclose(file);
    
    printf("✓ Program ASTC created: %s\n", output_file);
    printf("  Header size: %zu bytes\n", sizeof(ASTCHeader));
    printf("  ASTC data size: %zu bytes\n", astc_data_size);
    printf("  Total size: %zu bytes\n", sizeof(ASTCHeader) + astc_data_size);
    
    // 清理
    free(astc_data);
    ast_free(ast);
    
    return 0;
}
