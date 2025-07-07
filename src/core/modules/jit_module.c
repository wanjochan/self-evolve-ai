/**
 * jit_module.c - JIT Compiler Module
 * 
 * Provides JIT compilation capabilities as a module.
 * Depends on the memory and utils modules.
 */

#include "../module.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// Module name
static const char* MODULE_NAME = "jit";

// Dependencies
MODULE_DEPENDS_ON(memory);
MODULE_DEPENDS_ON(utils);

// Function type definitions for memory module functions
typedef void* (*memory_alloc_t)(size_t size, int pool);
typedef void (*memory_free_t)(void* ptr);
typedef void* (*memory_calloc_t)(size_t count, size_t size, int pool);
typedef void* (*allocate_executable_memory_t)(size_t size);
typedef void (*free_executable_memory_t)(void* ptr, size_t size);

// Function type definitions for utils module functions
typedef int (*detect_architecture_t)(void);
typedef const char* (*get_architecture_name_t)(int arch);

// Cached memory functions
static memory_alloc_t mem_alloc;
static memory_free_t mem_free;
static memory_calloc_t mem_calloc;
static allocate_executable_memory_t alloc_exec_mem;
static free_executable_memory_t free_exec_mem;

// Cached utils functions
static detect_architecture_t detect_arch;
static get_architecture_name_t get_arch_name;

// ===============================================
// Memory Pool Types (from memory.h)
// ===============================================

typedef enum {
    MEMORY_POOL_GENERAL,    // General purpose allocations
    MEMORY_POOL_BYTECODE,   // ASTC bytecode storage
    MEMORY_POOL_JIT,        // JIT compiled code
    MEMORY_POOL_MODULES,    // Native modules
    MEMORY_POOL_TEMP,       // Temporary allocations
    MEMORY_POOL_C99_AST,    // C99 AST nodes
    MEMORY_POOL_C99_SYMBOLS,// C99 symbol table
    MEMORY_POOL_C99_STRINGS,// C99 string literals
    MEMORY_POOL_COUNT
} MemoryPoolType;

// ===============================================
// Architecture Types (from utils.h)
// ===============================================

typedef enum {
    ARCH_UNKNOWN = 0,
    ARCH_X86_32,
    ARCH_X86_64,
    ARCH_ARM32,
    ARCH_ARM64,
    ARCH_RISCV32,
    ARCH_RISCV64,
    ARCH_MIPS32,
    ARCH_MIPS64,
    ARCH_PPC32,
    ARCH_PPC64,
    ARCH_COUNT
} DetectedArchitecture;

// ===============================================
// JIT Extension Types and Enums
// ===============================================

/**
 * JIT availability status
 */
typedef enum {
    JIT_AVAILABLE = 0,
    JIT_UNAVAILABLE = 1,
    JIT_DISABLED = 2,
    JIT_ERROR = 3
} JITAvailability;

/**
 * JIT compilation status codes
 */
typedef enum {
    JIT_SUCCESS = 0,
    JIT_ERROR_INVALID_INPUT = -1,
    JIT_ERROR_MEMORY_ALLOCATION = -2,
    JIT_ERROR_UNSUPPORTED_ARCH = -3,
    JIT_ERROR_COMPILATION_FAILED = -4,
    JIT_ERROR_BUFFER_OVERFLOW = -5,
    JIT_ERROR_NOT_AVAILABLE = -6
} JITResult;

/**
 * JIT optimization levels
 */
typedef enum {
    JIT_OPT_NONE = 0,       // No optimization
    JIT_OPT_BASIC = 1,      // Basic optimizations
    JIT_OPT_AGGRESSIVE = 2  // Aggressive optimizations
} JITOptLevel;

/**
 * JIT compilation flags
 */
typedef enum {
    JIT_FLAG_NONE = 0,
    JIT_FLAG_DEBUG_INFO = 1,        // Include debug information
    JIT_FLAG_PROFILE = 2,           // Enable profiling
    JIT_FLAG_CACHE_RESULT = 4,      // Cache compilation result
    JIT_FLAG_VERIFY_CODE = 8,       // Verify generated code
    JIT_FLAG_C99_MODE = 16,         // C99 compiler mode
    JIT_FLAG_OPTIMIZE_C99 = 32      // C99-specific optimizations
} JITFlags;

// ===============================================
// JIT Extension Structures
// ===============================================

/**
 * JIT Compiler Context (Optional Extension)
 */
typedef struct JITCompiler {
    DetectedArchitecture target_arch;       // Target architecture
    JITOptLevel opt_level;                  // Optimization level
    uint32_t flags;                         // Compilation flags
    
    // Code generation buffers
    uint8_t* code_buffer;                   // Generated machine code
    size_t code_size;                       // Current code size
    size_t code_capacity;                   // Buffer capacity
    
    // Compilation state
    uint32_t* label_table;                  // Jump label addresses
    size_t label_count;                     // Number of labels
    size_t label_capacity;                  // Label table capacity
    
    // Error handling
    JITResult last_error;                   // Last error code
    char error_message[512];                // Error message
    
    // Statistics
    size_t bytes_compiled;                  // Total bytes compiled
    size_t functions_compiled;              // Number of functions compiled
    uint64_t compilation_time_us;           // Total compilation time in microseconds
} JITCompiler;

/**
 * JIT Context for ASTC compilation
 */
typedef struct JITContext {
    uint32_t target_arch;
    uint32_t target_bits;
    void* compiler_state;
    bool initialized;
} JITContext;

/**
 * C99-specific JIT compilation context
 */
typedef struct C99JITContext {
    JITCompiler* base_jit;              // Base JIT compiler
    void* ast_root;                      // C99 AST root (void* to avoid dependency)
    char* source_file;                   // Source file path
    char* target_arch;                   // Target architecture
    int optimization_level;              // Optimization level
    bool debug_mode;                     // Debug mode flag

    // C99-specific state
    uint32_t function_count;             // Number of functions
    uint32_t variable_count;             // Number of variables
    uint32_t* function_addresses;        // Function entry points
    char** function_names;               // Function names
} C99JITContext;

// ===============================================
// JIT Extension Implementation
// ===============================================

static bool jit_initialized = false;

/**
 * Check JIT availability
 */
static JITAvailability jit_check_availability(void) {
    // Check if current architecture supports JIT
    DetectedArchitecture arch = detect_arch();
    
    switch (arch) {
        case ARCH_X86_64:
        case ARCH_X86_32:
            return JIT_AVAILABLE;
        case ARCH_ARM64:
        case ARCH_ARM32:
            return JIT_UNAVAILABLE; // Not implemented yet
        default:
            return JIT_UNAVAILABLE;
    }
}

/**
 * Check if architecture is supported
 */
static bool jit_is_arch_supported(DetectedArchitecture arch) {
    switch (arch) {
        case ARCH_X86_64:
        case ARCH_X86_32:
            return true;
        default:
            return false;
    }
}

/**
 * Initialize JIT compiler
 */
static JITCompiler* jit_init_compiler(DetectedArchitecture target_arch, JITOptLevel opt_level, uint32_t flags) {
    if (jit_check_availability() != JIT_AVAILABLE) {
        return NULL;
    }
    
    JITCompiler* jit = mem_alloc(sizeof(JITCompiler), MEMORY_POOL_JIT);
    if (!jit) {
        return NULL;
    }
    
    memset(jit, 0, sizeof(JITCompiler));
    jit->target_arch = target_arch;
    jit->opt_level = opt_level;
    jit->flags = flags;
    
    // Allocate code buffer
    jit->code_capacity = 64 * 1024; // 64KB
    jit->code_buffer = alloc_exec_mem(jit->code_capacity);
    if (!jit->code_buffer) {
        mem_free(jit);
        return NULL;
    }
    
    printf("JIT: Initialized compiler for %s\n", get_arch_name(target_arch));
    return jit;
}

/**
 * Cleanup JIT compiler
 */
static void jit_cleanup_compiler(JITCompiler* jit) {
    if (!jit) return;
    
    if (jit->code_buffer) {
        free_exec_mem(jit->code_buffer, jit->code_capacity);
    }
    
    if (jit->label_table) {
        mem_free(jit->label_table);
    }
    
    mem_free(jit);
}

/**
 * Simplified JIT compilation
 */
static JITResult jit_compile_bytecode(JITCompiler* jit, const uint8_t* bytecode, size_t bytecode_size, uint32_t entry_point) {
    if (!jit || !bytecode) {
        return JIT_ERROR_INVALID_INPUT;
    }
    
    printf("JIT: Compiling %zu bytes of bytecode\n", bytecode_size);
    
    // Simplified compilation - just create a return instruction
    // In a real implementation, this would parse ASTC bytecode and generate native code
    if (jit->target_arch == ARCH_X86_64) {
        // x64: mov eax, 42; ret
        jit->code_buffer[0] = 0xB8; // mov eax, imm32
        *(uint32_t*)(jit->code_buffer + 1) = 42;
        jit->code_buffer[5] = 0xC3; // ret
        jit->code_size = 6;
    } else {
        return JIT_ERROR_UNSUPPORTED_ARCH;
    }
    
    printf("JIT: Generated %zu bytes of native code\n", jit->code_size);
    return JIT_SUCCESS;
}

/**
 * Get compiled entry point
 */
static void* jit_get_entry_point(JITCompiler* jit) {
    if (!jit || jit->code_size == 0) {
        return NULL;
    }
    return jit->code_buffer;
}

/**
 * Get compiled code size
 */
static size_t jit_get_code_size(JITCompiler* jit) {
    return jit ? jit->code_size : 0;
}

/**
 * Execute compiled code
 */
static int jit_execute(JITCompiler* jit, void** args, int arg_count, void* result) {
    if (!jit || !jit->code_buffer) {
        return -1;
    }
    
    typedef int (*jit_func_t)(void);
    jit_func_t func = (jit_func_t)jit->code_buffer;
    
    int exec_result = func();
    if (result) {
        *(int*)result = exec_result;
    }
    
    return 0;
}

/**
 * Get error message
 */
static const char* jit_get_error_message(JITCompiler* jit) {
    return jit && jit->error_message[0] ? jit->error_message : "No error";
}

/**
 * Get JIT version
 */
static const char* jit_get_version(void) {
    return "JIT Module v1.0";
}

/**
 * Print JIT information
 */
static void jit_print_info(void) {
    JITAvailability avail = jit_check_availability();
    printf("JIT Module Information:\n");
    printf("  Version: %s\n", jit_get_version());
    printf("  Availability: %s\n", 
           avail == JIT_AVAILABLE ? "Available" : 
           avail == JIT_UNAVAILABLE ? "Unavailable" :
           avail == JIT_DISABLED ? "Disabled" : "Error");
    printf("  Supported Architectures: x86_32, x86_64\n");
    printf("  Current Architecture: %s\n", get_arch_name(detect_arch()));
}

// ===============================================
// ASTC JIT Compilation Functions
// ===============================================

/**
 * Create JIT context
 */
static JITContext* jit_create_context(uint32_t target_arch, uint32_t target_bits) {
    if (jit_check_availability() != JIT_AVAILABLE) {
        return NULL;
    }
    
    JITContext* ctx = mem_alloc(sizeof(JITContext), MEMORY_POOL_JIT);
    if (!ctx) {
        return NULL;
    }
    
    memset(ctx, 0, sizeof(JITContext));
    ctx->target_arch = target_arch;
    ctx->target_bits = target_bits;
    ctx->initialized = true;
    
    return ctx;
}

/**
 * Destroy JIT context
 */
static void jit_destroy_context(JITContext* ctx) {
    if (!ctx) return;
    
    if (ctx->compiler_state) {
        mem_free(ctx->compiler_state);
    }
    
    mem_free(ctx);
}

/**
 * Compile ASTC bytecode to machine code
 */
static int jit_compile_astc(JITContext* ctx, const uint8_t* bytecode, size_t bytecode_size,
                     uint8_t** machine_code, size_t* machine_code_size) {
    if (!ctx || !bytecode || !machine_code || !machine_code_size) {
        return JIT_ERROR_INVALID_INPUT;
    }
    
    if (!ctx->initialized) {
        return JIT_ERROR_NOT_AVAILABLE;
    }
    
    // Create a JIT compiler if not already created
    if (!ctx->compiler_state) {
        JITCompiler* compiler = jit_init_compiler(ctx->target_arch, JIT_OPT_BASIC, JIT_FLAG_NONE);
        if (!compiler) {
            return JIT_ERROR_NOT_AVAILABLE;
        }
        ctx->compiler_state = compiler;
    }
    
    JITCompiler* compiler = (JITCompiler*)ctx->compiler_state;
    
    // Compile bytecode
    JITResult result = jit_compile_bytecode(compiler, bytecode, bytecode_size, 0);
    if (result != JIT_SUCCESS) {
        return result;
    }
    
    // Get compiled code
    size_t code_size = jit_get_code_size(compiler);
    if (code_size == 0) {
        return JIT_ERROR_COMPILATION_FAILED;
    }
    
    // Allocate memory for the output
    uint8_t* code = alloc_exec_mem(code_size);
    if (!code) {
        return JIT_ERROR_MEMORY_ALLOCATION;
    }
    
    // Copy compiled code
    memcpy(code, compiler->code_buffer, code_size);
    
    // Return results
    *machine_code = code;
    *machine_code_size = code_size;
    
    return JIT_SUCCESS;
}

/**
 * Free compiled machine code
 */
static void jit_free_code(uint8_t* machine_code) {
    if (machine_code) {
        // Note: We don't know the size here, so we can't use free_exec_mem
        // In a real implementation, we would track the size or use a different approach
        free_exec_mem(machine_code, 0);
    }
}

// ===============================================
// C99 JIT Compilation Functions
// ===============================================

/**
 * Create C99 JIT context
 */
static C99JITContext* c99_jit_create_context(const char* target_arch, int opt_level) {
    if (jit_check_availability() != JIT_AVAILABLE) {
        return NULL;
    }
    
    C99JITContext* ctx = mem_alloc(sizeof(C99JITContext), MEMORY_POOL_JIT);
    if (!ctx) {
        return NULL;
    }
    
    memset(ctx, 0, sizeof(C99JITContext));
    
    // Set target architecture
    DetectedArchitecture arch = ARCH_UNKNOWN;
    if (strcmp(target_arch, "x86_64") == 0) {
        arch = ARCH_X86_64;
    } else if (strcmp(target_arch, "x86_32") == 0) {
        arch = ARCH_X86_32;
    } else {
        mem_free(ctx);
        return NULL;
    }
    
    // Create base JIT compiler
    ctx->base_jit = jit_init_compiler(arch, opt_level, JIT_FLAG_C99_MODE);
    if (!ctx->base_jit) {
        mem_free(ctx);
        return NULL;
    }
    
    // Set properties
    ctx->target_arch = mem_alloc(strlen(target_arch) + 1, MEMORY_POOL_JIT);
    strcpy(ctx->target_arch, target_arch);
    ctx->optimization_level = opt_level;
    
    return ctx;
}

/**
 * Destroy C99 JIT context
 */
static void c99_jit_destroy_context(C99JITContext* ctx) {
    if (!ctx) return;
    
    if (ctx->base_jit) {
        jit_cleanup_compiler(ctx->base_jit);
    }
    
    if (ctx->target_arch) {
        mem_free(ctx->target_arch);
    }
    
    if (ctx->source_file) {
        mem_free(ctx->source_file);
    }
    
    if (ctx->function_addresses) {
        mem_free(ctx->function_addresses);
    }
    
    if (ctx->function_names) {
        for (uint32_t i = 0; i < ctx->function_count; i++) {
            if (ctx->function_names[i]) {
                mem_free(ctx->function_names[i]);
            }
        }
        mem_free(ctx->function_names);
    }
    
    mem_free(ctx);
}

/**
 * Set C99 source file
 */
static JITResult c99_jit_set_source(C99JITContext* ctx, const char* source_file) {
    if (!ctx || !source_file) {
        return JIT_ERROR_INVALID_INPUT;
    }
    
    if (ctx->source_file) {
        mem_free(ctx->source_file);
    }
    
    ctx->source_file = mem_alloc(strlen(source_file) + 1, MEMORY_POOL_JIT);
    if (!ctx->source_file) {
        return JIT_ERROR_MEMORY_ALLOCATION;
    }
    
    strcpy(ctx->source_file, source_file);
    return JIT_SUCCESS;
}

// ===============================================
// Module Symbols
// ===============================================

static struct {
    const char* name;
    void* symbol;
} jit_symbols[] = {
    {"check_availability", jit_check_availability},
    {"is_arch_supported", jit_is_arch_supported},
    {"init_compiler", jit_init_compiler},
    {"cleanup_compiler", jit_cleanup_compiler},
    {"compile_bytecode", jit_compile_bytecode},
    {"get_entry_point", jit_get_entry_point},
    {"get_code_size", jit_get_code_size},
    {"execute", jit_execute},
    {"get_error_message", jit_get_error_message},
    {"get_version", jit_get_version},
    {"print_info", jit_print_info},
    {"create_context", jit_create_context},
    {"destroy_context", jit_destroy_context},
    {"compile_astc", jit_compile_astc},
    {"free_code", jit_free_code},
    {"c99_create_context", c99_jit_create_context},
    {"c99_destroy_context", c99_jit_destroy_context},
    {"c99_set_source", c99_jit_set_source},
    {NULL, NULL}
};

// ===============================================
// Module Interface
// ===============================================

/**
 * Resolve a symbol from this module
 */
static void* jit_resolve(const char* symbol) {
    for (int i = 0; jit_symbols[i].name != NULL; i++) {
        if (strcmp(jit_symbols[i].name, symbol) == 0) {
            return jit_symbols[i].symbol;
        }
    }
    return NULL;
}

/**
 * Initialize the module
 */
static int jit_init(void) {
    // Resolve dependencies
    Module* memory_module = module_load("memory");
    if (!memory_module) {
        return -1;
    }
    
    Module* utils_module = module_load("utils");
    if (!utils_module) {
        return -1;
    }
    
    // Resolve memory functions
    mem_alloc = module_resolve(memory_module, "alloc");
    mem_free = module_resolve(memory_module, "free");
    mem_calloc = module_resolve(memory_module, "calloc");
    alloc_exec_mem = module_resolve(memory_module, "allocate_executable_memory");
    free_exec_mem = module_resolve(memory_module, "free_executable_memory");
    
    // Resolve utils functions
    detect_arch = module_resolve(utils_module, "detect_architecture");
    get_arch_name = module_resolve(utils_module, "get_architecture_name");
    
    if (!mem_alloc || !mem_free || !mem_calloc || !alloc_exec_mem || !free_exec_mem ||
        !detect_arch || !get_arch_name) {
        return -1;
    }
    
    jit_initialized = true;
    return 0;
}

/**
 * Clean up the module
 */
static void jit_cleanup(void) {
    jit_initialized = false;
}

// Module definition - updated to match new module.h structure
Module module_jit = {
    .name = MODULE_NAME,
    .state = MODULE_UNLOADED,
    .error = NULL,
    .init = jit_init,
    .cleanup = jit_cleanup,
    .resolve = jit_resolve
};

// Register the module
// REGISTER_MODULE(jit); 