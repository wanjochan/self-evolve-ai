/**
 * astc_module.c - ASTC Module with Forwarding Calls
 * 
 * Provides unified interface for ASTC compilation and native code generation.
 * This module acts as a bridge between the loader and the core compiler functions.
 * Follows PRD.md Layer 2 specification.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// Include the core compiler headers
#include "../c2astc.h"
#include "../astc2native.h"
#include "../utils.h"

// ===============================================
// ASTC Module Interface
// ===============================================

/**
 * Convert C source file to ASTC bytecode
 * This is a forwarding call to the core c2astc functionality
 * 
 * @param c_file_path Path to the C source file
 * @param astc_file_path Path for the output ASTC file
 * @param options Compilation options (can be NULL for defaults)
 * @return 0 on success, -1 on error
 */
int c2astc(const char* c_file_path, const char* astc_file_path, const C2AstcOptions* options) {
    if (!c_file_path || !astc_file_path) {
        print_error("ASTC Module: Invalid file paths provided");
        return -1;
    }
    
    printf("ASTC Module: Converting C to ASTC: %s -> %s\n", c_file_path, astc_file_path);
    
    // Use default options if none provided
    C2AstcOptions default_opts;
    if (!options) {
        default_opts = c2astc_default_options();
        options = &default_opts;
    }
    
    // Convert C source to AST
    struct ASTNode* ast = c2astc_convert_file(c_file_path, options);
    if (!ast) {
        const char* error = c2astc_get_error();
        print_error("ASTC Module: C to AST conversion failed: %s", error ? error : "Unknown error");
        return -1;
    }
    
    // Convert AST to ASTC bytecode
    size_t bytecode_size;
    unsigned char* bytecode = ast_to_astc_bytecode_with_options(ast, options, &bytecode_size);
    if (!bytecode) {
        print_error("ASTC Module: AST to ASTC bytecode conversion failed");
        c2astc_free(ast);
        return -1;
    }
    
    // Write ASTC bytecode to file
    FILE* output_file = fopen(astc_file_path, "wb");
    if (!output_file) {
        print_error("ASTC Module: Cannot create output file: %s", astc_file_path);
        c2astc_free(bytecode);
        c2astc_free(ast);
        return -1;
    }
    
    size_t written = fwrite(bytecode, 1, bytecode_size, output_file);
    fclose(output_file);
    
    if (written != bytecode_size) {
        print_error("ASTC Module: Failed to write complete ASTC file");
        c2astc_free(bytecode);
        c2astc_free(ast);
        return -1;
    }
    
    printf("ASTC Module: Successfully generated ASTC file (%zu bytes)\n", bytecode_size);
    
    // Cleanup
    c2astc_free(bytecode);
    c2astc_free(ast);
    
    return 0;
}

/**
 * Convert ASTC bytecode to native executable
 * This is a forwarding call to the core astc2native functionality
 * 
 * @param astc_file_path Path to the ASTC bytecode file
 * @param native_file_path Path for the output native file
 * @param target_arch Target architecture (can be NULL for auto-detect)
 * @return 0 on success, -1 on error
 */
int astc2native(const char* astc_file_path, const char* native_file_path, const char* target_arch) {
    if (!astc_file_path || !native_file_path) {
        print_error("ASTC Module: Invalid file paths provided");
        return -1;
    }
    
    printf("ASTC Module: Converting ASTC to native: %s -> %s\n", astc_file_path, native_file_path);
    
    // Determine target architecture
    TargetArch arch = ARCH_UNKNOWN;
    if (target_arch) {
        arch = parse_target_architecture(target_arch);
        if (arch == ARCH_UNKNOWN) {
            print_warning("ASTC Module: Unknown target architecture '%s', using auto-detect", target_arch);
        }
    }
    
    if (arch == ARCH_UNKNOWN) {
        arch = detect_runtime_architecture();
        printf("ASTC Module: Auto-detected architecture: %s\n", get_architecture_name(arch));
    }
    
    if (!is_architecture_supported(arch)) {
        print_error("ASTC Module: Unsupported target architecture: %s", get_architecture_name(arch));
        return -1;
    }
    
    // Forward the call to the core astc2native function
    int result = compile_astc_to_runtime_bin(astc_file_path, native_file_path);
    
    if (result == 0) {
        printf("ASTC Module: Successfully generated native file: %s\n", native_file_path);
    } else {
        print_error("ASTC Module: Failed to generate native file");
    }
    
    return result;
}

/**
 * Direct C to native compilation (combines c2astc and astc2native)
 * 
 * @param c_file_path Path to the C source file
 * @param native_file_path Path for the output native file
 * @param options Compilation options (can be NULL for defaults)
 * @param target_arch Target architecture (can be NULL for auto-detect)
 * @return 0 on success, -1 on error
 */
int c2native(const char* c_file_path, const char* native_file_path, const C2AstcOptions* options, const char* target_arch) {
    if (!c_file_path || !native_file_path) {
        print_error("ASTC Module: Invalid file paths provided");
        return -1;
    }
    
    printf("ASTC Module: Direct C to native compilation: %s -> %s\n", c_file_path, native_file_path);
    
    // Create temporary ASTC file
    char temp_astc_path[512];
    safe_snprintf(temp_astc_path, sizeof(temp_astc_path), "%s.tmp.astc", native_file_path);
    
    // Step 1: C to ASTC
    if (c2astc(c_file_path, temp_astc_path, options) != 0) {
        return -1;
    }
    
    // Step 2: ASTC to native
    int result = astc2native(temp_astc_path, native_file_path, target_arch);
    
    // Cleanup temporary file
    remove(temp_astc_path);
    
    if (result == 0) {
        printf("ASTC Module: Direct compilation completed successfully\n");
    }
    
    return result;
}

/**
 * Get ASTC module version information
 */
void astc_module_print_version(void) {
    printf("ASTC Module v1.0 - Unified ASTC Compilation Interface\n");
    printf("Components:\n");
    printf("  - C to ASTC Compiler\n");
    printf("  - ASTC to Native Compiler\n");
    printf("  - Direct C to Native Pipeline\n");
    
    // Print component versions
    c2astc_print_version();
}

/**
 * Initialize ASTC module
 * @return 0 on success, -1 on error
 */
int astc_module_init(void) {
    printf("ASTC Module: Initializing compilation infrastructure\n");
    
    // Check if core components are available
    // This is a basic check - in a real implementation we might do more validation
    
    printf("ASTC Module: Initialization completed\n");
    return 0;
}

/**
 * Cleanup ASTC module resources
 */
void astc_module_cleanup(void) {
    printf("ASTC Module: Cleaning up resources\n");
    // Any cleanup needed for the module
}

// ===============================================
// ASTC Module Interface Structure
// ===============================================

typedef struct {
    int (*init)(void);
    void (*cleanup)(void);
    int (*c2astc)(const char* c_file, const char* astc_file, const C2AstcOptions* options);
    int (*astc2native)(const char* astc_file, const char* native_file, const char* target_arch);
    int (*c2native)(const char* c_file, const char* native_file, const C2AstcOptions* options, const char* target_arch);
    void (*print_version)(void);
} AstcModuleInterface;

static AstcModuleInterface astc_module_interface = {
    .init = astc_module_init,
    .cleanup = astc_module_cleanup,
    .c2astc = c2astc,
    .astc2native = astc2native,
    .c2native = c2native,
    .print_version = astc_module_print_version
};

/**
 * Get the ASTC module interface
 * @return Pointer to the module interface
 */
const AstcModuleInterface* get_astc_module_interface(void) {
    return &astc_module_interface;
}
