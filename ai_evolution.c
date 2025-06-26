/**
 * ai_evolution.c - AI驱动进化算法实现
 * 
 * 实现AI驱动的代码进化算法核心功能
 */

#include "ai_evolution.h"
#include "runtime.h"
#include "c2astc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// ===============================================
// 内部辅助函数
// ===============================================

static double random_double() {
    return (double)rand() / RAND_MAX;
}

static int random_int(int min, int max) {
    return min + rand() % (max - min + 1);
}

// ===============================================
// 核心函数实现
// ===============================================

bool ai_evolution_init(AIEvolutionEngine* engine, size_t population_size) {
    if (!engine) return false;
    
    // 初始化随机数种子
    srand((unsigned int)time(NULL));
    
    // 初始化引擎参数
    engine->population = NULL;
    engine->population_size = population_size;
    engine->current_generation = 0;
    engine->mutation_rate = AI_EVO_DEFAULT_MUTATION_RATE;
    engine->crossover_rate = AI_EVO_DEFAULT_CROSSOVER_RATE;
    engine->strategy = OPT_PERFORMANCE;
    engine->learning_rate = AI_EVO_DEFAULT_LEARNING_RATE;
    engine->exploration_rate = AI_EVO_DEFAULT_EXPLORATION_RATE;
    
    // 初始化历史记录
    engine->history_capacity = 1000;
    engine->history = (PerformanceMetrics*)malloc(engine->history_capacity * sizeof(PerformanceMetrics));
    if (!engine->history) return false;
    engine->history_size = 0;
    
    printf("AI Evolution Engine initialized with population size: %zu\n", population_size);
    return true;
}

void ai_evolution_cleanup(AIEvolutionEngine* engine) {
    if (!engine) return;
    
    // 清理种群
    CodeVariant* current = engine->population;
    while (current) {
        CodeVariant* next = current->next;
        ai_evolution_free_variant(current);
        current = next;
    }
    
    // 清理历史记录
    free(engine->history);
    
    printf("AI Evolution Engine cleaned up\n");
}

bool ai_evolution_add_variant(AIEvolutionEngine* engine, const char* source_code) {
    if (!engine || !source_code) return false;
    
    CodeVariant* variant = ai_evolution_create_variant(source_code);
    if (!variant) return false;
    
    // 评估新变体
    variant->metrics = ai_evolution_evaluate(engine, variant);
    variant->fitness_score = ai_evolution_calculate_fitness(&variant->metrics, engine->strategy);
    variant->generation = engine->current_generation;
    variant->created_time = time(NULL);
    
    // 添加到种群
    variant->next = engine->population;
    engine->population = variant;
    
    printf("Added variant to population (fitness: %.3f)\n", variant->fitness_score);
    return true;
}

PerformanceMetrics ai_evolution_evaluate(AIEvolutionEngine* engine, CodeVariant* variant) {
    PerformanceMetrics metrics = {0};
    
    if (!variant || !variant->source_code) {
        return metrics;
    }
    
    // 模拟性能评估
    clock_t start_time = clock();
    
    // 1. 编译测试
    C2AstcOptions options = c2astc_default_options();
    struct ASTNode* ast = c2astc_convert(variant->source_code, &options);
    
    if (ast) {
        // 编译成功
        metrics.error_count = 0;
        
        // 2. 代码大小评估
        metrics.code_size = strlen(variant->source_code);
        
        // 3. 模拟执行时间（基于代码复杂度）
        metrics.execution_time = (double)metrics.code_size / 1000.0 + random_double() * 0.1;
        
        // 4. 模拟内存使用
        metrics.memory_usage = metrics.code_size * 2 + random_int(100, 1000);
        
        // 5. 模拟CPU利用率
        metrics.cpu_utilization = 0.3 + random_double() * 0.4;
        
        // 6. 成功率
        metrics.success_rate = 1.0;
        
        ast_free(ast);
    } else {
        // 编译失败
        metrics.error_count = 1;
        metrics.success_rate = 0.0;
        metrics.execution_time = 999.0; // 惩罚值
    }
    
    clock_t end_time = clock();
    double eval_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    printf("Evaluated variant: errors=%d, size=%zu, time=%.3f, fitness=%.3f\n", 
           metrics.error_count, metrics.code_size, metrics.execution_time, 
           ai_evolution_calculate_fitness(&metrics, engine->strategy));
    
    return metrics;
}

double ai_evolution_calculate_fitness(const PerformanceMetrics* metrics, OptimizationStrategy strategy) {
    if (!metrics) return 0.0;
    
    // 如果有错误，适应度很低
    if (metrics->error_count > 0) {
        return 0.1;
    }
    
    double fitness = 0.0;
    
    switch (strategy) {
        case OPT_PERFORMANCE:
            // 性能优化：执行时间越短越好
            fitness = 1.0 / (1.0 + metrics->execution_time);
            break;
            
        case OPT_MEMORY:
            // 内存优化：内存使用越少越好
            fitness = 1.0 / (1.0 + metrics->memory_usage / 1000.0);
            break;
            
        case OPT_SIZE:
            // 代码大小优化：代码越小越好
            fitness = 1.0 / (1.0 + metrics->code_size / 100.0);
            break;
            
        case OPT_RELIABILITY:
            // 可靠性优化：成功率越高越好
            fitness = metrics->success_rate;
            break;
            
        case OPT_MAINTAINABILITY:
            // 可维护性优化：综合考虑多个因素
            fitness = (metrics->success_rate * 0.4 + 
                      (1.0 / (1.0 + metrics->code_size / 200.0)) * 0.3 +
                      (1.0 / (1.0 + metrics->execution_time)) * 0.3);
            break;
    }
    
    return fmax(0.0, fmin(1.0, fitness));
}

bool ai_evolution_evolve_generation(AIEvolutionEngine* engine) {
    if (!engine || !engine->population) return false;
    
    printf("=== Evolving Generation %d ===\n", engine->current_generation + 1);
    
    // 1. 选择最优个体
    CodeVariant* best = ai_evolution_select_best(engine);
    if (!best) return false;
    
    printf("Best variant fitness: %.3f\n", best->fitness_score);
    
    // 2. 记录历史
    if (engine->history_size < engine->history_capacity) {
        engine->history[engine->history_size++] = best->metrics;
    }
    
    // 3. 生成新一代
    size_t new_variants = 0;
    
    // 变异操作
    if (random_double() < engine->mutation_rate) {
        CodeVariant* mutated = ai_evolution_mutate(engine, best);
        if (mutated) {
            mutated->next = engine->population;
            engine->population = mutated;
            new_variants++;
        }
    }
    
    // 4. 自适应调整参数
    ai_evolution_adapt_parameters(engine);
    
    engine->current_generation++;
    
    printf("Generated %zu new variants in generation %d\n", new_variants, engine->current_generation);
    return true;
}

CodeVariant* ai_evolution_select_best(AIEvolutionEngine* engine) {
    if (!engine || !engine->population) return NULL;
    
    CodeVariant* best = engine->population;
    CodeVariant* current = engine->population->next;
    
    while (current) {
        if (current->fitness_score > best->fitness_score) {
            best = current;
        }
        current = current->next;
    }
    
    return best;
}

CodeVariant* ai_evolution_mutate(AIEvolutionEngine* engine, CodeVariant* parent) {
    if (!engine || !parent || !parent->source_code) return NULL;
    
    // 简单的变异：在代码中添加注释或修改变量名
    size_t source_len = strlen(parent->source_code);
    char* mutated_code = (char*)malloc(source_len + 100);
    if (!mutated_code) return NULL;
    
    // 复制原代码
    strcpy(mutated_code, parent->source_code);
    
    // 简单变异：添加优化注释
    if (random_double() < 0.5) {
        strcat(mutated_code, "\n// AI Evolution: Performance optimized");
    } else {
        strcat(mutated_code, "\n// AI Evolution: Memory optimized");
    }
    
    CodeVariant* mutated = ai_evolution_create_variant(mutated_code);
    free(mutated_code);
    
    if (mutated) {
        mutated->parent = parent;
        mutated->metrics = ai_evolution_evaluate(engine, mutated);
        mutated->fitness_score = ai_evolution_calculate_fitness(&mutated->metrics, engine->strategy);
        mutated->generation = engine->current_generation + 1;
        mutated->created_time = time(NULL);
        
        printf("Mutated variant created (fitness: %.3f -> %.3f)\n", 
               parent->fitness_score, mutated->fitness_score);
    }
    
    return mutated;
}

void ai_evolution_adapt_parameters(AIEvolutionEngine* engine) {
    if (!engine) return;
    
    // 根据进化历史自适应调整参数
    if (engine->history_size >= 5) {
        // 计算最近几代的改进率
        double recent_improvement = 0.0;
        for (size_t i = engine->history_size - 4; i < engine->history_size - 1; i++) {
            double current_fitness = ai_evolution_calculate_fitness(&engine->history[i], engine->strategy);
            double next_fitness = ai_evolution_calculate_fitness(&engine->history[i + 1], engine->strategy);
            recent_improvement += (next_fitness - current_fitness);
        }
        recent_improvement /= 3.0;
        
        // 如果改进缓慢，增加探索率
        if (recent_improvement < 0.01) {
            engine->exploration_rate = fmin(0.5, engine->exploration_rate * 1.1);
            engine->mutation_rate = fmin(0.3, engine->mutation_rate * 1.1);
        } else {
            // 如果改进快速，减少探索率
            engine->exploration_rate = fmax(0.1, engine->exploration_rate * 0.9);
            engine->mutation_rate = fmax(0.05, engine->mutation_rate * 0.9);
        }
        
        printf("Adapted parameters: mutation_rate=%.3f, exploration_rate=%.3f\n", 
               engine->mutation_rate, engine->exploration_rate);
    }
}

// ===============================================
// 辅助函数实现
// ===============================================

CodeVariant* ai_evolution_create_variant(const char* source_code) {
    if (!source_code) return NULL;
    
    CodeVariant* variant = (CodeVariant*)malloc(sizeof(CodeVariant));
    if (!variant) return NULL;
    
    variant->source_code = strdup(source_code);
    variant->astc_code = NULL;
    variant->fitness_score = 0.0;
    variant->generation = 0;
    variant->created_time = time(NULL);
    variant->parent = NULL;
    variant->next = NULL;
    
    // 初始化性能指标
    memset(&variant->metrics, 0, sizeof(PerformanceMetrics));
    
    return variant;
}

void ai_evolution_free_variant(CodeVariant* variant) {
    if (!variant) return;
    
    free(variant->source_code);
    free(variant->astc_code);
    free(variant);
}

void ai_evolution_print_stats(AIEvolutionEngine* engine) {
    if (!engine) return;
    
    printf("\n=== AI Evolution Statistics ===\n");
    printf("Current Generation: %d\n", engine->current_generation);
    printf("Population Size: %zu\n", engine->population_size);
    printf("Mutation Rate: %.3f\n", engine->mutation_rate);
    printf("Exploration Rate: %.3f\n", engine->exploration_rate);
    printf("History Size: %zu\n", engine->history_size);
    
    if (engine->population) {
        CodeVariant* best = ai_evolution_select_best(engine);
        if (best) {
            printf("Best Fitness: %.3f\n", best->fitness_score);
            printf("Best Execution Time: %.3f\n", best->metrics.execution_time);
            printf("Best Memory Usage: %zu\n", best->metrics.memory_usage);
        }
    }
    
    printf("===============================\n\n");
}
