#define _GNU_SOURCE
/**
 * libc_module.c - Standardized LibC Module Implementation (Layer 2)
 * 
 * Standard implementation for libc_{arch}_{bits}.native modules.
 * Provides C standard library forwarding for ASTC programs.
 * Follows PRD.md Layer 2 specification and native module format.
 * 
 * This file will be compiled into:
 * - libc_x64_64.native
 * - libc_arm64_64.native
 * - libc_x86_32.native
 * - libc_arm32_32.native
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#endif

// ===============================================
// LibC Module Interface (PRD.md compliant)
// ===============================================

typedef struct {
    const char* name;
    const char* version;
    const char* arch;
    int bits;
    uint32_t api_version;
    uint32_t function_count;
} LibCModuleInfo;

typedef struct {
    const char* name;
    void* function_ptr;
    const char* signature;
} LibCFunction;

// ===============================================
// Function Forward Declarations
// ===============================================

char* libc_strerror(int errnum);

// ===============================================
// Module Information (Architecture-specific)
// ===============================================

#ifdef _WIN32
    #ifdef _M_X64
        static LibCModuleInfo libc_info = {
            .name = "libc_core", .version = "1.0.0", .arch = "x64", .bits = 64, .api_version = 1
        };
    #elif defined(_M_ARM64)
        static LibCModuleInfo libc_info = {
            .name = "libc_core", .version = "1.0.0", .arch = "arm64", .bits = 64, .api_version = 1
        };
    #elif defined(_M_IX86)
        static LibCModuleInfo libc_info = {
            .name = "libc_core", .version = "1.0.0", .arch = "x86", .bits = 32, .api_version = 1
        };
    #endif
#else
    #ifdef __x86_64__
        static LibCModuleInfo libc_info = {
            .name = "libc_core", .version = "1.0.0", .arch = "x64", .bits = 64, .api_version = 1
        };
    #elif defined(__aarch64__)
        static LibCModuleInfo libc_info = {
            .name = "libc_core", .version = "1.0.0", .arch = "arm64", .bits = 64, .api_version = 1
        };
    #elif defined(__i386__)
        static LibCModuleInfo libc_info = {
            .name = "libc_core", .version = "1.0.0", .arch = "x86", .bits = 32, .api_version = 1
        };
    #elif defined(__arm__)
        static LibCModuleInfo libc_info = {
            .name = "libc_core", .version = "1.0.0", .arch = "arm32", .bits = 32, .api_version = 1
        };
    #endif
#endif

// T3.2 Enhanced Module State and Statistics
static bool libc_initialized = false;
static bool libc_debug_mode = false;  // T3.2: Configurable debug output

// T3.2 Enhanced Statistics
typedef struct {
    // Memory statistics
    uint64_t malloc_count;
    uint64_t free_count;
    uint64_t calloc_count;
    uint64_t realloc_count;
    uint64_t total_allocated;
    uint64_t total_freed;
    uint64_t current_usage;
    uint64_t peak_usage;

    // Function call statistics
    uint64_t string_operations;
    uint64_t math_operations;
    uint64_t io_operations;
    uint64_t file_operations;
    uint64_t time_operations;
    uint64_t total_function_calls;

    // Performance metrics
    double total_execution_time;
    uint64_t cache_hits;
    uint64_t cache_misses;
} LibcModuleStats;

static LibcModuleStats g_libc_stats = {0};

// ===============================================
// T3.2 Performance Optimization Macros
// ===============================================

#define LIBC_DEBUG_PRINT(fmt, ...) \
    do { if (libc_debug_mode) printf("LibC: " fmt, ##__VA_ARGS__); } while(0)

#define LIBC_STATS_INCREMENT(field) \
    do { g_libc_stats.field++; g_libc_stats.total_function_calls++; } while(0)

#define LIBC_STATS_ADD(field, value) \
    do { g_libc_stats.field += (value); } while(0)

// T3.2 Function call timing macro
#define LIBC_TIMED_CALL(call, category) \
    do { \
        clock_t start = clock(); \
        call; \
        clock_t end = clock(); \
        g_libc_stats.total_execution_time += ((double)(end - start)) / CLOCKS_PER_SEC; \
        LIBC_STATS_INCREMENT(category); \
    } while(0)

// ===============================================
// T3.2 Enhanced LibC Function Implementations
// ===============================================

// T3.2 Optimized File I/O Functions
FILE* libc_fopen(const char* filename, const char* mode) {
    if (!filename || !mode) {
        return NULL;
    }
    LIBC_DEBUG_PRINT("fopen(%s, %s)\n", filename, mode);
    LIBC_STATS_INCREMENT(file_operations);
    return fopen(filename, mode);
}

int libc_fclose(FILE* stream) {
    if (!stream) {
        return EOF;
    }
    LIBC_DEBUG_PRINT("fclose()\n");
    LIBC_STATS_INCREMENT(file_operations);
    return fclose(stream);
}

size_t libc_fread(void* ptr, size_t size, size_t count, FILE* stream) {
    if (!ptr || !stream) {
        return 0;
    }
    LIBC_DEBUG_PRINT("fread(size=%zu, count=%zu)\n", size, count);
    LIBC_STATS_INCREMENT(io_operations);
    return fread(ptr, size, count, stream);
}

size_t libc_fwrite(const void* ptr, size_t size, size_t count, FILE* stream) {
    if (!ptr || !stream) {
        return 0;
    }
    LIBC_DEBUG_PRINT("fwrite(size=%zu, count=%zu)\n", size, count);
    LIBC_STATS_INCREMENT(io_operations);
    return fwrite(ptr, size, count, stream);
}

int libc_fseek(FILE* stream, long offset, int whence) {
    if (!stream) {
        return -1;
    }
    LIBC_DEBUG_PRINT("fseek(offset=%ld, whence=%d)\n", offset, whence);
    LIBC_STATS_INCREMENT(file_operations);
    return fseek(stream, offset, whence);
}

long libc_ftell(FILE* stream) {
    if (!stream) {
        return -1L;
    }
    LIBC_DEBUG_PRINT("ftell()\n");
    LIBC_STATS_INCREMENT(file_operations);
    return ftell(stream);
}

int libc_feof(FILE* stream) {
    if (!stream) {
        return 1;
    }
    return feof(stream);
}

int libc_ferror(FILE* stream) {
    if (!stream) {
        return 1;
    }
    return ferror(stream);
}

// T3.2 Optimized String Functions
size_t libc_strlen(const char* str) {
    if (!str) {
        return 0;
    }
    LIBC_STATS_INCREMENT(string_operations);
    return strlen(str);
}

char* libc_strcpy(char* dest, const char* src) {
    if (!dest || !src) {
        return dest;
    }
    LIBC_STATS_INCREMENT(string_operations);
    return strcpy(dest, src);
}

char* libc_strncpy(char* dest, const char* src, size_t n) {
    if (!dest || !src) {
        return dest;
    }
    LIBC_DEBUG_PRINT("strncpy(n=%zu)\n", n);
    LIBC_STATS_INCREMENT(string_operations);
    return strncpy(dest, src, n);
}

char* libc_strcat(char* dest, const char* src) {
    if (!dest || !src) {
        return dest;
    }
    LIBC_STATS_INCREMENT(string_operations);
    return strcat(dest, src);
}

char* libc_strncat(char* dest, const char* src, size_t n) {
    if (!dest || !src) {
        return dest;
    }
    LIBC_DEBUG_PRINT("strncat(n=%zu)\n", n);
    LIBC_STATS_INCREMENT(string_operations);
    return strncat(dest, src, n);
}

int libc_strcmp(const char* s1, const char* s2) {
    if (!s1 || !s2) {
        return 0;
    }
    LIBC_STATS_INCREMENT(string_operations);
    return strcmp(s1, s2);
}

int libc_strncmp(const char* s1, const char* s2, size_t n) {
    if (!s1 || !s2) {
        return 0;
    }
    LIBC_DEBUG_PRINT("strncmp(n=%zu)\n", n);
    LIBC_STATS_INCREMENT(string_operations);
    return strncmp(s1, s2, n);
}

char* libc_strchr(const char* s, int c) {
    if (!s) {
        return NULL;
    }
    LIBC_DEBUG_PRINT("strchr(c=%c)\n", c);
    LIBC_STATS_INCREMENT(string_operations);
    return strchr(s, c);
}

char* libc_strrchr(const char* s, int c) {
    if (!s) {
        return NULL;
    }
    LIBC_DEBUG_PRINT("strrchr(c=%c)\n", c);
    LIBC_STATS_INCREMENT(string_operations);
    return strrchr(s, c);
}

char* libc_strstr(const char* haystack, const char* needle) {
    if (!haystack || !needle) {
        return NULL;
    }
    LIBC_STATS_INCREMENT(string_operations);
    return strstr(haystack, needle);
}

// ===============================================
// T3.2 Enhanced Memory Management System
// ===============================================

// T3.2 Advanced Memory Pool System
#define MEMORY_POOL_SIZE (2 * 1024 * 1024)  // 2MB pool (increased)
#define SMALL_ALLOC_THRESHOLD 512            // Increased threshold
#define MEMORY_ALIGNMENT 16                  // 16-byte alignment
#define MAX_CACHED_BLOCKS 256                // Free block cache

// T3.2 Memory block header for tracking
typedef struct MemoryBlock {
    size_t size;
    uint32_t magic;
    struct MemoryBlock* next;
} MemoryBlock;

#define MEMORY_MAGIC 0xDEADBEEF

// T3.2 Enhanced memory pools
static char g_memory_pool[MEMORY_POOL_SIZE] __attribute__((aligned(MEMORY_ALIGNMENT)));
static size_t g_pool_offset = 0;
static bool g_pool_enabled = true;

// T3.2 Free block cache for reuse
static MemoryBlock* g_free_blocks[MAX_CACHED_BLOCKS];
static size_t g_free_block_count = 0;

// T3.2 Memory allocation statistics (moved to global stats)

// T3.2 Enhanced malloc with advanced optimization
static void* libc_find_free_block(size_t size) {
    for (size_t i = 0; i < g_free_block_count; i++) {
        if (g_free_blocks[i] && g_free_blocks[i]->size >= size) {
            MemoryBlock* block = g_free_blocks[i];
            // Remove from free list
            g_free_blocks[i] = g_free_blocks[--g_free_block_count];
            g_libc_stats.cache_hits++;
            return (char*)block + sizeof(MemoryBlock);
        }
    }
    g_libc_stats.cache_misses++;
    return NULL;
}

static void libc_add_free_block(MemoryBlock* block) {
    if (g_free_block_count < MAX_CACHED_BLOCKS) {
        g_free_blocks[g_free_block_count++] = block;
    } else {
        // Cache full, actually free the block
        free(block);
    }
}

void* libc_malloc_enhanced(size_t size) {
    if (size == 0) {
        return NULL;
    }

    // T3.2: Align size to memory boundary
    size_t aligned_size = (size + MEMORY_ALIGNMENT - 1) & ~(MEMORY_ALIGNMENT - 1);
    void* ptr = NULL;

    // T3.2: Try to reuse cached free blocks first
    ptr = libc_find_free_block(aligned_size);
    if (ptr) {
        LIBC_DEBUG_PRINT("malloc_enhanced(%zu) from cache -> %p\n", size, ptr);
        LIBC_STATS_INCREMENT(malloc_count);
        LIBC_STATS_ADD(current_usage, aligned_size);
        return ptr;
    }

    // T3.2: Use memory pool for small allocations
    if (g_pool_enabled && aligned_size <= SMALL_ALLOC_THRESHOLD &&
        g_pool_offset + aligned_size + sizeof(MemoryBlock) <= MEMORY_POOL_SIZE) {
        MemoryBlock* block = (MemoryBlock*)&g_memory_pool[g_pool_offset];
        block->size = aligned_size;
        block->magic = MEMORY_MAGIC;
        block->next = NULL;

        ptr = (char*)block + sizeof(MemoryBlock);
        g_pool_offset += aligned_size + sizeof(MemoryBlock);
        LIBC_DEBUG_PRINT("malloc_enhanced(%zu) from pool -> %p\n", size, ptr);
    } else {
        // T3.2: Use system malloc for large allocations
        MemoryBlock* block = malloc(aligned_size + sizeof(MemoryBlock));
        if (block) {
            block->size = aligned_size;
            block->magic = MEMORY_MAGIC;
            block->next = NULL;
            ptr = (char*)block + sizeof(MemoryBlock);
        }
        LIBC_DEBUG_PRINT("malloc_enhanced(%zu) from system -> %p\n", size, ptr);
    }

    if (ptr) {
        LIBC_STATS_INCREMENT(malloc_count);
        LIBC_STATS_ADD(total_allocated, aligned_size);
        LIBC_STATS_ADD(current_usage, aligned_size);

        if (g_libc_stats.current_usage > g_libc_stats.peak_usage) {
            g_libc_stats.peak_usage = g_libc_stats.current_usage;
        }
    }

    return ptr;
}

// T3.2 Enhanced free function with caching and validation
void libc_free_enhanced(void* ptr) {
    if (!ptr) {
        return;
    }

    // T3.2: Get the memory block header
    MemoryBlock* block = (MemoryBlock*)((char*)ptr - sizeof(MemoryBlock));

    // T3.2: Validate magic number for safety
    if (block->magic != MEMORY_MAGIC) {
        LIBC_DEBUG_PRINT("free_enhanced: Invalid magic number, using system free\n");
        free(ptr);  // Fallback to system free
        LIBC_STATS_INCREMENT(free_count);
        return;
    }

    LIBC_STATS_INCREMENT(free_count);
    LIBC_STATS_ADD(total_freed, block->size);
    g_libc_stats.current_usage -= block->size;

    // T3.2: Check if it's from the memory pool
    if ((char*)block >= g_memory_pool &&
        (char*)block < g_memory_pool + MEMORY_POOL_SIZE) {
        // Pool memory - add to free cache for reuse
        libc_add_free_block(block);
        LIBC_DEBUG_PRINT("free_enhanced(%p) cached for reuse\n", ptr);
    } else {
        // System memory - add to cache or free
        libc_add_free_block(block);
        LIBC_DEBUG_PRINT("free_enhanced(%p) system memory cached\n", ptr);
    }
}

// T3.2 Enhanced Statistics and Performance Functions
void libc_get_module_stats(LibcModuleStats* stats) {
    if (stats) {
        *stats = g_libc_stats;
    }
}

void libc_print_performance_report(void) {
    printf("\n=== T3.2 LibC Module Performance Report ===\n");
    printf("Memory Management:\n");
    printf("  Total allocated: %lu bytes\n", g_libc_stats.total_allocated);
    printf("  Total freed: %lu bytes\n", g_libc_stats.total_freed);
    printf("  Current usage: %lu bytes\n", g_libc_stats.current_usage);
    printf("  Peak usage: %lu bytes\n", g_libc_stats.peak_usage);
    printf("  Malloc calls: %lu\n", g_libc_stats.malloc_count);
    printf("  Free calls: %lu\n", g_libc_stats.free_count);
    printf("  Calloc calls: %lu\n", g_libc_stats.calloc_count);
    printf("  Realloc calls: %lu\n", g_libc_stats.realloc_count);

    printf("\nFunction Call Statistics:\n");
    printf("  String operations: %lu\n", g_libc_stats.string_operations);
    printf("  Math operations: %lu\n", g_libc_stats.math_operations);
    printf("  I/O operations: %lu\n", g_libc_stats.io_operations);
    printf("  File operations: %lu\n", g_libc_stats.file_operations);
    printf("  Time operations: %lu\n", g_libc_stats.time_operations);
    printf("  Total function calls: %lu\n", g_libc_stats.total_function_calls);

    printf("\nPerformance Metrics:\n");
    printf("  Total execution time: %.3f seconds\n", g_libc_stats.total_execution_time);
    printf("  Cache hits: %lu\n", g_libc_stats.cache_hits);
    printf("  Cache misses: %lu\n", g_libc_stats.cache_misses);

    if (g_libc_stats.cache_hits + g_libc_stats.cache_misses > 0) {
        double hit_rate = (100.0 * g_libc_stats.cache_hits) /
                         (g_libc_stats.cache_hits + g_libc_stats.cache_misses);
        printf("  Cache hit rate: %.1f%%\n", hit_rate);
    }

    printf("\nMemory Pool Status:\n");
    printf("  Pool usage: %zu / %d bytes (%.1f%%)\n",
           g_pool_offset, MEMORY_POOL_SIZE,
           (double)g_pool_offset / MEMORY_POOL_SIZE * 100.0);
    printf("  Free blocks cached: %zu / %d\n", g_free_block_count, MAX_CACHED_BLOCKS);
    printf("  Debug mode: %s\n", libc_debug_mode ? "ON" : "OFF");
    printf("==========================================\n\n");
}

void libc_reset_stats(void) {
    memset(&g_libc_stats, 0, sizeof(LibcModuleStats));
    g_pool_offset = 0;
    g_free_block_count = 0;
    printf("LibC: Statistics reset\n");
}

void libc_set_debug_mode(bool enabled) {
    libc_debug_mode = enabled;
    printf("LibC: Debug mode %s\n", enabled ? "enabled" : "disabled");
}

// T3.2 Missing C99 Functions Implementation

// Wide character support (basic implementation)
#include <wchar.h>  // Add missing header for wide character functions

// Note: These functions are now declared in the header above
// and implemented below only if not already defined elsewhere

// ===============================================
// T3.2 Optimized Math Functions
// ===============================================

// Trigonometric functions
double libc_sin(double x) {
    LIBC_STATS_INCREMENT(math_operations);
    return sin(x);
}

double libc_cos(double x) {
    LIBC_STATS_INCREMENT(math_operations);
    return cos(x);
}

double libc_tan(double x) {
    LIBC_STATS_INCREMENT(math_operations);
    return tan(x);
}

double libc_asin(double x) {
    LIBC_STATS_INCREMENT(math_operations);
    return asin(x);
}

double libc_acos(double x) {
    LIBC_STATS_INCREMENT(math_operations);
    return acos(x);
}

double libc_atan(double x) {
    LIBC_STATS_INCREMENT(math_operations);
    return atan(x);
}

double libc_atan2(double y, double x) {
    LIBC_DEBUG_PRINT("atan2(%f, %f)\n", y, x);
    LIBC_STATS_INCREMENT(math_operations);
    return atan2(y, x);
}

// Exponential and logarithmic functions
double libc_exp(double x) {
    LIBC_STATS_INCREMENT(math_operations);
    return exp(x);
}

// T3.2 Additional C99 math functions
double libc_round(double x) {
    LIBC_STATS_INCREMENT(math_operations);
    return round(x);
}

double libc_trunc(double x) {
    LIBC_STATS_INCREMENT(math_operations);
    return trunc(x);
}

double libc_floor(double x) {
    LIBC_STATS_INCREMENT(math_operations);
    return floor(x);
}

double libc_ceil(double x) {
    LIBC_STATS_INCREMENT(math_operations);
    return ceil(x);
}

long long libc_llabs(long long x) {
    LIBC_STATS_INCREMENT(math_operations);
    return llabs(x);
}

double libc_log(double x) {
    printf("LibC: log(%f)\n", x);
    return log(x);
}

double libc_log10(double x) {
    printf("LibC: log10(%f)\n", x);
    return log10(x);
}

double libc_pow(double base, double exponent) {
    printf("LibC: pow(%f, %f)\n", base, exponent);
    return pow(base, exponent);
}

double libc_sqrt(double x) {
    printf("LibC: sqrt(%f)\n", x);
    return sqrt(x);
}

// Rounding and remainder functions (duplicates removed)

double libc_fabs(double x) {
    printf("LibC: fabs(%f)\n", x);
    return fabs(x);
}

double libc_fmod(double x, double y) {
    printf("LibC: fmod(%f, %f)\n", x, y);
    return fmod(x, y);
}

// Hyperbolic functions
double libc_sinh(double x) {
    printf("LibC: sinh(%f)\n", x);
    return sinh(x);
}

double libc_cosh(double x) {
    printf("LibC: cosh(%f)\n", x);
    return cosh(x);
}

double libc_tanh(double x) {
    printf("LibC: tanh(%f)\n", x);
    return tanh(x);
}

// Integer math functions
int libc_abs(int x) {
    printf("LibC: abs(%d)\n", x);
    return abs(x);
}

long libc_labs(long x) {
    printf("LibC: labs(%ld)\n", x);
    return labs(x);
}

// Random number functions
int libc_rand(void) {
    int result = rand();
    printf("LibC: rand() -> %d\n", result);
    return result;
}

void libc_srand(unsigned int seed) {
    printf("LibC: srand(%u)\n", seed);
    srand(seed);
}

// ===============================================
// 增强的C99标准库函数 (T4.1)
// ===============================================

// 扩展的stdio函数
int libc_fprintf(FILE* stream, const char* format, ...) {
    if (!stream || !format) return -1;

    va_list args;
    va_start(args, format);
    int result = vfprintf(stream, format, args);
    va_end(args);

    printf("LibC: fprintf() -> %d\n", result);
    return result;
}

int libc_fscanf(FILE* stream, const char* format, ...) {
    if (!stream || !format) return EOF;

    va_list args;
    va_start(args, format);
    int result = vfscanf(stream, format, args);
    va_end(args);

    printf("LibC: fscanf() -> %d\n", result);
    return result;
}

int libc_scanf(const char* format, ...) {
    if (!format) return EOF;

    va_list args;
    va_start(args, format);
    int result = vscanf(format, args);
    va_end(args);

    printf("LibC: scanf() -> %d\n", result);
    return result;
}

int libc_sscanf(const char* str, const char* format, ...) {
    if (!str || !format) return EOF;

    va_list args;
    va_start(args, format);
    int result = vsscanf(str, format, args);
    va_end(args);

    printf("LibC: sscanf() -> %d\n", result);
    return result;
}

// 扩展的字符串函数
char* libc_strdup(const char* s) {
    if (!s) return NULL;

    size_t len = strlen(s) + 1;
    char* dup = malloc(len);
    if (dup) {
        memcpy(dup, s, len);
    }

    printf("LibC: strdup(%zu bytes)\n", len);
    return dup;
}

// 重复函数已删除，使用上面的实现

char* libc_strtok(char* str, const char* delim) {
    char* result = strtok(str, delim);
    printf("LibC: strtok() -> %p\n", (void*)result);
    return result;
}

// T3.2 Missing functions implementation
char* libc_strndup(const char* s, size_t n) {
    if (!s) return NULL;

    size_t len = strnlen(s, n);
    char* dup = libc_malloc_enhanced(len + 1);
    if (dup) {
        memcpy(dup, s, len);
        dup[len] = '\0';
    }
    LIBC_STATS_INCREMENT(string_operations);
    return dup;
}

// Wide character functions (basic implementation)
size_t libc_wcslen(const wchar_t* s) {
    if (!s) return 0;
    LIBC_STATS_INCREMENT(string_operations);
    return wcslen(s);
}

wchar_t* libc_wcscpy(wchar_t* dest, const wchar_t* src) {
    if (!dest || !src) return dest;
    LIBC_STATS_INCREMENT(string_operations);
    return wcscpy(dest, src);
}

int libc_wcscmp(const wchar_t* s1, const wchar_t* s2) {
    if (!s1 || !s2) return 0;
    LIBC_STATS_INCREMENT(string_operations);
    return wcscmp(s1, s2);
}

double libc_remainder(double x, double y) {
    double result = remainder(x, y);
    printf("LibC: remainder(%f, %f) -> %f\n", x, y, result);
    return result;
}

// 类型转换函数
int libc_atoi(const char* str) {
    if (!str) return 0;

    int result = atoi(str);
    printf("LibC: atoi(%s) -> %d\n", str, result);
    return result;
}

long libc_atol(const char* str) {
    if (!str) return 0L;

    long result = atol(str);
    printf("LibC: atol(%s) -> %ld\n", str, result);
    return result;
}

double libc_atof(const char* str) {
    if (!str) return 0.0;

    double result = atof(str);
    printf("LibC: atof(%s) -> %f\n", str, result);
    return result;
}

long libc_strtol(const char* str, char** endptr, int base) {
    if (!str) return 0L;

    long result = strtol(str, endptr, base);
    printf("LibC: strtol(%s, base=%d) -> %ld\n", str, base, result);
    return result;
}

double libc_strtod(const char* str, char** endptr) {
    if (!str) return 0.0;

    double result = strtod(str, endptr);
    printf("LibC: strtod(%s) -> %f\n", str, result);
    return result;
}

// 字符分类函数
int libc_isalpha(int c) {
    int result = isalpha(c);
    printf("LibC: isalpha(%c) -> %d\n", c, result);
    return result;
}

int libc_isdigit(int c) {
    int result = isdigit(c);
    printf("LibC: isdigit(%c) -> %d\n", c, result);
    return result;
}

int libc_isalnum(int c) {
    int result = isalnum(c);
    printf("LibC: isalnum(%c) -> %d\n", c, result);
    return result;
}

int libc_isspace(int c) {
    int result = isspace(c);
    printf("LibC: isspace(%c) -> %d\n", c, result);
    return result;
}

int libc_toupper(int c) {
    int result = toupper(c);
    printf("LibC: toupper(%c) -> %c\n", c, result);
    return result;
}

int libc_tolower(int c) {
    int result = tolower(c);
    printf("LibC: tolower(%c) -> %c\n", c, result);
    return result;
}

// ===============================================
// 系统调用接口实现 (T4.2)
// ===============================================

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <process.h>
#define mkdir(path, mode) _mkdir(path)
#define rmdir(path) _rmdir(path)
#define getcwd(buf, size) _getcwd(buf, size)
#define chdir(path) _chdir(path)
#define access(path, mode) _access(path, mode)
#define unlink(path) _unlink(path)
#define getpid() _getpid()
#else
#include <sys/wait.h>
#include <dirent.h>
#endif

// 文件系统操作
int libc_open(const char* pathname, int flags, ...) {
    if (!pathname) return -1;

    int fd;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode_t mode = va_arg(args, mode_t);
        va_end(args);
        fd = open(pathname, flags, mode);
    } else {
        fd = open(pathname, flags);
    }

    printf("LibC: open(%s, %d) -> %d\n", pathname, flags, fd);
    return fd;
}

int libc_close(int fd) {
    int result = close(fd);
    printf("LibC: close(%d) -> %d\n", fd, result);
    return result;
}

ssize_t libc_read(int fd, void* buf, size_t count) {
    if (!buf) return -1;

    ssize_t result = read(fd, buf, count);
    printf("LibC: read(%d, %zu bytes) -> %zd\n", fd, count, result);
    return result;
}

ssize_t libc_write(int fd, const void* buf, size_t count) {
    if (!buf) return -1;

    ssize_t result = write(fd, buf, count);
    printf("LibC: write(%d, %zu bytes) -> %zd\n", fd, count, result);
    return result;
}

off_t libc_lseek(int fd, off_t offset, int whence) {
    off_t result = lseek(fd, offset, whence);
    printf("LibC: lseek(%d, %ld, %d) -> %ld\n", fd, offset, whence, result);
    return result;
}

int libc_stat(const char* pathname, struct stat* statbuf) {
    if (!pathname || !statbuf) return -1;

    int result = stat(pathname, statbuf);
    printf("LibC: stat(%s) -> %d\n", pathname, result);
    return result;
}

int libc_mkdir(const char* pathname, mode_t mode) {
    if (!pathname) return -1;

    int result = mkdir(pathname, mode);
    printf("LibC: mkdir(%s, %o) -> %d\n", pathname, mode, result);
    return result;
}

int libc_rmdir(const char* pathname) {
    if (!pathname) return -1;

    int result = rmdir(pathname);
    printf("LibC: rmdir(%s) -> %d\n", pathname, result);
    return result;
}

int libc_unlink(const char* pathname) {
    if (!pathname) return -1;

    int result = unlink(pathname);
    printf("LibC: unlink(%s) -> %d\n", pathname, result);
    return result;
}

char* libc_getcwd(char* buf, size_t size) {
    char* result = getcwd(buf, size);
    printf("LibC: getcwd() -> %s\n", result ? result : "NULL");
    return result;
}

int libc_chdir(const char* path) {
    if (!path) return -1;

    int result = chdir(path);
    printf("LibC: chdir(%s) -> %d\n", path, result);
    return result;
}

int libc_access(const char* pathname, int mode) {
    if (!pathname) return -1;

    int result = access(pathname, mode);
    printf("LibC: access(%s, %d) -> %d\n", pathname, mode, result);
    return result;
}

// 进程管理
pid_t libc_getpid(void) {
    pid_t result = getpid();
    printf("LibC: getpid() -> %d\n", result);
    return result;
}

#ifndef _WIN32
pid_t libc_fork(void) {
    pid_t result = fork();
    printf("LibC: fork() -> %d\n", result);
    return result;
}

int libc_execv(const char* path, char* const argv[]) {
    if (!path || !argv) return -1;

    printf("LibC: execv(%s)\n", path);
    return execv(path, argv);
}

int libc_execvp(const char* file, char* const argv[]) {
    if (!file || !argv) return -1;

    printf("LibC: execvp(%s)\n", file);
    return execvp(file, argv);
}

pid_t libc_wait(int* wstatus) {
    pid_t result = wait(wstatus);
    printf("LibC: wait() -> %d\n", result);
    return result;
}

pid_t libc_waitpid(pid_t pid, int* wstatus, int options) {
    pid_t result = waitpid(pid, wstatus, options);
    printf("LibC: waitpid(%d, %d) -> %d\n", pid, options, result);
    return result;
}
#endif

void libc_exit(int status) {
    printf("LibC: exit(%d)\n", status);
    exit(status);
}

int libc_system(const char* command) {
    if (!command) return -1;

    printf("LibC: system(%s)\n", command);
    return system(command);
}

// 时间和日期处理
time_t libc_time(time_t* tloc) {
    time_t result = time(tloc);
    printf("LibC: time() -> %ld\n", result);
    return result;
}

struct tm* libc_localtime(const time_t* timep) {
    if (!timep) return NULL;

    struct tm* result = localtime(timep);
    printf("LibC: localtime(%ld)\n", *timep);
    return result;
}

struct tm* libc_gmtime(const time_t* timep) {
    if (!timep) return NULL;

    struct tm* result = gmtime(timep);
    printf("LibC: gmtime(%ld)\n", *timep);
    return result;
}

time_t libc_mktime(struct tm* tm) {
    if (!tm) return -1;

    time_t result = mktime(tm);
    printf("LibC: mktime() -> %ld\n", result);
    return result;
}

size_t libc_strftime(char* s, size_t maxsize, const char* format, const struct tm* tm) {
    if (!s || !format || !tm) return 0;

    size_t result = strftime(s, maxsize, format, tm);
    printf("LibC: strftime() -> %zu\n", result);
    return result;
}

clock_t libc_clock(void) {
    clock_t result = clock();
    printf("LibC: clock() -> %ld\n", result);
    return result;
}

// 信号处理
#ifndef _WIN32
int libc_kill(pid_t pid, int sig) {
    int result = kill(pid, sig);
    printf("LibC: kill(%d, %d) -> %d\n", pid, sig, result);
    return result;
}

void (*libc_signal(int sig, void (*handler)(int)))(int) {
    printf("LibC: signal(%d)\n", sig);
    return signal(sig, handler);
}
#endif

// 环境变量
char* libc_getenv(const char* name) {
    if (!name) return NULL;

    char* result = getenv(name);
    printf("LibC: getenv(%s) -> %s\n", name, result ? result : "NULL");
    return result;
}

int libc_setenv(const char* name, const char* value, int overwrite) {
    if (!name || !value) return -1;

    #ifdef _WIN32
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%s=%s", name, value);
    int result = _putenv(buffer);
    #else
    int result = setenv(name, value, overwrite);
    #endif

    printf("LibC: setenv(%s, %s, %d) -> %d\n", name, value, overwrite, result);
    return result;
}

int libc_unsetenv(const char* name) {
    if (!name) return -1;

    #ifdef _WIN32
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%s=", name);
    int result = _putenv(buffer);
    #else
    int result = unsetenv(name);
    #endif

    printf("LibC: unsetenv(%s) -> %d\n", name, result);
    return result;
}

// ===============================================
// Error Handling and errno Management
// ===============================================

// Thread-local errno simulation (simplified)
static int g_libc_errno = 0;

// Error message table
static const char* error_messages[] = {
    "Success",                          // 0
    "Operation not permitted",          // EPERM
    "No such file or directory",        // ENOENT
    "No such process",                  // ESRCH
    "Interrupted system call",          // EINTR
    "I/O error",                        // EIO
    "No such device or address",        // ENXIO
    "Argument list too long",           // E2BIG
    "Exec format error",                // ENOEXEC
    "Bad file number",                  // EBADF
    "No child processes",               // ECHILD
    "Try again",                        // EAGAIN
    "Out of memory",                    // ENOMEM
    "Permission denied",                // EACCES
    "Bad address",                      // EFAULT
    "Block device required",            // ENOTBLK
    "Device or resource busy",          // EBUSY
    "File exists",                      // EEXIST
    "Cross-device link",                // EXDEV
    "No such device",                   // ENODEV
    "Not a directory",                  // ENOTDIR
    "Is a directory",                   // EISDIR
    "Invalid argument",                 // EINVAL
    "File table overflow",              // ENFILE
    "Too many open files",              // EMFILE
    "Not a typewriter",                 // ENOTTY
    "Text file busy",                   // ETXTBSY
    "File too large",                   // EFBIG
    "No space left on device",         // ENOSPC
    "Illegal seek",                     // ESPIPE
    "Read-only file system",            // EROFS
    "Too many links"                    // EMLINK
};

#define MAX_ERROR_CODE (sizeof(error_messages) / sizeof(error_messages[0]) - 1)

// Get current errno
int* libc_get_errno_ptr(void) {
    return &g_libc_errno;
}

int libc_get_errno(void) {
    return g_libc_errno;
}

void libc_set_errno(int error_code) {
    g_libc_errno = error_code;
    printf("LibC: errno set to %d (%s)\n", error_code, libc_strerror(error_code));
}

void libc_clear_errno(void) {
    g_libc_errno = 0;
}

// Error message functions
char* libc_strerror(int errnum) {
    static char unknown_error[64];

    if (errnum >= 0 && errnum <= MAX_ERROR_CODE) {
        return (char*)error_messages[errnum];
    } else {
        snprintf(unknown_error, sizeof(unknown_error), "Unknown error %d", errnum);
        return unknown_error;
    }
}

void libc_perror(const char* s) {
    if (s && *s) {
        printf("%s: %s\n", s, libc_strerror(g_libc_errno));
    } else {
        printf("%s\n", libc_strerror(g_libc_errno));
    }
}

// Error checking wrapper functions
void* libc_malloc_safe(size_t size) {
    void* ptr = libc_malloc_enhanced(size);
    if (!ptr && size > 0) {
        libc_set_errno(12); // ENOMEM
    }
    return ptr;
}

FILE* libc_fopen_safe(const char* filename, const char* mode) {
    FILE* file = libc_fopen(filename, mode);
    if (!file) {
        libc_set_errno(2); // ENOENT - simplified error handling
    }
    return file;
}

int libc_fclose_safe(FILE* stream) {
    int result = libc_fclose(stream);
    if (result != 0) {
        libc_set_errno(9); // EBADF - simplified error handling
    }
    return result;
}

// Error reporting functions
void libc_print_error_stats(void) {
    printf("=== LibC Error Statistics ===\n");
    printf("Current errno: %d (%s)\n", g_libc_errno, libc_strerror(g_libc_errno));
    printf("Error handling: Enhanced\n");
    printf("Thread safety: Simplified (single-threaded)\n");
    printf("============================\n");
}

// ===============================================
// LibC Function Implementations
// ===============================================

// Memory management functions (T3.2 optimized)
void* libc_malloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr) {
        LIBC_STATS_INCREMENT(malloc_count);
        LIBC_STATS_ADD(total_allocated, size);
    }
    return ptr;
}

void libc_free(void* ptr) {
    if (ptr) {
        free(ptr);
        LIBC_STATS_INCREMENT(free_count);
    }
}

void* libc_calloc(size_t num, size_t size) {
    void* ptr = calloc(num, size);
    if (ptr) {
        LIBC_STATS_INCREMENT(calloc_count);
        LIBC_STATS_ADD(total_allocated, (num * size));
    }
    return ptr;
}

void* libc_realloc(void* ptr, size_t size) {
    void* new_ptr = realloc(ptr, size);
    if (new_ptr && !ptr) {
        LIBC_STATS_INCREMENT(realloc_count);
        LIBC_STATS_ADD(total_allocated, size);
    }
    return new_ptr;
}

// String functions already defined above (lines 165-210) with debug output
// Removed duplicate definitions to fix compilation errors

// Memory functions
void* libc_memcpy(void* dest, const void* src, size_t n) {
    return memcpy(dest, src, n);
}

void* libc_memmove(void* dest, const void* src, size_t n) {
    return memmove(dest, src, n);
}

void* libc_memset(void* s, int c, size_t n) {
    return memset(s, c, n);
}

int libc_memcmp(const void* s1, const void* s2, size_t n) {
    return memcmp(s1, s2, n);
}

// I/O functions
int libc_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vprintf(format, args);
    va_end(args);
    return result;
}

int libc_sprintf(char* str, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vsprintf(str, format, args);
    va_end(args);
    return result;
}

int libc_snprintf(char* str, size_t size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vsnprintf(str, size, format, args);
    va_end(args);
    return result;
}

int libc_puts(const char* str) {
    return puts(str);
}

int libc_putchar(int c) {
    return putchar(c);
}

// ===============================================
// Function Table
// ===============================================

static LibCFunction libc_functions[] = {
    // Memory management
    {"malloc", libc_malloc, "void*(size_t)"},
    {"free", libc_free, "void(void*)"},
    {"calloc", libc_calloc, "void*(size_t,size_t)"},
    {"realloc", libc_realloc, "void*(void*,size_t)"},
    
    // String functions
    {"strlen", libc_strlen, "size_t(const char*)"},
    {"strcpy", libc_strcpy, "char*(char*,const char*)"},
    {"strncpy", libc_strncpy, "char*(char*,const char*,size_t)"},
    {"strcmp", libc_strcmp, "int(const char*,const char*)"},
    {"strncmp", libc_strncmp, "int(const char*,const char*,size_t)"},
    {"strcat", libc_strcat, "char*(char*,const char*)"},
    {"strncat", libc_strncat, "char*(char*,const char*,size_t)"},
    
    // Memory functions
    {"memcpy", libc_memcpy, "void*(void*,const void*,size_t)"},
    {"memmove", libc_memmove, "void*(void*,const void*,size_t)"},
    {"memset", libc_memset, "void*(void*,int,size_t)"},
    {"memcmp", libc_memcmp, "int(const void*,const void*,size_t)"},
    
    // I/O functions
    {"printf", libc_printf, "int(const char*,...)"},
    {"sprintf", libc_sprintf, "int(char*,const char*,...)"},
    {"snprintf", libc_snprintf, "int(char*,size_t,const char*,...)"},
    {"puts", libc_puts, "int(const char*)"},
    {"putchar", libc_putchar, "int(int)"},

    // T3.2 Enhanced memory functions
    {"malloc_enhanced", libc_malloc_enhanced, "void*(size_t)"},
    {"free_enhanced", libc_free_enhanced, "void(void*)"},

    // T3.2 Additional string functions
    {"strdup", libc_strdup, "char*(const char*)"},
    {"strndup", libc_strndup, "char*(const char*,size_t)"},
    {"strchr", libc_strchr, "char*(const char*,int)"},
    {"strrchr", libc_strrchr, "char*(const char*,int)"},
    {"strstr", libc_strstr, "char*(const char*,const char*)"},

    // T3.2 Wide character functions
    {"wcslen", libc_wcslen, "size_t(const wchar_t*)"},
    {"wcscpy", libc_wcscpy, "wchar_t*(wchar_t*,const wchar_t*)"},
    {"wcscmp", libc_wcscmp, "int(const wchar_t*,const wchar_t*)"},

    // T3.2 Enhanced math functions
    {"sin", libc_sin, "double(double)"},
    {"cos", libc_cos, "double(double)"},
    {"tan", libc_tan, "double(double)"},
    {"round", libc_round, "double(double)"},
    {"trunc", libc_trunc, "double(double)"},
    {"floor", libc_floor, "double(double)"},
    {"ceil", libc_ceil, "double(double)"},
    {"llabs", libc_llabs, "long long(long long)"},

    // T3.2 Performance and statistics functions
    {"libc_get_module_stats", libc_get_module_stats, "void(LibcModuleStats*)"},
    {"libc_print_performance_report", libc_print_performance_report, "void(void)"},
    {"libc_reset_stats", libc_reset_stats, "void(void)"},
    {"libc_set_debug_mode", libc_set_debug_mode, "void(bool)"},

    {NULL, NULL, NULL} // Terminator
};

// ===============================================
// LibC Module Functions
// ===============================================

int libc_native_init(void) {
    if (libc_initialized) {
        return 0; // Already initialized
    }
    
    printf("LibC Module: Initializing T3.2 Enhanced libc_%s_%d.native\n",
           libc_info.arch, libc_info.bits);
    printf("Architecture: %s %d-bit\n", libc_info.arch, libc_info.bits);
    printf("API Version: %u\n", libc_info.api_version);

    // Count functions
    libc_info.function_count = 0;
    for (int i = 0; libc_functions[i].name != NULL; i++) {
        libc_info.function_count++;
    }

    printf("LibC Module: ✅ T3.2 Registered %u functions\n", libc_info.function_count);
    printf("LibC Module: ✅ T3.2 Enhanced memory management with %dMB pool\n", MEMORY_POOL_SIZE / (1024*1024));
    printf("LibC Module: ✅ T3.2 Free block caching (%d max blocks)\n", MAX_CACHED_BLOCKS);
    printf("LibC Module: ✅ T3.2 Performance statistics and monitoring\n");
    printf("LibC Module: ✅ T3.2 Configurable debug output (default: OFF)\n");
    printf("LibC Module: ✅ T3.2 Extended C99 function support\n");

    // T3.2: Initialize enhanced statistics
    memset(&g_libc_stats, 0, sizeof(LibcModuleStats));
    memset(g_free_blocks, 0, sizeof(g_free_blocks));
    g_free_block_count = 0;
    g_pool_offset = 0;
    libc_debug_mode = false;  // Default to quiet mode for performance

    libc_initialized = true;
    printf("LibC Module: T3.2 Enhanced initialization completed\n");
    return 0;
}

void libc_native_cleanup(void) {
    if (!libc_initialized) {
        return;
    }

    printf("LibC Module: T3.2 Cleaning up libc_%s_%d.native\n",
           libc_info.arch, libc_info.bits);

    // T3.2: Print comprehensive performance report
    libc_print_performance_report();

    libc_initialized = false;
}

void* libc_native_get_function(const char* name) {
    if (!libc_initialized) {
        return NULL;
    }
    
    for (int i = 0; libc_functions[i].name != NULL; i++) {
        if (strcmp(libc_functions[i].name, name) == 0) {
            return libc_functions[i].function_ptr;
        }
    }
    
    return NULL;
}

const LibCModuleInfo* libc_native_get_info(void) {
    return &libc_info;
}

void libc_native_get_stats(uint64_t* malloc_calls, uint64_t* free_calls, uint64_t* total_alloc) {
    if (malloc_calls) *malloc_calls = g_libc_stats.malloc_count;
    if (free_calls) *free_calls = g_libc_stats.free_count;
    if (total_alloc) *total_alloc = g_libc_stats.total_allocated;
}

// ===============================================
// Module Entry Points (Required Exports)
// ===============================================

/**
 * libc_native_main - Main entry point for LibC module
 * 
 * This is called when the module is loaded directly.
 * Primarily for testing purposes.
 */
int libc_native_main(int argc, char* argv[]) {
    printf("LibC Module Test Mode\n");
    printf("====================\n");
    
    // Initialize
    int init_result = libc_native_init();
    if (init_result != 0) {
        fprintf(stderr, "LibC Error: Initialization failed\n");
        return -1;
    }
    
    // Run basic tests
    printf("Testing basic functions...\n");
    
    // Test malloc/free
    void* ptr = libc_malloc(1024);
    if (ptr) {
        printf("✓ malloc(1024) succeeded\n");
        libc_free(ptr);
        printf("✓ free() succeeded\n");
    }
    
    // Test string functions
    char buffer[256];
    libc_strcpy(buffer, "Hello, ");
    libc_strcat(buffer, "World!");
    printf("✓ String test: %s\n", buffer);
    
    // Test printf
    libc_printf("✓ Printf test: %d + %d = %d\n", 2, 3, 5);
    
    printf("LibC Module: All tests passed\n");
    
    // Cleanup
    libc_native_cleanup();
    
    return 0;
}

// ===============================================
// Module Metadata (for .native format)
// ===============================================

const char* libc_module_name = "libc_core";
const char* libc_module_version = "1.0.0";
const char* libc_module_author = "Self-Evolve AI Team";
const char* libc_module_description = "C Standard Library Forwarding Module";
const char* libc_module_license = "MIT";

// Export table for .native format
const char* libc_exports[] = {
    "libc_native_init",
    "libc_native_cleanup", 
    "libc_native_get_function",
    "libc_native_get_info",
    "libc_native_get_stats",
    "libc_native_main",
    // Individual functions
    "malloc", "free", "calloc", "realloc",
    "strlen", "strcpy", "strncpy", "strcmp", "strncmp", "strcat", "strncat",
    "memcpy", "memmove", "memset", "memcmp",
    "printf", "sprintf", "snprintf", "puts", "putchar",
    NULL
};

// Dependencies for .native format
const char* libc_dependencies[] = {
    NULL // No dependencies
};

// ===============================================
// Module Initialization (Constructor)
// ===============================================

#ifdef _WIN32
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            // Module loaded
            break;
        case DLL_PROCESS_DETACH:
            // Module unloaded
            libc_native_cleanup();
            break;
    }
    return TRUE;
}
#else
__attribute__((constructor))
void libc_module_constructor(void) {
    // Module loaded
}

__attribute__((destructor))
void libc_module_destructor(void) {
    // Module unloaded
    libc_native_cleanup();
}
#endif

// ===============================================
// Module System Integration
// ===============================================

#include "../module.h"

/**
 * Initialize the module
 */
static int libc_module_init(void) {
    return libc_native_init();
}

/**
 * Clean up the module
 */
static void libc_module_cleanup(void) {
    libc_native_cleanup();
}

/**
 * Resolve a symbol from this module
 */
static void* libc_module_resolve(const char* symbol) {
    return libc_native_get_function(symbol);
}

// Module definition - compatible with new module.h structure
Module module_libc = {
    .name = "libc",
    .state = MODULE_UNLOADED,
    .error = NULL,
    .init = libc_module_init,
    .cleanup = libc_module_cleanup,
    .resolve = libc_module_resolve
};

// 注意：不再需要REGISTER_MODULE，使用动态加载机制
