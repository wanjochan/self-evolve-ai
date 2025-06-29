/**
 * dynamic_runtime_selector.c - 动态Runtime选择器实现
 */

#include "dynamic_runtime_selector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ===============================================
// 初始化和清理
// ===============================================

RuntimeSelector* runtime_selector_init(void) {
    RuntimeSelector* selector = calloc(1, sizeof(RuntimeSelector));
    if (!selector) return NULL;
    
    selector->capacity = 16;
    selector->runtimes = calloc(selector->capacity, sizeof(RuntimeInfo));
    if (!selector->runtimes) {
        free(selector);
        return NULL;
    }
    
    // 检测当前环境
    selector->current_arch = rt_detect_architecture();
    selector->current_os = rt_detect_os();
    selector->current_abi = rt_detect_abi();
    
    // 估算系统资源
    selector->available_memory = 1024 * 1024 * 1024; // 假设1GB可用内存
    selector->cpu_cores = 4; // 假设4核CPU
    selector->has_fpu = true; // 现代CPU都有FPU
    
    printf("Runtime selector initialized for %s/%s/%s\n",
           rt_get_architecture_name(selector->current_arch),
           rt_get_os_name(selector->current_os),
           rt_get_abi_name(selector->current_abi));
    
    return selector;
}

void runtime_selector_free(RuntimeSelector* selector) {
    if (selector) {
        free(selector->runtimes);
        free(selector);
    }
}

// ===============================================
// Runtime注册和扫描
// ===============================================

int runtime_selector_register_runtime(RuntimeSelector* selector, const RuntimeInfo* runtime) {
    if (!selector || !runtime) return -1;
    
    // 扩容检查
    if (selector->runtime_count >= selector->capacity) {
        selector->capacity *= 2;
        RuntimeInfo* new_runtimes = realloc(selector->runtimes, 
                                           selector->capacity * sizeof(RuntimeInfo));
        if (!new_runtimes) return -1;
        selector->runtimes = new_runtimes;
    }
    
    // 复制Runtime信息
    selector->runtimes[selector->runtime_count] = *runtime;
    selector->runtime_count++;
    
    printf("Registered runtime: %s v%s (%s)\n", 
           runtime->name, runtime->version, runtime->filename);
    
    return 0;
}

int runtime_selector_scan_runtimes(RuntimeSelector* selector, const char* runtime_dir) {
    if (!selector || !runtime_dir) return -1;
    
    printf("Scanning runtime directory: %s\n", runtime_dir);
    
    // 模拟扫描几个常见的Runtime文件
    const char* runtime_files[] = {
        "simple_runtime_enhanced_v2.exe",
        "enhanced_runtime_with_libc_v2.exe",
        "c99_runtime.exe",
        "evolver0_runtime.exe"
    };
    
    for (int i = 0; i < 4; i++) {
        RuntimeInfo runtime = {0};
        
        // 设置基本信息
        snprintf(runtime.name, sizeof(runtime.name), "Runtime_%d", i + 1);
        strcpy(runtime.version, "1.0.0");
        snprintf(runtime.filename, sizeof(runtime.filename), "bin/%s", runtime_files[i]);
        
        // 设置架构信息
        runtime.architecture = selector->current_arch;
        runtime.os = selector->current_os;
        runtime.abi = selector->current_abi;
        
        // 设置性能特征
        runtime.file_size = 50000 + i * 10000; // 模拟不同大小
        runtime.memory_footprint = 1024 * 1024 + i * 512 * 1024; // 1-3MB
        runtime.startup_time = 1000 + i * 500; // 1-3.5ms
        runtime.execution_speed = 90 - i * 10; // 90, 80, 70, 60
        
        // 设置功能支持
        runtime.supports_floating_point = true;
        runtime.supports_threading = (i >= 1);
        runtime.supports_file_io = true;
        runtime.supports_network = (i >= 2);
        runtime.supports_graphics = (i >= 3);
        runtime.max_libc_functions = 50 + i * 25;
        
        // 设置资源限制
        runtime.max_memory = 100 * 1024 * 1024; // 100MB
        runtime.max_stack_size = 8 * 1024 * 1024; // 8MB
        runtime.max_heap_size = 64 * 1024 * 1024; // 64MB
        
        // 检查可用性
        runtime.available = runtime_verify_availability(&runtime);
        
        runtime_selector_register_runtime(selector, &runtime);
    }
    
    printf("Scanned %d runtime files\n", selector->runtime_count);
    return selector->runtime_count;
}

// ===============================================
// 程序需求分析
// ===============================================

ProgramRequirements runtime_analyze_program(const char* program_file) {
    ProgramRequirements req = {0};
    
    printf("Analyzing program requirements: %s\n", program_file);
    
    // 简化的程序分析 - 实际实现应该解析ASTC文件
    FILE* fp = fopen(program_file, "rb");
    if (!fp) {
        printf("Warning: Cannot open program file for analysis\n");
        return req;
    }
    
    // 获取文件大小
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fclose(fp);
    
    // 基于文件大小估算需求
    req.min_memory = file_size * 2; // 假设需要2倍文件大小的内存
    req.min_stack_size = 64 * 1024; // 64KB栈
    req.min_heap_size = 1024 * 1024; // 1MB堆
    
    // 基于文件名推测功能需求
    if (strstr(program_file, "malloc") || strstr(program_file, "memory")) {
        req.min_heap_size *= 4; // 内存密集型程序
    }
    
    if (strstr(program_file, "float") || strstr(program_file, "math")) {
        req.needs_floating_point = true;
    }
    
    if (strstr(program_file, "file") || strstr(program_file, "io")) {
        req.needs_file_io = true;
    }
    
    // 估算libc函数使用
    req.libc_functions_used = 10; // 基本假设
    req.optimization_preference = 2; // 平衡优化
    
    printf("Program requirements:\n");
    printf("  Min memory: %u bytes\n", req.min_memory);
    printf("  Min stack: %u bytes\n", req.min_stack_size);
    printf("  Min heap: %u bytes\n", req.min_heap_size);
    printf("  Needs FP: %s\n", req.needs_floating_point ? "yes" : "no");
    printf("  Needs File I/O: %s\n", req.needs_file_io ? "yes" : "no");
    
    return req;
}

// ===============================================
// 兼容性和性能评分
// ===============================================

uint32_t runtime_calculate_compatibility(const RuntimeInfo* runtime, 
                                        const ProgramRequirements* requirements) {
    if (!runtime || !requirements) return 0;
    
    uint32_t score = 100;
    
    // 检查架构兼容性
    if (!runtime->available) {
        return 0; // 不可用的Runtime得分为0
    }
    
    // 检查功能需求
    if (requirements->needs_floating_point && !runtime->supports_floating_point) {
        score -= 30;
    }
    
    if (requirements->needs_threading && !runtime->supports_threading) {
        score -= 20;
    }
    
    if (requirements->needs_file_io && !runtime->supports_file_io) {
        score -= 25;
    }
    
    if (requirements->needs_network && !runtime->supports_network) {
        score -= 15;
    }
    
    if (requirements->needs_graphics && !runtime->supports_graphics) {
        score -= 10;
    }
    
    // 检查资源需求
    if (requirements->min_memory > runtime->max_memory) {
        score -= 40; // 内存不足是严重问题
    }
    
    if (requirements->min_stack_size > runtime->max_stack_size) {
        score -= 20;
    }
    
    if (requirements->min_heap_size > runtime->max_heap_size) {
        score -= 20;
    }
    
    // 检查libc函数支持
    if (requirements->libc_functions_used > runtime->max_libc_functions) {
        score -= 15;
    }
    
    return score > 0 ? score : 0;
}

uint32_t runtime_calculate_performance_score(const RuntimeInfo* runtime,
                                            const SelectionCriteria* criteria) {
    if (!runtime || !criteria) return 0;
    
    uint32_t score = 0;
    uint32_t total_weight = criteria->speed_weight + criteria->size_weight + 
                           criteria->memory_weight + criteria->compatibility_weight;
    
    if (total_weight == 0) return 0;
    
    // 速度评分 (执行速度越高越好)
    uint32_t speed_score = runtime->execution_speed;
    score += (speed_score * criteria->speed_weight) / total_weight;
    
    // 大小评分 (文件越小越好)
    uint32_t size_score = 100 - (runtime->file_size / 1000); // 简化计算
    if (size_score > 100) size_score = 100;
    score += (size_score * criteria->size_weight) / total_weight;
    
    // 内存评分 (内存占用越小越好)
    uint32_t memory_score = 100 - (runtime->memory_footprint / (1024 * 1024));
    if (memory_score > 100) memory_score = 100;
    score += (memory_score * criteria->memory_weight) / total_weight;
    
    // 兼容性评分
    score += (runtime->compatibility_score * criteria->compatibility_weight) / total_weight;
    
    return score;
}

// ===============================================
// Runtime选择
// ===============================================

const RuntimeInfo* runtime_select_best(RuntimeSelector* selector, 
                                      const ProgramRequirements* requirements,
                                      const SelectionCriteria* criteria) {
    if (!selector || !requirements || !criteria) return NULL;
    
    printf("Selecting best runtime from %d candidates...\n", selector->runtime_count);
    
    const RuntimeInfo* best_runtime = NULL;
    uint32_t best_score = 0;
    
    for (uint32_t i = 0; i < selector->runtime_count; i++) {
        RuntimeInfo* runtime = &selector->runtimes[i];
        
        // 计算兼容性评分
        runtime->compatibility_score = runtime_calculate_compatibility(runtime, requirements);
        
        if (runtime->compatibility_score == 0) {
            printf("  %s: incompatible\n", runtime->name);
            continue; // 不兼容的Runtime跳过
        }
        
        // 计算综合性能评分
        uint32_t performance_score = runtime_calculate_performance_score(runtime, criteria);
        
        printf("  %s: compatibility=%u, performance=%u\n", 
               runtime->name, runtime->compatibility_score, performance_score);
        
        if (performance_score > best_score) {
            best_score = performance_score;
            best_runtime = runtime;
        }
    }
    
    if (best_runtime) {
        printf("Selected runtime: %s (score: %u)\n", best_runtime->name, best_score);
    } else {
        printf("No compatible runtime found!\n");
    }
    
    return best_runtime;
}

// ===============================================
// 辅助函数
// ===============================================

bool runtime_verify_availability(const RuntimeInfo* runtime) {
    if (!runtime) return false;
    
    // 检查文件是否存在
    FILE* fp = fopen(runtime->filename, "rb");
    if (fp) {
        fclose(fp);
        return true;
    }
    
    return false;
}

SelectionCriteria runtime_get_default_criteria(SelectionStrategy strategy) {
    SelectionCriteria criteria = {0};
    criteria.strategy = strategy;
    
    switch (strategy) {
        case STRATEGY_FASTEST:
            criteria.speed_weight = 70;
            criteria.size_weight = 10;
            criteria.memory_weight = 10;
            criteria.compatibility_weight = 10;
            break;
            
        case STRATEGY_SMALLEST:
            criteria.speed_weight = 10;
            criteria.size_weight = 70;
            criteria.memory_weight = 10;
            criteria.compatibility_weight = 10;
            break;
            
        case STRATEGY_BALANCED:
            criteria.speed_weight = 25;
            criteria.size_weight = 25;
            criteria.memory_weight = 25;
            criteria.compatibility_weight = 25;
            break;
            
        case STRATEGY_MEMORY_EFFICIENT:
            criteria.speed_weight = 10;
            criteria.size_weight = 20;
            criteria.memory_weight = 60;
            criteria.compatibility_weight = 10;
            break;
            
        case STRATEGY_COMPATIBILITY:
            criteria.speed_weight = 10;
            criteria.size_weight = 10;
            criteria.memory_weight = 10;
            criteria.compatibility_weight = 70;
            break;
            
        default:
            criteria.speed_weight = 25;
            criteria.size_weight = 25;
            criteria.memory_weight = 25;
            criteria.compatibility_weight = 25;
            break;
    }
    
    return criteria;
}

void runtime_list_available(const RuntimeSelector* selector) {
    if (!selector) return;
    
    printf("Available runtimes (%d):\n", selector->runtime_count);
    for (uint32_t i = 0; i < selector->runtime_count; i++) {
        const RuntimeInfo* runtime = &selector->runtimes[i];
        printf("  %d. %s v%s\n", i + 1, runtime->name, runtime->version);
        printf("     File: %s\n", runtime->filename);
        printf("     Size: %u bytes, Memory: %u KB\n", 
               runtime->file_size, runtime->memory_footprint / 1024);
        printf("     Speed: %u, Available: %s\n", 
               runtime->execution_speed, runtime->available ? "yes" : "no");
    }
}
