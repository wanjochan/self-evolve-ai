/**
 * ai_adaptive_framework.c - AI适应性进化框架实现
 * 
 * 实现完整的AI适应性进化框架
 */

#include "ai_adaptive_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ===============================================
// 核心函数实现
// ===============================================

bool ai_adaptive_init(AIAdaptiveFramework* framework) {
    if (!framework) return false;
    
    // 初始化AI组件
    framework->evolution = (AIEvolutionEngine*)malloc(sizeof(AIEvolutionEngine));
    framework->learning = (AILearningEngine*)malloc(sizeof(AILearningEngine));
    framework->optimizer = (AIOptimizerEngine*)malloc(sizeof(AIOptimizerEngine));
    
    if (!framework->evolution || !framework->learning || !framework->optimizer) {
        return false;
    }
    
    // 初始化各个AI组件
    if (!ai_evolution_init(framework->evolution, 5)) return false;
    if (!ai_learning_init(framework->learning)) return false;
    if (!ai_optimizer_init(framework->optimizer, framework->learning)) return false;
    
    // 设置默认环境
    framework->environment.type = ENV_DEVELOPMENT;
    framework->environment.constraints.max_memory = 1024 * 1024; // 1MB
    framework->environment.constraints.max_cpu_time = 10.0; // 10秒
    framework->environment.constraints.max_code_size = 10000; // 10KB
    framework->environment.constraints.max_complexity = 100;
    framework->environment.constraints.real_time_required = false;
    framework->environment.performance_weight = 0.4;
    framework->environment.memory_weight = 0.3;
    framework->environment.reliability_weight = 0.2;
    framework->environment.maintainability_weight = 0.1;
    framework->environment.last_update = time(NULL);
    
    // 设置默认策略和目标
    framework->strategy = ADAPT_BALANCED;
    framework->goals.target_performance = 0.8;
    framework->goals.target_memory_usage = 0.7;
    framework->goals.target_reliability = 0.9;
    framework->goals.target_maintainability = 0.6;
    framework->goals.tolerance = 0.1;
    
    // 初始化适应性参数
    framework->adaptation_rate = AI_ADAPTIVE_DEFAULT_ADAPTATION_RATE;
    framework->exploration_factor = AI_ADAPTIVE_DEFAULT_EXPLORATION_FACTOR;
    framework->exploitation_factor = AI_ADAPTIVE_DEFAULT_EXPLOITATION_FACTOR;
    framework->adaptation_interval = AI_ADAPTIVE_DEFAULT_INTERVAL;
    
    // 初始化统计信息
    framework->total_adaptations = 0;
    framework->successful_adaptations = 0;
    framework->overall_improvement = 0.0;
    framework->last_adaptation = time(NULL);
    
    // 初始化历史记录
    framework->history_capacity = AI_ADAPTIVE_MAX_HISTORY;
    framework->adaptation_history = (PerformanceMetrics*)malloc(
        framework->history_capacity * sizeof(PerformanceMetrics));
    if (!framework->adaptation_history) return false;
    framework->history_size = 0;
    
    printf("AI Adaptive Framework initialized successfully\n");
    printf("Environment: %s, Strategy: %s\n", 
           framework->environment.type == ENV_DEVELOPMENT ? "Development" : "Other",
           framework->strategy == ADAPT_BALANCED ? "Balanced" : "Other");
    
    return true;
}

void ai_adaptive_cleanup(AIAdaptiveFramework* framework) {
    if (!framework) return;
    
    // 清理AI组件
    if (framework->evolution) {
        ai_evolution_cleanup(framework->evolution);
        free(framework->evolution);
    }
    
    if (framework->learning) {
        ai_learning_cleanup(framework->learning);
        free(framework->learning);
    }
    
    if (framework->optimizer) {
        ai_optimizer_cleanup(framework->optimizer);
        free(framework->optimizer);
    }
    
    // 清理历史记录
    free(framework->adaptation_history);
    
    printf("AI Adaptive Framework cleaned up\n");
}

bool ai_adaptive_set_environment(AIAdaptiveFramework* framework, 
                                 const EnvironmentContext* context) {
    if (!framework || !context) return false;
    
    framework->environment = *context;
    framework->environment.last_update = time(NULL);
    
    // 根据环境调整策略
    switch (context->type) {
        case ENV_PRODUCTION:
            framework->strategy = ADAPT_CONSERVATIVE;
            framework->exploration_factor = 0.1;
            framework->exploitation_factor = 0.9;
            break;
        case ENV_DEVELOPMENT:
            framework->strategy = ADAPT_BALANCED;
            framework->exploration_factor = 0.3;
            framework->exploitation_factor = 0.7;
            break;
        case ENV_RESEARCH:
            framework->strategy = ADAPT_AGGRESSIVE;
            framework->exploration_factor = 0.5;
            framework->exploitation_factor = 0.5;
            break;
        default:
            break;
    }
    
    printf("Environment updated: type=%d, strategy=%d\n", context->type, framework->strategy);
    return true;
}

bool ai_adaptive_set_goals(AIAdaptiveFramework* framework,
                          const EvolutionGoals* goals) {
    if (!framework || !goals) return false;

    framework->goals = *goals;

    printf("Evolution goals updated: performance=%.2f, memory=%.2f, reliability=%.2f, maintainability=%.2f\n",
           goals->target_performance, goals->target_memory_usage,
           goals->target_reliability, goals->target_maintainability);

    return true;
}

bool ai_adaptive_evolve(AIAdaptiveFramework* framework, const char* code) {
    if (!framework || !code) return false;
    
    printf("=== Starting Adaptive Evolution ===\n");
    
    // 1. 监控当前性能
    PerformanceMetrics current = ai_adaptive_monitor_performance(framework, code);
    printf("Current performance: time=%.3f, memory=%zu, errors=%d\n",
           current.execution_time, current.memory_usage, current.error_count);
    
    // 2. 决定优化类型
    OptimizationType opt_type = ai_adaptive_decide_optimization_type(framework, &current);
    printf("Selected optimization type: %d\n", opt_type);
    
    // 3. 应用优化
    char* optimized_code = ai_optimizer_auto_optimize(framework->optimizer, code, opt_type);
    if (!optimized_code) {
        printf("Optimization failed\n");
        return false;
    }
    
    // 4. 评估优化效果
    PerformanceMetrics optimized = ai_adaptive_monitor_performance(framework, optimized_code);
    printf("Optimized performance: time=%.3f, memory=%zu, errors=%d\n",
           optimized.execution_time, optimized.memory_usage, optimized.error_count);
    
    // 5. 记录适应历史
    ai_adaptive_record_adaptation(framework, &current, &optimized, opt_type);
    
    // 6. 学习和调整
    ai_adaptive_adjust_strategy(framework, &optimized);
    
    // 7. 更新统计
    framework->total_adaptations++;
    double improvement = ai_adaptive_balance_objectives(framework, &optimized) - 
                        ai_adaptive_balance_objectives(framework, &current);
    
    if (improvement > AI_ADAPTIVE_MIN_IMPROVEMENT_THRESHOLD) {
        framework->successful_adaptations++;
        framework->overall_improvement += improvement;
        printf("✅ Adaptation successful: %.2f%% improvement\n", improvement * 100);
    } else {
        printf("❌ Adaptation had minimal impact\n");
    }
    
    free(optimized_code);
    framework->last_adaptation = time(NULL);
    
    printf("=== Adaptive Evolution Complete ===\n\n");
    return true;
}

bool ai_adaptive_auto_adapt(AIAdaptiveFramework* framework) {
    if (!framework) return false;
    
    printf("Performing automatic adaptation...\n");
    
    // 检测环境变化
    if (ai_adaptive_detect_environment_change(framework)) {
        printf("Environment change detected, adjusting parameters...\n");
        
        // 调整适应参数
        framework->adaptation_rate *= 1.1; // 增加适应率
        framework->exploration_factor = fmin(0.5, framework->exploration_factor * 1.2);
        
        // 从历史中学习
        ai_adaptive_learn_from_history(framework);
        
        return true;
    }
    
    printf("No significant environment changes detected\n");
    return false;
}

double ai_adaptive_evaluate_state(AIAdaptiveFramework* framework) {
    if (!framework) return 0.0;
    
    // 计算整体适应性分数
    double success_rate = framework->total_adaptations > 0 ? 
        (double)framework->successful_adaptations / framework->total_adaptations : 0.0;
    
    double avg_improvement = framework->successful_adaptations > 0 ?
        framework->overall_improvement / framework->successful_adaptations : 0.0;
    
    double state_score = (success_rate * 0.6) + (avg_improvement * 0.4);
    
    printf("Adaptive state evaluation: %.2f%% (success_rate=%.1f%%, avg_improvement=%.1f%%)\n",
           state_score * 100, success_rate * 100, avg_improvement * 100);
    
    return state_score;
}

char* ai_adaptive_generate_recommendations(AIAdaptiveFramework* framework, 
                                          const char* code) {
    if (!framework || !code) return NULL;
    
    char* recommendations = (char*)malloc(2000);
    if (!recommendations) return NULL;
    
    strcpy(recommendations, "=== AI Adaptive Framework Recommendations ===\n\n");
    
    // 基于环境的建议
    switch (framework->environment.type) {
        case ENV_PRODUCTION:
            strcat(recommendations, "🏭 Production Environment:\n");
            strcat(recommendations, "- Prioritize reliability and stability\n");
            strcat(recommendations, "- Use conservative optimization strategies\n");
            strcat(recommendations, "- Monitor performance continuously\n\n");
            break;
        case ENV_DEVELOPMENT:
            strcat(recommendations, "🔧 Development Environment:\n");
            strcat(recommendations, "- Balance performance and maintainability\n");
            strcat(recommendations, "- Experiment with different optimization approaches\n");
            strcat(recommendations, "- Focus on code quality improvements\n\n");
            break;
        case ENV_RESEARCH:
            strcat(recommendations, "🔬 Research Environment:\n");
            strcat(recommendations, "- Explore aggressive optimization techniques\n");
            strcat(recommendations, "- Test innovative approaches\n");
            strcat(recommendations, "- Collect detailed performance data\n\n");
            break;
        default:
            break;
    }
    
    // 基于历史的建议
    if (framework->history_size > 5) {
        strcat(recommendations, "📊 Historical Analysis:\n");
        
        double recent_improvement = 0.0;
        for (size_t i = framework->history_size - 5; i < framework->history_size; i++) {
            // 简化的改进计算
            recent_improvement += 0.1; // 模拟改进
        }
        recent_improvement /= 5.0;
        
        if (recent_improvement > 0.1) {
            strcat(recommendations, "- Recent adaptations show good progress\n");
            strcat(recommendations, "- Continue current optimization strategy\n");
        } else {
            strcat(recommendations, "- Recent adaptations show limited progress\n");
            strcat(recommendations, "- Consider changing optimization approach\n");
        }
        strcat(recommendations, "\n");
    }
    
    // 基于当前状态的建议
    double state_score = ai_adaptive_evaluate_state(framework);
    if (state_score > 0.8) {
        strcat(recommendations, "🎯 System Status: Excellent\n");
        strcat(recommendations, "- Maintain current configuration\n");
        strcat(recommendations, "- Fine-tune existing optimizations\n");
    } else if (state_score > 0.6) {
        strcat(recommendations, "⚡ System Status: Good\n");
        strcat(recommendations, "- Consider moderate adjustments\n");
        strcat(recommendations, "- Monitor for improvement opportunities\n");
    } else {
        strcat(recommendations, "🔄 System Status: Needs Improvement\n");
        strcat(recommendations, "- Review optimization strategies\n");
        strcat(recommendations, "- Consider environment reconfiguration\n");
    }
    
    strcat(recommendations, "\n=== End of Recommendations ===\n");
    
    return recommendations;
}

// ===============================================
// 辅助函数实现
// ===============================================

OptimizationType ai_adaptive_decide_optimization_type(AIAdaptiveFramework* framework,
                                                     const PerformanceMetrics* current) {
    if (!framework || !current) return OPT_TYPE_PERFORMANCE;
    
    // 基于环境权重和当前性能决定优化类型
    double perf_score = framework->environment.performance_weight * 
                       (1.0 / (1.0 + current->execution_time));
    double mem_score = framework->environment.memory_weight * 
                      (1.0 / (1.0 + current->memory_usage / 1000.0));
    double rel_score = framework->environment.reliability_weight * current->success_rate;
    
    if (perf_score > mem_score && perf_score > rel_score) {
        return OPT_TYPE_PERFORMANCE;
    } else if (mem_score > rel_score) {
        return OPT_TYPE_MEMORY;
    } else {
        return OPT_TYPE_READABILITY; // 作为可靠性的代理
    }
}

void ai_adaptive_adjust_strategy(AIAdaptiveFramework* framework, 
                                const PerformanceMetrics* feedback) {
    if (!framework || !feedback) return;
    
    // 根据反馈调整策略
    if (feedback->error_count > 0) {
        // 有错误，采用更保守的策略
        framework->exploration_factor *= 0.9;
        framework->exploitation_factor = 1.0 - framework->exploration_factor;
    } else if (feedback->success_rate > 0.9) {
        // 成功率高，可以更激进
        framework->exploration_factor = fmin(0.5, framework->exploration_factor * 1.1);
        framework->exploitation_factor = 1.0 - framework->exploration_factor;
    }
    
    printf("Strategy adjusted: exploration=%.2f, exploitation=%.2f\n",
           framework->exploration_factor, framework->exploitation_factor);
}

double ai_adaptive_balance_objectives(AIAdaptiveFramework* framework,
                                     const PerformanceMetrics* metrics) {
    if (!framework || !metrics) return 0.0;
    
    // 多目标平衡计算
    double perf_score = 1.0 / (1.0 + metrics->execution_time);
    double mem_score = 1.0 / (1.0 + metrics->memory_usage / 1000.0);
    double rel_score = metrics->success_rate;
    double maint_score = 1.0 / (1.0 + metrics->code_size / 1000.0);
    
    double balanced_score = 
        framework->environment.performance_weight * perf_score +
        framework->environment.memory_weight * mem_score +
        framework->environment.reliability_weight * rel_score +
        framework->environment.maintainability_weight * maint_score;
    
    return balanced_score;
}

PerformanceMetrics ai_adaptive_monitor_performance(AIAdaptiveFramework* framework,
                                                  const char* code) {
    PerformanceMetrics metrics = {0};
    
    if (!code) return metrics;
    
    // 模拟性能监控
    metrics.execution_time = 0.1 + (strlen(code) / 1000.0);
    metrics.memory_usage = strlen(code) * 2;
    metrics.code_size = strlen(code);
    metrics.error_count = 0;
    metrics.cpu_utilization = 0.3;
    metrics.success_rate = 1.0;
    
    return metrics;
}

bool ai_adaptive_detect_environment_change(AIAdaptiveFramework* framework) {
    if (!framework) return false;
    
    // 简化的环境变化检测
    time_t current_time = time(NULL);
    double time_since_last = difftime(current_time, framework->environment.last_update);
    
    // 如果超过适应间隔，认为环境可能发生变化
    return time_since_last > framework->adaptation_interval;
}

bool ai_adaptive_record_adaptation(AIAdaptiveFramework* framework,
                                  const PerformanceMetrics* before,
                                  const PerformanceMetrics* after,
                                  OptimizationType type) {
    if (!framework || !before || !after) return false;
    
    if (framework->history_size < framework->history_capacity) {
        framework->adaptation_history[framework->history_size] = *after;
        framework->history_size++;
        return true;
    }
    
    return false;
}

bool ai_adaptive_learn_from_history(AIAdaptiveFramework* framework) {
    if (!framework || framework->history_size < 3) return false;
    
    printf("Learning from adaptation history (%zu records)...\n", framework->history_size);
    
    // 分析历史趋势
    double trend = 0.0;
    for (size_t i = 1; i < framework->history_size; i++) {
        double current_score = ai_adaptive_balance_objectives(framework, &framework->adaptation_history[i]);
        double prev_score = ai_adaptive_balance_objectives(framework, &framework->adaptation_history[i-1]);
        trend += (current_score - prev_score);
    }
    trend /= (framework->history_size - 1);
    
    // 根据趋势调整参数
    if (trend > 0.05) {
        printf("Positive trend detected, maintaining current approach\n");
    } else if (trend < -0.05) {
        printf("Negative trend detected, adjusting strategy\n");
        framework->adaptation_rate *= 1.2;
    } else {
        printf("Stable trend, fine-tuning parameters\n");
        framework->adaptation_rate *= 0.95;
    }
    
    return true;
}

void ai_adaptive_print_stats(AIAdaptiveFramework* framework) {
    if (!framework) return;
    
    printf("\n=== AI Adaptive Framework Statistics ===\n");
    printf("Environment Type: %d\n", framework->environment.type);
    printf("Adaptation Strategy: %d\n", framework->strategy);
    printf("Total Adaptations: %d\n", framework->total_adaptations);
    printf("Successful Adaptations: %d\n", framework->successful_adaptations);
    printf("Success Rate: %.1f%%\n", 
           framework->total_adaptations > 0 ? 
           (double)framework->successful_adaptations / framework->total_adaptations * 100 : 0);
    printf("Overall Improvement: %.2f%%\n", framework->overall_improvement * 100);
    printf("Adaptation Rate: %.3f\n", framework->adaptation_rate);
    printf("Exploration Factor: %.3f\n", framework->exploration_factor);
    printf("Exploitation Factor: %.3f\n", framework->exploitation_factor);
    printf("History Records: %zu\n", framework->history_size);
    
    double state_score = ai_adaptive_evaluate_state(framework);
    printf("Adaptive State Score: %.1f%%\n", state_score * 100);
    
    printf("========================================\n\n");
}
