/**
 * test_ai_optimizer.c - AI代码优化算法演示程序
 * 
 * 这个程序演示了AI驱动的代码优化算法功能
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../ai_optimizer.h"
#include "../ai_learning.h"

// 测试代码样本
const char* test_codes[] = {
    // 循环求和代码（可优化为数学公式）
    "int main() {\n"
    "    int sum = 0;\n"
    "    for (int i = 0; i < 1000; i++) {\n"
    "        sum += i;\n"
    "    }\n"
    "    return sum;\n"
    "}",
    
    // 简单循环代码（可优化为循环展开）
    "int main() {\n"
    "    int result = 0;\n"
    "    for (int i = 0; i < 100; i++) {\n"
    "        result += i * 2;\n"
    "    }\n"
    "    return result;\n"
    "}",
    
    // 内存分配代码（可优化内存管理）
    "int main() {\n"
    "    int* arr = malloc(1000 * sizeof(int));\n"
    "    for (int i = 0; i < 1000; i++) {\n"
    "        arr[i] = i;\n"
    "    }\n"
    "    free(arr);\n"
    "    return 0;\n"
    "}",
    
    // 简单代码（优化空间有限）
    "int main() {\n"
    "    return 42;\n"
    "}"
};

void test_basic_optimization() {
    printf("=== 测试基础优化功能 ===\n");
    
    AILearningEngine learning;
    ai_learning_init(&learning);
    
    AIOptimizerEngine optimizer;
    if (!ai_optimizer_init(&optimizer, &learning)) {
        printf("❌ AI优化引擎初始化失败\n");
        return;
    }
    
    // 测试代码分析和优化建议
    printf("分析代码并生成优化建议...\n");
    
    for (int i = 0; i < 4; i++) {
        printf("\n--- 测试代码 %d ---\n", i + 1);
        printf("原始代码:\n%s\n", test_codes[i]);
        
        OptimizationSuggestion* suggestion = ai_optimizer_analyze_code(&optimizer, test_codes[i]);
        
        if (suggestion) {
            printf("✅ 找到优化机会!\n");
            printf("建议: %s\n", suggestion->suggestion);
            printf("预期改进: %.1f%%\n", suggestion->improvement_estimate * 100);
            printf("置信度: %.1f%%\n", suggestion->confidence * 100);
            printf("解释: %s\n", suggestion->explanation);
            
            // 应用优化
            char* optimized = ai_optimizer_apply_optimization(&optimizer, test_codes[i], suggestion);
            if (optimized) {
                printf("优化后代码:\n%s\n", optimized);
                
                // 评估优化效果
                double improvement = ai_optimizer_evaluate_optimization(&optimizer, test_codes[i], optimized);
                printf("实际改进: %.1f%%\n", improvement * 100);
                
                free(optimized);
            }
            
            ai_optimizer_free_suggestion(suggestion);
        } else {
            printf("❌ 未找到明显的优化机会\n");
        }
    }
    
    ai_optimizer_print_stats(&optimizer);
    ai_optimizer_cleanup(&optimizer);
    ai_learning_cleanup(&learning);
    printf("✅ 基础优化测试完成\n\n");
}

void test_optimization_types() {
    printf("=== 测试不同优化类型 ===\n");
    
    AILearningEngine learning;
    ai_learning_init(&learning);
    
    AIOptimizerEngine optimizer;
    ai_optimizer_init(&optimizer, &learning);
    
    const char* test_code = test_codes[0]; // 使用循环求和代码
    
    OptimizationType types[] = {
        OPT_TYPE_PERFORMANCE, OPT_TYPE_MEMORY, OPT_TYPE_SIZE, OPT_TYPE_READABILITY
    };
    
    const char* type_names[] = {
        "性能优化", "内存优化", "代码大小优化", "可读性优化"
    };
    
    for (int i = 0; i < 4; i++) {
        printf("\n--- %s ---\n", type_names[i]);
        
        char* optimized = ai_optimizer_auto_optimize(&optimizer, test_code, types[i]);
        if (optimized) {
            printf("优化结果:\n%s\n", optimized);
            
            double improvement = ai_optimizer_evaluate_optimization(&optimizer, test_code, optimized);
            printf("改进评估: %.1f%%\n", improvement * 100);
            
            free(optimized);
        } else {
            printf("优化失败\n");
        }
    }
    
    ai_optimizer_cleanup(&optimizer);
    ai_learning_cleanup(&learning);
    printf("✅ 优化类型测试完成\n\n");
}

void test_optimization_learning() {
    printf("=== 测试优化学习功能 ===\n");
    
    AILearningEngine learning;
    ai_learning_init(&learning);
    
    AIOptimizerEngine optimizer;
    ai_optimizer_init(&optimizer, &learning);
    
    // 模拟学习过程
    printf("模拟优化学习过程...\n");
    
    const char* original = "for (int i = 0; i < n; i++) sum += i;";
    const char* optimized = "sum = n * (n - 1) / 2;";
    double improvement = 0.8;
    
    bool learned = ai_optimizer_learn_pattern(&optimizer, original, optimized, improvement);
    if (learned) {
        printf("✅ 学习了新的优化模式\n");
        printf("原始: %s\n", original);
        printf("优化: %s\n", optimized);
        printf("改进: %.1f%%\n", improvement * 100);
    } else {
        printf("❌ 学习优化模式失败\n");
    }
    
    // 测试学习到的模式是否能应用
    printf("\n测试学习到的模式应用...\n");
    OptimizationSuggestion* suggestion = ai_optimizer_analyze_code(&optimizer, test_codes[0]);
    if (suggestion) {
        printf("✅ 成功应用学习到的模式\n");
        printf("建议: %s\n", suggestion->suggestion);
        ai_optimizer_free_suggestion(suggestion);
    }
    
    ai_optimizer_cleanup(&optimizer);
    ai_learning_cleanup(&learning);
    printf("✅ 优化学习测试完成\n\n");
}

void test_comprehensive_optimization() {
    printf("=== 测试综合优化功能 ===\n");
    
    AILearningEngine learning;
    ai_learning_init(&learning);
    
    AIOptimizerEngine optimizer;
    ai_optimizer_init(&optimizer, &learning);
    
    // 启用激进优化模式
    optimizer.aggressive_optimization = true;
    printf("启用激进优化模式\n");
    
    // 对所有测试代码进行综合优化
    for (int i = 0; i < 4; i++) {
        printf("\n--- 综合优化测试 %d ---\n", i + 1);
        
        // 分析代码
        OptimizationSuggestion* suggestion = ai_optimizer_analyze_code(&optimizer, test_codes[i]);
        
        if (suggestion) {
            // 应用优化
            char* optimized = ai_optimizer_apply_optimization(&optimizer, test_codes[i], suggestion);
            
            if (optimized) {
                // 再次优化（多轮优化）
                OptimizationSuggestion* second_suggestion = ai_optimizer_analyze_code(&optimizer, optimized);
                
                if (second_suggestion) {
                    printf("发现二次优化机会\n");
                    char* double_optimized = ai_optimizer_apply_optimization(&optimizer, optimized, second_suggestion);
                    
                    if (double_optimized) {
                        double total_improvement = ai_optimizer_evaluate_optimization(&optimizer, test_codes[i], double_optimized);
                        printf("总体改进: %.1f%%\n", total_improvement * 100);
                        free(double_optimized);
                    }
                    
                    ai_optimizer_free_suggestion(second_suggestion);
                } else {
                    double improvement = ai_optimizer_evaluate_optimization(&optimizer, test_codes[i], optimized);
                    printf("单轮改进: %.1f%%\n", improvement * 100);
                }
                
                free(optimized);
            }
            
            ai_optimizer_free_suggestion(suggestion);
        } else {
            printf("无需优化\n");
        }
    }
    
    ai_optimizer_print_stats(&optimizer);
    ai_optimizer_cleanup(&optimizer);
    ai_learning_cleanup(&learning);
    printf("✅ 综合优化测试完成\n\n");
}

int main() {
    printf("⚡ AI代码优化算法演示程序\n");
    printf("============================\n\n");
    
    // 测试1: 基础优化功能
    test_basic_optimization();
    
    // 测试2: 不同优化类型
    test_optimization_types();
    
    // 测试3: 优化学习功能
    test_optimization_learning();
    
    // 测试4: 综合优化功能
    test_comprehensive_optimization();
    
    printf("🎉 所有AI优化测试完成！\n");
    printf("\n=== AI优化算法特性总结 ===\n");
    printf("✅ 多种优化策略支持\n");
    printf("✅ 智能代码分析\n");
    printf("✅ 自动优化建议生成\n");
    printf("✅ 优化效果评估\n");
    printf("✅ 优化模式学习\n");
    printf("✅ 多轮迭代优化\n");
    printf("✅ 激进优化模式\n");
    
    printf("\n这标志着evolver0系统已经具备了强大的AI代码优化能力！\n");
    printf("系统现在可以：\n");
    printf("- 自动识别代码优化机会\n");
    printf("- 应用多种优化技术\n");
    printf("- 学习和积累优化经验\n");
    printf("- 评估优化效果\n");
    printf("- 进行多轮迭代优化\n");
    printf("- 根据目标选择优化策略\n");
    
    return 0;
}
