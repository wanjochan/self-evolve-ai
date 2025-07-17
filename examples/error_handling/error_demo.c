/**
 * 统一错误处理系统示例
 * 
 * 展示如何使用 Self-Evolving AI 的统一错误处理系统，
 * 包括错误报告、分类、恢复和统计。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/core/unified_error_handler.h"

// 自定义错误处理器
void custom_error_handler(const UnifiedError* error, void* user_data) {
    printf("🚨 自定义错误处理器触发:\n");
    printf("   域: %s\n", unified_error_domain_to_string(error->domain));
    printf("   严重性: %s\n", unified_error_severity_to_string(error->severity));
    printf("   消息: %s\n", error->message ? error->message : "无消息");
    
    if (error->details) {
        printf("   详情: %s\n", error->details);
    }
    
    if (error->suggestion) {
        printf("   建议: %s\n", error->suggestion);
    }
    
    printf("   位置: %s:%d in %s()\n", 
           error->file ? error->file : "unknown",
           error->line,
           error->function ? error->function : "unknown");
}

// 自定义恢复处理器
bool custom_recovery_handler(UnifiedError* error, void* user_data) {
    printf("🔧 自定义恢复处理器触发:\n");
    printf("   策略: %s\n", unified_error_recovery_strategy_to_string(error->recovery_strategy));
    printf("   重试: %d/%d\n", error->retry_count, error->max_retries);
    
    // 模拟恢复逻辑
    switch (error->recovery_strategy) {
        case ERROR_RECOVERY_RETRY:
            if (error->retry_count < error->max_retries) {
                printf("   ✅ 重试成功\n");
                return true;
            }
            printf("   ❌ 重试次数已达上限\n");
            return false;
            
        case ERROR_RECOVERY_FALLBACK:
            printf("   ✅ 回退到安全状态\n");
            return true;
            
        default:
            printf("   ❓ 无法恢复\n");
            return false;
    }
}

// 演示基础错误报告
void demo_basic_error_reporting() {
    printf("=== 基础错误报告演示 ===\n");
    
    // 初始化错误系统
    unified_error_system_init();
    
    // 报告不同类型的错误
    printf("1. 报告核心系统错误:\n");
    ERROR_REPORT(g_unified_error_manager, 
                ERROR_CORE_INVALID_PARAM, 
                ERROR_SEVERITY_ERROR, 
                "参数验证失败");
    
    printf("\n2. 报告模块系统错误:\n");
    ERROR_REPORT_WITH_DETAILS(g_unified_error_manager,
                             ERROR_MODULE_NOT_FOUND,
                             ERROR_SEVERITY_ERROR,
                             "模块未找到",
                             "指定的模块文件不存在于系统路径中");
    
    printf("\n3. 报告编译器错误:\n");
    ERROR_REPORT_FULL(g_unified_error_manager,
                     ERROR_COMPILER_SYNTAX,
                     ERROR_SEVERITY_ERROR,
                     "语法错误",
                     "第15行缺少分号",
                     "在语句末尾添加分号 ';'");
    
    // 打印错误摘要
    printf("\n错误摘要:\n");
    unified_error_print_summary(g_unified_error_manager);
    
    unified_error_system_cleanup();
}

// 演示错误严重性过滤
void demo_severity_filtering() {
    printf("\n=== 错误严重性过滤演示 ===\n");
    
    unified_error_system_init();
    
    // 设置最小严重性为 ERROR
    unified_error_set_min_severity(g_unified_error_manager, ERROR_SEVERITY_ERROR);
    printf("设置最小严重性为 ERROR\n");
    
    // 报告不同严重性的错误
    printf("\n报告 DEBUG 级别错误 (应被过滤):\n");
    UnifiedError* debug_error = ERROR_REPORT(g_unified_error_manager,
                                           ERROR_CORE_TIMEOUT,
                                           ERROR_SEVERITY_DEBUG,
                                           "调试信息");
    printf("DEBUG 错误是否被记录: %s\n", debug_error ? "是" : "否");
    
    printf("\n报告 WARNING 级别错误 (应被过滤):\n");
    UnifiedError* warning_error = ERROR_REPORT(g_unified_error_manager,
                                             ERROR_CORE_TIMEOUT,
                                             ERROR_SEVERITY_WARNING,
                                             "警告信息");
    printf("WARNING 错误是否被记录: %s\n", warning_error ? "是" : "否");
    
    printf("\n报告 ERROR 级别错误 (应被记录):\n");
    UnifiedError* error_error = ERROR_REPORT(g_unified_error_manager,
                                           ERROR_CORE_TIMEOUT,
                                           ERROR_SEVERITY_ERROR,
                                           "错误信息");
    printf("ERROR 错误是否被记录: %s\n", error_error ? "是" : "否");
    
    printf("\n当前错误统计:\n");
    unified_error_print_summary(g_unified_error_manager);
    
    unified_error_system_cleanup();
}

// 演示自定义错误处理器
void demo_custom_handlers() {
    printf("\n=== 自定义错误处理器演示 ===\n");
    
    unified_error_system_init();
    
    // 设置自定义处理器
    unified_error_set_handler(g_unified_error_manager, custom_error_handler, NULL);
    unified_error_set_recovery_handler(g_unified_error_manager, custom_recovery_handler, NULL);
    
    printf("已设置自定义错误和恢复处理器\n\n");
    
    // 报告错误 (将触发自定义处理器)
    printf("报告错误 (将触发自定义处理器):\n");
    ERROR_REPORT_FULL(g_unified_error_manager,
                     ERROR_MODULE_LOAD_FAILED,
                     ERROR_SEVERITY_ERROR,
                     "模块加载失败",
                     "动态库文件损坏",
                     "重新安装模块或使用备份文件");
    
    unified_error_system_cleanup();
}

// 演示错误恢复机制
void demo_error_recovery() {
    printf("\n=== 错误恢复机制演示 ===\n");
    
    unified_error_system_init();
    unified_error_set_recovery_handler(g_unified_error_manager, custom_recovery_handler, NULL);
    
    // 创建可恢复的错误
    printf("创建可重试的错误:\n");
    UnifiedError* retry_error = ERROR_REPORT(g_unified_error_manager,
                                           ERROR_MODULE_LOAD_FAILED,
                                           ERROR_SEVERITY_ERROR,
                                           "模块加载失败，尝试重试");
    
    if (retry_error) {
        printf("\n手动触发恢复尝试:\n");
        for (int i = 0; i < 3; i++) {
            printf("尝试 %d:\n", i + 1);
            bool recovered = unified_error_attempt_recovery(g_unified_error_manager, retry_error);
            printf("恢复结果: %s\n\n", recovered ? "成功" : "失败");
            
            if (!recovered) break;
        }
    }
    
    // 创建回退错误
    printf("创建需要回退的错误:\n");
    UnifiedError* fallback_error = ERROR_REPORT(g_unified_error_manager,
                                               ERROR_COMPILER_INTERNAL,
                                               ERROR_SEVERITY_CRITICAL,
                                               "编译器内部错误");
    
    if (fallback_error) {
        // 设置回退策略
        unified_error_set_recovery_strategy(fallback_error, ERROR_RECOVERY_FALLBACK, 1);
        
        printf("\n尝试回退恢复:\n");
        bool recovered = unified_error_attempt_recovery(g_unified_error_manager, fallback_error);
        printf("回退恢复结果: %s\n", recovered ? "成功" : "失败");
    }
    
    unified_error_system_cleanup();
}

// 演示错误统计和分析
void demo_error_statistics() {
    printf("\n=== 错误统计和分析演示 ===\n");
    
    unified_error_system_init();
    
    // 生成一些测试错误
    printf("生成测试错误数据...\n");
    
    // 不同域的错误
    ERROR_REPORT(g_unified_error_manager, ERROR_CORE_INIT_FAILED, ERROR_SEVERITY_CRITICAL, "核心初始化失败");
    ERROR_REPORT(g_unified_error_manager, ERROR_MODULE_NOT_FOUND, ERROR_SEVERITY_ERROR, "模块未找到");
    ERROR_REPORT(g_unified_error_manager, ERROR_COMPILER_SYNTAX, ERROR_SEVERITY_ERROR, "语法错误");
    ERROR_REPORT(g_unified_error_manager, ERROR_CORE_OUT_OF_MEMORY, ERROR_SEVERITY_CRITICAL, "内存不足");
    ERROR_REPORT(g_unified_error_manager, ERROR_MODULE_VERSION_MISMATCH, ERROR_SEVERITY_WARNING, "版本不匹配");
    
    // 打印详细统计
    printf("\n详细错误统计:\n");
    unified_error_print_summary(g_unified_error_manager);
    
    // 按严重性统计
    printf("\n按严重性分类:\n");
    for (int i = 0; i < 6; i++) {
        uint32_t count = unified_error_count_by_severity(g_unified_error_manager, (ErrorSeverity)i);
        if (count > 0) {
            printf("  %s: %u 个\n", unified_error_severity_to_string((ErrorSeverity)i), count);
        }
    }
    
    // 获取最后一个错误
    UnifiedError* last_error = unified_error_get_last(g_unified_error_manager);
    if (last_error) {
        printf("\n最后一个错误:\n");
        unified_error_print(last_error);
    }
    
    unified_error_system_cleanup();
}

int main() {
    printf("Self-Evolving AI 统一错误处理系统示例\n");
    printf("=====================================\n");
    
    // 运行各种演示
    demo_basic_error_reporting();
    demo_severity_filtering();
    demo_custom_handlers();
    demo_error_recovery();
    demo_error_statistics();
    
    printf("\n🎉 错误处理系统演示完成！\n");
    printf("\n要点总结:\n");
    printf("• 使用 ERROR_REPORT 宏报告错误\n");
    printf("• 设置自定义错误和恢复处理器\n");
    printf("• 配置错误严重性过滤\n");
    printf("• 实现智能错误恢复机制\n");
    printf("• 监控错误统计和趋势\n");
    
    return 0;
}

/*
编译和运行说明：

1. 编译示例：
   gcc examples/error_handling/error_demo.c src/core/unified_error_handler.c \
       -I. -o examples/error_handling/error_demo

2. 运行示例：
   ./examples/error_handling/error_demo

3. 预期输出：
   - 各种错误报告演示
   - 错误严重性过滤效果
   - 自定义处理器触发
   - 错误恢复尝试结果
   - 详细的错误统计信息

学习要点：
- 理解错误域和严重性分类
- 掌握错误恢复策略的使用
- 学会配置和使用自定义处理器
- 了解错误统计和分析功能
*/
