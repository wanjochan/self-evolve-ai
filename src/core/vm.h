/**
 * vm.h - Core VM Engine
 * 
 * Core virtual machine engine for ASTC bytecode execution.
 * Provides the fundamental VM runtime without module dependencies.
 */

#ifndef VM_H
#define VM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "astc.h"
#include "error.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

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
// VM Context
// ===============================================

typedef struct VMContext {
    // VM state
    VMState state;
    
    // Program data
    const uint8_t* bytecode;
    size_t bytecode_size;
    size_t program_counter;
    
    // Execution stack
    uint64_t stack[VM_STACK_SIZE];
    size_t stack_pointer;
    
    // Registers
    uint64_t registers[VM_REGISTER_COUNT];
    
    // Call stack
    size_t call_stack[VM_MAX_CALL_DEPTH];
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
    ErrorCode last_error;
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
// VM Functions
// ===============================================

/**
 * Initialize VM system
 */
int vm_init(void);

/**
 * Cleanup VM system
 */
void vm_cleanup(void);

/**
 * Create VM context
 */
VMContext* vm_create_context(void);

/**
 * Destroy VM context
 */
void vm_destroy_context(VMContext* ctx);

/**
 * Load ASTC program into VM context
 */
int vm_load_program(VMContext* ctx, const uint8_t* bytecode, size_t size);

/**
 * Execute VM program
 */
int vm_execute(VMContext* ctx);

/**
 * Execute single instruction
 */
int vm_step(VMContext* ctx);

/**
 * Reset VM context
 */
void vm_reset(VMContext* ctx);

/**
 * Get VM state
 */
VMState vm_get_state(const VMContext* ctx);

/**
 * Set VM state
 */
void vm_set_state(VMContext* ctx, VMState state);

/**
 * Get execution statistics
 */
void vm_get_stats(const VMContext* ctx, uint64_t* instructions, uint64_t* cycles);

/**
 * Print VM context for debugging
 */
void vm_print_context(const VMContext* ctx);

/**
 * Validate bytecode
 */
bool vm_validate_bytecode(const uint8_t* bytecode, size_t size);

// ===============================================
// VM Utilities
// ===============================================

/**
 * Get opcode name for debugging
 */
const char* vm_get_opcode_name(VMOpcode opcode);

/**
 * Disassemble instruction
 */
int vm_disassemble_instruction(const uint8_t* bytecode, size_t offset, 
                              char* buffer, size_t buffer_size);

/**
 * Disassemble program
 */
int vm_disassemble_program(const uint8_t* bytecode, size_t size);

#ifdef __cplusplus
}
#endif

#endif // VM_H
