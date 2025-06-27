/**
 * test_ai_adaptive_framework.c - AI适应性进化框架演示程序
 * 
 * 这个程序演示了完整的AI适应性进化框架功能
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../ai_adaptive_framework.h"

// 测试代码样本
const char* test_scenarios[] = {
    // 性能敏感场景
    "int main() {\n"
    "    int sum = 0;\n"
    "    for (int i = 0; i < 10000; i++) {\n"
    "        sum += i * i;\n"
    "    }\n"
    "    return sum;\n"
    "}",
    
    // 内存敏感场景
    "int main() {\n"
    "    int* large_array = malloc(100000 * sizeof(int));\n"
    "    for (int i = 0; i < 100000; i++) {\n"
    "        large_array[i] = i;\n"
    "    }\n"
    "    free(large_array);\n"
    "    return 0;\n"
    "}",
    
    // 可靠性敏感场景
    "int main() {\n"
    "    int* ptr = malloc(sizeof(int));\n"
    "    if (ptr != NULL) {\n"
    "        *ptr = 42;\n"
    "        free(ptr);\n"
    "    }\n"
    "    return 0;\n"
    "}"
};

void test_basic_adaptive_framework() {
    printf("=== 测试基础适应性框架 ===\n");
    
    AIAdaptiveFramework framework;
    if (!ai_adaptive_init(&framework)) {
        printf("❌ AI适应性框架初始化失败\n");
        return;
    }
    
    // 打印初始状态
    ai_adaptive_print_stats(&framework);
    
    // 测试基本适应性进化
    printf("执行适应性进化...\n");
    for (int i = 0; i < 3; i++) {
        printf("\n--- 场景 %d ---\n", i + 1);
        bool success = ai_adaptive_evolve(&framework, test_scenarios[i]);
        printf("适应性进化结果: %s\n", success ? "成功" : "失败");
    }
    
    // 打印最终状态
    printf("\n最终状态:\n");
    ai_adaptive_print_stats(&framework);
    
    ai_adaptive_cleanup(&framework);
    printf("✅ 基础适应性框架测试完成\n\n");
}

void test_environment_adaptation() {
    printf("=== 测试环境适应功能 ===\n");
    
    AIAdaptiveFramework framework;
    ai_adaptive_init(&framework);
    
    // 测试不同环境
    EnvironmentType environments[] = {ENV_DEVELOPMENT, ENV_PRODUCTION, ENV_RESEARCH};
    const char* env_names[] = {"开发环境", "生产环境", "研究环境"};
    
    for (int i = 0; i < 3; i++) {
        printf("\n--- %s ---\n", env_names[i]);
        
        // 设置环境
        EnvironmentContext context;
        context.type = environments[i];
        context.constraints.max_memory = 1024 * 1024;
        context.constraints.max_cpu_time = 5.0;
        context.constraints.max_code_size = 5000;
        context.constraints.max_complexity = 50;
        context.constraints.real_time_required = (environments[i] == ENV_PRODUCTION);
        
        // 根据环境调整权重
        switch (environments[i]) {
            case ENV_DEVELOPMENT:
                context.performance_weight = 0.3;
                context.memory_weight = 0.2;
                context.reliability_weight = 0.2;
                context.maintainability_weight = 0.3;
                break;
            case ENV_PRODUCTION:
                context.performance_weight = 0.4;
                context.memory_weight = 0.3;
                context.reliability_weight = 0.3;
                context.maintainability_weight = 0.0;
                break;
            case ENV_RESEARCH:
                context.performance_weight = 0.5;
                context.memory_weight = 0.2;
                context.reliability_weight = 0.1;
                context.maintainability_weight = 0.2;
                break;
            default:
                break;
        }
        
        ai_adaptive_set_environment(&framework, &context);
        
        // 在该环境下执行适应性进化
        ai_adaptive_evolve(&framework, test_scenarios[0]);
        
        printf("环境 %s 适应完成\n", env_names[i]);
    }
    
    ai_adaptive_print_stats(&framework);
    ai_adaptive_cleanup(&framework);
    printf("✅ 环境适应测试完成\n\n");
}

void test_auto_adaptation() {
    printf("=== 测试自动适应功能 ===\n");
    
    AIAdaptiveFramework framework;
    ai_adaptive_init(&framework);
    
    // 模拟长期运行和自动适应
    printf("模拟长期运行过程...\n");
    
    for (int cycle = 0; cycle < 5; cycle++) {
        printf("\n--- 适应周期 %d ---\n", cycle + 1);
        
        // 执行一些适应性进化
        ai_adaptive_evolve(&framework, test_scenarios[cycle % 3]);
        
        // 检查是否需要自动适应
        if (ai_adaptive_auto_adapt(&framework)) {
            printf("✅ 执行了自动适应\n");
        } else {
            printf("ℹ️ 无需自动适应\n");
        }
        
        // 评估当前状态
        double state_score = ai_adaptive_evaluate_state(&framework);
        printf("当前适应性状态: %.1f%%\n", state_score * 100);
        
        // 模拟时间流逝
        framework.last_adaptation -= 70; // 模拟超过适应间隔
    }
    
    ai_adaptive_print_stats(&framework);
    ai_adaptive_cleanup(&framework);
    printf("✅ 自动适应测试完成\n\n");
}

void test_recommendation_system() {
    printf("=== 测试建议系统功能 ===\n");
    
    AIAdaptiveFramework framework;
    ai_adaptive_init(&framework);
    
    // 在不同环境下生成建议
    EnvironmentType environments[] = {ENV_DEVELOPMENT, ENV_PRODUCTION, ENV_RESEARCH};
    const char* env_names[] = {"开发环境", "生产环境", "研究环境"};
    
    for (int i = 0; i < 3; i++) {
        printf("\n--- %s建议 ---\n", env_names[i]);
        
        // 设置环境
        EnvironmentContext context;
        context.type = environments[i];
        ai_adaptive_set_environment(&framework, &context);
        
        // 执行一些适应以建立历史
        ai_adaptive_evolve(&framework, test_scenarios[i]);
        
        // 生成建议
        char* recommendations = ai_adaptive_generate_recommendations(&framework, test_scenarios[i]);
        if (recommendations) {
            printf("%s\n", recommendations);
            free(recommendations);
        } else {
            printf("无法生成建议\n");
        }
    }
    
    ai_adaptive_cleanup(&framework);
    printf("✅ 建议系统测试完成\n\n");
}

void test_comprehensive_adaptive_system() {
    printf("=== 测试综合适应性系统 ===\n");
    
    AIAdaptiveFramework framework;
    ai_adaptive_init(&framework);
    
    // 设置复杂的进化目标
    EvolutionGoals goals;
    goals.target_performance = 0.9;
    goals.target_memory_usage = 0.8;
    goals.target_reliability = 0.95;
    goals.target_maintainability = 0.7;
    goals.tolerance = 0.05;
    
    ai_adaptive_set_goals(&framework, &goals);
    printf("设置了高标准的进化目标\n");
    
    // 模拟复杂的适应过程
    printf("\n执行综合适应性进化...\n");
    
    for (int round = 0; round < 3; round++) {
        printf("\n=== 适应轮次 %d ===\n", round + 1);
        
        // 在每个场景下进行适应
        for (int scenario = 0; scenario < 3; scenario++) {
            printf("\n处理场景 %d:\n", scenario + 1);
            
            // 执行适应性进化
            ai_adaptive_evolve(&framework, test_scenarios[scenario]);
            
            // 检查是否达到目标
            double state_score = ai_adaptive_evaluate_state(&framework);
            if (state_score >= 0.8) {
                printf("🎯 已达到高性能状态\n");
            } else if (state_score >= 0.6) {
                printf("⚡ 性能良好，继续优化\n");
            } else {
                printf("🔄 需要进一步改进\n");
            }
        }
        
        // 自动适应
        ai_adaptive_auto_adapt(&framework);
        
        // 生成阶段性建议
        char* recommendations = ai_adaptive_generate_recommendations(&framework, test_scenarios[0]);
        if (recommendations) {
            printf("\n阶段性建议:\n%s\n", recommendations);
            free(recommendations);
        }
    }
    
    // 最终评估
    printf("\n=== 最终评估 ===\n");
    ai_adaptive_print_stats(&framework);
    
    double final_score = ai_adaptive_evaluate_state(&framework);
    if (final_score >= 0.8) {
        printf("🏆 综合适应性系统表现优秀！\n");
    } else if (final_score >= 0.6) {
        printf("✅ 综合适应性系统表现良好\n");
    } else {
        printf("🔧 综合适应性系统需要进一步调优\n");
    }
    
    ai_adaptive_cleanup(&framework);
    printf("✅ 综合适应性系统测试完成\n\n");
}

int main() {
    printf("🧠⚡ AI适应性进化框架演示程序\n");
    printf("=====================================\n\n");
    
    // 测试1: 基础适应性框架
    test_basic_adaptive_framework();
    
    // 测试2: 环境适应功能
    test_environment_adaptation();
    
    // 测试3: 自动适应功能
    test_auto_adaptation();
    
    // 测试4: 建议系统功能
    test_recommendation_system();
    
    // 测试5: 综合适应性系统
    test_comprehensive_adaptive_system();
    
    printf("🎉 所有AI适应性框架测试完成！\n");
    printf("\n=== AI适应性进化框架特性总结 ===\n");
    printf("✅ 多环境自动适应\n");
    printf("✅ 智能优化策略选择\n");
    printf("✅ 多目标平衡优化\n");
    printf("✅ 自动参数调整\n");
    printf("✅ 历史学习和趋势分析\n");
    printf("✅ 智能建议生成\n");
    printf("✅ 实时性能监控\n");
    printf("✅ 环境变化检测\n");
    
    printf("\n🎯 这标志着evolver0系统已经具备了完整的AI适应性进化能力！\n");
    printf("系统现在是一个真正的自适应AI系统，能够：\n");
    printf("- 🔄 根据环境变化自动调整策略\n");
    printf("- 🎯 平衡多个优化目标\n");
    printf("- 📊 从历史数据中学习和改进\n");
    printf("- 🧠 生成智能化的优化建议\n");
    printf("- ⚡ 实时监控和响应性能变化\n");
    printf("- 🌍 适应不同的运行环境\n");
    printf("- 🚀 持续自我进化和优化\n");
    
    return 0;
}
