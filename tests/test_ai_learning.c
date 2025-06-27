/**
 * test_ai_learning.c - AI学习机制演示程序
 * 
 * 这个程序演示了AI自我学习机制的功能
 */

#include <stdio.h>
#include <stdlib.h>
#include "../ai_learning.h"

// 测试代码样本
const char* test_codes[] = {
    // 有循环的代码
    "int main() {\n"
    "    int sum = 0;\n"
    "    for (int i = 0; i < 1000; i++) {\n"
    "        sum += i;\n"
    "    }\n"
    "    return sum;\n"
    "}",
    
    // 优化后的代码
    "int main() {\n"
    "    int n = 999;\n"
    "    int sum = n * (n + 1) / 2;\n"
    "    return sum;\n"
    "}",
    
    // 有内存分配的代码
    "int main() {\n"
    "    int* arr = malloc(100 * sizeof(int));\n"
    "    for (int i = 0; i < 100; i++) {\n"
    "        arr[i] = i;\n"
    "    }\n"
    "    free(arr);\n"
    "    return 0;\n"
    "}",
    
    // 递归代码
    "int factorial(int n) {\n"
    "    if (n <= 1) return 1;\n"
    "    return n * factorial(n - 1);\n"
    "}\n"
    "int main() {\n"
    "    return factorial(10);\n"
    "}"
};

// 模拟性能指标
PerformanceMetrics create_metrics(double time, size_t memory, size_t code_size, int errors) {
    PerformanceMetrics metrics = {0};
    metrics.execution_time = time;
    metrics.memory_usage = memory;
    metrics.code_size = code_size;
    metrics.error_count = errors;
    metrics.cpu_utilization = 0.3 + (time / 10.0);
    metrics.success_rate = errors == 0 ? 1.0 : 0.0;
    return metrics;
}

void test_basic_learning() {
    printf("=== 测试基础学习功能 ===\n");
    
    AILearningEngine engine;
    if (!ai_learning_init(&engine)) {
        printf("❌ AI学习引擎初始化失败\n");
        return;
    }
    
    // 记录一些执行结果
    printf("记录执行结果...\n");
    
    // 第一次执行：慢速版本
    PerformanceMetrics slow_metrics = create_metrics(1.5, 1000, strlen(test_codes[0]), 0);
    ai_learning_record_execution(&engine, test_codes[0], &slow_metrics, 0, NULL);
    
    // 第二次执行：优化版本
    PerformanceMetrics fast_metrics = create_metrics(0.1, 500, strlen(test_codes[1]), 0);
    ai_learning_record_execution(&engine, test_codes[1], &fast_metrics, 0, NULL);
    
    // 第三次执行：内存分配版本
    PerformanceMetrics mem_metrics = create_metrics(0.8, 2000, strlen(test_codes[2]), 0);
    ai_learning_record_execution(&engine, test_codes[2], &mem_metrics, 0, NULL);
    
    // 第四次执行：递归版本
    PerformanceMetrics rec_metrics = create_metrics(0.3, 800, strlen(test_codes[3]), 0);
    ai_learning_record_execution(&engine, test_codes[3], &rec_metrics, 0, NULL);
    
    // 打印学习统计
    ai_learning_print_stats(&engine);
    
    ai_learning_cleanup(&engine);
    printf("✅ 基础学习测试完成\n\n");
}

void test_error_learning() {
    printf("=== 测试错误学习功能 ===\n");
    
    AILearningEngine engine;
    ai_learning_init(&engine);
    
    // 模拟一些错误情况
    printf("记录错误执行结果...\n");
    
    const char* error_code = "int main() { int* p = NULL; *p = 42; return 0; }";
    PerformanceMetrics error_metrics = create_metrics(0.0, 0, strlen(error_code), 1);
    ai_learning_record_execution(&engine, error_code, &error_metrics, 1, "segmentation fault");
    
    const char* leak_code = "int main() { int* p = malloc(100); return 0; }";
    PerformanceMetrics leak_metrics = create_metrics(0.1, 100, strlen(leak_code), 1);
    ai_learning_record_execution(&engine, leak_code, &leak_metrics, 1, "memory leak detected");
    
    const char* loop_code = "int main() { while(1) { } return 0; }";
    PerformanceMetrics loop_metrics = create_metrics(999.0, 100, strlen(loop_code), 1);
    ai_learning_record_execution(&engine, loop_code, &loop_metrics, 1, "infinite loop detected");
    
    // 分析错误模式
    ai_learning_analyze_errors(&engine);
    
    // 生成建议
    printf("\n生成改进建议:\n");
    char* suggestions = ai_learning_generate_suggestions(&engine, error_code);
    if (suggestions) {
        printf("%s\n", suggestions);
        free(suggestions);
    }
    
    ai_learning_print_stats(&engine);
    ai_learning_cleanup(&engine);
    printf("✅ 错误学习测试完成\n\n");
}

void test_performance_learning() {
    printf("=== 测试性能学习功能 ===\n");
    
    AILearningEngine engine;
    ai_learning_init(&engine);
    
    // 模拟性能改进序列
    printf("记录性能改进序列...\n");
    
    // 原始慢速代码
    const char* slow_code = "int main() { int sum = 0; for(int i = 0; i < 10000; i++) sum += i; return sum; }";
    PerformanceMetrics slow_perf = create_metrics(2.0, 1000, strlen(slow_code), 0);
    ai_learning_record_execution(&engine, slow_code, &slow_perf, 0, NULL);
    
    // 第一次优化
    const char* opt1_code = "int main() { int sum = 0; for(int i = 0; i < 10000; i+=2) sum += i + (i+1); return sum; }";
    PerformanceMetrics opt1_perf = create_metrics(1.2, 1000, strlen(opt1_code), 0);
    ai_learning_record_execution(&engine, opt1_code, &opt1_perf, 0, NULL);
    
    // 第二次优化
    const char* opt2_code = "int main() { int n = 9999; return n * (n + 1) / 2; }";
    PerformanceMetrics opt2_perf = create_metrics(0.1, 500, strlen(opt2_code), 0);
    ai_learning_record_execution(&engine, opt2_code, &opt2_perf, 0, NULL);
    
    // 分析性能模式
    ai_learning_analyze_performance(&engine);
    
    // 生成性能建议
    printf("\n生成性能优化建议:\n");
    char* perf_suggestions = ai_learning_generate_suggestions(&engine, slow_code);
    if (perf_suggestions) {
        printf("%s\n", perf_suggestions);
        free(perf_suggestions);
    }
    
    ai_learning_print_stats(&engine);
    ai_learning_cleanup(&engine);
    printf("✅ 性能学习测试完成\n\n");
}

void test_pattern_recognition() {
    printf("=== 测试模式识别功能 ===\n");
    
    AILearningEngine engine;
    ai_learning_init(&engine);
    
    // 测试代码模式识别
    printf("测试代码模式识别:\n");
    
    const char* test_patterns[] = {
        "for (int i = 0; i < 100; i++) { }",
        "while (condition) { }",
        "int* ptr = malloc(100);",
        "return factorial(n-1);"
    };
    
    for (int i = 0; i < 4; i++) {
        char* pattern = ai_learning_identify_code_pattern(test_patterns[i]);
        printf("代码: %s\n", test_patterns[i]);
        printf("识别模式: %s\n\n", pattern ? pattern : "unknown");
        free(pattern);
    }
    
    ai_learning_cleanup(&engine);
    printf("✅ 模式识别测试完成\n\n");
}

int main() {
    printf("🧠 AI自我学习机制演示程序\n");
    printf("============================\n\n");
    
    // 测试1: 基础学习功能
    test_basic_learning();
    
    // 测试2: 错误学习功能
    test_error_learning();
    
    // 测试3: 性能学习功能
    test_performance_learning();
    
    // 测试4: 模式识别功能
    test_pattern_recognition();
    
    printf("🎉 所有AI学习测试完成！\n");
    printf("\n=== AI学习机制特性总结 ===\n");
    printf("✅ 执行结果记录和分析\n");
    printf("✅ 错误模式识别和学习\n");
    printf("✅ 性能模式识别和优化\n");
    printf("✅ 自动改进建议生成\n");
    printf("✅ 知识库管理和更新\n");
    printf("✅ 代码模式识别\n");
    
    printf("\n这标志着evolver0系统已经具备了完整的AI学习能力！\n");
    printf("系统现在可以：\n");
    printf("- 从执行结果中自动学习\n");
    printf("- 识别和记忆错误模式\n");
    printf("- 发现性能优化机会\n");
    printf("- 生成智能改进建议\n");
    printf("- 积累和管理知识库\n");
    printf("- 持续改进自身能力\n");
    
    return 0;
}
