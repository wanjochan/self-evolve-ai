/**
 * memory_module.c - Memory Management Module
 * 
 * Provides memory management functionality as a module.
 * Implements the functionality defined in memory.h.
 */

#include "../module.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

// Module name
static const char* MODULE_NAME = "memory";

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
// C99 Compiler Memory Management
// ===============================================

typedef struct C99MemoryContext {
    size_t ast_nodes_allocated;
    size_t symbols_allocated;
    size_t strings_allocated;
    size_t total_c99_memory;
    bool leak_detection_enabled;
} C99MemoryContext;

// ===============================================
// Module State
// ===============================================

// 全局内存统计
static MemoryStats g_memory_stats = {0};

// 内存池状态
static bool g_initialized = false;

// 调试级别
static int g_debug_level = 0;

// ===============================================
// Memory Management Functions
// ===============================================

/**
 * Initialize memory management system
 */
static int memory_init(void) {
    if (g_initialized) {
        return 0;
    }
    
    memset(&g_memory_stats, 0, sizeof(MemoryStats));
    g_initialized = true;
    
    if (g_debug_level > 0) {
        printf("Memory: Initialized memory management system\n");
    }
    
    return 0;
}

/**
 * Cleanup memory management system
 */
static void memory_cleanup(void) {
    if (!g_initialized) {
        return;
    }
    
    if (g_debug_level > 0) {
        memory_print_report();
    }
    
    g_initialized = false;
    
    if (g_debug_level > 0) {
        printf("Memory: Cleaned up memory management system\n");
    }
}

/**
 * Basic memory allocation function
 */
static void* memory_alloc_basic(size_t size) {
    return malloc(size);
}

/**
 * Allocate memory from specific pool
 */
static void* memory_alloc(size_t size, MemoryPoolType pool) {
    if (!g_initialized || pool >= MEMORY_POOL_COUNT) {
        return malloc(size); // Fallback to standard malloc if not initialized
    }
    
    void* ptr = malloc(size);
    if (ptr) {
        g_memory_stats.total_allocated += size;
        g_memory_stats.current_usage += size;
        g_memory_stats.allocation_count++;
        g_memory_stats.pool_usage[pool] += size;
        
        if (g_memory_stats.current_usage > g_memory_stats.peak_usage) {
            g_memory_stats.peak_usage = g_memory_stats.current_usage;
        }
        
        if (g_debug_level > 1) {
            printf("Memory: Allocated %zu bytes in pool %d at %p\n", size, pool, ptr);
        }
    }
    
    return ptr;
}

/**
 * Basic memory reallocation function
 */
static void* memory_realloc_basic(void* ptr, size_t size) {
    return realloc(ptr, size);
}

/**
 * Reallocate memory
 */
static void* memory_realloc(void* ptr, size_t new_size, MemoryPoolType pool) {
    if (!g_initialized) {
        return realloc(ptr, new_size); // Fallback to standard realloc if not initialized
    }
    
    void* new_ptr = realloc(ptr, new_size);
    if (new_ptr) {
        g_memory_stats.total_allocated += new_size;
        g_memory_stats.current_usage += new_size;
        g_memory_stats.pool_usage[pool] += new_size;
        
        if (g_memory_stats.current_usage > g_memory_stats.peak_usage) {
            g_memory_stats.peak_usage = g_memory_stats.current_usage;
        }
        
        if (g_debug_level > 1) {
            printf("Memory: Reallocated from %p to %p, new size %zu bytes in pool %d\n", 
                   ptr, new_ptr, new_size, pool);
        }
    }
    
    return new_ptr;
}

/**
 * Basic memory free function
 */
static void memory_free_basic(void* ptr) {
    free(ptr);
}

/**
 * Free memory
 */
static void memory_free(void* ptr) {
    if (!g_initialized || !ptr) {
        free(ptr); // Fallback to standard free if not initialized
        return;
    }
    
    if (g_debug_level > 1) {
        printf("Memory: Freed memory at %p\n", ptr);
    }
    
    free(ptr);
    g_memory_stats.free_count++;
}

/**
 * Memory copy function
 */
static void* memory_copy(void* dest, const void* src, size_t size) {
    return memcpy(dest, src, size);
}

/**
 * Memory set function
 */
static void* memory_set(void* dest, int value, size_t size) {
    return memset(dest, value, size);
}

/**
 * Allocate zero-initialized memory
 */
static void* memory_calloc(size_t count, size_t size, MemoryPoolType pool) {
    size_t total_size = count * size;
    void* ptr = memory_alloc(total_size, pool);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

/**
 * Duplicate string with memory tracking
 */
static char* memory_strdup(const char* str, MemoryPoolType pool) {
    if (!str) {
        return NULL;
    }
    
    size_t len = strlen(str) + 1;
    char* dup = (char*)memory_alloc(len, pool);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

/**
 * Get memory statistics
 */
static void memory_get_stats(MemoryStats* stats) {
    if (!stats) {
        return;
    }
    
    if (g_initialized) {
        *stats = g_memory_stats;
    } else {
        memset(stats, 0, sizeof(MemoryStats));
    }
}

/**
 * Print memory usage report
 */
static void memory_print_report(void) {
    if (!g_initialized) {
        printf("Memory: System not initialized\n");
        return;
    }
    
    printf("Memory Usage Report:\n");
    printf("  Total Allocated: %zu bytes\n", g_memory_stats.total_allocated);
    printf("  Total Freed: %zu bytes\n", g_memory_stats.total_freed);
    printf("  Current Usage: %zu bytes\n", g_memory_stats.current_usage);
    printf("  Peak Usage: %zu bytes\n", g_memory_stats.peak_usage);
    printf("  Allocation Count: %zu\n", g_memory_stats.allocation_count);
    printf("  Free Count: %zu\n", g_memory_stats.free_count);
    
    printf("  Pool Usage:\n");
    for (int i = 0; i < MEMORY_POOL_COUNT; i++) {
        printf("    Pool %d: %zu bytes\n", i, g_memory_stats.pool_usage[i]);
    }
}

/**
 * Check for memory leaks
 */
static bool memory_check_leaks(void) {
    if (!g_initialized) {
        return false;
    }
    
    bool has_leaks = g_memory_stats.allocation_count > g_memory_stats.free_count;
    
    if (has_leaks && g_debug_level > 0) {
        printf("Memory: Detected %zu possible leaks\n", 
               g_memory_stats.allocation_count - g_memory_stats.free_count);
    }
    
    return has_leaks;
}

/**
 * Set memory debugging level
 */
static void memory_set_debug_level(int level) {
    g_debug_level = level;
    
    if (g_initialized && g_debug_level > 0) {
        printf("Memory: Debug level set to %d\n", g_debug_level);
    }
}

// ===============================================
// C99 Compiler Memory Management
// ===============================================

/**
 * Create C99 compiler memory context
 */
static C99MemoryContext* c99_memory_create_context(void) {
    C99MemoryContext* ctx = (C99MemoryContext*)memory_alloc(
        sizeof(C99MemoryContext), MEMORY_POOL_GENERAL);
    
    if (ctx) {
        memset(ctx, 0, sizeof(C99MemoryContext));
        ctx->leak_detection_enabled = true;
        
        if (g_debug_level > 0) {
            printf("Memory: Created C99 memory context at %p\n", ctx);
        }
    }
    
    return ctx;
}

/**
 * Destroy C99 compiler memory context and check for leaks
 */
static void c99_memory_destroy_context(C99MemoryContext* ctx) {
    if (!ctx) {
        return;
    }
    
    if (ctx->leak_detection_enabled && g_debug_level > 0) {
        printf("Memory: C99 context statistics on destruction:\n");
        printf("  AST nodes: %zu\n", ctx->ast_nodes_allocated);
        printf("  Symbols: %zu\n", ctx->symbols_allocated);
        printf("  Strings: %zu\n", ctx->strings_allocated);
        printf("  Total: %zu\n", ctx->total_c99_memory);
    }
    
    memory_free(ctx);
    
    if (g_debug_level > 0) {
        printf("Memory: Destroyed C99 memory context\n");
    }
}

/**
 * Allocate memory for C99 AST node
 */
static void* c99_memory_alloc_ast_node(C99MemoryContext* ctx, size_t size) {
    if (!ctx) {
        return memory_alloc(size, MEMORY_POOL_C99_AST);
    }
    
    void* ptr = memory_alloc(size, MEMORY_POOL_C99_AST);
    if (ptr) {
        ctx->ast_nodes_allocated++;
        ctx->total_c99_memory += size;
    }
    
    return ptr;
}

/**
 * Allocate memory for C99 symbol table entry
 */
static void* c99_memory_alloc_symbol(C99MemoryContext* ctx, size_t size) {
    if (!ctx) {
        return memory_alloc(size, MEMORY_POOL_C99_SYMBOLS);
    }
    
    void* ptr = memory_alloc(size, MEMORY_POOL_C99_SYMBOLS);
    if (ptr) {
        ctx->symbols_allocated++;
        ctx->total_c99_memory += size;
    }
    
    return ptr;
}

/**
 * Allocate memory for C99 string literal
 */
static char* c99_memory_alloc_string(C99MemoryContext* ctx, const char* str) {
    if (!ctx || !str) {
        return memory_strdup(str, MEMORY_POOL_C99_STRINGS);
    }
    
    size_t len = strlen(str) + 1;
    char* dup = (char*)memory_alloc(len, MEMORY_POOL_C99_STRINGS);
    
    if (dup) {
        strcpy(dup, str);
        ctx->strings_allocated++;
        ctx->total_c99_memory += len;
    }
    
    return dup;
}

/**
 * Free C99 compiler memory with context tracking
 */
static void c99_memory_free(C99MemoryContext* ctx, void* ptr) {
    // Just use regular memory_free since we don't track individual allocations
    memory_free(ptr);
}

/**
 * Get C99 compiler memory statistics
 */
static void c99_memory_get_stats(C99MemoryContext* ctx, MemoryStats* stats) {
    if (!ctx || !stats) {
        return;
    }
    
    // Fill in general memory stats
    memory_get_stats(stats);
    
    // Add C99-specific stats to the output structure
    // (we don't modify the stats structure directly, just provide info)
}

/**
 * Print C99 compiler memory report
 */
static void c99_memory_print_report(C99MemoryContext* ctx) {
    if (!ctx) {
        printf("Memory: C99 context is NULL\n");
        return;
    }
    
    printf("C99 Memory Report:\n");
    printf("  AST Nodes: %zu\n", ctx->ast_nodes_allocated);
    printf("  Symbols: %zu\n", ctx->symbols_allocated);
    printf("  Strings: %zu\n", ctx->strings_allocated);
    printf("  Total Memory: %zu bytes\n", ctx->total_c99_memory);
}

// ===============================================
// Executable Memory Allocation
// ===============================================

/**
 * Allocate executable memory (cross-platform)
 */
static void* allocate_executable_memory(size_t size) {
#ifdef _WIN32
    return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#else
    #include <sys/mman.h>
    void* ptr = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return (ptr == MAP_FAILED) ? NULL : ptr;
#endif
}

/**
 * Free executable memory
 */
static void free_executable_memory(void* ptr, size_t size) {
#ifdef _WIN32
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    #include <sys/mman.h>
    munmap(ptr, size);
#endif
}

// ===============================================
// Symbol Table
// ===============================================

// Symbol table
static struct {
    const char* name;
    void* symbol;
} memory_symbols[] = {
    // Basic memory functions
    {"alloc", memory_alloc_basic},
    {"realloc", memory_realloc_basic},
    {"free", memory_free_basic},
    {"copy", memory_copy},
    {"set", memory_set},
    
    // Enhanced memory management
    {"init", memory_init},
    {"cleanup", memory_cleanup},
    {"alloc_pool", memory_alloc},
    {"realloc_pool", memory_realloc},
    {"calloc", memory_calloc},
    {"strdup", memory_strdup},
    {"get_stats", memory_get_stats},
    {"print_report", memory_print_report},
    {"check_leaks", memory_check_leaks},
    {"set_debug_level", memory_set_debug_level},
    
    // C99 memory management
    {"c99_create_context", c99_memory_create_context},
    {"c99_destroy_context", c99_memory_destroy_context},
    {"c99_alloc_ast_node", c99_memory_alloc_ast_node},
    {"c99_alloc_symbol", c99_memory_alloc_symbol},
    {"c99_alloc_string", c99_memory_alloc_string},
    {"c99_free", c99_memory_free},
    {"c99_get_stats", c99_memory_get_stats},
    {"c99_print_report", c99_memory_print_report},
    
    // Executable memory
    {"allocate_executable", allocate_executable_memory},
    {"free_executable", free_executable_memory},
    
    {NULL, NULL}  // Sentinel
};

// ===============================================
// Module Interface
// ===============================================

// Module load function
static int memory_load_module(void) {
    // Initialize memory system
    return memory_init();
}

// Module unload function
static void memory_unload_module(void) {
    // Cleanup memory system
    memory_cleanup();
}

// Symbol resolution function
static void* memory_resolve_symbol(const char* symbol) {
    if (!symbol) {
        return NULL;
    }
    
    for (int i = 0; memory_symbols[i].name; i++) {
        if (strcmp(memory_symbols[i].name, symbol) == 0) {
            return memory_symbols[i].symbol;
        }
    }
    
    return NULL;
}

// Module definition
static Module module_memory = {
    .name = MODULE_NAME,
    .handle = NULL,
    .state = MODULE_UNLOADED,
    .error = NULL,
    .load = memory_load_module,
    .unload = memory_unload_module,
    .resolve = memory_resolve_symbol,
    .on_init = NULL,
    .on_exit = NULL,
    .on_error = NULL
};

// Register module
REGISTER_MODULE(memory); 