/**
 * tool_c2astc.c - C源码到ASTC转换工具
 *
 * 将C源码编译为ASTC格式文件
 * 流程: source.c → (c2astc编译器) → output.astc
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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <input.c> [output.astc]\n", argv[0]);
        return 1;
    }
    
    const char* input_file = argv[1];
    const char* output_file = (argc > 2) ? argv[2] : "evolver0_program.astc";
    
    printf("Building Program ASTC...\n");
    printf("Input: %s\n", input_file);
    printf("Output: %s\n", output_file);
    
    // 使用c2astc编译C源码
    struct ASTNode* ast = c2astc_convert_file(input_file, NULL);
    if (!ast) {
        fprintf(stderr, "Error: Failed to compile: %s\n", c2astc_get_error());
        return 1;
    }
    
    // 序列化ASTC
    size_t astc_data_size;
    unsigned char* astc_data = c2astc_serialize(ast, &astc_data_size);
    if (!astc_data) {
        fprintf(stderr, "Error: Failed to serialize: %s\n", c2astc_get_error());
        ast_free(ast);
        return 1;
    }
    
    // 创建ASTC文件
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
    header.entry_point = 0;
    
    // 写入头部和数据
    fwrite(&header, sizeof(ASTCHeader), 1, file);
    fwrite(astc_data, astc_data_size, 1, file);
    fclose(file);
    
    printf("✓ Program ASTC created: %s (%zu bytes)\n", output_file, sizeof(ASTCHeader) + astc_data_size);
    
    // 清理
    free(astc_data);
    ast_free(ast);
    
    return 0;
}
