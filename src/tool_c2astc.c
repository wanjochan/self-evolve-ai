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
#include "runtime/compiler_c2astc.h"

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
        printf("Usage: %s <input.c> [output.astc] [-O0|-O1|-O2|-O3] [-g]\n", argv[0]);
        printf("Options:\n");
        printf("  -O0    No optimization (default)\n");
        printf("  -O1    Basic optimization\n");
        printf("  -O2    Advanced optimization\n");
        printf("  -O3    Aggressive optimization\n");
        printf("  -g     Generate debug information\n");
        return 1;
    }

    const char* input_file = argv[1];
    const char* output_file = (argc > 2) ? argv[2] : "evolver0_program.astc";

    // 解析优化选项
    C2AstcOptions options = c2astc_default_options();
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "-O1") == 0) {
            options.optimize_level = 1;
        } else if (strcmp(argv[i], "-O2") == 0) {
            options.optimize_level = 2;
        } else if (strcmp(argv[i], "-O3") == 0) {
            options.optimize_level = 3;
        } else if (strcmp(argv[i], "-g") == 0) {
            options.emit_debug_info = true;
        }
    }

    printf("Building Program ASTC...\n");
    printf("Input: %s\n", input_file);
    printf("Output: %s\n", output_file);
    if (options.optimize_level > 0) {
        printf("Optimization: O%d\n", options.optimize_level);
    }
    if (options.emit_debug_info) {
        printf("Debug info: enabled\n");
    }

    // 使用c2astc编译C源码
    struct ASTNode* ast = c2astc_convert_file(input_file, &options);
    if (!ast) {
        fprintf(stderr, "Error: Failed to compile: %s\n", c2astc_get_error());
        return 1;
    }
    
    // 将AST转换为ASTC字节码（使用优化选项）
    size_t astc_data_size;
    unsigned char* astc_data = ast_to_astc_bytecode_with_options(ast, &options, &astc_data_size);
    if (!astc_data) {
        fprintf(stderr, "Error: Failed to generate ASTC bytecode: %s\n", c2astc_get_error());
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
