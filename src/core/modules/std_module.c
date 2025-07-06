/**
 * std_module.c - Standard Library Native Module (Layer 2)
 * 
 * Implements standard library functions as a native module.
 * Provides C standard library functionality for ASTC programs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#include <malloc.h>
#else
#include <unistd.h>
#include <sys/mman.h>
#endif

// Include core components
#include "../core/astc.h"
#include "../core/native.h"
#include "../core/utils.h"

// ===============================================
// Standard Library Module Information
// ===============================================

static bool std_initialized = false;

typedef struct {
    const char* name;
    const char* version;
    const char* description;
    const char* arch;
    int bits;
} StdModuleInfo;

static StdModuleInfo std_info = {
    .name = "std",
    .version = "1.0.0",
    .description = "Standard Library Native Module",
    .arch = 
#if defined(_M_X64) || defined(__x86_64__)
        "x64",
        .bits = 64
#elif defined(_M_IX86) || defined(__i386__)
        "x86",
        .bits = 32
#elif defined(_M_ARM64) || defined(__aarch64__)
        "arm64",
        .bits = 64
#elif defined(_M_ARM) || defined(__arm__)
        "arm32",
        .bits = 32
#else
        "unknown",
        .bits = 0
#endif
};

// ===============================================
// Memory Management Functions
// ===============================================

/**
 * Standard malloc implementation
 */
void* std_malloc(size_t size) {
    void* ptr = malloc(size);
    printf("STD Module: malloc(%zu) = %p\n", size, ptr);
    return ptr;
}

/**
 * Standard free implementation
 */
void std_free(void* ptr) {
    printf("STD Module: free(%p)\n", ptr);
    free(ptr);
}

/**
 * Standard calloc implementation
 */
void* std_calloc(size_t num, size_t size) {
    void* ptr = calloc(num, size);
    printf("STD Module: calloc(%zu, %zu) = %p\n", num, size, ptr);
    return ptr;
}

/**
 * Standard realloc implementation
 */
void* std_realloc(void* ptr, size_t size) {
    void* new_ptr = realloc(ptr, size);
    printf("STD Module: realloc(%p, %zu) = %p\n", ptr, size, new_ptr);
    return new_ptr;
}

// ===============================================
// String Functions
// ===============================================

/**
 * Standard strlen implementation
 */
size_t std_strlen(const char* str) {
    if (!str) return 0;
    size_t len = strlen(str);
    printf("STD Module: strlen(\"%s\") = %zu\n", str, len);
    return len;
}

/**
 * Standard strcpy implementation
 */
char* std_strcpy(char* dest, const char* src) {
    if (!dest || !src) return dest;
    char* result = strcpy(dest, src);
    printf("STD Module: strcpy(dest, \"%s\")\n", src);
    return result;
}

/**
 * Standard strcmp implementation
 */
int std_strcmp(const char* str1, const char* str2) {
    if (!str1 || !str2) return 0;
    int result = strcmp(str1, str2);
    printf("STD Module: strcmp(\"%s\", \"%s\") = %d\n", str1, str2, result);
    return result;
}

/**
 * Standard strcat implementation
 */
char* std_strcat(char* dest, const char* src) {
    if (!dest || !src) return dest;
    char* result = strcat(dest, src);
    printf("STD Module: strcat(dest, \"%s\")\n", src);
    return result;
}

// ===============================================
// I/O Functions
// ===============================================

/**
 * Standard printf implementation
 */
int std_printf(const char* format, ...) {
    if (!format) return 0;
    
    va_list args;
    va_start(args, format);
    int result = vprintf(format, args);
    va_end(args);
    
    return result;
}

/**
 * Standard sprintf implementation
 */
int std_sprintf(char* str, const char* format, ...) {
    if (!str || !format) return 0;
    
    va_list args;
    va_start(args, format);
    int result = vsprintf(str, format, args);
    va_end(args);
    
    return result;
}

/**
 * Standard puts implementation
 */
int std_puts(const char* str) {
    if (!str) return EOF;
    int result = puts(str);
    return result;
}

// ===============================================
// Math Functions
// ===============================================

/**
 * Standard sin implementation
 */
double std_sin(double x) {
    double result = sin(x);
    printf("STD Module: sin(%f) = %f\n", x, result);
    return result;
}

/**
 * Standard cos implementation
 */
double std_cos(double x) {
    double result = cos(x);
    printf("STD Module: cos(%f) = %f\n", x, result);
    return result;
}

/**
 * Standard sqrt implementation
 */
double std_sqrt(double x) {
    double result = sqrt(x);
    printf("STD Module: sqrt(%f) = %f\n", x, result);
    return result;
}

/**
 * Standard pow implementation
 */
double std_pow(double base, double exp) {
    double result = pow(base, exp);
    printf("STD Module: pow(%f, %f) = %f\n", base, exp, result);
    return result;
}

// ===============================================
// Utility Functions
// ===============================================

/**
 * Standard atoi implementation
 */
int std_atoi(const char* str) {
    if (!str) return 0;
    int result = atoi(str);
    printf("STD Module: atoi(\"%s\") = %d\n", str, result);
    return result;
}

/**
 * Standard atof implementation
 */
double std_atof(const char* str) {
    if (!str) return 0.0;
    double result = atof(str);
    printf("STD Module: atof(\"%s\") = %f\n", str, result);
    return result;
}

/**
 * Standard exit implementation
 */
void std_exit(int status) {
    printf("STD Module: exit(%d)\n", status);
    exit(status);
}

// ===============================================
// Module Core Functions
// ===============================================

/**
 * Module initialization
 */
int std_module_init(void) {
    if (std_initialized) {
        return 0; // Already initialized
    }
    
    printf("STD Module: Initializing %s_%s_%d.native\n", 
           std_info.name, std_info.arch, std_info.bits);
    
    std_initialized = true;
    return 0;
}

/**
 * Module cleanup
 */
void std_module_cleanup(void) {
    if (!std_initialized) {
        return;
    }
    
    printf("STD Module: Cleaning up %s_%s_%d.native\n",
           std_info.name, std_info.arch, std_info.bits);
    
    std_initialized = false;
}

/**
 * Get module info
 */
const StdModuleInfo* std_module_get_info(void) {
    return &std_info;
}

/**
 * Get function by name (for dynamic linking)
 */
void* std_module_get_function(const char* name) {
    if (!name) return NULL;
    
    // Memory functions
    if (strcmp(name, "malloc") == 0) return (void*)std_malloc;
    if (strcmp(name, "free") == 0) return (void*)std_free;
    if (strcmp(name, "calloc") == 0) return (void*)std_calloc;
    if (strcmp(name, "realloc") == 0) return (void*)std_realloc;
    
    // String functions
    if (strcmp(name, "strlen") == 0) return (void*)std_strlen;
    if (strcmp(name, "strcpy") == 0) return (void*)std_strcpy;
    if (strcmp(name, "strcmp") == 0) return (void*)std_strcmp;
    if (strcmp(name, "strcat") == 0) return (void*)std_strcat;
    
    // I/O functions
    if (strcmp(name, "printf") == 0) return (void*)std_printf;
    if (strcmp(name, "sprintf") == 0) return (void*)std_sprintf;
    if (strcmp(name, "puts") == 0) return (void*)std_puts;
    
    // Math functions
    if (strcmp(name, "sin") == 0) return (void*)std_sin;
    if (strcmp(name, "cos") == 0) return (void*)std_cos;
    if (strcmp(name, "sqrt") == 0) return (void*)std_sqrt;
    if (strcmp(name, "pow") == 0) return (void*)std_pow;
    
    // Utility functions
    if (strcmp(name, "atoi") == 0) return (void*)std_atoi;
    if (strcmp(name, "atof") == 0) return (void*)std_atof;
    if (strcmp(name, "exit") == 0) return (void*)std_exit;
    
    printf("STD Module: Function '%s' not found\n", name);
    return NULL;
}

// ===============================================
// Module Entry Points (Required Exports)
// ===============================================

// Note: This is a native module, not an executable
// No main() function needed - c2native.exe will extract the machine code
// and create the proper .native format with NATV headers

// Note: No DLL/shared library exports needed
// We use custom .native format with native.c system for symbol management
// This follows PRD.md specification: use mmap() alike, not libdl or traditional DLL
// All exports are handled via native_module_add_export() in main() function
