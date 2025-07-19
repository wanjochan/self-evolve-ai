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
#include <time.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif
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
C99BinResult c99bin_optimize_ast(ASTNode* ast, int optimization_level);
static bool c99bin_constant_folding_pass(ASTNode* ast);
static bool c99bin_dead_code_elimination_pass(ASTNode* ast);
static bool c99bin_advanced_optimization_pass(ASTNode* ast);

// T3.2.2 - 错误诊断和警告系统
typedef struct {
    char** source_lines;           // 源代码行数组
    int source_line_count;         // 源代码行数
    char* filename;                // 当前文件名
    bool show_warnings;            // 是否显示警告
    bool warnings_as_errors;       // 警告作为错误
    int max_errors;                // 最大错误数
    int error_count;               // 当前错误数
    int warning_count;             // 当前警告数
} C99BinDiagnostics;

static C99BinDiagnostics c99bin_diagnostics = {0};

// 诊断函数声明
static void c99bin_diagnostic_init(const char* filename, const char* source_code);
static void c99bin_diagnostic_cleanup(void);
static void c99bin_diagnostic_error(int line, int column, const char* message, const char* suggestion);
static void c99bin_diagnostic_warning(int line, int column, const char* message);
static void c99bin_diagnostic_note(int line, int column, const char* message);
static void c99bin_diagnostic_print_context(int line, int column, int length);
static void c99bin_diagnostic_summary(void);
static void c99bin_perform_semantic_checks(ASTNode* ast);
static void c99bin_check_ast_node(ASTNode* node, int depth);

// T3.2.3 - 调试信息生成系统
typedef struct {
    char* filename;                // 源文件名
    int line_count;                // 总行数
    bool debug_enabled;            // 是否启用调试信息
    bool generate_symbols;         // 是否生成符号表
    bool generate_line_numbers;    // 是否生成行号信息

    // 符号表信息
    char** function_names;         // 函数名列表
    int* function_lines;           // 函数起始行号
    int function_count;            // 函数数量

    // 行号映射
    int* source_lines;             // 源代码行号
    size_t* code_offsets;          // 对应的代码偏移
    int line_mapping_count;        // 行号映射数量
} C99BinDebugInfo;

static C99BinDebugInfo c99bin_debug_info = {0};

// 调试信息函数声明
static void c99bin_debug_init(const char* filename, bool enable_debug);
static void c99bin_debug_cleanup(void);
static void c99bin_debug_add_function(const char* name, int line);
static void c99bin_debug_add_line_mapping(int source_line, size_t code_offset);
static void c99bin_debug_generate_symbols(ASTNode* ast);
static void c99bin_debug_write_info(const char* output_file);
static void c99bin_debug_collect_symbols(ASTNode* node);

// T3.2.4 - 编译性能优化系统
typedef struct {
    bool performance_mode;         // 是否启用性能模式
    bool enable_caching;           // 是否启用缓存
    bool enable_parallel;          // 是否启用并行编译
    int max_threads;               // 最大线程数

    // 性能统计
    clock_t start_time;            // 编译开始时间
    clock_t end_time;              // 编译结束时间
    size_t peak_memory;            // 峰值内存使用
    size_t current_memory;         // 当前内存使用

    // 缓存信息
    char* cache_dir;               // 缓存目录
    int cache_hits;                // 缓存命中次数
    int cache_misses;              // 缓存未命中次数

    // 编译阶段计时
    clock_t lexer_time;            // 词法分析时间
    clock_t parser_time;           // 语法分析时间
    clock_t semantic_time;         // 语义分析时间
    clock_t optimization_time;     // 优化时间
    clock_t codegen_time;          // 代码生成时间
} C99BinPerformance;

static C99BinPerformance c99bin_performance = {0};

// 性能优化函数声明
static void c99bin_performance_init(bool enable_performance_mode);
static void c99bin_performance_cleanup(void);
static void c99bin_performance_start_timer(void);
static void c99bin_performance_end_timer(void);
static void c99bin_performance_start_phase(const char* phase_name);
static void c99bin_performance_end_phase(const char* phase_name);
static void c99bin_performance_update_memory(void);
static void c99bin_performance_print_stats(void);
static bool c99bin_performance_check_cache(const char* source_file, const char* output_file);
static void c99bin_performance_save_cache(const char* source_file, const char* output_file);

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

    // T3.2.2 - 初始化诊断系统
    printf("C99Bin Module: Phase 1.5 - Initializing diagnostics...\n");
    c99bin_diagnostic_init(source_file, source_code);

    // T3.2.3 - 初始化调试信息系统 (默认启用)
    printf("C99Bin Module: Phase 1.6 - Initializing debug info...\n");
    c99bin_debug_init(source_file, true);

    // T3.2.4 - 初始化性能优化系统 (默认启用)
    printf("C99Bin Module: Phase 1.7 - Initializing performance optimization...\n");
    c99bin_performance_init(true);
    c99bin_performance_start_timer();
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

        // T3.2.3 - 写入调试信息
        printf("C99Bin Module: Phase 5 - Writing debug information...\n");
        c99bin_debug_write_info(output_file);

        // T3.2.4 - 保存到缓存
        c99bin_performance_save_cache(source_file, output_file);

        // T3.2.4 - 结束性能计时并打印统计
        c99bin_performance_end_timer();
        c99bin_performance_print_stats();

        // T3.2.2 - 打印诊断摘要
        c99bin_diagnostic_summary();
        c99bin_diagnostic_cleanup();
        c99bin_debug_cleanup();
        c99bin_performance_cleanup();

        return C99BIN_SUCCESS;
    } else {
        printf("C99Bin Module: ❌ ELF generation failed: %s\n", c99bin_state.error_message);

        // T3.2.4 - 结束性能计时并打印统计（即使失败）
        c99bin_performance_end_timer();
        c99bin_performance_print_stats();

        // T3.2.2 - 打印诊断摘要（即使失败）
        c99bin_diagnostic_summary();
        c99bin_diagnostic_cleanup();
        c99bin_debug_cleanup();
        c99bin_performance_cleanup();

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

    // T3.2.4 - 对象编译不使用缓存（因为没有输出文件）
    printf("C99Bin Module: Object compilation - cache not applicable\n");

    // 使用pipeline前端进行解析
    c99bin_performance_start_phase("Frontend Parsing");
    if (c99bin_state.pipeline_module) {
        ASTNode* (*frontend_compile)(const char*) =
            (ASTNode* (*)(const char*))c99bin_state.pipeline_module->sym(c99bin_state.pipeline_module, "frontend_compile");

        if (frontend_compile) {
            ASTNode* ast_root = frontend_compile(source_code);
            c99bin_performance_end_phase("Frontend Parsing");

            if (ast_root) {
                // T3.2.2 - 执行语义分析和诊断检查
                c99bin_performance_start_phase("Semantic Analysis");
                printf("C99Bin Module: Phase 2.3 - Performing semantic analysis...\n");
                c99bin_perform_semantic_checks(ast_root);
                c99bin_performance_end_phase("Semantic Analysis");

                // T3.2.3 - 生成调试符号信息
                c99bin_performance_start_phase("Debug Symbol Generation");
                printf("C99Bin Module: Phase 2.4 - Generating debug symbols...\n");
                c99bin_debug_generate_symbols(ast_root);
                c99bin_performance_end_phase("Debug Symbol Generation");

                // T3.2.1 - 应用AST优化 (默认优化级别1)
                c99bin_performance_start_phase("AST Optimization");
                printf("C99Bin Module: Phase 2.5 - Applying AST optimizations...\n");
                C99BinResult opt_result = c99bin_optimize_ast(ast_root, 1);
                if (opt_result != C99BIN_SUCCESS) {
                    printf("C99Bin Module: ⚠️ Optimization failed, continuing with unoptimized AST\n");
                }
                c99bin_performance_end_phase("AST Optimization");

                // 使用compiler模块生成目标代码
                c99bin_performance_start_phase("Code Generation");
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
 * AST优化功能
 * @param ast 要优化的AST节点
 * @param optimization_level 优化级别 (0-3)
 * @return C99BinResult 优化结果
 */
C99BinResult c99bin_optimize_ast(ASTNode* ast, int optimization_level) {
    if (!ast) {
        strcpy(c99bin_state.error_message, "Invalid AST for optimization");
        return C99BIN_ERROR_INVALID_INPUT;
    }

    if (optimization_level < 0 || optimization_level > 3) {
        strcpy(c99bin_state.error_message, "Invalid optimization level (must be 0-3)");
        return C99BIN_ERROR_INVALID_INPUT;
    }

    printf("C99Bin Module: Applying optimizations (level %d)...\n", optimization_level);

    // 如果优化级别为0，跳过优化
    if (optimization_level == 0) {
        printf("C99Bin Module: ✅ No optimization requested (level 0)\n");
        return C99BIN_SUCCESS;
    }

    // 应用常量折叠优化
    if (optimization_level >= 1) {
        printf("C99Bin Module: Applying constant folding...\n");
        if (c99bin_constant_folding_pass(ast)) {
            printf("C99Bin Module: ✅ Constant folding applied\n");
        } else {
            printf("C99Bin Module: ⚠️ Constant folding made no changes\n");
        }
    }

    // 应用死代码消除
    if (optimization_level >= 2) {
        printf("C99Bin Module: Applying dead code elimination...\n");
        if (c99bin_dead_code_elimination_pass(ast)) {
            printf("C99Bin Module: ✅ Dead code elimination applied\n");
        } else {
            printf("C99Bin Module: ⚠️ Dead code elimination made no changes\n");
        }
    }

    // 应用高级优化
    if (optimization_level >= 3) {
        printf("C99Bin Module: Applying advanced optimizations...\n");
        if (c99bin_advanced_optimization_pass(ast)) {
            printf("C99Bin Module: ✅ Advanced optimizations applied\n");
        } else {
            printf("C99Bin Module: ⚠️ Advanced optimizations made no changes\n");
        }
    }

    printf("C99Bin Module: ✅ Optimization completed (level %d)\n", optimization_level);
    return C99BIN_SUCCESS;
}

/**
 * 常量折叠优化通道
 * @param ast AST节点
 * @return bool 是否进行了优化
 */
static bool c99bin_constant_folding_pass(ASTNode* ast) {
    if (!ast) return false;

    bool optimized = false;

    switch (ast->type) {
        case ASTC_BINARY_OP: {
            // 递归优化子节点
            if (c99bin_constant_folding_pass(ast->data.binary_op.left)) optimized = true;
            if (c99bin_constant_folding_pass(ast->data.binary_op.right)) optimized = true;

            // 检查是否可以折叠常量
            if (ast->data.binary_op.left->type == ASTC_EXPR_CONSTANT &&
                ast->data.binary_op.right->type == ASTC_EXPR_CONSTANT) {

                int left_val = ast->data.binary_op.left->data.constant.int_val;
                int right_val = ast->data.binary_op.right->data.constant.int_val;
                int result_val = 0;
                bool can_fold = true;

                switch (ast->data.binary_op.op) {
                    case ASTC_OP_ADD:
                        result_val = left_val + right_val;
                        break;
                    case ASTC_OP_SUB:
                        result_val = left_val - right_val;
                        break;
                    case ASTC_OP_MUL:
                        result_val = left_val * right_val;
                        break;
                    case ASTC_OP_DIV:
                        if (right_val != 0) {
                            result_val = left_val / right_val;
                        } else {
                            can_fold = false; // 避免除零
                        }
                        break;
                    default:
                        can_fold = false;
                        break;
                }

                if (can_fold) {
                    // 释放原有子节点
                    ast_free(ast->data.binary_op.left);
                    ast_free(ast->data.binary_op.right);

                    // 转换为常量节点
                    ast->type = ASTC_EXPR_CONSTANT;
                    ast->data.constant.type = ASTC_TYPE_INT;
                    ast->data.constant.int_val = result_val;

                    printf("C99Bin Module: Folded constant: %d %s %d = %d\n",
                           left_val,
                           (ast->data.binary_op.op == ASTC_OP_ADD) ? "+" :
                           (ast->data.binary_op.op == ASTC_OP_SUB) ? "-" :
                           (ast->data.binary_op.op == ASTC_OP_MUL) ? "*" : "/",
                           right_val, result_val);

                    optimized = true;
                }
            }
            break;
        }

        case ASTC_FUNC_DECL:
            if (ast->data.func_decl.body) {
                if (c99bin_constant_folding_pass(ast->data.func_decl.body)) optimized = true;
            }
            break;

        case ASTC_COMPOUND_STMT:
            for (int i = 0; i < ast->data.compound_stmt.statement_count; i++) {
                if (c99bin_constant_folding_pass(ast->data.compound_stmt.statements[i])) {
                    optimized = true;
                }
            }
            break;

        case ASTC_IF_STMT:
            if (c99bin_constant_folding_pass(ast->data.if_stmt.condition)) optimized = true;
            if (c99bin_constant_folding_pass(ast->data.if_stmt.then_branch)) optimized = true;
            if (ast->data.if_stmt.else_branch &&
                c99bin_constant_folding_pass(ast->data.if_stmt.else_branch)) optimized = true;
            break;

        case ASTC_RETURN_STMT:
            if (ast->data.return_stmt.value &&
                c99bin_constant_folding_pass(ast->data.return_stmt.value)) optimized = true;
            break;

        default:
            // 对于其他节点类型，暂时不处理
            break;
    }

    return optimized;
}

/**
 * 死代码消除优化通道
 * @param ast AST节点
 * @return bool 是否进行了优化
 */
static bool c99bin_dead_code_elimination_pass(ASTNode* ast) {
    if (!ast) return false;

    bool optimized = false;

    switch (ast->type) {
        case ASTC_IF_STMT: {
            // 检查条件是否为常量
            if (ast->data.if_stmt.condition->type == ASTC_EXPR_CONSTANT) {
                int condition_val = ast->data.if_stmt.condition->data.constant.int_val;

                if (condition_val != 0) {
                    // 条件总是真，消除else分支
                    if (ast->data.if_stmt.else_branch) {
                        printf("C99Bin Module: Eliminating dead else branch (condition always true)\n");
                        ast_free(ast->data.if_stmt.else_branch);
                        ast->data.if_stmt.else_branch = NULL;
                        optimized = true;
                    }
                } else {
                    // 条件总是假，消除then分支
                    printf("C99Bin Module: Eliminating dead then branch (condition always false)\n");
                    ast_free(ast->data.if_stmt.then_branch);

                    if (ast->data.if_stmt.else_branch) {
                        ast->data.if_stmt.then_branch = ast->data.if_stmt.else_branch;
                        ast->data.if_stmt.else_branch = NULL;
                    } else {
                        // 整个if语句都是死代码，但这里简化处理
                        ast->data.if_stmt.then_branch = NULL;
                    }
                    optimized = true;
                }
            }

            // 递归处理子节点
            if (c99bin_dead_code_elimination_pass(ast->data.if_stmt.condition)) optimized = true;
            if (ast->data.if_stmt.then_branch &&
                c99bin_dead_code_elimination_pass(ast->data.if_stmt.then_branch)) optimized = true;
            if (ast->data.if_stmt.else_branch &&
                c99bin_dead_code_elimination_pass(ast->data.if_stmt.else_branch)) optimized = true;
            break;
        }

        case ASTC_COMPOUND_STMT: {
            // 检查复合语句中的死代码
            for (int i = 0; i < ast->data.compound_stmt.statement_count; i++) {
                if (c99bin_dead_code_elimination_pass(ast->data.compound_stmt.statements[i])) {
                    optimized = true;
                }

                // 检查return语句后的死代码
                if (ast->data.compound_stmt.statements[i]->type == ASTC_RETURN_STMT) {
                    if (i + 1 < ast->data.compound_stmt.statement_count) {
                        printf("C99Bin Module: Eliminating %d dead statements after return\n",
                               ast->data.compound_stmt.statement_count - i - 1);

                        // 释放return后的所有语句
                        for (int j = i + 1; j < ast->data.compound_stmt.statement_count; j++) {
                            ast_free(ast->data.compound_stmt.statements[j]);
                        }

                        // 调整语句数量
                        ast->data.compound_stmt.statement_count = i + 1;
                        optimized = true;
                        break;
                    }
                }
            }
            break;
        }

        case ASTC_FUNC_DECL:
            if (ast->data.func_decl.body &&
                c99bin_dead_code_elimination_pass(ast->data.func_decl.body)) optimized = true;
            break;

        case ASTC_RETURN_STMT:
            if (ast->data.return_stmt.value &&
                c99bin_dead_code_elimination_pass(ast->data.return_stmt.value)) optimized = true;
            break;

        default:
            // 对于其他节点类型，暂时不处理
            break;
    }

    return optimized;
}

/**
 * 高级优化通道
 * @param ast AST节点
 * @return bool 是否进行了优化
 */
static bool c99bin_advanced_optimization_pass(ASTNode* ast) {
    if (!ast) return false;

    bool optimized = false;

    // 简化的高级优化：函数内联、循环展开等
    switch (ast->type) {
        case ASTC_BINARY_OP: {
            // 代数简化
            if (ast->data.binary_op.op == ASTC_OP_MUL) {
                // x * 1 = x, x * 0 = 0
                if (ast->data.binary_op.right->type == ASTC_EXPR_CONSTANT) {
                    int right_val = ast->data.binary_op.right->data.constant.int_val;

                    if (right_val == 1) {
                        // x * 1 = x
                        printf("C99Bin Module: Optimizing x * 1 = x\n");
                        ASTNode* left = ast->data.binary_op.left;
                        ast_free(ast->data.binary_op.right);

                        // 复制左节点的内容到当前节点
                        *ast = *left;
                        free(left); // 只释放节点结构，不释放内容
                        optimized = true;
                    } else if (right_val == 0) {
                        // x * 0 = 0
                        printf("C99Bin Module: Optimizing x * 0 = 0\n");
                        ast_free(ast->data.binary_op.left);
                        ast_free(ast->data.binary_op.right);

                        ast->type = ASTC_EXPR_CONSTANT;
                        ast->data.constant.type = ASTC_TYPE_INT;
                        ast->data.constant.int_val = 0;
                        optimized = true;
                    }
                }
            } else if (ast->data.binary_op.op == ASTC_OP_ADD) {
                // x + 0 = x
                if (ast->data.binary_op.right->type == ASTC_EXPR_CONSTANT &&
                    ast->data.binary_op.right->data.constant.int_val == 0) {

                    printf("C99Bin Module: Optimizing x + 0 = x\n");
                    ASTNode* left = ast->data.binary_op.left;
                    ast_free(ast->data.binary_op.right);

                    *ast = *left;
                    free(left);
                    optimized = true;
                }
            }
            break;
        }

        case ASTC_FUNC_DECL:
            if (ast->data.func_decl.body &&
                c99bin_advanced_optimization_pass(ast->data.func_decl.body)) optimized = true;
            break;

        case ASTC_COMPOUND_STMT:
            for (int i = 0; i < ast->data.compound_stmt.statement_count; i++) {
                if (c99bin_advanced_optimization_pass(ast->data.compound_stmt.statements[i])) {
                    optimized = true;
                }
            }
            break;

        default:
            break;
    }

    return optimized;
}



// ===============================================
// T3.2.2 - 错误诊断和警告系统实现
// ===============================================

/**
 * 初始化诊断系统
 * @param filename 源文件名
 * @param source_code 源代码内容
 */
static void c99bin_diagnostic_init(const char* filename, const char* source_code) {
    // 清理之前的状态
    c99bin_diagnostic_cleanup();

    // 设置文件名
    if (filename) {
        c99bin_diagnostics.filename = strdup(filename);
    }

    // 分割源代码为行
    if (source_code) {
        // 计算行数
        int line_count = 1;
        for (const char* p = source_code; *p; p++) {
            if (*p == '\n') line_count++;
        }

        // 分配行数组
        c99bin_diagnostics.source_lines = malloc(line_count * sizeof(char*));
        c99bin_diagnostics.source_line_count = line_count;

        // 分割行
        const char* line_start = source_code;
        int current_line = 0;

        for (const char* p = source_code; *p; p++) {
            if (*p == '\n' || *(p + 1) == '\0') {
                size_t line_length = p - line_start + (*p != '\n' ? 1 : 0);
                c99bin_diagnostics.source_lines[current_line] = malloc(line_length + 1);
                strncpy(c99bin_diagnostics.source_lines[current_line], line_start, line_length);
                c99bin_diagnostics.source_lines[current_line][line_length] = '\0';

                current_line++;
                line_start = p + 1;
            }
        }
    }

    // 设置默认选项
    c99bin_diagnostics.show_warnings = true;
    c99bin_diagnostics.warnings_as_errors = false;
    c99bin_diagnostics.max_errors = 20;
    c99bin_diagnostics.error_count = 0;
    c99bin_diagnostics.warning_count = 0;

    printf("C99Bin Diagnostics: Initialized for %s (%d lines)\n",
           filename ? filename : "<unknown>", c99bin_diagnostics.source_line_count);
}

/**
 * 清理诊断系统
 */
static void c99bin_diagnostic_cleanup(void) {
    if (c99bin_diagnostics.filename) {
        free(c99bin_diagnostics.filename);
        c99bin_diagnostics.filename = NULL;
    }

    if (c99bin_diagnostics.source_lines) {
        for (int i = 0; i < c99bin_diagnostics.source_line_count; i++) {
            if (c99bin_diagnostics.source_lines[i]) {
                free(c99bin_diagnostics.source_lines[i]);
            }
        }
        free(c99bin_diagnostics.source_lines);
        c99bin_diagnostics.source_lines = NULL;
    }

    c99bin_diagnostics.source_line_count = 0;
    c99bin_diagnostics.error_count = 0;
    c99bin_diagnostics.warning_count = 0;
}

/**
 * 报告错误
 * @param line 行号 (1-based)
 * @param column 列号 (1-based)
 * @param message 错误消息
 * @param suggestion 建议修复方案
 */
static void c99bin_diagnostic_error(int line, int column, const char* message, const char* suggestion) {
    if (c99bin_diagnostics.error_count >= c99bin_diagnostics.max_errors) {
        return; // 达到最大错误数
    }

    c99bin_diagnostics.error_count++;

    // 打印错误信息
    printf("%s:%d:%d: \033[1;31merror:\033[0m %s\n",
           c99bin_diagnostics.filename ? c99bin_diagnostics.filename : "<unknown>",
           line, column, message);

    // 打印源代码上下文
    c99bin_diagnostic_print_context(line, column, 1);

    // 打印建议
    if (suggestion) {
        printf("  \033[1;36mnote:\033[0m %s\n", suggestion);
    }

    printf("\n");
}

/**
 * 报告警告
 * @param line 行号 (1-based)
 * @param column 列号 (1-based)
 * @param message 警告消息
 */
static void c99bin_diagnostic_warning(int line, int column, const char* message) {
    if (!c99bin_diagnostics.show_warnings) {
        return;
    }

    c99bin_diagnostics.warning_count++;

    // 如果警告作为错误处理
    if (c99bin_diagnostics.warnings_as_errors) {
        c99bin_diagnostic_error(line, column, message, "treat this warning as an error");
        return;
    }

    // 打印警告信息
    printf("%s:%d:%d: \033[1;33mwarning:\033[0m %s\n",
           c99bin_diagnostics.filename ? c99bin_diagnostics.filename : "<unknown>",
           line, column, message);

    // 打印源代码上下文
    c99bin_diagnostic_print_context(line, column, 1);

    printf("\n");
}

/**
 * 报告提示信息
 * @param line 行号 (1-based)
 * @param column 列号 (1-based)
 * @param message 提示消息
 */
static void c99bin_diagnostic_note(int line, int column, const char* message) {
    printf("%s:%d:%d: \033[1;36mnote:\033[0m %s\n",
           c99bin_diagnostics.filename ? c99bin_diagnostics.filename : "<unknown>",
           line, column, message);

    c99bin_diagnostic_print_context(line, column, 1);
    printf("\n");
}

/**
 * 打印源代码上下文
 * @param line 行号 (1-based)
 * @param column 列号 (1-based)
 * @param length 高亮长度
 */
static void c99bin_diagnostic_print_context(int line, int column, int length) {
    if (!c99bin_diagnostics.source_lines || line < 1 || line > c99bin_diagnostics.source_line_count) {
        return;
    }

    // 打印源代码行
    printf("  %4d | %s\n", line, c99bin_diagnostics.source_lines[line - 1]);

    // 打印指示符
    printf("       | ");
    for (int i = 1; i < column; i++) {
        printf(" ");
    }
    printf("\033[1;32m");
    for (int i = 0; i < length; i++) {
        printf("^");
    }
    printf("\033[0m");
}

/**
 * 打印诊断摘要
 */
static void c99bin_diagnostic_summary(void) {
    if (c99bin_diagnostics.error_count > 0 || c99bin_diagnostics.warning_count > 0) {
        printf("\n=== Compilation Summary ===\n");

        if (c99bin_diagnostics.error_count > 0) {
            printf("\033[1;31m%d error(s)\033[0m", c99bin_diagnostics.error_count);
        }

        if (c99bin_diagnostics.warning_count > 0) {
            if (c99bin_diagnostics.error_count > 0) printf(", ");
            printf("\033[1;33m%d warning(s)\033[0m", c99bin_diagnostics.warning_count);
        }

        printf(" generated.\n");

        if (c99bin_diagnostics.error_count > 0) {
            printf("Compilation \033[1;31mfailed\033[0m due to errors.\n");
        } else {
            printf("Compilation \033[1;32msucceeded\033[0m with warnings.\n");
        }
    } else {
        printf("Compilation \033[1;32msucceeded\033[0m with no issues.\n");
    }
}

/**
 * 执行语义分析和诊断检查
 * @param ast AST根节点
 */
static void c99bin_perform_semantic_checks(ASTNode* ast) {
    if (!ast) {
        c99bin_diagnostic_error(1, 1, "Empty AST - no code to analyze", "Check if source file is empty");
        return;
    }

    printf("C99Bin Module: Performing semantic analysis and diagnostics...\n");

    // 递归检查AST节点
    c99bin_check_ast_node(ast, 0);

    printf("C99Bin Module: Semantic analysis completed\n");
}

/**
 * 检查单个AST节点
 * @param node AST节点
 * @param depth 递归深度
 */
static void c99bin_check_ast_node(ASTNode* node, int depth) {
    if (!node) return;

    // 避免过深递归
    if (depth > 100) {
        c99bin_diagnostic_warning(node->line, node->column, "Very deep AST structure detected");
        return;
    }

    switch (node->type) {
        case ASTC_FUNC_DECL: {
            // 检查函数声明
            if (!node->data.func_decl.name) {
                c99bin_diagnostic_error(node->line, node->column,
                                      "Function declaration missing name",
                                      "Add a function name");
            } else {
                // 检查函数名命名规范
                const char* name = node->data.func_decl.name;
                if (strlen(name) > 63) {
                    c99bin_diagnostic_warning(node->line, node->column,
                                            "Function name is very long (>63 characters)");
                }

                // 检查是否以下划线开头（可能与系统函数冲突）
                if (name[0] == '_') {
                    c99bin_diagnostic_warning(node->line, node->column,
                                            "Function name starts with underscore (may conflict with system functions)");
                }
            }

            // 检查函数体
            if (node->data.func_decl.body) {
                c99bin_check_ast_node(node->data.func_decl.body, depth + 1);
            } else {
                c99bin_diagnostic_note(node->line, node->column,
                                     "Function declaration without body (forward declaration)");
            }
            break;
        }

        case ASTC_BINARY_OP: {
            // 检查二元操作
            if (!node->data.binary_op.left || !node->data.binary_op.right) {
                c99bin_diagnostic_error(node->line, node->column,
                                      "Binary operation missing operand(s)",
                                      "Check expression syntax");
            } else {
                // 检查除零操作
                if (node->data.binary_op.op == ASTC_OP_DIV &&
                    node->data.binary_op.right->type == ASTC_EXPR_CONSTANT &&
                    node->data.binary_op.right->data.constant.int_val == 0) {
                    c99bin_diagnostic_error(node->line, node->column,
                                          "Division by zero",
                                          "Use a non-zero divisor");
                }

                // 递归检查操作数
                c99bin_check_ast_node(node->data.binary_op.left, depth + 1);
                c99bin_check_ast_node(node->data.binary_op.right, depth + 1);
            }
            break;
        }

        case ASTC_IF_STMT: {
            // 检查if语句
            if (!node->data.if_stmt.condition) {
                c99bin_diagnostic_error(node->line, node->column,
                                      "If statement missing condition",
                                      "Add a boolean condition");
            } else {
                c99bin_check_ast_node(node->data.if_stmt.condition, depth + 1);
            }

            if (node->data.if_stmt.then_branch) {
                c99bin_check_ast_node(node->data.if_stmt.then_branch, depth + 1);
            }

            if (node->data.if_stmt.else_branch) {
                c99bin_check_ast_node(node->data.if_stmt.else_branch, depth + 1);
            }
            break;
        }

        case ASTC_RETURN_STMT: {
            // 检查return语句
            if (node->data.return_stmt.value) {
                c99bin_check_ast_node(node->data.return_stmt.value, depth + 1);
            }
            break;
        }

        case ASTC_COMPOUND_STMT: {
            // 检查复合语句
            for (int i = 0; i < node->data.compound_stmt.statement_count; i++) {
                if (node->data.compound_stmt.statements[i]) {
                    c99bin_check_ast_node(node->data.compound_stmt.statements[i], depth + 1);
                }
            }
            break;
        }

        case ASTC_EXPR_IDENTIFIER: {
            // 检查标识符
            if (!node->data.identifier.name) {
                c99bin_diagnostic_error(node->line, node->column,
                                      "Empty identifier",
                                      "Provide a valid identifier name");
            } else {
                const char* name = node->data.identifier.name;

                // 检查标识符长度
                if (strlen(name) > 63) {
                    c99bin_diagnostic_warning(node->line, node->column,
                                            "Identifier name is very long (>63 characters)");
                }

                // 检查是否使用了C关键字作为标识符
                if (strcmp(name, "auto") == 0 || strcmp(name, "break") == 0 ||
                    strcmp(name, "case") == 0 || strcmp(name, "char") == 0 ||
                    strcmp(name, "const") == 0 || strcmp(name, "continue") == 0) {
                    c99bin_diagnostic_error(node->line, node->column,
                                          "Using C keyword as identifier",
                                          "Choose a different identifier name");
                }
            }
            break;
        }

        default:
            // 对于其他节点类型，暂时不进行特殊检查
            break;
    }
}

// ===============================================
// T3.2.3 - 调试信息生成系统实现
// ===============================================

/**
 * 初始化调试信息系统
 * @param filename 源文件名
 * @param enable_debug 是否启用调试信息
 */
static void c99bin_debug_init(const char* filename, bool enable_debug) {
    // 清理之前的状态
    c99bin_debug_cleanup();

    // 设置基本信息
    if (filename) {
        c99bin_debug_info.filename = strdup(filename);
    }

    c99bin_debug_info.debug_enabled = enable_debug;
    c99bin_debug_info.generate_symbols = enable_debug;
    c99bin_debug_info.generate_line_numbers = enable_debug;

    // 初始化数组
    c99bin_debug_info.function_names = NULL;
    c99bin_debug_info.function_lines = NULL;
    c99bin_debug_info.function_count = 0;

    c99bin_debug_info.source_lines = NULL;
    c99bin_debug_info.code_offsets = NULL;
    c99bin_debug_info.line_mapping_count = 0;

    if (enable_debug) {
        printf("C99Bin Debug: Debug information generation enabled for %s\n",
               filename ? filename : "<unknown>");
    }
}

/**
 * 清理调试信息系统
 */
static void c99bin_debug_cleanup(void) {
    if (c99bin_debug_info.filename) {
        free(c99bin_debug_info.filename);
        c99bin_debug_info.filename = NULL;
    }

    // 清理函数信息
    if (c99bin_debug_info.function_names) {
        for (int i = 0; i < c99bin_debug_info.function_count; i++) {
            if (c99bin_debug_info.function_names[i]) {
                free(c99bin_debug_info.function_names[i]);
            }
        }
        free(c99bin_debug_info.function_names);
        c99bin_debug_info.function_names = NULL;
    }

    if (c99bin_debug_info.function_lines) {
        free(c99bin_debug_info.function_lines);
        c99bin_debug_info.function_lines = NULL;
    }

    // 清理行号映射
    if (c99bin_debug_info.source_lines) {
        free(c99bin_debug_info.source_lines);
        c99bin_debug_info.source_lines = NULL;
    }

    if (c99bin_debug_info.code_offsets) {
        free(c99bin_debug_info.code_offsets);
        c99bin_debug_info.code_offsets = NULL;
    }

    c99bin_debug_info.function_count = 0;
    c99bin_debug_info.line_mapping_count = 0;
    c99bin_debug_info.debug_enabled = false;
}

/**
 * 添加函数符号信息
 * @param name 函数名
 * @param line 函数起始行号
 */
static void c99bin_debug_add_function(const char* name, int line) {
    if (!c99bin_debug_info.debug_enabled || !name) {
        return;
    }

    // 扩展数组
    c99bin_debug_info.function_names = realloc(c99bin_debug_info.function_names,
                                              (c99bin_debug_info.function_count + 1) * sizeof(char*));
    c99bin_debug_info.function_lines = realloc(c99bin_debug_info.function_lines,
                                              (c99bin_debug_info.function_count + 1) * sizeof(int));

    if (!c99bin_debug_info.function_names || !c99bin_debug_info.function_lines) {
        printf("C99Bin Debug: ⚠️ Memory allocation failed for function symbol\n");
        return;
    }

    // 添加函数信息
    c99bin_debug_info.function_names[c99bin_debug_info.function_count] = strdup(name);
    c99bin_debug_info.function_lines[c99bin_debug_info.function_count] = line;
    c99bin_debug_info.function_count++;

    printf("C99Bin Debug: Added function symbol: %s at line %d\n", name, line);
}

/**
 * 添加行号映射
 * @param source_line 源代码行号
 * @param code_offset 代码偏移
 */
static void c99bin_debug_add_line_mapping(int source_line, size_t code_offset) {
    if (!c99bin_debug_info.debug_enabled) {
        return;
    }

    // 扩展数组
    c99bin_debug_info.source_lines = realloc(c99bin_debug_info.source_lines,
                                            (c99bin_debug_info.line_mapping_count + 1) * sizeof(int));
    c99bin_debug_info.code_offsets = realloc(c99bin_debug_info.code_offsets,
                                            (c99bin_debug_info.line_mapping_count + 1) * sizeof(size_t));

    if (!c99bin_debug_info.source_lines || !c99bin_debug_info.code_offsets) {
        printf("C99Bin Debug: ⚠️ Memory allocation failed for line mapping\n");
        return;
    }

    // 添加映射信息
    c99bin_debug_info.source_lines[c99bin_debug_info.line_mapping_count] = source_line;
    c99bin_debug_info.code_offsets[c99bin_debug_info.line_mapping_count] = code_offset;
    c99bin_debug_info.line_mapping_count++;

    printf("C99Bin Debug: Added line mapping: line %d -> offset 0x%zx\n", source_line, code_offset);
}

/**
 * 从AST生成符号信息
 * @param ast AST根节点
 */
static void c99bin_debug_generate_symbols(ASTNode* ast) {
    if (!c99bin_debug_info.debug_enabled || !ast) {
        return;
    }

    printf("C99Bin Debug: Generating symbol information from AST...\n");

    // 递归遍历AST收集符号信息
    c99bin_debug_collect_symbols(ast);

    printf("C99Bin Debug: Symbol generation completed (%d functions found)\n",
           c99bin_debug_info.function_count);
}

/**
 * 递归收集符号信息
 * @param node AST节点
 */
static void c99bin_debug_collect_symbols(ASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case ASTC_FUNC_DECL: {
            // 收集函数符号
            if (node->data.func_decl.name) {
                c99bin_debug_add_function(node->data.func_decl.name, node->line);
            }

            // 递归处理函数体
            if (node->data.func_decl.body) {
                c99bin_debug_collect_symbols(node->data.func_decl.body);
            }
            break;
        }

        case ASTC_COMPOUND_STMT: {
            // 处理复合语句中的所有语句
            for (int i = 0; i < node->data.compound_stmt.statement_count; i++) {
                if (node->data.compound_stmt.statements[i]) {
                    c99bin_debug_collect_symbols(node->data.compound_stmt.statements[i]);
                }
            }
            break;
        }

        case ASTC_IF_STMT: {
            // 处理if语句的各个分支
            if (node->data.if_stmt.condition) {
                c99bin_debug_collect_symbols(node->data.if_stmt.condition);
            }
            if (node->data.if_stmt.then_branch) {
                c99bin_debug_collect_symbols(node->data.if_stmt.then_branch);
            }
            if (node->data.if_stmt.else_branch) {
                c99bin_debug_collect_symbols(node->data.if_stmt.else_branch);
            }
            break;
        }

        case ASTC_BINARY_OP: {
            // 处理二元操作的操作数
            if (node->data.binary_op.left) {
                c99bin_debug_collect_symbols(node->data.binary_op.left);
            }
            if (node->data.binary_op.right) {
                c99bin_debug_collect_symbols(node->data.binary_op.right);
            }
            break;
        }

        case ASTC_RETURN_STMT: {
            // 处理return语句的值
            if (node->data.return_stmt.value) {
                c99bin_debug_collect_symbols(node->data.return_stmt.value);
            }
            break;
        }

        default:
            // 对于其他节点类型，暂时不处理
            break;
    }
}

/**
 * 写入调试信息到文件
 * @param output_file 输出文件路径
 */
static void c99bin_debug_write_info(const char* output_file) {
    if (!c99bin_debug_info.debug_enabled || !output_file) {
        return;
    }

    // 生成调试信息文件名
    char debug_file[512];
    snprintf(debug_file, sizeof(debug_file), "%s.debug", output_file);

    FILE* fp = fopen(debug_file, "w");
    if (!fp) {
        printf("C99Bin Debug: ⚠️ Failed to create debug file: %s\n", debug_file);
        return;
    }

    printf("C99Bin Debug: Writing debug information to %s\n", debug_file);

    // 写入文件头
    fprintf(fp, "# C99Bin Debug Information\n");
    fprintf(fp, "# Generated for: %s\n", c99bin_debug_info.filename ? c99bin_debug_info.filename : "<unknown>");
    fprintf(fp, "# Format: Simple text-based debug info\n\n");

    // 写入符号表
    if (c99bin_debug_info.function_count > 0) {
        fprintf(fp, "[SYMBOLS]\n");
        for (int i = 0; i < c99bin_debug_info.function_count; i++) {
            fprintf(fp, "FUNCTION %s %d\n",
                   c99bin_debug_info.function_names[i],
                   c99bin_debug_info.function_lines[i]);
        }
        fprintf(fp, "\n");
    }

    // 写入行号映射
    if (c99bin_debug_info.line_mapping_count > 0) {
        fprintf(fp, "[LINE_NUMBERS]\n");
        for (int i = 0; i < c99bin_debug_info.line_mapping_count; i++) {
            fprintf(fp, "LINE %d 0x%zx\n",
                   c99bin_debug_info.source_lines[i],
                   c99bin_debug_info.code_offsets[i]);
        }
        fprintf(fp, "\n");
    }

    // 写入统计信息
    fprintf(fp, "[STATISTICS]\n");
    fprintf(fp, "FUNCTIONS %d\n", c99bin_debug_info.function_count);
    fprintf(fp, "LINE_MAPPINGS %d\n", c99bin_debug_info.line_mapping_count);

    fclose(fp);

    printf("C99Bin Debug: ✅ Debug information written successfully\n");
    printf("C99Bin Debug: - %d function symbols\n", c99bin_debug_info.function_count);
    printf("C99Bin Debug: - %d line mappings\n", c99bin_debug_info.line_mapping_count);
}

// ===============================================
// T3.2.4 - 编译性能优化系统实现
// ===============================================

/**
 * 初始化性能优化系统
 * @param enable_performance_mode 是否启用性能模式
 */
static void c99bin_performance_init(bool enable_performance_mode) {
    memset(&c99bin_performance, 0, sizeof(C99BinPerformance));

    c99bin_performance.performance_mode = enable_performance_mode;
    c99bin_performance.enable_caching = enable_performance_mode;
    c99bin_performance.enable_parallel = false; // 暂时禁用并行编译
    c99bin_performance.max_threads = 1;

    // 设置缓存目录
    c99bin_performance.cache_dir = strdup(".c99bin_cache");

    if (enable_performance_mode) {
        printf("C99Bin Performance: Performance optimization enabled\n");
        printf("C99Bin Performance: Caching: %s\n", c99bin_performance.enable_caching ? "ON" : "OFF");
        printf("C99Bin Performance: Parallel: %s\n", c99bin_performance.enable_parallel ? "ON" : "OFF");

        // 创建缓存目录
        #ifdef _WIN32
        _mkdir(c99bin_performance.cache_dir);
        #else
        mkdir(c99bin_performance.cache_dir, 0755);
        #endif
    }
}

/**
 * 清理性能优化系统
 */
static void c99bin_performance_cleanup(void) {
    if (c99bin_performance.cache_dir) {
        free(c99bin_performance.cache_dir);
        c99bin_performance.cache_dir = NULL;
    }

    memset(&c99bin_performance, 0, sizeof(C99BinPerformance));
}

/**
 * 开始计时
 */
static void c99bin_performance_start_timer(void) {
    c99bin_performance.start_time = clock();
    c99bin_performance_update_memory();
}

/**
 * 结束计时
 */
static void c99bin_performance_end_timer(void) {
    c99bin_performance.end_time = clock();
    c99bin_performance_update_memory();
}

/**
 * 开始阶段计时
 * @param phase_name 阶段名称
 */
static void c99bin_performance_start_phase(const char* phase_name) {
    if (!c99bin_performance.performance_mode) return;

    printf("C99Bin Performance: Starting phase: %s\n", phase_name);
    // 这里可以记录具体阶段的开始时间
}

/**
 * 结束阶段计时
 * @param phase_name 阶段名称
 */
static void c99bin_performance_end_phase(const char* phase_name) {
    if (!c99bin_performance.performance_mode) return;

    printf("C99Bin Performance: Completed phase: %s\n", phase_name);
    // 这里可以记录具体阶段的结束时间
}

/**
 * 更新内存使用统计
 */
static void c99bin_performance_update_memory(void) {
    if (!c99bin_performance.performance_mode) return;

    // 简化的内存统计 - 在实际实现中可以使用更精确的方法
    #ifdef __linux__
    FILE* status = fopen("/proc/self/status", "r");
    if (status) {
        char line[256];
        while (fgets(line, sizeof(line), status)) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                size_t memory_kb = 0;
                sscanf(line, "VmRSS: %zu kB", &memory_kb);
                c99bin_performance.current_memory = memory_kb * 1024;
                if (c99bin_performance.current_memory > c99bin_performance.peak_memory) {
                    c99bin_performance.peak_memory = c99bin_performance.current_memory;
                }
                break;
            }
        }
        fclose(status);
    }
    #endif
}

/**
 * 打印性能统计
 */
static void c99bin_performance_print_stats(void) {
    if (!c99bin_performance.performance_mode) return;

    double compile_time = (double)(c99bin_performance.end_time - c99bin_performance.start_time) / CLOCKS_PER_SEC;

    printf("\n=== C99Bin Performance Statistics ===\n");
    printf("Total compilation time: %.3f seconds\n", compile_time);
    printf("Peak memory usage: %.2f MB\n", (double)c99bin_performance.peak_memory / (1024 * 1024));
    printf("Current memory usage: %.2f MB\n", (double)c99bin_performance.current_memory / (1024 * 1024));

    if (c99bin_performance.enable_caching) {
        printf("Cache hits: %d\n", c99bin_performance.cache_hits);
        printf("Cache misses: %d\n", c99bin_performance.cache_misses);
        if (c99bin_performance.cache_hits + c99bin_performance.cache_misses > 0) {
            double hit_rate = (double)c99bin_performance.cache_hits /
                             (c99bin_performance.cache_hits + c99bin_performance.cache_misses) * 100.0;
            printf("Cache hit rate: %.1f%%\n", hit_rate);
        }
    }

    printf("Performance mode: %s\n", c99bin_performance.performance_mode ? "ENABLED" : "DISABLED");
    printf("=====================================\n");
}

/**
 * 检查缓存
 * @param source_file 源文件路径
 * @param output_file 输出文件路径
 * @return 是否找到有效缓存
 */
static bool c99bin_performance_check_cache(const char* source_file, const char* output_file) {
    if (!c99bin_performance.enable_caching || !source_file || !output_file) {
        return false;
    }

    // 生成缓存文件路径
    char cache_file[512];
    snprintf(cache_file, sizeof(cache_file), "%s/%s.cache",
             c99bin_performance.cache_dir, source_file);

    // 检查缓存文件是否存在且比源文件新
    struct stat source_stat, cache_stat;
    if (stat(source_file, &source_stat) != 0) {
        return false;
    }

    if (stat(cache_file, &cache_stat) != 0) {
        c99bin_performance.cache_misses++;
        return false;
    }

    // 检查缓存是否比源文件新
    if (cache_stat.st_mtime > source_stat.st_mtime) {
        printf("C99Bin Performance: Cache hit for %s\n", source_file);
        c99bin_performance.cache_hits++;

        // 复制缓存文件到输出文件
        FILE* cache_fp = fopen(cache_file, "rb");
        FILE* output_fp = fopen(output_file, "wb");

        if (cache_fp && output_fp) {
            char buffer[4096];
            size_t bytes;
            while ((bytes = fread(buffer, 1, sizeof(buffer), cache_fp)) > 0) {
                fwrite(buffer, 1, bytes, output_fp);
            }
            fclose(cache_fp);
            fclose(output_fp);
            return true;
        }

        if (cache_fp) fclose(cache_fp);
        if (output_fp) fclose(output_fp);
    }

    c99bin_performance.cache_misses++;
    return false;
}

/**
 * 保存到缓存
 * @param source_file 源文件路径
 * @param output_file 输出文件路径
 */
static void c99bin_performance_save_cache(const char* source_file, const char* output_file) {
    if (!c99bin_performance.enable_caching || !source_file || !output_file) {
        return;
    }

    // 生成缓存文件路径
    char cache_file[512];
    snprintf(cache_file, sizeof(cache_file), "%s/%s.cache",
             c99bin_performance.cache_dir, source_file);

    // 复制输出文件到缓存
    FILE* output_fp = fopen(output_file, "rb");
    FILE* cache_fp = fopen(cache_file, "wb");

    if (output_fp && cache_fp) {
        char buffer[4096];
        size_t bytes;
        while ((bytes = fread(buffer, 1, sizeof(buffer), output_fp)) > 0) {
            fwrite(buffer, 1, bytes, cache_fp);
        }
        printf("C99Bin Performance: Cached result for %s\n", source_file);
    }

    if (output_fp) fclose(output_fp);
    if (cache_fp) fclose(cache_fp);
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
        // 实现AST清理函数 - 递归释放AST节点
        c99bin_free_ast_node(c99bin_state.ast_root);
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

// AST节点递归清理函数
static void c99bin_free_ast_node(struct ASTNode* node) {
    if (!node) return;
    
    // 根据节点类型递归清理子节点
    switch (node->type) {
        case AST_BINARY_OP:
            if (node->data.binary_op.left) {
                c99bin_free_ast_node(node->data.binary_op.left);
            }
            if (node->data.binary_op.right) {
                c99bin_free_ast_node(node->data.binary_op.right);
            }
            break;
            
        case AST_UNARY_OP:
            if (node->data.unary_op.operand) {
                c99bin_free_ast_node(node->data.unary_op.operand);
            }
            break;
            
        case AST_IF_STMT:
            if (node->data.if_stmt.condition) {
                c99bin_free_ast_node(node->data.if_stmt.condition);
            }
            if (node->data.if_stmt.then_branch) {
                c99bin_free_ast_node(node->data.if_stmt.then_branch);
            }
            if (node->data.if_stmt.else_branch) {
                c99bin_free_ast_node(node->data.if_stmt.else_branch);
            }
            break;
            
        case AST_COMPOUND_STMT:
            if (node->data.compound_stmt.statements) {
                for (int i = 0; i < node->data.compound_stmt.statement_count; i++) {
                    c99bin_free_ast_node(node->data.compound_stmt.statements[i]);
                }
                free(node->data.compound_stmt.statements);
            }
            break;
            
        case AST_IDENTIFIER:
            if (node->data.identifier.name) {
                free(node->data.identifier.name);
            }
            break;
            
        default:
            // 其他节点类型暂时不需要特殊清理
            break;
    }
    
    // 释放节点本身
    free(node);
}

// 测试导出函数
int test_export_function(void) {
    printf("C99Bin Module: test_export_function called\n");
    return 99;  // 返回99表示c99bin模块
}
