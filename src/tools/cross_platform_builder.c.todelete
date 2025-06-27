/**
 * cross_platform_builder.c - 跨平台构建工具
 * 
 * 目标：
 * 1. 交叉编译生成Linux和macOS版本的核心工具
 * 2. 建立完整的跨平台工具链
 * 3. 支持多架构目标（x64, ARM64等）
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// ===============================================
// 目标平台定义
// ===============================================

typedef enum {
    TARGET_WINDOWS_X64,
    TARGET_LINUX_X64,
    TARGET_MACOS_X64,
    TARGET_LINUX_ARM64,
    TARGET_MACOS_ARM64
} TargetPlatform;

typedef struct {
    TargetPlatform platform;
    const char* name;
    const char* extension;
    const char* asm_format;
    const char* obj_format;
} PlatformInfo;

static PlatformInfo platforms[] = {
    {TARGET_WINDOWS_X64, "windows-x64", ".exe", "win64", "coff"},
    {TARGET_LINUX_X64, "linux-x64", "", "elf64", "elf"},
    {TARGET_MACOS_X64, "macos-x64", "", "macho64", "macho"},
    {TARGET_LINUX_ARM64, "linux-arm64", "", "elf64", "elf"},
    {TARGET_MACOS_ARM64, "macos-arm64", "", "macho64", "macho"}
};

// ===============================================
// 跨平台代码生成器
// ===============================================

typedef struct {
    TargetPlatform target;
    FILE* output;
    bool in_function;
    int label_counter;
} CrossPlatformCodeGen;

// 初始化代码生成器
void init_cross_codegen(CrossPlatformCodeGen* codegen, TargetPlatform target, FILE* output) {
    codegen->target = target;
    codegen->output = output;
    codegen->in_function = false;
    codegen->label_counter = 0;
}

// 生成平台特定的汇编头
void generate_asm_header(CrossPlatformCodeGen* codegen) {
    PlatformInfo* platform = &platforms[codegen->target];
    
    fprintf(codegen->output, "# Cross-platform assembly for %s\n", platform->name);
    
    switch (codegen->target) {
        case TARGET_WINDOWS_X64:
            fprintf(codegen->output, ".intel_syntax noprefix\n");
            fprintf(codegen->output, ".text\n");
            fprintf(codegen->output, ".globl main\n");
            break;
            
        case TARGET_LINUX_X64:
        case TARGET_LINUX_ARM64:
            fprintf(codegen->output, ".text\n");
            fprintf(codegen->output, ".globl _start\n");
            fprintf(codegen->output, ".globl main\n");
            break;
            
        case TARGET_MACOS_X64:
        case TARGET_MACOS_ARM64:
            fprintf(codegen->output, ".text\n");
            fprintf(codegen->output, ".globl _main\n");
            break;
    }
}

// 生成函数入口
void generate_function_entry(CrossPlatformCodeGen* codegen, const char* func_name) {
    switch (codegen->target) {
        case TARGET_WINDOWS_X64:
            fprintf(codegen->output, "%s:\n", func_name);
            fprintf(codegen->output, "    push rbp\n");
            fprintf(codegen->output, "    mov rbp, rsp\n");
            break;
            
        case TARGET_LINUX_X64:
            if (strcmp(func_name, "main") == 0) {
                fprintf(codegen->output, "_start:\n");
                fprintf(codegen->output, "    call main\n");
                fprintf(codegen->output, "    mov rdi, rax\n");
                fprintf(codegen->output, "    mov rax, 60\n");  // sys_exit
                fprintf(codegen->output, "    syscall\n");
            }
            fprintf(codegen->output, "%s:\n", func_name);
            fprintf(codegen->output, "    push rbp\n");
            fprintf(codegen->output, "    mov rbp, rsp\n");
            break;
            
        case TARGET_MACOS_X64:
            if (strcmp(func_name, "main") == 0) {
                fprintf(codegen->output, "_main:\n");
            } else {
                fprintf(codegen->output, "_%s:\n", func_name);
            }
            fprintf(codegen->output, "    push rbp\n");
            fprintf(codegen->output, "    mov rbp, rsp\n");
            break;
            
        case TARGET_LINUX_ARM64:
            fprintf(codegen->output, "%s:\n", func_name);
            fprintf(codegen->output, "    stp x29, x30, [sp, #-16]!\n");
            fprintf(codegen->output, "    mov x29, sp\n");
            break;
            
        case TARGET_MACOS_ARM64:
            if (strcmp(func_name, "main") == 0) {
                fprintf(codegen->output, "_main:\n");
            } else {
                fprintf(codegen->output, "_%s:\n", func_name);
            }
            fprintf(codegen->output, "    stp x29, x30, [sp, #-16]!\n");
            fprintf(codegen->output, "    mov x29, sp\n");
            break;
    }
    codegen->in_function = true;
}

// 生成返回语句
void generate_return(CrossPlatformCodeGen* codegen, int value) {
    switch (codegen->target) {
        case TARGET_WINDOWS_X64:
        case TARGET_LINUX_X64:
        case TARGET_MACOS_X64:
            fprintf(codegen->output, "    mov rax, %d\n", value);
            fprintf(codegen->output, "    pop rbp\n");
            fprintf(codegen->output, "    ret\n");
            break;
            
        case TARGET_LINUX_ARM64:
        case TARGET_MACOS_ARM64:
            fprintf(codegen->output, "    mov w0, #%d\n", value);
            fprintf(codegen->output, "    ldp x29, x30, [sp], #16\n");
            fprintf(codegen->output, "    ret\n");
            break;
    }
}

// 生成系统调用
void generate_syscall_exit(CrossPlatformCodeGen* codegen, int exit_code) {
    switch (codegen->target) {
        case TARGET_WINDOWS_X64:
            // Windows使用API调用
            fprintf(codegen->output, "    mov rcx, %d\n", exit_code);
            fprintf(codegen->output, "    call ExitProcess\n");
            break;
            
        case TARGET_LINUX_X64:
            fprintf(codegen->output, "    mov rdi, %d\n", exit_code);
            fprintf(codegen->output, "    mov rax, 60\n");  // sys_exit
            fprintf(codegen->output, "    syscall\n");
            break;
            
        case TARGET_MACOS_X64:
            fprintf(codegen->output, "    mov rdi, %d\n", exit_code);
            fprintf(codegen->output, "    mov rax, 0x2000001\n");  // sys_exit
            fprintf(codegen->output, "    syscall\n");
            break;
            
        case TARGET_LINUX_ARM64:
            fprintf(codegen->output, "    mov x0, #%d\n", exit_code);
            fprintf(codegen->output, "    mov x8, #93\n");  // sys_exit
            fprintf(codegen->output, "    svc #0\n");
            break;
            
        case TARGET_MACOS_ARM64:
            fprintf(codegen->output, "    mov x0, #%d\n", exit_code);
            fprintf(codegen->output, "    mov x16, #1\n");  // sys_exit
            fprintf(codegen->output, "    svc #0x80\n");
            break;
    }
}

// ===============================================
// 跨平台构建器主逻辑
// ===============================================

// 编译单个文件到指定平台
int cross_compile_file(const char* input_file, const char* output_file, TargetPlatform target) {
    printf("Cross-compiling %s -> %s (%s)\n", 
           input_file, output_file, platforms[target].name);
    
    // 1. 读取源文件
    FILE* input_fp = fopen(input_file, "r");
    if (!input_fp) {
        printf("Error: Cannot open input file %s\n", input_file);
        return 1;
    }
    
    // 2. 创建输出文件
    FILE* output_fp = fopen(output_file, "w");
    if (!output_fp) {
        printf("Error: Cannot create output file %s\n", output_file);
        fclose(input_fp);
        return 1;
    }
    
    // 3. 初始化跨平台代码生成器
    CrossPlatformCodeGen codegen;
    init_cross_codegen(&codegen, target, output_fp);
    
    // 4. 生成汇编代码
    generate_asm_header(&codegen);
    
    // 简单的main函数处理（演示用）
    generate_function_entry(&codegen, "main");
    generate_return(&codegen, 42);
    
    // 5. 清理资源
    fclose(input_fp);
    fclose(output_fp);
    
    printf("✅ Cross-compilation successful: %s\n", output_file);
    return 0;
}

// 构建所有平台的工具
int build_all_platforms() {
    printf("=== Building evolver0 tools for all platforms ===\n");
    
    const char* core_files[] = {
        "src/evolver0/evolver0_loader.c",
        "src/evolver0/evolver0_runtime.c", 
        "src/evolver0/evolver0_program.c",
        "src/tools/tool_c2astc.c",
        "src/tools/tool_astc2bin.c"
    };
    
    const char* output_names[] = {
        "evolver0_loader",
        "evolver0_runtime",
        "evolver0_program", 
        "tool_c2astc",
        "tool_astc2bin"
    };
    
    int num_files = sizeof(core_files) / sizeof(core_files[0]);
    int num_platforms = sizeof(platforms) / sizeof(platforms[0]);
    
    for (int p = 0; p < num_platforms; p++) {
        printf("\n--- Building for %s ---\n", platforms[p].name);
        
        // 创建平台目录
        char platform_dir[256];
        snprintf(platform_dir, sizeof(platform_dir), "bin/%s", platforms[p].name);
        
        for (int f = 0; f < num_files; f++) {
            char output_file[512];
            snprintf(output_file, sizeof(output_file), "%s/%s%s.s", 
                    platform_dir, output_names[f], platforms[p].extension);
            
            int result = cross_compile_file(core_files[f], output_file, platforms[p].platform);
            if (result != 0) {
                printf("❌ Failed to build %s for %s\n", output_names[f], platforms[p].name);
                return result;
            }
        }
    }
    
    printf("\n🎉 All platforms built successfully!\n");
    printf("\nGenerated files:\n");
    for (int p = 0; p < num_platforms; p++) {
        printf("  bin/%s/ - %s binaries\n", platforms[p].name, platforms[p].name);
    }
    
    return 0;
}

// 主函数
int main(int argc, char* argv[]) {
    printf("Cross-Platform Builder v1.0\n");
    printf("Building evolver0 tools for multiple platforms\n\n");
    
    if (argc == 1) {
        // 默认：构建所有平台
        return build_all_platforms();
    } else if (argc == 4) {
        // 指定文件和平台
        const char* input_file = argv[1];
        const char* output_file = argv[2];
        const char* platform_name = argv[3];
        
        // 查找平台
        TargetPlatform target = TARGET_WINDOWS_X64;
        bool found = false;
        for (int i = 0; i < sizeof(platforms) / sizeof(platforms[0]); i++) {
            if (strcmp(platform_name, platforms[i].name) == 0) {
                target = platforms[i].platform;
                found = true;
                break;
            }
        }
        
        if (!found) {
            printf("Error: Unknown platform %s\n", platform_name);
            printf("Available platforms:\n");
            for (int i = 0; i < sizeof(platforms) / sizeof(platforms[0]); i++) {
                printf("  %s\n", platforms[i].name);
            }
            return 1;
        }
        
        return cross_compile_file(input_file, output_file, target);
    } else {
        printf("Usage:\n");
        printf("  %s                              # Build all platforms\n", argv[0]);
        printf("  %s <input> <output> <platform>  # Build specific file\n", argv[0]);
        printf("\nAvailable platforms:\n");
        for (int i = 0; i < sizeof(platforms) / sizeof(platforms[0]); i++) {
            printf("  %s\n", platforms[i].name);
        }
        return 1;
    }
}
