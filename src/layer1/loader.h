/**
 * loader.h - Unified PRD-Compatible Loader Header (Layer 1)
 * 
 * Header file for the consolidated loader implementation.
 * Defines interfaces and structures for PRD-compliant loading.
 */

#ifndef LOADER_H
#define LOADER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/utsname.h>
#endif

// Architecture detection
typedef enum {
    ARCH_UNKNOWN = 0,
    ARCH_X86_64 = 1,
    ARCH_ARM64 = 2,
    ARCH_X86_32 = 3,
    ARCH_ARM32 = 4
} DetectedArchitecture;

// Loader configuration
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

// Performance statistics
typedef struct {
    clock_t start_time;
    clock_t detection_time;
    clock_t vm_load_time;
    clock_t program_load_time;
    clock_t execution_time;
    clock_t end_time;
} PerformanceStats;

// VM module interface
typedef struct {
    void* handle;                  // Module handle
    const char* module_path;       // Path to module
    DetectedArchitecture arch;     // Architecture
    int (*vm_main)(int argc, char* argv[]);
    const void* (*get_interface)(void);
} LoadedVMModule;

// Function prototypes
DetectedArchitecture detect_architecture(void);
const char* get_architecture_string(DetectedArchitecture arch);
const char* get_vm_module_path(DetectedArchitecture arch);
LoadedVMModule* load_vm_module(DetectedArchitecture arch, const char* override_path);
void unload_vm_module(LoadedVMModule* vm_module);
int execute_program(LoadedVMModule* vm_module, const UnifiedLoaderConfig* config, PerformanceStats* stats);
void print_performance_stats(const PerformanceStats* stats, const UnifiedLoaderConfig* config);
void print_usage(const char* program_name);
int parse_arguments(int argc, char* argv[], UnifiedLoaderConfig* config);

#endif // LOADER_H
