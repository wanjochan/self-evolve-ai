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
// 核心编译接口
// ===============================================

/**
 * 编译C源码到可执行文件
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

    // T2.1 - 集成pipeline前端解析
    if (c99bin_state.pipeline_module) {
        printf("C99Bin Module: Using pipeline frontend for parsing...\n");

        // 调用pipeline模块的前端编译函数
        void* frontend_compile = c99bin_state.pipeline_module->sym(c99bin_state.pipeline_module, "frontend_compile");
        if (frontend_compile) {
            printf("C99Bin Module: Found frontend_compile function\n");
            // TODO: 调用frontend_compile并获取AST
        } else {
            printf("C99Bin Module: frontend_compile function not found\n");
        }
    } else {
        printf("C99Bin Module: Pipeline module not available, using fallback parser\n");
        // TODO: 实现简单的fallback解析器
    }

    // T3.1 - 实现AST到机器码的直接生成
    if (c99bin_state.compiler_module) {
        printf("C99Bin Module: Using compiler JIT for code generation...\n");

        // 调用compiler模块的JIT编译函数
        void* jit_compile = c99bin_state.compiler_module->sym(c99bin_state.compiler_module, "jit_compile");
        if (jit_compile) {
            printf("C99Bin Module: Found jit_compile function\n");
            // TODO: 调用jit_compile生成机器码
        } else {
            printf("C99Bin Module: jit_compile function not found\n");
        }
    } else {
        printf("C99Bin Module: Compiler module not available, using fallback codegen\n");
        // TODO: 实现简单的fallback代码生成器
    }

    // T4.1 - 实现ELF文件格式生成
    printf("C99Bin Module: Generating ELF executable...\n");
    // TODO: 实现ELF文件生成

    strcpy(c99bin_state.error_message, "Implementation in progress");
    return C99BIN_ERROR_CODEGEN_FAILED;
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
    
    printf("C99Bin Module: Generating ELF file %s (%zu bytes)\n", output_file, code_size);
    
    // TODO: T4.1 - 实现完整的ELF文件格式生成
    // 1. ELF文件头
    // 2. 程序头表
    // 3. 代码段
    // 4. 入口点设置
    
    strcpy(c99bin_state.error_message, "ELF generation not implemented yet");
    return C99BIN_ERROR_CODEGEN_FAILED;
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
    
    // 清理分配的内存
    if (c99bin_state.source_code) {
        free(c99bin_state.source_code);
        c99bin_state.source_code = NULL;
    }
    
    if (c99bin_state.machine_code) {
        free(c99bin_state.machine_code);
        c99bin_state.machine_code = NULL;
    }
    
    // 重置状态
    memset(&c99bin_state, 0, sizeof(C99BinState));
    
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
