/**
 * dynamic_runtime_test.c - 测试动态Runtime选择器
 */

#include <stdio.h>
#include <stdlib.h>
#include "../src/runtime/dynamic_runtime_selector.h"

void test_selection_strategy(RuntimeSelector* selector, 
                           const ProgramRequirements* requirements,
                           SelectionStrategy strategy,
                           const char* strategy_name) {
    printf("\n=== Testing %s Strategy ===\n", strategy_name);
    
    SelectionCriteria criteria = runtime_get_default_criteria(strategy);
    printf("Criteria weights: speed=%u, size=%u, memory=%u, compatibility=%u\n",
           criteria.speed_weight, criteria.size_weight, 
           criteria.memory_weight, criteria.compatibility_weight);
    
    const RuntimeInfo* selected = runtime_select_best(selector, requirements, &criteria);
    
    if (selected) {
        printf("Selected: %s v%s\n", selected->name, selected->version);
        printf("  File: %s\n", selected->filename);
        printf("  Performance: speed=%u, size=%u bytes, memory=%u KB\n",
               selected->execution_speed, selected->file_size, 
               selected->memory_footprint / 1024);
    } else {
        printf("No suitable runtime found!\n");
    }
}

int main() {
    printf("=== Dynamic Runtime Selection Test ===\n");
    
    // 初始化Runtime选择器
    RuntimeSelector* selector = runtime_selector_init();
    if (!selector) {
        printf("❌ Failed to initialize runtime selector\n");
        return 1;
    }
    
    printf("✅ Runtime selector initialized\n");
    
    // 扫描可用的Runtime
    int runtime_count = runtime_selector_scan_runtimes(selector, "bin/");
    if (runtime_count <= 0) {
        printf("❌ No runtimes found\n");
        runtime_selector_free(selector);
        return 1;
    }
    
    printf("✅ Found %d runtimes\n", runtime_count);
    
    // 列出所有可用的Runtime
    runtime_list_available(selector);
    
    // 分析程序需求 - 使用我们之前创建的测试程序
    ProgramRequirements requirements = runtime_analyze_program("tests/simple_malloc_test.astc");
    
    // 测试不同的选择策略
    test_selection_strategy(selector, &requirements, STRATEGY_FASTEST, "Fastest");
    test_selection_strategy(selector, &requirements, STRATEGY_SMALLEST, "Smallest");
    test_selection_strategy(selector, &requirements, STRATEGY_BALANCED, "Balanced");
    test_selection_strategy(selector, &requirements, STRATEGY_MEMORY_EFFICIENT, "Memory Efficient");
    test_selection_strategy(selector, &requirements, STRATEGY_COMPATIBILITY, "Compatibility");
    
    // 测试特殊需求的程序
    printf("\n=== Testing High-Performance Program ===\n");
    ProgramRequirements hp_requirements = {0};
    hp_requirements.min_memory = 10 * 1024 * 1024; // 10MB
    hp_requirements.min_stack_size = 1024 * 1024;  // 1MB
    hp_requirements.min_heap_size = 8 * 1024 * 1024; // 8MB
    hp_requirements.needs_floating_point = true;
    hp_requirements.needs_threading = true;
    hp_requirements.needs_file_io = true;
    hp_requirements.libc_functions_used = 50;
    hp_requirements.optimization_preference = 1; // 速度优先
    
    SelectionCriteria hp_criteria = runtime_get_default_criteria(STRATEGY_FASTEST);
    const RuntimeInfo* hp_runtime = runtime_select_best(selector, &hp_requirements, &hp_criteria);
    
    if (hp_runtime) {
        printf("High-performance program selected: %s\n", hp_runtime->name);
        printf("  Supports threading: %s\n", hp_runtime->supports_threading ? "yes" : "no");
        printf("  Supports FP: %s\n", hp_runtime->supports_floating_point ? "yes" : "no");
        printf("  Max libc functions: %u\n", hp_runtime->max_libc_functions);
    }
    
    // 测试内存受限的程序
    printf("\n=== Testing Memory-Constrained Program ===\n");
    ProgramRequirements mc_requirements = {0};
    mc_requirements.min_memory = 512 * 1024; // 512KB
    mc_requirements.min_stack_size = 32 * 1024; // 32KB
    mc_requirements.min_heap_size = 256 * 1024; // 256KB
    mc_requirements.needs_floating_point = false;
    mc_requirements.needs_threading = false;
    mc_requirements.needs_file_io = false;
    mc_requirements.libc_functions_used = 5;
    mc_requirements.optimization_preference = 0; // 大小优先
    
    SelectionCriteria mc_criteria = runtime_get_default_criteria(STRATEGY_SMALLEST);
    const RuntimeInfo* mc_runtime = runtime_select_best(selector, &mc_requirements, &mc_criteria);
    
    if (mc_runtime) {
        printf("Memory-constrained program selected: %s\n", mc_runtime->name);
        printf("  File size: %u bytes\n", mc_runtime->file_size);
        printf("  Memory footprint: %u KB\n", mc_runtime->memory_footprint / 1024);
        printf("  Startup time: %u μs\n", mc_runtime->startup_time);
    }
    
    // 测试不兼容的程序
    printf("\n=== Testing Incompatible Program ===\n");
    ProgramRequirements incompatible_requirements = {0};
    incompatible_requirements.min_memory = 200 * 1024 * 1024; // 200MB (超过限制)
    incompatible_requirements.needs_graphics = true; // 大多数runtime不支持
    incompatible_requirements.libc_functions_used = 1000; // 超过支持范围
    
    SelectionCriteria incompatible_criteria = runtime_get_default_criteria(STRATEGY_BALANCED);
    const RuntimeInfo* incompatible_runtime = runtime_select_best(selector, &incompatible_requirements, &incompatible_criteria);
    
    if (!incompatible_runtime) {
        printf("✅ Correctly identified incompatible program\n");
    } else {
        printf("⚠️ Unexpectedly found runtime for incompatible program: %s\n", incompatible_runtime->name);
    }
    
    // 清理
    runtime_selector_free(selector);
    
    printf("\n=== All Dynamic Runtime Selection Tests Completed! ===\n");
    return 0;
}
