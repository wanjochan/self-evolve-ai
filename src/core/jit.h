/**
 * jit.h - Core JIT Compiler Interface
 * 
 * Provides a unified, cross-architecture JIT compilation interface for the
 * self-evolve-ai system. This is the core JIT module that integrates with
 * the VM module to provide real-time compilation capabilities.
 * 
 * Based on analysis of src/ext/compiler/codegen*.c files, this interface
 * abstracts the architecture-specific code generation into a clean API.
 */

#ifndef JIT_H
#define JIT_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "astc.h"
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// JIT Core Types and Enums
// ===============================================

/**
 * JIT compilation status codes
 */
typedef enum {
    JIT_SUCCESS = 0,
    JIT_ERROR_INVALID_INPUT = -1,
    JIT_ERROR_MEMORY_ALLOCATION = -2,
    JIT_ERROR_UNSUPPORTED_ARCH = -3,
    JIT_ERROR_COMPILATION_FAILED = -4,
    JIT_ERROR_BUFFER_OVERFLOW = -5
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
    JIT_FLAG_VERIFY_CODE = 8        // Verify generated code
} JITFlags;

// ===============================================
// JIT Core Structures
// ===============================================

/**
 * JIT Compiler Context
 */
typedef struct JITCompiler {
    DetectedArchitecture target_arch;       // Target architecture
    JITOptLevel opt_level;         // Optimization level
    uint32_t flags;                // Compilation flags
    
    // Code generation buffers
    uint8_t* code_buffer;          // Generated machine code
    size_t code_size;              // Current code size
    size_t code_capacity;          // Buffer capacity
    
    // Compilation state
    uint32_t* label_table;         // Jump label addresses
    size_t label_count;            // Number of labels
    size_t label_capacity;         // Label table capacity
    
    // Error handling
    JITResult last_error;          // Last error code
    char error_message[512];       // Error message
    
    // Statistics
    size_t bytes_compiled;         // Total bytes compiled
    size_t functions_compiled;     // Number of functions compiled
    uint64_t compilation_time_us;  // Total compilation time in microseconds
} JITCompiler;

/**
 * Compiled function information
 */
typedef struct JITFunction {
    void* entry_point;             // Function entry point
    size_t code_size;              // Function code size
    char name[64];                 // Function name
    uint32_t signature;            // Function signature hash
    bool is_optimized;             // Whether function is optimized
} JITFunction;

/**
 * JIT compilation statistics
 */
typedef struct JITStats {
    size_t total_compilations;     // Total number of compilations
    size_t cache_hits;             // Cache hits
    size_t cache_misses;           // Cache misses
    uint64_t total_compile_time;   // Total compilation time
    uint64_t average_compile_time; // Average compilation time
    size_t total_code_size;        // Total generated code size
} JITStats;

// ===============================================
// JIT Core Interface Functions
// ===============================================

/**
 * Initialize JIT compiler
 * @param target_arch Target architecture (ARCH_AUTO for auto-detection)
 * @param opt_level Optimization level
 * @param flags Compilation flags
 * @return Initialized JIT compiler or NULL on error
 */
JITCompiler* jit_init(DetectedArchitecture target_arch, JITOptLevel opt_level, uint32_t flags);

/**
 * Cleanup JIT compiler and free resources
 * @param jit JIT compiler instance
 */
void jit_cleanup(JITCompiler* jit);

/**
 * Compile ASTC bytecode to native machine code
 * @param jit JIT compiler instance
 * @param bytecode ASTC bytecode
 * @param bytecode_size Size of bytecode
 * @param entry_point Entry point offset in bytecode
 * @return JIT compilation result
 */
JITResult jit_compile_bytecode(JITCompiler* jit, const uint8_t* bytecode, 
                              size_t bytecode_size, uint32_t entry_point);

/**
 * Compile single ASTC function to native code
 * @param jit JIT compiler instance
 * @param function_bytecode Function bytecode
 * @param bytecode_size Size of function bytecode
 * @param function_info Output function information
 * @return JIT compilation result
 */
JITResult jit_compile_function(JITCompiler* jit, const uint8_t* function_bytecode,
                              size_t bytecode_size, JITFunction* function_info);

/**
 * Get compiled code entry point
 * @param jit JIT compiler instance
 * @return Entry point address or NULL if not compiled
 */
void* jit_get_entry_point(JITCompiler* jit);

/**
 * Get compiled code size
 * @param jit JIT compiler instance
 * @return Size of compiled code
 */
size_t jit_get_code_size(JITCompiler* jit);

/**
 * Execute compiled code
 * @param jit JIT compiler instance
 * @param args Function arguments
 * @param arg_count Number of arguments
 * @param result Pointer to store result
 * @return Execution result (0 = success)
 */
int jit_execute(JITCompiler* jit, void** args, int arg_count, void* result);

/**
 * Get JIT compilation statistics
 * @param jit JIT compiler instance
 * @param stats Output statistics structure
 */
void jit_get_stats(JITCompiler* jit, JITStats* stats);

/**
 * Reset JIT compilation statistics
 * @param jit JIT compiler instance
 */
void jit_reset_stats(JITCompiler* jit);

/**
 * Get last error message
 * @param jit JIT compiler instance
 * @return Error message string or NULL if no error
 */
const char* jit_get_error_message(JITCompiler* jit);

/**
 * Set JIT optimization level
 * @param jit JIT compiler instance
 * @param opt_level New optimization level
 */
void jit_set_optimization_level(JITCompiler* jit, JITOptLevel opt_level);

/**
 * Enable/disable JIT compilation flags
 * @param jit JIT compiler instance
 * @param flags Flags to set
 * @param enable Whether to enable or disable flags
 */
void jit_set_flags(JITCompiler* jit, uint32_t flags, bool enable);

// ===============================================
// JIT Cache Management
// ===============================================

/**
 * Initialize JIT compilation cache
 * @param max_cache_size Maximum cache size in bytes
 * @return 0 on success, -1 on error
 */
int jit_cache_init(size_t max_cache_size);

/**
 * Cleanup JIT compilation cache
 */
void jit_cache_cleanup(void);

/**
 * Look up compiled code in cache
 * @param bytecode_hash Hash of the bytecode
 * @param entry_point Cached entry point (output)
 * @param code_size Cached code size (output)
 * @return true if found in cache, false otherwise
 */
bool jit_cache_lookup(uint64_t bytecode_hash, void** entry_point, size_t* code_size);

/**
 * Store compiled code in cache
 * @param bytecode_hash Hash of the bytecode
 * @param entry_point Compiled entry point
 * @param code_size Compiled code size
 * @return 0 on success, -1 on error
 */
int jit_cache_store(uint64_t bytecode_hash, void* entry_point, size_t code_size);

/**
 * Clear JIT compilation cache
 */
void jit_cache_clear(void);

// ===============================================
// JIT Utility Functions
// ===============================================

/**
 * Calculate hash of bytecode for caching
 * @param bytecode Bytecode to hash
 * @param size Size of bytecode
 * @return Hash value
 */
uint64_t jit_hash_bytecode(const uint8_t* bytecode, size_t size);

/**
 * Check if architecture is supported by JIT
 * @param arch Architecture to check
 * @return true if supported, false otherwise
 */
bool jit_is_architecture_supported(DetectedArchitecture arch);

/**
 * Get JIT compiler version string
 * @return Version string
 */
const char* jit_get_version(void);

/**
 * Print JIT compiler information
 */
void jit_print_info(void);

#ifdef __cplusplus
}
#endif

#endif // JIT_H
