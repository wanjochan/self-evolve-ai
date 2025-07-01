/**
 * vm_core_separated.c - Separated VM Core Implementation
 * 
 * Pure VM core focused only on bytecode execution, stack management,
 * and basic instruction processing. All other functionality moved to modules.
 */

#include "../include/core_astc.h"
#include "../include/logger.h"
#include "../include/module_communication.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// VM Core State (minimal)
typedef struct {
    // Execution state
    uint32_t* stack;
    uint32_t stack_size;
    uint32_t stack_pointer;
    uint32_t program_counter;
    
    // Current execution context
    ASTNode* current_module;
    ASTNode* current_function;
    
    // Basic registers
    uint64_t registers[16];
    uint32_t flags;
    
    // Error state
    int error_code;
    char error_message[256];
    
    // Statistics (minimal)
    uint64_t instruction_count;
    uint64_t function_calls;
    
    // Module interface
    bool module_system_available;
} VMCoreState;

// Global VM core state
static VMCoreState g_vm_core = {0};

// VM Core Error Codes
#define VM_CORE_SUCCESS           0
#define VM_CORE_ERROR_STACK_OVERFLOW   -1
#define VM_CORE_ERROR_STACK_UNDERFLOW  -2
#define VM_CORE_ERROR_INVALID_INSTRUCTION -3
#define VM_CORE_ERROR_DIVISION_BY_ZERO -4
#define VM_CORE_ERROR_MODULE_CALL_FAILED -5

// Initialize VM core
int vm_core_init(uint32_t stack_size) {
    memset(&g_vm_core, 0, sizeof(g_vm_core));
    
    g_vm_core.stack_size = stack_size ? stack_size : 64 * 1024; // Default 64KB
    g_vm_core.stack = malloc(g_vm_core.stack_size * sizeof(uint32_t));
    if (!g_vm_core.stack) {
        LOG_VM_ERROR("Failed to allocate VM stack");
        return VM_CORE_ERROR_STACK_OVERFLOW;
    }
    
    g_vm_core.stack_pointer = 0;
    g_vm_core.program_counter = 0;
    g_vm_core.error_code = VM_CORE_SUCCESS;
    
    // Check if module system is available
    g_vm_core.module_system_available = (module_comm_is_initialized() == 0);
    
    LOG_VM_INFO("VM Core initialized with %u stack slots", g_vm_core.stack_size);
    LOG_VM_INFO("Module system available: %s", g_vm_core.module_system_available ? "Yes" : "No");
    
    return VM_CORE_SUCCESS;
}

// Cleanup VM core
void vm_core_cleanup(void) {
    if (g_vm_core.stack) {
        free(g_vm_core.stack);
        g_vm_core.stack = NULL;
    }
    
    LOG_VM_INFO("VM Core cleaned up");
    LOG_VM_INFO("Final stats - Instructions: %llu, Function calls: %llu", 
               g_vm_core.instruction_count, g_vm_core.function_calls);
}

// Set VM error
static void vm_core_set_error(int error_code, const char* message) {
    g_vm_core.error_code = error_code;
    strncpy(g_vm_core.error_message, message, sizeof(g_vm_core.error_message) - 1);
    g_vm_core.error_message[sizeof(g_vm_core.error_message) - 1] = '\0';
    LOG_VM_ERROR("VM Core Error %d: %s", error_code, message);
}

// Stack operations
static int vm_stack_push(uint32_t value) {
    if (g_vm_core.stack_pointer >= g_vm_core.stack_size) {
        vm_core_set_error(VM_CORE_ERROR_STACK_OVERFLOW, "Stack overflow");
        return VM_CORE_ERROR_STACK_OVERFLOW;
    }
    g_vm_core.stack[g_vm_core.stack_pointer++] = value;
    return VM_CORE_SUCCESS;
}

static uint32_t vm_stack_pop(void) {
    if (g_vm_core.stack_pointer == 0) {
        vm_core_set_error(VM_CORE_ERROR_STACK_UNDERFLOW, "Stack underflow");
        return 0;
    }
    return g_vm_core.stack[--g_vm_core.stack_pointer];
}

static uint32_t vm_stack_peek(uint32_t offset) {
    if (offset >= g_vm_core.stack_pointer) {
        return 0;
    }
    return g_vm_core.stack[g_vm_core.stack_pointer - 1 - offset];
}

// Execute single instruction (core VM functionality only)
int vm_core_execute_instruction(ASTNodeType instruction, const ASTCValue* operands, int operand_count) {
    g_vm_core.instruction_count++;
    
    switch (instruction) {
        // Stack operations
        case AST_NOP:
            // No operation
            break;
            
        case AST_I32_CONST:
            if (operand_count > 0) {
                return vm_stack_push(operands[0].data.i32);
            }
            break;
            
        case AST_I64_CONST:
            if (operand_count > 0) {
                // Push 64-bit value as two 32-bit values
                uint64_t value = operands[0].data.i64;
                if (vm_stack_push((uint32_t)(value & 0xFFFFFFFF)) != VM_CORE_SUCCESS) return g_vm_core.error_code;
                return vm_stack_push((uint32_t)(value >> 32));
            }
            break;
            
        case AST_DROP:
            vm_stack_pop();
            return g_vm_core.error_code;
            
        // Arithmetic operations
        case AST_I32_ADD: {
            uint32_t b = vm_stack_pop();
            uint32_t a = vm_stack_pop();
            if (g_vm_core.error_code != VM_CORE_SUCCESS) return g_vm_core.error_code;
            return vm_stack_push(a + b);
        }
        
        case AST_I32_SUB: {
            uint32_t b = vm_stack_pop();
            uint32_t a = vm_stack_pop();
            if (g_vm_core.error_code != VM_CORE_SUCCESS) return g_vm_core.error_code;
            return vm_stack_push(a - b);
        }
        
        case AST_I32_MUL: {
            uint32_t b = vm_stack_pop();
            uint32_t a = vm_stack_pop();
            if (g_vm_core.error_code != VM_CORE_SUCCESS) return g_vm_core.error_code;
            return vm_stack_push(a * b);
        }
        
        case AST_I32_DIV_S: {
            uint32_t b = vm_stack_pop();
            uint32_t a = vm_stack_pop();
            if (g_vm_core.error_code != VM_CORE_SUCCESS) return g_vm_core.error_code;
            if (b == 0) {
                vm_core_set_error(VM_CORE_ERROR_DIVISION_BY_ZERO, "Division by zero");
                return VM_CORE_ERROR_DIVISION_BY_ZERO;
            }
            return vm_stack_push((int32_t)a / (int32_t)b);
        }
        
        // Bitwise operations
        case AST_I32_AND: {
            uint32_t b = vm_stack_pop();
            uint32_t a = vm_stack_pop();
            if (g_vm_core.error_code != VM_CORE_SUCCESS) return g_vm_core.error_code;
            return vm_stack_push(a & b);
        }
        
        case AST_I32_OR: {
            uint32_t b = vm_stack_pop();
            uint32_t a = vm_stack_pop();
            if (g_vm_core.error_code != VM_CORE_SUCCESS) return g_vm_core.error_code;
            return vm_stack_push(a | b);
        }
        
        case AST_I32_XOR: {
            uint32_t b = vm_stack_pop();
            uint32_t a = vm_stack_pop();
            if (g_vm_core.error_code != VM_CORE_SUCCESS) return g_vm_core.error_code;
            return vm_stack_push(a ^ b);
        }
        
        // Comparison operations
        case AST_I32_EQ: {
            uint32_t b = vm_stack_pop();
            uint32_t a = vm_stack_pop();
            if (g_vm_core.error_code != VM_CORE_SUCCESS) return g_vm_core.error_code;
            return vm_stack_push(a == b ? 1 : 0);
        }
        
        case AST_I32_NE: {
            uint32_t b = vm_stack_pop();
            uint32_t a = vm_stack_pop();
            if (g_vm_core.error_code != VM_CORE_SUCCESS) return g_vm_core.error_code;
            return vm_stack_push(a != b ? 1 : 0);
        }
        
        case AST_I32_LT_S: {
            uint32_t b = vm_stack_pop();
            uint32_t a = vm_stack_pop();
            if (g_vm_core.error_code != VM_CORE_SUCCESS) return g_vm_core.error_code;
            return vm_stack_push((int32_t)a < (int32_t)b ? 1 : 0);
        }
        
        // Control flow (basic)
        case AST_BR:
            // Unconditional branch - handled by caller
            return 1; // Signal branch
            
        case AST_BR_IF: {
            uint32_t condition = vm_stack_pop();
            if (g_vm_core.error_code != VM_CORE_SUCCESS) return g_vm_core.error_code;
            return condition ? 1 : 0; // Signal conditional branch
        }
        
        case AST_RETURN:
            return 2; // Signal return
            
        // Module calls (delegated to module system)
        case AST_CALL:
            if (g_vm_core.module_system_available) {
                // Delegate to module system
                g_vm_core.function_calls++;
                return vm_core_delegate_module_call(operands, operand_count);
            } else {
                vm_core_set_error(VM_CORE_ERROR_MODULE_CALL_FAILED, "Module system not available");
                return VM_CORE_ERROR_MODULE_CALL_FAILED;
            }
            
        default:
            vm_core_set_error(VM_CORE_ERROR_INVALID_INSTRUCTION, "Unsupported instruction");
            return VM_CORE_ERROR_INVALID_INSTRUCTION;
    }
    
    return VM_CORE_SUCCESS;
}

// Delegate module call to module system
int vm_core_delegate_module_call(const ASTCValue* operands, int operand_count) {
    if (!g_vm_core.module_system_available) {
        return VM_CORE_ERROR_MODULE_CALL_FAILED;
    }
    
    // This is where the VM core interfaces with the module system
    // The actual implementation depends on the module communication protocol
    
    LOG_VM_DEBUG("Delegating module call with %d operands", operand_count);
    
    // For now, return success - actual implementation would:
    // 1. Extract function name from operands
    // 2. Call module_comm_call_function()
    // 3. Handle return value and push to stack
    
    return VM_CORE_SUCCESS;
}

// Execute bytecode sequence
int vm_core_execute_bytecode(const uint8_t* bytecode, size_t size) {
    if (!bytecode || size == 0) {
        vm_core_set_error(VM_CORE_ERROR_INVALID_INSTRUCTION, "Invalid bytecode");
        return VM_CORE_ERROR_INVALID_INSTRUCTION;
    }
    
    g_vm_core.program_counter = 0;
    
    while (g_vm_core.program_counter < size) {
        // Simple bytecode format: [opcode][operand_count][operands...]
        uint8_t opcode = bytecode[g_vm_core.program_counter++];
        
        if (g_vm_core.program_counter >= size) {
            vm_core_set_error(VM_CORE_ERROR_INVALID_INSTRUCTION, "Unexpected end of bytecode");
            return VM_CORE_ERROR_INVALID_INSTRUCTION;
        }
        
        uint8_t operand_count = bytecode[g_vm_core.program_counter++];
        
        // Extract operands (simplified)
        ASTCValue operands[4] = {0}; // Max 4 operands
        for (int i = 0; i < operand_count && i < 4; i++) {
            if (g_vm_core.program_counter + 4 > size) {
                vm_core_set_error(VM_CORE_ERROR_INVALID_INSTRUCTION, "Invalid operand");
                return VM_CORE_ERROR_INVALID_INSTRUCTION;
            }
            
            operands[i].type = ASTC_TYPE_I32;
            operands[i].data.i32 = *(uint32_t*)(bytecode + g_vm_core.program_counter);
            g_vm_core.program_counter += 4;
        }
        
        // Execute instruction
        int result = vm_core_execute_instruction((ASTNodeType)opcode, operands, operand_count);
        
        if (result < 0) {
            return result; // Error
        } else if (result == 1) {
            // Branch - would need branch target from operands
            LOG_VM_DEBUG("Branch instruction executed");
        } else if (result == 2) {
            // Return
            LOG_VM_DEBUG("Return instruction executed");
            break;
        }
    }
    
    return VM_CORE_SUCCESS;
}

// Get VM core state
void vm_core_get_state(uint32_t* stack_pointer, uint32_t* program_counter, int* error_code) {
    if (stack_pointer) *stack_pointer = g_vm_core.stack_pointer;
    if (program_counter) *program_counter = g_vm_core.program_counter;
    if (error_code) *error_code = g_vm_core.error_code;
}

// Get VM core statistics
void vm_core_get_stats(uint64_t* instruction_count, uint64_t* function_calls) {
    if (instruction_count) *instruction_count = g_vm_core.instruction_count;
    if (function_calls) *function_calls = g_vm_core.function_calls;
}

// Reset VM core state
int vm_core_reset(void) {
    g_vm_core.stack_pointer = 0;
    g_vm_core.program_counter = 0;
    g_vm_core.error_code = VM_CORE_SUCCESS;
    memset(g_vm_core.registers, 0, sizeof(g_vm_core.registers));
    g_vm_core.flags = 0;
    g_vm_core.error_message[0] = '\0';
    
    LOG_VM_DEBUG("VM Core state reset");
    return VM_CORE_SUCCESS;
}

// Check if VM core has error
bool vm_core_has_error(void) {
    return g_vm_core.error_code != VM_CORE_SUCCESS;
}

// Get last error message
const char* vm_core_get_error_message(void) {
    return g_vm_core.error_message;
}

// Get last error code
int vm_core_get_error_code(void) {
    return g_vm_core.error_code;
}

// Clear error state
void vm_core_clear_error(void) {
    g_vm_core.error_code = VM_CORE_SUCCESS;
    g_vm_core.error_message[0] = '\0';
}
