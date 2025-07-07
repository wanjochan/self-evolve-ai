/**
 * vm_module.c - Virtual Machine Module
 * 
 * Standard implementation for vm_{arch}_{bits}.native modules.
 * Follows PRD.md Layer 2 specification and native module format.
 * 
 * This file will be compiled into:
 * - vm_x64_64.native
 * - vm_arm64_64.native  
 * - vm_x86_32.native
 * - vm_arm32_32.native
 * Provides VM functionality as a module.
 * Depends on the memory module.
 */

#include "../module.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

// Module name
#define MODULE_NAME "vm"

// Function type definitions for memory module functions
typedef void* (*memory_alloc_t)(size_t size);
typedef void* (*memory_realloc_t)(void* ptr, size_t size);
typedef void (*memory_free_t)(void* ptr);
typedef void* (*memory_copy_t)(void* dest, const void* src, size_t size);
typedef void* (*memory_set_t)(void* dest, int value, size_t size);

// Global function pointers for memory operations
static memory_alloc_t mem_alloc = NULL;
static memory_realloc_t mem_realloc = NULL;
static memory_free_t mem_free = NULL;
static memory_copy_t mem_copy = NULL;
static memory_set_t mem_set = NULL;

// ===============================================
// VM Core Definitions
// ===============================================

// VM状态
typedef enum {
    VM_STATE_UNINITIALIZED,
    VM_STATE_READY,
    VM_STATE_RUNNING,
    VM_STATE_PAUSED,
    VM_STATE_STOPPED,
    VM_STATE_ERROR
} VMState;

// VM错误代码
typedef enum {
    VM_ERROR_NONE = 0,
    VM_ERROR_INVALID_CONTEXT,
    VM_ERROR_INVALID_BYTECODE,
    VM_ERROR_STACK_OVERFLOW,
    VM_ERROR_STACK_UNDERFLOW,
    VM_ERROR_INVALID_INSTRUCTION,
    VM_ERROR_INVALID_OPERAND,
    VM_ERROR_DIVISION_BY_ZERO,
    VM_ERROR_OUT_OF_MEMORY,
    VM_ERROR_CALL_DEPTH_EXCEEDED,
    VM_ERROR_UNKNOWN
} VMErrorCode;

// VM上下文
typedef struct VMContext {
    // VM state
    VMState state;
    
    // Program data
    uint8_t* bytecode;
    size_t bytecode_size;
    size_t program_counter;
    
    // Execution stack
    uint64_t* stack;
    size_t stack_size;
    size_t stack_pointer;
    
    // Registers
    uint64_t* registers;
    size_t register_count;
    
    // Call stack
    size_t* call_stack;
    size_t call_stack_size;
    size_t call_depth;
    
    // Flags
    bool zero_flag;
    bool carry_flag;
    bool overflow_flag;
    bool negative_flag;
    
    // Statistics
    uint64_t instruction_count;
    uint64_t cycle_count;
    
    // Error handling
    VMErrorCode last_error;
    char error_message[256];
    
} VMContext;

// ASTC指令集
typedef enum {
    // Control flow
    VM_OP_NOP = 0x00,
    VM_OP_HALT = 0x01,
    VM_OP_JUMP = 0x02,
    VM_OP_JUMP_IF = 0x03,
    VM_OP_CALL = 0x04,
    VM_OP_RETURN = 0x05,
    
    // Data movement
    VM_OP_LOAD_IMM = 0x10,
    VM_OP_LOAD_REG = 0x11,
    VM_OP_STORE_REG = 0x12,
    VM_OP_MOVE = 0x13,
    
    // Arithmetic
    VM_OP_ADD = 0x20,
    VM_OP_SUB = 0x21,
    VM_OP_MUL = 0x22,
    VM_OP_DIV = 0x23,
    VM_OP_MOD = 0x24,
    
    // Logical
    VM_OP_AND = 0x30,
    VM_OP_OR = 0x31,
    VM_OP_XOR = 0x32,
    VM_OP_NOT = 0x33,
    VM_OP_SHL = 0x34,
    VM_OP_SHR = 0x35,
    
    // Comparison
    VM_OP_CMP = 0x40,
    VM_OP_TEST = 0x41,
    
    // Stack operations
    VM_OP_PUSH = 0x50,
    VM_OP_POP = 0x51,
    
    // System calls
    VM_OP_SYSCALL = 0x60,
    VM_OP_PRINT = 0x61,
    VM_OP_MALLOC = 0x62,
    VM_OP_FREE = 0x63,
    
    // Exit
    VM_OP_EXIT = 0xFF
} VMOpcode;

// ASTC文件头
typedef struct {
    char magic[4];          // "ASTC"
    uint32_t version;       // 版本号
    uint32_t flags;         // 标志
    uint32_t entry_point;   // 入口点
    uint32_t source_size;   // 源码大小
} ASTCHeader;

// ===============================================
// VM Core Functions
// ===============================================

// Function declarations
static VMContext* vm_create_context(void);
static void vm_destroy_context(VMContext* ctx);
static int vm_load_program(VMContext* ctx, const uint8_t* bytecode, size_t size);
static int vm_execute(VMContext* ctx);
static int vm_step(VMContext* ctx);
static void vm_reset(VMContext* ctx);
static VMState vm_get_state(const VMContext* ctx);
static void vm_set_state(VMContext* ctx, VMState state);
static void vm_get_stats(const VMContext* ctx, uint64_t* instructions, uint64_t* cycles);
static void vm_print_context(const VMContext* ctx);
static bool vm_validate_bytecode(const uint8_t* bytecode, size_t size);
static const char* vm_get_opcode_name(VMOpcode opcode);
static int vm_disassemble_instruction(const uint8_t* bytecode, size_t offset, char* buffer, size_t buffer_size);
static int vm_disassemble_program(const uint8_t* bytecode, size_t size);
static void vm_set_error(VMContext* ctx, VMErrorCode error, const char* format, ...);

/**
 * 创建VM上下文
 */
static VMContext* vm_create_context(void) {
    VMContext* ctx = mem_alloc ? mem_alloc(sizeof(VMContext)) : malloc(sizeof(VMContext));
    if (!ctx) {
        return NULL;
    }
    
    // 初始化上下文
    if (mem_set) {
        mem_set(ctx, 0, sizeof(VMContext));
    } else {
        memset(ctx, 0, sizeof(VMContext));
    }
    
    ctx->state = VM_STATE_UNINITIALIZED;
    
    // 分配栈空间 (64KB)
    ctx->stack_size = 64 * 1024 / sizeof(uint64_t);
    ctx->stack = mem_alloc ? mem_alloc(ctx->stack_size * sizeof(uint64_t)) : malloc(ctx->stack_size * sizeof(uint64_t));
    if (!ctx->stack) {
        if (mem_free) mem_free(ctx); else free(ctx);
        return NULL;
    }
    
    // 分配寄存器 (32个)
    ctx->register_count = 32;
    ctx->registers = mem_alloc ? mem_alloc(ctx->register_count * sizeof(uint64_t)) : malloc(ctx->register_count * sizeof(uint64_t));
    if (!ctx->registers) {
        if (mem_free) mem_free(ctx->stack); else free(ctx->stack);
        if (mem_free) mem_free(ctx); else free(ctx);
        return NULL;
    }
    
    // 分配调用栈 (1024层)
    ctx->call_stack_size = 1024;
    ctx->call_stack = mem_alloc ? mem_alloc(ctx->call_stack_size * sizeof(size_t)) : malloc(ctx->call_stack_size * sizeof(size_t));
    if (!ctx->call_stack) {
        if (mem_free) mem_free(ctx->registers); else free(ctx->registers);
        if (mem_free) mem_free(ctx->stack); else free(ctx->stack);
        if (mem_free) mem_free(ctx); else free(ctx);
        return NULL;
    }
    
    ctx->state = VM_STATE_READY;
    return ctx;
}

/**
 * 销毁VM上下文
 */
static void vm_destroy_context(VMContext* ctx) {
    if (!ctx) {
        return;
    }
    
    if (ctx->bytecode) {
        if (mem_free) mem_free(ctx->bytecode); else free(ctx->bytecode);
    }
    
    if (ctx->call_stack) {
        if (mem_free) mem_free(ctx->call_stack); else free(ctx->call_stack);
    }
    
    if (ctx->registers) {
        if (mem_free) mem_free(ctx->registers); else free(ctx->registers);
    }
    
    if (ctx->stack) {
        if (mem_free) mem_free(ctx->stack); else free(ctx->stack);
    }
    
    if (mem_free) mem_free(ctx); else free(ctx);
}

/**
 * 加载程序到VM
 */
static int vm_load_program(VMContext* ctx, const uint8_t* bytecode, size_t size) {
    if (!ctx || !bytecode || size == 0) {
        if (ctx) vm_set_error(ctx, VM_ERROR_INVALID_BYTECODE, "Invalid parameters");
        return -1;
    }
    
    // 验证字节码
    if (!vm_validate_bytecode(bytecode, size)) {
        vm_set_error(ctx, VM_ERROR_INVALID_BYTECODE, "Bytecode validation failed");
        return -1;
    }
    
    // 分配内存并复制字节码
    if (ctx->bytecode) {
        if (mem_free) mem_free(ctx->bytecode); else free(ctx->bytecode);
    }
    
    ctx->bytecode = mem_alloc ? mem_alloc(size) : malloc(size);
    if (!ctx->bytecode) {
        vm_set_error(ctx, VM_ERROR_OUT_OF_MEMORY, "Failed to allocate bytecode memory");
        return -1;
    }
    
    if (mem_copy) {
        mem_copy(ctx->bytecode, bytecode, size);
    } else {
        memcpy(ctx->bytecode, bytecode, size);
    }
    
    ctx->bytecode_size = size;
    ctx->program_counter = 0;
    ctx->stack_pointer = 0;
    ctx->call_depth = 0;
    
    // 重置统计信息
    ctx->instruction_count = 0;
    ctx->cycle_count = 0;
    
    // 重置标志
    ctx->zero_flag = false;
    ctx->carry_flag = false;
    ctx->overflow_flag = false;
    ctx->negative_flag = false;
    
    ctx->state = VM_STATE_READY;
    ctx->last_error = VM_ERROR_NONE;
    
    return 0;
}

/**
 * 执行VM程序
 */
static int vm_execute(VMContext* ctx) {
    if (!ctx) {
        return -1;
    }
    
    if (ctx->state != VM_STATE_READY) {
        vm_set_error(ctx, VM_ERROR_INVALID_CONTEXT, "VM not ready");
        return -1;
    }
    
    ctx->state = VM_STATE_RUNNING;
    
    // 执行主循环
    while (ctx->state == VM_STATE_RUNNING && ctx->program_counter < ctx->bytecode_size) {
        if (vm_step(ctx) != 0) {
            break;
        }
    }
    
    if (ctx->state == VM_STATE_RUNNING) {
        ctx->state = VM_STATE_STOPPED;
    }
    
    return (ctx->last_error == VM_ERROR_NONE) ? 0 : -1;
}

/**
 * 执行单步指令
 */
static int vm_step(VMContext* ctx) {
    if (!ctx || ctx->state != VM_STATE_RUNNING) {
        return -1;
    }
    
    if (ctx->program_counter >= ctx->bytecode_size) {
        vm_set_error(ctx, VM_ERROR_INVALID_INSTRUCTION, "Program counter out of bounds");
        ctx->state = VM_STATE_ERROR;
        return -1;
    }
    
    // 读取指令
    VMOpcode opcode = (VMOpcode)ctx->bytecode[ctx->program_counter];
    ctx->program_counter++;
    ctx->instruction_count++;
    ctx->cycle_count++;
    
    // 执行指令
    switch (opcode) {
        case VM_OP_NOP:
            // 无操作
            break;
            
        case VM_OP_HALT:
            ctx->state = VM_STATE_STOPPED;
            break;
            
        case VM_OP_PRINT: {
            // 简单的打印指令 - 打印栈顶值
            if (ctx->stack_pointer == 0) {
                vm_set_error(ctx, VM_ERROR_STACK_UNDERFLOW, "Stack underflow in PRINT");
                ctx->state = VM_STATE_ERROR;
                return -1;
            }
            uint64_t value = ctx->stack[--ctx->stack_pointer];
            printf("VM Print: %llu\n", (unsigned long long)value);
            break;
        }
        
        case VM_OP_PUSH: {
            // 推送立即数到栈
            if (ctx->program_counter + 8 > ctx->bytecode_size) {
                vm_set_error(ctx, VM_ERROR_INVALID_INSTRUCTION, "Incomplete PUSH instruction");
                ctx->state = VM_STATE_ERROR;
                return -1;
            }
            
            if (ctx->stack_pointer >= ctx->stack_size) {
                vm_set_error(ctx, VM_ERROR_STACK_OVERFLOW, "Stack overflow in PUSH");
                ctx->state = VM_STATE_ERROR;
                return -1;
            }
            
            // 读取8字节立即数
            uint64_t value = 0;
            for (int i = 0; i < 8; i++) {
                value |= ((uint64_t)ctx->bytecode[ctx->program_counter + i]) << (i * 8);
            }
            ctx->program_counter += 8;
            
            ctx->stack[ctx->stack_pointer++] = value;
            break;
        }
        
        case VM_OP_POP: {
            // 弹出栈顶值
            if (ctx->stack_pointer == 0) {
                vm_set_error(ctx, VM_ERROR_STACK_UNDERFLOW, "Stack underflow in POP");
                ctx->state = VM_STATE_ERROR;
                return -1;
            }
            ctx->stack_pointer--;
            break;
        }
        
        case VM_OP_ADD: {
            // 加法：弹出两个值，推送结果
            if (ctx->stack_pointer < 2) {
                vm_set_error(ctx, VM_ERROR_STACK_UNDERFLOW, "Stack underflow in ADD");
                ctx->state = VM_STATE_ERROR;
                return -1;
            }
            uint64_t b = ctx->stack[--ctx->stack_pointer];
            uint64_t a = ctx->stack[--ctx->stack_pointer];
            uint64_t result = a + b;
            ctx->stack[ctx->stack_pointer++] = result;
            break;
        }
        
        case VM_OP_EXIT: {
            // 退出指令
            ctx->state = VM_STATE_STOPPED;
            break;
        }
        
        default:
            vm_set_error(ctx, VM_ERROR_INVALID_INSTRUCTION, "Unknown opcode: 0x%02X", opcode);
            ctx->state = VM_STATE_ERROR;
            return -1;
    }
    
    return 0;
}

/**
 * 重置VM状态
 */
static void vm_reset(VMContext* ctx) {
    if (!ctx) {
        return;
    }
    
    ctx->program_counter = 0;
    ctx->stack_pointer = 0;
    ctx->call_depth = 0;
    
    // 清空寄存器
    if (ctx->registers && mem_set) {
        mem_set(ctx->registers, 0, ctx->register_count * sizeof(uint64_t));
    } else if (ctx->registers) {
        memset(ctx->registers, 0, ctx->register_count * sizeof(uint64_t));
    }
    
    // 重置标志
    ctx->zero_flag = false;
    ctx->carry_flag = false;
    ctx->overflow_flag = false;
    ctx->negative_flag = false;
    
    // 重置统计信息
    ctx->instruction_count = 0;
    ctx->cycle_count = 0;
    
    ctx->state = VM_STATE_READY;
    ctx->last_error = VM_ERROR_NONE;
}

/**
 * 获取VM状态
 */
static VMState vm_get_state(const VMContext* ctx) {
    return ctx ? ctx->state : VM_STATE_UNINITIALIZED;
}

/**
 * 设置VM状态
 */
static void vm_set_state(VMContext* ctx, VMState state) {
    if (ctx) {
        ctx->state = state;
    }
}

/**
 * 获取执行统计信息
 */
static void vm_get_stats(const VMContext* ctx, uint64_t* instructions, uint64_t* cycles) {
    if (ctx) {
        if (instructions) *instructions = ctx->instruction_count;
        if (cycles) *cycles = ctx->cycle_count;
    }
}

/**
 * 打印VM上下文信息
 */
static void vm_print_context(const VMContext* ctx) {
    if (!ctx) {
        printf("VM Context: NULL\n");
        return;
    }
    
    printf("VM Context:\n");
    printf("  State: %d\n", ctx->state);
    printf("  PC: %zu\n", ctx->program_counter);
    printf("  SP: %zu\n", ctx->stack_pointer);
    printf("  Instructions: %llu\n", (unsigned long long)ctx->instruction_count);
    printf("  Cycles: %llu\n", (unsigned long long)ctx->cycle_count);
    printf("  Error: %d (%s)\n", ctx->last_error, ctx->error_message);
    
    // 打印栈顶几个值
    printf("  Stack (top 4): ");
    for (int i = 0; i < 4 && i < (int)ctx->stack_pointer; i++) {
        printf("%llu ", (unsigned long long)ctx->stack[ctx->stack_pointer - 1 - i]);
    }
    printf("\n");
}

/**
 * 验证字节码
 */
static bool vm_validate_bytecode(const uint8_t* bytecode, size_t size) {
    if (!bytecode || size == 0) {
        return false;
    }
    
    // 基本长度检查
    if (size < 1) {
        return false;
    }
    
    // 简单的指令验证
    for (size_t i = 0; i < size; ) {
        VMOpcode opcode = (VMOpcode)bytecode[i];
        i++;
        
        switch (opcode) {
            case VM_OP_NOP:
            case VM_OP_HALT:
            case VM_OP_POP:
            case VM_OP_ADD:
            case VM_OP_PRINT:
            case VM_OP_EXIT:
                // 单字节指令
                break;
                
            case VM_OP_PUSH:
                // 需要8字节立即数
                if (i + 8 > size) {
                    return false;
                }
                i += 8;
                break;
                
            default:
                // 未知指令，但不立即失败
                break;
        }
    }
    
    return true;
}

/**
 * 获取操作码名称
 */
static const char* vm_get_opcode_name(VMOpcode opcode) {
    switch (opcode) {
        case VM_OP_NOP: return "NOP";
        case VM_OP_HALT: return "HALT";
        case VM_OP_JUMP: return "JUMP";
        case VM_OP_JUMP_IF: return "JUMP_IF";
        case VM_OP_CALL: return "CALL";
        case VM_OP_RETURN: return "RETURN";
        case VM_OP_LOAD_IMM: return "LOAD_IMM";
        case VM_OP_LOAD_REG: return "LOAD_REG";
        case VM_OP_STORE_REG: return "STORE_REG";
        case VM_OP_MOVE: return "MOVE";
        case VM_OP_ADD: return "ADD";
        case VM_OP_SUB: return "SUB";
        case VM_OP_MUL: return "MUL";
        case VM_OP_DIV: return "DIV";
        case VM_OP_MOD: return "MOD";
        case VM_OP_AND: return "AND";
        case VM_OP_OR: return "OR";
        case VM_OP_XOR: return "XOR";
        case VM_OP_NOT: return "NOT";
        case VM_OP_SHL: return "SHL";
        case VM_OP_SHR: return "SHR";
        case VM_OP_CMP: return "CMP";
        case VM_OP_TEST: return "TEST";
        case VM_OP_PUSH: return "PUSH";
        case VM_OP_POP: return "POP";
        case VM_OP_SYSCALL: return "SYSCALL";
        case VM_OP_PRINT: return "PRINT";
        case VM_OP_MALLOC: return "MALLOC";
        case VM_OP_FREE: return "FREE";
        case VM_OP_EXIT: return "EXIT";
        default: return "UNKNOWN";
    }
}

/**
 * 反汇编单条指令
 */
static int vm_disassemble_instruction(const uint8_t* bytecode, size_t offset, 
                                     char* buffer, size_t buffer_size) {
    if (!bytecode || !buffer || buffer_size == 0) {
        return -1;
    }
    
    VMOpcode opcode = (VMOpcode)bytecode[offset];
    const char* name = vm_get_opcode_name(opcode);
    
    switch (opcode) {
        case VM_OP_PUSH:
            if (offset + 9 <= buffer_size) {
                uint64_t value = 0;
                for (int i = 0; i < 8; i++) {
                    value |= ((uint64_t)bytecode[offset + 1 + i]) << (i * 8);
                }
                snprintf(buffer, buffer_size, "%s %llu", name, (unsigned long long)value);
                return 9; // 1 + 8 bytes
            }
            break;
            
        default:
            snprintf(buffer, buffer_size, "%s", name);
            return 1;
    }
    
    return -1;
}

/**
 * 反汇编整个程序
 */
static int vm_disassemble_program(const uint8_t* bytecode, size_t size) {
    if (!bytecode || size == 0) {
        return -1;
    }
    
    printf("Disassembly:\n");
    printf("============\n");
    
    for (size_t offset = 0; offset < size; ) {
        char instruction[256];
        int bytes = vm_disassemble_instruction(bytecode, offset, instruction, sizeof(instruction));
        
        if (bytes <= 0) {
            printf("%04zx: <invalid>\n", offset);
            offset++;
        } else {
            printf("%04zx: %s\n", offset, instruction);
            offset += bytes;
        }
    }
    
    return 0;
}

/**
 * 设置错误信息
 */
static void vm_set_error(VMContext* ctx, VMErrorCode error, const char* format, ...) {
    if (!ctx) {
        return;
    }
    
    ctx->last_error = error;
    
    if (format) {
        va_list args;
        va_start(args, format);
        vsnprintf(ctx->error_message, sizeof(ctx->error_message), format, args);
        va_end(args);
    } else {
        ctx->error_message[0] = '\0';
    }
}

/**
 * 加载并执行ASTC文件 - 供simple_loader调用
 */
int vm_execute_astc(const char* astc_file, int argc, char* argv[]) {
    printf("VM: 执行ASTC文件 %s\n", astc_file);
    
    if (!astc_file) {
        return -1;
    }

    // 打开ASTC文件
    FILE* file = fopen(astc_file, "rb");
    if (!file) {
        printf("VM: 错误: 无法打开ASTC文件 %s\n", astc_file);
        return -1;
    }

    // 读取头部
    ASTCHeader header;
    if (fread(&header, sizeof(ASTCHeader), 1, file) != 1) {
        printf("VM: 错误: 读取ASTC头部失败\n");
        fclose(file);
        return -1;
    }

    // 验证魔数
    if (memcmp(header.magic, "ASTC", 4) != 0) {
        printf("VM: 错误: 无效的ASTC文件格式\n");
        fclose(file);
        return -1;
    }

    printf("VM: ASTC文件版本: %u\n", header.version);
    printf("VM: 源码大小: %u 字节\n", header.source_size);

    // 跳过源码部分
    fseek(file, header.source_size, SEEK_CUR);

    // 读取字节码大小
    uint32_t bytecode_size;
    if (fread(&bytecode_size, sizeof(uint32_t), 1, file) != 1) {
        printf("VM: 错误: 读取字节码大小失败\n");
        fclose(file);
        return -1;
    }

    printf("VM: 字节码大小: %u 字节\n", bytecode_size);

    // 读取字节码
    uint8_t* bytecode = mem_alloc ? mem_alloc(bytecode_size) : malloc(bytecode_size);
    if (!bytecode) {
        printf("VM: 错误: 内存分配失败\n");
        fclose(file);
        return -1;
    }

    if (fread(bytecode, 1, bytecode_size, file) != bytecode_size) {
        printf("VM: 错误: 读取字节码失败\n");
        if (mem_free) mem_free(bytecode); else free(bytecode);
        fclose(file);
        return -1;
    }

    fclose(file);

    // 反汇编字节码（调试用）
    printf("\n=== ASTC字节码反汇编 ===\n");
    vm_disassemble_program(bytecode, bytecode_size);
    printf("\n");

    // 创建VM上下文
    VMContext* ctx = vm_create_context();
    if (!ctx) {
        printf("VM: 错误: 创建VM上下文失败\n");
        if (mem_free) mem_free(bytecode); else free(bytecode);
        return -1;
    }

    // 加载程序
    int load_result = vm_load_program(ctx, bytecode, bytecode_size);
    if (load_result != 0) {
        printf("VM: 错误: 加载程序失败\n");
        vm_destroy_context(ctx);
        if (mem_free) mem_free(bytecode); else free(bytecode);
        return -1;
    }

    printf("=== 执行ASTC程序 ===\n");

    // 执行程序
    int exec_result = vm_execute(ctx);

    printf("=== 执行完成 ===\n");

    // 打印执行统计
    uint64_t instructions, cycles;
    vm_get_stats(ctx, &instructions, &cycles);
    printf("VM: 执行统计: %llu 指令, %llu 周期\n", 
           (unsigned long long)instructions, (unsigned long long)cycles);

    // 清理
    vm_destroy_context(ctx);
    if (mem_free) mem_free(bytecode); else free(bytecode);

    return exec_result;
}

/**
 * 兼容性函数
 */
int execute_astc(const char* astc_file, int argc, char* argv[]) {
    return vm_execute_astc(astc_file, argc, argv);
}

/**
 * Native模块主入口
 */
int native_main(int argc, char* argv[]) {
    if (argc < 2) {
        return -1;
    }
    return vm_execute_astc(argv[1], argc - 1, argv + 1);
}

// ===============================================
// Module Interface
// ===============================================

// Symbol table
static struct {
    const char* name;
    void* symbol;
} vm_symbols[] = {
    {"create_context", vm_create_context},
    {"destroy_context", vm_destroy_context},
    {"load_program", vm_load_program},
    {"execute", vm_execute},
    {"step", vm_step},
    {"reset", vm_reset},
    {"get_state", vm_get_state},
    {"set_state", vm_set_state},
    {"get_stats", vm_get_stats},
    {"print_context", vm_print_context},
    {"validate_bytecode", vm_validate_bytecode},
    {"get_opcode_name", vm_get_opcode_name},
    {"disassemble_instruction", vm_disassemble_instruction},
    {"disassemble_program", vm_disassemble_program},
    {"vm_execute_astc", vm_execute_astc},
    {"execute_astc", execute_astc},
    {"native_main", native_main},
    {NULL, NULL}  // Sentinel
};

// Module init function
static int vm_init(void) {
    // 初始化内存函数指针为标准库函数
    mem_alloc = malloc;
    mem_realloc = realloc;
    mem_free = free;
    mem_copy = memcpy;
    mem_set = memset;
    
    return 0;
}

// Module cleanup function
static void vm_cleanup(void) {
    // Nothing to clean up
}

// Symbol resolution function
static void* vm_resolve(const char* symbol) {
    if (!symbol) {
        return NULL;
    }
    
    for (int i = 0; vm_symbols[i].name; i++) {
        if (strcmp(vm_symbols[i].name, symbol) == 0) {
            return vm_symbols[i].symbol;
        }
    }
    
    return NULL;
}

// Module definition - updated to match new module.h structure
Module module_vm = {
    .name = MODULE_NAME,
    .state = MODULE_UNLOADED,
    .error = NULL,
    .init = vm_init,
    .cleanup = vm_cleanup,
    .resolve = vm_resolve
};

// 注意：不再需要REGISTER_MODULE，使用动态加载机制
