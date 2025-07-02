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
