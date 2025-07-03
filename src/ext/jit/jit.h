/**
 * jit.h - Extended JIT Compiler Interface (Optional Performance Enhancement)
 * 
 * Provides optional JIT compilation capabilities for the self-evolve-ai system.
 * This is an EXTENSION module, not a core requirement.
 * 
 * The system can function without JIT by using:
 * - ASTC bytecode interpretation
 * - Direct native module compilation
 * - External compiler integration (TCC fallback)
 * 
 * JIT provides performance benefits but is not essential for basic operation.
 */

#ifndef EXT_JIT_H
#define EXT_JIT_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../../core/astc.h"
#include "../../core/utils.h"

#ifdef __cplusplus
extern "C" {
#endif

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
    JIT_FLAG_VERIFY_CODE = 8        // Verify generated code
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
 * JIT Extension Interface (Optional)
 */
typedef struct {
    // Availability checking
    JITAvailability (*check_availability)(void);
    bool (*is_supported)(DetectedArchitecture arch);
    
    // Lifecycle management
    JITCompiler* (*init)(DetectedArchitecture target_arch, JITOptLevel opt_level, uint32_t flags);
    void (*cleanup)(JITCompiler* jit);
    
    // Compilation functions
    JITResult (*compile_bytecode)(JITCompiler* jit, const uint8_t* bytecode, size_t bytecode_size, uint32_t entry_point);
    void* (*get_entry_point)(JITCompiler* jit);
    size_t (*get_code_size)(JITCompiler* jit);
    
    // Execution
    int (*execute)(JITCompiler* jit, void** args, int arg_count, void* result);
    
    // Cache management (optional)
    int (*cache_init)(size_t max_cache_size);
    void (*cache_cleanup)(void);
    bool (*cache_lookup)(uint64_t bytecode_hash, void** entry_point, size_t* code_size);
    int (*cache_store)(uint64_t bytecode_hash, void* entry_point, size_t code_size);
    
    // Utility functions
    uint64_t (*hash_bytecode)(const uint8_t* bytecode, size_t size);
    const char* (*get_error_message)(JITCompiler* jit);
    const char* (*get_version)(void);
} JITExtensionInterface;

// ===============================================
// JIT Extension API
// ===============================================

/**
 * Check if JIT extension is available
 * @return JIT availability status
 */
JITAvailability jit_ext_check_availability(void);

/**
 * Get JIT extension interface (if available)
 * @return JIT interface or NULL if not available
 */
JITExtensionInterface* jit_ext_get_interface(void);

/**
 * Initialize JIT extension system
 * @return 0 on success, -1 if JIT not available
 */
int jit_ext_init(void);

/**
 * Cleanup JIT extension system
 */
void jit_ext_cleanup(void);

/**
 * Check if JIT is supported for architecture
 * @param arch Architecture to check
 * @return true if supported, false otherwise
 */
bool jit_ext_is_arch_supported(DetectedArchitecture arch);

/**
 * Get JIT extension version
 * @return Version string or NULL if not available
 */
const char* jit_ext_get_version(void);

/**
 * Print JIT extension information
 */
void jit_ext_print_info(void);

// ===============================================
// Fallback Macros (when JIT not available)
// ===============================================

#ifndef JIT_EXTENSION_AVAILABLE
#define JIT_EXTENSION_AVAILABLE 0
#endif

#if !JIT_EXTENSION_AVAILABLE
// Provide no-op implementations when JIT is not available
#define jit_ext_check_availability() JIT_UNAVAILABLE
#define jit_ext_get_interface() NULL
#define jit_ext_init() -1
#define jit_ext_cleanup() do {} while(0)
#define jit_ext_is_arch_supported(arch) false
#define jit_ext_get_version() "JIT Extension Not Available"
#define jit_ext_print_info() printf("JIT Extension: Not Available\n")
#endif

#ifdef __cplusplus
}
#endif

#endif // EXT_JIT_H
