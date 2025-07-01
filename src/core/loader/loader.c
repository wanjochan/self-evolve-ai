/**
 * loader.c - Universal Loader (PRD-compliant)
 * 
 * 跨平台统一启动器，识别硬件环境，加载对应 vm_{arch}_{bits}.native
 * 符合PRD.md Layer 1规范
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations for native format functions
typedef struct LoadedModule LoadedModule;
LoadedModule* load_native_module(const char* filename);
void unload_native_module(LoadedModule* module);
void* get_export_function(LoadedModule* module, const char* name);

// Simplified header structure for compilation
struct {
    uint32_t architecture;
    uint64_t code_size;
    uint64_t data_size;
    uint32_t export_count;
} header;

// ===============================================
// Architecture Detection
// ===============================================

typedef enum {
    ARCH_X86_64,
    ARCH_ARM64,
    ARCH_X86_32,
    ARCH_UNKNOWN
} Architecture;

typedef enum {
    BITS_32,
    BITS_64,
    BITS_UNKNOWN
} BitWidth;

/**
 * Detect current architecture
 */
Architecture detect_architecture(void) {
#if defined(__x86_64__) || defined(_M_X64)
    return ARCH_X86_64;
#elif defined(__aarch64__) || defined(_M_ARM64)
    return ARCH_ARM64;
#elif defined(__i386__) || defined(_M_IX86)
    return ARCH_X86_32;
#else
    return ARCH_UNKNOWN;
#endif
}

/**
 * Detect bit width
 */
BitWidth detect_bit_width(void) {
#if defined(_WIN64) || defined(__LP64__)
    return BITS_64;
#else
    return BITS_32;
#endif
}

/**
 * Get architecture string
 */
const char* get_arch_string(Architecture arch) {
    switch (arch) {
        case ARCH_X86_64: return "x86_64";
        case ARCH_ARM64: return "arm64";
        case ARCH_X86_32: return "x86_32";
        default: return "unknown";
    }
}

/**
 * Get bit width string
 */
const char* get_bits_string(BitWidth bits) {
    switch (bits) {
        case BITS_64: return "64";
        case BITS_32: return "32";
        default: return "unknown";
    }
}

// ===============================================
// VM Module Loading
// ===============================================

/**
 * Construct VM module filename based on detected architecture
 */
int construct_vm_module_path(char* buffer, size_t buffer_size) {
    Architecture arch = detect_architecture();
    BitWidth bits = detect_bit_width();
    
    if (arch == ARCH_UNKNOWN || bits == BITS_UNKNOWN) {
        printf("Error: Unsupported architecture or bit width\n");
        return -1;
    }
    
    const char* arch_str = get_arch_string(arch);
    const char* bits_str = get_bits_string(bits);
    
    // Format: vm_{arch}_{bits}.native
    int result = snprintf(buffer, buffer_size, "vm_%s_%s.native", arch_str, bits_str);
    
    if (result < 0 || result >= buffer_size) {
        printf("Error: VM module path too long\n");
        return -1;
    }
    
    return 0;
}

/**
 * Load and execute VM module
 */
int load_and_execute_vm(const char* vm_module_path, const char* program_path) {
    printf("Loading VM module: %s\n", vm_module_path);
    printf("Program to execute: %s\n", program_path);
    
    // Load the VM module
    LoadedModule* vm_module = load_native_module(vm_module_path);
    if (!vm_module) {
        printf("Error: Failed to load VM module: %s\n", vm_module_path);
        return -1;
    }
    
    printf("VM module loaded successfully:\n");
    printf("  Architecture: detected\n");
    printf("  Module loaded and ready\n");
    
    // Find main function in VM module
    typedef int (*main_func_t)(int argc, char* argv[]);
    main_func_t vm_main = (main_func_t)get_export_function(vm_module, "main");
    
    if (!vm_main) {
        printf("Error: VM module does not export 'main' function\n");
        unload_native_module(vm_module);
        return -1;
    }
    
    // Prepare arguments for VM
    char* vm_argv[] = {
        (char*)vm_module_path,  // argv[0] = VM module path
        (char*)program_path,    // argv[1] = program to execute
        NULL
    };
    int vm_argc = program_path ? 2 : 1;
    
    printf("Executing VM main function...\n");
    
    // Execute VM with the program
    int result = vm_main(vm_argc, vm_argv);
    
    printf("VM execution completed with result: %d\n", result);
    
    // Cleanup
    unload_native_module(vm_module);
    return result;
}

// ===============================================
// Main Loader Logic
// ===============================================

void print_usage(const char* program_name) {
    printf("Universal Loader v1.0 (PRD-compliant)\n");
    printf("Usage: %s [program.astc]\n\n", program_name);
    
    printf("Description:\n");
    printf("  Cross-platform unified launcher that detects hardware environment\n");
    printf("  and loads the appropriate vm_{arch}_{bits}.native module.\n\n");
    
    printf("Examples:\n");
    printf("  %s program.astc        # Load and execute ASTC program\n", program_name);
    printf("  %s                     # Start VM in interactive mode\n", program_name);
    
    printf("\nSupported architectures:\n");
    printf("  x86_64_64.native       # 64-bit x86_64\n");
    printf("  arm64_64.native        # 64-bit ARM64\n");
    printf("  x86_32_32.native       # 32-bit x86\n");
    
    printf("\nArchitecture detection:\n");
    Architecture arch = detect_architecture();
    BitWidth bits = detect_bit_width();
    printf("  Current: %s_%s\n", get_arch_string(arch), get_bits_string(bits));
}

int main(int argc, char* argv[]) {
    // Handle help request
    if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        print_usage(argv[0]);
        return 0;
    }
    
    printf("Universal Loader v1.0 - Self-Evolve AI System\n");
    printf("==============================================\n");
    
    // Detect architecture and construct VM module path
    char vm_module_path[256];
    if (construct_vm_module_path(vm_module_path, sizeof(vm_module_path)) != 0) {
        return 1;
    }
    
    printf("Detected architecture: %s_%s\n", 
           get_arch_string(detect_architecture()), 
           get_bits_string(detect_bit_width()));
    printf("VM module: %s\n", vm_module_path);
    
    // Determine program to execute
    const char* program_path = NULL;
    if (argc > 1) {
        program_path = argv[1];
        printf("Program: %s\n", program_path);
    } else {
        printf("No program specified, starting VM in interactive mode\n");
    }
    
    printf("\n");
    
    // Load and execute VM
    int result = load_and_execute_vm(vm_module_path, program_path);
    
    if (result == 0) {
        printf("\nLoader completed successfully\n");
    } else {
        printf("\nLoader failed with error code: %d\n", result);
    }
    
    return result;
}

// ===============================================
// Platform-specific Implementation Notes
// ===============================================

/*
 * TODO: Future enhancements for cross-platform support
 * 
 * 1. Cosmopolitan-style universal binary:
 *    - Single executable that works on Windows/Linux/macOS
 *    - Runtime detection of OS and architecture
 *    - Embedded VM modules for all supported platforms
 * 
 * 2. Dynamic VM module discovery:
 *    - Search in standard directories (./modules/, /usr/lib/self-evolve/, etc.)
 *    - Version-aware module loading
 *    - Fallback to compatible architectures
 * 
 * 3. Security enhancements:
 *    - VM module signature verification
 *    - Sandboxed execution environment
 *    - Resource limits and monitoring
 * 
 * 4. Performance optimizations:
 *    - VM module caching
 *    - JIT compilation hints
 *    - Memory-mapped module loading
 */
