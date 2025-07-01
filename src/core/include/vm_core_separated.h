/**
 * vm_core_separated.h - Separated VM Core Interface
 * 
 * Header for pure VM core focused only on bytecode execution
 */

#ifndef VM_CORE_SEPARATED_H
#define VM_CORE_SEPARATED_H

#include "core_astc.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// VM Core Error Codes
#define VM_CORE_SUCCESS           0
#define VM_CORE_ERROR_STACK_OVERFLOW   -1
#define VM_CORE_ERROR_STACK_UNDERFLOW  -2
#define VM_CORE_ERROR_INVALID_INSTRUCTION -3
#define VM_CORE_ERROR_DIVISION_BY_ZERO -4
#define VM_CORE_ERROR_MODULE_CALL_FAILED -5

// VM Core Configuration
typedef struct {
    uint32_t stack_size;        // Stack size in 32-bit words
    bool enable_module_calls;   // Enable module system integration
    bool enable_debugging;      // Enable debugging features
} VMCoreConfig;

// VM Core Statistics
typedef struct {
    uint64_t instruction_count;
    uint64_t function_calls;
    uint32_t stack_usage;
    uint32_t max_stack_usage;
    int error_count;
} VMCoreStats;

// Core VM functions

/**
 * Initialize VM core
 * @param stack_size Stack size in 32-bit words (0 for default)
 * @return VM_CORE_SUCCESS on success, error code on failure
 */
int vm_core_init(uint32_t stack_size);

/**
 * Cleanup VM core
 */
void vm_core_cleanup(void);

/**
 * Configure VM core
 * @param config Configuration structure
 * @return VM_CORE_SUCCESS on success, error code on failure
 */
int vm_core_configure(const VMCoreConfig* config);

/**
 * Execute single instruction
 * @param instruction ASTC instruction type
 * @param operands Instruction operands
 * @param operand_count Number of operands
 * @return VM_CORE_SUCCESS on success, positive for control flow, negative for error
 */
int vm_core_execute_instruction(ASTNodeType instruction, const ASTCValue* operands, int operand_count);

/**
 * Execute bytecode sequence
 * @param bytecode Bytecode buffer
 * @param size Size of bytecode in bytes
 * @return VM_CORE_SUCCESS on success, error code on failure
 */
int vm_core_execute_bytecode(const uint8_t* bytecode, size_t size);

/**
 * Delegate module call to module system
 * @param operands Call operands
 * @param operand_count Number of operands
 * @return VM_CORE_SUCCESS on success, error code on failure
 */
int vm_core_delegate_module_call(const ASTCValue* operands, int operand_count);

// State management

/**
 * Get VM core state
 * @param stack_pointer Pointer to store current stack pointer
 * @param program_counter Pointer to store current program counter
 * @param error_code Pointer to store current error code
 */
void vm_core_get_state(uint32_t* stack_pointer, uint32_t* program_counter, int* error_code);

/**
 * Get VM core statistics
 * @param instruction_count Pointer to store instruction count
 * @param function_calls Pointer to store function call count
 */
void vm_core_get_stats(uint64_t* instruction_count, uint64_t* function_calls);

/**
 * Get detailed statistics
 * @param stats Pointer to store detailed statistics
 */
void vm_core_get_detailed_stats(VMCoreStats* stats);

/**
 * Reset VM core state
 * @return VM_CORE_SUCCESS on success, error code on failure
 */
int vm_core_reset(void);

// Error handling

/**
 * Check if VM core has error
 * @return true if error, false otherwise
 */
bool vm_core_has_error(void);

/**
 * Get last error message
 * @return Error message string
 */
const char* vm_core_get_error_message(void);

/**
 * Get last error code
 * @return Error code
 */
int vm_core_get_error_code(void);

/**
 * Clear error state
 */
void vm_core_clear_error(void);

// Stack operations (for module integration)

/**
 * Push value to stack
 * @param value Value to push
 * @return VM_CORE_SUCCESS on success, error code on failure
 */
int vm_core_stack_push(uint32_t value);

/**
 * Pop value from stack
 * @return Popped value (0 if error)
 */
uint32_t vm_core_stack_pop(void);

/**
 * Peek at stack value
 * @param offset Offset from top (0 = top)
 * @return Stack value (0 if invalid offset)
 */
uint32_t vm_core_stack_peek(uint32_t offset);

/**
 * Get current stack depth
 * @return Current stack depth
 */
uint32_t vm_core_stack_depth(void);

// Register operations

/**
 * Set register value
 * @param reg Register number (0-15)
 * @param value Value to set
 * @return VM_CORE_SUCCESS on success, error code on failure
 */
int vm_core_set_register(int reg, uint64_t value);

/**
 * Get register value
 * @param reg Register number (0-15)
 * @return Register value
 */
uint64_t vm_core_get_register(int reg);

/**
 * Set flags register
 * @param flags Flags value
 */
void vm_core_set_flags(uint32_t flags);

/**
 * Get flags register
 * @return Flags value
 */
uint32_t vm_core_get_flags(void);

// Program counter operations

/**
 * Set program counter
 * @param pc Program counter value
 */
void vm_core_set_program_counter(uint32_t pc);

/**
 * Get program counter
 * @return Program counter value
 */
uint32_t vm_core_get_program_counter(void);

/**
 * Advance program counter
 * @param offset Offset to advance
 */
void vm_core_advance_program_counter(int32_t offset);

// Debugging support

/**
 * Set breakpoint
 * @param pc Program counter for breakpoint
 * @return VM_CORE_SUCCESS on success, error code on failure
 */
int vm_core_set_breakpoint(uint32_t pc);

/**
 * Remove breakpoint
 * @param pc Program counter for breakpoint
 * @return VM_CORE_SUCCESS on success, error code on failure
 */
int vm_core_remove_breakpoint(uint32_t pc);

/**
 * Check if at breakpoint
 * @return true if at breakpoint, false otherwise
 */
bool vm_core_at_breakpoint(void);

/**
 * Single step execution
 * @param bytecode Bytecode buffer
 * @param size Size of bytecode
 * @return VM_CORE_SUCCESS on success, error code on failure
 */
int vm_core_single_step(const uint8_t* bytecode, size_t size);

/**
 * Dump VM state for debugging
 * @param output_file File to write to (NULL for stdout)
 */
void vm_core_dump_state(const char* output_file);

// Module integration helpers

/**
 * Check if module system is available
 * @return true if available, false otherwise
 */
bool vm_core_module_system_available(void);

/**
 * Set module system availability
 * @param available Module system availability
 */
void vm_core_set_module_system_available(bool available);

/**
 * Register module call handler
 * @param handler Function pointer to module call handler
 * @return VM_CORE_SUCCESS on success, error code on failure
 */
int vm_core_register_module_call_handler(int (*handler)(const ASTCValue*, int));

// Utility functions

/**
 * Validate bytecode
 * @param bytecode Bytecode buffer
 * @param size Size of bytecode
 * @return true if valid, false otherwise
 */
bool vm_core_validate_bytecode(const uint8_t* bytecode, size_t size);

/**
 * Get instruction name
 * @param instruction Instruction type
 * @return Instruction name string
 */
const char* vm_core_get_instruction_name(ASTNodeType instruction);

/**
 * Get instruction operand count
 * @param instruction Instruction type
 * @return Number of operands expected
 */
int vm_core_get_instruction_operand_count(ASTNodeType instruction);

#ifdef __cplusplus
}
#endif

#endif // VM_CORE_SEPARATED_H
