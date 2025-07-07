/**
 * simple_c2astc.c - 简化的C到ASTC转换工具
 * 
 * 基于模块系统的简单C到ASTC转换器
 */

#include "../core/module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// ASTC文件格式定义
typedef struct {
    char magic[4];          // "ASTC"
    uint32_t version;       // 版本号
    uint32_t flags;         // 标志
    uint32_t entry_point;   // 入口点
    uint32_t source_size;   // 源码大小
} ASTCHeader;

/**
 * 读取文件内容
 */
static char* read_file(const char* filename, size_t* size) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("错误: 无法打开文件 %s\n", filename);
        return NULL;
    }
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // 分配内存并读取
    char* content = malloc(*size + 1);
    if (!content) {
        printf("错误: 内存分配失败\n");
        fclose(file);
        return NULL;
    }
    
    if (fread(content, 1, *size, file) != *size) {
        printf("错误: 读取文件失败\n");
        free(content);
        fclose(file);
        return NULL;
    }
    
    content[*size] = '\0';
    fclose(file);
    return content;
}

/**
 * 生成简单的ASTC字节码
 */
static uint8_t* generate_simple_bytecode(const char* source_code, size_t source_size, size_t* bytecode_size) {
    // 简单的字节码生成：基于源码内容生成基本指令
    size_t max_bytecode_size = 1024;
    uint8_t* bytecode = malloc(max_bytecode_size);
    if (!bytecode) {
        return NULL;
    }
    
    size_t pos = 0;
    
    // 分析源码并生成字节码
    if (strstr(source_code, "printf")) {
        // 如果包含printf，生成打印指令
        bytecode[pos++] = 0x10;  // CONST_I32
        bytecode[pos++] = 0x00;  // 字符串索引0
        bytecode[pos++] = 0x00;
        bytecode[pos++] = 0x00;
        bytecode[pos++] = 0x00;
        
        bytecode[pos++] = 0x20;  // CALL
        bytecode[pos++] = 0x01;  // printf函数ID
        bytecode[pos++] = 0x00;
        bytecode[pos++] = 0x00;
        bytecode[pos++] = 0x00;
    }
    
    if (strstr(source_code, "return")) {
        // 如果包含return，生成返回指令
        bytecode[pos++] = 0x10;  // CONST_I32
        bytecode[pos++] = 0x00;  // 返回值0
        bytecode[pos++] = 0x00;
        bytecode[pos++] = 0x00;
        bytecode[pos++] = 0x00;
        
        bytecode[pos++] = 0x0F;  // RETURN
    }
    
    // 总是以HALT结束
    bytecode[pos++] = 0x01;  // HALT
    
    *bytecode_size = pos;
    return bytecode;
}

/**
 * 创建ASTC文件
 */
static int create_astc_file(const char* input_file, const char* output_file) {
    printf("simple_c2astc: 转换 %s -> %s\n", input_file, output_file);
    
    // 读取源文件
    size_t source_size;
    char* source_code = read_file(input_file, &source_size);
    if (!source_code) {
        return 1;
    }
    
    printf("simple_c2astc: 读取源文件 (%zu 字节)\n", source_size);
    
    // 生成字节码
    size_t bytecode_size;
    uint8_t* bytecode = generate_simple_bytecode(source_code, source_size, &bytecode_size);
    if (!bytecode) {
        printf("错误: 字节码生成失败\n");
        free(source_code);
        return 1;
    }
    
    printf("simple_c2astc: 生成字节码 (%zu 字节)\n", bytecode_size);
    
    // 创建输出文件
    FILE* output = fopen(output_file, "wb");
    if (!output) {
        printf("错误: 无法创建输出文件 %s\n", output_file);
        free(bytecode);
        free(source_code);
        return 1;
    }
    
    // 写入ASTC头部
    ASTCHeader header;
    memcpy(header.magic, "ASTC", 4);
    header.version = 1;
    header.flags = 0;
    header.entry_point = 0;
    header.source_size = (uint32_t)source_size;
    
    fwrite(&header, sizeof(ASTCHeader), 1, output);
    
    // 写入源码（用于调试）
    fwrite(source_code, 1, source_size, output);
    
    // 写入字节码大小和字节码
    uint32_t bytecode_size_field = (uint32_t)bytecode_size;
    fwrite(&bytecode_size_field, sizeof(uint32_t), 1, output);
    fwrite(bytecode, 1, bytecode_size, output);
    
    fclose(output);
    free(bytecode);
    free(source_code);
    
    printf("simple_c2astc: 成功创建 %s\n", output_file);
    return 0;
}

/**
 * 打印使用说明
 */
static void print_usage(const char* program_name) {
    printf("Simple C to ASTC Converter\n");
    printf("用法: %s <input.c> <output.astc>\n", program_name);
    printf("\n");
    printf("说明:\n");
    printf("  将C源码转换为ASTC字节码格式\n");
    printf("  这是一个简化版本，用于测试三层架构\n");
    printf("\n");
    printf("示例:\n");
    printf("  %s examples/hello_world.c examples/hello_world.astc\n", program_name);
    printf("  %s examples/test_program.c examples/test_program.astc\n", program_name);
}

/**
 * 主函数
 */
int main(int argc, char* argv[]) {
    printf("Simple C to ASTC Converter v1.0\n");
    printf("===============================\n");
    
    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char* input_file = argv[1];
    const char* output_file = argv[2];
    
    // 初始化模块系统（可选，这个工具可以独立工作）
    if (module_system_init() == 0) {
        printf("simple_c2astc: 模块系统已初始化\n");
    } else {
        printf("simple_c2astc: 模块系统初始化失败，使用独立模式\n");
    }
    
    // 执行转换
    int result = create_astc_file(input_file, output_file);
    
    // 清理模块系统
    module_system_cleanup();
    
    return result;
}
