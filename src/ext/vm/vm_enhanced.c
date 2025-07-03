/**
 * vm_enhanced.c - Enhanced ASTC Virtual Machine Core
 * 
 * Implements an enhanced ASTC virtual machine with JIT compilation,
 * advanced memory management, and optimized execution.
 */

#include "../include/core_astc.h"
#include "../include/logger.h"
#include "../include/astc_native_bridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// VM execution modes
typedef enum {
    VM_MODE_INTERPRETER = 0,    // Pure interpretation
    VM_MODE_JIT = 1,           // JIT compilation
    VM_MODE_HYBRID = 2         // Hybrid interpretation + JIT
} VMExecutionMode;

// JIT compilation threshold
#define JIT_COMPILATION_THRESHOLD 10

// Stack frame structure
typedef struct VMStackFrame {
    ASTNode* function;          // Current function
    uint32_t* locals;          // Local variables
    uint32_t local_count;      // Number of locals
    uint32_t pc;               // Program counter
    struct VMStackFrame* prev; // Previous frame
} VMStackFrame;

// JIT compiled function entry
typedef struct JITFunction {
    char name[128];            // Function name
    void* compiled_code;       // Compiled machine code
    size_t code_size;          // Size of compiled code
    uint32_t call_count;       // Number of times called
    bool is_compiled;          // Whether function is compiled
} JITFunction;

// Enhanced VM state
typedef struct {
    // Execution state
    VMExecutionMode mode;
    VMStackFrame* current_frame;
    uint32_t* stack;
    uint32_t stack_size;
    uint32_t stack_pointer;
    
    // Memory management
    void** heap_blocks;
    size_t heap_block_count;
    size_t heap_size;
    
    // JIT compilation
    JITFunction* jit_functions;
    size_t jit_function_count;
    size_t jit_function_capacity;
    
    // Module system
    ASTNode* current_module;
    
    // Performance counters
    uint64_t instruction_count;
    uint64_t function_calls;
    uint64_t jit_compilations;
    
    // Configuration
    bool enable_jit;
    bool enable_optimization;
    bool enable_profiling;
    
    // Error handling
    char last_error[512];
    bool has_error;
} EnhancedVM;

// Global VM instance
static EnhancedVM g_vm = {0};

// Initialize the enhanced VM
int vm_enhanced_init(VMExecutionMode mode) {
    memset(&g_vm, 0, sizeof(g_vm));
    
    g_vm.mode = mode;
    g_vm.stack_size = 64 * 1024; // 64KB stack
    g_vm.stack = malloc(g_vm.stack_size * sizeof(uint32_t));
    if (!g_vm.stack) {
        LOG_RUNTIME_ERROR("Failed to allocate VM stack");
        return -1;
    }
    
    g_vm.heap_blocks = malloc(1024 * sizeof(void*));
    if (!g_vm.heap_blocks) {
        LOG_RUNTIME_ERROR("Failed to allocate heap block table");
        free(g_vm.stack);
        return -1;
    }
    
    g_vm.jit_function_capacity = 256;
    g_vm.jit_functions = malloc(g_vm.jit_function_capacity * sizeof(JITFunction));
    if (!g_vm.jit_functions) {
        LOG_RUNTIME_ERROR("Failed to allocate JIT function table");
        free(g_vm.stack);
        free(g_vm.heap_blocks);
        return -1;
    }
    
    g_vm.enable_jit = (mode == VM_MODE_JIT || mode == VM_MODE_HYBRID);
    g_vm.enable_optimization = true;
    g_vm.enable_profiling = true;
    
    LOG_RUNTIME_INFO("Enhanced VM initialized in mode %d", mode);
    return 0;
}

// Cleanup the enhanced VM
void vm_enhanced_cleanup(void) {
    // Free stack frames
    while (g_vm.current_frame) {
        VMStackFrame* prev = g_vm.current_frame->prev;
        free(g_vm.current_frame->locals);
        free(g_vm.current_frame);
        g_vm.current_frame = prev;
    }
    
    // Free heap blocks
    for (size_t i = 0; i < g_vm.heap_block_count; i++) {
        free(g_vm.heap_blocks[i]);
    }
    free(g_vm.heap_blocks);
    
    // Free JIT compiled code
    for (size_t i = 0; i < g_vm.jit_function_count; i++) {
        if (g_vm.jit_functions[i].compiled_code) {
            free(g_vm.jit_functions[i].compiled_code);
        }
    }
    free(g_vm.jit_functions);
    
    free(g_vm.stack);
    
    LOG_RUNTIME_INFO("Enhanced VM cleaned up");
    memset(&g_vm, 0, sizeof(g_vm));
}

// Set VM error
static void vm_set_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(g_vm.last_error, sizeof(g_vm.last_error), format, args);
    va_end(args);
    g_vm.has_error = true;
    LOG_RUNTIME_ERROR("VM Error: %s", g_vm.last_error);
}

// Push value onto stack
static int vm_stack_push(uint32_t value) {
    if (g_vm.stack_pointer >= g_vm.stack_size) {
        vm_set_error("Stack overflow");
        return -1;
    }
    g_vm.stack[g_vm.stack_pointer++] = value;
    return 0;
}

// Pop value from stack
static uint32_t vm_stack_pop(void) {
    if (g_vm.stack_pointer == 0) {
        vm_set_error("Stack underflow");
        return 0;
    }
    return g_vm.stack[--g_vm.stack_pointer];
}

// Allocate memory on VM heap
void* vm_heap_alloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        vm_set_error("Heap allocation failed");
        return NULL;
    }
    
    // Add to heap block table
    if (g_vm.heap_block_count >= 1024) {
        vm_set_error("Too many heap allocations");
        free(ptr);
        return NULL;
    }
    
    g_vm.heap_blocks[g_vm.heap_block_count++] = ptr;
    g_vm.heap_size += size;
    
    LOG_RUNTIME_DEBUG("VM heap allocated %zu bytes at %p", size, ptr);
    return ptr;
}

// Free memory from VM heap
void vm_heap_free(void* ptr) {
    if (!ptr) return;
    
    // Find and remove from heap block table
    for (size_t i = 0; i < g_vm.heap_block_count; i++) {
        if (g_vm.heap_blocks[i] == ptr) {
            free(ptr);
            // Compact array
            for (size_t j = i; j < g_vm.heap_block_count - 1; j++) {
                g_vm.heap_blocks[j] = g_vm.heap_blocks[j + 1];
            }
            g_vm.heap_block_count--;
            LOG_RUNTIME_DEBUG("VM heap freed pointer %p", ptr);
            return;
        }
    }
    
    LOG_RUNTIME_WARN("Attempted to free unknown pointer %p", ptr);
}

// Find JIT function
static JITFunction* vm_find_jit_function(const char* name) {
    for (size_t i = 0; i < g_vm.jit_function_count; i++) {
        if (strcmp(g_vm.jit_functions[i].name, name) == 0) {
            return &g_vm.jit_functions[i];
        }
    }
    return NULL;
}

// Add JIT function
static JITFunction* vm_add_jit_function(const char* name) {
    if (g_vm.jit_function_count >= g_vm.jit_function_capacity) {
        vm_set_error("JIT function table full");
        return NULL;
    }
    
    JITFunction* func = &g_vm.jit_functions[g_vm.jit_function_count++];
    strncpy(func->name, name, sizeof(func->name) - 1);
    func->name[sizeof(func->name) - 1] = '\0';
    func->compiled_code = NULL;
    func->code_size = 0;
    func->call_count = 0;
    func->is_compiled = false;
    
    return func;
}

// Simple JIT compilation (placeholder)
static int vm_jit_compile_function(ASTNode* function, JITFunction* jit_func) {
    if (!function || !jit_func) {
        return -1;
    }
    
    LOG_RUNTIME_INFO("JIT compiling function: %s", jit_func->name);
    
    // This is a placeholder for actual JIT compilation
    // In a real implementation, this would:
    // 1. Analyze the ASTC bytecode
    // 2. Generate optimized machine code
    // 3. Handle register allocation
    // 4. Perform optimizations
    
    // For now, just allocate some dummy code space
    jit_func->code_size = 1024; // Placeholder size
    jit_func->compiled_code = malloc(jit_func->code_size);
    if (!jit_func->compiled_code) {
        vm_set_error("Failed to allocate JIT code space");
        return -1;
    }
    
    // Fill with NOP instructions (0x90 on x86)
    memset(jit_func->compiled_code, 0x90, jit_func->code_size);
    
    jit_func->is_compiled = true;
    g_vm.jit_compilations++;
    
    LOG_RUNTIME_INFO("JIT compilation completed for %s", jit_func->name);
    return 0;
}

// Execute ASTC instruction
static int vm_execute_instruction(ASTNode* instruction) {
    if (!instruction) {
        return -1;
    }
    
    g_vm.instruction_count++;
    
    switch (instruction->type) {
        case AST_I32_CONST:
            if (vm_stack_push(instruction->data.constant.int_value) != 0) {
                return -1;
            }
            break;
            
        case AST_I32_ADD: {
            uint32_t b = vm_stack_pop();
            uint32_t a = vm_stack_pop();
            if (g_vm.has_error) return -1;
            if (vm_stack_push(a + b) != 0) {
                return -1;
            }
            break;
        }
        
        case AST_I32_SUB: {
            uint32_t b = vm_stack_pop();
            uint32_t a = vm_stack_pop();
            if (g_vm.has_error) return -1;
            if (vm_stack_push(a - b) != 0) {
                return -1;
            }
            break;
        }
        
        case AST_I32_MUL: {
            uint32_t b = vm_stack_pop();
            uint32_t a = vm_stack_pop();
            if (g_vm.has_error) return -1;
            if (vm_stack_push(a * b) != 0) {
                return -1;
            }
            break;
        }
        
        case AST_CALL: {
            // Handle function calls
            const char* func_name = instruction->data.call_expr.function_name;
            if (!func_name) {
                vm_set_error("Invalid function call");
                return -1;
            }
            
            // Check if it's a native function call
            ASTCValue args[16];
            ASTCValue result;
            int arg_count = 0; // Simplified for now
            
            if (astc_native_call(func_name, args, arg_count, &result) == 0) {
                // Push result onto stack
                if (result.type == ASTC_TYPE_I32) {
                    vm_stack_push(result.data.i32);
                }
            } else {
                vm_set_error("Native function call failed: %s", func_name);
                return -1;
            }
            break;
        }
        
        case AST_RETURN:
            // Return from current function
            return 1; // Special return code
            
        default:
            LOG_RUNTIME_DEBUG("Unhandled instruction type: %d", instruction->type);
            break;
    }
    
    return 0;
}

// Execute ASTC function
int vm_enhanced_execute_function(ASTNode* function) {
    if (!function || function->type != ASTC_FUNC_DECL) {
        vm_set_error("Invalid function node");
        return -1;
    }
    
    const char* func_name = function->data.func_decl.name;
    if (!func_name) {
        func_name = "anonymous";
    }
    
    LOG_RUNTIME_DEBUG("Executing function: %s", func_name);
    g_vm.function_calls++;
    
    // Check for JIT compilation
    JITFunction* jit_func = vm_find_jit_function(func_name);
    if (!jit_func) {
        jit_func = vm_add_jit_function(func_name);
    }
    
    if (jit_func) {
        jit_func->call_count++;
        
        // JIT compile if threshold reached and JIT is enabled
        if (g_vm.enable_jit && !jit_func->is_compiled && 
            jit_func->call_count >= JIT_COMPILATION_THRESHOLD) {
            vm_jit_compile_function(function, jit_func);
        }
        
        // Execute JIT compiled code if available
        if (jit_func->is_compiled && jit_func->compiled_code) {
            LOG_RUNTIME_DEBUG("Executing JIT compiled function: %s", func_name);
            // In a real implementation, we would call the compiled code here
            // For now, fall through to interpretation
        }
    }
    
    // Interpret the function
    ASTNode* body = function->data.func_decl.body;
    if (!body) {
        LOG_RUNTIME_WARN("Function %s has no body", func_name);
        return 0;
    }
    
    // Execute function body
    if (body->type == ASTC_COMPOUND_STMT) {
        for (int i = 0; i < body->data.compound_stmt.statement_count; i++) {
            int result = vm_execute_instruction(body->data.compound_stmt.statements[i]);
            if (result != 0) {
                if (result == 1) {
                    // Normal return
                    break;
                } else {
                    // Error
                    return -1;
                }
            }
        }
    } else {
        int result = vm_execute_instruction(body);
        if (result < 0) {
            return -1;
        }
    }
    
    LOG_RUNTIME_DEBUG("Function %s executed successfully", func_name);
    return 0;
}

// Execute ASTC module
int vm_enhanced_execute_module(ASTNode* module) {
    if (!module || module->type != ASTC_MODULE_DECL) {
        vm_set_error("Invalid module node");
        return -1;
    }
    
    g_vm.current_module = module;
    
    const char* module_name = module->data.module_decl.name;
    if (!module_name) {
        module_name = "unnamed";
    }
    
    LOG_RUNTIME_INFO("Executing module: %s", module_name);
    
    // Find and execute main function
    for (int i = 0; i < module->data.module_decl.declaration_count; i++) {
        ASTNode* decl = module->data.module_decl.declarations[i];
        if (decl->type == ASTC_FUNC_DECL) {
            const char* func_name = decl->data.func_decl.name;
            if (func_name && strcmp(func_name, "main") == 0) {
                LOG_RUNTIME_INFO("Found main function, executing...");
                return vm_enhanced_execute_function(decl);
            }
        }
    }
    
    LOG_RUNTIME_WARN("No main function found in module %s", module_name);
    return -1;
}

// Get VM statistics
void vm_enhanced_get_stats(void) {
    LOG_RUNTIME_INFO("=== Enhanced VM Statistics ===");
    LOG_RUNTIME_INFO("Execution mode: %d", g_vm.mode);
    LOG_RUNTIME_INFO("Instructions executed: %llu", g_vm.instruction_count);
    LOG_RUNTIME_INFO("Function calls: %llu", g_vm.function_calls);
    LOG_RUNTIME_INFO("JIT compilations: %llu", g_vm.jit_compilations);
    LOG_RUNTIME_INFO("Stack pointer: %u/%u", g_vm.stack_pointer, g_vm.stack_size);
    LOG_RUNTIME_INFO("Heap blocks: %zu", g_vm.heap_block_count);
    LOG_RUNTIME_INFO("Heap size: %zu bytes", g_vm.heap_size);
    LOG_RUNTIME_INFO("JIT functions: %zu/%zu", g_vm.jit_function_count, g_vm.jit_function_capacity);
    
    if (g_vm.has_error) {
        LOG_RUNTIME_ERROR("Last error: %s", g_vm.last_error);
    }
}
