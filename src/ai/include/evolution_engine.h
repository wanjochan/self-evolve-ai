#ifndef EVOLUTION_ENGINE_H
#define EVOLUTION_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 演化引擎配置
typedef struct {
    uint32_t population_size;
    double mutation_rate;
    double crossover_rate;
    uint32_t max_generations;
} EvolutionConfig;

// 个体表示
typedef struct {
    void* genome;
    size_t genome_size;
    double fitness;
    uint32_t generation;
} Individual;

// 种群表示
typedef struct {
    Individual* individuals;
    size_t size;
    size_t capacity;
    uint32_t current_generation;
} Population;

// 演化引擎
typedef struct {
    EvolutionConfig config;
    Population* population;
    double (*fitness_function)(const void* genome, size_t size);
    void (*mutation_function)(void* genome, size_t size, double rate);
    void (*crossover_function)(const void* parent1, const void* parent2, void* offspring, size_t size);
} EvolutionEngine;

// 基本操作
EvolutionEngine* evolution_engine_create(const EvolutionConfig* config);
void evolution_engine_destroy(EvolutionEngine* engine);
int evolution_engine_evolve(EvolutionEngine* engine);
const Individual* evolution_engine_get_best(const EvolutionEngine* engine);

#ifdef __cplusplus
}
#endif

#endif // EVOLUTION_ENGINE_H