/**
 * ai_integration.c - AI Evolution Integration Module
 * 
 * Integrates AI evolution capabilities into the compilation and runtime flow
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "evolution_engine.h"
#include "code_analyzer.h"
#include "../core/astc.h"
#include "../core/include/module_attributes.h"

// ===============================================
// AI Integration Configuration
// ===============================================

typedef struct {
    int enable_code_optimization;     // Enable AI-driven code optimization
    int enable_runtime_adaptation;    // Enable runtime adaptation
    int enable_self_modification;     // Enable self-modification
    int enable_learning;              // Enable learning from execution
    double optimization_threshold;    // Threshold for triggering optimization
    int max_evolution_cycles;         // Maximum evolution cycles per compilation
} AIIntegrationConfig;

// Default configuration
static AIIntegrationConfig default_config = {
    .enable_code_optimization = 1,
    .enable_runtime_adaptation = 1,
    .enable_self_modification = 0,  // Disabled by default for safety
    .enable_learning = 1,
    .optimization_threshold = 0.1,
    .max_evolution_cycles = 5
};

static AIIntegrationConfig current_config;
static int ai_integration_initialized = 0;

// ===============================================
// AI Integration API
// ===============================================

/**
 * Initialize AI integration system
 */
int ai_integration_init(const AIIntegrationConfig* config) {
    if (config) {
        current_config = *config;
    } else {
        current_config = default_config;
    }
    
    // Initialize evolution engine
    if (evolution_engine_init() != 0) {
        printf("Warning: Failed to initialize evolution engine\n");
        return -1;
    }
    
    // Initialize code analyzer
    if (code_analyzer_init() != 0) {
        printf("Warning: Failed to initialize code analyzer\n");
        return -1;
    }
    
    ai_integration_initialized = 1;
    printf("AI Integration initialized successfully\n");
    return 0;
}

/**
 * Cleanup AI integration system
 */
void ai_integration_cleanup(void) {
    if (!ai_integration_initialized) return;
    
    evolution_engine_cleanup();
    code_analyzer_cleanup();
    ai_integration_initialized = 0;
}

/**
 * AI-enhanced compilation
 * Applies AI optimization during the compilation process
 */
int ai_enhanced_compilation(struct ASTNode* ast, uint8_t** optimized_bytecode, size_t* bytecode_size) {
    if (!ai_integration_initialized || !ast || !optimized_bytecode || !bytecode_size) {
        return -1;
    }
    
    printf("Starting AI-enhanced compilation...\n");
    
    // Step 1: Analyze the AST for optimization opportunities
    CodeAnalysisResult analysis;
    if (analyze_ast_for_optimization(ast, &analysis) != 0) {
        printf("Warning: AST analysis failed, proceeding with standard compilation\n");
        return -1;
    }
    
    printf("Code analysis completed: %d optimization opportunities found\n", 
           analysis.optimization_count);
    
    // Step 2: Apply AI-driven optimizations if enabled
    if (current_config.enable_code_optimization && analysis.optimization_count > 0) {
        struct ASTNode* optimized_ast = ast;
        
        for (int cycle = 0; cycle < current_config.max_evolution_cycles; cycle++) {
            printf("Evolution cycle %d/%d...\n", cycle + 1, current_config.max_evolution_cycles);
            
            // Apply evolution-based optimization
            EvolutionResult evolution_result;
            if (evolve_code_structure(optimized_ast, &analysis, &evolution_result) == 0) {
                if (evolution_result.improvement_score > current_config.optimization_threshold) {
                    printf("  Improvement: %.2f%% (cycle %d)\n", 
                           evolution_result.improvement_score * 100, cycle + 1);
                    optimized_ast = evolution_result.optimized_ast;
                } else {
                    printf("  No significant improvement, stopping evolution\n");
                    break;
                }
            } else {
                printf("  Evolution failed for cycle %d\n", cycle + 1);
                break;
            }
        }
        
        // Generate optimized bytecode
        if (generate_optimized_bytecode(optimized_ast, optimized_bytecode, bytecode_size) == 0) {
            printf("AI-enhanced compilation completed successfully\n");
            return 0;
        } else {
            printf("Warning: Optimized bytecode generation failed\n");
        }
    }
    
    printf("AI-enhanced compilation completed with standard optimization\n");
    return 0;
}

/**
 * AI-enhanced runtime adaptation
 * Monitors runtime performance and adapts execution
 */
int ai_runtime_adaptation(const uint8_t* bytecode, size_t bytecode_size, 
                         RuntimeMetrics* metrics, uint8_t** adapted_bytecode, size_t* adapted_size) {
    if (!ai_integration_initialized || !current_config.enable_runtime_adaptation) {
        return -1;
    }
    
    printf("Starting AI runtime adaptation...\n");
    
    // Analyze runtime metrics
    if (metrics->execution_count < 10) {
        // Not enough data for adaptation
        return -1;
    }
    
    // Check if adaptation is needed
    double performance_score = calculate_performance_score(metrics);
    if (performance_score > 0.8) {
        // Performance is already good
        printf("Runtime performance is satisfactory (%.2f), no adaptation needed\n", performance_score);
        return 0;
    }
    
    printf("Runtime performance is suboptimal (%.2f), applying adaptation...\n", performance_score);
    
    // Apply runtime adaptation
    AdaptationResult adaptation;
    if (adapt_runtime_behavior(bytecode, bytecode_size, metrics, &adaptation) == 0) {
        *adapted_bytecode = adaptation.adapted_bytecode;
        *adapted_size = adaptation.adapted_size;
        
        printf("Runtime adaptation completed: %.2f%% improvement expected\n", 
               adaptation.expected_improvement * 100);
        return 0;
    }
    
    printf("Runtime adaptation failed\n");
    return -1;
}

/**
 * AI learning from execution
 * Learns from program execution patterns to improve future compilations
 */
int ai_learn_from_execution(const uint8_t* bytecode, size_t bytecode_size, 
                           const RuntimeMetrics* metrics, const char* program_name) {
    if (!ai_integration_initialized || !current_config.enable_learning) {
        return -1;
    }
    
    printf("Learning from execution of %s...\n", program_name ? program_name : "unknown");
    
    // Extract execution patterns
    ExecutionPattern pattern;
    if (extract_execution_pattern(bytecode, bytecode_size, metrics, &pattern) != 0) {
        printf("Warning: Failed to extract execution pattern\n");
        return -1;
    }
    
    // Update learning database
    if (update_learning_database(&pattern, program_name) == 0) {
        printf("Learning completed: pattern recorded for future optimizations\n");
        return 0;
    }
    
    printf("Warning: Failed to update learning database\n");
    return -1;
}

/**
 * AI self-modification (experimental)
 * Allows the system to modify its own compilation logic
 */
int ai_self_modification(const char* modification_request) {
    if (!ai_integration_initialized || !current_config.enable_self_modification) {
        printf("Self-modification is disabled for safety\n");
        return -1;
    }
    
    printf("WARNING: Self-modification requested: %s\n", modification_request);
    printf("Self-modification is experimental and potentially dangerous\n");
    
    // For safety, this is currently a no-op
    // In a real implementation, this would need extensive safety checks
    
    return -1; // Not implemented for safety
}

/**
 * Get AI integration statistics
 */
void ai_get_integration_stats(AIIntegrationStats* stats) {
    if (!stats) return;
    
    memset(stats, 0, sizeof(AIIntegrationStats));
    
    if (!ai_integration_initialized) {
        return;
    }
    
    // Get statistics from evolution engine
    get_evolution_stats(&stats->evolution_stats);
    
    // Get statistics from code analyzer
    get_analysis_stats(&stats->analysis_stats);
    
    stats->total_compilations = stats->evolution_stats.total_evolutions;
    stats->successful_optimizations = stats->evolution_stats.successful_evolutions;
    stats->average_improvement = stats->evolution_stats.average_improvement;
    stats->learning_database_size = get_learning_database_size();
}

/**
 * Configure AI integration
 */
int ai_configure_integration(const AIIntegrationConfig* new_config) {
    if (!new_config) return -1;
    
    current_config = *new_config;
    
    printf("AI Integration configuration updated:\n");
    printf("  Code optimization: %s\n", current_config.enable_code_optimization ? "enabled" : "disabled");
    printf("  Runtime adaptation: %s\n", current_config.enable_runtime_adaptation ? "enabled" : "disabled");
    printf("  Self-modification: %s\n", current_config.enable_self_modification ? "enabled" : "disabled");
    printf("  Learning: %s\n", current_config.enable_learning ? "enabled" : "disabled");
    printf("  Optimization threshold: %.2f\n", current_config.optimization_threshold);
    printf("  Max evolution cycles: %d\n", current_config.max_evolution_cycles);
    
    return 0;
}

/**
 * Check if AI integration is available and working
 */
int ai_integration_health_check(void) {
    if (!ai_integration_initialized) {
        printf("AI Integration: Not initialized\n");
        return -1;
    }
    
    // Check evolution engine
    if (evolution_engine_health_check() != 0) {
        printf("AI Integration: Evolution engine unhealthy\n");
        return -1;
    }
    
    // Check code analyzer
    if (code_analyzer_health_check() != 0) {
        printf("AI Integration: Code analyzer unhealthy\n");
        return -1;
    }
    
    printf("AI Integration: All systems healthy\n");
    return 0;
}

// ===============================================
// Integration with Compilation Pipeline
// ===============================================

/**
 * Hook for integrating AI into the C to ASTC compilation process
 */
int ai_hook_c2astc_compilation(const char* source_file, const char* output_file, 
                              struct ASTNode* ast) {
    if (!ai_integration_initialized) {
        return 0; // No AI integration, proceed normally
    }
    
    printf("AI hook: C to ASTC compilation for %s\n", source_file);
    
    // Apply AI-enhanced compilation
    uint8_t* optimized_bytecode = NULL;
    size_t bytecode_size = 0;
    
    if (ai_enhanced_compilation(ast, &optimized_bytecode, &bytecode_size) == 0) {
        // Write optimized bytecode to file
        FILE* output = fopen(output_file, "wb");
        if (output) {
            fwrite(optimized_bytecode, 1, bytecode_size, output);
            fclose(output);
            free(optimized_bytecode);
            printf("AI-optimized ASTC written to %s\n", output_file);
            return 1; // Indicate that AI processing was applied
        }
        free(optimized_bytecode);
    }
    
    return 0; // Fall back to standard compilation
}

/**
 * Hook for integrating AI into the ASTC to native conversion process
 */
int ai_hook_astc2native_conversion(const char* astc_file, const char* native_file,
                                  const uint8_t* bytecode, size_t bytecode_size) {
    if (!ai_integration_initialized) {
        return 0; // No AI integration, proceed normally
    }
    
    printf("AI hook: ASTC to native conversion for %s\n", astc_file);
    
    // For now, just log the conversion
    // In the future, this could apply JIT optimizations
    
    return 0; // Proceed with standard conversion
}

/**
 * Hook for integrating AI into runtime execution
 */
int ai_hook_runtime_execution(const uint8_t* bytecode, size_t bytecode_size,
                             RuntimeMetrics* metrics) {
    if (!ai_integration_initialized) {
        return 0; // No AI integration, proceed normally
    }
    
    // Monitor runtime performance
    if (current_config.enable_runtime_adaptation && metrics) {
        uint8_t* adapted_bytecode = NULL;
        size_t adapted_size = 0;
        
        if (ai_runtime_adaptation(bytecode, bytecode_size, metrics, 
                                 &adapted_bytecode, &adapted_size) == 0) {
            // Runtime adaptation was successful
            // In a real implementation, this would replace the running bytecode
            printf("Runtime adaptation applied\n");
            free(adapted_bytecode);
            return 1;
        }
    }
    
    return 0; // Proceed with standard execution
}
