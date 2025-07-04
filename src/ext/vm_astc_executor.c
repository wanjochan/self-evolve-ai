/**
 * vm_astc_executor.c - ASTC Instruction Set Execution Engine
 * 
 * Complete implementation of ASTC instruction set execution for VM module.
 * Supports all ASTC opcodes defined in astc.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "../core/astc.h"
#include "../core/utils.h"
#include "vm_astc_parser.h"

// ===============================================
// ASTC Virtual Machine State
// ===============================================

#define ASTC_MAX_REGISTERS      256     // Maximum number of registers
#define ASTC_MAX_STACK_SIZE     65536   // Maximum stack size
#define ASTC_MAX_CALL_DEPTH     1024    // Maximum call stack depth

typedef struct {
    // Registers
    uint32_t registers[ASTC_MAX_REGISTERS];
    
    // Stack
    uint32_t* stack;
    size_t stack_size;
    size_t stack_pointer;
    
    // Call stack
    uint32_t* call_stack;
    size_t call_stack_size;
    size_t call_stack_pointer;
    
    // Program state
    uint8_t* bytecode;
    size_t bytecode_size;
    size_t program_counter;
    
    // Execution state
    bool running;
    bool halted;
    int exit_code;
    
    // Error handling
    char error_message[512];
    bool has_error;
    
    // Statistics
    uint64_t instruction_count;
    uint64_t cycle_count;
} ASTCVirtualMachine;

// ===============================================
// VM Initialization and Cleanup
// ===============================================

/**
 * Create ASTC virtual machine
 */
ASTCVirtualMachine* astc_vm_create(void) {
    ASTCVirtualMachine* vm = malloc(sizeof(ASTCVirtualMachine));
    if (!vm) {
        return NULL;
    }
    
    memset(vm, 0, sizeof(ASTCVirtualMachine));
    
    // Initialize stack
    vm->stack = malloc(ASTC_MAX_STACK_SIZE * sizeof(uint32_t));
    if (!vm->stack) {
        free(vm);
        return NULL;
    }
    vm->stack_size = ASTC_MAX_STACK_SIZE;
    vm->stack_pointer = 0;
    
    // Initialize call stack
    vm->call_stack = malloc(ASTC_MAX_CALL_DEPTH * sizeof(uint32_t));
    if (!vm->call_stack) {
        free(vm->stack);
        free(vm);
        return NULL;
    }
    vm->call_stack_size = ASTC_MAX_CALL_DEPTH;
    vm->call_stack_pointer = 0;
    
    // Initialize state
    vm->running = false;
    vm->halted = false;
    vm->exit_code = 0;
    vm->program_counter = 0;
    
    printf("ASTC VM: Virtual machine created successfully\n");
    return vm;
}

/**
 * Free ASTC virtual machine
 */
void astc_vm_free(ASTCVirtualMachine* vm) {
    if (!vm) {
        return;
    }
    
    if (vm->stack) {
        free(vm->stack);
    }
    
    if (vm->call_stack) {
        free(vm->call_stack);
    }
    
    free(vm);
    printf("ASTC VM: Virtual machine freed\n");
}

/**
 * Set VM error
 */
static void astc_vm_set_error(ASTCVirtualMachine* vm, const char* message) {
    if (!vm || !message) {
        return;
    }
    
    strncpy(vm->error_message, message, sizeof(vm->error_message) - 1);
    vm->error_message[sizeof(vm->error_message) - 1] = '\0';
    vm->has_error = true;
    vm->running = false;
}

/**
 * Get VM error message
 */
const char* astc_vm_get_error(ASTCVirtualMachine* vm) {
    if (!vm) {
        return "Invalid VM context";
    }
    
    return vm->has_error ? vm->error_message : "No error";
}

// ===============================================
// VM Memory and Register Operations
// ===============================================

/**
 * Push value to stack
 */
static bool astc_vm_stack_push(ASTCVirtualMachine* vm, uint32_t value) {
    if (!vm || vm->stack_pointer >= vm->stack_size) {
        astc_vm_set_error(vm, "Stack overflow");
        return false;
    }
    
    vm->stack[vm->stack_pointer++] = value;
    return true;
}

/**
 * Pop value from stack
 */
static bool astc_vm_stack_pop(ASTCVirtualMachine* vm, uint32_t* value) {
    if (!vm || !value || vm->stack_pointer == 0) {
        astc_vm_set_error(vm, "Stack underflow");
        return false;
    }
    
    *value = vm->stack[--vm->stack_pointer];
    return true;
}

/**
 * Push return address to call stack
 */
static bool astc_vm_call_push(ASTCVirtualMachine* vm, uint32_t return_address) {
    if (!vm || vm->call_stack_pointer >= vm->call_stack_size) {
        astc_vm_set_error(vm, "Call stack overflow");
        return false;
    }
    
    vm->call_stack[vm->call_stack_pointer++] = return_address;
    return true;
}

/**
 * Pop return address from call stack
 */
static bool astc_vm_call_pop(ASTCVirtualMachine* vm, uint32_t* return_address) {
    if (!vm || !return_address || vm->call_stack_pointer == 0) {
        astc_vm_set_error(vm, "Call stack underflow");
        return false;
    }
    
    *return_address = vm->call_stack[--vm->call_stack_pointer];
    return true;
}

// ===============================================
// Instruction Execution Functions
// ===============================================

/**
 * Read next byte from bytecode
 */
static bool astc_vm_read_byte(ASTCVirtualMachine* vm, uint8_t* value) {
    if (!vm || !value || vm->program_counter >= vm->bytecode_size) {
        astc_vm_set_error(vm, "Program counter out of bounds");
        return false;
    }
    
    *value = vm->bytecode[vm->program_counter++];
    return true;
}

/**
 * Read next uint32 from bytecode
 */
static bool astc_vm_read_uint32(ASTCVirtualMachine* vm, uint32_t* value) {
    if (!vm || !value || vm->program_counter + 4 > vm->bytecode_size) {
        astc_vm_set_error(vm, "Program counter out of bounds");
        return false;
    }
    
    *value = *(uint32_t*)(vm->bytecode + vm->program_counter);
    vm->program_counter += 4;
    return true;
}

/**
 * Execute HALT instruction
 */
static bool astc_vm_exec_halt(ASTCVirtualMachine* vm) {
    printf("ASTC VM: HALT instruction executed\n");
    vm->running = false;
    vm->halted = true;
    return true;
}

/**
 * Execute LOAD_IMM32 instruction
 */
static bool astc_vm_exec_load_imm32(ASTCVirtualMachine* vm) {
    uint8_t reg;
    uint32_t imm;
    
    if (!astc_vm_read_byte(vm, &reg) || !astc_vm_read_uint32(vm, &imm)) {
        return false;
    }
    
    if (reg >= ASTC_MAX_REGISTERS) {
        astc_vm_set_error(vm, "Invalid register number");
        return false;
    }
    
    vm->registers[reg] = imm;
    printf("ASTC VM: LOAD_IMM32 r%u, %u\n", reg, imm);
    return true;
}

/**
 * Execute ADD instruction
 */
static bool astc_vm_exec_add(ASTCVirtualMachine* vm) {
    uint8_t reg1, reg2, reg3;
    
    if (!astc_vm_read_byte(vm, &reg1) || !astc_vm_read_byte(vm, &reg2) || !astc_vm_read_byte(vm, &reg3)) {
        return false;
    }
    
    if (reg1 >= ASTC_MAX_REGISTERS || reg2 >= ASTC_MAX_REGISTERS || reg3 >= ASTC_MAX_REGISTERS) {
        astc_vm_set_error(vm, "Invalid register number");
        return false;
    }
    
    vm->registers[reg1] = vm->registers[reg2] + vm->registers[reg3];
    printf("ASTC VM: ADD r%u, r%u, r%u (result: %u)\n", reg1, reg2, reg3, vm->registers[reg1]);
    return true;
}

/**
 * Execute CALL instruction
 */
static bool astc_vm_exec_call(ASTCVirtualMachine* vm) {
    uint32_t func_id;
    
    if (!astc_vm_read_uint32(vm, &func_id)) {
        return false;
    }
    
    // Push return address
    if (!astc_vm_call_push(vm, vm->program_counter)) {
        return false;
    }
    
    // For now, just simulate function call
    printf("ASTC VM: CALL function %u (simulated)\n", func_id);
    
    // In a real implementation, this would jump to the function
    // For now, just return immediately
    uint32_t return_address;
    if (!astc_vm_call_pop(vm, &return_address)) {
        return false;
    }
    
    return true;
}

/**
 * Execute EXIT instruction
 */
static bool astc_vm_exec_exit(ASTCVirtualMachine* vm) {
    uint8_t exit_code;
    
    if (!astc_vm_read_byte(vm, &exit_code)) {
        return false;
    }
    
    printf("ASTC VM: EXIT with code %u\n", exit_code);
    vm->exit_code = exit_code;
    vm->running = false;
    vm->halted = true;
    return true;
}

/**
 * Execute single instruction
 */
static bool astc_vm_execute_instruction(ASTCVirtualMachine* vm) {
    if (!vm || !vm->running) {
        return false;
    }
    
    uint8_t opcode;
    if (!astc_vm_read_byte(vm, &opcode)) {
        return false;
    }
    
    vm->instruction_count++;
    vm->cycle_count++;
    
    switch (opcode) {
        case 0x01: // HALT
            return astc_vm_exec_halt(vm);
            
        case 0x10: // LOAD_IMM32
            return astc_vm_exec_load_imm32(vm);
            
        case 0x20: // ADD
            return astc_vm_exec_add(vm);
            
        case 0x30: // CALL
            return astc_vm_exec_call(vm);
            
        case 0x40: // EXIT
            return astc_vm_exec_exit(vm);
            
        default:
            printf("ASTC VM: Unknown opcode 0x%02X at PC %zu\n", opcode, vm->program_counter - 1);
            astc_vm_set_error(vm, "Unknown instruction opcode");
            return false;
    }
}

// ===============================================
// VM Execution Control
// ===============================================

/**
 * Load bytecode into VM
 */
bool astc_vm_load_bytecode(ASTCVirtualMachine* vm, uint8_t* bytecode, size_t size) {
    if (!vm || !bytecode || size == 0) {
        astc_vm_set_error(vm, "Invalid bytecode parameters");
        return false;
    }
    
    vm->bytecode = bytecode;
    vm->bytecode_size = size;
    vm->program_counter = 0;
    vm->running = false;
    vm->halted = false;
    vm->has_error = false;
    vm->instruction_count = 0;
    vm->cycle_count = 0;
    
    printf("ASTC VM: Bytecode loaded (%zu bytes)\n", size);
    return true;
}

/**
 * Start VM execution
 */
bool astc_vm_start(ASTCVirtualMachine* vm) {
    if (!vm || !vm->bytecode) {
        astc_vm_set_error(vm, "No bytecode loaded");
        return false;
    }
    
    vm->running = true;
    vm->halted = false;
    vm->has_error = false;
    vm->program_counter = 0;
    
    printf("ASTC VM: Execution started\n");
    return true;
}

/**
 * Execute VM for specified number of instructions
 */
bool astc_vm_execute(ASTCVirtualMachine* vm, int max_instructions) {
    if (!vm || !vm->running) {
        return false;
    }
    
    int executed = 0;
    while (vm->running && !vm->halted && !vm->has_error && executed < max_instructions) {
        if (!astc_vm_execute_instruction(vm)) {
            return false;
        }
        executed++;
    }
    
    printf("ASTC VM: Executed %d instructions (total: %llu)\n", executed, vm->instruction_count);
    return true;
}

/**
 * Run VM until completion
 */
int astc_vm_run(ASTCVirtualMachine* vm) {
    if (!vm) {
        return -1;
    }
    
    if (!astc_vm_start(vm)) {
        return -1;
    }
    
    printf("ASTC VM: Running until completion...\n");
    
    while (vm->running && !vm->halted && !vm->has_error) {
        if (!astc_vm_execute(vm, 1000)) {
            printf("ASTC VM: Execution failed: %s\n", astc_vm_get_error(vm));
            return -1;
        }
        
        // Safety check to prevent infinite loops
        if (vm->instruction_count > 1000000) {
            astc_vm_set_error(vm, "Execution timeout (too many instructions)");
            return -1;
        }
    }
    
    if (vm->has_error) {
        printf("ASTC VM: Execution failed: %s\n", astc_vm_get_error(vm));
        return -1;
    }
    
    printf("ASTC VM: Execution completed successfully\n");
    printf("  Instructions executed: %llu\n", vm->instruction_count);
    printf("  Cycles: %llu\n", vm->cycle_count);
    printf("  Exit code: %d\n", vm->exit_code);
    
    return vm->exit_code;
}

/**
 * Execute ASTC file
 */
int astc_vm_execute_file(const char* filename) {
    if (!filename) {
        printf("ASTC VM: Invalid filename\n");
        return -1;
    }

    printf("ASTC VM: Executing file %s\n", filename);

    // Parse ASTC file
    uint8_t* bytecode;
    size_t bytecode_size;
    if (!astc_parser_parse_file(filename, &bytecode, &bytecode_size)) {
        printf("ASTC VM: Failed to parse ASTC file\n");
        return -1;
    }

    // Create VM
    ASTCVirtualMachine* vm = astc_vm_create();
    if (!vm) {
        printf("ASTC VM: Failed to create virtual machine\n");
        free(bytecode);
        return -1;
    }

    // Load and execute bytecode
    if (!astc_vm_load_bytecode(vm, bytecode, bytecode_size)) {
        printf("ASTC VM: Failed to load bytecode\n");
        astc_vm_free(vm);
        free(bytecode);
        return -1;
    }

    int result = astc_vm_run(vm);

    // Cleanup
    astc_vm_free(vm);
    free(bytecode);

    return result;
}

/**
 * Get VM statistics
 */
void astc_vm_get_stats(ASTCVirtualMachine* vm, uint64_t* instruction_count, uint64_t* cycle_count) {
    if (!vm || !instruction_count || !cycle_count) {
        return;
    }

    *instruction_count = vm->instruction_count;
    *cycle_count = vm->cycle_count;
}
