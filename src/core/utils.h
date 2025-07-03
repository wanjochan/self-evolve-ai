/*
 * utils.h - Header file for utility functions
 * Common utilities used across the self-evolve AI system
 */

#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

// ===============================================
// Architecture Detection Types
// ===============================================

typedef enum {
    ARCH_UNKNOWN = 0,
    ARCH_X86_32,
    ARCH_X86_64,
    ARCH_ARM32,
    ARCH_ARM64
} DetectedArchitecture;

// ===============================================
// Configuration Types
// ===============================================

typedef struct {
    // Basic options
    int verbose_mode;
    int debug_mode;
    int performance_stats;
    int interactive_mode;
    int autonomous_mode;
    int security_level;

    // File paths
    const char* program_file;
    const char* vm_module_override;
    const char* config_file;

    // Program arguments
    int program_argc;
    char** program_argv;
} UnifiedLoaderConfig;

/**
 * Performance statistics structure
 */
typedef struct {
    clock_t start_time;
    clock_t detection_time;
    clock_t vm_load_time;
    clock_t program_load_time;
    clock_t execution_time;
    clock_t end_time;
} PerformanceStats;

/**
 * Loaded VM Module structure for managing native modules
 */
typedef struct {
    void* mapped_memory;           // mmap映射的内存地址
    size_t mapped_size;            // 映射的内存大小
    const char* module_path;       // Path to module
    DetectedArchitecture arch;     // Architecture

    // .native模块的入口点 (从映射内存中解析)
    void* entry_point;             // 模块入口点
    void* code_section;            // 代码段地址
    size_t code_size;              // 代码段大小

    // 执行函数指针 (指向映射内存中的机器码)
    int (*vm_execute)(const char* astc_file, int argc, char* argv[]);
} LoadedVMModule;

// ===============================================
// Architecture Detection Functions
// ===============================================

/**
 * Detect the current system architecture
 * @return DetectedArchitecture enum value
 */
DetectedArchitecture detect_architecture(void);

/**
 * Get string representation of architecture
 * @param arch Architecture enum value
 * @return String representation (e.g., "x64_64", "arm64")
 */
const char* get_architecture_string(DetectedArchitecture arch);

/**
 * Get bit width of architecture
 * @param arch Architecture enum value
 * @return Bit width (32 or 64), or 0 for unknown
 */
int get_architecture_bits(DetectedArchitecture arch);

// ===============================================
// Path Construction Functions
// ===============================================

/**
 * Construct VM module path based on architecture and configuration
 * @param buffer Output buffer for the path
 * @param buffer_size Size of the output buffer
 * @param config Loader configuration
 * @return 0 on success, -1 on error
 */
int construct_vm_module_path(char* buffer, size_t buffer_size, const UnifiedLoaderConfig* config);

// ===============================================
// Logging and Error Handling Functions
// ===============================================

/**
 * Print error message to stderr
 * @param format Printf-style format string
 * @param ... Variable arguments
 */
void print_error(const char* format, ...);

/**
 * Print verbose message if verbose mode is enabled
 * @param config Loader configuration (checked for verbose_mode)
 * @param format Printf-style format string
 * @param ... Variable arguments
 */
void print_verbose(const UnifiedLoaderConfig* config, const char* format, ...);

/**
 * Print informational message
 * @param format Printf-style format string
 * @param ... Variable arguments
 */
void print_info(const char* format, ...);

/**
 * Print warning message
 * @param format Printf-style format string
 * @param ... Variable arguments
 */
void print_warning(const char* format, ...);

/**
 * Print debug message if debug mode is enabled
 * @param config Loader configuration (checked for debug_mode)
 * @param format Printf-style format string
 * @param ... Variable arguments
 */
void print_debug(const UnifiedLoaderConfig* config, const char* format, ...);

// ===============================================
// Memory Management Functions
// ===============================================

/**
 * Allocate executable memory (cross-platform)
 * @param size Size of memory to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
void* allocate_executable_memory(size_t size);

/**
 * Free executable memory (cross-platform)
 * @param ptr Pointer to memory to free
 * @param size Size of memory (required on some platforms)
 */
void free_executable_memory(void* ptr, size_t size);

// ===============================================
// File Utility Functions
// ===============================================

/**
 * Check if a file exists
 * @param path File path to check
 * @return 1 if file exists, 0 otherwise
 */
int file_exists(const char* path);

/**
 * Get the size of a file
 * @param path File path
 * @return File size in bytes, or -1 on error
 */
long get_file_size(const char* path);

/**
 * Read entire file into a buffer
 * @param path File path to read
 * @param buffer Output pointer to allocated buffer (caller must free)
 * @param size Output size of the file
 * @return 0 on success, -1 on error
 */
int read_file_to_buffer(const char* path, void** buffer, size_t* size);

// ===============================================
// String Utility Functions
// ===============================================

/**
 * Safe string duplication (handles NULL input)
 * @param str String to duplicate
 * @return Duplicated string (caller must free), or NULL
 */
char* safe_strdup(const char* str);

/**
 * Safe snprintf with error checking
 * @param buffer Output buffer
 * @param size Buffer size
 * @param format Printf-style format string
 * @param ... Variable arguments
 * @return Number of characters written, or -1 on error
 */
int safe_snprintf(char* buffer, size_t size, const char* format, ...);

// ===============================================
// VM Module Management Functions
// ===============================================

/**
 * Parse native module format and set up execution entry points
 * @param mapped_memory Pointer to mapped native module memory
 * @param file_size Size of the native module file
 * @param vm_module VM module structure to populate
 * @return 0 on success, -1 on error
 */
int parse_native_module(void* mapped_memory, size_t file_size, LoadedVMModule* vm_module);

/**
 * Load VM module from file path
 * @param vm_path Path to the .native VM module file
 * @param vm_module VM module structure to populate
 * @param config Loader configuration
 * @return 0 on success, -1 on error
 */
int load_vm_module(const char* vm_path, LoadedVMModule* vm_module, const UnifiedLoaderConfig* config);

/**
 * Unload VM module and free resources
 * @param vm_module VM module structure to unload
 */
void unload_vm_module(LoadedVMModule* vm_module);

/**
 * Execute ASTC program via native module
 * @param vm_module Loaded VM module
 * @param astc_file Path to ASTC program file
 * @param argc Number of arguments
 * @param argv Argument array
 * @return Program exit code
 */
int execute_astc_via_native_module(LoadedVMModule* vm_module, const char* astc_file, int argc, char* argv[]);

/**
 * Execute program through the VM module
 * @param vm_module Loaded VM module
 * @param config Loader configuration
 * @param stats Performance statistics (optional)
 * @return Program exit code
 */
int execute_program(LoadedVMModule* vm_module, const UnifiedLoaderConfig* config, PerformanceStats* stats);

#endif // UTILS_H
