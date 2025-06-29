/**
 * tool_astc2rt.c - ASTC到Runtime转换工具
 *
 * 将ASTC文件转译为轻量化的.rt Runtime格式
 * 流程: runtime.astc → (代码生成) → runtime{arch}{bits}.rt
 * 符合PRD.md新规范：专注于libc转发封装的轻量化设计
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "runtime/compiler_astc2rt.h"

static void print_usage(const char* program_name) {
    printf("Usage: %s <input_file> <output_file>\n\n", program_name);
    printf("Options:\n");
    printf("  -c                     Treat input as C source file instead of ASTC\n");
    printf("  -help                  Display this help message\n");
    printf("\nExamples:\n");
    printf("  %s runtime.astc runtime_x64_64.rt     # Convert ASTC to RT\n", program_name);
    printf("  %s -c runtime.c runtime_x64_64.rt     # Convert C to RT directly\n", program_name);
}

int main(int argc, char* argv[]) {
    int i;
    int is_c_file = 0;
    const char* input_file = NULL;
    const char* output_file = NULL;

    // 解析命令行参数
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0) {
            is_c_file = 1;
        } else if (strcmp(argv[i], "-help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (input_file == NULL) {
            input_file = argv[i];
        } else if (output_file == NULL) {
            output_file = argv[i];
        } else {
            printf("Error: Too many arguments\n");
            print_usage(argv[0]);
            return 1;
        }
    }

    // 检查必要参数
    if (!input_file || !output_file) {
        printf("Error: Missing required arguments\n");
        print_usage(argv[0]);
        return 1;
    }

    printf("ASTC to Runtime Converter v1.0\n");
    printf("Input file: %s\n", input_file);
    printf("Output file: %s\n", output_file);
    printf("Input type: %s\n", is_c_file ? "C source" : "ASTC binary");

    // 检测并显示目标架构
    TargetArch target_arch = detect_runtime_architecture();
    printf("Target architecture: %s\n", get_architecture_name(target_arch));

    int result;
    if (is_c_file) {
        result = compile_c_to_runtime_bin(input_file, output_file);
    } else {
        result = compile_astc_to_runtime_bin(input_file, output_file);
    }

    if (result == 0) {
        printf("Conversion completed successfully.\n");
    } else {
        printf("Conversion failed.\n");
    }

    return result;
}
