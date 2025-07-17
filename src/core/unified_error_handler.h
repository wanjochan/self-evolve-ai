#ifndef UNIFIED_ERROR_HANDLER_H
#define UNIFIED_ERROR_HANDLER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/**
 * 统一错误处理系统
 * 
 * 为整个项目提供统一的错误处理、报告和恢复机制
 */

// 错误域定义
typedef enum {
    ERROR_DOMAIN_CORE = 0x1000,        // 核心系统错误
    ERROR_DOMAIN_MODULE = 0x2000,      // 模块系统错误
    ERROR_DOMAIN_COMPILER = 0x3000,    // 编译器错误
    ERROR_DOMAIN_RUNTIME = 0x4000,     // 运行时错误
    ERROR_DOMAIN_MEMORY = 0x5000,      // 内存管理错误
    ERROR_DOMAIN_IO = 0x6000,          // I/O错误
    ERROR_DOMAIN_SECURITY = 0x7000,    // 安全错误
    ERROR_DOMAIN_NETWORK = 0x8000,     // 网络错误
    ERROR_DOMAIN_USER = 0x9000         // 用户错误
} ErrorDomain;

// 错误严重性级别
typedef enum {
    ERROR_SEVERITY_DEBUG = 0,      // 调试信息
    ERROR_SEVERITY_INFO = 1,       // 信息
    ERROR_SEVERITY_WARNING = 2,    // 警告
    ERROR_SEVERITY_ERROR = 3,      // 错误
    ERROR_SEVERITY_CRITICAL = 4,   // 严重错误
    ERROR_SEVERITY_FATAL = 5       // 致命错误
} ErrorSeverity;

// 错误恢复策略
typedef enum {
    ERROR_RECOVERY_NONE = 0,       // 无恢复
    ERROR_RECOVERY_RETRY = 1,      // 重试
    ERROR_RECOVERY_FALLBACK = 2,   // 回退
    ERROR_RECOVERY_RESTART = 3,    // 重启
    ERROR_RECOVERY_ABORT = 4       // 中止
} ErrorRecoveryStrategy;

// 错误信息结构
typedef struct UnifiedError {
    uint32_t error_code;           // 错误代码 (域 + 具体错误)
    ErrorDomain domain;            // 错误域
    ErrorSeverity severity;        // 严重性
    time_t timestamp;              // 时间戳
    
    // 位置信息
    const char* file;              // 源文件
    int line;                      // 行号
    const char* function;          // 函数名
    
    // 错误描述
    char* message;                 // 错误消息
    char* details;                 // 详细信息
    char* suggestion;              // 建议解决方案
    
    // 上下文信息
    void* context;                 // 上下文数据
    size_t context_size;           // 上下文大小
    
    // 恢复信息
    ErrorRecoveryStrategy recovery_strategy;
    int max_retries;               // 最大重试次数
    int retry_count;               // 当前重试次数
    
    // 链表指针
    struct UnifiedError* next;
    struct UnifiedError* related;  // 相关错误
} UnifiedError;

// 错误处理器回调
typedef void (*ErrorHandler)(const UnifiedError* error, void* user_data);

// 错误恢复回调
typedef bool (*ErrorRecoveryHandler)(UnifiedError* error, void* user_data);

// 错误管理器
typedef struct {
    UnifiedError* first_error;     // 错误链表头
    UnifiedError* last_error;      // 错误链表尾
    
    // 统计信息
    uint32_t total_errors;         // 总错误数
    uint32_t errors_by_severity[6]; // 按严重性分类的错误数
    uint32_t errors_by_domain[10]; // 按域分类的错误数
    
    // 配置
    ErrorSeverity min_severity;    // 最小报告严重性
    bool auto_recovery_enabled;    // 自动恢复开关
    bool detailed_logging;         // 详细日志开关
    
    // 回调函数
    ErrorHandler error_handler;
    ErrorRecoveryHandler recovery_handler;
    void* user_data;
    
    // 限制
    uint32_t max_errors;           // 最大错误数
    uint32_t max_retries_global;   // 全局最大重试次数
} UnifiedErrorManager;

// 错误代码定义宏
#define MAKE_ERROR_CODE(domain, code) ((domain) | (code))

// 核心系统错误代码
#define ERROR_CORE_INIT_FAILED          MAKE_ERROR_CODE(ERROR_DOMAIN_CORE, 0x01)
#define ERROR_CORE_INVALID_PARAM        MAKE_ERROR_CODE(ERROR_DOMAIN_CORE, 0x02)
#define ERROR_CORE_OUT_OF_MEMORY        MAKE_ERROR_CODE(ERROR_DOMAIN_CORE, 0x03)
#define ERROR_CORE_RESOURCE_BUSY        MAKE_ERROR_CODE(ERROR_DOMAIN_CORE, 0x04)
#define ERROR_CORE_TIMEOUT              MAKE_ERROR_CODE(ERROR_DOMAIN_CORE, 0x05)

// 模块系统错误代码
#define ERROR_MODULE_NOT_FOUND          MAKE_ERROR_CODE(ERROR_DOMAIN_MODULE, 0x01)
#define ERROR_MODULE_LOAD_FAILED        MAKE_ERROR_CODE(ERROR_DOMAIN_MODULE, 0x02)
#define ERROR_MODULE_SYMBOL_NOT_FOUND   MAKE_ERROR_CODE(ERROR_DOMAIN_MODULE, 0x03)
#define ERROR_MODULE_VERSION_MISMATCH   MAKE_ERROR_CODE(ERROR_DOMAIN_MODULE, 0x04)
#define ERROR_MODULE_DEPENDENCY_FAILED  MAKE_ERROR_CODE(ERROR_DOMAIN_MODULE, 0x05)

// 编译器错误代码
#define ERROR_COMPILER_SYNTAX           MAKE_ERROR_CODE(ERROR_DOMAIN_COMPILER, 0x01)
#define ERROR_COMPILER_SEMANTIC         MAKE_ERROR_CODE(ERROR_DOMAIN_COMPILER, 0x02)
#define ERROR_COMPILER_TYPE_MISMATCH    MAKE_ERROR_CODE(ERROR_DOMAIN_COMPILER, 0x03)
#define ERROR_COMPILER_UNDEFINED_SYMBOL MAKE_ERROR_CODE(ERROR_DOMAIN_COMPILER, 0x04)
#define ERROR_COMPILER_INTERNAL         MAKE_ERROR_CODE(ERROR_DOMAIN_COMPILER, 0x05)

// 便利宏
#define ERROR_REPORT(manager, code, severity, msg) \
    unified_error_report(manager, code, severity, __FILE__, __LINE__, __func__, msg, NULL, NULL)

#define ERROR_REPORT_WITH_DETAILS(manager, code, severity, msg, details) \
    unified_error_report(manager, code, severity, __FILE__, __LINE__, __func__, msg, details, NULL)

#define ERROR_REPORT_WITH_SUGGESTION(manager, code, severity, msg, suggestion) \
    unified_error_report(manager, code, severity, __FILE__, __LINE__, __func__, msg, NULL, suggestion)

#define ERROR_REPORT_FULL(manager, code, severity, msg, details, suggestion) \
    unified_error_report(manager, code, severity, __FILE__, __LINE__, __func__, msg, details, suggestion)

// 初始化和清理
UnifiedErrorManager* unified_error_manager_create(void);
void unified_error_manager_destroy(UnifiedErrorManager* manager);
int unified_error_manager_init(UnifiedErrorManager* manager);
void unified_error_manager_cleanup(UnifiedErrorManager* manager);

// 配置
void unified_error_set_handler(UnifiedErrorManager* manager, ErrorHandler handler, void* user_data);
void unified_error_set_recovery_handler(UnifiedErrorManager* manager, ErrorRecoveryHandler handler, void* user_data);
void unified_error_set_min_severity(UnifiedErrorManager* manager, ErrorSeverity severity);
void unified_error_enable_auto_recovery(UnifiedErrorManager* manager, bool enabled);
void unified_error_enable_detailed_logging(UnifiedErrorManager* manager, bool enabled);

// 错误报告
UnifiedError* unified_error_report(UnifiedErrorManager* manager, uint32_t error_code, ErrorSeverity severity,
                                  const char* file, int line, const char* function,
                                  const char* message, const char* details, const char* suggestion);

UnifiedError* unified_error_report_with_context(UnifiedErrorManager* manager, uint32_t error_code, ErrorSeverity severity,
                                               const char* file, int line, const char* function,
                                               const char* message, const char* details, const char* suggestion,
                                               void* context, size_t context_size);

// 错误恢复
bool unified_error_attempt_recovery(UnifiedErrorManager* manager, UnifiedError* error);
void unified_error_set_recovery_strategy(UnifiedError* error, ErrorRecoveryStrategy strategy, int max_retries);

// 错误查询
UnifiedError* unified_error_get_last(UnifiedErrorManager* manager);
UnifiedError* unified_error_find_by_code(UnifiedErrorManager* manager, uint32_t error_code);
uint32_t unified_error_count_by_severity(UnifiedErrorManager* manager, ErrorSeverity severity);
uint32_t unified_error_count_by_domain(UnifiedErrorManager* manager, ErrorDomain domain);

// 错误显示
void unified_error_print(const UnifiedError* error);
void unified_error_print_all(UnifiedErrorManager* manager);
void unified_error_print_summary(UnifiedErrorManager* manager);

// 错误清理
void unified_error_clear_all(UnifiedErrorManager* manager);
void unified_error_clear_by_severity(UnifiedErrorManager* manager, ErrorSeverity severity);
void unified_error_clear_by_domain(UnifiedErrorManager* manager, ErrorDomain domain);

// 工具函数
const char* unified_error_domain_to_string(ErrorDomain domain);
const char* unified_error_severity_to_string(ErrorSeverity severity);
const char* unified_error_recovery_strategy_to_string(ErrorRecoveryStrategy strategy);
const char* unified_error_code_to_string(uint32_t error_code);

// 全局错误管理器
extern UnifiedErrorManager* g_unified_error_manager;

// 全局初始化/清理函数
int unified_error_system_init(void);
void unified_error_system_cleanup(void);

#endif // UNIFIED_ERROR_HANDLER_H
