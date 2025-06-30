/**
 * vm_x64_64_native.c - VM Core Module for x64 64-bit Architecture
 * 
 * This implements the VM core module according to PRD.md requirements:
 * - ASTC virtual machine execution
 * - JIT compilation to x64 machine code
 * - Memory management
 * - Module loading interface for .native modules
 * 
 * File naming follows PRD.md convention: vm_{arch}_{bits}.native
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Include existing VM components
#include "runtime/core_runtime.h"
#include "runtime/core_astc.h"
#include "runtime/vm_astc.c"
#include "runtime/enhanced_jit_compiler.c"
#include "runtime/codegen_x64.c"

// ===============================================
// VM Core Module Interface (PRD.md compliant)
// ===============================================

typedef struct {
    const char* name;
    const char* version;
    const char* arch;
    int bits;
} VMModuleInfo;

typedef struct {
    void* (*load_native_module)(const char* module_path);
    int (*unload_native_module)(void* module_handle);
    void* (*get_module_function)(void* module_handle, const char* function_name);
} VMModuleLoader;

// ===============================================
// VM Core Module Implementation
// ===============================================

static VMModuleInfo vm_info = {
    .name = "vm_core",
    .version = "1.0.0",
    .arch = "x64",
    .bits = 64
};

static VMModuleLoader module_loader = {0};

// VM Core initialization
int vm_core_init(void) {
    printf("VM Core Module: Initializing vm_x64_64.native\n");
    printf("Architecture: %s %d-bit\n", vm_info.arch, vm_info.bits);
    
    // Initialize JIT compiler for x64
    // Initialize memory management
    // Initialize module loading system
    
    return 0;
}

// VM Core cleanup
void vm_core_cleanup(void) {
    printf("VM Core Module: Cleaning up vm_x64_64.native\n");
}

// Execute ASTC program
int vm_core_execute_astc(const char* astc_file, int argc, char* argv[]) {
    printf("VM Core: Executing ASTC file: %s\n", astc_file);
    
    // Load ASTC file
    FILE* file = fopen(astc_file, "rb");
    if (!file) {
        fprintf(stderr, "VM Core Error: Cannot open ASTC file: %s\n", astc_file);
        return -1;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Read ASTC data
    uint8_t* astc_data = malloc(file_size);
    if (!astc_data) {
        fprintf(stderr, "VM Core Error: Memory allocation failed\n");
        fclose(file);
        return -1;
    }
    
    size_t bytes_read = fread(astc_data, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != file_size) {
        fprintf(stderr, "VM Core Error: Failed to read ASTC file completely\n");
        free(astc_data);
        return -1;
    }
    
    printf("VM Core: Loaded %zu bytes of ASTC bytecode\n", file_size);
    
    // Create VM instance
    RuntimeVM* vm = runtime_create();
    if (!vm) {
        fprintf(stderr, "VM Core Error: Failed to create VM instance\n");
        free(astc_data);
        return -1;
    }
    
    // Load ASTC program into VM
    int load_result = runtime_load_astc(vm, astc_data, file_size);
    if (load_result != 0) {
        fprintf(stderr, "VM Core Error: Failed to load ASTC program\n");
        runtime_destroy(vm);
        free(astc_data);
        return -1;
    }
    
    // Execute main function
    printf("VM Core: Starting ASTC program execution\n");
    int exec_result = runtime_execute_main(vm, argc, argv);
    
    printf("VM Core: Program execution completed with result: %d\n", exec_result);
    
    // Cleanup
    runtime_destroy(vm);
    free(astc_data);
    
    return exec_result;
}

// Load native module (for libc_x64_64.native etc.)
void* vm_core_load_native_module(const char* module_path) {
    printf("VM Core: Loading native module: %s\n", module_path);
    
    // TODO: Implement dynamic loading of .native modules
    // This will be used to load libc_x64_64.native and other modules
    
    return NULL; // Placeholder
}

// Get module information
const VMModuleInfo* vm_core_get_info(void) {
    return &vm_info;
}

// ===============================================
// Module Entry Points (PRD.md compliant)
// ===============================================

// Main entry point for vm_x64_64.native module
int vm_native_main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: vm_x64_64.native <astc_file> [args...]\n");
        return -1;
    }
    
    // Initialize VM core
    int init_result = vm_core_init();
    if (init_result != 0) {
        fprintf(stderr, "VM Core Error: Initialization failed\n");
        return -1;
    }
    
    // Execute ASTC program
    const char* astc_file = argv[1];
    int exec_result = vm_core_execute_astc(astc_file, argc - 1, argv + 1);
    
    // Cleanup
    vm_core_cleanup();
    
    return exec_result;
}

// Export module interface for loader
typedef struct {
    int (*init)(void);
    void (*cleanup)(void);
    int (*execute_astc)(const char* astc_file, int argc, char* argv[]);
    void* (*load_native_module)(const char* module_path);
    const VMModuleInfo* (*get_info)(void);
} VMCoreInterface;

static VMCoreInterface vm_interface = {
    .init = vm_core_init,
    .cleanup = vm_core_cleanup,
    .execute_astc = vm_core_execute_astc,
    .load_native_module = vm_core_load_native_module,
    .get_info = vm_core_get_info
};

// Get VM interface for loader
const VMCoreInterface* vm_get_interface(void) {
    return &vm_interface;
}

// ===============================================
// Standalone executable entry point
// ===============================================

#ifdef VM_STANDALONE
int main(int argc, char* argv[]) {
    return vm_native_main(argc, argv);
}
#endif
