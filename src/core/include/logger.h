#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// 日志级别
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARNING = 2,
    LOG_ERROR = 3,
    LOG_FATAL = 4
} LogLevel;

// 基本日志函数
void log_message(LogLevel level, const char* format, ...);
void log_debug(const char* format, ...);
void log_info(const char* format, ...);
void log_warning(const char* format, ...);
void log_error(const char* format, ...);
void log_fatal(const char* format, ...);

// 日志配置
void log_set_level(LogLevel level);
void log_set_output(FILE* output);

#ifdef __cplusplus
}
#endif

#endif // LOGGER_H