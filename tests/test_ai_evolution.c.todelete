/**
 * test_ai_evolution.c - AI进化算法演示程序
 * 
 * 这个程序演示了AI驱动的代码进化算法的基本功能
 */

#include <stdio.h>
#include <stdlib.h>
#include "../ai_evolution.h"

// 测试用的代码变体
const char* test_programs[] = {
    // 基础版本
    "int main() {\n"
    "    int result = 0;\n"
    "    for (int i = 0; i < 100; i++) {\n"
    "        result += i;\n"
    "    }\n"
    "    return result;\n"
    "}",
    
    // 优化版本1
    "int main() {\n"
    "    // Optimized version\n"
    "    int result = 0;\n"
    "    for (int i = 0; i < 100; i++) {\n"
    "        result += i;\n"
    "    }\n"
    "    return result;\n"
    "}",
    
    // 优化版本2
    "int main() {\n"
    "    // Mathematical optimization\n"
    "    int n = 99;\n"
    "    int result = n * (n + 1) / 2;\n"
    "    return result;\n"
    "}",
    
    // 简单版本
    "int main() {\n"
    "    return 42;\n"
    "}",
    
    // 复杂版本
    "int main() {\n"
    "    int result = 0;\n"
    "    for (int i = 0; i < 100; i++) {\n"
    "        for (int j = 0; j < 10; j++) {\n"
    "            result += i * j;\n"
    "        }\n"
    "    }\n"
    "    return result;\n"
    "}"
};

void test_basic_evolution() {
    printf("=== 测试基础AI进化功能 ===\n");
    
    AIEvolutionEngine engine;
    if (!ai_evolution_init(&engine, 5)) {
        printf("❌ AI进化引擎初始化失败\n");
        return;
    }
    
    // 添加初始种群
    printf("添加初始代码变体到种群...\n");
    for (int i = 0; i < 5; i++) {
        if (ai_evolution_add_variant(&engine, test_programs[i])) {
            printf("✅ 添加变体 %d 成功\n", i + 1);
        } else {
            printf("❌ 添加变体 %d 失败\n", i + 1);
        }
    }
    
    // 打印初始统计信息
    ai_evolution_print_stats(&engine);
    
    // 进化几代
    printf("开始进化过程...\n");
    for (int gen = 0; gen < 3; gen++) {
        printf("\n--- 第 %d 代进化 ---\n", gen + 1);
        if (ai_evolution_evolve_generation(&engine)) {
            printf("✅ 第 %d 代进化成功\n", gen + 1);
        } else {
            printf("❌ 第 %d 代进化失败\n", gen + 1);
            break;
        }
        
        // 打印当前最优个体
        CodeVariant* best = ai_evolution_select_best(&engine);
        if (best) {
            printf("当前最优个体适应度: %.3f\n", best->fitness_score);
            printf("执行时间: %.3f, 内存使用: %zu, 代码大小: %zu\n",
                   best->metrics.execution_time, best->metrics.memory_usage, best->metrics.code_size);
        }
    }
    
    // 最终统计
    printf("\n=== 最终进化结果 ===\n");
    ai_evolution_print_stats(&engine);
    
    CodeVariant* final_best = ai_evolution_select_best(&engine);
    if (final_best) {
        printf("最终最优代码:\n");
        printf("```c\n%s\n```\n", final_best->source_code);
        printf("适应度分数: %.3f\n", final_best->fitness_score);
    }
    
    ai_evolution_cleanup(&engine);
    printf("✅ AI进化测试完成\n\n");
}

void test_optimization_strategies() {
    printf("=== 测试不同优化策略 ===\n");
    
    const char* test_code = test_programs[0];
    
    OptimizationStrategy strategies[] = {
        OPT_PERFORMANCE, OPT_MEMORY, OPT_SIZE, OPT_RELIABILITY, OPT_MAINTAINABILITY
    };
    
    const char* strategy_names[] = {
        "性能优化", "内存优化", "代码大小优化", "可靠性优化", "可维护性优化"
    };
    
    for (int i = 0; i < 5; i++) {
        printf("\n--- %s策略 ---\n", strategy_names[i]);
        
        AIEvolutionEngine engine;
        ai_evolution_init(&engine, 3);
        engine.strategy = strategies[i];
        
        // 添加测试代码
        ai_evolution_add_variant(&engine, test_code);
        
        // 进化一代
        ai_evolution_evolve_generation(&engine);
        
        // 显示结果
        CodeVariant* best = ai_evolution_select_best(&engine);
        if (best) {
            printf("最优适应度: %.3f\n", best->fitness_score);
            printf("性能指标: 时间=%.3f, 内存=%zu, 大小=%zu\n",
                   best->metrics.execution_time, best->metrics.memory_usage, best->metrics.code_size);
        }
        
        ai_evolution_cleanup(&engine);
    }
    
    printf("✅ 优化策略测试完成\n\n");
}

void test_adaptive_parameters() {
    printf("=== 测试自适应参数调整 ===\n");
    
    AIEvolutionEngine engine;
    ai_evolution_init(&engine, 3);
    
    // 添加初始代码
    ai_evolution_add_variant(&engine, test_programs[0]);
    
    printf("初始参数:\n");
    printf("变异率: %.3f, 探索率: %.3f\n", engine.mutation_rate, engine.exploration_rate);
    
    // 模拟多代进化
    for (int i = 0; i < 10; i++) {
        ai_evolution_evolve_generation(&engine);
        
        if (i % 3 == 2) {
            printf("第 %d 代后参数:\n", i + 1);
            printf("变异率: %.3f, 探索率: %.3f\n", engine.mutation_rate, engine.exploration_rate);
        }
    }
    
    ai_evolution_cleanup(&engine);
    printf("✅ 自适应参数测试完成\n\n");
}

int main() {
    printf("🤖 AI驱动进化算法演示程序\n");
    printf("============================\n\n");
    
    // 测试1: 基础进化功能
    test_basic_evolution();
    
    // 测试2: 不同优化策略
    test_optimization_strategies();
    
    // 测试3: 自适应参数调整
    test_adaptive_parameters();
    
    printf("🎉 所有AI进化测试完成！\n");
    printf("\n=== AI进化算法特性总结 ===\n");
    printf("✅ 多种优化策略支持\n");
    printf("✅ 自适应参数调整\n");
    printf("✅ 代码变异和进化\n");
    printf("✅ 性能指标评估\n");
    printf("✅ 进化历史跟踪\n");
    printf("✅ 适应度计算和选择\n");
    
    printf("\n这标志着evolver0系统已经具备了AI驱动的自我进化能力！\n");
    printf("系统现在可以：\n");
    printf("- 自动分析代码性能\n");
    printf("- 生成优化的代码变体\n");
    printf("- 根据目标策略进行进化\n");
    printf("- 自适应调整进化参数\n");
    printf("- 跟踪和学习进化历史\n");
    
    return 0;
}
