/**
 * 模块系统使用示例
 * 
 * 展示如何使用 Self-Evolving AI 的模块系统，
 * 包括模块加载、符号解析和错误处理。
 */

#include <stdio.h>
#include <stdlib.h>
#include "../../src/core/module_stability.h"

// 演示基础模块加载
void demo_basic_module_loading() {
    printf("=== 基础模块加载演示 ===\n");
    
    // 初始化模块系统
    if (module_stability_init(NULL) != 0) {
        printf("❌ 模块系统初始化失败\n");
        return;
    }
    
    // 加载 layer0 模块
    void* layer0_handle = stable_module_load("layer0");
    if (layer0_handle) {
        printf("✅ layer0 模块加载成功\n");
        
        // 获取模块统计信息
        ModuleStats* stats = module_get_stats("layer0");
        if (stats) {
            printf("   加载次数: %lu\n", stats->load_count);
            printf("   健康状态: %s\n", 
                   stats->health == MODULE_HEALTH_HEALTHY ? "健康" : "异常");
        }
    } else {
        printf("❌ layer0 模块加载失败\n");
    }
    
    // 尝试加载其他模块
    const char* modules[] = {"pipeline", "compiler", "libc"};
    int module_count = sizeof(modules) / sizeof(modules[0]);
    
    printf("\n尝试加载其他模块:\n");
    for (int i = 0; i < module_count; i++) {
        void* handle = stable_module_load(modules[i]);
        if (handle) {
            printf("✅ %s 模块加载成功\n", modules[i]);
        } else {
            printf("❌ %s 模块加载失败\n", modules[i]);
        }
    }
    
    // 清理模块系统
    module_stability_cleanup();
    printf("\n模块系统已清理\n");
}

// 演示模块健康监控
void demo_module_health_monitoring() {
    printf("\n=== 模块健康监控演示 ===\n");
    
    module_stability_init(NULL);
    
    // 加载模块并监控健康状态
    void* handle = stable_module_load("layer0");
    if (handle) {
        // 检查健康状态
        ModuleHealthStatus health = module_get_health("layer0");
        const char* health_names[] = {"未知", "健康", "警告", "错误", "严重"};
        printf("模块健康状态: %s\n", health_names[health]);
        
        // 获取详细统计信息
        ModuleStats* stats = module_get_stats("layer0");
        if (stats) {
            printf("详细统计信息:\n");
            printf("  加载次数: %lu\n", stats->load_count);
            printf("  符号解析次数: %lu\n", stats->symbol_resolve_count);
            printf("  错误次数: %lu\n", stats->error_count);
            printf("  最后加载时间: %.3f\n", stats->last_load_time);
        }
        
        // 打印模块统计摘要
        printf("\n模块统计摘要:\n");
        module_print_module_stats("layer0");
    }
    
    module_stability_cleanup();
}

// 演示错误处理
void demo_error_handling() {
    printf("\n=== 错误处理演示 ===\n");
    
    module_stability_init(NULL);
    
    // 尝试加载不存在的模块
    printf("尝试加载不存在的模块...\n");
    void* invalid_handle = stable_module_load("nonexistent_module");
    if (!invalid_handle) {
        printf("✅ 正确处理了不存在的模块\n");
    }
    
    // 尝试解析不存在的符号
    void* layer0_handle = stable_module_load("layer0");
    if (layer0_handle) {
        printf("\n尝试解析不存在的符号...\n");
        void* invalid_symbol = stable_module_resolve("layer0", "nonexistent_function");
        if (!invalid_symbol) {
            printf("✅ 正确处理了不存在的符号\n");
        }
    }
    
    module_stability_cleanup();
}

// 演示性能测试
void demo_performance_test() {
    printf("\n=== 性能测试演示 ===\n");
    
    module_stability_init(NULL);
    
    // 测试重复加载性能
    printf("测试重复加载性能 (10次)...\n");
    
    for (int i = 0; i < 10; i++) {
        void* handle = stable_module_load("layer0");
        if (handle) {
            printf(".");
            fflush(stdout);
        } else {
            printf("X");
            fflush(stdout);
        }
    }
    printf("\n");
    
    // 获取性能统计
    ModuleStats* stats = module_get_stats("layer0");
    if (stats) {
        printf("性能统计:\n");
        printf("  总加载次数: %lu\n", stats->load_count);
        printf("  平均加载时间: %.3f秒\n", stats->last_load_time);
    }
    
    // 打印系统统计
    printf("\n系统统计:\n");
    module_print_system_stats();
    
    module_stability_cleanup();
}

int main() {
    printf("Self-Evolving AI 模块系统示例\n");
    printf("================================\n");
    
    // 运行各种演示
    demo_basic_module_loading();
    demo_module_health_monitoring();
    demo_error_handling();
    demo_performance_test();
    
    printf("\n🎉 模块系统演示完成！\n");
    printf("\n要点总结:\n");
    printf("• 使用 stable_module_load() 加载模块\n");
    printf("• 使用 module_get_stats() 获取统计信息\n");
    printf("• 使用 module_get_health() 检查健康状态\n");
    printf("• 始终调用 module_stability_cleanup() 清理资源\n");
    
    return 0;
}

/*
编译和运行说明：

1. 确保模块系统已构建：
   ./build_modules_gcc.sh

2. 编译示例：
   gcc examples/basic_usage/module_example.c src/core/module_stability.c \
       -I. -o examples/basic_usage/module_example -ldl

3. 运行示例：
   ./examples/basic_usage/module_example

4. 预期输出：
   - 模块加载成功/失败信息
   - 模块健康状态和统计信息
   - 错误处理演示
   - 性能测试结果

注意事项：
- 确保 bin/layer2/ 目录下有模块文件
- 如果模块加载失败，检查文件权限和路径
- 性能结果可能因系统而异
*/
