#ifndef ENHANCED_DEBUG_SYSTEM_H
#define ENHANCED_DEBUG_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <stdio.h>

// T4.1 调试工具增强
// 目标: 调试信息详细准确，便于问题定位

#ifdef __cplusplus
extern "C" {
#endif

// 调试级别
typedef enum {
    DEBUG_LEVEL_NONE = 0,
    DEBUG_LEVEL_ERROR = 1,
    DEBUG_LEVEL_WARNING = 2,
    DEBUG_LEVEL_INFO = 3,
    DEBUG_LEVEL_DEBUG = 4,
    DEBUG_LEVEL_TRACE = 5,
    DEBUG_LEVEL_ALL = 6
} DebugLevel;

// 调试类别
typedef enum {
    DEBUG_CATEGORY_GENERAL = 0x0001,
    DEBUG_CATEGORY_MODULE = 0x0002,
    DEBUG_CATEGORY_MEMORY = 0x0004,
    DEBUG_CATEGORY_COMPILER = 0x0008,
    DEBUG_CATEGORY_RUNTIME = 0x0010,
    DEBUG_CATEGORY_NETWORK = 0x0020,
    DEBUG_CATEGORY_IO = 0x0040,
    DEBUG_CATEGORY_SECURITY = 0x0080,
    DEBUG_CATEGORY_PERFORMANCE = 0x0100,
    DEBUG_CATEGORY_ALL = 0xFFFF
} DebugCategory;

// 调试输出格式
typedef enum {
    DEBUG_FORMAT_PLAIN,
    DEBUG_FORMAT_COLORED,
    DEBUG_FORMAT_JSON,
    DEBUG_FORMAT_XML
} DebugFormat;

// 调试输出目标
typedef enum {
    DEBUG_OUTPUT_CONSOLE,
    DEBUG_OUTPUT_FILE,
    DEBUG_OUTPUT_SYSLOG,
    DEBUG_OUTPUT_NETWORK
} DebugOutput;

// 调试上下文信息
typedef struct {
    const char* file;
    int line;
    const char* function;
    const char* module;
    uint64_t thread_id;
    double timestamp;
    uint32_t sequence;
} DebugContext;

// 调试消息
typedef struct DebugMessage {
    uint32_t id;
    DebugLevel level;
    DebugCategory category;
    DebugContext context;
    char* message;
    char* details;
    char* suggestion;
    time_t timestamp;
    struct DebugMessage* next;
} DebugMessage;

// 调试配置
typedef struct {
    DebugLevel min_level;
    uint32_t enabled_categories;
    DebugFormat format;
    DebugOutput output;
    
    bool enable_timestamps;
    bool enable_colors;
    bool enable_context;
    bool enable_stack_trace;
    bool enable_memory_tracking;
    bool enable_performance_tracking;
    
    const char* log_file;
    size_t max_log_size;
    int max_log_files;
    
    bool enable_filtering;
    const char* filter_pattern;
    
    bool enable_buffering;
    size_t buffer_size;
    double flush_interval;
} DebugConfig;

// 调试统计
typedef struct {
    uint64_t total_messages;
    uint64_t messages_by_level[DEBUG_LEVEL_ALL + 1];
    uint64_t messages_by_category[16];
    uint64_t dropped_messages;
    uint64_t filtered_messages;
    
    double total_time;
    double avg_message_time;
    size_t memory_usage;
    
    time_t start_time;
    time_t last_message_time;
} DebugStats;

// 增强调试系统
typedef struct {
    DebugConfig config;
    DebugStats stats;
    
    DebugMessage* message_head;
    DebugMessage* message_tail;
    size_t message_count;
    
    FILE* log_file;
    char* buffer;
    size_t buffer_pos;
    
    bool is_initialized;
    uint32_t next_message_id;
    uint32_t next_sequence;
    
    // 回调函数
    void (*message_handler)(const DebugMessage* msg, void* user_data);
    void* user_data;
    
    // 性能跟踪
    double last_flush_time;
    uint64_t flush_count;
    
    // 内存跟踪
    size_t allocated_memory;
    size_t peak_memory;
} EnhancedDebugSystem;

// 全局调试系统实例
extern EnhancedDebugSystem g_debug_system;

// 初始化和清理
int enhanced_debug_init(const DebugConfig* config);
void enhanced_debug_cleanup(void);
bool enhanced_debug_is_initialized(void);

// 配置管理
DebugConfig enhanced_debug_get_default_config(void);
int enhanced_debug_set_config(const DebugConfig* config);
DebugConfig enhanced_debug_get_config(void);

// 调试消息接口
void enhanced_debug_log(DebugLevel level, DebugCategory category,
                       const char* file, int line, const char* function,
                       const char* format, ...);

void enhanced_debug_log_with_details(DebugLevel level, DebugCategory category,
                                   const char* file, int line, const char* function,
                                   const char* message, const char* details, const char* suggestion);

// 便利宏
#define DEBUG_ERROR(category, ...) \
    enhanced_debug_log(DEBUG_LEVEL_ERROR, category, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define DEBUG_WARNING(category, ...) \
    enhanced_debug_log(DEBUG_LEVEL_WARNING, category, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define DEBUG_INFO(category, ...) \
    enhanced_debug_log(DEBUG_LEVEL_INFO, category, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define DEBUG_DEBUG(category, ...) \
    enhanced_debug_log(DEBUG_LEVEL_DEBUG, category, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define DEBUG_TRACE(category, ...) \
    enhanced_debug_log(DEBUG_LEVEL_TRACE, category, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

// 特定类别的便利宏
#define DEBUG_MODULE(...) DEBUG_INFO(DEBUG_CATEGORY_MODULE, __VA_ARGS__)
#define DEBUG_MEMORY(...) DEBUG_DEBUG(DEBUG_CATEGORY_MEMORY, __VA_ARGS__)
#define DEBUG_COMPILER(...) DEBUG_DEBUG(DEBUG_CATEGORY_COMPILER, __VA_ARGS__)
#define DEBUG_RUNTIME(...) DEBUG_DEBUG(DEBUG_CATEGORY_RUNTIME, __VA_ARGS__)
#define DEBUG_PERFORMANCE(...) DEBUG_INFO(DEBUG_CATEGORY_PERFORMANCE, __VA_ARGS__)

// 条件调试
#define DEBUG_IF(condition, level, category, ...) \
    do { if (condition) enhanced_debug_log(level, category, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__); } while(0)

// 调试断言
#define DEBUG_ASSERT(condition, ...) \
    do { if (!(condition)) { \
        DEBUG_ERROR(DEBUG_CATEGORY_GENERAL, "Assertion failed: " #condition ". " __VA_ARGS__); \
        abort(); \
    } } while(0)

// 性能测量
typedef struct {
    const char* name;
    double start_time;
    DebugCategory category;
} DebugTimer;

DebugTimer enhanced_debug_timer_start(const char* name, DebugCategory category);
void enhanced_debug_timer_end(DebugTimer* timer);

#define DEBUG_TIMER_START(name, category) \
    DebugTimer _debug_timer = enhanced_debug_timer_start(name, category)

#define DEBUG_TIMER_END() \
    enhanced_debug_timer_end(&_debug_timer)

// 内存跟踪
void enhanced_debug_track_allocation(void* ptr, size_t size, const char* file, int line);
void enhanced_debug_track_deallocation(void* ptr, const char* file, int line);

#define DEBUG_MALLOC(size) \
    ({ void* _ptr = malloc(size); enhanced_debug_track_allocation(_ptr, size, __FILE__, __LINE__); _ptr; })

#define DEBUG_FREE(ptr) \
    do { enhanced_debug_track_deallocation(ptr, __FILE__, __LINE__); free(ptr); } while(0)

// 堆栈跟踪
void enhanced_debug_print_stack_trace(void);
char* enhanced_debug_get_stack_trace(void);

// 消息管理
void enhanced_debug_flush(void);
void enhanced_debug_clear_messages(void);
DebugMessage* enhanced_debug_get_messages(void);
size_t enhanced_debug_get_message_count(void);

// 过滤和搜索
void enhanced_debug_set_filter(const char* pattern);
DebugMessage* enhanced_debug_find_messages(DebugLevel min_level, DebugCategory category);

// 统计和分析
DebugStats enhanced_debug_get_stats(void);
void enhanced_debug_reset_stats(void);
void enhanced_debug_print_stats(void);

// 导出和导入
int enhanced_debug_export_log(const char* filename, DebugFormat format);
int enhanced_debug_import_log(const char* filename);

// 实用工具
const char* enhanced_debug_level_to_string(DebugLevel level);
const char* enhanced_debug_category_to_string(DebugCategory category);
DebugLevel enhanced_debug_string_to_level(const char* str);
DebugCategory enhanced_debug_string_to_category(const char* str);

// 高级功能
void enhanced_debug_set_message_handler(void (*handler)(const DebugMessage*, void*), void* user_data);
void enhanced_debug_enable_category(DebugCategory category);
void enhanced_debug_disable_category(DebugCategory category);
void enhanced_debug_set_level(DebugLevel level);

// 调试会话管理
typedef struct {
    uint32_t session_id;
    time_t start_time;
    time_t end_time;
    uint64_t message_count;
    char description[256];
} DebugSession;

uint32_t enhanced_debug_start_session(const char* description);
void enhanced_debug_end_session(uint32_t session_id);
DebugSession* enhanced_debug_get_session(uint32_t session_id);

#ifdef __cplusplus
}
#endif

#endif // ENHANCED_DEBUG_SYSTEM_H
