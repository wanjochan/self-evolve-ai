/**
 * performance_optimizer.h - Performance Optimization System
 * 
 * Header for comprehensive performance optimization including JIT compiler
 * optimization, memory management optimization, and module loading optimization
 */

#ifndef PERFORMANCE_OPTIMIZER_H
#define PERFORMANCE_OPTIMIZER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Optimization levels
typedef enum {
    OPT_LEVEL_NONE = 0,        // No optimization
    OPT_LEVEL_BASIC = 1,       // Basic optimizations
    OPT_LEVEL_STANDARD = 2,    // Standard optimizations
    OPT_LEVEL_AGGRESSIVE = 3   // Aggressive optimizations
} OptimizationLevel;

// Optimization categories
typedef enum {
    OPT_CATEGORY_JIT = 1,
    OPT_CATEGORY_MEMORY = 2,
    OPT_CATEGORY_MODULE_LOADING = 3,
    OPT_CATEGORY_INSTRUCTION = 4,
    OPT_CATEGORY_CACHE = 5,
    OPT_CATEGORY_BRANCH_PREDICTION = 6
} OptimizationCategory;

// Performance metrics
typedef struct {
    uint64_t execution_time_ns;
    uint64_t memory_usage_bytes;
    uint64_t cache_misses;
    uint64_t branch_mispredictions;
    uint64_t instruction_count;
    uint64_t jit_compilation_time_ns;
    uint64_t module_load_time_ns;
    double cpu_utilization;
} PerformanceMetrics;

// Optimization configuration
typedef struct {
    OptimizationLevel level;
    bool enable_jit_optimization;
    bool enable_memory_optimization;
    bool enable_module_optimization;
    bool enable_instruction_optimization;
    bool enable_cache_optimization;
    bool enable_branch_optimization;
    
    // JIT optimization settings
    int jit_threshold;
    int max_inline_depth;
    bool enable_loop_unrolling;
    bool enable_dead_code_elimination;
    
    // Memory optimization settings
    size_t memory_pool_size;
    bool enable_garbage_collection;
    bool enable_memory_compaction;
    
    // Module optimization settings
    bool enable_module_caching;
    bool enable_lazy_loading;
    bool enable_preloading;
} OptimizationConfig;

// Hot spot information
typedef struct {
    void* address;
    uint64_t hit_count;
    uint64_t execution_time;
    double hotness_score;
} HotSpotInfo;

// Optimization statistics
typedef struct {
    uint64_t optimizations_applied;
    uint64_t performance_improvements;
    uint64_t optimization_failures;
    uint64_t total_optimization_time_ns;
    int hot_spot_count;
} OptimizationStats;

// Core optimization functions

/**
 * Initialize performance optimizer
 * @return 0 on success, -1 on error
 */
int performance_optimizer_init(void);

/**
 * Cleanup performance optimizer
 */
void performance_optimizer_cleanup(void);

/**
 * Configure performance optimizer
 * @param config Optimization configuration
 * @return 0 on success, -1 on error
 */
int configure_performance_optimizer(const OptimizationConfig* config);

/**
 * Get optimization configuration
 * @param config Pointer to store configuration
 */
void get_optimization_config(OptimizationConfig* config);

// Performance measurement

/**
 * Measure baseline performance
 * @return 0 on success, -1 on error
 */
int measure_baseline_performance(void);

/**
 * Capture current performance metrics
 * @param metrics Pointer to store metrics
 * @return 0 on success, -1 on error
 */
int capture_performance_metrics(PerformanceMetrics* metrics);

/**
 * Calculate performance improvement
 * @return Improvement ratio (positive = improvement, negative = degradation)
 */
double calculate_performance_improvement(void);

/**
 * Calculate performance score
 * @param metrics Performance metrics
 * @return Performance score (higher is better)
 */
double calculate_performance_score(const PerformanceMetrics* metrics);

// Optimization execution

/**
 * Run comprehensive performance optimization
 * @return Number of optimizations applied, -1 on error
 */
int run_performance_optimization(void);

/**
 * Apply specific optimization category
 * @param category Optimization category
 * @return Number of optimizations applied, -1 on error
 */
int apply_optimization_category(OptimizationCategory category);

/**
 * Apply JIT optimizations
 * @return Number of optimizations applied
 */
int apply_jit_optimizations(void);

/**
 * Apply memory optimizations
 * @return Number of optimizations applied
 */
int apply_memory_optimizations(void);

/**
 * Apply module loading optimizations
 * @return Number of optimizations applied
 */
int apply_module_optimizations(void);

/**
 * Apply instruction optimizations
 * @return Number of optimizations applied
 */
int apply_instruction_optimizations(void);

/**
 * Apply cache optimizations
 * @return Number of optimizations applied
 */
int apply_cache_optimizations(void);

/**
 * Apply branch prediction optimizations
 * @return Number of optimizations applied
 */
int apply_branch_optimizations(void);

// Specific optimization techniques

/**
 * Eliminate dead code
 * @return Number of optimizations applied
 */
int eliminate_dead_code(void);

/**
 * Unroll loops
 * @return Number of optimizations applied
 */
int unroll_loops(void);

/**
 * Inline functions
 * @param max_depth Maximum inlining depth
 * @return Number of optimizations applied
 */
int inline_functions(int max_depth);

/**
 * Fold constants
 * @return Number of optimizations applied
 */
int fold_constants(void);

/**
 * Optimize register allocation
 * @return Number of optimizations applied
 */
int optimize_register_allocation(void);

/**
 * Optimize memory pools
 * @return Number of optimizations applied
 */
int optimize_memory_pools(void);

/**
 * Optimize garbage collection
 * @return Number of optimizations applied
 */
int optimize_garbage_collection(void);

/**
 * Compact memory
 * @return Number of optimizations applied
 */
int compact_memory(void);

/**
 * Optimize memory layout
 * @return Number of optimizations applied
 */
int optimize_memory_layout(void);

/**
 * Enable module caching
 * @return Number of optimizations applied
 */
int enable_module_caching(void);

/**
 * Enable lazy loading
 * @return Number of optimizations applied
 */
int enable_lazy_loading(void);

/**
 * Preload modules
 * @return Number of optimizations applied
 */
int preload_modules(void);

/**
 * Optimize symbol resolution
 * @return Number of optimizations applied
 */
int optimize_symbol_resolution(void);

// Hot spot analysis

/**
 * Identify performance hot spots
 * @param hot_spots Array to store hot spot information
 * @param max_hot_spots Maximum number of hot spots to return
 * @return Number of hot spots found
 */
int identify_hot_spots(HotSpotInfo* hot_spots, int max_hot_spots);

/**
 * Add hot spot
 * @param address Code address
 * @param execution_time Execution time
 * @return 0 on success, -1 on error
 */
int add_hot_spot(void* address, uint64_t execution_time);

/**
 * Update hot spot statistics
 * @param address Code address
 * @param execution_time Additional execution time
 * @return 0 on success, -1 on error
 */
int update_hot_spot(void* address, uint64_t execution_time);

/**
 * Get hot spot count
 * @return Number of tracked hot spots
 */
int get_hot_spot_count(void);

// Statistics and monitoring

/**
 * Get optimization statistics
 * @param optimizations_applied Pointer to store optimization count
 * @param improvements Pointer to store improvement count
 * @param failures Pointer to store failure count
 */
void get_optimization_stats(uint64_t* optimizations_applied, uint64_t* improvements, uint64_t* failures);

/**
 * Get detailed optimization statistics
 * @param stats Pointer to store detailed statistics
 */
void get_detailed_optimization_stats(OptimizationStats* stats);

/**
 * Get current performance metrics
 * @param metrics Pointer to store current metrics
 */
void get_current_performance_metrics(PerformanceMetrics* metrics);

/**
 * Get baseline performance metrics
 * @param metrics Pointer to store baseline metrics
 */
void get_baseline_performance_metrics(PerformanceMetrics* metrics);

/**
 * Reset optimization statistics
 */
void reset_optimization_stats(void);

// Utility functions

/**
 * Get optimization level string
 * @param level Optimization level
 * @return String representation
 */
const char* get_optimization_level_string(OptimizationLevel level);

/**
 * Get optimization category string
 * @param category Optimization category
 * @return String representation
 */
const char* get_optimization_category_string(OptimizationCategory category);

/**
 * Validate optimization configuration
 * @param config Configuration to validate
 * @return true if valid, false otherwise
 */
bool validate_optimization_config(const OptimizationConfig* config);

/**
 * Create default optimization configuration
 * @param level Optimization level
 * @param config Pointer to store configuration
 * @return 0 on success, -1 on error
 */
int create_default_optimization_config(OptimizationLevel level, OptimizationConfig* config);

/**
 * Compare performance metrics
 * @param metrics1 First metrics
 * @param metrics2 Second metrics
 * @return Comparison result (-1, 0, 1)
 */
int compare_performance_metrics(const PerformanceMetrics* metrics1, const PerformanceMetrics* metrics2);

/**
 * Format performance metrics to string
 * @param metrics Performance metrics
 * @param buffer Buffer to store string
 * @param buffer_size Size of buffer
 * @return 0 on success, -1 on error
 */
int format_performance_metrics(const PerformanceMetrics* metrics, char* buffer, size_t buffer_size);

// Error codes
#define OPTIMIZER_SUCCESS           0
#define OPTIMIZER_ERROR_INVALID     -1
#define OPTIMIZER_ERROR_NOT_INIT    -2
#define OPTIMIZER_ERROR_CONFIG      -3
#define OPTIMIZER_ERROR_METRICS     -4

// Default configuration values
#define DEFAULT_JIT_THRESHOLD       10
#define DEFAULT_MAX_INLINE_DEPTH    3
#define DEFAULT_MEMORY_POOL_SIZE    (64 * 1024 * 1024)  // 64MB
#define DEFAULT_HOT_SPOT_THRESHOLD  1000

#ifdef __cplusplus
}
#endif

#endif // PERFORMANCE_OPTIMIZER_H
