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
#include "../core/include/core_astc.h"
#include "../core/include/logger.h"
#include "../core/include/astc_program_modules.h"
#include "../compiler/c2astc.h"

#define ASTC_MAGIC "ASTC"
#define ASTC_VERSION 1

typedef struct {
    char magic[4];          // "ASTC"
    uint32_t version;       // 版本号
    uint32_t size;          // 数据大小
    uint32_t entry_point;   // 入口点
} ASTCHeader;

static void print_usage(const char* program_name) {
    printf("Usage: %s [options] <input.c> [output.astc]\n", program_name);
    printf("\nOptions:\n");
    printf("  -h, --help     Show this help message\n");
    printf("  -v, --version  Show version information\n");
    printf("  -o <file>      Specify output file\n");
    printf("  -O0            No optimization (default)\n");
    printf("  -O1            Basic optimization\n");
    printf("  -O2            Advanced optimization\n");
    printf("  -O3            Aggressive optimization\n");
    printf("  -g             Generate debug information\n");
    printf("  -I <dir>       Add include directory\n");
    printf("  -D <macro>     Define preprocessor macro\n");
    printf("  -Wall          Enable all warnings\n");
    printf("  -Werror        Treat warnings as errors\n");
    printf("  -std=c99       Use C99 standard (default)\n");
    printf("  -std=c11       Use C11 standard\n");
    printf("  -c             Compile only (no linking)\n");
    printf("  -S             Generate assembly output\n");
    printf("  -E             Preprocess only\n");
    printf("\nExamples:\n");
    printf("  %s hello.c                    # Compile to evolver0_program.astc\n", program_name);
    printf("  %s hello.c hello.astc         # Compile to hello.astc\n", program_name);
    printf("  %s -o hello.astc hello.c      # Compile with -o option\n", program_name);
    printf("  %s -O2 -g hello.c             # Compile with optimization and debug info\n", program_name);
    printf("  %s -I./include -D DEBUG hello.c  # With include dir and macro\n", program_name);
    printf("  %s -Wall -Werror -std=c99 hello.c  # With warnings and C99 standard\n", program_name);
}

static void print_version(void) {
    printf("tool_c2astc - C to ASTC Compiler v1.0\n");
    printf("Part of Self-Evolve AI C99 Compiler System\n");
    printf("Built: %s %s\n", __DATE__, __TIME__);
}

int main(int argc, char* argv[]) {
    const char* input_file = NULL;
    const char* output_file = NULL;
    C2AstcOptions options = c2astc_default_options();

    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                output_file = argv[++i];
            } else {
                fprintf(stderr, "Error: -o requires output filename\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-O0") == 0) {
            options.optimize_level = 0;
        } else if (strcmp(argv[i], "-O1") == 0) {
            options.optimize_level = 1;
        } else if (strcmp(argv[i], "-O2") == 0) {
            options.optimize_level = 2;
        } else if (strcmp(argv[i], "-O3") == 0) {
            options.optimize_level = 3;
        } else if (strcmp(argv[i], "-g") == 0) {
            options.emit_debug_info = true;
        } else if (strcmp(argv[i], "-I") == 0) {
            if (i + 1 < argc) {
                if (options.include_dir_count < 16) {
                    strncpy(options.include_dirs[options.include_dir_count], argv[++i], 255);
                    options.include_dirs[options.include_dir_count][255] = '\0';
                    options.include_dir_count++;
                    printf("Include directory: %s\n", argv[i]);
                } else {
                    fprintf(stderr, "Error: Too many include directories (max 16)\n");
                    return 1;
                }
            } else {
                fprintf(stderr, "Error: -I requires directory path\n");
                return 1;
            }
        } else if (strncmp(argv[i], "-I", 2) == 0) {
            // Handle -Ipath format
            if (options.include_dir_count < 16) {
                strncpy(options.include_dirs[options.include_dir_count], argv[i] + 2, 255);
                options.include_dirs[options.include_dir_count][255] = '\0';
                options.include_dir_count++;
                printf("Include directory: %s\n", argv[i] + 2);
            } else {
                fprintf(stderr, "Error: Too many include directories (max 16)\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-D") == 0) {
            if (i + 1 < argc) {
                if (options.macro_count < 32) {
                    strncpy(options.macros[options.macro_count], argv[++i], 255);
                    options.macros[options.macro_count][255] = '\0';
                    options.macro_count++;
                    printf("Define macro: %s\n", argv[i]);
                } else {
                    fprintf(stderr, "Error: Too many macro definitions (max 32)\n");
                    return 1;
                }
            } else {
                fprintf(stderr, "Error: -D requires macro definition\n");
                return 1;
            }
        } else if (strncmp(argv[i], "-D", 2) == 0) {
            // Handle -Dmacro format
            if (options.macro_count < 32) {
                strncpy(options.macros[options.macro_count], argv[i] + 2, 255);
                options.macros[options.macro_count][255] = '\0';
                options.macro_count++;
                printf("Define macro: %s\n", argv[i] + 2);
            } else {
                fprintf(stderr, "Error: Too many macro definitions (max 32)\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-Wall") == 0) {
            options.enable_warnings = true;
        } else if (strcmp(argv[i], "-Werror") == 0) {
            options.warnings_as_errors = true;
        } else if (strcmp(argv[i], "-std=c99") == 0) {
            options.c_standard = C_STD_C99;
        } else if (strcmp(argv[i], "-std=c11") == 0) {
            options.c_standard = C_STD_C11;
        } else if (strcmp(argv[i], "-c") == 0) {
            options.compile_only = true;
        } else if (strcmp(argv[i], "-S") == 0) {
            options.generate_assembly = true;
        } else if (strcmp(argv[i], "-E") == 0) {
            options.preprocess_only = true;
        } else if (strcmp(argv[i], "-w") == 0) {
            options.enable_warnings = false;
        } else if (strcmp(argv[i], "-Wextra") == 0) {
            options.enable_warnings = true;
            // 可以在这里添加额外的警告选项
        } else if (strcmp(argv[i], "-pedantic") == 0) {
            options.enable_warnings = true;
            // 严格模式
        } else if (strcmp(argv[i], "-fPIC") == 0) {
            // 位置无关代码，暂时忽略
        } else if (strcmp(argv[i], "-shared") == 0) {
            // 共享库，暂时忽略
        } else if (strcmp(argv[i], "-static") == 0) {
            // 静态链接，暂时忽略
        } else if (argv[i][0] != '-') {
            if (!input_file) {
                input_file = argv[i];
            } else if (!output_file) {
                output_file = argv[i];
            } else {
                fprintf(stderr, "Error: Too many input files\n");
                print_usage(argv[0]);
                return 1;
            }
        } else {
            fprintf(stderr, "Error: Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    // 检查必要参数
    if (!input_file) {
        fprintf(stderr, "Error: No input file specified\n");
        print_usage(argv[0]);
        return 1;
    }

    // 设置默认输出文件
    if (!output_file) {
        output_file = "evolver0_program.astc";
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
    if (options.enable_warnings) {
        printf("Warnings: enabled\n");
    }
    if (options.warnings_as_errors) {
        printf("Warnings as errors: enabled\n");
    }
    if (options.c_standard == C_STD_C11) {
        printf("C standard: C11\n");
    } else {
        printf("C standard: C99\n");
    }
    if (options.include_dir_count > 0) {
        printf("Include directories: %d\n", options.include_dir_count);
    }
    if (options.macro_count > 0) {
        printf("Macro definitions: %d\n", options.macro_count);
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
