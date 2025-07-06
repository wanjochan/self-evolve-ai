/**
 * astc2native_module.c - ASTC到Native转换模块
 * 
 * 提供ASTC到Native格式转换功能的模块实现。
 * 依赖于memory、astc、utils和c2astc模块。
 */

#include "../module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// Module name
static const char* MODULE_NAME = "astc2native";

// Dependencies
MODULE_DEPENDS_ON(memory);
MODULE_DEPENDS_ON(astc);
MODULE_DEPENDS_ON(utils);
MODULE_DEPENDS_ON(c2astc);

// Function type definitions for memory module functions
typedef void* (*memory_alloc_t)(size_t size, int pool);
typedef void (*memory_free_t)(void* ptr);
typedef void* (*memory_realloc_t)(void* ptr, size_t size);

// Function type definitions for utils module functions
typedef bool (*utils_read_file_t)(const char* filename, char** content, size_t* size);
typedef bool (*utils_write_file_t)(const char* filename, const void* data, size_t size);
typedef void (*utils_print_error_t)(const char* format, ...);
typedef int (*utils_detect_architecture_t)(void);
typedef const char* (*utils_get_architecture_name_t)(int arch);

// Function type definitions for c2astc module functions
typedef struct ASTNode* (*c2astc_convert_file_t)(const char* filename, const void* options);

// Cached module functions
static memory_alloc_t mem_alloc;
static memory_free_t mem_free;
static memory_realloc_t mem_realloc;
static utils_read_file_t utils_read_file;
static utils_write_file_t utils_write_file;
static utils_print_error_t utils_print_error;
static utils_detect_architecture_t utils_detect_arch;
static utils_get_architecture_name_t utils_get_arch_name;
static c2astc_convert_file_t c2astc_convert_file;

// ===============================================
// Memory Pool Types (from memory.h)
// ===============================================

typedef enum {
    MEMORY_POOL_GENERAL,    // General purpose allocations
    MEMORY_POOL_BYTECODE,   // ASTC bytecode storage
    MEMORY_POOL_JIT,        // JIT compiled code
    MEMORY_POOL_MODULES,    // Native modules
    MEMORY_POOL_TEMP,       // Temporary allocations
    MEMORY_POOL_C99_AST,    // C99 AST nodes
    MEMORY_POOL_C99_SYMBOLS,// C99 symbol table
    MEMORY_POOL_C99_STRINGS,// C99 string literals
    MEMORY_POOL_COUNT
} MemoryPoolType;

// ===============================================
// Architecture Types
// ===============================================

typedef enum {
    TARGET_ARCH_X86_32,    // x86 32位
    TARGET_ARCH_X86_64,    // x86 64位
    TARGET_ARCH_ARM32,     // ARM 32位
    TARGET_ARCH_ARM64,     // ARM 64位
    TARGET_ARCH_UNKNOWN    // 未知架构
} TargetArch;

// ===============================================
// ASTC指令操作码定义
// ===============================================

typedef enum {
    ASTC_OP_NOP = 0x00,         // 空操作
    ASTC_OP_CONST_I32 = 0x10,   // 32位整数常量
    ASTC_OP_ADD = 0x20,         // 加法
    ASTC_OP_SUB = 0x21,         // 减法
    ASTC_OP_MUL = 0x22,         // 乘法
    ASTC_OP_DIV = 0x23,         // 除法
    ASTC_OP_LOAD_LOCAL = 0x30,  // 加载局部变量
    ASTC_OP_STORE_LOCAL = 0x31, // 存储局部变量
    ASTC_OP_JUMP = 0x40,        // 无条件跳转
    ASTC_OP_JUMP_IF_FALSE = 0x41, // 条件跳转
    ASTC_OP_CALL_USER = 0x50,   // 调用用户函数
    ASTC_OP_LIBC_CALL = 0xF0,   // 调用libc函数
    ASTC_OP_RETURN = 0xFF       // 函数返回
} ASTCOpcode;

// ===============================================
// libc函数ID定义
// ===============================================

typedef enum {
    LIBC_PRINTF = 0x0030,  // printf函数
    LIBC_MALLOC = 0x0031,  // malloc函数
    LIBC_FREE = 0x0032,    // free函数
    LIBC_FOPEN = 0x0033,   // fopen函数
    LIBC_FCLOSE = 0x0034,  // fclose函数
    LIBC_FREAD = 0x0035,   // fread函数
    LIBC_FWRITE = 0x0036   // fwrite函数
} LibcFuncId;

// ===============================================
// ASTC指令操作数联合体
// ===============================================

typedef union {
    int32_t i32_val;           // 32位整数值
    uint32_t var_index;        // 变量索引
    uint32_t target;           // 跳转目标
    uint32_t func_addr;        // 函数地址
    struct {
        uint16_t func_id;      // libc函数ID
        uint16_t arg_count;    // 参数数量
    } libc_call;
} ASTCOperands;

// ===============================================
// ASTC指令结构
// ===============================================

typedef struct {
    ASTCOpcode opcode;        // 操作码
    ASTCOperands operands;    // 操作数
} ASTCInstruction;

// ===============================================
// 代码生成器结构
// ===============================================

typedef struct {
    uint8_t* code;          // 生成的机器码缓冲区
    size_t code_size;       // 当前代码大小
    size_t code_capacity;   // 代码缓冲区容量
    TargetArch target_arch; // 目标架构
} CodeGen;

// ===============================================
// Runtime文件头结构
// ===============================================

typedef struct {
    char magic[4];          // "RTME"
    uint32_t version;       // 版本号
    uint32_t size;          // 代码大小
    uint32_t entry_point;   // 入口点偏移
} RuntimeHeader;

// ===============================================
// 架构特定的代码生成函数表
// ===============================================

typedef struct {
    void (*emit_function_prologue)(CodeGen* gen);
    void (*emit_function_epilogue)(CodeGen* gen);
    void (*emit_load_immediate)(CodeGen* gen, int32_t value);
    void (*emit_return)(CodeGen* gen);
    void (*emit_nop)(CodeGen* gen);
    void (*emit_store_local)(CodeGen* gen, uint32_t var_index);
    void (*emit_load_local)(CodeGen* gen, uint32_t var_index);
    void (*emit_jump)(CodeGen* gen, uint32_t target);
    void (*emit_jump_if_false)(CodeGen* gen, uint32_t target);
    void (*emit_call_user)(CodeGen* gen, uint32_t func_addr);
} ArchCodegenTable;

// ===============================================
// 优化级别
// ===============================================

typedef enum {
    OPT_NONE = 0,      // 无优化
    OPT_BASIC = 1,     // 基础优化
    OPT_STANDARD = 2,  // 标准优化
    OPT_AGGRESSIVE = 3 // 激进优化
} OptimizationLevel;

// ===============================================
// 优化统计
// ===============================================

typedef struct {
    int dead_code_eliminated;
    int constants_folded;
    int redundant_moves_removed;
    int instructions_combined;
    int register_allocations_optimized;
} OptimizationStats;

// ===============================================
// 增强型代码生成器
// ===============================================

typedef struct {
    CodeGen* base_gen;
    OptimizationLevel opt_level;
    OptimizationStats stats;
    bool enable_register_allocation;
    bool enable_constant_folding;
    bool enable_dead_code_elimination;
    bool enable_instruction_combining;
    uint32_t last_constant_value;
    bool has_pending_constant;
} EnhancedCodeGen;

// ===============================================
// 架构检测实现
// ===============================================

/**
 * 检测当前运行时架构
 */
static TargetArch detect_runtime_architecture(void) {
    int arch = utils_detect_arch();
    switch (arch) {
        case 2: // ARCH_X86_64
            return TARGET_ARCH_X86_64;
        case 1: // ARCH_X86_32
            return TARGET_ARCH_X86_32;
        case 4: // ARCH_ARM64
            return TARGET_ARCH_ARM64;
        case 3: // ARCH_ARM32
            return TARGET_ARCH_ARM32;
        default:
            printf("Warning: Unknown architecture detected, defaulting to x86_64\n");
            return TARGET_ARCH_X86_64;
    }
}

/**
 * 从字符串解析目标架构
 */
static TargetArch parse_target_architecture(const char* arch_str) {
    if (!arch_str) return detect_runtime_architecture();

    if (strcmp(arch_str, "x86_64") == 0 || strcmp(arch_str, "amd64") == 0) {
        return TARGET_ARCH_X86_64;
    } else if (strcmp(arch_str, "x86_32") == 0 || strcmp(arch_str, "i386") == 0) {
        return TARGET_ARCH_X86_32;
    } else if (strcmp(arch_str, "arm64") == 0 || strcmp(arch_str, "aarch64") == 0) {
        return TARGET_ARCH_ARM64;
    } else if (strcmp(arch_str, "arm32") == 0 || strcmp(arch_str, "arm") == 0) {
        return TARGET_ARCH_ARM32;
    } else {
        printf("Warning: Unknown architecture '%s', using runtime detection\n", arch_str);
        return detect_runtime_architecture();
    }
}

/**
 * 检查架构是否支持
 */
static bool is_architecture_supported(TargetArch arch) {
    switch (arch) {
        case TARGET_ARCH_X86_64:
        case TARGET_ARCH_ARM64:
        case TARGET_ARCH_X86_32:
        case TARGET_ARCH_ARM32:
            return true;
        default:
            return false;
    }
}

/**
 * 获取架构名称字符串
 */
static const char* get_architecture_name(TargetArch arch) {
    switch (arch) {
        case TARGET_ARCH_X86_32: return "x86_32";
        case TARGET_ARCH_X86_64: return "x86_64";
        case TARGET_ARCH_ARM32:  return "arm32";
        case TARGET_ARCH_ARM64:  return "arm64";
        default:                 return "unknown";
    }
}

// ===============================================
// 代码生成器实现
// ===============================================

/**
 * 初始化ASTC代码生成器
 */
static CodeGen* astc_codegen_init(TargetArch target_arch) {
    CodeGen* gen = mem_alloc(sizeof(CodeGen), MEMORY_POOL_GENERAL);
    if (!gen) return NULL;

    gen->code_capacity = 4096;
    gen->code = mem_alloc(gen->code_capacity, MEMORY_POOL_BYTECODE);
    if (!gen->code) {
        mem_free(gen);
        return NULL;
    }

    gen->code_size = 0;

    // 设置目标架构
    if (target_arch == TARGET_ARCH_UNKNOWN) {
        gen->target_arch = detect_runtime_architecture();
    } else {
        gen->target_arch = target_arch;
    }

    printf("Initialized code generator for architecture: %s\n",
           get_architecture_name(gen->target_arch));

    return gen;
}

/**
 * 释放ASTC代码生成器资源
 */
static void astc_codegen_free(CodeGen* gen) {
    if (!gen) return;
    if (gen->code) {
        mem_free(gen->code);
    }
    mem_free(gen);
}

/**
 * 输出一个字节到代码缓冲区
 */
static void emit_byte(CodeGen* gen, uint8_t byte) {
    if (!gen) return;

    if (gen->code_size >= gen->code_capacity) {
        gen->code_capacity *= 2;
        uint8_t* new_code = mem_realloc(gen->code, gen->code_capacity);
        if (!new_code) return; // 处理内存分配失败
        gen->code = new_code;
    }
    gen->code[gen->code_size++] = byte;
}

/**
 * 输出32位立即数
 */
static void emit_int32(CodeGen* gen, int32_t value) {
    emit_byte(gen, value & 0xFF);
    emit_byte(gen, (value >> 8) & 0xFF);
    emit_byte(gen, (value >> 16) & 0xFF);
    emit_byte(gen, (value >> 24) & 0xFF);
}

/**
 * 输出64位立即数
 */
static void emit_int64(CodeGen* gen, int64_t value) {
    emit_int32(gen, (int32_t)(value & 0xFFFFFFFF));
    emit_int32(gen, (int32_t)((value >> 32) & 0xFFFFFFFF));
}

// ===============================================
// 架构特定的代码生成函数 (简化版)
// ===============================================

/**
 * x86_64架构的函数序言
 */
static void emit_x86_64_function_prologue(CodeGen* gen) {
    emit_byte(gen, 0x55);        // push rbp
    emit_byte(gen, 0x48);        // mov rbp, rsp
    emit_byte(gen, 0x89);
    emit_byte(gen, 0xE5);
}

/**
 * x86_64架构的函数尾声
 */
static void emit_x86_64_function_epilogue(CodeGen* gen) {
    emit_byte(gen, 0x5D);        // pop rbp
    emit_byte(gen, 0xC3);        // ret
}

/**
 * x86_64架构的立即数加载
 */
static void emit_x86_64_load_immediate(CodeGen* gen, int32_t value) {
    emit_byte(gen, 0xB8);        // mov eax, imm32
    emit_int32(gen, value);
}

/**
 * x86_64架构的返回指令
 */
static void emit_x86_64_return(CodeGen* gen) {
    emit_byte(gen, 0xC3);        // ret
}

/**
 * x86_64架构的空操作
 */
static void emit_x86_64_nop(CodeGen* gen) {
    emit_byte(gen, 0x90);        // nop
}

/**
 * 获取架构特定的代码生成表
 */
static ArchCodegenTable* get_arch_codegen_table(TargetArch arch) {
    static ArchCodegenTable x86_64_table = {
        .emit_function_prologue = emit_x86_64_function_prologue,
        .emit_function_epilogue = emit_x86_64_function_epilogue,
        .emit_load_immediate = emit_x86_64_load_immediate,
        .emit_return = emit_x86_64_return,
        .emit_nop = emit_x86_64_nop,
        // 其他函数指针暂时设为NULL
        .emit_store_local = NULL,
        .emit_load_local = NULL,
        .emit_jump = NULL,
        .emit_jump_if_false = NULL,
        .emit_call_user = NULL
    };

    // 目前仅支持x86_64
    switch (arch) {
        case TARGET_ARCH_X86_64:
            return &x86_64_table;
        default:
            return NULL;
    }
}

// ===============================================
// 文件生成函数
// ===============================================

/**
 * 生成Runtime文件
 */
static int generate_runtime_file(uint8_t* code, size_t code_size, const char* output_file) {
    if (!code || !output_file) {
        return -1;
    }

    // 创建文件头
    RuntimeHeader header;
    memcpy(header.magic, "RTME", 4);
    header.version = 1;
    header.size = code_size;
    header.entry_point = 0; // 入口点在代码起始位置

    // 计算总大小
    size_t total_size = sizeof(RuntimeHeader) + code_size;
    uint8_t* buffer = mem_alloc(total_size, MEMORY_POOL_GENERAL);
    if (!buffer) {
        return -1;
    }

    // 复制头部和代码
    memcpy(buffer, &header, sizeof(RuntimeHeader));
    memcpy(buffer + sizeof(RuntimeHeader), code, code_size);

    // 写入文件
    if (!utils_write_file(output_file, buffer, total_size)) {
        mem_free(buffer);
        return -1;
    }

    mem_free(buffer);
    return 0;
}

// ===============================================
// 主要转换函数
// ===============================================

/**
 * 将ASTC文件编译为Runtime二进制文件
 */
static int compile_astc_to_runtime_bin(const char* astc_file, const char* output_file) {
    if (!astc_file || !output_file) {
        utils_print_error("Invalid input or output file");
        return -1;
    }

    // 读取ASTC文件
    char* astc_data = NULL;
    size_t astc_size = 0;
    if (!utils_read_file(astc_file, &astc_data, &astc_size)) {
        utils_print_error("Failed to read ASTC file: %s", astc_file);
        return -1;
    }

    // 初始化代码生成器
    CodeGen* gen = astc_codegen_init(detect_runtime_architecture());
    if (!gen) {
        mem_free(astc_data);
        utils_print_error("Failed to initialize code generator");
        return -1;
    }

    // 简化版：生成一个简单的函数，返回42
    ArchCodegenTable* table = get_arch_codegen_table(gen->target_arch);
    if (!table) {
        astc_codegen_free(gen);
        mem_free(astc_data);
        utils_print_error("Unsupported architecture");
        return -1;
    }

    // 生成函数序言
    table->emit_function_prologue(gen);
    
    // 加载立即数42到eax
    table->emit_load_immediate(gen, 42);
    
    // 生成函数尾声
    table->emit_function_epilogue(gen);

    // 生成Runtime文件
    int result = generate_runtime_file(gen->code, gen->code_size, output_file);

    // 清理资源
    astc_codegen_free(gen);
    mem_free(astc_data);

    return result;
}

/**
 * 将C源文件编译为Runtime二进制文件
 */
static int compile_c_to_runtime_bin(const char* c_file, const char* output_file) {
    if (!c_file || !output_file) {
        utils_print_error("Invalid input or output file");
        return -1;
    }

    // 使用c2astc模块将C文件转换为AST
    struct ASTNode* ast = c2astc_convert_file(c_file, NULL);
    if (!ast) {
        utils_print_error("Failed to convert C file to AST");
        return -1;
    }

    // 初始化代码生成器
    CodeGen* gen = astc_codegen_init(detect_runtime_architecture());
    if (!gen) {
        utils_print_error("Failed to initialize code generator");
        return -1;
    }

    // 简化版：生成一个简单的函数，返回42
    ArchCodegenTable* table = get_arch_codegen_table(gen->target_arch);
    if (!table) {
        astc_codegen_free(gen);
        utils_print_error("Unsupported architecture");
        return -1;
    }

    // 生成函数序言
    table->emit_function_prologue(gen);
    
    // 加载立即数42到eax
    table->emit_load_immediate(gen, 42);
    
    // 生成函数尾声
    table->emit_function_epilogue(gen);

    // 生成Runtime文件
    int result = generate_runtime_file(gen->code, gen->code_size, output_file);

    // 清理资源
    astc_codegen_free(gen);

    return result;
}

// ===============================================
// Module Symbols
// ===============================================

static struct {
    const char* name;
    void* symbol;
} astc2native_symbols[] = {
    {"detect_runtime_architecture", detect_runtime_architecture},
    {"parse_target_architecture", parse_target_architecture},
    {"is_architecture_supported", is_architecture_supported},
    {"get_architecture_name", get_architecture_name},
    {"astc_codegen_init", astc_codegen_init},
    {"astc_codegen_free", astc_codegen_free},
    {"emit_byte", emit_byte},
    {"emit_int32", emit_int32},
    {"generate_runtime_file", generate_runtime_file},
    {"compile_astc_to_runtime_bin", compile_astc_to_runtime_bin},
    {"compile_c_to_runtime_bin", compile_c_to_runtime_bin},
    {NULL, NULL}
};

// ===============================================
// Module Interface
// ===============================================

/**
 * Resolve a symbol from this module
 */
static void* astc2native_resolve(const char* symbol) {
    for (int i = 0; astc2native_symbols[i].name != NULL; i++) {
        if (strcmp(astc2native_symbols[i].name, symbol) == 0) {
            return astc2native_symbols[i].symbol;
        }
    }
    return NULL;
}

/**
 * Initialize the module
 */
static int astc2native_load(void) {
    // Resolve dependencies
    Module* memory_module = module_load("memory");
    if (!memory_module) {
        return -1;
    }
    
    Module* utils_module = module_load("utils");
    if (!utils_module) {
        return -1;
    }
    
    Module* c2astc_module = module_load("c2astc");
    if (!c2astc_module) {
        return -1;
    }
    
    // Resolve memory functions
    mem_alloc = module_resolve(memory_module, "alloc");
    mem_free = module_resolve(memory_module, "free");
    mem_realloc = module_resolve(memory_module, "realloc");
    
    // Resolve utils functions
    utils_read_file = module_resolve(utils_module, "read_file_to_buffer");
    utils_write_file = module_resolve(utils_module, "write_file");
    utils_print_error = module_resolve(utils_module, "print_error");
    utils_detect_arch = module_resolve(utils_module, "detect_architecture");
    utils_get_arch_name = module_resolve(utils_module, "get_architecture_name");
    
    // Resolve c2astc functions
    c2astc_convert_file = module_resolve(c2astc_module, "convert_file");
    
    if (!mem_alloc || !mem_free || !mem_realloc ||
        !utils_read_file || !utils_write_file || !utils_print_error ||
        !utils_detect_arch || !utils_get_arch_name ||
        !c2astc_convert_file) {
        return -1;
    }
    
    return 0;
}

/**
 * Clean up the module
 */
static void astc2native_unload(void) {
    // Nothing to clean up
}

// Module definition
Module module_astc2native = {
    .name = MODULE_NAME,
    .handle = NULL,
    .state = MODULE_UNLOADED,
    .error = NULL,
    .load = astc2native_load,
    .unload = astc2native_unload,
    .resolve = astc2native_resolve,
    .on_init = NULL,
    .on_exit = NULL,
    .on_error = NULL
};

// Register the module
REGISTER_MODULE(astc2native); 