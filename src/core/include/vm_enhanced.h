/**
 * vm_enhanced.h - Enhanced ASTC Virtual Machine Core
 * 
 * Header for enhanced ASTC virtual machine with JIT compilation
 */

#ifndef VM_ENHANCED_H
#define VM_ENHANCED_H

#include "core_astc.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// VM execution modes
typedef enum {
    VM_MODE_INTERPRETER = 0,    // Pure interpretation
    VM_MODE_JIT = 1,           // JIT compilation
    VM_MODE_HYBRID = 2         // Hybrid interpretation + JIT
} VMExecutionMode;

// VM configuration
typedef struct {
    VMExecutionMode mode;
    bool enable_jit;
    bool enable_optimization;
    bool enable_profiling;
    uint32_t stack_size;
    uint32_t jit_threshold;
    uint32_t max_heap_size;
} VMConfig;

// VM statistics
typedef struct {
    uint64_t instruction_count;
    uint64_t function_calls;
    uint64_t jit_compilations;
    uint32_t stack_usage;
    size_t heap_usage;
    size_t jit_function_count;
} VMStats;

// Enhanced VM functions

/**
 * Initialize the enhanced VM
 * @param mode Execution mode (interpreter, JIT, or hybrid)
 * @return 0 on success, -1 on error
 */
int vm_enhanced_init(VMExecutionMode mode);

/**
 * Cleanup the enhanced VM
 */
void vm_enhanced_cleanup(void);

/**
 * Execute an ASTC function
 * @param function Function AST node to execute
 * @return 0 on success, -1 on error
 */
int vm_enhanced_execute_function(ASTNode* function);

/**
 * Execute an ASTC module
 * @param module Module AST node to execute
 * @return 0 on success, -1 on error
 */
int vm_enhanced_execute_module(ASTNode* module);

/**
 * Get VM statistics
 */
void vm_enhanced_get_stats(void);

/**
 * Configure the VM
 * @param config VM configuration
 * @return 0 on success, -1 on error
 */
int vm_enhanced_configure(const VMConfig* config);

/**
 * Get current VM configuration
 * @param config Pointer to store current configuration
 * @return 0 on success, -1 on error
 */
int vm_enhanced_get_config(VMConfig* config);

/**
 * Reset VM state
 * @return 0 on success, -1 on error
 */
int vm_enhanced_reset(void);

/**
 * Check if VM has error
 * @return true if VM has error, false otherwise
 */
bool vm_enhanced_has_error(void);

/**
 * Get last VM error message
 * @return Error message string
 */
const char* vm_enhanced_get_error(void);

/**
 * Clear VM error state
 */
void vm_enhanced_clear_error(void);

// Memory management functions

/**
 * Allocate memory on VM heap
 * @param size Size in bytes to allocate
 * @return Pointer to allocated memory, NULL on error
 */
void* vm_heap_alloc(size_t size);

/**
 * Free memory from VM heap
 * @param ptr Pointer to memory to free
 */
void vm_heap_free(void* ptr);

/**
 * Get heap usage statistics
 * @param total_size Pointer to store total heap size
 * @param used_size Pointer to store used heap size
 * @param block_count Pointer to store number of allocated blocks
 * @return 0 on success, -1 on error
 */
int vm_heap_get_stats(size_t* total_size, size_t* used_size, size_t* block_count);

// JIT compilation functions

/**
 * Enable/disable JIT compilation
 * @param enable true to enable, false to disable
 */
void vm_jit_set_enabled(bool enable);

/**
 * Check if JIT compilation is enabled
 * @return true if enabled, false otherwise
 */
bool vm_jit_is_enabled(void);

/**
 * Set JIT compilation threshold
 * @param threshold Number of calls before JIT compilation
 */
void vm_jit_set_threshold(uint32_t threshold);

/**
 * Get JIT compilation threshold
 * @return Current JIT threshold
 */
uint32_t vm_jit_get_threshold(void);

/**
 * Force JIT compilation of a function
 * @param function Function AST node to compile
 * @return 0 on success, -1 on error
 */
int vm_jit_compile_function(ASTNode* function);

/**
 * Check if function is JIT compiled
 * @param function_name Name of the function
 * @return true if compiled, false otherwise
 */
bool vm_jit_is_function_compiled(const char* function_name);

// Debugging and profiling functions

/**
 * Enable/disable VM profiling
 * @param enable true to enable, false to disable
 */
void vm_profiling_set_enabled(bool enable);

/**
 * Get profiling data for a function
 * @param function_name Name of the function
 * @param call_count Pointer to store call count
 * @param total_time Pointer to store total execution time
 * @return 0 on success, -1 on error
 */
int vm_profiling_get_function_data(const char* function_name, 
                                  uint64_t* call_count, uint64_t* total_time);

/**
 * Reset profiling data
 */
void vm_profiling_reset(void);

/**
 * Dump VM state for debugging
 * @param output_file File to write debug info to (NULL for stdout)
 */
void vm_debug_dump_state(const char* output_file);

/**
 * Set breakpoint at instruction
 * @param function_name Function name
 * @param instruction_offset Instruction offset within function
 * @return 0 on success, -1 on error
 */
int vm_debug_set_breakpoint(const char* function_name, uint32_t instruction_offset);

/**
 * Remove breakpoint
 * @param function_name Function name
 * @param instruction_offset Instruction offset within function
 * @return 0 on success, -1 on error
 */
int vm_debug_remove_breakpoint(const char* function_name, uint32_t instruction_offset);

// Stack manipulation functions

/**
 * Get current stack depth
 * @return Current stack depth
 */
uint32_t vm_stack_get_depth(void);

/**
 * Get stack value at offset from top
 * @param offset Offset from stack top (0 = top)
 * @return Stack value, 0 if invalid offset
 */
uint32_t vm_stack_peek(uint32_t offset);

/**
 * Dump stack contents for debugging
 * @param max_entries Maximum number of entries to dump
 */
void vm_stack_dump(uint32_t max_entries);

// Error codes
#define VM_SUCCESS           0
#define VM_ERROR_INVALID     -1
#define VM_ERROR_MEMORY      -2
#define VM_ERROR_STACK_OVERFLOW -3
#define VM_ERROR_STACK_UNDERFLOW -4
#define VM_ERROR_INVALID_INSTRUCTION -5
#define VM_ERROR_FUNCTION_NOT_FOUND -6
#define VM_ERROR_JIT_COMPILATION -7

#ifdef __cplusplus
}
#endif

#endif // VM_ENHANCED_H
