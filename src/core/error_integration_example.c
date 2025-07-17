#include "unified_error_handler.h"
#include "module_stability.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * 错误处理机制集成示例
 * 
 * 展示如何在实际系统中集成统一错误处理机制
 */

// 自定义错误处理器
void system_error_handler(const UnifiedError* error, void* user_data) {
    printf("🚨 系统错误处理器被触发:\n");
    printf("   错误代码: 0x%X\n", error->error_code);
    printf("   严重性: %s\n", unified_error_severity_to_string(error->severity));
    printf("   消息: %s\n", error->message ? error->message : "无消息");
    
    // 根据错误严重性采取不同行动
    switch (error->severity) {
        case ERROR_SEVERITY_CRITICAL:
        case ERROR_SEVERITY_FATAL:
            printf("   🔥 严重错误，启动紧急处理程序\n");
            // 在实际系统中，这里可能会触发系统备份、日志记录等
            break;
        case ERROR_SEVERITY_ERROR:
            printf("   ⚠️  错误，尝试恢复\n");
            break;
        default:
            printf("   ℹ️  一般性问题，记录日志\n");
            break;
    }
}

// 自定义恢复处理器
bool system_recovery_handler(UnifiedError* error, void* user_data) {
    printf("🔧 系统恢复处理器被触发:\n");
    printf("   恢复策略: %s\n", unified_error_recovery_strategy_to_string(error->recovery_strategy));
    printf("   重试次数: %d/%d\n", error->retry_count, error->max_retries);
    
    switch (error->recovery_strategy) {
        case ERROR_RECOVERY_RETRY:
            printf("   🔄 执行重试操作\n");
            // 模拟重试逻辑
            if (error->retry_count < error->max_retries) {
                printf("   ✅ 重试成功\n");
                return true;
            } else {
                printf("   ❌ 重试次数已达上限\n");
                return false;
            }
            
        case ERROR_RECOVERY_FALLBACK:
            printf("   🔀 执行回退操作\n");
            // 模拟回退逻辑
            printf("   ✅ 回退到安全状态\n");
            return true;
            
        case ERROR_RECOVERY_RESTART:
            printf("   🔄 需要重启系统\n");
            return false;
            
        case ERROR_RECOVERY_ABORT:
            printf("   🛑 操作被中止\n");
            return false;
            
        default:
            printf("   ❓ 未知恢复策略\n");
            return false;
    }
}

// 模拟模块加载函数（集成错误处理）
int safe_module_load(const char* module_name) {
    printf("\n📦 尝试加载模块: %s\n", module_name);
    
    // 参数验证
    if (!module_name || strlen(module_name) == 0) {
        ERROR_REPORT(g_unified_error_manager, ERROR_CORE_INVALID_PARAM, ERROR_SEVERITY_ERROR,
                    "模块名称无效");
        return -1;
    }
    
    // 模拟模块加载
    if (strcmp(module_name, "nonexistent") == 0) {
        ERROR_REPORT_WITH_SUGGESTION(g_unified_error_manager, ERROR_MODULE_NOT_FOUND, ERROR_SEVERITY_ERROR,
                                     "模块未找到", "检查模块是否存在于正确路径");
        return -1;
    }
    
    if (strcmp(module_name, "corrupted") == 0) {
        ERROR_REPORT_FULL(g_unified_error_manager, ERROR_MODULE_LOAD_FAILED, ERROR_SEVERITY_CRITICAL,
                         "模块加载失败", "模块文件已损坏", "重新安装模块或使用备份");
        return -1;
    }
    
    if (strcmp(module_name, "version_mismatch") == 0) {
        ERROR_REPORT_WITH_DETAILS(g_unified_error_manager, ERROR_MODULE_VERSION_MISMATCH, ERROR_SEVERITY_WARNING,
                                 "模块版本不匹配", "期望版本2.0，实际版本1.5");
        // 警告但继续加载
    }
    
    printf("✅ 模块 %s 加载成功\n", module_name);
    return 0;
}

// 模拟编译函数（集成错误处理）
int safe_compile(const char* source_file) {
    printf("\n🔨 尝试编译文件: %s\n", source_file);
    
    if (!source_file) {
        ERROR_REPORT(g_unified_error_manager, ERROR_CORE_INVALID_PARAM, ERROR_SEVERITY_ERROR,
                    "源文件路径为空");
        return -1;
    }
    
    // 模拟编译错误
    if (strstr(source_file, "syntax_error")) {
        ERROR_REPORT_FULL(g_unified_error_manager, ERROR_COMPILER_SYNTAX, ERROR_SEVERITY_ERROR,
                         "语法错误", "第15行缺少分号", "在语句末尾添加分号");
        return -1;
    }
    
    if (strstr(source_file, "undefined_symbol")) {
        ERROR_REPORT_WITH_SUGGESTION(g_unified_error_manager, ERROR_COMPILER_UNDEFINED_SYMBOL, ERROR_SEVERITY_ERROR,
                                     "未定义的符号 'foo'", "检查函数声明或包含正确的头文件");
        return -1;
    }
    
    printf("✅ 文件 %s 编译成功\n", source_file);
    return 0;
}

// 演示错误处理机制
void demonstrate_error_handling() {
    printf("=== 错误处理机制集成演示 ===\n");
    
    // 初始化全局错误系统
    unified_error_system_init();
    
    // 设置自定义处理器
    unified_error_set_handler(g_unified_error_manager, system_error_handler, NULL);
    unified_error_set_recovery_handler(g_unified_error_manager, system_recovery_handler, NULL);
    
    // 启用详细日志
    unified_error_enable_detailed_logging(g_unified_error_manager, true);
    
    printf("\n🎯 测试场景1: 正常操作\n");
    safe_module_load("layer0");
    safe_compile("hello.c");
    
    printf("\n🎯 测试场景2: 参数错误\n");
    safe_module_load(NULL);
    safe_compile(NULL);
    
    printf("\n🎯 测试场景3: 模块加载错误\n");
    safe_module_load("nonexistent");
    safe_module_load("corrupted");
    
    printf("\n🎯 测试场景4: 编译错误\n");
    safe_compile("syntax_error.c");
    safe_compile("undefined_symbol.c");
    
    printf("\n🎯 测试场景5: 警告处理\n");
    safe_module_load("version_mismatch");
    
    // 打印错误摘要
    printf("\n📊 错误处理摘要:\n");
    unified_error_print_summary(g_unified_error_manager);
    
    // 清理
    unified_error_system_cleanup();
}

// 演示错误恢复机制
void demonstrate_error_recovery() {
    printf("\n=== 错误恢复机制演示 ===\n");
    
    unified_error_system_init();
    unified_error_set_recovery_handler(g_unified_error_manager, system_recovery_handler, NULL);
    
    // 创建一个可恢复的错误
    UnifiedError* error = ERROR_REPORT(g_unified_error_manager, ERROR_MODULE_LOAD_FAILED, ERROR_SEVERITY_ERROR,
                                      "模块加载失败，尝试恢复");
    
    if (error) {
        printf("📋 错误详情:\n");
        unified_error_print(error);
        
        // 手动尝试恢复
        printf("\n🔧 手动触发恢复:\n");
        bool recovered = unified_error_attempt_recovery(g_unified_error_manager, error);
        printf("恢复结果: %s\n", recovered ? "成功" : "失败");
        
        // 再次尝试恢复
        printf("\n🔧 再次尝试恢复:\n");
        recovered = unified_error_attempt_recovery(g_unified_error_manager, error);
        printf("恢复结果: %s\n", recovered ? "成功" : "失败");
    }
    
    unified_error_system_cleanup();
}

int main() {
    printf("🚀 统一错误处理机制集成示例\n");
    printf("=====================================\n");
    
    // 演示错误处理
    demonstrate_error_handling();
    
    // 演示错误恢复
    demonstrate_error_recovery();
    
    printf("\n✅ 演示完成！\n");
    printf("统一错误处理机制提供了:\n");
    printf("  • 统一的错误报告接口\n");
    printf("  • 自动错误分类和统计\n");
    printf("  • 可配置的错误处理策略\n");
    printf("  • 智能错误恢复机制\n");
    printf("  • 详细的错误信息和建议\n");
    
    return 0;
}
