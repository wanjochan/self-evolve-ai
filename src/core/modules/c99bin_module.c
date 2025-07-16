/**
 * c99bin_module.c - C99 Binary Compiler Module
 * 
 * C99二进制编译器模块，采用JIT技术直接生成可执行文件：
 * - 复用pipeline前端: C源码 -> AST (c2astc)
 * - 复用compiler JIT: AST -> 机器码 (JIT技术)
 * - 新增AOT编译: 机器码 -> 可执行文件 (ELF/PE)
 * - 绕过ASTC中间表示，直接处理AST
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../module.h"
#include "pipeline_common.h"

// 前向声明 - 避免循环依赖
extern Module* load_module(const char* path);

// ===============================================
// C99bin编译器状态
// ===============================================

typedef enum {
    C99BIN_SUCCESS = 0,
    C99BIN_ERROR_INVALID_INPUT = 1,
    C99BIN_ERROR_PARSE_FAILED = 2,
    C99BIN_ERROR_CODEGEN_FAILED = 3,
    C99BIN_ERROR_LINK_FAILED = 4,
    C99BIN_ERROR_FILE_IO = 5,
    C99BIN_ERROR_MEMORY_ALLOC = 6
} C99BinResult;

typedef struct {
    // 依赖模块
    Module* pipeline_module;    // 复用前端解析
    Module* compiler_module;    // 复用JIT技术
    Module* layer0_module;      // 复用基础服务
    
    // 编译状态
    bool initialized;
    char* source_code;
    ASTNode* ast_root;
    uint8_t* machine_code;
    size_t machine_code_size;
    
    // 错误信息
    char error_message[512];
} C99BinState;

static C99BinState c99bin_state = {0};

// ===============================================
// 前向声明
// ===============================================

C99BinResult c99bin_generate_elf(const uint8_t* machine_code, size_t code_size, const char* output_file);
C99BinResult c99bin_generate_pe(const uint8_t* machine_code, size_t code_size, const char* output_file);
C99BinResult c99bin_compile_to_object(const char* source_file, uint8_t** object_code, size_t* object_size);
C99BinResult c99bin_link_objects(uint8_t** object_codes, size_t* object_sizes, int object_count, const char* output_file);

// ===============================================
// 多文件编译支持结构
// ===============================================

typedef struct {
    char** source_files;           // 源文件列表
    int source_count;              // 源文件数量
    ASTNode** ast_nodes;           // 对应的AST节点
    uint8_t** object_codes;        // 对应的目标代码
    size_t* object_sizes;          // 目标代码大小
    char** object_files;           // 目标文件路径 (.o文件)
} MultiFileProject;

static MultiFileProject multi_project = {0};

// ===============================================
// 核心编译接口
// ===============================================

/**
 * 编译C源码到可执行文件 (单文件版本)
 * @param source_file C源码文件路径
 * @param output_file 输出可执行文件路径
 * @return C99BinResult 编译结果
 */
C99BinResult c99bin_compile_to_executable(const char* source_file, const char* output_file) {
    if (!source_file || !output_file) {
        strcpy(c99bin_state.error_message, "Invalid input parameters");
        return C99BIN_ERROR_INVALID_INPUT;
    }

    printf("C99Bin Module: Compiling %s to %s\n", source_file, output_file);

    // T2.1.1 - 集成pipeline前端解析 (完整实现)
    printf("C99Bin Module: Phase 1 - Loading source file...\n");

    // 读取源文件
    FILE* source_fp = fopen(source_file, "r");
    if (!source_fp) {
        snprintf(c99bin_state.error_message, sizeof(c99bin_state.error_message),
                "Cannot open source file: %s", source_file);
        return C99BIN_ERROR_FILE_IO;
    }

    // 获取文件大小
    fseek(source_fp, 0, SEEK_END);
    long source_size = ftell(source_fp);
    fseek(source_fp, 0, SEEK_SET);

    // 分配内存并读取源码
    char* source_code = malloc(source_size + 1);
    if (!source_code) {
        fclose(source_fp);
        strcpy(c99bin_state.error_message, "Memory allocation failed for source code");
        return C99BIN_ERROR_MEMORY_ALLOC;
    }

    fread(source_code, 1, source_size, source_fp);
    source_code[source_size] = '\0';
    fclose(source_fp);

    printf("C99Bin Module: Source loaded (%ld bytes)\n", source_size);
    c99bin_state.source_code = source_code;

    // T2.1.2 - 使用pipeline前端进行词法和语法分析
    if (c99bin_state.pipeline_module) {
        printf("C99Bin Module: Phase 2 - Using pipeline frontend for parsing...\n");

        // 调用pipeline模块的前端编译函数
        ASTNode* (*frontend_compile)(const char*) =
            (ASTNode* (*)(const char*))c99bin_state.pipeline_module->sym(c99bin_state.pipeline_module, "frontend_compile");

        if (frontend_compile) {
            printf("C99Bin Module: Found frontend_compile function, parsing AST...\n");
            c99bin_state.ast_root = frontend_compile(source_code);

            if (c99bin_state.ast_root) {
                printf("C99Bin Module: ✅ AST generation successful\n");
            } else {
                strcpy(c99bin_state.error_message, "Frontend parsing failed - invalid C syntax");
                c99bin_state.source_code = NULL;  // 避免double free
                free(source_code);
                return C99BIN_ERROR_PARSE_FAILED;
            }
        } else {
            printf("C99Bin Module: ⚠️ frontend_compile function not found, using fallback\n");
            strcpy(c99bin_state.error_message, "Pipeline frontend not available");
            c99bin_state.source_code = NULL;  // 避免double free
            free(source_code);
            return C99BIN_ERROR_PARSE_FAILED;
        }
    } else {
        printf("C99Bin Module: ❌ Pipeline module not available\n");
        strcpy(c99bin_state.error_message, "Pipeline module dependency missing");
        c99bin_state.source_code = NULL;  // 避免double free
        free(source_code);
        return C99BIN_ERROR_PARSE_FAILED;
    }

    // T2.1.3 - 使用compiler模块进行AST到机器码生成
    if (c99bin_state.compiler_module) {
        printf("C99Bin Module: Phase 3 - Using compiler JIT for code generation...\n");

        // 查找JIT编译函数
        int (*jit_compile_ast)(ASTNode*, uint8_t**, size_t*) =
            (int (*)(ASTNode*, uint8_t**, size_t*))c99bin_state.compiler_module->sym(c99bin_state.compiler_module, "jit_compile_ast");

        if (jit_compile_ast) {
            printf("C99Bin Module: Found jit_compile_ast function, generating machine code...\n");

            int result = jit_compile_ast(c99bin_state.ast_root,
                                       &c99bin_state.machine_code,
                                       &c99bin_state.machine_code_size);

            if (result == 0 && c99bin_state.machine_code && c99bin_state.machine_code_size > 0) {
                printf("C99Bin Module: ✅ Machine code generation successful (%zu bytes)\n",
                       c99bin_state.machine_code_size);
            } else {
                strcpy(c99bin_state.error_message, "JIT compilation failed - unsupported AST structure");
                return C99BIN_ERROR_CODEGEN_FAILED;
            }
        } else {
            printf("C99Bin Module: ⚠️ jit_compile_ast function not found, trying alternative...\n");

            // 尝试查找通用的JIT编译函数
            int (*jit_compile)(void*, uint8_t**, size_t*) =
                (int (*)(void*, uint8_t**, size_t*))c99bin_state.compiler_module->sym(c99bin_state.compiler_module, "jit_compile");

            if (jit_compile) {
                printf("C99Bin Module: Using generic jit_compile function...\n");
                int result = jit_compile(c99bin_state.ast_root,
                                       &c99bin_state.machine_code,
                                       &c99bin_state.machine_code_size);

                if (result == 0 && c99bin_state.machine_code) {
                    printf("C99Bin Module: ✅ Generic JIT compilation successful (%zu bytes)\n",
                           c99bin_state.machine_code_size);
                } else {
                    strcpy(c99bin_state.error_message, "Generic JIT compilation failed");
                    return C99BIN_ERROR_CODEGEN_FAILED;
                }
            } else {
                strcpy(c99bin_state.error_message, "No suitable JIT compiler function found");
                return C99BIN_ERROR_CODEGEN_FAILED;
            }
        }
    } else {
        printf("C99Bin Module: ❌ Compiler module not available\n");
        strcpy(c99bin_state.error_message, "Compiler module dependency missing");
        return C99BIN_ERROR_CODEGEN_FAILED;
    }

    // T2.1.4 - 实现ELF文件格式生成
    printf("C99Bin Module: Phase 4 - Generating ELF executable...\n");

    C99BinResult elf_result = c99bin_generate_elf(c99bin_state.machine_code,
                                                  c99bin_state.machine_code_size,
                                                  output_file);

    if (elf_result == C99BIN_SUCCESS) {
        printf("C99Bin Module: ✅ ELF executable generated successfully: %s\n", output_file);
        return C99BIN_SUCCESS;
    } else {
        printf("C99Bin Module: ❌ ELF generation failed: %s\n", c99bin_state.error_message);
        return elf_result;
    }
}

/**
 * 编译单个源文件到目标代码 (不生成可执行文件)
 * @param source_file C源码文件路径
 * @param object_code 输出的目标代码指针
 * @param object_size 输出的目标代码大小
 * @return C99BinResult 编译结果
 */
C99BinResult c99bin_compile_to_object(const char* source_file, uint8_t** object_code, size_t* object_size) {
    if (!source_file || !object_code || !object_size) {
        strcpy(c99bin_state.error_message, "Invalid input parameters for object compilation");
        return C99BIN_ERROR_INVALID_INPUT;
    }

    printf("C99Bin Module: Compiling %s to object code...\n", source_file);

    // 读取源文件
    FILE* source_fp = fopen(source_file, "r");
    if (!source_fp) {
        snprintf(c99bin_state.error_message, sizeof(c99bin_state.error_message),
                "Cannot open source file: %s", source_file);
        return C99BIN_ERROR_FILE_IO;
    }

    // 获取文件大小
    fseek(source_fp, 0, SEEK_END);
    long source_size = ftell(source_fp);
    fseek(source_fp, 0, SEEK_SET);

    // 分配内存并读取源码
    char* source_code = malloc(source_size + 1);
    if (!source_code) {
        fclose(source_fp);
        strcpy(c99bin_state.error_message, "Memory allocation failed for source code");
        return C99BIN_ERROR_MEMORY_ALLOC;
    }

    fread(source_code, 1, source_size, source_fp);
    source_code[source_size] = '\0';
    fclose(source_fp);

    // 使用pipeline前端进行解析
    if (c99bin_state.pipeline_module) {
        ASTNode* (*frontend_compile)(const char*) =
            (ASTNode* (*)(const char*))c99bin_state.pipeline_module->sym(c99bin_state.pipeline_module, "frontend_compile");

        if (frontend_compile) {
            ASTNode* ast_root = frontend_compile(source_code);

            if (ast_root) {
                // 使用compiler模块生成目标代码
                if (c99bin_state.compiler_module) {
                    int (*jit_compile_ast)(ASTNode*, uint8_t**, size_t*) =
                        (int (*)(ASTNode*, uint8_t**, size_t*))c99bin_state.compiler_module->sym(c99bin_state.compiler_module, "jit_compile_ast");

                    if (jit_compile_ast) {
                        int result = jit_compile_ast(ast_root, object_code, object_size);

                        if (result == 0 && *object_code && *object_size > 0) {
                            printf("C99Bin Module: ✅ Object code generation successful (%zu bytes)\n", *object_size);
                            free(source_code);
                            return C99BIN_SUCCESS;
                        } else {
                            strcpy(c99bin_state.error_message, "Object code generation failed");
                            free(source_code);
                            return C99BIN_ERROR_CODEGEN_FAILED;
                        }
                    }
                }
                strcpy(c99bin_state.error_message, "Compiler module not available for object generation");
            } else {
                strcpy(c99bin_state.error_message, "Frontend parsing failed for object compilation");
            }
        } else {
            strcpy(c99bin_state.error_message, "Frontend compile function not found");
        }
    } else {
        strcpy(c99bin_state.error_message, "Pipeline module not available for object compilation");
    }

    free(source_code);
    return C99BIN_ERROR_PARSE_FAILED;
}

/**
 * 生成ELF可执行文件
 * @param machine_code 机器码数据
 * @param code_size 机器码大小
 * @param output_file 输出文件路径
 * @return C99BinResult 生成结果
 */
C99BinResult c99bin_generate_elf(const uint8_t* machine_code, size_t code_size, const char* output_file) {
    if (!machine_code || !output_file || code_size == 0) {
        strcpy(c99bin_state.error_message, "Invalid ELF generation parameters");
        return C99BIN_ERROR_INVALID_INPUT;
    }

    printf("C99Bin Module: Generating ELF file %s (%zu bytes machine code)\n", output_file, code_size);

    // ELF64 文件头结构
    typedef struct {
        unsigned char e_ident[16];    // ELF标识
        uint16_t e_type;              // 文件类型
        uint16_t e_machine;           // 机器类型
        uint32_t e_version;           // 版本
        uint64_t e_entry;             // 入口点地址
        uint64_t e_phoff;             // 程序头表偏移
        uint64_t e_shoff;             // 节头表偏移
        uint32_t e_flags;             // 标志
        uint16_t e_ehsize;            // ELF头大小
        uint16_t e_phentsize;         // 程序头表条目大小
        uint16_t e_phnum;             // 程序头表条目数
        uint16_t e_shentsize;         // 节头表条目大小
        uint16_t e_shnum;             // 节头表条目数
        uint16_t e_shstrndx;          // 字符串表索引
    } __attribute__((packed)) Elf64_Ehdr;

    // ELF64 程序头结构
    typedef struct {
        uint32_t p_type;              // 段类型
        uint32_t p_flags;             // 段标志
        uint64_t p_offset;            // 文件偏移
        uint64_t p_vaddr;             // 虚拟地址
        uint64_t p_paddr;             // 物理地址
        uint64_t p_filesz;            // 文件中大小
        uint64_t p_memsz;             // 内存中大小
        uint64_t p_align;             // 对齐
    } __attribute__((packed)) Elf64_Phdr;

    // 计算文件布局
    const uint64_t base_addr = 0x400000;           // 标准加载地址
    const size_t elf_header_size = sizeof(Elf64_Ehdr);
    const size_t program_header_size = sizeof(Elf64_Phdr);
    const size_t headers_size = elf_header_size + program_header_size;
    const uint64_t code_offset = (headers_size + 0xF) & ~0xF;  // 16字节对齐
    const uint64_t entry_point = base_addr + code_offset;

    // 创建ELF文件头
    Elf64_Ehdr elf_header = {0};

    // ELF标识
    elf_header.e_ident[0] = 0x7F;  // ELF魔数
    elf_header.e_ident[1] = 'E';
    elf_header.e_ident[2] = 'L';
    elf_header.e_ident[3] = 'F';
    elf_header.e_ident[4] = 2;     // 64位
    elf_header.e_ident[5] = 1;     // 小端
    elf_header.e_ident[6] = 1;     // ELF版本
    elf_header.e_ident[7] = 0;     // System V ABI

    // 文件头字段
    elf_header.e_type = 2;         // ET_EXEC (可执行文件)
    elf_header.e_machine = 0x3E;   // EM_X86_64
    elf_header.e_version = 1;      // EV_CURRENT
    elf_header.e_entry = entry_point;
    elf_header.e_phoff = elf_header_size;
    elf_header.e_shoff = 0;        // 无节头表
    elf_header.e_flags = 0;
    elf_header.e_ehsize = elf_header_size;
    elf_header.e_phentsize = program_header_size;
    elf_header.e_phnum = 1;        // 一个程序头
    elf_header.e_shentsize = 0;
    elf_header.e_shnum = 0;
    elf_header.e_shstrndx = 0;

    // 创建程序头
    Elf64_Phdr program_header = {0};
    program_header.p_type = 1;     // PT_LOAD
    program_header.p_flags = 5;    // PF_R | PF_X (可读可执行)
    program_header.p_offset = 0;
    program_header.p_vaddr = base_addr;
    program_header.p_paddr = base_addr;
    program_header.p_filesz = code_offset + code_size;
    program_header.p_memsz = code_offset + code_size;
    program_header.p_align = 0x1000;  // 4KB对齐

    // 写入ELF文件
    FILE* elf_file = fopen(output_file, "wb");
    if (!elf_file) {
        snprintf(c99bin_state.error_message, sizeof(c99bin_state.error_message),
                "Cannot create ELF file: %s", output_file);
        return C99BIN_ERROR_FILE_IO;
    }

    // 写入ELF头
    if (fwrite(&elf_header, sizeof(elf_header), 1, elf_file) != 1) {
        fclose(elf_file);
        strcpy(c99bin_state.error_message, "Failed to write ELF header");
        return C99BIN_ERROR_FILE_IO;
    }

    // 写入程序头
    if (fwrite(&program_header, sizeof(program_header), 1, elf_file) != 1) {
        fclose(elf_file);
        strcpy(c99bin_state.error_message, "Failed to write program header");
        return C99BIN_ERROR_FILE_IO;
    }

    // 填充到代码偏移位置
    size_t current_pos = elf_header_size + program_header_size;
    while (current_pos < code_offset) {
        fputc(0, elf_file);
        current_pos++;
    }

    // 写入机器码
    if (fwrite(machine_code, 1, code_size, elf_file) != code_size) {
        fclose(elf_file);
        strcpy(c99bin_state.error_message, "Failed to write machine code");
        return C99BIN_ERROR_FILE_IO;
    }

    fclose(elf_file);

    // 设置可执行权限
    if (chmod(output_file, 0755) != 0) {
        strcpy(c99bin_state.error_message, "Failed to set executable permissions");
        return C99BIN_ERROR_FILE_IO;
    }

    printf("C99Bin Module: ✅ ELF file generated successfully\n");
    printf("C99Bin Module: Entry point: 0x%lx, Code size: %zu bytes\n", entry_point, code_size);

    return C99BIN_SUCCESS;
}

/**
 * 链接多个目标代码到可执行文件
 * @param object_codes 目标代码数组
 * @param object_sizes 目标代码大小数组
 * @param object_count 目标代码数量
 * @param output_file 输出可执行文件路径
 * @return C99BinResult 链接结果
 */
C99BinResult c99bin_link_objects(uint8_t** object_codes, size_t* object_sizes, int object_count, const char* output_file) {
    if (!object_codes || !object_sizes || object_count <= 0 || !output_file) {
        strcpy(c99bin_state.error_message, "Invalid parameters for object linking");
        return C99BIN_ERROR_INVALID_INPUT;
    }

    printf("C99Bin Module: Linking %d object files to %s\n", object_count, output_file);

    // 计算总的代码大小
    size_t total_size = 0;
    for (int i = 0; i < object_count; i++) {
        total_size += object_sizes[i];
        printf("C99Bin Module: Object %d: %zu bytes\n", i, object_sizes[i]);
    }

    printf("C99Bin Module: Total linked code size: %zu bytes\n", total_size);

    // 分配内存用于合并的代码
    uint8_t* linked_code = malloc(total_size);
    if (!linked_code) {
        strcpy(c99bin_state.error_message, "Memory allocation failed for linked code");
        return C99BIN_ERROR_MEMORY_ALLOC;
    }

    // 简化的链接：直接拼接所有目标代码
    size_t offset = 0;
    for (int i = 0; i < object_count; i++) {
        memcpy(linked_code + offset, object_codes[i], object_sizes[i]);
        offset += object_sizes[i];
    }

    printf("C99Bin Module: Code linking completed, generating ELF...\n");

    // 生成ELF可执行文件
    C99BinResult result = c99bin_generate_elf(linked_code, total_size, output_file);

    free(linked_code);

    if (result == C99BIN_SUCCESS) {
        printf("C99Bin Module: ✅ Multi-file linking successful: %s\n", output_file);
    } else {
        printf("C99Bin Module: ❌ Multi-file linking failed: %s\n", c99bin_state.error_message);
    }

    return result;
}

/**
 * 多文件编译到可执行文件
 * @param source_files 源文件路径数组
 * @param source_count 源文件数量
 * @param output_file 输出可执行文件路径
 * @return C99BinResult 编译结果
 */
C99BinResult c99bin_compile_multiple_files(const char** source_files, int source_count, const char* output_file) {
    if (!source_files || source_count <= 0 || !output_file) {
        strcpy(c99bin_state.error_message, "Invalid parameters for multi-file compilation");
        return C99BIN_ERROR_INVALID_INPUT;
    }

    printf("C99Bin Module: Multi-file compilation (%d files) to %s\n", source_count, output_file);

    // 初始化多文件项目结构
    multi_project.source_files = malloc(source_count * sizeof(char*));
    multi_project.object_codes = malloc(source_count * sizeof(uint8_t*));
    multi_project.object_sizes = malloc(source_count * sizeof(size_t));
    multi_project.source_count = source_count;

    if (!multi_project.source_files || !multi_project.object_codes || !multi_project.object_sizes) {
        strcpy(c99bin_state.error_message, "Memory allocation failed for multi-file project");
        return C99BIN_ERROR_MEMORY_ALLOC;
    }

    // 编译每个源文件到目标代码
    for (int i = 0; i < source_count; i++) {
        multi_project.source_files[i] = strdup(source_files[i]);

        printf("C99Bin Module: Compiling file %d/%d: %s\n", i + 1, source_count, source_files[i]);

        C99BinResult result = c99bin_compile_to_object(source_files[i],
                                                      &multi_project.object_codes[i],
                                                      &multi_project.object_sizes[i]);

        if (result != C99BIN_SUCCESS) {
            printf("C99Bin Module: ❌ Failed to compile %s\n", source_files[i]);
            // 清理已分配的内存
            for (int j = 0; j <= i; j++) {
                if (multi_project.source_files[j]) free(multi_project.source_files[j]);
                if (j < i && multi_project.object_codes[j]) free(multi_project.object_codes[j]);
            }
            free(multi_project.source_files);
            free(multi_project.object_codes);
            free(multi_project.object_sizes);
            return result;
        }

        printf("C99Bin Module: ✅ Compiled %s (%zu bytes object code)\n",
               source_files[i], multi_project.object_sizes[i]);
    }

    // 链接所有目标代码
    printf("C99Bin Module: Linking %d object files...\n", source_count);

    C99BinResult link_result = c99bin_link_objects(multi_project.object_codes,
                                                   multi_project.object_sizes,
                                                   source_count,
                                                   output_file);

    // 清理内存
    for (int i = 0; i < source_count; i++) {
        if (multi_project.source_files[i]) free(multi_project.source_files[i]);
        if (multi_project.object_codes[i]) free(multi_project.object_codes[i]);
    }
    free(multi_project.source_files);
    free(multi_project.object_codes);
    free(multi_project.object_sizes);

    return link_result;
}

/**
 * 生成PE可执行文件 (Windows)
 * @param machine_code 机器码数据
 * @param code_size 机器码大小
 * @param output_file 输出文件路径
 * @return C99BinResult 生成结果
 */
C99BinResult c99bin_generate_pe(const uint8_t* machine_code, size_t code_size, const char* output_file) {
    if (!machine_code || !output_file || code_size == 0) {
        strcpy(c99bin_state.error_message, "Invalid PE generation parameters");
        return C99BIN_ERROR_INVALID_INPUT;
    }
    
    printf("C99Bin Module: Generating PE file %s (%zu bytes)\n", output_file, code_size);
    
    // TODO: T4.2 - 实现PE文件格式生成
    strcpy(c99bin_state.error_message, "PE generation not implemented yet");
    return C99BIN_ERROR_CODEGEN_FAILED;
}

/**
 * 获取错误信息
 * @return 最后的错误信息
 */
const char* c99bin_get_error(void) {
    return c99bin_state.error_message;
}

/**
 * 检查模块是否已初始化
 * @return true如果已初始化
 */
bool c99bin_is_initialized(void) {
    return c99bin_state.initialized;
}

// ===============================================
// 模块依赖管理
// ===============================================

/**
 * 设置依赖模块 (由外部调用者提供)
 * @param pipeline_module Pipeline模块指针
 * @param compiler_module Compiler模块指针
 * @param layer0_module Layer0模块指针
 * @return C99BinResult 设置结果
 */
C99BinResult c99bin_set_dependencies(Module* pipeline_module, Module* compiler_module, Module* layer0_module) {
    printf("C99Bin Module: Setting dependency modules...\n");

    c99bin_state.pipeline_module = pipeline_module;
    c99bin_state.compiler_module = compiler_module;
    c99bin_state.layer0_module = layer0_module;

    if (pipeline_module) {
        printf("C99Bin Module: Pipeline module set successfully\n");
    } else {
        printf("C99Bin Module: Pipeline module not provided\n");
    }

    if (compiler_module) {
        printf("C99Bin Module: Compiler module set successfully\n");
    } else {
        printf("C99Bin Module: Compiler module not provided\n");
    }

    if (layer0_module) {
        printf("C99Bin Module: Layer0 module set successfully\n");
    } else {
        printf("C99Bin Module: Layer0 module not provided\n");
    }

    printf("C99Bin Module: Dependency setup completed\n");
    return C99BIN_SUCCESS;
}

/**
 * 加载依赖模块 (简化版本，不调用load_module)
 * @return C99BinResult 加载结果
 */
static C99BinResult c99bin_load_dependencies(void) {
    printf("C99Bin Module: Dependency loading skipped - use c99bin_set_dependencies() instead\n");
    return C99BIN_SUCCESS;
}

// ===============================================
// 模块符号表
// ===============================================

static struct {
    const char* name;
    void* symbol;
} c99bin_symbols[] = {
    // 核心编译接口
    {"c99bin_compile_to_executable", c99bin_compile_to_executable},
    {"c99bin_generate_elf", c99bin_generate_elf},
    {"c99bin_generate_pe", c99bin_generate_pe},

    // 状态查询接口
    {"c99bin_get_error", c99bin_get_error},
    {"c99bin_is_initialized", c99bin_is_initialized},

    // 依赖管理接口
    {"c99bin_set_dependencies", c99bin_set_dependencies},
    {"c99bin_load_dependencies", c99bin_load_dependencies},

    {NULL, NULL}  // 结束标记
};

// ===============================================
// 模块接口实现
// ===============================================

/**
 * 解析符号
 * @param symbol 符号名称
 * @return 符号地址，未找到返回NULL
 */
static void* c99bin_resolve(const char* symbol) {
    if (!symbol) return NULL;
    
    for (int i = 0; c99bin_symbols[i].name; i++) {
        if (strcmp(c99bin_symbols[i].name, symbol) == 0) {
            return c99bin_symbols[i].symbol;
        }
    }
    
    return NULL;
}

/**
 * 初始化模块
 * @return 0成功，-1失败
 */
static int c99bin_init(void) {
    printf("C99Bin Module: Initializing C99 binary compiler...\n");
    
    // 清理状态
    memset(&c99bin_state, 0, sizeof(C99BinState));
    
    // TODO: T1.4 - 基础架构检测和多平台支持
    printf("C99Bin Module: Detecting target architecture...\n");
    
    // 加载依赖模块
    C99BinResult result = c99bin_load_dependencies();
    if (result != C99BIN_SUCCESS) {
        printf("C99Bin Module: Failed to load dependencies: %s\n", c99bin_state.error_message);
        return -1;
    }
    
    c99bin_state.initialized = true;
    printf("C99Bin Module: Initialization completed successfully\n");
    
    return 0;
}

/**
 * 清理模块
 */
static void c99bin_cleanup(void) {
    printf("C99Bin Module: Cleaning up...\n");

    // 清理分配的内存 (安全检查)
    if (c99bin_state.source_code) {
        free(c99bin_state.source_code);
        c99bin_state.source_code = NULL;
    }

    if (c99bin_state.machine_code) {
        free(c99bin_state.machine_code);
        c99bin_state.machine_code = NULL;
        c99bin_state.machine_code_size = 0;
    }

    // 清理AST (如果存在)
    if (c99bin_state.ast_root) {
        // TODO: 实现AST清理函数
        // ast_free(c99bin_state.ast_root);
        c99bin_state.ast_root = NULL;
    }

    // 重置状态
    c99bin_state.initialized = false;
    c99bin_state.pipeline_module = NULL;
    c99bin_state.compiler_module = NULL;
    c99bin_state.layer0_module = NULL;
    c99bin_state.error_message[0] = '\0';

    printf("C99Bin Module: Cleanup completed\n");
}

// ===============================================
// 模块定义
// ===============================================

// C99Bin模块定义
Module module_c99bin = {
    .name = "c99bin",
    .state = MODULE_UNLOADED,
    .error = NULL,
    .init = c99bin_init,
    .cleanup = c99bin_cleanup,
    .resolve = c99bin_resolve
};

// ===============================================
// 导出函数 (用于.native文件)
// ===============================================

// 这些函数将被放在特定偏移量处，供动态加载使用
int module_init(void) {
    return c99bin_init();
}

void module_cleanup(void) {
    c99bin_cleanup();
}

void* c99bin_module_resolve(const char* symbol) {
    return c99bin_resolve(symbol);
}

// 测试导出函数
int test_export_function(void) {
    printf("C99Bin Module: test_export_function called\n");
    return 99;  // 返回99表示c99bin模块
}
