/**
 * test_vm_simple.c - Simple VM for testing .native format
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/core/native.h"
#include "src/core/utils.h"

/**
 * Simple VM execution function
 */
int vm_core_execute_astc(const char* astc_file, int argc, char* argv[]) {
    printf("Simple VM: Executing %s with %d args\n", astc_file, argc);
    
    // Check if file exists
    FILE* file = fopen(astc_file, "rb");
    if (!file) {
        printf("Simple VM Error: Cannot open %s\n", astc_file);
        return 1;
    }
    
    // Check ASTC magic
    char magic[4];
    if (fread(magic, 1, 4, file) == 4 && memcmp(magic, "ASTC", 4) == 0) {
        printf("Simple VM: Valid ASTC file, simulating execution\n");
        fclose(file);
        return 0;
    } else {
        printf("Simple VM Error: Invalid ASTC format\n");
        fclose(file);
        return 1;
    }
}

/**
 * Create a proper .native module with vm_core_execute_astc function
 */
int main() {
    printf("Creating test VM .native module\n");
    
    // Create native module
    NativeModule* module = native_module_create(NATIVE_ARCH_X86_64, NATIVE_TYPE_VM);
    if (!module) {
        printf("Error: Failed to create native module\n");
        return 1;
    }
    
    // Simple code section (just a return instruction for now)
    uint8_t code[] = {0xC3}; // ret instruction
    if (native_module_set_code(module, code, sizeof(code), 0) != 0) {
        printf("Error: Failed to set code section\n");
        native_module_free(module);
        return 1;
    }
    
    // Add vm_core_execute_astc export
    if (native_module_add_export(module, "vm_core_execute_astc", NATIVE_EXPORT_FUNCTION, 0, sizeof(code)) != 0) {
        printf("Error: Failed to add export\n");
        native_module_free(module);
        return 1;
    }
    
    // Write to file
    if (native_module_write_file(module, "bin/layer2/vm_x64_64.native") != 0) {
        printf("Error: Failed to write .native file\n");
        native_module_free(module);
        return 1;
    }
    
    printf("Success: Created bin/layer2/vm_x64_64.native\n");
    
    // Test loading
    NativeModule* loaded = native_module_load_file("bin/layer2/vm_x64_64.native");
    if (loaded) {
        printf("Success: Loaded .native file\n");
        
        void* symbol = native_module_get_symbol(loaded, "vm_core_execute_astc");
        if (symbol) {
            printf("Success: Found vm_core_execute_astc symbol at %p\n", symbol);
        } else {
            printf("Warning: vm_core_execute_astc symbol not found\n");
        }
        
        native_module_free(loaded);
    } else {
        printf("Error: Failed to load .native file\n");
    }
    
    native_module_free(module);
    return 0;
}
