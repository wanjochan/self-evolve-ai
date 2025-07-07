/**
 * compiler_module.c - Compiler Module
 *
 * 编译器模块，整合了特殊编译方式：
 * - JIT: 即时编译，运行时将字节码编译为机器码
 * - FFI: 外部函数接口，与C库和系统API交互
 *
 * 注意：AOT编译功能已移至pipeline_module.c的backend部分
 */

#include "../module.h"
#include "../astc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

// Platform-specific includes
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

// ===============================================
// 模块信息
// ===============================================

#define MODULE_NAME "compiler"
#define MODULE_VERSION "1.0.0"
#define MODULE_DESCRIPTION "Integrated JIT/FFI compiler module"

// 依赖layer0模块 (通过动态加载)

// ===============================================
// 编译器类型和状态
// ===============================================

// 编译器类型
typedef enum {
    COMPILER_JIT,    // 即时编译器
    COMPILER_FFI     // 外部函数接口
} CompilerType;

// 目标架构
typedef enum {
    TARGET_X86_32,
    TARGET_X86_64,
    TARGET_ARM32,
    TARGET_ARM64,
    TARGET_UNKNOWN
} TargetArch;

// 优化级别
typedef enum {
    OPT_NONE = 0,
    OPT_BASIC = 1,
    OPT_STANDARD = 2,
    OPT_AGGRESSIVE = 3
} OptLevel;

// 编译状态
typedef enum {
    COMPILE_SUCCESS = 0,
    COMPILE_ERROR_INVALID_INPUT = -1,
    COMPILE_ERROR_UNSUPPORTED_ARCH = -2,
    COMPILE_ERROR_MEMORY_ALLOC = -3,
    COMPILE_ERROR_CODEGEN_FAILED = -4,
    COMPILE_ERROR_LINK_FAILED = -5,
    COMPILE_ERROR_FFI_FAILED = -6
} CompileResult;

// ===============================================
// JIT编译器实现
// ===============================================

// JIT编译器上下文
typedef struct {
    TargetArch target_arch;
    OptLevel opt_level;
    uint8_t* code_buffer;
    size_t code_size;
    size_t code_capacity;
    uint32_t* label_table;
    size_t label_count;
    char error_message[256];
} JITCompiler;

// JIT可执行代码块
typedef struct {
    void* code_ptr;
    size_t code_size;
    bool is_executable;
} JITCodeBlock;

// 创建JIT编译器
static JITCompiler* jit_create_compiler(TargetArch arch, OptLevel opt_level) {
    JITCompiler* jit = malloc(sizeof(JITCompiler));
    if (!jit) return NULL;
    
    jit->target_arch = arch;
    jit->opt_level = opt_level;
    jit->code_capacity = 4096;
    jit->code_buffer = malloc(jit->code_capacity);
    jit->code_size = 0;
    jit->label_count = 0;
    jit->label_table = malloc(64 * sizeof(uint32_t));
    jit->error_message[0] = '\0';
    
    if (!jit->code_buffer || !jit->label_table) {
        free(jit->code_buffer);
        free(jit->label_table);
        free(jit);
        return NULL;
    }
    
    return jit;
}

// 销毁JIT编译器
static void jit_destroy_compiler(JITCompiler* jit) {
    if (!jit) return;
    
    free(jit->code_buffer);
    free(jit->label_table);
    free(jit);
}

// 分配可执行内存
static void* allocate_executable_memory(size_t size) {
#ifdef _WIN32
    return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#else
    return mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, 
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
}

// 释放可执行内存
static void free_executable_memory(void* ptr, size_t size) {
    if (!ptr) return;
    
#ifdef _WIN32
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    munmap(ptr, size);
#endif
}

// JIT编译ASTC字节码
static CompileResult jit_compile_bytecode(JITCompiler* jit, const uint8_t* bytecode, 
                                        size_t bytecode_size, JITCodeBlock* result) {
    if (!jit || !bytecode || !result) {
        return COMPILE_ERROR_INVALID_INPUT;
    }
    
    // 简化的JIT编译实现
    // 这里生成简单的x86-64机器码
    if (jit->target_arch != TARGET_X86_64) {
        strcpy(jit->error_message, "Only x86-64 architecture supported");
        return COMPILE_ERROR_UNSUPPORTED_ARCH;
    }
    
    // 重置代码缓冲区
    jit->code_size = 0;
    
    // 生成函数序言
    uint8_t prologue[] = {
        0x55,                    // push rbp
        0x48, 0x89, 0xe5,        // mov rbp, rsp
        0x48, 0x83, 0xec, 0x20   // sub rsp, 32
    };
    
    if (jit->code_size + sizeof(prologue) > jit->code_capacity) {
        strcpy(jit->error_message, "Code buffer overflow");
        return COMPILE_ERROR_CODEGEN_FAILED;
    }
    
    memcpy(jit->code_buffer + jit->code_size, prologue, sizeof(prologue));
    jit->code_size += sizeof(prologue);
    
    // 处理字节码指令（增强版）
    for (size_t i = 0; i < bytecode_size; ) {
        uint8_t opcode = bytecode[i];

        switch (opcode) {
            case 0x00: // NOP
                i++;
                break;

            case 0x01: // HALT
                // 生成 exit 系统调用
                {
                    uint8_t exit_code[] = {
                        0x48, 0xc7, 0xc0, 0x3c, 0x00, 0x00, 0x00,  // mov rax, 60 (sys_exit)
                        0x48, 0xc7, 0xc7, 0x00, 0x00, 0x00, 0x00,  // mov rdi, 0 (exit status)
                        0x0f, 0x05                                  // syscall
                    };

                    if (jit->code_size + sizeof(exit_code) > jit->code_capacity) {
                        strcpy(jit->error_message, "Code buffer overflow");
                        return COMPILE_ERROR_CODEGEN_FAILED;
                    }

                    memcpy(jit->code_buffer + jit->code_size, exit_code, sizeof(exit_code));
                    jit->code_size += sizeof(exit_code);
                }
                i++;
                break;

            case 0x10: // LOAD_IMM
                if (i + 9 < bytecode_size) {
                    uint8_t reg = bytecode[i + 1];
                    int64_t value = *(int64_t*)(bytecode + i + 2);

                    // 生成 mov reg, immediate (64位)
                    if (reg == 0) { // rax
                        uint8_t load_imm[] = {0x48, 0xb8}; // mov rax, imm64

                        if (jit->code_size + sizeof(load_imm) + 8 > jit->code_capacity) {
                            strcpy(jit->error_message, "Code buffer overflow");
                            return COMPILE_ERROR_CODEGEN_FAILED;
                        }

                        memcpy(jit->code_buffer + jit->code_size, load_imm, sizeof(load_imm));
                        jit->code_size += sizeof(load_imm);
                        *(int64_t*)(jit->code_buffer + jit->code_size) = value;
                        jit->code_size += 8;
                    } else if (reg == 1) { // rbx
                        uint8_t load_imm[] = {0x48, 0xbb}; // mov rbx, imm64

                        if (jit->code_size + sizeof(load_imm) + 8 > jit->code_capacity) {
                            strcpy(jit->error_message, "Code buffer overflow");
                            return COMPILE_ERROR_CODEGEN_FAILED;
                        }

                        memcpy(jit->code_buffer + jit->code_size, load_imm, sizeof(load_imm));
                        jit->code_size += sizeof(load_imm);
                        *(int64_t*)(jit->code_buffer + jit->code_size) = value;
                        jit->code_size += 8;
                    }
                    i += 10;
                } else {
                    i++;
                }
                break;

            case 0x20: // ADD
                if (i + 3 < bytecode_size) {
                    uint8_t reg1 = bytecode[i + 1];
                    uint8_t reg2 = bytecode[i + 2];
                    uint8_t dst_reg = bytecode[i + 3];

                    // 简化实现：只支持 rax = rax + rbx
                    if (reg1 == 0 && reg2 == 1 && dst_reg == 0) {
                        uint8_t add_instr[] = {0x48, 0x01, 0xd8}; // add rax, rbx

                        if (jit->code_size + sizeof(add_instr) > jit->code_capacity) {
                            strcpy(jit->error_message, "Code buffer overflow");
                            return COMPILE_ERROR_CODEGEN_FAILED;
                        }

                        memcpy(jit->code_buffer + jit->code_size, add_instr, sizeof(add_instr));
                        jit->code_size += sizeof(add_instr);
                    }
                    i += 4;
                } else {
                    i++;
                }
                break;

            case 0x21: // SUB
                if (i + 3 < bytecode_size) {
                    uint8_t reg1 = bytecode[i + 1];
                    uint8_t reg2 = bytecode[i + 2];
                    uint8_t dst_reg = bytecode[i + 3];

                    // 简化实现：只支持 rax = rax - rbx
                    if (reg1 == 0 && reg2 == 1 && dst_reg == 0) {
                        uint8_t sub_instr[] = {0x48, 0x29, 0xd8}; // sub rax, rbx

                        if (jit->code_size + sizeof(sub_instr) > jit->code_capacity) {
                            strcpy(jit->error_message, "Code buffer overflow");
                            return COMPILE_ERROR_CODEGEN_FAILED;
                        }

                        memcpy(jit->code_buffer + jit->code_size, sub_instr, sizeof(sub_instr));
                        jit->code_size += sizeof(sub_instr);
                    }
                    i += 4;
                } else {
                    i++;
                }
                break;

            case 0x31: // RETURN
                {
                    // 生成函数结尾
                    uint8_t epilogue[] = {
                        0x48, 0x89, 0xec,    // mov rsp, rbp
                        0x5d,                // pop rbp
                        0xc3                 // ret
                    };

                    if (jit->code_size + sizeof(epilogue) > jit->code_capacity) {
                        strcpy(jit->error_message, "Code buffer overflow");
                        return COMPILE_ERROR_CODEGEN_FAILED;
                    }

                    memcpy(jit->code_buffer + jit->code_size, epilogue, sizeof(epilogue));
                    jit->code_size += sizeof(epilogue);
                }
                i++;
                break;

            case 0x50: // PUSH
                if (i + 1 < bytecode_size) {
                    uint8_t reg = bytecode[i + 1];

                    if (reg == 0) { // push rax
                        uint8_t push_instr[] = {0x50}; // push rax

                        if (jit->code_size + sizeof(push_instr) > jit->code_capacity) {
                            strcpy(jit->error_message, "Code buffer overflow");
                            return COMPILE_ERROR_CODEGEN_FAILED;
                        }

                        memcpy(jit->code_buffer + jit->code_size, push_instr, sizeof(push_instr));
                        jit->code_size += sizeof(push_instr);
                    }
                    i += 2;
                } else {
                    i++;
                }
                break;

            case 0x51: // POP
                if (i + 1 < bytecode_size) {
                    uint8_t reg = bytecode[i + 1];

                    if (reg == 0) { // pop rax
                        uint8_t pop_instr[] = {0x58}; // pop rax

                        if (jit->code_size + sizeof(pop_instr) > jit->code_capacity) {
                            strcpy(jit->error_message, "Code buffer overflow");
                            return COMPILE_ERROR_CODEGEN_FAILED;
                        }

                        memcpy(jit->code_buffer + jit->code_size, pop_instr, sizeof(pop_instr));
                        jit->code_size += sizeof(pop_instr);
                    }
                    i += 2;
                } else {
                    i++;
                }
                break;

            default:
                i++;
                break;
        }
    }
    
    // 分配可执行内存并复制代码
    result->code_ptr = allocate_executable_memory(jit->code_size);
    if (!result->code_ptr) {
        strcpy(jit->error_message, "Failed to allocate executable memory");
        return COMPILE_ERROR_MEMORY_ALLOC;
    }
    
    memcpy(result->code_ptr, jit->code_buffer, jit->code_size);
    result->code_size = jit->code_size;
    result->is_executable = true;
    
    return COMPILE_SUCCESS;
}

// 执行JIT编译的代码
static int jit_execute_code(JITCodeBlock* code_block) {
    if (!code_block || !code_block->code_ptr || !code_block->is_executable) {
        return -1;
    }
    
    // 将代码指针转换为函数指针并调用
    int (*func)(void) = (int (*)(void))code_block->code_ptr;
    return func();
}

// 释放JIT代码块
static void jit_free_code_block(JITCodeBlock* code_block) {
    if (!code_block) return;
    
    if (code_block->code_ptr) {
        free_executable_memory(code_block->code_ptr, code_block->code_size);
        code_block->code_ptr = NULL;
    }
    
    code_block->code_size = 0;
    code_block->is_executable = false;
}



// ===============================================
// FFI (外部函数接口) 实现
// ===============================================

// FFI函数签名
typedef struct {
    char name[64];
    void* function_ptr;
    int arg_count;
    char return_type;
    char arg_types[8];
} FFIFunction;

// FFI上下文
typedef struct {
    FFIFunction* functions;
    size_t function_count;
    size_t function_capacity;
    void** loaded_libraries;
    size_t library_count;
    char error_message[256];
} FFIContext;

// 创建FFI上下文
static FFIContext* ffi_create_context(void) {
    FFIContext* ffi = malloc(sizeof(FFIContext));
    if (!ffi) return NULL;
    
    ffi->function_capacity = 32;
    ffi->functions = malloc(ffi->function_capacity * sizeof(FFIFunction));
    ffi->function_count = 0;
    ffi->loaded_libraries = malloc(16 * sizeof(void*));
    ffi->library_count = 0;
    ffi->error_message[0] = '\0';
    
    if (!ffi->functions || !ffi->loaded_libraries) {
        free(ffi->functions);
        free(ffi->loaded_libraries);
        free(ffi);
        return NULL;
    }
    
    return ffi;
}

// 销毁FFI上下文
static void ffi_destroy_context(FFIContext* ffi) {
    if (!ffi) return;
    
    // 关闭加载的库
    for (size_t i = 0; i < ffi->library_count; i++) {
        if (ffi->loaded_libraries[i]) {
#ifdef _WIN32
            FreeLibrary((HMODULE)ffi->loaded_libraries[i]);
#else
            dlclose(ffi->loaded_libraries[i]);
#endif
        }
    }
    
    free(ffi->functions);
    free(ffi->loaded_libraries);
    free(ffi);
}

// 加载外部库
static CompileResult ffi_load_library(FFIContext* ffi, const char* library_path) {
    if (!ffi || !library_path) {
        return COMPILE_ERROR_INVALID_INPUT;
    }
    
    void* handle = NULL;
    
#ifdef _WIN32
    handle = LoadLibraryA(library_path);
#else
    handle = dlopen(library_path, RTLD_LAZY);
#endif
    
    if (!handle) {
        strcpy(ffi->error_message, "Failed to load library");
        return COMPILE_ERROR_FFI_FAILED;
    }
    
    // 添加到已加载库列表
    if (ffi->library_count < 16) {
        ffi->loaded_libraries[ffi->library_count++] = handle;
    }
    
    return COMPILE_SUCCESS;
}

// 注册FFI函数
static CompileResult ffi_register_function(FFIContext* ffi, const char* name, 
                                         void* function_ptr, int arg_count, 
                                         char return_type, const char* arg_types) {
    if (!ffi || !name || !function_ptr) {
        return COMPILE_ERROR_INVALID_INPUT;
    }
    
    if (ffi->function_count >= ffi->function_capacity) {
        strcpy(ffi->error_message, "FFI function table full");
        return COMPILE_ERROR_FFI_FAILED;
    }
    
    FFIFunction* func = &ffi->functions[ffi->function_count++];
    strncpy(func->name, name, sizeof(func->name) - 1);
    func->name[sizeof(func->name) - 1] = '\0';
    func->function_ptr = function_ptr;
    func->arg_count = arg_count;
    func->return_type = return_type;
    
    if (arg_types) {
        strncpy(func->arg_types, arg_types, sizeof(func->arg_types) - 1);
        func->arg_types[sizeof(func->arg_types) - 1] = '\0';
    }
    
    return COMPILE_SUCCESS;
}

// 查找FFI函数
static FFIFunction* ffi_find_function(FFIContext* ffi, const char* name) {
    if (!ffi || !name) return NULL;
    
    for (size_t i = 0; i < ffi->function_count; i++) {
        if (strcmp(ffi->functions[i].name, name) == 0) {
            return &ffi->functions[i];
        }
    }
    
    return NULL;
}

// 调用FFI函数（简化版）
static int ffi_call_function(FFIContext* ffi, const char* name, void** args, void* result) {
    FFIFunction* func = ffi_find_function(ffi, name);
    if (!func) {
        strcpy(ffi->error_message, "Function not found");
        return -1;
    }
    
    // 简化的函数调用实现
    // 实际实现需要根据调用约定和参数类型进行复杂的处理
    if (func->arg_count == 0 && func->return_type == 'i') {
        int (*f)(void) = (int (*)(void))func->function_ptr;
        *(int*)result = f();
        return 0;
    }
    
    strcpy(ffi->error_message, "Unsupported function signature");
    return -1;
}

// ===============================================
// 统一编译器接口
// ===============================================

// 编译器上下文
typedef struct {
    CompilerType type;
    TargetArch target_arch;
    OptLevel opt_level;
    
    union {
        JITCompiler* jit;
        FFIContext* ffi;
    } compiler;
    
    char error_message[512];
} CompilerContext;

// 创建编译器上下文
static CompilerContext* compiler_create_context(CompilerType type, TargetArch arch, OptLevel opt_level) {
    CompilerContext* ctx = malloc(sizeof(CompilerContext));
    if (!ctx) return NULL;
    
    ctx->type = type;
    ctx->target_arch = arch;
    ctx->opt_level = opt_level;
    ctx->error_message[0] = '\0';
    
    switch (type) {
        case COMPILER_JIT:
            ctx->compiler.jit = jit_create_compiler(arch, opt_level);
            if (!ctx->compiler.jit) {
                free(ctx);
                return NULL;
            }
            break;

        case COMPILER_FFI:
            ctx->compiler.ffi = ffi_create_context();
            if (!ctx->compiler.ffi) {
                free(ctx);
                return NULL;
            }
            break;
    }
    
    return ctx;
}

// 销毁编译器上下文
static void compiler_destroy_context(CompilerContext* ctx) {
    if (!ctx) return;
    
    switch (ctx->type) {
        case COMPILER_JIT:
            jit_destroy_compiler(ctx->compiler.jit);
            break;
        case COMPILER_FFI:
            ffi_destroy_context(ctx->compiler.ffi);
            break;
    }
    
    free(ctx);
}

// 编译字节码
static CompileResult compiler_compile_bytecode(CompilerContext* ctx, const uint8_t* bytecode,
                                             size_t bytecode_size, const char* output_file,
                                             void** result) {
    if (!ctx || !bytecode) {
        return COMPILE_ERROR_INVALID_INPUT;
    }
    
    switch (ctx->type) {
        case COMPILER_JIT: {
            JITCodeBlock* code_block = malloc(sizeof(JITCodeBlock));
            if (!code_block) return COMPILE_ERROR_MEMORY_ALLOC;

            CompileResult res = jit_compile_bytecode(ctx->compiler.jit, bytecode, bytecode_size, code_block);
            if (res == COMPILE_SUCCESS) {
                *result = code_block;
            } else {
                free(code_block);
                strcpy(ctx->error_message, ctx->compiler.jit->error_message);
            }
            return res;
        }

        case COMPILER_FFI:
            strcpy(ctx->error_message, "FFI does not support bytecode compilation");
            return COMPILE_ERROR_INVALID_INPUT;
    }
    
    return COMPILE_ERROR_INVALID_INPUT;
}

// 获取错误信息
static const char* compiler_get_error(CompilerContext* ctx) {
    return ctx ? ctx->error_message : "Invalid compiler context";
}

// ===============================================
// 模块符号表
// ===============================================

static struct {
    const char* name;
    void* symbol;
} compiler_symbols[] = {
    // 编译器上下文管理
    {"compiler_create_context", compiler_create_context},
    {"compiler_destroy_context", compiler_destroy_context},
    {"compiler_compile_bytecode", compiler_compile_bytecode},
    {"compiler_get_error", compiler_get_error},
    
    // JIT编译器
    {"jit_create_compiler", jit_create_compiler},
    {"jit_destroy_compiler", jit_destroy_compiler},
    {"jit_compile_bytecode", jit_compile_bytecode},
    {"jit_execute_code", jit_execute_code},
    {"jit_free_code_block", jit_free_code_block},
    

    
    // FFI接口
    {"ffi_create_context", ffi_create_context},
    {"ffi_destroy_context", ffi_destroy_context},
    {"ffi_load_library", ffi_load_library},
    {"ffi_register_function", ffi_register_function},
    {"ffi_find_function", ffi_find_function},
    {"ffi_call_function", ffi_call_function},
    
    // 内存管理
    {"allocate_executable_memory", allocate_executable_memory},
    {"free_executable_memory", free_executable_memory},
    
    {NULL, NULL}
};

// ===============================================
// 模块初始化和清理
// ===============================================

static int compiler_init(void) {
    printf("Compiler Module: Initializing integrated compiler...\n");
    printf("Compiler Module: JIT compiler initialized\n");
    printf("Compiler Module: FFI interface initialized\n");
    printf("Compiler Module: Note - AOT compiler moved to pipeline_module\n");

    return 0;
}

static void compiler_cleanup(void) {
    printf("Compiler Module: Cleaning up integrated compiler...\n");
}

// 解析符号
static void* compiler_resolve(const char* symbol) {
    if (!symbol) return NULL;
    
    for (int i = 0; compiler_symbols[i].name; i++) {
        if (strcmp(compiler_symbols[i].name, symbol) == 0) {
            return compiler_symbols[i].symbol;
        }
    }
    
    return NULL;
}

// ===============================================
// 模块定义
// ===============================================

Module module_compiler = {
    .name = "compiler",
    .state = MODULE_UNLOADED,
    .error = NULL,
    .init = compiler_init,
    .cleanup = compiler_cleanup,
    .resolve = compiler_resolve
}; 