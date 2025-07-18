#include "enhanced_debug_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>

// 全局调试系统实例
EnhancedDebugSystem g_debug_system = {0};

// 获取高精度时间
static double get_current_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// 获取线程ID
static uint64_t get_thread_id(void) {
    return (uint64_t)pthread_self();
}

// 获取默认配置
DebugConfig enhanced_debug_get_default_config(void) {
    DebugConfig config = {
        .min_level = DEBUG_LEVEL_INFO,
        .enabled_categories = DEBUG_CATEGORY_ALL,
        .format = DEBUG_FORMAT_COLORED,
        .output = DEBUG_OUTPUT_CONSOLE,
        
        .enable_timestamps = true,
        .enable_colors = true,
        .enable_context = true,
        .enable_stack_trace = false,
        .enable_memory_tracking = false,
        .enable_performance_tracking = true,
        
        .log_file = NULL,
        .max_log_size = 10 * 1024 * 1024, // 10MB
        .max_log_files = 5,
        
        .enable_filtering = false,
        .filter_pattern = NULL,
        
        .enable_buffering = true,
        .buffer_size = 8192,
        .flush_interval = 1.0
    };
    return config;
}

// 初始化增强调试系统
int enhanced_debug_init(const DebugConfig* config) {
    if (g_debug_system.is_initialized) {
        return 0; // 已经初始化
    }
    
    // 使用默认配置或提供的配置
    if (config) {
        g_debug_system.config = *config;
    } else {
        g_debug_system.config = enhanced_debug_get_default_config();
    }
    
    // 初始化统计信息
    memset(&g_debug_system.stats, 0, sizeof(DebugStats));
    g_debug_system.stats.start_time = time(NULL);
    
    // 初始化消息链表
    g_debug_system.message_head = NULL;
    g_debug_system.message_tail = NULL;
    g_debug_system.message_count = 0;
    
    // 初始化缓冲区
    if (g_debug_system.config.enable_buffering) {
        g_debug_system.buffer = malloc(g_debug_system.config.buffer_size);
        if (!g_debug_system.buffer) {
            return -1;
        }
        g_debug_system.buffer_pos = 0;
    }
    
    // 打开日志文件
    if (g_debug_system.config.log_file) {
        g_debug_system.log_file = fopen(g_debug_system.config.log_file, "a");
        if (!g_debug_system.log_file) {
            fprintf(stderr, "Warning: Could not open log file: %s\n", g_debug_system.config.log_file);
        }
    }
    
    g_debug_system.is_initialized = true;
    g_debug_system.next_message_id = 1;
    g_debug_system.next_sequence = 1;
    g_debug_system.last_flush_time = get_current_time();
    
    printf("Enhanced Debug System: 初始化完成\n");
    printf("  调试级别: %s\n", enhanced_debug_level_to_string(g_debug_system.config.min_level));
    printf("  输出格式: %s\n", g_debug_system.config.format == DEBUG_FORMAT_COLORED ? "彩色" : "普通");
    printf("  启用时间戳: %s\n", g_debug_system.config.enable_timestamps ? "是" : "否");
    printf("  启用上下文: %s\n", g_debug_system.config.enable_context ? "是" : "否");
    printf("  缓冲大小: %zu 字节\n", g_debug_system.config.buffer_size);
    
    return 0;
}

// 清理增强调试系统
void enhanced_debug_cleanup(void) {
    if (!g_debug_system.is_initialized) {
        return;
    }
    
    // 刷新缓冲区
    enhanced_debug_flush();
    
    // 清理消息链表
    enhanced_debug_clear_messages();
    
    // 关闭日志文件
    if (g_debug_system.log_file) {
        fclose(g_debug_system.log_file);
        g_debug_system.log_file = NULL;
    }
    
    // 释放缓冲区
    if (g_debug_system.buffer) {
        free(g_debug_system.buffer);
        g_debug_system.buffer = NULL;
    }
    
    g_debug_system.is_initialized = false;
    printf("Enhanced Debug System: 清理完成\n");
}

// 检查是否已初始化
bool enhanced_debug_is_initialized(void) {
    return g_debug_system.is_initialized;
}

// 创建调试消息
static DebugMessage* create_debug_message(DebugLevel level, DebugCategory category,
                                        const char* file, int line, const char* function,
                                        const char* message, const char* details, const char* suggestion) {
    DebugMessage* msg = malloc(sizeof(DebugMessage));
    if (!msg) return NULL;
    
    memset(msg, 0, sizeof(DebugMessage));
    
    msg->id = g_debug_system.next_message_id++;
    msg->level = level;
    msg->category = category;
    msg->timestamp = time(NULL);
    
    // 设置上下文
    msg->context.file = file ? strdup(file) : NULL;
    msg->context.line = line;
    msg->context.function = function ? strdup(function) : NULL;
    msg->context.thread_id = get_thread_id();
    msg->context.timestamp = get_current_time();
    msg->context.sequence = g_debug_system.next_sequence++;
    
    // 复制消息内容
    msg->message = message ? strdup(message) : NULL;
    msg->details = details ? strdup(details) : NULL;
    msg->suggestion = suggestion ? strdup(suggestion) : NULL;
    
    return msg;
}

// 释放调试消息
static void free_debug_message(DebugMessage* msg) {
    if (!msg) return;
    
    free((void*)msg->context.file);
    free((void*)msg->context.function);
    free(msg->message);
    free(msg->details);
    free(msg->suggestion);
    free(msg);
}

// 格式化调试消息
static void format_debug_message(const DebugMessage* msg, char* buffer, size_t buffer_size) {
    if (!msg || !buffer) return;
    
    const char* level_str = enhanced_debug_level_to_string(msg->level);
    const char* category_str = enhanced_debug_category_to_string(msg->category);
    
    // 颜色代码
    const char* color_start = "";
    const char* color_end = "";
    
    if (g_debug_system.config.enable_colors && g_debug_system.config.format == DEBUG_FORMAT_COLORED) {
        switch (msg->level) {
            case DEBUG_LEVEL_ERROR:   color_start = "\033[1;31m"; break; // 红色
            case DEBUG_LEVEL_WARNING: color_start = "\033[1;33m"; break; // 黄色
            case DEBUG_LEVEL_INFO:    color_start = "\033[1;32m"; break; // 绿色
            case DEBUG_LEVEL_DEBUG:   color_start = "\033[1;36m"; break; // 青色
            case DEBUG_LEVEL_TRACE:   color_start = "\033[1;37m"; break; // 白色
            default: break;
        }
        color_end = "\033[0m";
    }
    
    // 构建消息
    int pos = 0;
    
    // 时间戳
    if (g_debug_system.config.enable_timestamps) {
        struct tm* tm_info = localtime(&msg->timestamp);
        pos += snprintf(buffer + pos, buffer_size - pos, "[%02d:%02d:%02d.%03d] ",
                       tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
                       (int)((msg->context.timestamp - (int)msg->context.timestamp) * 1000));
    }
    
    // 级别和类别
    pos += snprintf(buffer + pos, buffer_size - pos, "%s[%s:%s]%s ",
                   color_start, level_str, category_str, color_end);
    
    // 上下文信息
    if (g_debug_system.config.enable_context && msg->context.file) {
        const char* filename = strrchr(msg->context.file, '/');
        filename = filename ? filename + 1 : msg->context.file;
        pos += snprintf(buffer + pos, buffer_size - pos, "%s:%d:%s() ",
                       filename, msg->context.line, 
                       msg->context.function ? msg->context.function : "unknown");
    }
    
    // 线程ID
    pos += snprintf(buffer + pos, buffer_size - pos, "[T:%lu] ", msg->context.thread_id);
    
    // 消息内容
    if (msg->message) {
        pos += snprintf(buffer + pos, buffer_size - pos, "%s", msg->message);
    }
    
    // 详细信息
    if (msg->details) {
        pos += snprintf(buffer + pos, buffer_size - pos, " | Details: %s", msg->details);
    }
    
    // 建议
    if (msg->suggestion) {
        pos += snprintf(buffer + pos, buffer_size - pos, " | Suggestion: %s", msg->suggestion);
    }
}

// 输出调试消息
static void output_debug_message(const DebugMessage* msg) {
    char buffer[4096];
    format_debug_message(msg, buffer, sizeof(buffer));
    
    // 输出到控制台
    if (g_debug_system.config.output == DEBUG_OUTPUT_CONSOLE) {
        printf("%s\n", buffer);
        fflush(stdout);
    }
    
    // 输出到文件
    if (g_debug_system.log_file) {
        fprintf(g_debug_system.log_file, "%s\n", buffer);
        fflush(g_debug_system.log_file);
    }
    
    // 缓冲输出
    if (g_debug_system.config.enable_buffering && g_debug_system.buffer) {
        size_t msg_len = strlen(buffer) + 1; // +1 for newline
        if (g_debug_system.buffer_pos + msg_len < g_debug_system.config.buffer_size) {
            strcpy(g_debug_system.buffer + g_debug_system.buffer_pos, buffer);
            g_debug_system.buffer_pos += msg_len - 1;
            g_debug_system.buffer[g_debug_system.buffer_pos++] = '\n';
        }
    }
}

// 记录调试消息
void enhanced_debug_log_with_details(DebugLevel level, DebugCategory category,
                                   const char* file, int line, const char* function,
                                   const char* message, const char* details, const char* suggestion) {
    if (!g_debug_system.is_initialized) {
        if (enhanced_debug_init(NULL) != 0) {
            return; // 初始化失败
        }
    }
    
    // 检查级别和类别过滤
    if (level < g_debug_system.config.min_level) {
        g_debug_system.stats.filtered_messages++;
        return;
    }
    
    if (!(g_debug_system.config.enabled_categories & category)) {
        g_debug_system.stats.filtered_messages++;
        return;
    }
    
    // 创建消息
    DebugMessage* msg = create_debug_message(level, category, file, line, function,
                                           message, details, suggestion);
    if (!msg) {
        g_debug_system.stats.dropped_messages++;
        return;
    }
    
    // 添加到消息链表
    if (!g_debug_system.message_head) {
        g_debug_system.message_head = msg;
        g_debug_system.message_tail = msg;
    } else {
        g_debug_system.message_tail->next = msg;
        g_debug_system.message_tail = msg;
    }
    g_debug_system.message_count++;
    
    // 更新统计信息
    g_debug_system.stats.total_messages++;
    if (level <= DEBUG_LEVEL_ALL) {
        g_debug_system.stats.messages_by_level[level]++;
    }
    
    // 计算类别索引
    int category_index = 0;
    uint32_t cat = category;
    while (cat > 1) {
        cat >>= 1;
        category_index++;
    }
    if (category_index < 16) {
        g_debug_system.stats.messages_by_category[category_index]++;
    }
    
    g_debug_system.stats.last_message_time = time(NULL);
    
    // 输出消息
    output_debug_message(msg);
    
    // 调用消息处理器
    if (g_debug_system.message_handler) {
        g_debug_system.message_handler(msg, g_debug_system.user_data);
    }
    
    // 检查是否需要刷新
    double current_time = get_current_time();
    if (current_time - g_debug_system.last_flush_time >= g_debug_system.config.flush_interval) {
        enhanced_debug_flush();
    }
}

// 记录调试消息（格式化版本）
void enhanced_debug_log(DebugLevel level, DebugCategory category,
                       const char* file, int line, const char* function,
                       const char* format, ...) {
    char message[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    enhanced_debug_log_with_details(level, category, file, line, function,
                                   message, NULL, NULL);
}

// 刷新缓冲区
void enhanced_debug_flush(void) {
    if (!g_debug_system.is_initialized) return;

    if (g_debug_system.buffer && g_debug_system.buffer_pos > 0) {
        if (g_debug_system.log_file) {
            fwrite(g_debug_system.buffer, 1, g_debug_system.buffer_pos, g_debug_system.log_file);
            fflush(g_debug_system.log_file);
        }
        g_debug_system.buffer_pos = 0;
        g_debug_system.flush_count++;
    }

    g_debug_system.last_flush_time = get_current_time();
}

// 清理所有消息
void enhanced_debug_clear_messages(void) {
    if (!g_debug_system.is_initialized) return;

    DebugMessage* current = g_debug_system.message_head;
    while (current) {
        DebugMessage* next = current->next;
        free_debug_message(current);
        current = next;
    }

    g_debug_system.message_head = NULL;
    g_debug_system.message_tail = NULL;
    g_debug_system.message_count = 0;
}

// 获取统计信息
DebugStats enhanced_debug_get_stats(void) {
    if (!g_debug_system.is_initialized) {
        DebugStats empty_stats = {0};
        return empty_stats;
    }

    // 更新计算字段
    if (g_debug_system.stats.total_messages > 0) {
        g_debug_system.stats.avg_message_time =
            g_debug_system.stats.total_time / g_debug_system.stats.total_messages;
    }

    return g_debug_system.stats;
}

// 打印统计信息
void enhanced_debug_print_stats(void) {
    if (!g_debug_system.is_initialized) {
        printf("Enhanced Debug System: 未初始化\n");
        return;
    }

    DebugStats stats = enhanced_debug_get_stats();

    printf("=== 增强调试系统统计信息 ===\n");
    printf("总消息数: %lu\n", stats.total_messages);
    printf("过滤消息: %lu\n", stats.filtered_messages);
    printf("丢弃消息: %lu\n", stats.dropped_messages);
    printf("缓存消息: %zu\n", g_debug_system.message_count);
    printf("刷新次数: %lu\n", g_debug_system.flush_count);

    printf("\n按级别统计:\n");
    const char* level_names[] = {"NONE", "ERROR", "WARNING", "INFO", "DEBUG", "TRACE", "ALL"};
    for (int i = 1; i <= DEBUG_LEVEL_ALL; i++) {
        if (stats.messages_by_level[i] > 0) {
            printf("  %s: %lu\n", level_names[i], stats.messages_by_level[i]);
        }
    }

    printf("\n按类别统计:\n");
    const char* category_names[] = {
        "GENERAL", "MODULE", "MEMORY", "COMPILER", "RUNTIME",
        "NETWORK", "IO", "SECURITY", "PERFORMANCE"
    };
    for (int i = 0; i < 9; i++) {
        if (stats.messages_by_category[i] > 0) {
            printf("  %s: %lu\n", category_names[i], stats.messages_by_category[i]);
        }
    }

    printf("\n运行时间: %ld 秒\n", time(NULL) - stats.start_time);
    printf("内存使用: %zu 字节\n", g_debug_system.allocated_memory);
    printf("峰值内存: %zu 字节\n", g_debug_system.peak_memory);
    printf("=============================\n");
}

// 级别转字符串
const char* enhanced_debug_level_to_string(DebugLevel level) {
    switch (level) {
        case DEBUG_LEVEL_NONE: return "NONE";
        case DEBUG_LEVEL_ERROR: return "ERROR";
        case DEBUG_LEVEL_WARNING: return "WARNING";
        case DEBUG_LEVEL_INFO: return "INFO";
        case DEBUG_LEVEL_DEBUG: return "DEBUG";
        case DEBUG_LEVEL_TRACE: return "TRACE";
        case DEBUG_LEVEL_ALL: return "ALL";
        default: return "UNKNOWN";
    }
}

// 类别转字符串
const char* enhanced_debug_category_to_string(DebugCategory category) {
    switch (category) {
        case DEBUG_CATEGORY_GENERAL: return "GENERAL";
        case DEBUG_CATEGORY_MODULE: return "MODULE";
        case DEBUG_CATEGORY_MEMORY: return "MEMORY";
        case DEBUG_CATEGORY_COMPILER: return "COMPILER";
        case DEBUG_CATEGORY_RUNTIME: return "RUNTIME";
        case DEBUG_CATEGORY_NETWORK: return "NETWORK";
        case DEBUG_CATEGORY_IO: return "IO";
        case DEBUG_CATEGORY_SECURITY: return "SECURITY";
        case DEBUG_CATEGORY_PERFORMANCE: return "PERFORMANCE";
        default: return "UNKNOWN";
    }
}

// 性能计时器
DebugTimer enhanced_debug_timer_start(const char* name, DebugCategory category) {
    DebugTimer timer;
    timer.name = name;
    timer.start_time = get_current_time();
    timer.category = category;

    if (g_debug_system.config.enable_performance_tracking) {
        DEBUG_TRACE(category, "Timer started: %s", name);
    }

    return timer;
}

void enhanced_debug_timer_end(DebugTimer* timer) {
    if (!timer) return;

    double end_time = get_current_time();
    double elapsed = end_time - timer->start_time;

    if (g_debug_system.config.enable_performance_tracking) {
        DEBUG_INFO(timer->category, "Timer %s: %.6f seconds", timer->name, elapsed);
    }
}

// 设置消息处理器
void enhanced_debug_set_message_handler(void (*handler)(const DebugMessage*, void*), void* user_data) {
    if (!g_debug_system.is_initialized) return;

    g_debug_system.message_handler = handler;
    g_debug_system.user_data = user_data;
}

// 设置调试级别
void enhanced_debug_set_level(DebugLevel level) {
    if (!g_debug_system.is_initialized) return;

    g_debug_system.config.min_level = level;
    DEBUG_INFO(DEBUG_CATEGORY_GENERAL, "Debug level set to: %s",
               enhanced_debug_level_to_string(level));
}

// 启用类别
void enhanced_debug_enable_category(DebugCategory category) {
    if (!g_debug_system.is_initialized) return;

    g_debug_system.config.enabled_categories |= category;
    DEBUG_INFO(DEBUG_CATEGORY_GENERAL, "Debug category enabled: %s",
               enhanced_debug_category_to_string(category));
}

// 禁用类别
void enhanced_debug_disable_category(DebugCategory category) {
    if (!g_debug_system.is_initialized) return;

    g_debug_system.config.enabled_categories &= ~category;
    DEBUG_INFO(DEBUG_CATEGORY_GENERAL, "Debug category disabled: %s",
               enhanced_debug_category_to_string(category));
}
