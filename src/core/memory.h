/**
 * memory.h - Core Memory Management System
 * 
 * Unified memory allocation and management for the ASTC system.
 * Provides debugging, tracking, and optimization features.
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// Memory Pool Types
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
// Memory Statistics
// ===============================================

typedef struct {
    size_t total_allocated;
    size_t total_freed;
    size_t current_usage;
    size_t peak_usage;
    size_t allocation_count;
    size_t free_count;
    size_t pool_usage[MEMORY_POOL_COUNT];
} MemoryStats;

// ===============================================
// Memory Management Functions
// ===============================================

/**
 * Initialize memory management system
 */
int memory_init(void);

/**
 * Cleanup memory management system
 */
void memory_cleanup(void);

/**
 * Allocate memory from specific pool
 */
void* memory_alloc(size_t size, MemoryPoolType pool);

/**
 * Reallocate memory
 */
void* memory_realloc(void* ptr, size_t new_size, MemoryPoolType pool);

/**
 * Free memory
 */
void memory_free(void* ptr);

/**
 * Allocate zero-initialized memory
 */
void* memory_calloc(size_t count, size_t size, MemoryPoolType pool);

/**
 * Duplicate string with memory tracking
 */
char* memory_strdup(const char* str, MemoryPoolType pool);

/**
 * Get memory statistics
 */
void memory_get_stats(MemoryStats* stats);

/**
 * Print memory usage report
 */
void memory_print_report(void);

/**
 * Check for memory leaks
 */
bool memory_check_leaks(void);

/**
 * Set memory debugging level
 */
void memory_set_debug_level(int level);

// ===============================================
// C99 Compiler Memory Management
// ===============================================

/**
 * C99 compiler memory context for tracking compilation memory usage
 */
typedef struct C99MemoryContext {
    size_t ast_nodes_allocated;
    size_t symbols_allocated;
    size_t strings_allocated;
    size_t total_c99_memory;
    bool leak_detection_enabled;
} C99MemoryContext;

/**
 * Create C99 compiler memory context
 */
C99MemoryContext* c99_memory_create_context(void);

/**
 * Destroy C99 compiler memory context and check for leaks
 */
void c99_memory_destroy_context(C99MemoryContext* ctx);

/**
 * Allocate memory for C99 AST node
 */
void* c99_memory_alloc_ast_node(C99MemoryContext* ctx, size_t size);

/**
 * Allocate memory for C99 symbol table entry
 */
void* c99_memory_alloc_symbol(C99MemoryContext* ctx, size_t size);

/**
 * Allocate memory for C99 string literal
 */
char* c99_memory_alloc_string(C99MemoryContext* ctx, const char* str);

/**
 * Free C99 compiler memory with context tracking
 */
void c99_memory_free(C99MemoryContext* ctx, void* ptr);

/**
 * Get C99 compiler memory statistics
 */
void c99_memory_get_stats(C99MemoryContext* ctx, MemoryStats* stats);

/**
 * Print C99 compiler memory report
 */
void c99_memory_print_report(C99MemoryContext* ctx);

// ===============================================
// Convenience Macros
// ===============================================

#define ALLOC(size) memory_alloc(size, MEMORY_POOL_GENERAL)
#define CALLOC(count, size) memory_calloc(count, size, MEMORY_POOL_GENERAL)
#define REALLOC(ptr, size) memory_realloc(ptr, size, MEMORY_POOL_GENERAL)
#define FREE(ptr) memory_free(ptr)
#define STRDUP(str) memory_strdup(str, MEMORY_POOL_GENERAL)

#define ALLOC_BYTECODE(size) memory_alloc(size, MEMORY_POOL_BYTECODE)
#define ALLOC_JIT(size) memory_alloc(size, MEMORY_POOL_JIT)
#define ALLOC_MODULE(size) memory_alloc(size, MEMORY_POOL_MODULES)
#define ALLOC_TEMP(size) memory_alloc(size, MEMORY_POOL_TEMP)

// C99 Compiler Memory Macros
#define ALLOC_C99_AST(size) memory_alloc(size, MEMORY_POOL_C99_AST)
#define ALLOC_C99_SYMBOL(size) memory_alloc(size, MEMORY_POOL_C99_SYMBOLS)
#define ALLOC_C99_STRING(size) memory_alloc(size, MEMORY_POOL_C99_STRINGS)

// C99 Context-aware macros (when context is available)
#define C99_ALLOC_AST(ctx, size) c99_memory_alloc_ast_node(ctx, size)
#define C99_ALLOC_SYMBOL(ctx, size) c99_memory_alloc_symbol(ctx, size)
#define C99_ALLOC_STRING(ctx, str) c99_memory_alloc_string(ctx, str)
#define C99_FREE(ctx, ptr) c99_memory_free(ctx, ptr)

#ifdef __cplusplus
}
#endif

#endif // MEMORY_H
