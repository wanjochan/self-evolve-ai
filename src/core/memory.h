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

#ifdef __cplusplus
}
#endif

#endif // MEMORY_H
