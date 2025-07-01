/**
 * evolution_engine_enhanced.h - Enhanced AI Evolution Engine
 * 
 * Header for advanced AI evolution engine with module integration
 */

#ifndef EVOLUTION_ENGINE_ENHANCED_H
#define EVOLUTION_ENGINE_ENHANCED_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Enhanced evolution strategies
typedef enum {
    EVOLUTION_STRATEGY_RANDOM = 0,      // Random mutations
    EVOLUTION_STRATEGY_GUIDED = 1,      // Guided by fitness function
    EVOLUTION_STRATEGY_GENETIC = 2,     // Genetic algorithm approach
    EVOLUTION_STRATEGY_NEURAL = 3,      // Neural network guided
    EVOLUTION_STRATEGY_HYBRID = 4       // Combination of strategies
} EvolutionStrategy;

// Evolution metrics
typedef struct {
    uint64_t total_iterations;
    uint64_t successful_mutations;
    uint64_t failed_mutations;
    uint64_t code_improvements;
    uint64_t performance_gains;
    uint64_t bug_fixes;
    double average_fitness;
    double best_fitness;
    time_t last_improvement;
} EvolutionMetrics;

// Evolution target types
typedef enum {
    EVOLUTION_TARGET_PERFORMANCE = 1,
    EVOLUTION_TARGET_MEMORY = 2,
    EVOLUTION_TARGET_RELIABILITY = 3,
    EVOLUTION_TARGET_MODULARITY = 4,
    EVOLUTION_TARGET_MAINTAINABILITY = 5
} EvolutionTarget;

// Enhanced evolution engine functions

/**
 * Initialize enhanced evolution engine
 * @param strategy Evolution strategy to use
 * @return 0 on success, -1 on error
 */
int evolution_engine_enhanced_init(EvolutionStrategy strategy);

/**
 * Cleanup enhanced evolution engine
 */
void evolution_engine_enhanced_cleanup(void);

/**
 * Register AI evolution interfaces with module system
 * @return 0 on success, -1 on error
 */
int evolution_register_ai_interfaces(void);

/**
 * Analyze code for improvement opportunities
 * @param file_path Path to code file to analyze
 * @return Number of improvement opportunities found, -1 on error
 */
int evolution_analyze_code_for_improvement(const char* file_path);

/**
 * Generate improved code using AI strategies
 * @param original_code Original source code
 * @param improvement_target Target for improvement
 * @return Improved code (caller must free), NULL on error
 */
char* evolution_generate_improved_code_enhanced(const char* original_code, const char* improvement_target);

/**
 * Evaluate code fitness
 * @param code Code to evaluate
 * @return Fitness score (higher is better)
 */
double evolution_evaluate_code_fitness(const char* code);

/**
 * Get evolution metrics
 * @param metrics Pointer to store metrics
 */
void evolution_get_enhanced_metrics(EvolutionMetrics* metrics);

/**
 * Set evolution strategy
 * @param strategy Strategy to use
 */
void evolution_set_strategy(EvolutionStrategy strategy);

/**
 * Enable/disable autonomous mode
 * @param enabled true to enable, false to disable
 */
void evolution_set_autonomous_mode(bool enabled);

/**
 * Set evolution target
 * @param target Target type for evolution
 */
void evolution_set_target(EvolutionTarget target);

/**
 * Run one evolution iteration
 * @return 0 on success, -1 on error
 */
int evolution_run_iteration(void);

/**
 * Run continuous evolution
 * @param max_iterations Maximum iterations to run (0 = unlimited)
 * @return 0 on success, -1 on error
 */
int evolution_run_continuous(uint64_t max_iterations);

// Strategy-specific improvement functions

/**
 * Apply guided improvements
 * @param code Code to improve (modified in place)
 * @param max_len Maximum length of code buffer
 * @param target Improvement target
 */
void evolution_apply_guided_improvements(char* code, size_t max_len, const char* target);

/**
 * Apply genetic algorithm improvements
 * @param code Code to improve (modified in place)
 * @param max_len Maximum length of code buffer
 * @param target Improvement target
 */
void evolution_apply_genetic_improvements(char* code, size_t max_len, const char* target);

/**
 * Apply neural network guided improvements
 * @param code Code to improve (modified in place)
 * @param max_len Maximum length of code buffer
 * @param target Improvement target
 */
void evolution_apply_neural_improvements(char* code, size_t max_len, const char* target);

/**
 * Apply hybrid improvements
 * @param code Code to improve (modified in place)
 * @param max_len Maximum length of code buffer
 * @param target Improvement target
 */
void evolution_apply_hybrid_improvements(char* code, size_t max_len, const char* target);

/**
 * Apply random improvements
 * @param code Code to improve (modified in place)
 * @param max_len Maximum length of code buffer
 * @param target Improvement target
 */
void evolution_apply_random_improvements(char* code, size_t max_len, const char* target);

// Advanced evolution features

/**
 * Train neural network for code improvement
 * @param training_data_path Path to training data
 * @return 0 on success, -1 on error
 */
int evolution_train_neural_network(const char* training_data_path);

/**
 * Save evolution state
 * @param file_path Path to save state to
 * @return 0 on success, -1 on error
 */
int evolution_save_state(const char* file_path);

/**
 * Load evolution state
 * @param file_path Path to load state from
 * @return 0 on success, -1 on error
 */
int evolution_load_state(const char* file_path);

/**
 * Generate evolution report
 * @param output_path Path to write report to
 * @return 0 on success, -1 on error
 */
int evolution_generate_report(const char* output_path);

/**
 * Validate evolved code
 * @param code Code to validate
 * @param original_code Original code for comparison
 * @return true if valid improvement, false otherwise
 */
bool evolution_validate_improvement(const char* code, const char* original_code);

/**
 * Rollback to previous version
 * @param steps Number of steps to rollback
 * @return 0 on success, -1 on error
 */
int evolution_rollback(int steps);

// Configuration and tuning

/**
 * Set fitness weights
 * @param performance_weight Weight for performance improvements
 * @param memory_weight Weight for memory improvements
 * @param reliability_weight Weight for reliability improvements
 * @param maintainability_weight Weight for maintainability improvements
 */
void evolution_set_fitness_weights(double performance_weight, double memory_weight,
                                  double reliability_weight, double maintainability_weight);

/**
 * Set mutation rate
 * @param rate Mutation rate (0.0 to 1.0)
 */
void evolution_set_mutation_rate(double rate);

/**
 * Set crossover rate for genetic algorithm
 * @param rate Crossover rate (0.0 to 1.0)
 */
void evolution_set_crossover_rate(double rate);

/**
 * Set population size for genetic algorithm
 * @param size Population size
 */
void evolution_set_population_size(int size);

/**
 * Enable/disable learning mode
 * @param enabled true to enable, false to disable
 */
void evolution_set_learning_enabled(bool enabled);

/**
 * Enable/disable module evolution
 * @param enabled true to enable, false to disable
 */
void evolution_set_module_evolution_enabled(bool enabled);

// Utility functions

/**
 * Get strategy name
 * @param strategy Strategy enum value
 * @return Strategy name string
 */
const char* evolution_get_strategy_name(EvolutionStrategy strategy);

/**
 * Get target name
 * @param target Target enum value
 * @return Target name string
 */
const char* evolution_get_target_name(EvolutionTarget target);

/**
 * Check if evolution is running
 * @return true if running, false otherwise
 */
bool evolution_is_running(void);

/**
 * Get current fitness score
 * @return Current fitness score
 */
double evolution_get_current_fitness(void);

/**
 * Get evolution progress
 * @return Progress percentage (0.0 to 100.0)
 */
double evolution_get_progress(void);

// Error codes
#define EVOLUTION_SUCCESS           0
#define EVOLUTION_ERROR_INVALID     -1
#define EVOLUTION_ERROR_MEMORY      -2
#define EVOLUTION_ERROR_FILE_IO     -3
#define EVOLUTION_ERROR_COMPILATION -4
#define EVOLUTION_ERROR_VALIDATION  -5
#define EVOLUTION_ERROR_NETWORK     -6

#ifdef __cplusplus
}
#endif

#endif // EVOLUTION_ENGINE_ENHANCED_H
