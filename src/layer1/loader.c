/**
 * loader.c - Enhanced PRD-Compatible Loader (Layer 1)
 *
 * Enhanced loader implementation following PRD.md three-layer architecture.
 * Integrates with the new core module system and VM architecture.
 *
 * PRD.md Layer 1: loader_{arch}.exe
 * - Cross-platform unified startup
 * - Hardware environment detection
 * - Load corresponding vm_{arch}_{bits}.native
 * - Unified entry point for simplified deployment
 * - Integration with core module system
 *
 * Architecture: loader_{arch}.exe -> vm_{arch}_{bits}.native -> {program}.astc
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <stdarg.h>

// Include core system components
#include "../core/utils.h"
#include "../core/native.h"

#ifdef _WIN32
#include <windows.h>
// Define missing constants for older Windows SDKs
#ifndef PROCESSOR_ARCHITECTURE_ARM64
#define PROCESSOR_ARCHITECTURE_ARM64 12
#endif
#ifndef PROCESSOR_ARCHITECTURE_ARM32_ON_WIN64
#define PROCESSOR_ARCHITECTURE_ARM32_ON_WIN64 13
#endif
#else
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

// ===============================================
// Loader Core Interface Definitions
// ===============================================

/**
 * Loader configuration structure
 */
typedef struct {
    DetectedArchitecture target_arch;  // Target architecture
    int target_bits;                   // Architecture bits (32/64)
    char vm_module_path[512];          // Path to VM module
    char program_path[512];            // Path to program to execute
    int argc;                          // Program arguments count
    char** argv;                       // Program arguments
    bool verbose;                      // Verbose output
    bool force_arch;                   // Force specific architecture
} LoaderConfig;

/**
 * Loader interface structure
 */
typedef struct {
    // Lifecycle management
    int (*init)(LoaderConfig* config);
    void (*cleanup)(void);

    // Architecture detection
    DetectedArchitecture (*detect_architecture)(void);
    int (*get_architecture_bits)(DetectedArchitecture arch);
    const char* (*get_architecture_name)(DetectedArchitecture arch);

    // VM module management
    int (*load_vm_module)(const char* vm_path);
    int (*unload_vm_module)(void);
    int (*execute_program)(const char* program_path, int argc, char* argv[]);

    // Command line processing
    int (*parse_command_line)(int argc, char* argv[], LoaderConfig* config);
    void (*print_usage)(const char* program_name);
    void (*print_version)(void);

    // Error handling
    const char* (*get_last_error)(void);
    void (*set_verbose)(bool verbose);
} LoaderInterface;

// ===============================================
// Global Loader State
// ===============================================

static LoaderInterface* g_loader = NULL;
static LoaderConfig g_config = {0};
static NativeModuleHandle* g_vm_module = NULL;
static char g_last_error[512] = {0};
static bool g_verbose = false;



// ===============================================
// Loader Implementation Functions
// ===============================================

/**
 * Set loader error message
 */
static void loader_set_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(g_last_error, sizeof(g_last_error), format, args);
    va_end(args);

    if (g_verbose) {
        printf("Loader Error: %s\n", g_last_error);
    }
}

/**
 * Initialize loader
 */
static int loader_init(LoaderConfig* config) {
    if (!config) {
        loader_set_error("Invalid configuration");
        return -1;
    }

    // Copy configuration
    g_config = *config;
    g_verbose = config->verbose;

    // Initialize core systems
    if (native_module_system_init() != 0) {
        loader_set_error("Failed to initialize native module system");
        return -1;
    }

    if (g_verbose) {
        printf("Loader: Initialized for %s architecture\n",
               get_architecture_name(config->target_arch));
    }

    return 0;
}

/**
 * Cleanup loader
 */
static void loader_cleanup(void) {
    if (g_vm_module) {
        module_unload_native(g_vm_module);
        g_vm_module = NULL;
    }

    native_module_system_cleanup();

    if (g_verbose) {
        printf("Loader: Cleaned up\n");
    }
}

/**
 * Detect current architecture
 */
static DetectedArchitecture loader_detect_architecture(void) {
    return detect_architecture();
}

/**
 * Get architecture bits
 */
static int loader_get_architecture_bits(DetectedArchitecture arch) {
    switch (arch) {
        case ARCH_X86_32:
        case ARCH_ARM32:
            return 32;
        case ARCH_X86_64:
        case ARCH_ARM64:
            return 64;
        default:
            return 0;
    }
}

/**
 * Get architecture name
 */
static const char* loader_get_architecture_name(DetectedArchitecture arch) {
    return get_architecture_name(arch);
}

/**
 * Load VM module
 */
static int loader_load_vm_module(const char* vm_path) {
    if (!vm_path) {
        loader_set_error("Invalid VM module path");
        return -1;
    }

    if (g_verbose) {
        printf("Loader: Loading VM module %s\n", vm_path);
    }

    // Unload existing module if any
    if (g_vm_module) {
        module_unload_native(g_vm_module);
        g_vm_module = NULL;
    }

    // Load new VM module
    g_vm_module = module_open_native(vm_path, NULL, MODULE_FLAG_NONE);
    if (!g_vm_module) {
        loader_set_error("Failed to load VM module: %s", vm_path);
        return -1;
    }

    if (g_verbose) {
        printf("Loader: Successfully loaded VM module\n");
    }

    return 0;
}

/**
 * Unload VM module
 */
static int loader_unload_vm_module(void) {
    if (!g_vm_module) {
        return 0; // Already unloaded
    }

    if (g_verbose) {
        printf("Loader: Unloading VM module\n");
    }

    int result = module_unload_native(g_vm_module);
    g_vm_module = NULL;

    return result;
}

/**
 * Execute program through VM module
 */
static int loader_execute_program(const char* program_path, int argc, char* argv[]) {
    if (!program_path) {
        loader_set_error("Invalid program path");
        return -1;
    }

    if (!g_vm_module) {
        loader_set_error("No VM module loaded");
        return -1;
    }

    if (g_verbose) {
        printf("Loader: Executing program %s with %d arguments\n", program_path, argc);
    }

    // Look up module execution function - try different function names based on module type
    typedef int (*vm_execute_func_t)(const char*, int, char**);
    vm_execute_func_t vm_execute = NULL;

    // Try VM module function first
    vm_execute = (vm_execute_func_t)module_get_symbol_native(g_vm_module, "vm_core_execute_astc");

    if (!vm_execute) {
        // Try generic native_main function
        vm_execute = (vm_execute_func_t)module_get_symbol_native(g_vm_module, "native_main");
    }

    if (!vm_execute) {
        // Try module-specific main function
        vm_execute = (vm_execute_func_t)module_get_symbol_native(g_vm_module, "main");
    }

    if (!vm_execute) {
        loader_set_error("Module does not export any known execution function (vm_core_execute_astc, native_main, or main)");
        return -1;
    }

    // Execute program
    int result = vm_execute(program_path, argc, argv);

    if (g_verbose) {
        printf("Loader: Program execution completed with result %d\n", result);
    }

    return result;
}

/**
 * Parse command line arguments
 */
static int loader_parse_command_line(int argc, char* argv[], LoaderConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize config with defaults
    memset(config, 0, sizeof(LoaderConfig));
    config->target_arch = ARCH_UNKNOWN; // Will auto-detect
    config->target_bits = 0;
    config->verbose = false;
    config->force_arch = false;

    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            config->verbose = true;
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            return 1; // Signal to print help
        }
        else if (strcmp(argv[i], "--version") == 0) {
            return 2; // Signal to print version
        }
        else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--vm-module") == 0) {
            if (i + 1 >= argc) {
                loader_set_error("Option %s requires an argument", argv[i]);
                return -1;
            }
            safe_strncpy(config->vm_module_path, argv[i + 1], sizeof(config->vm_module_path));
            i++; // Skip next argument
        }
        else if (strcmp(argv[i], "--arch") == 0) {
            if (i + 1 >= argc) {
                loader_set_error("Option %s requires an argument", argv[i]);
                return -1;
            }
            // Parse architecture
            const char* arch_str = argv[i + 1];
            if (strcmp(arch_str, "x64") == 0 || strcmp(arch_str, "x86_64") == 0) {
                config->target_arch = ARCH_X86_64;
                config->force_arch = true;
            } else if (strcmp(arch_str, "x86") == 0 || strcmp(arch_str, "x86_32") == 0) {
                config->target_arch = ARCH_X86_32;
                config->force_arch = true;
            } else {
                loader_set_error("Unsupported architecture: %s", arch_str);
                return -1;
            }
            i++; // Skip next argument
        }
        else if (argv[i][0] == '-') {
            loader_set_error("Unknown option: %s", argv[i]);
            return -1;
        }
        else {
            // This should be the program path
            safe_strncpy(config->program_path, argv[i], sizeof(config->program_path));

            // Remaining arguments are for the program
            config->argc = argc - i - 1;
            config->argv = (argc > i + 1) ? &argv[i + 1] : NULL;
            break;
        }
        i++;
    }

    return 0;
}

/**
 * Print usage information
 */
static void loader_print_usage(const char* program_name) {
    printf("Self-Evolve AI Enhanced Loader v2.0 (PRD-Compatible)\n");
    printf("====================================================\n\n");

    printf("Usage: %s [options] [program.astc] [-- program_args...]\n\n", program_name);

    printf("Description:\n");
    printf("  Enhanced cross-platform loader with core module system integration.\n");
    printf("  Detects hardware environment and loads appropriate VM module.\n");
    printf("  Implements PRD.md Layer 1 specification with JIT support.\n\n");

    printf("Options:\n");
    printf("  -v, --verbose         Enable verbose output\n");
    printf("  -m, --vm-module PATH  Override VM module path\n");
    printf("  --arch ARCH           Force specific architecture (x64, x86)\n");
    printf("  -h, --help            Show this help message\n");
    printf("  --version             Show version information\n\n");

    printf("Examples:\n");
    printf("  %s program.astc                    # Basic execution\n", program_name);
    printf("  %s -v program.astc                 # Verbose mode\n", program_name);
    printf("  %s -m custom_vm.native prog.astc   # Custom VM module\n", program_name);
    printf("  %s --arch x64 program.astc         # Force x64 architecture\n", program_name);

    printf("\nSupported architectures: x86_64, x86_32\n");
    printf("VM module format: vm_{arch}_{bits}.native\n");
}

/**
 * Print version information
 */
static void loader_print_version(void) {
    printf("Self-Evolve AI Enhanced Loader v2.0\n");
    printf("Built with core module system integration\n");
    printf("JIT compilation support: Yes\n");
    printf("Supported architectures: x86_64, x86_32\n");

    DetectedArchitecture current_arch = detect_architecture();
    printf("Current architecture: %s (%d-bit)\n",
           get_architecture_name(current_arch),
           loader_get_architecture_bits(current_arch));
}

/**
 * Get last error message
 */
static const char* loader_get_last_error(void) {
    return g_last_error[0] ? g_last_error : NULL;
}

/**
 * Set verbose mode
 */
static void loader_set_verbose(bool verbose) {
    g_verbose = verbose;
}

// ===============================================
// Loader Interface Implementation
// ===============================================

static LoaderInterface g_loader_impl = {
    .init = loader_init,
    .cleanup = loader_cleanup,
    .detect_architecture = loader_detect_architecture,
    .get_architecture_bits = loader_get_architecture_bits,
    .get_architecture_name = loader_get_architecture_name,
    .load_vm_module = loader_load_vm_module,
    .unload_vm_module = loader_unload_vm_module,
    .execute_program = loader_execute_program,
    .parse_command_line = loader_parse_command_line,
    .print_usage = loader_print_usage,
    .print_version = loader_print_version,
    .get_last_error = loader_get_last_error,
    .set_verbose = loader_set_verbose
};

// ===============================================
// Main Function
// ===============================================

/**
 * Main entry point for the enhanced loader
 */
int main(int argc, char* argv[]) {
    LoaderConfig config;
    int parse_result;
    int exit_code = 0;

    // Set global loader interface
    g_loader = &g_loader_impl;

    // Parse command line arguments
    parse_result = g_loader->parse_command_line(argc, argv, &config);

    if (parse_result == 1) {
        // Help requested
        g_loader->print_usage(argv[0]);
        return 0;
    } else if (parse_result == 2) {
        // Version requested
        g_loader->print_version();
        return 0;
    } else if (parse_result < 0) {
        // Parse error
        printf("Error: %s\n", g_loader->get_last_error());
        printf("Use '%s --help' for usage information.\n", argv[0]);
        return 1;
    }

    // Set verbose mode
    g_loader->set_verbose(config.verbose);

    // Auto-detect architecture if not forced
    if (config.target_arch == ARCH_UNKNOWN) {
        config.target_arch = g_loader->detect_architecture();
        config.target_bits = g_loader->get_architecture_bits(config.target_arch);

        if (config.verbose) {
            printf("Loader: Auto-detected architecture: %s (%d-bit)\n",
                   g_loader->get_architecture_name(config.target_arch),
                   config.target_bits);
        }
    }

    // Initialize loader
    if (g_loader->init(&config) != 0) {
        printf("Error: Failed to initialize loader: %s\n", g_loader->get_last_error());
        return 1;
    }

    // Construct VM module path if not provided
    if (config.vm_module_path[0] == '\0') {
        snprintf(config.vm_module_path, sizeof(config.vm_module_path),
                "vm_%s_%d.native",
                g_loader->get_architecture_name(config.target_arch),
                config.target_bits);
    }

    // Load VM module
    if (g_loader->load_vm_module(config.vm_module_path) != 0) {
        printf("Error: Failed to load VM module: %s\n", g_loader->get_last_error());
        exit_code = 1;
        goto cleanup;
    }

    // Execute program if specified
    if (config.program_path[0] != '\0') {
        if (config.verbose) {
            printf("Loader: Executing program %s\n", config.program_path);
        }

        exit_code = g_loader->execute_program(config.program_path, config.argc, config.argv);

        if (exit_code != 0 && config.verbose) {
            printf("Loader: Program execution failed with code %d\n", exit_code);
            const char* error = g_loader->get_last_error();
            if (error) {
                printf("Loader: Error: %s\n", error);
            }
        }
    } else {
        printf("Error: No program specified\n");
        printf("Use '%s --help' for usage information.\n", argv[0]);
        exit_code = 1;
    }

cleanup:
    // Cleanup loader
    g_loader->cleanup();

    if (config.verbose) {
        printf("Loader: Exiting with code %d\n", exit_code);
    }

    return exit_code;
}
