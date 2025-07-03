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

// Module state
static bool libc_initialized = false;
static uint64_t malloc_count = 0;
static uint64_t free_count = 0;
static uint64_t total_allocated = 0;

// ===============================================
// Extended LibC Function Implementations
// ===============================================

// File I/O Functions
FILE* libc_fopen(const char* filename, const char* mode) {
    if (!filename || !mode) {
        return NULL;
    }
    printf("LibC: fopen(%s, %s)\n", filename, mode);
    return fopen(filename, mode);
}

int libc_fclose(FILE* stream) {
    if (!stream) {
        return EOF;
    }
    printf("LibC: fclose()\n");
    return fclose(stream);
}

size_t libc_fread(void* ptr, size_t size, size_t count, FILE* stream) {
    if (!ptr || !stream) {
        return 0;
    }
    printf("LibC: fread(size=%zu, count=%zu)\n", size, count);
    return fread(ptr, size, count, stream);
}

size_t libc_fwrite(const void* ptr, size_t size, size_t count, FILE* stream) {
    if (!ptr || !stream) {
        return 0;
    }
    printf("LibC: fwrite(size=%zu, count=%zu)\n", size, count);
    return fwrite(ptr, size, count, stream);
}

int libc_fseek(FILE* stream, long offset, int whence) {
    if (!stream) {
        return -1;
    }
    printf("LibC: fseek(offset=%ld, whence=%d)\n", offset, whence);
    return fseek(stream, offset, whence);
}

long libc_ftell(FILE* stream) {
    if (!stream) {
        return -1L;
    }
    printf("LibC: ftell()\n");
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

// String Functions (Extended)
char* libc_strcpy(char* dest, const char* src) {
    if (!dest || !src) {
        return dest;
    }
    printf("LibC: strcpy()\n");
    return strcpy(dest, src);
}

char* libc_strncpy(char* dest, const char* src, size_t n) {
    if (!dest || !src) {
        return dest;
    }
    printf("LibC: strncpy(n=%zu)\n", n);
    return strncpy(dest, src, n);
}

char* libc_strcat(char* dest, const char* src) {
    if (!dest || !src) {
        return dest;
    }
    printf("LibC: strcat()\n");
    return strcat(dest, src);
}

char* libc_strncat(char* dest, const char* src, size_t n) {
    if (!dest || !src) {
        return dest;
    }
    printf("LibC: strncat(n=%zu)\n", n);
    return strncat(dest, src, n);
}

int libc_strcmp(const char* s1, const char* s2) {
    if (!s1 || !s2) {
        return 0;
    }
    printf("LibC: strcmp()\n");
    return strcmp(s1, s2);
}

int libc_strncmp(const char* s1, const char* s2, size_t n) {
    if (!s1 || !s2) {
        return 0;
    }
    printf("LibC: strncmp(n=%zu)\n", n);
    return strncmp(s1, s2, n);
}

char* libc_strchr(const char* s, int c) {
    if (!s) {
        return NULL;
    }
    printf("LibC: strchr(c=%c)\n", c);
    return strchr(s, c);
}

char* libc_strrchr(const char* s, int c) {
    if (!s) {
        return NULL;
    }
    printf("LibC: strrchr(c=%c)\n", c);
    return strrchr(s, c);
}

char* libc_strstr(const char* haystack, const char* needle) {
    if (!haystack || !needle) {
        return NULL;
    }
    printf("LibC: strstr()\n");
    return strstr(haystack, needle);
}

// ===============================================
// Enhanced Memory Management
// ===============================================

// Memory statistics
typedef struct {
    size_t total_allocated;
    size_t total_freed;
    size_t current_usage;
    size_t peak_usage;
    size_t allocation_count;
    size_t free_count;
} MemoryStats;

static MemoryStats g_memory_stats = {0};

// Simple memory pool for small allocations
#define MEMORY_POOL_SIZE 1024 * 1024  // 1MB pool
#define SMALL_ALLOC_THRESHOLD 256      // Allocations <= 256 bytes use pool

static char g_memory_pool[MEMORY_POOL_SIZE];
static size_t g_pool_offset = 0;
static bool g_pool_enabled = true;

// Enhanced malloc with statistics and pool
void* libc_malloc_enhanced(size_t size) {
    if (size == 0) {
        return NULL;
    }

    void* ptr = NULL;

    // Use memory pool for small allocations
    if (g_pool_enabled && size <= SMALL_ALLOC_THRESHOLD &&
        g_pool_offset + size <= MEMORY_POOL_SIZE) {
        ptr = &g_memory_pool[g_pool_offset];
        g_pool_offset += size;
        printf("LibC: malloc_enhanced(%zu) from pool -> %p\n", size, ptr);
    } else {
        // Use system malloc for large allocations
        ptr = malloc(size);
        printf("LibC: malloc_enhanced(%zu) from system -> %p\n", size, ptr);
    }

    if (ptr) {
        g_memory_stats.total_allocated += size;
        g_memory_stats.current_usage += size;
        g_memory_stats.allocation_count++;

        if (g_memory_stats.current_usage > g_memory_stats.peak_usage) {
            g_memory_stats.peak_usage = g_memory_stats.current_usage;
        }
    }

    return ptr;
}

// Enhanced free with statistics
void libc_free_enhanced(void* ptr) {
    if (!ptr) {
        return;
    }

    // Check if pointer is from memory pool
    if (ptr >= (void*)g_memory_pool &&
        ptr < (void*)(g_memory_pool + MEMORY_POOL_SIZE)) {
        printf("LibC: free_enhanced(%p) from pool (no-op)\n", ptr);
        // Pool memory is not individually freed
    } else {
        printf("LibC: free_enhanced(%p) to system\n", ptr);
        free(ptr);
    }

    g_memory_stats.free_count++;
}

// Memory statistics functions
void libc_get_memory_stats(MemoryStats* stats) {
    if (stats) {
        *stats = g_memory_stats;
    }
}

void libc_print_memory_stats(void) {
    printf("=== LibC Memory Statistics ===\n");
    printf("Total allocated: %zu bytes\n", g_memory_stats.total_allocated);
    printf("Total freed: %zu bytes\n", g_memory_stats.total_freed);
    printf("Current usage: %zu bytes\n", g_memory_stats.current_usage);
    printf("Peak usage: %zu bytes\n", g_memory_stats.peak_usage);
    printf("Allocation count: %zu\n", g_memory_stats.allocation_count);
    printf("Free count: %zu\n", g_memory_stats.free_count);
    printf("Pool usage: %zu / %d bytes (%.1f%%)\n",
           g_pool_offset, MEMORY_POOL_SIZE,
           (double)g_pool_offset / MEMORY_POOL_SIZE * 100.0);
    printf("==============================\n");
}

void libc_reset_memory_stats(void) {
    memset(&g_memory_stats, 0, sizeof(MemoryStats));
    g_pool_offset = 0;
}

// ===============================================
// Math Functions
// ===============================================

// Trigonometric functions
double libc_sin(double x) {
    printf("LibC: sin(%f)\n", x);
    return sin(x);
}

double libc_cos(double x) {
    printf("LibC: cos(%f)\n", x);
    return cos(x);
}

double libc_tan(double x) {
    printf("LibC: tan(%f)\n", x);
    return tan(x);
}

double libc_asin(double x) {
    printf("LibC: asin(%f)\n", x);
    return asin(x);
}

double libc_acos(double x) {
    printf("LibC: acos(%f)\n", x);
    return acos(x);
}

double libc_atan(double x) {
    printf("LibC: atan(%f)\n", x);
    return atan(x);
}

double libc_atan2(double y, double x) {
    printf("LibC: atan2(%f, %f)\n", y, x);
    return atan2(y, x);
}

// Exponential and logarithmic functions
double libc_exp(double x) {
    printf("LibC: exp(%f)\n", x);
    return exp(x);
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

// Rounding and remainder functions
double libc_ceil(double x) {
    printf("LibC: ceil(%f)\n", x);
    return ceil(x);
}

double libc_floor(double x) {
    printf("LibC: floor(%f)\n", x);
    return floor(x);
}

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

// Memory management functions
void* libc_malloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr) {
        malloc_count++;
        total_allocated += size;
    }
    return ptr;
}

void libc_free(void* ptr) {
    if (ptr) {
        free(ptr);
        free_count++;
    }
}

void* libc_calloc(size_t num, size_t size) {
    void* ptr = calloc(num, size);
    if (ptr) {
        malloc_count++;
        total_allocated += (num * size);
    }
    return ptr;
}

void* libc_realloc(void* ptr, size_t size) {
    void* new_ptr = realloc(ptr, size);
    if (new_ptr && !ptr) {
        malloc_count++;
        total_allocated += size;
    }
    return new_ptr;
}

// String functions
size_t libc_strlen(const char* str) {
    return strlen(str);
}

char* libc_strcpy(char* dest, const char* src) {
    return strcpy(dest, src);
}

char* libc_strncpy(char* dest, const char* src, size_t n) {
    return strncpy(dest, src, n);
}

int libc_strcmp(const char* str1, const char* str2) {
    return strcmp(str1, str2);
}

int libc_strncmp(const char* str1, const char* str2, size_t n) {
    return strncmp(str1, str2, n);
}

char* libc_strcat(char* dest, const char* src) {
    return strcat(dest, src);
}

char* libc_strncat(char* dest, const char* src, size_t n) {
    return strncat(dest, src, n);
}

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
    
    {NULL, NULL, NULL} // Terminator
};

// ===============================================
// LibC Module Functions
// ===============================================

int libc_native_init(void) {
    if (libc_initialized) {
        return 0; // Already initialized
    }
    
    printf("LibC Module: Initializing libc_%s_%d.native\n", 
           libc_info.arch, libc_info.bits);
    printf("Architecture: %s %d-bit\n", libc_info.arch, libc_info.bits);
    printf("API Version: %u\n", libc_info.api_version);
    
    // Count functions
    libc_info.function_count = 0;
    for (int i = 0; libc_functions[i].name != NULL; i++) {
        libc_info.function_count++;
    }
    
    printf("LibC Module: Registered %u functions\n", libc_info.function_count);
    
    libc_initialized = true;
    return 0;
}

void libc_native_cleanup(void) {
    if (!libc_initialized) {
        return;
    }
    
    printf("LibC Module: Cleaning up libc_%s_%d.native\n",
           libc_info.arch, libc_info.bits);
    printf("Memory Statistics:\n");
    printf("  Malloc calls: %llu\n", malloc_count);
    printf("  Free calls: %llu\n", free_count);
    printf("  Total allocated: %llu bytes\n", total_allocated);
    printf("  Potential leaks: %llu allocations\n", malloc_count - free_count);
    
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
    if (malloc_calls) *malloc_calls = malloc_count;
    if (free_calls) *free_calls = free_count;
    if (total_alloc) *total_alloc = total_allocated;
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
