/**
 * evolution_strategy.h - AI自主进化策略算法
 * 
 * 实现基于遗传算法的编译器自主进化系统
 */

#ifndef EVOLUTION_STRATEGY_H
#define EVOLUTION_STRATEGY_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// 进化算法常量
// ===============================================

#define MAX_POPULATION_SIZE 100
#define MAX_GENERATIONS 1000
#define MAX_GENOME_SIZE 10000
#define MAX_MUTATION_RATE 0.1f
#define MAX_CROSSOVER_RATE 0.8f
#define MAX_FITNESS_TESTS 50
#define MAX_SPECIES_NAME_LEN 64

// ===============================================
// 基因和个体定义
// ===============================================

typedef enum {
    GENE_TYPE_INSTRUCTION = 1,    // 指令基因
    GENE_TYPE_OPTIMIZATION = 2,   // 优化基因
    GENE_TYPE_STRUCTURE = 3,      // 结构基因
    GENE_TYPE_PARAMETER = 4,      // 参数基因
    GENE_TYPE_FEATURE = 5         // 特性基因
} GeneType;

typedef struct Gene {
    GeneType type;
    uint32_t value;               // 基因值
    float weight;                 // 基因权重
    bool active;                  // 是否激活
    uint32_t mutation_count;      // 变异次数
    float stability;              // 稳定性评分
} Gene;

typedef struct Individual {
    uint32_t id;
    char species_name[MAX_SPECIES_NAME_LEN];
    
    Gene* genome;                 // 基因组
    uint32_t genome_size;         // 基因组大小
    
    // 适应度评估
    float fitness_score;          // 适应度评分
    float performance_score;      // 性能评分
    float stability_score;        // 稳定性评分
    float innovation_score;       // 创新性评分
    
    // 测试结果
    uint32_t tests_passed;        // 通过的测试数
    uint32_t tests_failed;        // 失败的测试数
    uint32_t compilation_time_ms; // 编译时间
    uint32_t execution_time_ms;   // 执行时间
    uint32_t memory_usage_kb;     // 内存使用
    
    // 进化历史
    uint32_t generation;          // 所属代数
    uint32_t parent1_id;          // 父代1 ID
    uint32_t parent2_id;          // 父代2 ID
    uint32_t mutation_count;      // 总变异次数
    
    // 状态标志
    bool is_viable;               // 是否可行
    bool is_tested;               // 是否已测试
    bool is_elite;                // 是否精英个体
} Individual;

// ===============================================
// 种群和进化环境
// ===============================================

typedef struct Population {
    Individual* individuals;      // 个体数组
    uint32_t size;               // 种群大小
    uint32_t capacity;           // 容量
    uint32_t generation;         // 当前代数
    
    // 种群统计
    float avg_fitness;           // 平均适应度
    float max_fitness;           // 最高适应度
    float min_fitness;           // 最低适应度
    uint32_t elite_count;        // 精英个体数量
    
    // 最佳个体
    Individual* best_individual;
    Individual* worst_individual;
} Population;

typedef struct EvolutionEnvironment {
    Population* current_population;
    Population* next_population;
    
    // 进化参数
    float mutation_rate;         // 变异率
    float crossover_rate;        // 交叉率
    float elite_ratio;           // 精英比例
    uint32_t tournament_size;    // 锦标赛大小
    
    // 适应度权重
    float performance_weight;    // 性能权重
    float stability_weight;      // 稳定性权重
    float innovation_weight;     // 创新性权重
    float efficiency_weight;     // 效率权重
    
    // 进化历史
    uint32_t total_generations;
    uint32_t stagnation_count;   // 停滞代数
    float best_fitness_history[MAX_GENERATIONS];
    
    // 测试环境
    char test_suite_path[256];
    char compiler_path[256];
    char runtime_path[256];
    
    bool verbose_logging;
    FILE* evolution_log;
} EvolutionEnvironment;

// ===============================================
// 适应度评估策略
// ===============================================

typedef enum {
    FITNESS_STRATEGY_PERFORMANCE = 1,  // 性能优先
    FITNESS_STRATEGY_STABILITY = 2,    // 稳定性优先
    FITNESS_STRATEGY_INNOVATION = 3,   // 创新性优先
    FITNESS_STRATEGY_BALANCED = 4,     // 平衡策略
    FITNESS_STRATEGY_ADAPTIVE = 5      // 自适应策略
} FitnessStrategy;

typedef struct FitnessTest {
    char name[64];
    char test_file[256];
    char expected_output[256];
    float weight;
    uint32_t timeout_ms;
    bool is_critical;
} FitnessTest;

// ===============================================
// 函数声明
// ===============================================

/**
 * 初始化进化环境
 */
EvolutionEnvironment* evolution_init(uint32_t population_size);

/**
 * 释放进化环境
 */
void evolution_free(EvolutionEnvironment* env);

/**
 * 创建初始种群
 */
int evolution_create_initial_population(EvolutionEnvironment* env);

/**
 * 评估个体适应度
 */
float evolution_evaluate_fitness(EvolutionEnvironment* env, Individual* individual);

/**
 * 选择操作 - 锦标赛选择
 */
Individual* evolution_tournament_selection(Population* population, uint32_t tournament_size);

/**
 * 交叉操作 - 单点交叉
 */
Individual* evolution_crossover(Individual* parent1, Individual* parent2, uint32_t offspring_id);

/**
 * 变异操作
 */
void evolution_mutate(Individual* individual, float mutation_rate);

/**
 * 进化一代
 */
int evolution_evolve_generation(EvolutionEnvironment* env);

/**
 * 运行完整的进化过程
 */
int evolution_run(EvolutionEnvironment* env, uint32_t max_generations);

/**
 * 保存最佳个体
 */
int evolution_save_best_individual(EvolutionEnvironment* env, const char* output_path);

/**
 * 加载个体
 */
Individual* evolution_load_individual(const char* file_path);

/**
 * 设置适应度测试
 */
int evolution_add_fitness_test(EvolutionEnvironment* env, const FitnessTest* test);

/**
 * 自适应参数调整
 */
void evolution_adaptive_parameters(EvolutionEnvironment* env);

/**
 * 多样性维护
 */
void evolution_maintain_diversity(Population* population);

/**
 * 精英保留策略
 */
void evolution_elitism(Population* current, Population* next, float elite_ratio);

/**
 * 性能基准测试
 */
float evolution_benchmark_performance(Individual* individual, const char* test_suite);

/**
 * 稳定性测试
 */
float evolution_test_stability(Individual* individual, uint32_t test_iterations);

/**
 * 创新性评估
 */
float evolution_evaluate_innovation(Individual* individual, Population* population);

/**
 * 生成进化报告
 */
int evolution_generate_report(EvolutionEnvironment* env, const char* report_path);

/**
 * 可视化进化过程
 */
int evolution_visualize_progress(EvolutionEnvironment* env, const char* output_path);

/**
 * 检查收敛条件
 */
bool evolution_check_convergence(EvolutionEnvironment* env);

/**
 * 重启进化（避免局部最优）
 */
int evolution_restart(EvolutionEnvironment* env);

#ifdef __cplusplus
}
#endif

#endif // EVOLUTION_STRATEGY_H
