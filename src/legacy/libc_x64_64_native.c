/**
 * libc_x64_64_native.c - Libc Forwarding Module for x64 64-bit Architecture
 * 
 * This implements the libc forwarding module according to PRD.md requirements:
 * - High-performance C standard library interface
 * - System library forwarding to native libc
 * - Module interface for dynamic loading by VM
 * 
 * File naming follows PRD.md convention: libc_{arch}_{bits}.native
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <ctype.h>

// Include existing libc forwarding implementation
#include "runtime/core_libc.c"

// ===============================================
// Libc Module Interface (PRD.md compliant)
// ===============================================

typedef struct {
    const char* name;
    const char* version;
    const char* arch;
    int bits;
    int function_count;
} LibcModuleInfo;

typedef struct {
    const char* name;
    void* function_ptr;
    const char* signature;
} LibcFunction;

// ===============================================
// Libc Module Implementation
// ===============================================

static LibcModuleInfo libc_info = {
    .name = "libc_forward",
    .version = "1.0.0",
    .arch = "x64",
    .bits = 64,
    .function_count = 0
};

// Libc module initialization
int libc_module_init(void) {
    printf("Libc Module: Initializing libc_x64_64.native\n");
    printf("Architecture: %s %d-bit\n", libc_info.arch, libc_info.bits);
    
    // Initialize libc forwarding system
    int result = libc_forward_init();
    if (result != 0) {
        fprintf(stderr, "Libc Module Error: Failed to initialize libc forwarding\n");
        return -1;
    }
    
    printf("Libc Module: Initialization completed\n");
    return 0;
}

// Libc module cleanup
void libc_module_cleanup(void) {
    printf("Libc Module: Cleaning up libc_x64_64.native\n");
    libc_forward_cleanup();
}

// Forward libc function call
int libc_module_call(const char* function_name, void* args, void* result) {
    if (!function_name) {
        return -1;
    }
    
    // Create LibcCall structure for forwarding
    LibcCall call = {0};
    strncpy(call.function_name, function_name, sizeof(call.function_name) - 1);
    
    // Forward the call to system libc
    return libc_forward_call(&call);
}

// Get function pointer by name
void* libc_module_get_function(const char* function_name) {
    if (!function_name) {
        return NULL;
    }
    
    // Map function names to actual libc functions
    if (strcmp(function_name, "printf") == 0) {
        return (void*)printf;
    } else if (strcmp(function_name, "malloc") == 0) {
        return (void*)malloc;
    } else if (strcmp(function_name, "free") == 0) {
        return (void*)free;
    } else if (strcmp(function_name, "strlen") == 0) {
        return (void*)strlen;
    } else if (strcmp(function_name, "strcpy") == 0) {
        return (void*)strcpy;
    } else if (strcmp(function_name, "strcat") == 0) {
        return (void*)strcat;
    } else if (strcmp(function_name, "strcmp") == 0) {
        return (void*)strcmp;
    } else if (strcmp(function_name, "memcpy") == 0) {
        return (void*)memcpy;
    } else if (strcmp(function_name, "memset") == 0) {
        return (void*)memset;
    } else if (strcmp(function_name, "fopen") == 0) {
        return (void*)fopen;
    } else if (strcmp(function_name, "fclose") == 0) {
        return (void*)fclose;
    } else if (strcmp(function_name, "fread") == 0) {
        return (void*)fread;
    } else if (strcmp(function_name, "fwrite") == 0) {
        return (void*)fwrite;
    } else if (strcmp(function_name, "exit") == 0) {
        return (void*)exit;
    }
    
    // Function not found
    return NULL;
}

// Get module information
const LibcModuleInfo* libc_module_get_info(void) {
    return &libc_info;
}

// List available functions
int libc_module_list_functions(LibcFunction** functions) {
    static LibcFunction libc_functions[] = {
        {"printf", (void*)printf, "int printf(const char* format, ...)"},
        {"malloc", (void*)malloc, "void* malloc(size_t size)"},
        {"free", (void*)free, "void free(void* ptr)"},
        {"strlen", (void*)strlen, "size_t strlen(const char* str)"},
        {"strcpy", (void*)strcpy, "char* strcpy(char* dest, const char* src)"},
        {"strcat", (void*)strcat, "char* strcat(char* dest, const char* src)"},
        {"strcmp", (void*)strcmp, "int strcmp(const char* str1, const char* str2)"},
        {"memcpy", (void*)memcpy, "void* memcpy(void* dest, const void* src, size_t n)"},
        {"memset", (void*)memset, "void* memset(void* ptr, int value, size_t n)"},
        {"fopen", (void*)fopen, "FILE* fopen(const char* filename, const char* mode)"},
        {"fclose", (void*)fclose, "int fclose(FILE* stream)"},
        {"fread", (void*)fread, "size_t fread(void* ptr, size_t size, size_t count, FILE* stream)"},
        {"fwrite", (void*)fwrite, "size_t fwrite(const void* ptr, size_t size, size_t count, FILE* stream)"},
        {"exit", (void*)exit, "void exit(int status)"},
        {NULL, NULL, NULL} // Terminator
    };
    
    *functions = libc_functions;
    
    // Count functions
    int count = 0;
    while (libc_functions[count].name != NULL) {
        count++;
    }
    
    libc_info.function_count = count;
    return count;
}

// ===============================================
// Module Entry Points (PRD.md compliant)
// ===============================================

// Main entry point for libc_x64_64.native module
int libc_native_main(int argc, char* argv[]) {
    printf("Libc Module: libc_x64_64.native standalone mode\n");
    
    // Initialize libc module
    int init_result = libc_module_init();
    if (init_result != 0) {
        fprintf(stderr, "Libc Module Error: Initialization failed\n");
        return -1;
    }
    
    // List available functions
    LibcFunction* functions;
    int function_count = libc_module_list_functions(&functions);
    
    printf("Available libc functions (%d total):\n", function_count);
    for (int i = 0; i < function_count; i++) {
        printf("  %s - %s\n", functions[i].name, functions[i].signature);
    }
    
    // Cleanup
    libc_module_cleanup();
    
    return 0;
}

// Export module interface for VM
typedef struct {
    int (*init)(void);
    void (*cleanup)(void);
    int (*call)(const char* function_name, void* args, void* result);
    void* (*get_function)(const char* function_name);
    const LibcModuleInfo* (*get_info)(void);
    int (*list_functions)(LibcFunction** functions);
} LibcModuleInterface;

static LibcModuleInterface libc_interface = {
    .init = libc_module_init,
    .cleanup = libc_module_cleanup,
    .call = libc_module_call,
    .get_function = libc_module_get_function,
    .get_info = libc_module_get_info,
    .list_functions = libc_module_list_functions
};

// Get libc interface for VM
const LibcModuleInterface* libc_get_interface(void) {
    return &libc_interface;
}

// ===============================================
// Standalone executable entry point
// ===============================================

#ifdef LIBC_STANDALONE
int main(int argc, char* argv[]) {
    return libc_native_main(argc, argv);
}
#endif
