/**
 * loader.c - Simplified PRD-Compatible Loader (Layer 1)
 *
 * Simplified loader implementation for initial PRD.md compliance.
 * This version focuses on basic functionality without complex dependencies.
 *
 * PRD.md Layer 1: loader_{arch}.exe
 * - Cross-platform unified startup
 * - Hardware environment detection
 * - Load corresponding vm_{arch}_{bits}.native
 * - Unified entry point for simplified deployment
 *
 * Architecture: loader_{arch}.exe -> vm_{arch}_{bits}.native -> {program}.astc
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#include <stdbool.h>

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
#endif
#include <stdbool.h>
#include <time.h>
#include <stdarg.h>



#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/utsname.h>
#endif

// #include "../include/native_format.h" // Removed - not needed for basic loader
#include "../core/utils.h"

// ===============================================
// Unified Loader Configuration
// ===============================================

// UnifiedLoaderConfig is now defined in utils.h

// PerformanceStats is now defined in utils.h

// ===============================================
// Architecture Detection (Unified)
// ===============================================

// DetectedArchitecture is now defined in utils.h

// detect_architecture() is now in utils.c

// get_architecture_string() and get_architecture_bits() are now in utils.c

// ===============================================
// VM Module Path Construction
// ===============================================

// construct_vm_module_path() is now in utils.c

// ===============================================
// Unified Error Handling
// ===============================================

// print_error(), print_verbose() and other print functions are now in utils.c

// ===============================================
// Unified Module Loading
// ===============================================

// LoadedVMModule is now defined in src/core/utils.h

// ===============================================
// Function Declarations
// ===============================================

// VM module functions are now in src/core/utils.c

// ===============================================
// Command Line Parsing
// ===============================================

void print_usage(const char* program_name) {
    printf("Self-Evolve AI Unified Loader v1.0 (PRD-Compatible)\n");
    printf("===================================================\n\n");

    printf("Usage: %s [options] [program.astc] [-- program_args...]\n\n", program_name);

    printf("Description:\n");
    printf("  Cross-platform unified launcher that detects hardware environment\n");
    printf("  and loads the appropriate vm_{arch}_{bits}.native module.\n");
    printf("  Implements PRD.md Layer 1 specification.\n\n");

    printf("Options:\n");
    printf("  -v, --verbose         Enable verbose output\n");
    printf("  -d, --debug           Enable debug mode\n");
    printf("  -p, --performance     Show performance statistics\n");
    printf("  -i, --interactive     Start in interactive mode\n");
    printf("  -a, --autonomous      Enable autonomous AI evolution mode\n");
    printf("  -s, --security LEVEL  Set security clearance level (0-3)\n");
    printf("  -m, --vm-module PATH  Override VM module path\n");
    printf("  -c, --config FILE     Load configuration from file\n");
    printf("  -h, --help            Show this help message\n");
    printf("  --                    Separate loader options from program arguments\n\n");

    printf("Examples:\n");
    printf("  %s program.astc                    # Basic execution\n", program_name);
    printf("  %s -v -d program.astc              # Verbose debug mode\n", program_name);
    printf("  %s -a --autonomous program.astc    # Autonomous evolution\n", program_name);
    printf("  %s -i                              # Interactive mode\n", program_name);
    printf("  %s -m custom_vm.native prog.astc   # Custom VM module\n", program_name);

    printf("\nSupported architectures:\n");
    printf("  x64_64.native          # 64-bit x86_64\n");
    printf("  arm64_64.native        # 64-bit ARM64\n");
    printf("  x86_32.native          # 32-bit x86\n");
    printf("  arm32_32.native        # 32-bit ARM\n");

    printf("\nPRD.md Three-Layer Architecture:\n");
    printf("  Layer 1: loader.exe                    # This program\n");
    printf("  Layer 2: vm_{arch}_{bits}.native       # VM module\n");
    printf("  Layer 3: {program}.astc                # ASTC bytecode\n");
}

int parse_arguments(int argc, char* argv[], UnifiedLoaderConfig* config) {
    // Initialize default configuration
    memset(config, 0, sizeof(UnifiedLoaderConfig));
    config->security_level = 1; // Default security level

    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 1; // Exit with success
        }
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            config->verbose_mode = true;
        }
        else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
            config->debug_mode = true;
        }
        else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--performance") == 0) {
            config->performance_stats = true;
        }
        else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interactive") == 0) {
            config->interactive_mode = true;
        }
        else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--autonomous") == 0) {
            config->autonomous_mode = true;
        }
        else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--security") == 0) {
            if (i + 1 >= argc) {
                print_error("Security level option requires a value");
                return -1;
            }
            config->security_level = atoi(argv[++i]);
            if (config->security_level > 3) {
                print_error("Security level must be 0-3");
                return -1;
            }
        }
        else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--vm-module") == 0) {
            if (i + 1 >= argc) {
                print_error("VM module option requires a path");
                return -1;
            }
            config->vm_module_override = argv[++i];
        }
        else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) {
            if (i + 1 >= argc) {
                print_error("Config option requires a file path");
                return -1;
            }
            config->config_file = argv[++i];
        }
        else if (strcmp(argv[i], "--") == 0) {
            // Everything after -- goes to the program
            i++;
            config->program_argc = argc - i;
            config->program_argv = &argv[i];
            print_verbose(config, "Found -- separator, program_argc=%d", config->program_argc);
            for (int j = 0; j < config->program_argc; j++) {
                print_verbose(config, "program_argv[%d] = %s", j, config->program_argv[j]);
            }
            break;
        }
        else if (argv[i][0] == '-') {
            print_error("Unknown option: %s", argv[i]);
            return -1;
        }
        else {
            // First non-option argument is the program file
            if (!config->program_file) {
                config->program_file = argv[i];
            } else {
                print_error("Multiple program files specified");
                return -1;
            }
        }
        i++;
    }

    // Validation
    if (!config->program_file && !config->interactive_mode) {
        print_error("No program file specified. Use -i for interactive mode or -h for help.");
        return -1;
    }

    return 0;
}

// ===============================================
// Main Function
// ===============================================

int main(int argc, char* argv[]) {
    PerformanceStats stats = {0};
    stats.start_time = clock();

    // Parse command line arguments
    UnifiedLoaderConfig config;
    int parse_result = parse_arguments(argc, argv, &config);
    if (parse_result != 0) {
        return parse_result > 0 ? 0 : 1;
    }

    // Print banner
    if (config.verbose_mode || config.debug_mode) {
        printf("Self-Evolve AI Unified Loader v1.0\n");
        printf("==================================\n");
    }

    // Architecture detection
    clock_t detect_start = clock();
    DetectedArchitecture arch = detect_architecture();
    stats.detection_time = clock() - detect_start;

    if (arch == ARCH_UNKNOWN) {
        print_error("Unsupported architecture");
        return 1;
    }

    print_verbose(&config, "Detected architecture: %s (%d-bit)",
                 get_architecture_string(arch), get_architecture_bits(arch));

    // Construct VM module path
    char vm_module_path[256];
    if (construct_vm_module_path(vm_module_path, sizeof(vm_module_path), &config) != 0) {
        return 1;
    }

    print_verbose(&config, "VM module path: %s", vm_module_path);

    // Load VM module
    clock_t vm_load_start = clock();
    LoadedVMModule vm_module = {0};
    if (load_vm_module(vm_module_path, &vm_module, &config) != 0) {
        return 1;
    }
    stats.vm_load_time = clock() - vm_load_start;

    // Execute program
    int execution_result = execute_program(&vm_module, &config, &stats);

    // Cleanup
    unload_vm_module(&vm_module);

    stats.end_time = clock();

    // Performance statistics
    if (config.performance_stats) {
        printf("\n=== Performance Statistics ===\n");
        printf("Architecture detection: %.2f ms\n",
               (double)stats.detection_time * 1000 / CLOCKS_PER_SEC);
        printf("VM module loading:      %.2f ms\n",
               (double)stats.vm_load_time * 1000 / CLOCKS_PER_SEC);
        printf("Program execution:      %.2f ms\n",
               (double)stats.execution_time * 1000 / CLOCKS_PER_SEC);
        printf("Total time:             %.2f ms\n",
               (double)(stats.end_time - stats.start_time) * 1000 / CLOCKS_PER_SEC);
        printf("==============================\n");
    }

    if (execution_result == 0) {
        print_verbose(&config, "Loader completed successfully");
    }

    return execution_result;
}
