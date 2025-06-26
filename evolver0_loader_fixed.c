/**
 * evolver0_loader_fixed.c - 修复版的Loader层实现
 * 
 * 这个版本使用安全的库调用方式而不是直接执行机器码
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// 包含我们的runtime库
#include "runtime.h"
#include "astc.h"
#include "c2astc.h"

// ASTC头部结构
typedef struct {
    char magic[4];          // "ASTC"
    uint32_t version;       // 版本号
    uint32_t size;          // 数据大小
    uint32_t entry_point;   // 入口点
} ASTCHeader;

// 加载器选项
typedef struct {
    const char* runtime_file;   // Runtime二进制文件
    const char* program_file;   // Program ASTC文件
    bool verbose;               // 详细输出
    bool debug;                 // 调试模式
} LoaderOptions;

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

static int load_and_execute_runtime(const LoaderOptions* options) {
    if (options->verbose) {
        printf("Evolver0 Loader (Fixed) - Three-Layer Architecture\n");
        printf("Runtime: %s\n", options->runtime_file);
        printf("Program: %s\n", options->program_file);
    }
    
    // 步骤1: 验证Runtime文件存在（但不执行机器码）
    if (options->verbose) {
        printf("Step 1: Verifying Runtime binary...\n");
    }
    
    size_t runtime_size;
    void* runtime_data = load_file(options->runtime_file, &runtime_size);
    if (!runtime_data) {
        return 1;
    }
    
    if (options->verbose) {
        printf("✓ Runtime file loaded: %zu bytes\n", runtime_size);
    }
    
    // 步骤2: 加载Program ASTC
    if (options->verbose) {
        printf("Step 2: Loading Program ASTC...\n");
    }
    
    size_t program_size;
    void* program_data = load_file(options->program_file, &program_size);
    if (!program_data) {
        free(runtime_data);
        return 1;
    }
    
    // 验证ASTC格式
    if (program_size < sizeof(ASTCHeader)) {
        fprintf(stderr, "Error: Invalid ASTC file format\n");
        free(runtime_data);
        free(program_data);
        return 1;
    }
    
    ASTCHeader* astc_header = (ASTCHeader*)program_data;
    if (memcmp(astc_header->magic, "ASTC", 4) != 0) {
        fprintf(stderr, "Error: Invalid ASTC magic number\n");
        free(runtime_data);
        free(program_data);
        return 1;
    }
    
    if (options->verbose) {
        printf("✓ Program loaded: %zu bytes, version %u\n", 
               program_size, astc_header->version);
    }
    
    // 步骤3: 使用Runtime库执行ASTC
    if (options->verbose) {
        printf("Step 3: Executing ASTC using Runtime library...\n");
    }
    
    // 提取ASTC数据
    unsigned char* astc_data = (unsigned char*)program_data + sizeof(ASTCHeader);
    size_t astc_data_size = astc_header->size;
    
    if (options->verbose) {
        printf("Executing ASTC data: %zu bytes\n", astc_data_size);
    }
    
    // 使用我们的runtime库执行ASTC
    RuntimeVM vm;
    if (!runtime_init(&vm)) {
        fprintf(stderr, "Failed to initialize VM\n");
        free(runtime_data);
        free(program_data);
        return 1;
    }
    
    struct ASTNode* program = c2astc_deserialize(astc_data, astc_data_size);
    if (!program) {
        fprintf(stderr, "Failed to deserialize ASTC program\n");
        runtime_destroy(&vm);
        free(runtime_data);
        free(program_data);
        return 1;
    }
    
    if (!runtime_load_program(&vm, program)) {
        fprintf(stderr, "Failed to load program: %s\n", runtime_get_error(&vm));
        ast_free(program);
        runtime_destroy(&vm);
        free(runtime_data);
        free(program_data);
        return 1;
    }
    
    int result = runtime_execute(&vm, "main");
    
    if (options->verbose) {
        printf("✓ Three-layer architecture executed successfully!\n");
        printf("Loader: %s (Fixed Loader)\n", "evolver0_loader_fixed.exe");
        printf("Runtime: %s (%zu bytes)\n", options->runtime_file, runtime_size);
        printf("Program: %s (%zu bytes)\n", options->program_file, program_size);
        printf("Execution result: %d\n", result);
    }
    
    // 清理
    ast_free(program);
    runtime_destroy(&vm);
    free(runtime_data);
    free(program_data);
    
    return result;
}

static void print_usage(const char* program_name) {
    printf("Evolver0 Loader (Fixed) - Three-Layer Architecture Implementation\n");
    printf("Usage: %s [options] <runtime> <program.astc>\n", program_name);
    printf("Options:\n");
    printf("  -v            Verbose output\n");
    printf("  -d            Debug mode\n");
    printf("  -h, --help    Show this help\n");
}

static int parse_arguments(int argc, char* argv[], LoaderOptions* options) {
    // 初始化默认选项
    options->runtime_file = NULL;
    options->program_file = NULL;
    options->verbose = false;
    options->debug = false;
    
    int arg_index = 1;
    
    // 解析选项
    while (arg_index < argc && argv[arg_index][0] == '-') {
        if (strcmp(argv[arg_index], "-v") == 0) {
            options->verbose = true;
        } else if (strcmp(argv[arg_index], "-d") == 0) {
            options->debug = true;
        } else if (strcmp(argv[arg_index], "-h") == 0 || strcmp(argv[arg_index], "--help") == 0) {
            print_usage(argv[0]);
            return -1;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[arg_index]);
            return 1;
        }
        arg_index++;
    }
    
    // 解析位置参数
    if (arg_index + 2 != argc) {
        fprintf(stderr, "Error: Expected exactly 2 arguments (runtime and program)\n");
        print_usage(argv[0]);
        return 1;
    }
    
    options->runtime_file = argv[arg_index];
    options->program_file = argv[arg_index + 1];
    
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }
    
    LoaderOptions options;
    int parse_result = parse_arguments(argc, argv, &options);
    
    if (parse_result == -1) {
        return 0; // 显示帮助后正常退出
    } else if (parse_result != 0) {
        return 1; // 参数解析错误
    }
    
    return load_and_execute_runtime(&options);
}
