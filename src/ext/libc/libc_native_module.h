/**
 * libc_native_module.h - Enhanced LibC Native Module
 * 
 * Header for comprehensive libc module that can be loaded as a .native module
 */

#ifndef LIBC_NATIVE_MODULE_H
#define LIBC_NATIVE_MODULE_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

// Module information
#define LIBC_MODULE_NAME "libc_x64_64.native"
#define LIBC_MODULE_VERSION "1.0.0"
#define LIBC_MODULE_AUTHOR "Self-Evolve AI"

// Module statistics structure
typedef struct {
    uint64_t function_calls;
    uint64_t malloc_calls;
    uint64_t free_calls;
    uint64_t string_operations;
    uint64_t math_operations;
    uint64_t io_operations;
    size_t total_allocated;
    size_t current_allocated;
} LibcModuleStats;

// Module lifecycle functions

/**
 * Initialize the libc module
 * @return 0 on success, -1 on error
 */
int libc_module_init(void);

/**
 * Cleanup the libc module
 */
void libc_module_cleanup(void);

/**
 * Register all libc functions with the native bridge
 * @return Number of functions registered, -1 on error
 */
int libc_module_register_functions(void);

/**
 * Get module statistics
 * @param stats Pointer to structure to fill with statistics
 */
void libc_module_get_stats(LibcModuleStats* stats);

/**
 * Module entry point (called when module is loaded)
 * @return 0 on success, -1 on error
 */
int libc_module_main(void);

// Enhanced memory management functions

/**
 * Enhanced malloc with tracking
 * @param size Size in bytes to allocate
 * @param file Source file name (for debugging)
 * @param line Source line number (for debugging)
 * @return Pointer to allocated memory, NULL on error
 */
void* libc_malloc_tracked(size_t size, const char* file, int line);

/**
 * Enhanced free with tracking
 * @param ptr Pointer to memory to free
 * @param file Source file name (for debugging)
 * @param line Source line number (for debugging)
 */
void libc_free_tracked(void* ptr, const char* file, int line);

// Convenience macros for tracked memory allocation
#define LIBC_MALLOC(size) libc_malloc_tracked(size, __FILE__, __LINE__)
#define LIBC_FREE(ptr) libc_free_tracked(ptr, __FILE__, __LINE__)

// Standard library function declarations

// Memory functions
void* libc_memcpy(void* dest, const void* src, size_t n);
void* libc_memset(void* s, int c, size_t n);
int libc_memcmp(const void* s1, const void* s2, size_t n);
void* libc_memmove(void* dest, const void* src, size_t n);

// String functions
size_t libc_strlen(const char* s);
char* libc_strcpy(char* dest, const char* src);
char* libc_strncpy(char* dest, const char* src, size_t n);
int libc_strcmp(const char* s1, const char* s2);
int libc_strncmp(const char* s1, const char* s2, size_t n);
char* libc_strcat(char* dest, const char* src);
char* libc_strncat(char* dest, const char* src, size_t n);
char* libc_strchr(const char* s, int c);
char* libc_strrchr(const char* s, int c);

// Math functions
double libc_sin(double x);
double libc_cos(double x);
double libc_tan(double x);
double libc_sqrt(double x);
double libc_pow(double x, double y);
double libc_log(double x);
double libc_exp(double x);

// I/O functions
int libc_printf(const char* format, ...);
int libc_sprintf(char* str, const char* format, ...);
int libc_snprintf(char* str, size_t size, const char* format, ...);

// Additional utility functions

/**
 * Check for memory leaks
 * @return Number of leaked blocks
 */
size_t libc_check_memory_leaks(void);

/**
 * Dump memory allocation statistics
 * @param output_file File to write to (NULL for stdout)
 */
void libc_dump_memory_stats(const char* output_file);

/**
 * Reset module statistics
 */
void libc_reset_stats(void);

/**
 * Enable/disable memory tracking
 * @param enable true to enable, false to disable
 */
void libc_set_memory_tracking(bool enable);

/**
 * Get memory tracking status
 * @return true if enabled, false otherwise
 */
bool libc_is_memory_tracking_enabled(void);

// Error codes
#define LIBC_SUCCESS           0
#define LIBC_ERROR_INVALID     -1
#define LIBC_ERROR_MEMORY      -2
#define LIBC_ERROR_NOT_FOUND   -3
#define LIBC_ERROR_INIT_FAILED -4

// Function export table for .native module
typedef struct {
    const char* name;
    void* function_ptr;
    const char* signature;
} LibcFunctionExport;

/**
 * Get function export table
 * @param count Pointer to store number of exports
 * @return Pointer to export table
 */
const LibcFunctionExport* libc_get_export_table(size_t* count);

/**
 * Get function by name
 * @param name Function name
 * @return Function pointer, NULL if not found
 */
void* libc_get_function(const char* name);

// Module metadata
typedef struct {
    const char* name;
    const char* version;
    const char* author;
    const char* description;
    const char* license;
    uint32_t build_timestamp;
    uint32_t function_count;
} LibcModuleMetadata;

/**
 * Get module metadata
 * @return Pointer to metadata structure
 */
const LibcModuleMetadata* libc_get_module_metadata(void);

#ifdef __cplusplus
}
#endif

#endif // LIBC_NATIVE_MODULE_H
