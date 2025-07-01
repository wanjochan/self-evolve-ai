/**
 * loader_common.c - 公共Loader实现
 * 
 * 从evolver0抽象出来的公共组件，实现正确的三层架构
 */

#include "loader.h"
#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ===============================================
// 文件加载函数 (从evolver0抽象)
// ===============================================

void* load_file(const char* filename, size_t* size) {
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
    
    fread(data, 1, *size, file);
    fclose(file);
    
    return data;
}

// ===============================================
// Runtime执行函数 (从evolver0抽象的核心架构)
// ===============================================

int execute_runtime_with_program(void* runtime_data, size_t runtime_size, 
                                 void* program_data, size_t program_size, 
                                 const LoaderOptions* options) {
    
    if (options->verbose) {
        printf("Step 3: Executing Runtime with Program...\n");
        printf("Executing ASTC data: %zu bytes\n", runtime_size);
    }
    
    // 检查Runtime格式
    if (runtime_size >= sizeof(RuntimeHeader) && 
        memcmp(runtime_data, "RTME", 4) == 0) {
        
        RuntimeHeader* runtime_header = (RuntimeHeader*)runtime_data;
        uint32_t version = runtime_header->version;
        uint32_t code_size = runtime_header->size;
        uint32_t entry_offset = runtime_header->entry_point;
        
        if (options->verbose) {
            printf("✓ RTME Runtime detected\n");
            printf("  Version: %u\n", version);
            printf("  Code size: %u bytes\n", code_size);
            printf("  Entry point offset: %u\n", entry_offset);
            printf("Loading Runtime machine code into memory...\n");
        }
        
        // 提取Runtime机器码
        uint8_t* runtime_code = (uint8_t*)runtime_data + entry_offset;
        
        if (options->verbose) {
            printf("Preparing Program data for Runtime...\n");
            printf("Attempting to execute Runtime machine code...\n");
        }
        
        // 使用平台抽象层分配可执行内存
        void* exec_mem = platform_alloc_executable(code_size);
        if (!exec_mem) {
            fprintf(stderr, "Error: Failed to allocate executable memory\n");
            return 1;
        }
        
        // 复制机器码到可执行内存
        memcpy(exec_mem, runtime_code, code_size);
        
        // 创建函数指针
        typedef int (*RuntimeFunc)(void* program_data, size_t program_size);
        RuntimeFunc runtime_func = (RuntimeFunc)exec_mem;
        
        if (options->verbose) {
            printf("Calling Runtime function with Program data...\n");
        }
        
        // 调用Runtime执行Program
        int result = runtime_func(program_data, program_size);
        
        if (options->verbose) {
            printf("Runtime returned: %d\n", result);
        }
        
        // 清理可执行内存
        platform_free_executable(exec_mem, code_size);
        
        if (options->verbose) {
            printf("✓ Pure Three-layer architecture executed successfully!\n");
            printf("Execution result: %d\n", result);
        }
        
        return result;
        
    } else {
        fprintf(stderr, "Error: Invalid Runtime format\n");
        return 1;
    }
}

// ===============================================
// 参数解析函数 (从evolver0抽象)
// ===============================================

bool parse_loader_arguments(int argc, char* argv[], LoaderOptions* options) {
    // 初始化默认值
    options->runtime_file = NULL;
    options->program_file = NULL;
    options->verbose = false;
    options->debug = false;
    
    // 解析参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            options->verbose = true;
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
            options->debug = true;
        } else if (argv[i][0] != '-') {
            if (!options->runtime_file) {
                options->runtime_file = argv[i];
            } else if (!options->program_file) {
                options->program_file = argv[i];
            }
        }
    }
    
    // 验证必需参数
    if (!options->runtime_file || !options->program_file) {
        printf("Usage: %s [options] <runtime.bin> <program.astc>\n", argv[0]);
        printf("Options:\n");
        printf("  -v, --verbose    Verbose output\n");
        printf("  -d, --debug      Debug mode\n");
        return false;
    }
    
    return true;
}
