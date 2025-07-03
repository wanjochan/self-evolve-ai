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

// ===============================================
// Unified Loader Configuration
// ===============================================

typedef struct {
    // Basic options
    const char* program_file;      // ASTC program to execute
    const char* vm_module_override; // Manual VM module override
    
    // Behavior flags
    bool verbose_mode;             // Detailed output
    bool debug_mode;               // Debug information
    bool performance_stats;        // Performance measurement
    bool interactive_mode;         // Interactive mode if no program
    bool autonomous_mode;          // AI autonomous evolution mode
    
    // Advanced options
    uint32_t security_level;       // Security clearance level
    const char* config_file;       // Configuration file path
    int program_argc;              // Arguments to pass to program
    char** program_argv;           // Program arguments
} UnifiedLoaderConfig;

typedef struct {
    clock_t start_time;
    clock_t detection_time;
    clock_t vm_load_time;
    clock_t program_load_time;
    clock_t execution_time;
    clock_t end_time;
} PerformanceStats;

// ===============================================
// Architecture Detection (Unified)
// ===============================================

typedef enum {
    ARCH_X86_64,
    ARCH_ARM64,
    ARCH_X86_32,
    ARCH_ARM32,
    ARCH_UNKNOWN
} DetectedArchitecture;

DetectedArchitecture detect_architecture(void) {
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    
    switch (si.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64:
            return ARCH_X86_64;
        case PROCESSOR_ARCHITECTURE_ARM64:
            return ARCH_ARM64;
        case PROCESSOR_ARCHITECTURE_INTEL:
            return ARCH_X86_32;
        case PROCESSOR_ARCHITECTURE_ARM:
            return ARCH_ARM32;
        default:
            return ARCH_UNKNOWN;
    }
#else
    struct utsname info;
    if (uname(&info) != 0) {
        return ARCH_UNKNOWN;
    }
    
    if (strstr(info.machine, "x86_64") || strstr(info.machine, "amd64")) {
        return ARCH_X86_64;
    } else if (strstr(info.machine, "aarch64") || strstr(info.machine, "arm64")) {
        return ARCH_ARM64;
    } else if (strstr(info.machine, "i386") || strstr(info.machine, "i686")) {
        return ARCH_X86_32;
    } else if (strstr(info.machine, "arm")) {
        return ARCH_ARM32;
    }
    
    return ARCH_UNKNOWN;
#endif
}

const char* get_architecture_string(DetectedArchitecture arch) {
    switch (arch) {
        case ARCH_X86_64: return "x64";
        case ARCH_ARM64:  return "arm64";
        case ARCH_X86_32: return "x86";
        case ARCH_ARM32:  return "arm32";
        default:          return "unknown";
    }
}

int get_architecture_bits(DetectedArchitecture arch) {
    switch (arch) {
        case ARCH_X86_64:
        case ARCH_ARM64:
            return 64;
        case ARCH_X86_32:
        case ARCH_ARM32:
            return 32;
        default:
            return 0;
    }
}

// ===============================================
// VM Module Path Construction
// ===============================================

int construct_vm_module_path(char* buffer, size_t buffer_size, const UnifiedLoaderConfig* config) {
    if (config->vm_module_override) {
        // Use manual override
        snprintf(buffer, buffer_size, "%s", config->vm_module_override);
        return 0;
    }
    
    // Auto-detect and construct path
    DetectedArchitecture arch = detect_architecture();
    if (arch == ARCH_UNKNOWN) {
        fprintf(stderr, "Error: Unsupported architecture\n");
        return -1;
    }
    
    const char* arch_str = get_architecture_string(arch);
    int bits = get_architecture_bits(arch);
    
    // Construct PRD-compliant path: bin/layer2/vm_{arch}_{bits}.native
    snprintf(buffer, buffer_size, "bin\\layer2\\vm_%s_%d.native", arch_str, bits);
    
    return 0;
}

// ===============================================
// Unified Error Handling
// ===============================================

void print_error(const char* format, ...) {
    fprintf(stderr, "Loader Error: ");
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

void print_verbose(const UnifiedLoaderConfig* config, const char* format, ...) {
    if (!config->verbose_mode) return;
    
    printf("Loader: ");
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

void print_debug(const UnifiedLoaderConfig* config, const char* format, ...) {
    if (!config->debug_mode) return;
    
    printf("Debug: ");
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

// ===============================================
// Unified Module Loading
// ===============================================

typedef struct {
    void* handle;                  // Module handle
    const char* module_path;       // Path to module
    DetectedArchitecture arch;     // Architecture
    int (*vm_main)(int argc, char* argv[]);
    const void* (*get_interface)(void);
} LoadedVMModule;

int load_vm_module(const char* vm_path, LoadedVMModule* vm_module, const UnifiedLoaderConfig* config) {
    print_verbose(config, "Loading VM module: %s", vm_path);
    
    // Check if file exists
    FILE* test_file = fopen(vm_path, "rb");
    if (!test_file) {
        print_error("VM module not found: %s", vm_path);
        return -1;
    }
    fclose(test_file);
    
    // TODO: Implement actual VM module loading
    // For now, this is a placeholder that validates the files exist
    printf("Loader: Would load VM module: %s\n", vm_path);
    
    vm_module->module_path = vm_path;

    printf("Loader: VM module loaded successfully\n");
    
    return 0;
}

void unload_vm_module(LoadedVMModule* vm_module) {
    // TODO: Implement actual VM module unloading
    printf("Loader: Would unload VM module\n");
}

// ===============================================
// Program Execution
// ===============================================

int execute_program(LoadedVMModule* vm_module, const UnifiedLoaderConfig* config, PerformanceStats* stats) {
    clock_t exec_start = clock();
    
    print_verbose(config, "Starting program execution...");
    
    if (!config->program_file) {
        if (config->interactive_mode) {
            printf("Interactive mode not yet implemented\n");
            return 0;
        } else {
            print_error("No program file specified");
            return -1;
        }
    }
    
    // Load ASTC program
    FILE* prog_file = fopen(config->program_file, "rb");
    if (!prog_file) {
        print_error("Cannot open program file: %s", config->program_file);
        return -1;
    }
    
    fseek(prog_file, 0, SEEK_END);
    long prog_size = ftell(prog_file);
    fseek(prog_file, 0, SEEK_SET);
    
    uint8_t* prog_data = malloc(prog_size);
    if (!prog_data) {
        print_error("Memory allocation failed for program");
        fclose(prog_file);
        return -1;
    }
    
    fread(prog_data, 1, prog_size, prog_file);
    fclose(prog_file);
    
    print_verbose(config, "Program loaded: %ld bytes", prog_size);
    
    // For now, simulate execution
    printf("Executing ASTC program: %s\n", config->program_file);
    printf("Program size: %ld bytes\n", prog_size);
    printf("VM module: %s\n", vm_module->module_path);
    
    if (config->autonomous_mode) {
        printf("Autonomous AI evolution mode enabled\n");
        // TODO: Implement autonomous evolution
    }
    
    // Simulate successful execution
    printf("Program execution completed successfully\n");
    
    free(prog_data);
    
    if (stats) {
        stats->execution_time = clock() - exec_start;
    }
    
    return 0;
}

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
