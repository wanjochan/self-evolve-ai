/**
 * logger.c - Implementation of Logging and Error Handling System
 */

#include "include/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

// Global logger state
static LoggerConfig g_config = {
    .min_level = LOG_LEVEL_INFO,
    .enable_colors = true,
    .enable_timestamps = true,
    .enable_categories = true,
    .log_to_file = false,
    .log_file_path = {0},
    .log_file = NULL
};

static ErrorContext g_last_error = {0};
static bool g_logger_initialized = false;

// Color codes for console output
static const char* LOG_COLORS[] = {
    "\033[37m",  // TRACE - White
    "\033[36m",  // DEBUG - Cyan
    "\033[32m",  // INFO - Green
    "\033[33m",  // WARN - Yellow
    "\033[31m",  // ERROR - Red
    "\033[35m",  // FATAL - Magenta
    "\033[0m"    // RESET
};

// Log level names
static const char* LOG_LEVEL_NAMES[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

// Category names
static const char* LOG_CATEGORY_NAMES[] = {
    "GENERAL", "LOADER", "COMPILER", "RUNTIME", "MODULE", "AI", "PERF"
};

// Error messages
static const char* ERROR_MESSAGES[] = {
    "Success",
    "Invalid argument",
    "File not found",
    "Memory allocation failed",
    "I/O operation failed",
    "Compilation failed",
    "Module load failed",
    "Symbol not found",
    "Platform unsupported",
    "Checksum mismatch",
    "Version incompatible"
};

int logger_init(void) {
    if (g_logger_initialized) {
        return 0;
    }

    // Initialize default configuration
    g_config.min_level = LOG_LEVEL_INFO;
    g_config.enable_colors = true;
    g_config.enable_timestamps = true;
    g_config.enable_categories = true;
    g_config.log_to_file = false;
    g_config.log_file = NULL;

    // Clear error state
    memset(&g_last_error, 0, sizeof(g_last_error));

    g_logger_initialized = true;
    return 0;
}

void logger_cleanup(void) {
    if (!g_logger_initialized) {
        return;
    }

    if (g_config.log_file) {
        fclose(g_config.log_file);
        g_config.log_file = NULL;
    }

    g_logger_initialized = false;
}

int logger_configure(const LoggerConfig* config) {
    if (!config) {
        return -1;
    }

    g_config = *config;

    // Open log file if needed
    if (g_config.log_to_file && strlen(g_config.log_file_path) > 0) {
        if (g_config.log_file) {
            fclose(g_config.log_file);
        }
        g_config.log_file = fopen(g_config.log_file_path, "a");
        if (!g_config.log_file) {
            return -1;
        }
    }

    return 0;
}

void logger_set_level(LogLevel level) {
    g_config.min_level = level;
}

int logger_set_file(const char* file_path, bool enable) {
    if (!file_path) {
        return -1;
    }

    strncpy(g_config.log_file_path, file_path, sizeof(g_config.log_file_path) - 1);
    g_config.log_file_path[sizeof(g_config.log_file_path) - 1] = '\0';
    g_config.log_to_file = enable;

    if (enable) {
        if (g_config.log_file) {
            fclose(g_config.log_file);
        }
        g_config.log_file = fopen(file_path, "a");
        return g_config.log_file ? 0 : -1;
    } else {
        if (g_config.log_file) {
            fclose(g_config.log_file);
            g_config.log_file = NULL;
        }
        return 0;
    }
}

void logger_log(LogLevel level, LogCategory category, const char* file, int line, 
                const char* function, const char* format, ...) {
    if (!g_logger_initialized || level < g_config.min_level) {
        return;
    }

    // Format timestamp
    char timestamp[64] = {0};
    if (g_config.enable_timestamps) {
        time_t now = time(NULL);
        struct tm* tm_info = localtime(&now);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    }

    // Format message
    char message[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    // Build log line
    char log_line[2048];
    int pos = 0;

    // Add color if enabled
    if (g_config.enable_colors && level < 6) {
        pos += snprintf(log_line + pos, sizeof(log_line) - pos, "%s", LOG_COLORS[level]);
    }

    // Add timestamp
    if (g_config.enable_timestamps) {
        pos += snprintf(log_line + pos, sizeof(log_line) - pos, "[%s] ", timestamp);
    }

    // Add level
    pos += snprintf(log_line + pos, sizeof(log_line) - pos, "[%s] ", LOG_LEVEL_NAMES[level]);

    // Add category
    if (g_config.enable_categories) {
        pos += snprintf(log_line + pos, sizeof(log_line) - pos, "[%s] ", LOG_CATEGORY_NAMES[category]);
    }

    // Add location info for debug/trace
    if (level <= LOG_LEVEL_DEBUG) {
        const char* filename = strrchr(file, '/');
        if (!filename) filename = strrchr(file, '\\');
        if (!filename) filename = file;
        else filename++;
        
        pos += snprintf(log_line + pos, sizeof(log_line) - pos, "%s:%d:%s() ", 
                       filename, line, function);
    }

    // Add message
    pos += snprintf(log_line + pos, sizeof(log_line) - pos, "%s", message);

    // Reset color
    if (g_config.enable_colors && level < 6) {
        pos += snprintf(log_line + pos, sizeof(log_line) - pos, "%s", LOG_COLORS[6]);
    }

    // Add newline
    pos += snprintf(log_line + pos, sizeof(log_line) - pos, "\n");

    // Output to console
    fprintf(stderr, "%s", log_line);

    // Output to file if enabled
    if (g_config.log_to_file && g_config.log_file) {
        // Remove color codes for file output
        char file_line[2048];
        int file_pos = 0;
        
        if (g_config.enable_timestamps) {
            file_pos += snprintf(file_line + file_pos, sizeof(file_line) - file_pos, "[%s] ", timestamp);
        }
        file_pos += snprintf(file_line + file_pos, sizeof(file_line) - file_pos, "[%s] ", LOG_LEVEL_NAMES[level]);
        if (g_config.enable_categories) {
            file_pos += snprintf(file_line + file_pos, sizeof(file_line) - file_pos, "[%s] ", LOG_CATEGORY_NAMES[category]);
        }
        if (level <= LOG_LEVEL_DEBUG) {
            const char* filename = strrchr(file, '/');
            if (!filename) filename = strrchr(file, '\\');
            if (!filename) filename = file;
            else filename++;
            file_pos += snprintf(file_line + file_pos, sizeof(file_line) - file_pos, "%s:%d:%s() ", 
                               filename, line, function);
        }
        file_pos += snprintf(file_line + file_pos, sizeof(file_line) - file_pos, "%s\n", message);
        
        fprintf(g_config.log_file, "%s", file_line);
        fflush(g_config.log_file);
    }
}

void logger_set_error(ErrorCode code, const char* file, int line,
                      const char* function, const char* format, ...) {
    g_last_error.code = code;
    g_last_error.timestamp = time(NULL);

    // Copy location info
    strncpy(g_last_error.file, file, sizeof(g_last_error.file) - 1);
    g_last_error.file[sizeof(g_last_error.file) - 1] = '\0';
    g_last_error.line = line;
    strncpy(g_last_error.function, function, sizeof(g_last_error.function) - 1);
    g_last_error.function[sizeof(g_last_error.function) - 1] = '\0';

    // Format error message
    va_list args;
    va_start(args, format);
    vsnprintf(g_last_error.message, sizeof(g_last_error.message), format, args);
    va_end(args);

    // Also log the error
    logger_log(LOG_LEVEL_ERROR, LOG_CAT_GENERAL, file, line, function,
               "Error %d: %s", code, g_last_error.message);
}

const ErrorContext* logger_get_last_error(void) {
    return &g_last_error;
}

void logger_clear_error(void) {
    memset(&g_last_error, 0, sizeof(g_last_error));
}

const char* logger_get_error_message(ErrorCode code) {
    if (code >= 0 && code < sizeof(ERROR_MESSAGES) / sizeof(ERROR_MESSAGES[0])) {
        return ERROR_MESSAGES[code];
    }
    return "Unknown error";
}
