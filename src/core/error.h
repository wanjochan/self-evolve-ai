/**
 * error.h - Core Error Handling System
 * 
 * Unified error reporting and handling for the ASTC system.
 * Provides structured error codes, messages, and debugging support.
 */

#ifndef ERROR_H
#define ERROR_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// Error Codes
// ===============================================

typedef enum {
    ERROR_SUCCESS = 0,
    
    // General errors (1-99)
    ERROR_INVALID_ARGUMENT = 1,
    ERROR_OUT_OF_MEMORY = 2,
    ERROR_FILE_NOT_FOUND = 3,
    ERROR_PERMISSION_DENIED = 4,
    ERROR_OPERATION_FAILED = 5,
    
    // ASTC errors (100-199)
    ERROR_ASTC_INVALID_FORMAT = 100,
    ERROR_ASTC_PARSE_FAILED = 101,
    ERROR_ASTC_COMPILE_FAILED = 102,
    ERROR_ASTC_EXECUTION_FAILED = 103,
    ERROR_ASTC_UNSUPPORTED_VERSION = 104,
    
    // JIT errors (200-299)
    ERROR_JIT_NOT_AVAILABLE = 200,
    ERROR_JIT_COMPILE_FAILED = 201,
    ERROR_JIT_UNSUPPORTED_ARCH = 202,
    ERROR_JIT_CACHE_FULL = 203,
    
    // Module errors (300-399)
    ERROR_MODULE_NOT_FOUND = 300,
    ERROR_MODULE_LOAD_FAILED = 301,
    ERROR_MODULE_INVALID_FORMAT = 302,
    ERROR_MODULE_SYMBOL_NOT_FOUND = 303,
    ERROR_MODULE_INIT_FAILED = 304,
    
    // VM errors (400-499)
    ERROR_VM_INIT_FAILED = 400,
    ERROR_VM_INVALID_BYTECODE = 401,
    ERROR_VM_STACK_OVERFLOW = 402,
    ERROR_VM_INVALID_INSTRUCTION = 403,
    ERROR_VM_RUNTIME_ERROR = 404,
    
    // System errors (500-599)
    ERROR_SYSTEM_INIT_FAILED = 500,
    ERROR_SYSTEM_RESOURCE_EXHAUSTED = 501,
    ERROR_SYSTEM_PLATFORM_UNSUPPORTED = 502,
    
    ERROR_COUNT
} ErrorCode;

// ===============================================
// Error Severity Levels
// ===============================================

typedef enum {
    ERROR_SEVERITY_INFO,
    ERROR_SEVERITY_WARNING,
    ERROR_SEVERITY_ERROR,
    ERROR_SEVERITY_FATAL
} ErrorSeverity;

// ===============================================
// Error Context
// ===============================================

typedef struct {
    ErrorCode code;
    ErrorSeverity severity;
    char message[512];
    char file[256];
    int line;
    char function[128];
    uint64_t timestamp;
} ErrorContext;

// ===============================================
// Error Handling Functions
// ===============================================

/**
 * Initialize error handling system
 */
int error_init(void);

/**
 * Cleanup error handling system
 */
void error_cleanup(void);

/**
 * Set error with context
 */
void error_set(ErrorCode code, ErrorSeverity severity, 
               const char* file, int line, const char* function,
               const char* format, ...);

/**
 * Get last error
 */
const ErrorContext* error_get_last(void);

/**
 * Clear last error
 */
void error_clear(void);

/**
 * Check if there's an error
 */
bool error_has_error(void);

/**
 * Get error message for code
 */
const char* error_get_message(ErrorCode code);

/**
 * Print error to stderr
 */
void error_print(const ErrorContext* error);

/**
 * Set error callback function
 */
typedef void (*ErrorCallback)(const ErrorContext* error);
void error_set_callback(ErrorCallback callback);

/**
 * Enable/disable error logging
 */
void error_set_logging(bool enabled);

// ===============================================
// Convenience Macros
// ===============================================

#define ERROR_SET(code, severity, format, ...) \
    error_set(code, severity, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define ERROR_SET_INFO(code, format, ...) \
    ERROR_SET(code, ERROR_SEVERITY_INFO, format, ##__VA_ARGS__)

#define ERROR_SET_WARNING(code, format, ...) \
    ERROR_SET(code, ERROR_SEVERITY_WARNING, format, ##__VA_ARGS__)

#define ERROR_SET_ERROR(code, format, ...) \
    ERROR_SET(code, ERROR_SEVERITY_ERROR, format, ##__VA_ARGS__)

#define ERROR_SET_FATAL(code, format, ...) \
    ERROR_SET(code, ERROR_SEVERITY_FATAL, format, ##__VA_ARGS__)

#define ERROR_RETURN_IF(condition, code, format, ...) \
    do { \
        if (condition) { \
            ERROR_SET_ERROR(code, format, ##__VA_ARGS__); \
            return -1; \
        } \
    } while(0)

#define ERROR_RETURN_NULL_IF(condition, code, format, ...) \
    do { \
        if (condition) { \
            ERROR_SET_ERROR(code, format, ##__VA_ARGS__); \
            return NULL; \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif // ERROR_H
