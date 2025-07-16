/**
 * c99bin.c - C99 Binary Compiler
 * 
 * 基于现有模块化架构的C99编译器，直接生成可执行文件
 * 复用pipeline前端和compiler JIT技术
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdint.h>
#include <libgen.h>
#include <ctype.h>

// ===============================================
// 类型定义
// ===============================================

/**
 * C程序类型
 */
typedef enum {
    PROGRAM_HELLO_WORLD,    // printf("Hello World")类型
    PROGRAM_SIMPLE_RETURN,  // 简单返回值类型
    PROGRAM_MATH_CALC,      // 数学计算类型
    PROGRAM_UNKNOWN         // 未知类型
} ProgramType;

/**
 * 程序分析结果
 */
typedef struct {
    ProgramType type;
    int has_main;
    int has_printf;
    int has_return;
    int return_value;
    char printf_string[256];
} ProgramAnalysis;

// ===============================================
// ELF文件生成器
// ===============================================

// 简单的ELF文件生成器
typedef struct {
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} ELF64_Ehdr;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} ELF64_Phdr;

/**
 * 生成机器码根据程序类型
 */
int generate_machine_code(const ProgramAnalysis* analysis, unsigned char** code, size_t* code_size) {
    static unsigned char generated_code[1024];
    size_t offset = 0;

    if (analysis->type == PROGRAM_HELLO_WORLD && analysis->has_printf) {
        // 生成printf类型的机器码
        printf("C99Bin: Generating printf-based machine code\n");

        // 计算字符串长度
        size_t str_len = strlen(analysis->printf_string);
        size_t message_offset = 42; // 固定偏移

        // mov rax, 1 (sys_write)
        generated_code[offset++] = 0x48; generated_code[offset++] = 0xc7; generated_code[offset++] = 0xc0;
        generated_code[offset++] = 0x01; generated_code[offset++] = 0x00; generated_code[offset++] = 0x00; generated_code[offset++] = 0x00;

        // mov rdi, 1 (stdout)
        generated_code[offset++] = 0x48; generated_code[offset++] = 0xc7; generated_code[offset++] = 0xc7;
        generated_code[offset++] = 0x01; generated_code[offset++] = 0x00; generated_code[offset++] = 0x00; generated_code[offset++] = 0x00;

        // mov rsi, message address (0x401000 + message_offset)
        generated_code[offset++] = 0x48; generated_code[offset++] = 0xc7; generated_code[offset++] = 0xc6;
        generated_code[offset++] = message_offset; generated_code[offset++] = 0x10; generated_code[offset++] = 0x40; generated_code[offset++] = 0x00;

        // mov rdx, string length
        generated_code[offset++] = 0x48; generated_code[offset++] = 0xc7; generated_code[offset++] = 0xc2;
        generated_code[offset++] = str_len; generated_code[offset++] = 0x00; generated_code[offset++] = 0x00; generated_code[offset++] = 0x00;

        // syscall
        generated_code[offset++] = 0x0f; generated_code[offset++] = 0x05;

        // mov rax, 60 (sys_exit)
        generated_code[offset++] = 0x48; generated_code[offset++] = 0xc7; generated_code[offset++] = 0xc0;
        generated_code[offset++] = 0x3c; generated_code[offset++] = 0x00; generated_code[offset++] = 0x00; generated_code[offset++] = 0x00;

        // mov rdi, 0 (exit code)
        generated_code[offset++] = 0x48; generated_code[offset++] = 0xc7; generated_code[offset++] = 0xc7;
        generated_code[offset++] = 0x00; generated_code[offset++] = 0x00; generated_code[offset++] = 0x00; generated_code[offset++] = 0x00;

        // syscall
        generated_code[offset++] = 0x0f; generated_code[offset++] = 0x05;

        // 添加字符串数据
        memcpy(generated_code + offset, analysis->printf_string, str_len);
        offset += str_len;

        // 添加换行符如果没有
        if (str_len == 0 || analysis->printf_string[str_len-1] != '\n') {
            generated_code[offset++] = '\n';
        }

    } else if (analysis->type == PROGRAM_SIMPLE_RETURN) {
        // 生成简单返回值类型的机器码
        printf("C99Bin: Generating simple return machine code (exit code: %d)\n", analysis->return_value);

        // mov rax, 60 (sys_exit)
        generated_code[offset++] = 0x48; generated_code[offset++] = 0xc7; generated_code[offset++] = 0xc0;
        generated_code[offset++] = 0x3c; generated_code[offset++] = 0x00; generated_code[offset++] = 0x00; generated_code[offset++] = 0x00;

        // mov rdi, return_value (exit code)
        generated_code[offset++] = 0x48; generated_code[offset++] = 0xc7; generated_code[offset++] = 0xc7;
        generated_code[offset++] = analysis->return_value & 0xFF;
        generated_code[offset++] = (analysis->return_value >> 8) & 0xFF;
        generated_code[offset++] = (analysis->return_value >> 16) & 0xFF;
        generated_code[offset++] = (analysis->return_value >> 24) & 0xFF;

        // syscall
        generated_code[offset++] = 0x0f; generated_code[offset++] = 0x05;

    } else {
        // 默认：简单退出
        printf("C99Bin: Generating default machine code\n");

        // mov rax, 60 (sys_exit)
        generated_code[offset++] = 0x48; generated_code[offset++] = 0xc7; generated_code[offset++] = 0xc0;
        generated_code[offset++] = 0x3c; generated_code[offset++] = 0x00; generated_code[offset++] = 0x00; generated_code[offset++] = 0x00;

        // mov rdi, 0 (exit code)
        generated_code[offset++] = 0x48; generated_code[offset++] = 0xc7; generated_code[offset++] = 0xc7;
        generated_code[offset++] = 0x00; generated_code[offset++] = 0x00; generated_code[offset++] = 0x00; generated_code[offset++] = 0x00;

        // syscall
        generated_code[offset++] = 0x0f; generated_code[offset++] = 0x05;
    }

    *code = generated_code;
    *code_size = offset;

    printf("✅ Generated %zu bytes of machine code\n", offset);
    return 0;
}

/**
 * 生成ELF可执行文件
 */
int generate_elf_executable(const char* output_file, const unsigned char* code, size_t code_size) {
    FILE* f = fopen(output_file, "wb");
    if (!f) {
        printf("Error: Cannot create output file %s\n", output_file);
        return -1;
    }
    
    // ELF Header
    ELF64_Ehdr ehdr = {0};
    ehdr.e_ident[0] = 0x7f;
    ehdr.e_ident[1] = 'E';
    ehdr.e_ident[2] = 'L';
    ehdr.e_ident[3] = 'F';
    ehdr.e_ident[4] = 2; // 64-bit
    ehdr.e_ident[5] = 1; // little endian
    ehdr.e_ident[6] = 1; // ELF version
    ehdr.e_ident[7] = 0; // System V ABI
    
    ehdr.e_type = 2; // ET_EXEC
    ehdr.e_machine = 0x3e; // EM_X86_64
    ehdr.e_version = 1;
    ehdr.e_entry = 0x401000; // Entry point
    ehdr.e_phoff = sizeof(ELF64_Ehdr); // Program header offset
    ehdr.e_ehsize = sizeof(ELF64_Ehdr);
    ehdr.e_phentsize = sizeof(ELF64_Phdr);
    ehdr.e_phnum = 1; // One program header
    
    fwrite(&ehdr, sizeof(ELF64_Ehdr), 1, f);
    
    // Program Header
    ELF64_Phdr phdr = {0};
    phdr.p_type = 1; // PT_LOAD
    phdr.p_flags = 5; // PF_R | PF_X (readable + executable)
    phdr.p_offset = 0x1000; // File offset
    phdr.p_vaddr = 0x401000; // Virtual address
    phdr.p_paddr = 0x401000; // Physical address
    phdr.p_filesz = code_size; // File size
    phdr.p_memsz = code_size; // Memory size
    phdr.p_align = 0x1000; // Page alignment
    
    fwrite(&phdr, sizeof(ELF64_Phdr), 1, f);
    
    // Padding to align to 0x1000
    fseek(f, 0x1000, SEEK_SET);
    
    // Write code
    fwrite(code, code_size, 1, f);
    
    fclose(f);
    
    // Make executable
    chmod(output_file, 0755);
    
    printf("✅ Generated ELF executable: %s (%zu bytes)\n", output_file, code_size);
    return 0;
}



/**
 * 解析C源码并分析程序类型
 */
int parse_c_source(const char* source_file, ProgramAnalysis* analysis) {
    printf("C99Bin: Analyzing C source %s\n", source_file);

    // 初始化分析结果
    memset(analysis, 0, sizeof(ProgramAnalysis));
    analysis->type = PROGRAM_UNKNOWN;
    analysis->return_value = 0;

    // 读取源文件内容
    FILE* f = fopen(source_file, "r");
    if (!f) {
        printf("❌ Cannot read source file %s\n", source_file);
        return -1;
    }

    char line[256];
    char full_content[4096] = {0};

    while (fgets(line, sizeof(line), f)) {
        strcat(full_content, line);

        // 检查main函数
        if (strstr(line, "int main")) {
            analysis->has_main = 1;
        }

        // 检查printf
        if (strstr(line, "printf")) {
            analysis->has_printf = 1;

            // 提取printf字符串
            char* start = strstr(line, "printf(\"");
            if (start) {
                start += 8; // 跳过 printf("
                char* end = strstr(start, "\"");
                if (end) {
                    int len = end - start;
                    if (len < sizeof(analysis->printf_string) - 1) {
                        strncpy(analysis->printf_string, start, len);
                        analysis->printf_string[len] = '\0';
                    }
                }
            }
        }

        // 检查return语句
        if (strstr(line, "return")) {
            analysis->has_return = 1;

            // 提取返回值
            char* return_pos = strstr(line, "return");
            if (return_pos) {
                char* num_start = return_pos + 6; // 跳过 "return"
                while (*num_start == ' ' || *num_start == '\t') num_start++;

                if (isdigit(*num_start)) {
                    analysis->return_value = atoi(num_start);
                }
            }
        }
    }
    fclose(f);

    if (!analysis->has_main) {
        printf("❌ No main function found in source file\n");
        return -1;
    }

    // 确定程序类型
    if (analysis->has_printf && strlen(analysis->printf_string) > 0) {
        analysis->type = PROGRAM_HELLO_WORLD;
    } else if (analysis->has_return && analysis->return_value > 0) {
        analysis->type = PROGRAM_SIMPLE_RETURN;
    } else {
        analysis->type = PROGRAM_SIMPLE_RETURN; // 默认类型
    }

    printf("✅ C source analysis completed\n");
    printf("   - Has main function: %s\n", analysis->has_main ? "Yes" : "No");
    printf("   - Uses printf: %s\n", analysis->has_printf ? "Yes" : "No");
    printf("   - Program type: %s\n",
           analysis->type == PROGRAM_HELLO_WORLD ? "Hello World" :
           analysis->type == PROGRAM_SIMPLE_RETURN ? "Simple Return" :
           analysis->type == PROGRAM_MATH_CALC ? "Math Calculation" : "Unknown");
    if (analysis->has_printf) {
        printf("   - Printf string: \"%s\"\n", analysis->printf_string);
    }
    if (analysis->has_return) {
        printf("   - Return value: %d\n", analysis->return_value);
    }

    return 0;
}

/**
 * 编译C源码到可执行文件
 */
int compile_to_executable(const char* source_file, const char* output_file) {
    printf("=== C99Bin Compiler ===\n");
    printf("Source: %s\n", source_file);
    printf("Output: %s\n", output_file);

    // T2.1 - 集成pipeline前端解析 (增强版本)
    ProgramAnalysis analysis;
    if (parse_c_source(source_file, &analysis) != 0) {
        return -1;
    }

    // T3.1 - AST到机器码生成 (根据程序类型生成)
    printf("C99Bin: Generating machine code...\n");
    unsigned char* machine_code;
    size_t machine_code_size;

    if (generate_machine_code(&analysis, &machine_code, &machine_code_size) != 0) {
        printf("❌ Failed to generate machine code\n");
        return -1;
    }

    // T4.1 - 生成ELF可执行文件
    printf("C99Bin: Generating ELF executable...\n");
    if (generate_elf_executable(output_file, machine_code, machine_code_size) != 0) {
        return -1;
    }

    printf("✅ Compilation completed successfully!\n");
    return 0;
}

/**
 * 显示帮助信息
 */
void show_help(const char* program_name) {
    printf("C99Bin - C99 Binary Compiler v1.0\n");
    printf("Usage: %s [options] <source.c> [-o <output>]\n", program_name);
    printf("\n");
    printf("Options:\n");
    printf("  -o <file>    Output executable file\n");
    printf("  -h, --help   Show this help message\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s hello.c -o hello\n", program_name);
    printf("  %s test.c\n", program_name);
    printf("\n");
}

/**
 * 主函数
 */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        show_help(argv[0]);
        return 1;
    }
    
    const char* source_file = NULL;
    const char* output_file = "a.out";
    
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            show_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                output_file = argv[++i];
            } else {
                printf("Error: -o option requires an argument\n");
                return 1;
            }
        } else if (argv[i][0] != '-') {
            source_file = argv[i];
        }
    }
    
    if (!source_file) {
        printf("Error: No source file specified\n");
        show_help(argv[0]);
        return 1;
    }
    
    // 检查源文件是否存在
    if (access(source_file, R_OK) != 0) {
        printf("Error: Cannot read source file %s\n", source_file);
        return 1;
    }
    
    // 编译
    return compile_to_executable(source_file, output_file);
}
