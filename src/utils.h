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

#endif // UTILS_H
