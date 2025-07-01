/**
 * ai_integration.h - AI Evolution Integration Header
 * 
 * Header file for AI evolution integration functionality
 */

#ifndef AI_INTEGRATION_H
#define AI_INTEGRATION_H

#include <stdint.h>
#include <stddef.h>
#include "../core/include/core_astc.h"

// ===============================================
// Configuration Structures
// ===============================================

typedef struct {
    int enable_code_optimization;     // Enable AI-driven code optimization
    int enable_runtime_adaptation;    // Enable runtime adaptation
    int enable_self_modification;     // Enable self-modification
    int enable_learning;              // Enable learning from execution
    double optimization_threshold;    // Threshold for triggering optimization
    int max_evolution_cycles;         // Maximum evolution cycles per compilation
} AIIntegrationConfig;

// ===============================================
// Runtime Metrics
// ===============================================

typedef struct {
    uint64_t execution_count;         // Number of times executed
    uint64_t total_execution_time;    // Total execution time in microseconds
    uint64_t memory_usage_peak;       // Peak memory usage in bytes
    uint64_t cache_misses;            // Number of cache misses
    uint64_t branch_mispredictions;   // Number of branch mispredictions
    double average_cpu_usage;         // Average CPU usage percentage
    int error_count;                  // Number of runtime errors
} RuntimeMetrics;

// ===============================================
// AI Analysis Results
// ===============================================

typedef struct {
    int optimization_count;           // Number of optimization opportunities
    double complexity_score;          // Code complexity score (0.0 - 1.0)
    double performance_prediction;    // Predicted performance score
    char* bottleneck_description;     // Description of performance bottlenecks
    int hotspot_count;               // Number of performance hotspots
    struct ASTNode** hotspot_nodes;  // AST nodes identified as hotspots
} CodeAnalysisResult;

typedef struct {
    struct ASTNode* optimized_ast;    // Optimized AST
    double improvement_score;         // Expected improvement (0.0 - 1.0)
    char* optimization_description;   // Description of optimizations applied
    int modifications_count;          // Number of modifications made
} EvolutionResult;

typedef struct {
    uint8_t* adapted_bytecode;        // Adapted bytecode
    size_t adapted_size;              // Size of adapted bytecode
    double expected_improvement;      // Expected performance improvement
    char* adaptation_description;     // Description of adaptations made
} AdaptationResult;

typedef struct {
    char* pattern_name;               // Name of the execution pattern
    double frequency;                 // Frequency of this pattern (0.0 - 1.0)
    uint8_t* pattern_signature;      // Signature of the pattern
    size_t signature_size;            // Size of the signature
    RuntimeMetrics typical_metrics;   // Typical metrics for this pattern
} ExecutionPattern;

// ===============================================
// Statistics Structures
// ===============================================

typedef struct {
    uint64_t total_evolutions;        // Total number of evolutions performed
    uint64_t successful_evolutions;   // Number of successful evolutions
    double average_improvement;       // Average improvement percentage
    uint64_t total_evolution_time;    // Total time spent on evolution
} EvolutionStats;

typedef struct {
    uint64_t total_analyses;          // Total number of code analyses
    uint64_t hotspots_found;          // Total hotspots found
    double average_complexity;        // Average code complexity
    uint64_t total_analysis_time;     // Total time spent on analysis
} AnalysisStats;

typedef struct {
    EvolutionStats evolution_stats;   // Evolution engine statistics
    AnalysisStats analysis_stats;     // Code analyzer statistics
    uint64_t total_compilations;      // Total AI-enhanced compilations
    uint64_t successful_optimizations; // Successful optimizations
    double average_improvement;       // Average improvement percentage
    uint64_t learning_database_size;  // Size of learning database
} AIIntegrationStats;

// ===============================================
// Core AI Integration Functions
// ===============================================

/**
 * Initialize AI integration system
 * @param config Configuration for AI integration (NULL for defaults)
 * @return 0 on success, -1 on error
 */
int ai_integration_init(const AIIntegrationConfig* config);

/**
 * Cleanup AI integration system
 */
void ai_integration_cleanup(void);

/**
 * AI-enhanced compilation
 * @param ast Input AST to optimize
 * @param optimized_bytecode Output optimized bytecode
 * @param bytecode_size Output bytecode size
 * @return 0 on success, -1 on error
 */
int ai_enhanced_compilation(struct ASTNode* ast, uint8_t** optimized_bytecode, size_t* bytecode_size);

/**
 * AI-enhanced runtime adaptation
 * @param bytecode Input bytecode
 * @param bytecode_size Input bytecode size
 * @param metrics Runtime metrics
 * @param adapted_bytecode Output adapted bytecode
 * @param adapted_size Output adapted size
 * @return 0 on success, -1 on error
 */
int ai_runtime_adaptation(const uint8_t* bytecode, size_t bytecode_size, 
                         RuntimeMetrics* metrics, uint8_t** adapted_bytecode, size_t* adapted_size);

/**
 * AI learning from execution
 * @param bytecode Executed bytecode
 * @param bytecode_size Bytecode size
 * @param metrics Execution metrics
 * @param program_name Program name for learning context
 * @return 0 on success, -1 on error
 */
int ai_learn_from_execution(const uint8_t* bytecode, size_t bytecode_size, 
                           const RuntimeMetrics* metrics, const char* program_name);

/**
 * AI self-modification (experimental)
 * @param modification_request Description of requested modification
 * @return 0 on success, -1 on error or if disabled
 */
int ai_self_modification(const char* modification_request);

// ===============================================
// Configuration and Monitoring
// ===============================================

/**
 * Configure AI integration
 * @param new_config New configuration
 * @return 0 on success, -1 on error
 */
int ai_configure_integration(const AIIntegrationConfig* new_config);

/**
 * Get AI integration statistics
 * @param stats Output statistics structure
 */
void ai_get_integration_stats(AIIntegrationStats* stats);

/**
 * Check AI integration health
 * @return 0 if healthy, -1 if unhealthy
 */
int ai_integration_health_check(void);

// ===============================================
// Pipeline Integration Hooks
// ===============================================

/**
 * Hook for C to ASTC compilation
 * @param source_file Source file path
 * @param output_file Output file path
 * @param ast Parsed AST
 * @return 1 if AI processing was applied, 0 if standard processing should continue
 */
int ai_hook_c2astc_compilation(const char* source_file, const char* output_file, 
                              struct ASTNode* ast);

/**
 * Hook for ASTC to native conversion
 * @param astc_file ASTC file path
 * @param native_file Native file path
 * @param bytecode ASTC bytecode
 * @param bytecode_size Bytecode size
 * @return 1 if AI processing was applied, 0 if standard processing should continue
 */
int ai_hook_astc2native_conversion(const char* astc_file, const char* native_file,
                                  const uint8_t* bytecode, size_t bytecode_size);

/**
 * Hook for runtime execution
 * @param bytecode Executing bytecode
 * @param bytecode_size Bytecode size
 * @param metrics Runtime metrics
 * @return 1 if AI processing was applied, 0 if standard processing should continue
 */
int ai_hook_runtime_execution(const uint8_t* bytecode, size_t bytecode_size,
                             RuntimeMetrics* metrics);

// ===============================================
// Utility Functions
// ===============================================

/**
 * Calculate performance score from metrics
 * @param metrics Runtime metrics
 * @return Performance score (0.0 - 1.0, higher is better)
 */
double calculate_performance_score(const RuntimeMetrics* metrics);

/**
 * Get learning database size
 * @return Number of patterns in learning database
 */
uint64_t get_learning_database_size(void);

// ===============================================
// Default Configuration
// ===============================================

/**
 * Get default AI integration configuration
 * @return Default configuration structure
 */
AIIntegrationConfig ai_get_default_config(void);

#endif // AI_INTEGRATION_H
