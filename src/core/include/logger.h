/**
 * logger.h - Comprehensive Logging and Error Handling System
 *
 * Provides structured logging with multiple levels and error handling
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Log levels
typedef enum {
    LOG_LEVEL_TRACE = 0,
    LOG_LEVEL_DEBUG = 1,
    LOG_LEVEL_INFO = 2,
    LOG_LEVEL_WARN = 3,
    LOG_LEVEL_ERROR = 4,
    LOG_LEVEL_FATAL = 5,
    LOG_LEVEL_OFF = 6
} LogLevel;

// Log categories
typedef enum {
    LOG_CAT_GENERAL = 0,
    LOG_CAT_LOADER = 1,
    LOG_CAT_COMPILER = 2,
    LOG_CAT_RUNTIME = 3,
    LOG_CAT_MODULE = 4,
    LOG_CAT_AI = 5,
    LOG_CAT_PERFORMANCE = 6
} LogCategory;

// Error codes
typedef enum {
    ERROR_SUCCESS = 0,
    ERROR_INVALID_ARGUMENT = 1,
    ERROR_FILE_NOT_FOUND = 2,
    ERROR_MEMORY_ALLOCATION = 3,
    ERROR_IO_OPERATION = 4,
    ERROR_COMPILATION_FAILED = 5,
    ERROR_MODULE_LOAD_FAILED = 6,
    ERROR_SYMBOL_NOT_FOUND = 7,
    ERROR_PLATFORM_UNSUPPORTED = 8,
    ERROR_CHECKSUM_MISMATCH = 9,
    ERROR_VERSION_INCOMPATIBLE = 10
} ErrorCode;

// Logger configuration
typedef struct {
    LogLevel min_level;
    bool enable_colors;
    bool enable_timestamps;
    bool enable_categories;
    bool log_to_file;
    char log_file_path[256];
    FILE* log_file;
} LoggerConfig;

// Error context
typedef struct {
    ErrorCode code;
    char message[512];
    char file[128];
    int line;
    char function[128];
    time_t timestamp;
} ErrorContext;

// Logger functions

/**
 * Initialize the logger system
 */
int logger_init(void);

/**
 * Cleanup the logger system
 */
void logger_cleanup(void);

/**
 * Configure the logger
 */
int logger_configure(const LoggerConfig* config);

/**
 * Set minimum log level
 */
void logger_set_level(LogLevel level);

/**
 * Enable/disable file logging
 */
int logger_set_file(const char* file_path, bool enable);

/**
 * Core logging function
 */
void logger_log(LogLevel level, LogCategory category, const char* file, int line, 
                const char* function, const char* format, ...);

/**
 * Set last error
 */
void logger_set_error(ErrorCode code, const char* file, int line, 
                      const char* function, const char* format, ...);

/**
 * Get last error
 */
const ErrorContext* logger_get_last_error(void);

/**
 * Clear last error
 */
void logger_clear_error(void);

/**
 * Get error message for error code
 */
const char* logger_get_error_message(ErrorCode code);

// Convenience macros
#define LOG_TRACE(cat, ...) logger_log(LOG_LEVEL_TRACE, cat, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_DEBUG(cat, ...) logger_log(LOG_LEVEL_DEBUG, cat, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_INFO(cat, ...) logger_log(LOG_LEVEL_INFO, cat, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_WARN(cat, ...) logger_log(LOG_LEVEL_WARN, cat, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_ERROR(cat, ...) logger_log(LOG_LEVEL_ERROR, cat, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_FATAL(cat, ...) logger_log(LOG_LEVEL_FATAL, cat, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define SET_ERROR(code, ...) logger_set_error(code, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define GET_ERROR() logger_get_last_error()
#define CLEAR_ERROR() logger_clear_error()

// Category-specific macros
#define LOG_LOADER_INFO(...) LOG_INFO(LOG_CAT_LOADER, __VA_ARGS__)
#define LOG_LOADER_ERROR(...) LOG_ERROR(LOG_CAT_LOADER, __VA_ARGS__)
#define LOG_COMPILER_INFO(...) LOG_INFO(LOG_CAT_COMPILER, __VA_ARGS__)
#define LOG_COMPILER_ERROR(...) LOG_ERROR(LOG_CAT_COMPILER, __VA_ARGS__)
#define LOG_RUNTIME_INFO(...) LOG_INFO(LOG_CAT_RUNTIME, __VA_ARGS__)
#define LOG_RUNTIME_ERROR(...) LOG_ERROR(LOG_CAT_RUNTIME, __VA_ARGS__)
#define LOG_MODULE_INFO(...) LOG_INFO(LOG_CAT_MODULE, __VA_ARGS__)
#define LOG_MODULE_ERROR(...) LOG_ERROR(LOG_CAT_MODULE, __VA_ARGS__)
#define LOG_AI_INFO(...) LOG_INFO(LOG_CAT_AI, __VA_ARGS__)
#define LOG_AI_ERROR(...) LOG_ERROR(LOG_CAT_AI, __VA_ARGS__)
#define LOG_PERF_INFO(...) LOG_INFO(LOG_CAT_PERFORMANCE, __VA_ARGS__)

// Error handling macros
#define CHECK_NULL(ptr, error_code) \
    do { \
        if (!(ptr)) { \
            SET_ERROR(error_code, "Null pointer: %s", #ptr); \
            return -1; \
        } \
    } while(0)

#define CHECK_RESULT(expr, error_code) \
    do { \
        if ((expr) != 0) { \
            SET_ERROR(error_code, "Operation failed: %s", #expr); \
            return -1; \
        } \
    } while(0)

#define RETURN_ON_ERROR(expr) \
    do { \
        int _result = (expr); \
        if (_result != 0) { \
            return _result; \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif // LOGGER_H
