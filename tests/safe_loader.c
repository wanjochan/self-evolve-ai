/**
 * safe_loader.c - 安全版本的Loader，使用库调用而非机器码执行
 * 
 * 这个版本避免直接执行机器码，而是使用我们已有的runtime库
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// 包含我们的runtime库
#include "../runtime.h"
#include "../astc.h"
#include "../c2astc.h"

// ASTC头部结构
typedef struct {
    char magic[4];          // "ASTC"
    uint32_t version;       // 版本号
    uint32_t size;          // 数据大小
    uint32_t entry_point;   // 入口点
} ASTCHeader;

static void* load_file(const char* filename, size_t* size) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file: %s\n", filename);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    void* data = malloc(*size);
    if (!data) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return NULL;
    }
    
    if (fread(data, 1, *size, file) != *size) {
        fprintf(stderr, "Error: Failed to read file: %s\n", filename);
        free(data);
        fclose(file);
        return NULL;
    }
    
    fclose(file);
    return data;
}

int main(int argc, char* argv[]) {
    printf("=== Safe Loader - 使用库调用执行ASTC ===\n");
    
    if (argc != 3) {
        printf("Usage: %s <runtime.bin> <program.astc>\n", argv[0]);
        return 1;
    }
    
    const char* runtime_file = argv[1];
    const char* program_file = argv[2];
    
    printf("Runtime: %s\n", runtime_file);
    printf("Program: %s\n", program_file);
    
    // 加载Program ASTC文件
    printf("Loading Program ASTC...\n");
    size_t program_size;
    void* program_data = load_file(program_file, &program_size);
    if (!program_data) {
        return 1;
    }
    
    // 验证ASTC格式
    if (program_size < sizeof(ASTCHeader)) {
        fprintf(stderr, "Error: Invalid ASTC file format\n");
        free(program_data);
        return 1;
    }
    
    ASTCHeader* astc_header = (ASTCHeader*)program_data;
    if (memcmp(astc_header->magic, "ASTC", 4) != 0) {
        fprintf(stderr, "Error: Invalid ASTC magic number\n");
        free(program_data);
        return 1;
    }
    
    printf("✓ ASTC file loaded: %zu bytes, version %u\n", 
           program_size, astc_header->version);
    
    // 提取ASTC数据
    unsigned char* astc_data = (unsigned char*)program_data + sizeof(ASTCHeader);
    size_t astc_data_size = astc_header->size;
    
    printf("Executing ASTC data: %zu bytes\n", astc_data_size);
    
    // 使用我们的runtime库执行ASTC
    printf("Initializing Runtime VM...\n");
    RuntimeVM vm;
    if (!runtime_init(&vm)) {
        fprintf(stderr, "Failed to initialize VM\n");
        free(program_data);
        return 1;
    }
    
    printf("Deserializing ASTC program...\n");
    struct ASTNode* program = c2astc_deserialize(astc_data, astc_data_size);
    if (!program) {
        fprintf(stderr, "Failed to deserialize ASTC program\n");
        runtime_destroy(&vm);
        free(program_data);
        return 1;
    }
    
    printf("Loading program to VM...\n");
    if (!runtime_load_program(&vm, program)) {
        fprintf(stderr, "Failed to load program: %s\n", runtime_get_error(&vm));
        ast_free(program);
        runtime_destroy(&vm);
        free(program_data);
        return 1;
    }
    
    printf("Executing main function...\n");
    int result = runtime_execute(&vm, "main");
    printf("Execution completed with result: %d\n", result);
    
    // 清理
    ast_free(program);
    runtime_destroy(&vm);
    free(program_data);
    
    printf("✅ Safe execution completed successfully!\n");
    return result;
}
