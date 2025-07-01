/**
 * libc_native_module.c - Enhanced LibC Native Module
 * 
 * Implements a comprehensive libc module that can be loaded as a .native module
 * and provides standard C library functions to ASTC programs.
 */

#include "../include/native_format.h"
#include "../include/logger.h"
#include "../include/astc_native_bridge.h"
#include "core_libc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Module metadata
static const char* MODULE_NAME = "libc_x64_64.native";
static const char* MODULE_VERSION = "1.0.0";
static const char* MODULE_AUTHOR = "Self-Evolve AI";
static const char* MODULE_DESCRIPTION = "Standard C Library Module";

// Module statistics
typedef struct {
    uint64_t function_calls;
    uint64_t malloc_calls;
    uint64_t free_calls;
    uint64_t string_operations;
    uint64_t math_operations;
    uint64_t io_operations;
    size_t total_allocated;
    size_t current_allocated;
} LibcModuleStats;

static LibcModuleStats g_module_stats = {0};

// Memory tracking for debugging
typedef struct MemoryBlock {
    void* ptr;
    size_t size;
    const char* file;
    int line;
    struct MemoryBlock* next;
} MemoryBlock;

static MemoryBlock* g_memory_blocks = NULL;
static size_t g_memory_block_count = 0;

// Initialize the libc module
int libc_module_init(void) {
    memset(&g_module_stats, 0, sizeof(g_module_stats));
    g_memory_blocks = NULL;
    g_memory_block_count = 0;
    
    LOG_MODULE_INFO("LibC native module initialized: %s v%s", MODULE_NAME, MODULE_VERSION);
    return 0;
}

// Cleanup the libc module
void libc_module_cleanup(void) {
    // Free any remaining memory blocks
    MemoryBlock* block = g_memory_blocks;
    while (block) {
        MemoryBlock* next = block->next;
        LOG_MODULE_WARN("Memory leak detected: %p (%zu bytes) allocated at %s:%d", 
                       block->ptr, block->size, block->file, block->line);
        free(block->ptr);
        free(block);
        block = next;
    }
    
    LOG_MODULE_INFO("LibC native module cleaned up");
    LOG_MODULE_INFO("Total function calls: %llu", g_module_stats.function_calls);
    LOG_MODULE_INFO("Total memory allocated: %zu bytes", g_module_stats.total_allocated);
    
    if (g_memory_block_count > 0) {
        LOG_MODULE_WARN("Memory leaks detected: %zu blocks", g_memory_block_count);
    }
}

// Add memory block to tracking list
static void add_memory_block(void* ptr, size_t size, const char* file, int line) {
    MemoryBlock* block = malloc(sizeof(MemoryBlock));
    if (block) {
        block->ptr = ptr;
        block->size = size;
        block->file = file;
        block->line = line;
        block->next = g_memory_blocks;
        g_memory_blocks = block;
        g_memory_block_count++;
        g_module_stats.current_allocated += size;
    }
}

// Remove memory block from tracking list
static void remove_memory_block(void* ptr) {
    MemoryBlock** current = &g_memory_blocks;
    while (*current) {
        if ((*current)->ptr == ptr) {
            MemoryBlock* to_remove = *current;
            *current = (*current)->next;
            g_module_stats.current_allocated -= to_remove->size;
            free(to_remove);
            g_memory_block_count--;
            return;
        }
        current = &(*current)->next;
    }
}

// Enhanced malloc with tracking
void* libc_malloc_tracked(size_t size, const char* file, int line) {
    g_module_stats.function_calls++;
    g_module_stats.malloc_calls++;
    
    void* ptr = malloc(size);
    if (ptr) {
        g_module_stats.total_allocated += size;
        add_memory_block(ptr, size, file, line);
        LOG_MODULE_DEBUG("malloc(%zu) = %p at %s:%d", size, ptr, file, line);
    } else {
        LOG_MODULE_ERROR("malloc(%zu) failed at %s:%d", size, file, line);
    }
    
    return ptr;
}

// Enhanced free with tracking
void libc_free_tracked(void* ptr, const char* file, int line) {
    g_module_stats.function_calls++;
    g_module_stats.free_calls++;
    
    if (ptr) {
        remove_memory_block(ptr);
        free(ptr);
        LOG_MODULE_DEBUG("free(%p) at %s:%d", ptr, file, line);
    } else {
        LOG_MODULE_WARN("free(NULL) called at %s:%d", file, line);
    }
}

// Standard library function implementations

// Memory functions
void* libc_memcpy(void* dest, const void* src, size_t n) {
    g_module_stats.function_calls++;
    return memcpy(dest, src, n);
}

void* libc_memset(void* s, int c, size_t n) {
    g_module_stats.function_calls++;
    return memset(s, c, n);
}

int libc_memcmp(const void* s1, const void* s2, size_t n) {
    g_module_stats.function_calls++;
    return memcmp(s1, s2, n);
}

void* libc_memmove(void* dest, const void* src, size_t n) {
    g_module_stats.function_calls++;
    return memmove(dest, src, n);
}

// String functions
size_t libc_strlen(const char* s) {
    g_module_stats.function_calls++;
    g_module_stats.string_operations++;
    return strlen(s);
}

char* libc_strcpy(char* dest, const char* src) {
    g_module_stats.function_calls++;
    g_module_stats.string_operations++;
    return strcpy(dest, src);
}

char* libc_strncpy(char* dest, const char* src, size_t n) {
    g_module_stats.function_calls++;
    g_module_stats.string_operations++;
    return strncpy(dest, src, n);
}

int libc_strcmp(const char* s1, const char* s2) {
    g_module_stats.function_calls++;
    g_module_stats.string_operations++;
    return strcmp(s1, s2);
}

int libc_strncmp(const char* s1, const char* s2, size_t n) {
    g_module_stats.function_calls++;
    g_module_stats.string_operations++;
    return strncmp(s1, s2, n);
}

char* libc_strcat(char* dest, const char* src) {
    g_module_stats.function_calls++;
    g_module_stats.string_operations++;
    return strcat(dest, src);
}

char* libc_strncat(char* dest, const char* src, size_t n) {
    g_module_stats.function_calls++;
    g_module_stats.string_operations++;
    return strncat(dest, src, n);
}

char* libc_strchr(const char* s, int c) {
    g_module_stats.function_calls++;
    g_module_stats.string_operations++;
    return strchr(s, c);
}

char* libc_strrchr(const char* s, int c) {
    g_module_stats.function_calls++;
    g_module_stats.string_operations++;
    return strrchr(s, c);
}

// Math functions
double libc_sin(double x) {
    g_module_stats.function_calls++;
    g_module_stats.math_operations++;
    return sin(x);
}

double libc_cos(double x) {
    g_module_stats.function_calls++;
    g_module_stats.math_operations++;
    return cos(x);
}

double libc_tan(double x) {
    g_module_stats.function_calls++;
    g_module_stats.math_operations++;
    return tan(x);
}

double libc_sqrt(double x) {
    g_module_stats.function_calls++;
    g_module_stats.math_operations++;
    return sqrt(x);
}

double libc_pow(double x, double y) {
    g_module_stats.function_calls++;
    g_module_stats.math_operations++;
    return pow(x, y);
}

double libc_log(double x) {
    g_module_stats.function_calls++;
    g_module_stats.math_operations++;
    return log(x);
}

double libc_exp(double x) {
    g_module_stats.function_calls++;
    g_module_stats.math_operations++;
    return exp(x);
}

// I/O functions
int libc_printf(const char* format, ...) {
    g_module_stats.function_calls++;
    g_module_stats.io_operations++;
    
    va_list args;
    va_start(args, format);
    int result = vprintf(format, args);
    va_end(args);
    
    return result;
}

int libc_sprintf(char* str, const char* format, ...) {
    g_module_stats.function_calls++;
    g_module_stats.io_operations++;
    
    va_list args;
    va_start(args, format);
    int result = vsprintf(str, format, args);
    va_end(args);
    
    return result;
}

int libc_snprintf(char* str, size_t size, const char* format, ...) {
    g_module_stats.function_calls++;
    g_module_stats.io_operations++;
    
    va_list args;
    va_start(args, format);
    int result = vsnprintf(str, size, format, args);
    va_end(args);
    
    return result;
}

// Register all libc functions with the native bridge
int libc_module_register_functions(void) {
    ASTCCallSignature sig;
    int registered = 0;
    
    LOG_MODULE_INFO("Registering libc functions with native bridge...");
    
    // Memory functions
    ASTC_SIG_INIT(sig, "Allocate memory");
    ASTC_SIG_ADD_PARAM(sig, ASTC_TYPE_I64);
    ASTC_SIG_SET_RETURN(sig, ASTC_TYPE_PTR);
    if (astc_native_register_interface("libc.malloc", MODULE_NAME, "malloc", &sig) == 0) registered++;
    
    ASTC_SIG_INIT(sig, "Free memory");
    ASTC_SIG_ADD_PARAM(sig, ASTC_TYPE_PTR);
    ASTC_SIG_SET_RETURN(sig, ASTC_TYPE_VOID);
    if (astc_native_register_interface("libc.free", MODULE_NAME, "free", &sig) == 0) registered++;
    
    // String functions
    ASTC_SIG_INIT(sig, "Get string length");
    ASTC_SIG_ADD_PARAM(sig, ASTC_TYPE_STRING);
    ASTC_SIG_SET_RETURN(sig, ASTC_TYPE_I64);
    if (astc_native_register_interface("libc.strlen", MODULE_NAME, "strlen", &sig) == 0) registered++;
    
    ASTC_SIG_INIT(sig, "Copy string");
    ASTC_SIG_ADD_PARAM(sig, ASTC_TYPE_PTR);
    ASTC_SIG_ADD_PARAM(sig, ASTC_TYPE_STRING);
    ASTC_SIG_SET_RETURN(sig, ASTC_TYPE_PTR);
    if (astc_native_register_interface("libc.strcpy", MODULE_NAME, "strcpy", &sig) == 0) registered++;
    
    ASTC_SIG_INIT(sig, "Compare strings");
    ASTC_SIG_ADD_PARAM(sig, ASTC_TYPE_STRING);
    ASTC_SIG_ADD_PARAM(sig, ASTC_TYPE_STRING);
    ASTC_SIG_SET_RETURN(sig, ASTC_TYPE_I32);
    if (astc_native_register_interface("libc.strcmp", MODULE_NAME, "strcmp", &sig) == 0) registered++;
    
    // Math functions
    ASTC_SIG_INIT(sig, "Square root");
    ASTC_SIG_ADD_PARAM(sig, ASTC_TYPE_F64);
    ASTC_SIG_SET_RETURN(sig, ASTC_TYPE_F64);
    if (astc_native_register_interface("libc.sqrt", MODULE_NAME, "sqrt", &sig) == 0) registered++;
    
    ASTC_SIG_INIT(sig, "Power function");
    ASTC_SIG_ADD_PARAM(sig, ASTC_TYPE_F64);
    ASTC_SIG_ADD_PARAM(sig, ASTC_TYPE_F64);
    ASTC_SIG_SET_RETURN(sig, ASTC_TYPE_F64);
    if (astc_native_register_interface("libc.pow", MODULE_NAME, "pow", &sig) == 0) registered++;
    
    // I/O functions
    ASTC_SIG_INIT(sig, "Print formatted string");
    ASTC_SIG_ADD_PARAM(sig, ASTC_TYPE_STRING);
    ASTC_SIG_SET_RETURN(sig, ASTC_TYPE_I32);
    if (astc_native_register_interface("libc.printf", MODULE_NAME, "printf", &sig) == 0) registered++;
    
    LOG_MODULE_INFO("Registered %d libc functions", registered);
    return registered;
}

// Get module statistics
void libc_module_get_stats(LibcModuleStats* stats) {
    if (stats) {
        *stats = g_module_stats;
    }
}

// Module entry point (called when module is loaded)
int libc_module_main(void) {
    if (libc_module_init() != 0) {
        return -1;
    }
    
    if (libc_module_register_functions() <= 0) {
        LOG_MODULE_ERROR("Failed to register libc functions");
        return -1;
    }
    
    LOG_MODULE_INFO("LibC native module loaded successfully");
    return 0;
}
