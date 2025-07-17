#include "unified_error_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// 全局错误管理器
UnifiedErrorManager* g_unified_error_manager = NULL;

// 错误域名称
static const char* error_domain_names[] = {
    "UNKNOWN", "CORE", "MODULE", "COMPILER", "RUNTIME", 
    "MEMORY", "IO", "SECURITY", "NETWORK", "USER"
};

// 错误严重性名称
static const char* error_severity_names[] = {
    "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL", "FATAL"
};

// 错误恢复策略名称
static const char* error_recovery_strategy_names[] = {
    "NONE", "RETRY", "FALLBACK", "RESTART", "ABORT"
};

// 创建错误管理器
UnifiedErrorManager* unified_error_manager_create(void) {
    UnifiedErrorManager* manager = malloc(sizeof(UnifiedErrorManager));
    if (!manager) return NULL;
    
    memset(manager, 0, sizeof(UnifiedErrorManager));
    
    // 设置默认配置
    manager->min_severity = ERROR_SEVERITY_WARNING;
    manager->auto_recovery_enabled = true;
    manager->detailed_logging = false;
    manager->max_errors = 1000;
    manager->max_retries_global = 3;
    
    return manager;
}

// 销毁错误管理器
void unified_error_manager_destroy(UnifiedErrorManager* manager) {
    if (!manager) return;
    
    unified_error_clear_all(manager);
    free(manager);
}

// 初始化错误管理器
int unified_error_manager_init(UnifiedErrorManager* manager) {
    if (!manager) return -1;
    
    // 重置统计信息
    memset(manager->errors_by_severity, 0, sizeof(manager->errors_by_severity));
    memset(manager->errors_by_domain, 0, sizeof(manager->errors_by_domain));
    manager->total_errors = 0;
    
    return 0;
}

// 清理错误管理器
void unified_error_manager_cleanup(UnifiedErrorManager* manager) {
    if (!manager) return;
    
    unified_error_clear_all(manager);
}

// 设置错误处理器
void unified_error_set_handler(UnifiedErrorManager* manager, ErrorHandler handler, void* user_data) {
    if (!manager) return;
    
    manager->error_handler = handler;
    manager->user_data = user_data;
}

// 设置恢复处理器
void unified_error_set_recovery_handler(UnifiedErrorManager* manager, ErrorRecoveryHandler handler, void* user_data) {
    if (!manager) return;
    
    manager->recovery_handler = handler;
    manager->user_data = user_data;
}

// 设置最小严重性
void unified_error_set_min_severity(UnifiedErrorManager* manager, ErrorSeverity severity) {
    if (!manager) return;
    
    manager->min_severity = severity;
}

// 启用自动恢复
void unified_error_enable_auto_recovery(UnifiedErrorManager* manager, bool enabled) {
    if (!manager) return;
    
    manager->auto_recovery_enabled = enabled;
}

// 启用详细日志
void unified_error_enable_detailed_logging(UnifiedErrorManager* manager, bool enabled) {
    if (!manager) return;
    
    manager->detailed_logging = enabled;
}

// 创建错误对象
static UnifiedError* create_error(uint32_t error_code, ErrorSeverity severity,
                                 const char* file, int line, const char* function,
                                 const char* message, const char* details, const char* suggestion) {
    UnifiedError* error = malloc(sizeof(UnifiedError));
    if (!error) return NULL;
    
    memset(error, 0, sizeof(UnifiedError));
    
    error->error_code = error_code;
    error->domain = (ErrorDomain)(error_code & 0xF000);
    error->severity = severity;
    error->timestamp = time(NULL);
    error->file = file;
    error->line = line;
    error->function = function;
    
    // 复制字符串
    error->message = message ? strdup(message) : NULL;
    error->details = details ? strdup(details) : NULL;
    error->suggestion = suggestion ? strdup(suggestion) : NULL;
    
    // 设置默认恢复策略
    switch (severity) {
        case ERROR_SEVERITY_WARNING:
            error->recovery_strategy = ERROR_RECOVERY_NONE;
            break;
        case ERROR_SEVERITY_ERROR:
            error->recovery_strategy = ERROR_RECOVERY_RETRY;
            error->max_retries = 3;
            break;
        case ERROR_SEVERITY_CRITICAL:
            error->recovery_strategy = ERROR_RECOVERY_FALLBACK;
            error->max_retries = 1;
            break;
        case ERROR_SEVERITY_FATAL:
            error->recovery_strategy = ERROR_RECOVERY_ABORT;
            break;
        default:
            error->recovery_strategy = ERROR_RECOVERY_NONE;
            break;
    }
    
    return error;
}

// 释放错误对象
static void free_error(UnifiedError* error) {
    if (!error) return;
    
    free(error->message);
    free(error->details);
    free(error->suggestion);
    free(error->context);
    free(error);
}

// 报告错误
UnifiedError* unified_error_report(UnifiedErrorManager* manager, uint32_t error_code, ErrorSeverity severity,
                                  const char* file, int line, const char* function,
                                  const char* message, const char* details, const char* suggestion) {
    if (!manager || severity < manager->min_severity) return NULL;
    
    // 检查错误数量限制
    if (manager->total_errors >= manager->max_errors) {
        return NULL;
    }
    
    UnifiedError* error = create_error(error_code, severity, file, line, function, message, details, suggestion);
    if (!error) return NULL;
    
    // 添加到错误链表
    if (!manager->first_error) {
        manager->first_error = error;
        manager->last_error = error;
    } else {
        manager->last_error->next = error;
        manager->last_error = error;
    }
    
    // 更新统计信息
    manager->total_errors++;
    if (severity < 6) {
        manager->errors_by_severity[severity]++;
    }
    
    ErrorDomain domain = (ErrorDomain)(error_code & 0xF000);
    int domain_index = (domain >> 12) - 1;
    if (domain_index >= 0 && domain_index < 10) {
        manager->errors_by_domain[domain_index]++;
    }
    
    // 调用错误处理器
    if (manager->error_handler) {
        manager->error_handler(error, manager->user_data);
    }
    
    // 自动恢复
    if (manager->auto_recovery_enabled && error->recovery_strategy != ERROR_RECOVERY_NONE) {
        unified_error_attempt_recovery(manager, error);
    }
    
    // 详细日志
    if (manager->detailed_logging) {
        unified_error_print(error);
    }
    
    return error;
}

// 尝试错误恢复
bool unified_error_attempt_recovery(UnifiedErrorManager* manager, UnifiedError* error) {
    if (!manager || !error) return false;
    
    // 检查重试次数
    if (error->retry_count >= error->max_retries) {
        return false;
    }
    
    error->retry_count++;
    
    // 调用恢复处理器
    if (manager->recovery_handler) {
        return manager->recovery_handler(error, manager->user_data);
    }
    
    // 默认恢复策略
    switch (error->recovery_strategy) {
        case ERROR_RECOVERY_RETRY:
            printf("Retrying operation (attempt %d/%d)...\n", error->retry_count, error->max_retries);
            return true;
            
        case ERROR_RECOVERY_FALLBACK:
            printf("Attempting fallback recovery...\n");
            return true;
            
        case ERROR_RECOVERY_RESTART:
            printf("Restart required for recovery\n");
            return false;
            
        case ERROR_RECOVERY_ABORT:
            printf("Operation aborted due to fatal error\n");
            return false;
            
        default:
            return false;
    }
}

// 获取最后一个错误
UnifiedError* unified_error_get_last(UnifiedErrorManager* manager) {
    if (!manager) return NULL;
    return manager->last_error;
}

// 按严重性统计错误
uint32_t unified_error_count_by_severity(UnifiedErrorManager* manager, ErrorSeverity severity) {
    if (!manager || severity >= 6) return 0;
    return manager->errors_by_severity[severity];
}

// 打印错误
void unified_error_print(const UnifiedError* error) {
    if (!error) return;
    
    printf("[%s:%s] %s:%d in %s(): %s\n",
           unified_error_domain_to_string(error->domain),
           unified_error_severity_to_string(error->severity),
           error->file ? error->file : "unknown",
           error->line,
           error->function ? error->function : "unknown",
           error->message ? error->message : "No message");
    
    if (error->details) {
        printf("  Details: %s\n", error->details);
    }
    
    if (error->suggestion) {
        printf("  Suggestion: %s\n", error->suggestion);
    }
    
    if (error->retry_count > 0) {
        printf("  Retries: %d/%d\n", error->retry_count, error->max_retries);
    }
}

// 打印所有错误
void unified_error_print_all(UnifiedErrorManager* manager) {
    if (!manager) return;
    
    UnifiedError* error = manager->first_error;
    while (error) {
        unified_error_print(error);
        error = error->next;
    }
}

// 打印错误摘要
void unified_error_print_summary(UnifiedErrorManager* manager) {
    if (!manager) return;
    
    printf("\n=== Error Summary ===\n");
    printf("Total errors: %u\n", manager->total_errors);
    
    for (int i = 0; i < 6; i++) {
        if (manager->errors_by_severity[i] > 0) {
            printf("  %s: %u\n", error_severity_names[i], manager->errors_by_severity[i]);
        }
    }
    
    printf("=====================\n");
}

// 清理所有错误
void unified_error_clear_all(UnifiedErrorManager* manager) {
    if (!manager) return;
    
    UnifiedError* error = manager->first_error;
    while (error) {
        UnifiedError* next = error->next;
        free_error(error);
        error = next;
    }
    
    manager->first_error = NULL;
    manager->last_error = NULL;
    manager->total_errors = 0;
    memset(manager->errors_by_severity, 0, sizeof(manager->errors_by_severity));
    memset(manager->errors_by_domain, 0, sizeof(manager->errors_by_domain));
}

// 工具函数
const char* unified_error_domain_to_string(ErrorDomain domain) {
    int index = (domain >> 12);
    if (index >= 1 && index <= 9) {
        return error_domain_names[index];
    }
    return error_domain_names[0]; // "UNKNOWN"
}

const char* unified_error_severity_to_string(ErrorSeverity severity) {
    if (severity < 6) {
        return error_severity_names[severity];
    }
    return "UNKNOWN";
}

const char* unified_error_recovery_strategy_to_string(ErrorRecoveryStrategy strategy) {
    if (strategy < 5) {
        return error_recovery_strategy_names[strategy];
    }
    return "UNKNOWN";
}

// 全局系统初始化
int unified_error_system_init(void) {
    if (g_unified_error_manager) {
        return 0; // 已经初始化
    }
    
    g_unified_error_manager = unified_error_manager_create();
    if (!g_unified_error_manager) {
        return -1;
    }
    
    return unified_error_manager_init(g_unified_error_manager);
}

// 全局系统清理
void unified_error_system_cleanup(void) {
    if (g_unified_error_manager) {
        unified_error_manager_destroy(g_unified_error_manager);
        g_unified_error_manager = NULL;
    }
}
