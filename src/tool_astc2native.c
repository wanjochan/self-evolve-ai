/**
 * tool_astc2native.c - ASTC到Native模块转换工具
 *
 * 将ASTC文件转译为.native模块格式（.rt格式，纯机器码+元数据，无OS文件头）
 * 流程: program.astc → (JIT编译) → vm_x64_64.native
 * 符合PRD.md架构：loader.exe → vm_{arch}_{bits}.native → program.astc
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "runtime/astc2native.h"

static void print_usage(const char* program_name) {
    printf("Usage: %s <input_file> <output_file>\n\n", program_name);
    printf("Options:\n");
    printf("  -c                     Treat input as C source file instead of ASTC\n");
    printf("  -help                  Display this help message\n");
    printf("\nExamples:\n");
    printf("  %s program.astc vm_x64_64.native      # Convert ASTC to native module\n", program_name);
    printf("  %s -c runtime.c vm_x64_64.native      # Convert C to native module\n", program_name);
    printf("\nSupported architectures: x86_64, arm64, x86_32, arm32\n");
    printf("Output format: .native (.rt format - pure machine code + metadata, no OS headers)\n");
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

    printf("ASTC to Native Module Converter v1.0\n");
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
