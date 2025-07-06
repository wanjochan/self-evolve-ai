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

// Module name
static const char* MODULE_NAME = "vm";

// Dependency on memory module
MODULE_DEPENDS_ON(memory);

// Function type definitions for memory module functions
typedef void* (*memory_alloc_t)(size_t size);
typedef void* (*memory_realloc_t)(void* ptr, size_t size);
typedef void (*memory_free_t)(void* ptr);
typedef void* (*memory_copy_t)(void* dest, const void* src, size_t size);
typedef void* (*memory_set_t)(void* dest, int value, size_t size);

// Cached memory functions
static memory_alloc_t mem_alloc;
static memory_realloc_t mem_realloc;
static memory_free_t mem_free;
static memory_copy_t mem_copy;
static memory_set_t mem_set;

// ===============================================
// VM Configuration
// ===============================================

#define VM_STACK_SIZE 8192
#define VM_REGISTER_COUNT 32
#define VM_MAX_CALL_DEPTH 256

// ===============================================
// VM State
// ===============================================

typedef enum {
    VM_STATE_UNINITIALIZED,
    VM_STATE_READY,
    VM_STATE_RUNNING,
    VM_STATE_PAUSED,
    VM_STATE_STOPPED,
    VM_STATE_ERROR
} VMState;

// ===============================================
// VM Error Codes
// ===============================================

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

// ===============================================
// VM Context
// ===============================================

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

// ===============================================
// VM Instructions
// ===============================================

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

// ===============================================
// Forward declarations
// ===============================================

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

// ===============================================
// VM Implementation
// ===============================================

/**
 * Create VM context
 */
static VMContext* vm_create_context(void) {
    VMContext* ctx = mem_alloc(sizeof(VMContext));
    if (!ctx) {
        return NULL;
    }

    // Initialize context
    mem_set(ctx, 0, sizeof(VMContext));
    
    // Allocate stack
    ctx->stack = mem_alloc(VM_STACK_SIZE * sizeof(uint64_t));
    if (!ctx->stack) {
        mem_free(ctx);
        return NULL;
    }
    ctx->stack_size = VM_STACK_SIZE;
    
    // Allocate registers
    ctx->registers = mem_alloc(VM_REGISTER_COUNT * sizeof(uint64_t));
    if (!ctx->registers) {
        mem_free(ctx->stack);
        mem_free(ctx);
        return NULL;
    }
    ctx->register_count = VM_REGISTER_COUNT;

    // Allocate call stack
    ctx->call_stack = mem_alloc(VM_MAX_CALL_DEPTH * sizeof(size_t));
    if (!ctx->call_stack) {
        mem_free(ctx->registers);
        mem_free(ctx->stack);
        mem_free(ctx);
        return NULL;
    }
    ctx->call_stack_size = VM_MAX_CALL_DEPTH;
    
    // Set initial state
    ctx->state = VM_STATE_UNINITIALIZED;
    
    return ctx;
}

/**
 * Destroy VM context
 */
static void vm_destroy_context(VMContext* ctx) {
    if (!ctx) {
        return;
    }

    // Free bytecode if owned by context
    if (ctx->bytecode) {
        mem_free(ctx->bytecode);
    }
    
    // Free call stack
    if (ctx->call_stack) {
        mem_free(ctx->call_stack);
    }
    
    // Free registers
    if (ctx->registers) {
        mem_free(ctx->registers);
    }
    
    // Free stack
    if (ctx->stack) {
        mem_free(ctx->stack);
    }
    
    // Free context
    mem_free(ctx);
}

/**
 * Load program into VM context
 */
static int vm_load_program(VMContext* ctx, const uint8_t* bytecode, size_t size) {
    if (!ctx || !bytecode || size == 0) {
        return -1;
    }

    // Reset context
    vm_reset(ctx);
    
    // Validate bytecode
    if (!vm_validate_bytecode(bytecode, size)) {
        vm_set_error(ctx, VM_ERROR_INVALID_BYTECODE, "Invalid bytecode format");
                    return -1;
                }
    
    // Allocate memory for bytecode copy
    ctx->bytecode = mem_alloc(size);
    if (!ctx->bytecode) {
        vm_set_error(ctx, VM_ERROR_OUT_OF_MEMORY, "Failed to allocate memory for bytecode");
                    return -1;
                }
    
    // Copy bytecode
    mem_copy(ctx->bytecode, bytecode, size);
    ctx->bytecode_size = size;
    
    // Set state to ready
    ctx->state = VM_STATE_READY;
    
                    return 0;
}

/**
 * Execute VM program
 */
static int vm_execute(VMContext* ctx) {
    if (!ctx || !ctx->bytecode) {
        return -1;
    }

    // Check if VM is ready
    if (ctx->state != VM_STATE_READY && ctx->state != VM_STATE_PAUSED) {
        vm_set_error(ctx, VM_ERROR_INVALID_CONTEXT, "VM not ready for execution");
        return -1;
    }

    // Set state to running
    ctx->state = VM_STATE_RUNNING;
    
    // Main execution loop
    while (ctx->state == VM_STATE_RUNNING) {
        // Execute one instruction
        int result = vm_step(ctx);
        
        // Check for errors or halt
        if (result != 0 || ctx->program_counter >= ctx->bytecode_size) {
            break;
        }
    }
    
    // Set state to stopped if still running
    if (ctx->state == VM_STATE_RUNNING) {
        ctx->state = VM_STATE_STOPPED;
    }
    
    // Return error code if in error state
    if (ctx->state == VM_STATE_ERROR) {
        return -1;
    }

    return 0;
}

/**
 * Execute single instruction
 */
static int vm_step(VMContext* ctx) {
    if (!ctx || !ctx->bytecode) {
        return -1;
    }

    // Check if VM is running
    if (ctx->state != VM_STATE_RUNNING) {
        return -1;
    }
    
    // Check program counter bounds
    if (ctx->program_counter >= ctx->bytecode_size) {
        vm_set_error(ctx, VM_ERROR_INVALID_INSTRUCTION, "Program counter out of bounds");
        ctx->state = VM_STATE_ERROR;
        return -1;
    }

    // Fetch opcode
    VMOpcode opcode = (VMOpcode)ctx->bytecode[ctx->program_counter++];
    
    // Execute instruction
        switch (opcode) {
        case VM_OP_NOP:
            // No operation
                break;

        case VM_OP_HALT:
            // Halt execution
            ctx->state = VM_STATE_STOPPED;
                break;

        case VM_OP_JUMP:
            // Jump to address
            if (ctx->program_counter + sizeof(uint32_t) > ctx->bytecode_size) {
                vm_set_error(ctx, VM_ERROR_INVALID_INSTRUCTION, "Jump target out of bounds");
                ctx->state = VM_STATE_ERROR;
                    return -1;
                }
            
            // Read target address
            uint32_t target = *(uint32_t*)(ctx->bytecode + ctx->program_counter);
            ctx->program_counter = target;
                break;

        case VM_OP_JUMP_IF:
            // Jump if condition is true
            if (ctx->program_counter + sizeof(uint32_t) > ctx->bytecode_size) {
                vm_set_error(ctx, VM_ERROR_INVALID_INSTRUCTION, "Jump target out of bounds");
                ctx->state = VM_STATE_ERROR;
                    return -1;
                }
            
            // Read target address
            uint32_t cond_target = *(uint32_t*)(ctx->bytecode + ctx->program_counter);
            ctx->program_counter += sizeof(uint32_t);
            
            // Jump if zero flag is set
            if (ctx->zero_flag) {
                ctx->program_counter = cond_target;
                }
                break;

        case VM_OP_CALL:
            // Call subroutine
            if (ctx->program_counter + sizeof(uint32_t) > ctx->bytecode_size) {
                vm_set_error(ctx, VM_ERROR_INVALID_INSTRUCTION, "Call target out of bounds");
                ctx->state = VM_STATE_ERROR;
                return -1;
        }

            // Check call depth
            if (ctx->call_depth >= ctx->call_stack_size) {
                vm_set_error(ctx, VM_ERROR_CALL_DEPTH_EXCEEDED, "Call stack overflow");
                ctx->state = VM_STATE_ERROR;
            return -1;
            }
            
            // Read target address
            uint32_t call_target = *(uint32_t*)(ctx->bytecode + ctx->program_counter);
            ctx->program_counter += sizeof(uint32_t);
            
            // Push return address to call stack
            ctx->call_stack[ctx->call_depth++] = ctx->program_counter;
            
            // Jump to target
            ctx->program_counter = call_target;
            break;
            
        case VM_OP_RETURN:
            // Return from subroutine
            if (ctx->call_depth == 0) {
                vm_set_error(ctx, VM_ERROR_STACK_UNDERFLOW, "Call stack underflow");
                ctx->state = VM_STATE_ERROR;
        return -1;
    }

            // Pop return address from call stack
            ctx->program_counter = ctx->call_stack[--ctx->call_depth];
            break;
            
        case VM_OP_LOAD_IMM:
            // Load immediate value into register
            if (ctx->program_counter + 1 + sizeof(uint64_t) > ctx->bytecode_size) {
                vm_set_error(ctx, VM_ERROR_INVALID_INSTRUCTION, "Load immediate operands out of bounds");
                ctx->state = VM_STATE_ERROR;
        return -1;
    }

            // Read register index
            uint8_t reg_idx = ctx->bytecode[ctx->program_counter++];
            
            // Check register index
            if (reg_idx >= ctx->register_count) {
                vm_set_error(ctx, VM_ERROR_INVALID_OPERAND, "Invalid register index");
                ctx->state = VM_STATE_ERROR;
        return -1;
    }

            // Read immediate value
            uint64_t imm_value = *(uint64_t*)(ctx->bytecode + ctx->program_counter);
            ctx->program_counter += sizeof(uint64_t);
            
            // Load value into register
            ctx->registers[reg_idx] = imm_value;
            break;
            
        // Add more instructions here...
            
        case VM_OP_EXIT:
            // Exit with status code
            if (ctx->program_counter < ctx->bytecode_size) {
                // Read exit code
                uint8_t exit_code = ctx->bytecode[ctx->program_counter++];
                
                // Store exit code in register 0
                ctx->registers[0] = exit_code;
            }
            
            // Halt execution
            ctx->state = VM_STATE_STOPPED;
            break;
            
        default:
            // Unknown opcode
            vm_set_error(ctx, VM_ERROR_INVALID_INSTRUCTION, "Unknown opcode: 0x%02X", opcode);
            ctx->state = VM_STATE_ERROR;
        return -1;
    }

    // Update statistics
    ctx->instruction_count++;
    ctx->cycle_count++; // Simplified cycle counting
    
    return 0;
}

/**
 * Reset VM context
 */
static void vm_reset(VMContext* ctx) {
    if (!ctx) {
        return;
    }
    
    // Reset program counter
    ctx->program_counter = 0;
    
    // Reset stack pointer
    ctx->stack_pointer = 0;
    
    // Reset call depth
    ctx->call_depth = 0;
    
    // Reset registers
    if (ctx->registers) {
        mem_set(ctx->registers, 0, ctx->register_count * sizeof(uint64_t));
    }
    
    // Reset flags
    ctx->zero_flag = false;
    ctx->carry_flag = false;
    ctx->overflow_flag = false;
    ctx->negative_flag = false;
    
    // Reset statistics
    ctx->instruction_count = 0;
    ctx->cycle_count = 0;
    
    // Reset error
    ctx->last_error = VM_ERROR_NONE;
    ctx->error_message[0] = '\0';
    
    // Set state to ready if bytecode is loaded
    if (ctx->bytecode) {
        ctx->state = VM_STATE_READY;
    } else {
        ctx->state = VM_STATE_UNINITIALIZED;
    }
}

/**
 * Get VM state
 */
static VMState vm_get_state(const VMContext* ctx) {
    return ctx ? ctx->state : VM_STATE_ERROR;
}

/**
 * Set VM state
 */
static void vm_set_state(VMContext* ctx, VMState state) {
    if (ctx) {
        ctx->state = state;
    }
}

/**
 * Get execution statistics
 */
static void vm_get_stats(const VMContext* ctx, uint64_t* instructions, uint64_t* cycles) {
    if (!ctx) {
        if (instructions) *instructions = 0;
        if (cycles) *cycles = 0;
        return;
    }

    if (instructions) *instructions = ctx->instruction_count;
    if (cycles) *cycles = ctx->cycle_count;
}

/**
 * Print VM context for debugging
 */
static void vm_print_context(const VMContext* ctx) {
    if (!ctx) {
        printf("VM Context: NULL\n");
        return;
    }
    
    printf("VM Context:\n");
    printf("  State: %d\n", ctx->state);
    printf("  Program Counter: %zu\n", ctx->program_counter);
    printf("  Stack Pointer: %zu\n", ctx->stack_pointer);
    printf("  Call Depth: %zu\n", ctx->call_depth);
    printf("  Flags: Z=%d C=%d O=%d N=%d\n",
           ctx->zero_flag, ctx->carry_flag, ctx->overflow_flag, ctx->negative_flag);
    printf("  Instructions: %llu\n", (unsigned long long)ctx->instruction_count);
    printf("  Cycles: %llu\n", (unsigned long long)ctx->cycle_count);
    
    // Print registers
    printf("  Registers:\n");
    for (size_t i = 0; i < ctx->register_count; i += 4) {
        printf("    ");
        for (size_t j = 0; j < 4 && i + j < ctx->register_count; j++) {
            printf("R%02zu=0x%016llx ", i + j, (unsigned long long)ctx->registers[i + j]);
        }
        printf("\n");
    }
    
    // Print top of stack
    printf("  Stack (top %d entries):\n", 8);
    for (size_t i = 0; i < 8 && i < ctx->stack_pointer; i++) {
        size_t idx = ctx->stack_pointer - i - 1;
        printf("    [%zu] = 0x%016llx\n", idx, (unsigned long long)ctx->stack[idx]);
    }
}

/**
 * Validate bytecode
 */
static bool vm_validate_bytecode(const uint8_t* bytecode, size_t size) {
    if (!bytecode || size == 0) {
        return false;
    }
    
    // Simplified validation - just check if size is reasonable
    // In a real implementation, we would check for valid instructions and structure
    return size >= 4 && size <= 1024 * 1024; // Between 4 bytes and 1MB
}

/**
 * Get opcode name for debugging
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
 * Disassemble instruction
 */
static int vm_disassemble_instruction(const uint8_t* bytecode, size_t offset, 
                                     char* buffer, size_t buffer_size) {
    if (!bytecode || !buffer || buffer_size == 0) {
        return -1;
    }

    // Get opcode
    VMOpcode opcode = (VMOpcode)bytecode[offset];
    
    // Format instruction
    const char* opcode_name = vm_get_opcode_name(opcode);
    snprintf(buffer, buffer_size, "%04zx: %02x %s", offset, opcode, opcode_name);
    
        return 0;
}

/**
 * Disassemble program
 */
static int vm_disassemble_program(const uint8_t* bytecode, size_t size) {
    if (!bytecode || size == 0) {
        return -1;
    }

    printf("VM Disassembly:\n");
    
    // Simple disassembly - just print opcodes
    for (size_t offset = 0; offset < size; offset++) {
        char buffer[128];
        vm_disassemble_instruction(bytecode, offset, buffer, sizeof(buffer));
        printf("%s\n", buffer);
    }
    
    return 0;
}

/**
 * Set error message
 */
static void vm_set_error(VMContext* ctx, VMErrorCode error, const char* format, ...) {
    if (!ctx) {
        return;
    }

    ctx->last_error = error;
    
    va_list args;
    va_start(args, format);
    vsnprintf(ctx->error_message, sizeof(ctx->error_message), format, args);
    va_end(args);
    
    ctx->state = VM_STATE_ERROR;
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
    {NULL, NULL}  // Sentinel
};

// Module load function
static int vm_load(void) {
    // Resolve required memory functions
    Module* memory = module_get("memory");
    if (!memory) {
        return -1;
    }
    
    mem_alloc = module_resolve(memory, "alloc");
    mem_realloc = module_resolve(memory, "realloc");
    mem_free = module_resolve(memory, "free");
    mem_copy = module_resolve(memory, "copy");
    mem_set = module_resolve(memory, "set");
    
    if (!mem_alloc || !mem_realloc || !mem_free || !mem_copy || !mem_set) {
        return -1;
    }
    
    return 0;
}

// Module unload function
static void vm_unload(void) {
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

// Module definition
static Module module_vm = {
    .name = MODULE_NAME,
    .handle = NULL,
    .state = MODULE_UNLOADED,
    .error = NULL,
    .load = vm_load,
    .unload = vm_unload,
    .resolve = vm_resolve,
    .on_init = NULL,
    .on_exit = NULL,
    .on_error = NULL
};

// Register module
REGISTER_MODULE(vm);
