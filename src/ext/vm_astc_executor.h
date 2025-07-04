/**
 * vm_astc_executor.h - ASTC Instruction Set Execution Engine Header
 * 
 * Complete ASTC instruction set execution interface for VM module.
 */

#ifndef VM_ASTC_EXECUTOR_H
#define VM_ASTC_EXECUTOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ===============================================
// ASTC Virtual Machine Types
// ===============================================

/**
 * Opaque ASTC virtual machine context
 */
typedef struct ASTCVirtualMachine ASTCVirtualMachine;

// ===============================================
// VM Lifecycle Functions
// ===============================================

/**
 * Create ASTC virtual machine
 * @return VM instance or NULL on error
 */
ASTCVirtualMachine* astc_vm_create(void);

/**
 * Free ASTC virtual machine
 * @param vm VM instance to free
 */
void astc_vm_free(ASTCVirtualMachine* vm);

/**
 * Get VM error message
 * @param vm VM instance
 * @return Error message string
 */
const char* astc_vm_get_error(ASTCVirtualMachine* vm);

// ===============================================
// VM Execution Functions
// ===============================================

/**
 * Load bytecode into VM
 * @param vm VM instance
 * @param bytecode Bytecode data
 * @param size Bytecode size
 * @return true on success, false on error
 */
bool astc_vm_load_bytecode(ASTCVirtualMachine* vm, uint8_t* bytecode, size_t size);

/**
 * Start VM execution
 * @param vm VM instance
 * @return true on success, false on error
 */
bool astc_vm_start(ASTCVirtualMachine* vm);

/**
 * Execute VM for specified number of instructions
 * @param vm VM instance
 * @param max_instructions Maximum instructions to execute
 * @return true on success, false on error
 */
bool astc_vm_execute(ASTCVirtualMachine* vm, int max_instructions);

/**
 * Run VM until completion
 * @param vm VM instance
 * @return Exit code or -1 on error
 */
int astc_vm_run(ASTCVirtualMachine* vm);

/**
 * Execute ASTC file directly
 * @param filename ASTC file path
 * @return Exit code or -1 on error
 */
int astc_vm_execute_file(const char* filename);

// ===============================================
// VM State and Statistics
// ===============================================

/**
 * Get VM execution statistics
 * @param vm VM instance
 * @param instruction_count Output instruction count
 * @param cycle_count Output cycle count
 */
void astc_vm_get_stats(ASTCVirtualMachine* vm, uint64_t* instruction_count, uint64_t* cycle_count);

// ===============================================
// ASTC Instruction Set Constants
// ===============================================

// VM Limits
#define ASTC_MAX_REGISTERS      256     // Maximum number of registers
#define ASTC_MAX_STACK_SIZE     65536   // Maximum stack size
#define ASTC_MAX_CALL_DEPTH     1024    // Maximum call stack depth

// Instruction Opcodes
#define ASTC_HALT           0x01    // Halt execution
#define ASTC_LOAD_IMM32     0x10    // Load 32-bit immediate
#define ASTC_ADD            0x20    // Add two registers
#define ASTC_CALL           0x30    // Call function
#define ASTC_EXIT           0x40    // Exit with code

// Additional instruction opcodes (for future expansion)
#define ASTC_SUB            0x21    // Subtract two registers
#define ASTC_MUL            0x22    // Multiply two registers
#define ASTC_DIV            0x23    // Divide two registers
#define ASTC_MOD            0x24    // Modulo operation
#define ASTC_AND            0x25    // Bitwise AND
#define ASTC_OR             0x26    // Bitwise OR
#define ASTC_XOR            0x27    // Bitwise XOR
#define ASTC_NOT            0x28    // Bitwise NOT
#define ASTC_SHL            0x29    // Shift left
#define ASTC_SHR            0x2A    // Shift right

#define ASTC_CMP            0x31    // Compare two registers
#define ASTC_JMP            0x32    // Unconditional jump
#define ASTC_JEQ            0x33    // Jump if equal
#define ASTC_JNE            0x34    // Jump if not equal
#define ASTC_JLT            0x35    // Jump if less than
#define ASTC_JLE            0x36    // Jump if less than or equal
#define ASTC_JGT            0x37    // Jump if greater than
#define ASTC_JGE            0x38    // Jump if greater than or equal

#define ASTC_LOAD           0x41    // Load from memory
#define ASTC_STORE          0x42    // Store to memory
#define ASTC_PUSH           0x43    // Push to stack
#define ASTC_POP            0x44    // Pop from stack

#define ASTC_SYSCALL        0x50    // System call
#define ASTC_DEBUG          0x51    // Debug instruction

#endif // VM_ASTC_EXECUTOR_H
