/**
 * ai_learning.c - AI自我学习机制实现
 * 
 * 实现AI系统的自我学习和知识积累功能
 */

#include "ai_learning.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ===============================================
// 核心函数实现
// ===============================================

bool ai_learning_init(AILearningEngine* engine) {
    if (!engine) return false;
    
    // 初始化知识库
    engine->knowledge.error_patterns = NULL;
    engine->knowledge.perf_patterns = NULL;
    engine->knowledge.records = NULL;
    engine->knowledge.record_count = 0;
    engine->knowledge.record_capacity = AI_LEARNING_MAX_RECORDS;
    engine->knowledge.total_executions = 0;
    engine->knowledge.successful_optimizations = 0;
    engine->knowledge.overall_improvement = 0.0;
    engine->knowledge.last_update = time(NULL);
    
    // 分配记录数组
    engine->knowledge.records = (ExecutionRecord*)malloc(
        engine->knowledge.record_capacity * sizeof(ExecutionRecord));
    if (!engine->knowledge.records) return false;
    
    // 初始化学习参数
    engine->learning_threshold = AI_LEARNING_DEFAULT_THRESHOLD;
    engine->confidence_threshold = AI_LEARNING_DEFAULT_CONFIDENCE;
    engine->min_pattern_occurrences = AI_LEARNING_MIN_PATTERN_COUNT;
    engine->pattern_decay_rate = AI_LEARNING_PATTERN_DECAY_RATE;
    engine->adaptation_rate = AI_LEARNING_ADAPTATION_RATE;
    engine->auto_update_enabled = true;
    
    // 添加一些预定义的错误模式
    ai_learning_learn_pattern(engine, "error", "segmentation fault", 
                             "Check array bounds and pointer validity");
    ai_learning_learn_pattern(engine, "error", "memory leak", 
                             "Ensure all malloc() calls have corresponding free()");
    ai_learning_learn_pattern(engine, "error", "infinite loop", 
                             "Check loop termination conditions");
    
    // 添加一些预定义的性能模式
    ai_learning_learn_pattern(engine, "performance", "for.*loop", 
                             "Consider loop unrolling or vectorization");
    ai_learning_learn_pattern(engine, "performance", "recursive.*function", 
                             "Consider iterative implementation or memoization");
    
    printf("AI Learning Engine initialized with knowledge base\n");
    return true;
}

void ai_learning_cleanup(AILearningEngine* engine) {
    if (!engine) return;
    
    // 清理错误模式
    ErrorPattern* error_current = engine->knowledge.error_patterns;
    while (error_current) {
        ErrorPattern* next = error_current->next;
        ai_learning_free_error_pattern(error_current);
        error_current = next;
    }
    
    // 清理性能模式
    PerformancePattern* perf_current = engine->knowledge.perf_patterns;
    while (perf_current) {
        PerformancePattern* next = perf_current->next;
        ai_learning_free_perf_pattern(perf_current);
        perf_current = next;
    }
    
    // 清理执行记录
    for (size_t i = 0; i < engine->knowledge.record_count; i++) {
        ai_learning_free_record(&engine->knowledge.records[i]);
    }
    free(engine->knowledge.records);
    
    printf("AI Learning Engine cleaned up\n");
}

bool ai_learning_record_execution(AILearningEngine* engine, const char* code, 
                                  const PerformanceMetrics* metrics, 
                                  int error_code, const char* error_msg) {
    if (!engine || !code || !metrics) return false;
    
    // 检查记录容量
    if (engine->knowledge.record_count >= engine->knowledge.record_capacity) {
        printf("Warning: Knowledge base record capacity reached\n");
        return false;
    }
    
    // 创建新记录
    ExecutionRecord* record = &engine->knowledge.records[engine->knowledge.record_count];
    record->code_snippet = strdup(code);
    record->metrics = *metrics;
    record->error_code = error_code;
    record->error_message = error_msg ? strdup(error_msg) : NULL;
    record->timestamp = time(NULL);
    record->improvement_score = 0.0; // 将在分析时计算
    
    engine->knowledge.record_count++;
    engine->knowledge.total_executions++;
    
    // 如果启用自动更新，触发学习
    if (engine->auto_update_enabled) {
        ai_learning_analyze_errors(engine);
        ai_learning_analyze_performance(engine);
    }
    
    printf("Recorded execution: code_size=%zu, errors=%d\n", 
           strlen(code), error_code);
    
    return true;
}

bool ai_learning_analyze_errors(AILearningEngine* engine) {
    if (!engine) return false;
    
    int error_count = 0;
    
    // 分析最近的错误记录
    for (size_t i = 0; i < engine->knowledge.record_count; i++) {
        ExecutionRecord* record = &engine->knowledge.records[i];
        
        if (record->error_code != 0 && record->error_message) {
            error_count++;
            
            // 尝试匹配已知错误模式
            ErrorPattern* pattern = ai_learning_match_error_pattern(engine, record->error_message);
            
            if (pattern) {
                // 更新现有模式
                pattern->occurrence_count++;
                pattern->confidence = fmin(1.0, pattern->confidence + 0.1);
            } else {
                // 创建新的错误模式（简化版）
                char pattern_name[100];
                snprintf(pattern_name, sizeof(pattern_name), "error_%d", error_count);
                
                ErrorPattern* new_pattern = ai_learning_create_error_pattern(
                    pattern_name, record->error_message, "Review code logic and syntax");
                
                if (new_pattern) {
                    new_pattern->next = engine->knowledge.error_patterns;
                    engine->knowledge.error_patterns = new_pattern;
                }
            }
        }
    }
    
    printf("Analyzed %d error patterns\n", error_count);
    return true;
}

bool ai_learning_analyze_performance(AILearningEngine* engine) {
    if (!engine) return false;
    
    int pattern_count = 0;
    
    // 分析性能记录，寻找改进模式
    for (size_t i = 1; i < engine->knowledge.record_count; i++) {
        ExecutionRecord* prev = &engine->knowledge.records[i-1];
        ExecutionRecord* curr = &engine->knowledge.records[i];
        
        // 计算性能改进
        if (prev->metrics.execution_time > 0 && curr->metrics.execution_time > 0) {
            double time_improvement = (prev->metrics.execution_time - curr->metrics.execution_time) 
                                    / prev->metrics.execution_time;
            
            if (time_improvement > engine->learning_threshold) {
                // 发现性能改进
                curr->improvement_score = time_improvement;
                engine->knowledge.successful_optimizations++;
                engine->knowledge.overall_improvement += time_improvement;
                pattern_count++;
                
                // 识别代码模式
                char* code_pattern = ai_learning_identify_code_pattern(curr->code_snippet);
                if (code_pattern) {
                    // 查找或创建性能模式
                    PerformancePattern* pattern = ai_learning_match_performance_pattern(engine, code_pattern);
                    
                    if (pattern) {
                        pattern->usage_count++;
                        pattern->avg_improvement = (pattern->avg_improvement + time_improvement) / 2.0;
                        pattern->success_rate = (double)pattern->usage_count / 
                                              (pattern->usage_count + 1);
                    } else {
                        // 创建新的性能模式
                        char pattern_name[100];
                        snprintf(pattern_name, sizeof(pattern_name), "perf_pattern_%d", pattern_count);
                        
                        PerformancePattern* new_pattern = ai_learning_create_perf_pattern(
                            pattern_name, code_pattern);
                        
                        if (new_pattern) {
                            new_pattern->avg_improvement = time_improvement;
                            new_pattern->usage_count = 1;
                            new_pattern->success_rate = 1.0;
                            new_pattern->next = engine->knowledge.perf_patterns;
                            engine->knowledge.perf_patterns = new_pattern;
                        }
                    }
                    
                    free(code_pattern);
                }
            }
        }
    }
    
    printf("Analyzed %d performance patterns\n", pattern_count);
    return true;
}

char* ai_learning_generate_suggestions(AILearningEngine* engine, const char* code) {
    if (!engine || !code) return NULL;
    
    char* suggestions = (char*)malloc(1000);
    if (!suggestions) return NULL;
    
    strcpy(suggestions, "AI Learning Suggestions:\n");
    
    // 基于错误模式的建议
    ErrorPattern* error_pattern = engine->knowledge.error_patterns;
    while (error_pattern) {
        if (strstr(code, error_pattern->code_pattern)) {
            strcat(suggestions, "- ");
            strcat(suggestions, error_pattern->solution);
            strcat(suggestions, "\n");
        }
        error_pattern = error_pattern->next;
    }
    
    // 基于性能模式的建议
    PerformancePattern* perf_pattern = engine->knowledge.perf_patterns;
    while (perf_pattern) {
        if (strstr(code, perf_pattern->code_pattern)) {
            char suggestion[200];
            snprintf(suggestion, sizeof(suggestion), 
                    "- Performance optimization (avg improvement: %.1f%%): %s\n",
                    perf_pattern->avg_improvement * 100, perf_pattern->pattern_name);
            strcat(suggestions, suggestion);
        }
        perf_pattern = perf_pattern->next;
    }
    
    // 如果没有找到特定建议，提供通用建议
    if (strlen(suggestions) < 50) {
        strcat(suggestions, "- Consider code profiling for performance bottlenecks\n");
        strcat(suggestions, "- Review memory allocation and deallocation\n");
        strcat(suggestions, "- Check for potential optimization opportunities\n");
    }
    
    return suggestions;
}

bool ai_learning_learn_pattern(AILearningEngine* engine, const char* pattern_type, 
                              const char* pattern, const char* solution) {
    if (!engine || !pattern_type || !pattern || !solution) return false;
    
    if (strcmp(pattern_type, "error") == 0) {
        // 学习错误模式
        ErrorPattern* new_pattern = ai_learning_create_error_pattern(
            "learned_error", pattern, solution);
        
        if (new_pattern) {
            new_pattern->next = engine->knowledge.error_patterns;
            engine->knowledge.error_patterns = new_pattern;
            return true;
        }
    } else if (strcmp(pattern_type, "performance") == 0) {
        // 学习性能模式
        PerformancePattern* new_pattern = ai_learning_create_perf_pattern(
            "learned_perf", pattern);
        
        if (new_pattern) {
            new_pattern->next = engine->knowledge.perf_patterns;
            engine->knowledge.perf_patterns = new_pattern;
            return true;
        }
    }
    
    return false;
}

void ai_learning_print_stats(AILearningEngine* engine) {
    if (!engine) return;
    
    printf("\n=== AI Learning Statistics ===\n");
    printf("Total Executions: %d\n", engine->knowledge.total_executions);
    printf("Successful Optimizations: %d\n", engine->knowledge.successful_optimizations);
    printf("Overall Improvement: %.2f%%\n", engine->knowledge.overall_improvement * 100);
    printf("Records in Knowledge Base: %zu\n", engine->knowledge.record_count);
    
    // 统计模式数量
    int error_patterns = 0;
    ErrorPattern* ep = engine->knowledge.error_patterns;
    while (ep) { error_patterns++; ep = ep->next; }
    
    int perf_patterns = 0;
    PerformancePattern* pp = engine->knowledge.perf_patterns;
    while (pp) { perf_patterns++; pp = pp->next; }
    
    printf("Error Patterns: %d\n", error_patterns);
    printf("Performance Patterns: %d\n", perf_patterns);
    printf("Learning Threshold: %.3f\n", engine->learning_threshold);
    printf("Confidence Threshold: %.3f\n", engine->confidence_threshold);
    printf("==============================\n\n");
}

// ===============================================
// 辅助函数实现
// ===============================================

ExecutionRecord* ai_learning_create_record(const char* code,
                                          const PerformanceMetrics* metrics,
                                          int error_code, const char* error_msg) {
    ExecutionRecord* record = (ExecutionRecord*)malloc(sizeof(ExecutionRecord));
    if (!record) return NULL;

    record->code_snippet = strdup(code);
    record->metrics = *metrics;
    record->error_code = error_code;
    record->error_message = error_msg ? strdup(error_msg) : NULL;
    record->timestamp = time(NULL);
    record->improvement_score = 0.0;

    return record;
}

void ai_learning_free_record(ExecutionRecord* record) {
    if (!record) return;

    free(record->code_snippet);
    free(record->error_message);
    // 注意：不要free record本身，因为它可能是数组的一部分
}

ErrorPattern* ai_learning_create_error_pattern(const char* name, const char* pattern,
                                              const char* solution) {
    ErrorPattern* ep = (ErrorPattern*)malloc(sizeof(ErrorPattern));
    if (!ep) return NULL;

    ep->pattern_name = strdup(name);
    ep->description = strdup("Auto-learned error pattern");
    ep->code_pattern = strdup(pattern);
    ep->solution = strdup(solution);
    ep->occurrence_count = 1;
    ep->confidence = 0.5;
    ep->next = NULL;

    return ep;
}

void ai_learning_free_error_pattern(ErrorPattern* pattern) {
    if (!pattern) return;

    free(pattern->pattern_name);
    free(pattern->description);
    free(pattern->code_pattern);
    free(pattern->solution);
    free(pattern);
}

PerformancePattern* ai_learning_create_perf_pattern(const char* name, const char* pattern) {
    PerformancePattern* pp = (PerformancePattern*)malloc(sizeof(PerformancePattern));
    if (!pp) return NULL;

    pp->pattern_name = strdup(name);
    pp->code_pattern = strdup(pattern);
    pp->avg_improvement = 0.0;
    pp->usage_count = 0;
    pp->success_rate = 0.0;
    pp->next = NULL;

    return pp;
}

void ai_learning_free_perf_pattern(PerformancePattern* pattern) {
    if (!pattern) return;

    free(pattern->pattern_name);
    free(pattern->code_pattern);
    free(pattern);
}

char* ai_learning_identify_code_pattern(const char* code) {
    if (!code) return NULL;

    // 简化的模式识别
    if (strstr(code, "for")) {
        return strdup("for_loop");
    } else if (strstr(code, "while")) {
        return strdup("while_loop");
    } else if (strstr(code, "malloc")) {
        return strdup("memory_allocation");
    } else if (strstr(code, "recursive") || strstr(code, "return") && strstr(code, "(")) {
        return strdup("recursive_function");
    } else {
        return strdup("general_code");
    }
}

ErrorPattern* ai_learning_match_error_pattern(AILearningEngine* engine,
                                             const char* error_msg) {
    if (!engine || !error_msg) return NULL;

    ErrorPattern* current = engine->knowledge.error_patterns;
    while (current) {
        if (strstr(error_msg, current->code_pattern)) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

PerformancePattern* ai_learning_match_performance_pattern(AILearningEngine* engine,
                                                         const char* code) {
    if (!engine || !code) return NULL;

    PerformancePattern* current = engine->knowledge.perf_patterns;
    while (current) {
        if (strstr(code, current->code_pattern)) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}
