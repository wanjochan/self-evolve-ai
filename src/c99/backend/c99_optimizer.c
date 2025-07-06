/**
 * c99_optimizer.c - C99 Code Optimizer Implementation
 */

#include "c99_optimizer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ===============================================
// Optimization Pass Registry
// ===============================================

static const OptimizationPass optimization_passes[] = {
    {
        OPT_PASS_CONSTANT_FOLDING,
        "constant-folding",
        "Fold constant expressions at compile time",
        opt_pass_constant_folding,
        false, false
    },
    {
        OPT_PASS_DEAD_CODE_ELIMINATION,
        "dead-code-elimination",
        "Remove unreachable and unused code",
        opt_pass_dead_code_elimination,
        false, true
    },
    {
        OPT_PASS_COMMON_SUBEXPRESSION,
        "common-subexpression",
        "Eliminate common subexpressions",
        opt_pass_common_subexpression,
        true, false
    },
    {
        OPT_PASS_LOOP_INVARIANT,
        "loop-invariant",
        "Move loop-invariant code outside loops",
        opt_pass_loop_optimization,
        false, true
    },
    {
        OPT_PASS_INLINE_EXPANSION,
        "inline-expansion",
        "Inline small functions",
        opt_pass_inline_expansion,
        false, true
    }
};

static const size_t num_optimization_passes = sizeof(optimization_passes) / sizeof(OptimizationPass);

// ===============================================
// Optimizer Context Management
// ===============================================

OptimizerContext* optimizer_create(OptimizationLevel level) {
    OptimizerContext* optimizer = malloc(sizeof(OptimizerContext));
    if (!optimizer) return NULL;
    
    memset(optimizer, 0, sizeof(OptimizerContext));
    optimizer->level = level;
    
    // Enable passes based on optimization level
    switch (level) {
        case OPT_LEVEL_NONE:
            // No optimizations
            break;
            
        case OPT_LEVEL_BASIC:
            optimizer->enabled_passes[OPT_PASS_CONSTANT_FOLDING] = true;
            optimizer->enabled_passes[OPT_PASS_DEAD_CODE_ELIMINATION] = true;
            break;
            
        case OPT_LEVEL_STANDARD:
            optimizer->enabled_passes[OPT_PASS_CONSTANT_FOLDING] = true;
            optimizer->enabled_passes[OPT_PASS_DEAD_CODE_ELIMINATION] = true;
            optimizer->enabled_passes[OPT_PASS_COMMON_SUBEXPRESSION] = true;
            optimizer->enabled_passes[OPT_PASS_LOOP_INVARIANT] = true;
            break;
            
        case OPT_LEVEL_AGGRESSIVE:
            // Enable all passes
            for (int i = 0; i < OPT_PASS_COUNT; i++) {
                optimizer->enabled_passes[i] = true;
            }
            optimizer->aggressive_inlining = true;
            break;
    }
    
    // Set default options
    optimizer->preserve_debug_info = true;
    optimizer->max_inline_size = 50;
    optimizer->max_unroll_count = 4;
    
    printf("Optimizer: Created optimizer context (level %d)\n", level);
    
    return optimizer;
}

void optimizer_destroy(OptimizerContext* optimizer) {
    if (!optimizer) return;
    
    free(optimizer);
}

// ===============================================
// Main Optimization Functions
// ===============================================

bool optimizer_optimize_ast(OptimizerContext* optimizer, struct ASTNode* ast) {
    if (!optimizer || !ast) return false;
    
    printf("Optimizer: Starting AST optimization (level %d)\n", optimizer->level);
    
    optimizer->original_size = optimizer_analyze_complexity(ast);
    
    // Run enabled optimization passes
    for (int i = 0; i < OPT_PASS_COUNT; i++) {
        if (optimizer->enabled_passes[i] && i < (int)num_optimization_passes) {
            const OptimizationPass* pass = &optimization_passes[i];
            
            printf("Optimizer: Running pass '%s'\n", pass->name);
            
            if (pass->run && pass->run(optimizer, ast)) {
                optimizer->passes_run++;
                optimizer->optimizations_applied++;
            } else {
                printf("Optimizer: Pass '%s' failed or made no changes\n", pass->name);
            }
        }
    }
    
    optimizer->optimized_size = optimizer_analyze_complexity(ast);
    
    printf("Optimizer: Completed optimization - %d passes run, %d optimizations applied\n",
           optimizer->passes_run, optimizer->optimizations_applied);
    
    return true;
}

bool optimizer_optimize_bytecode(OptimizerContext* optimizer, uint8_t* bytecode, size_t size) {
    if (!optimizer || !bytecode) return false;
    
    printf("Optimizer: Starting bytecode optimization\n");
    
    // Run peephole optimization on bytecode
    if (optimizer->enabled_passes[OPT_PASS_PEEPHOLE]) {
        return opt_pass_peephole(optimizer, bytecode, size);
    }
    
    return true;
}

bool optimizer_run_pass(OptimizerContext* optimizer, OptimizationPassType pass, struct ASTNode* ast) {
    if (!optimizer || !ast || pass >= OPT_PASS_COUNT) return false;
    
    if (pass < num_optimization_passes) {
        const OptimizationPass* opt_pass = &optimization_passes[pass];
        if (opt_pass->run) {
            return opt_pass->run(optimizer, ast);
        }
    }
    
    return false;
}

// ===============================================
// Optimization Pass Implementations
// ===============================================

bool opt_pass_constant_folding(OptimizerContext* optimizer, struct ASTNode* ast) {
    if (!optimizer || !ast) return false;
    
    printf("Optimizer: Running constant folding pass\n");
    
    // TODO: Implement constant folding
    // This would traverse the AST and fold constant expressions
    // For example: 2 + 3 -> 5, true && false -> false
    
    return true;
}

bool opt_pass_dead_code_elimination(OptimizerContext* optimizer, struct ASTNode* ast) {
    if (!optimizer || !ast) return false;
    
    printf("Optimizer: Running dead code elimination pass\n");
    
    // TODO: Implement dead code elimination
    // This would remove unreachable code and unused variables
    
    return true;
}

bool opt_pass_common_subexpression(OptimizerContext* optimizer, struct ASTNode* ast) {
    if (!optimizer || !ast) return false;
    
    printf("Optimizer: Running common subexpression elimination pass\n");
    
    // TODO: Implement common subexpression elimination
    // This would identify and eliminate redundant computations
    
    return true;
}

bool opt_pass_loop_optimization(OptimizerContext* optimizer, struct ASTNode* ast) {
    if (!optimizer || !ast) return false;
    
    printf("Optimizer: Running loop optimization pass\n");
    
    // TODO: Implement loop optimizations
    // This would include loop unrolling, loop invariant code motion, etc.
    
    return true;
}

bool opt_pass_inline_expansion(OptimizerContext* optimizer, struct ASTNode* ast) {
    if (!optimizer || !ast) return false;
    
    printf("Optimizer: Running function inlining pass\n");
    
    // TODO: Implement function inlining
    // This would inline small functions to reduce call overhead
    
    return true;
}

bool opt_pass_peephole(OptimizerContext* optimizer, uint8_t* bytecode, size_t size) {
    if (!optimizer || !bytecode) return false;
    
    printf("Optimizer: Running peephole optimization pass\n");
    
    // TODO: Implement peephole optimization
    // This would optimize small sequences of bytecode instructions
    
    return true;
}

// ===============================================
// Analysis Functions
// ===============================================

int optimizer_analyze_complexity(struct ASTNode* ast) {
    if (!ast) return 0;
    
    // TODO: Implement complexity analysis
    // This would count nodes, estimate execution cost, etc.
    
    return 100; // Placeholder
}

double optimizer_estimate_benefit(OptimizerContext* optimizer, OptimizationPassType pass, struct ASTNode* ast) {
    if (!optimizer || !ast) return 0.0;
    
    // TODO: Implement benefit estimation
    // This would estimate the performance improvement from a pass
    
    return 0.1; // Placeholder
}

bool optimizer_is_safe(OptimizerContext* optimizer, OptimizationPassType pass, struct ASTNode* ast) {
    if (!optimizer || !ast) return false;
    
    // TODO: Implement safety analysis
    // This would check if an optimization is safe to apply
    
    return true; // Placeholder
}

// ===============================================
// Utility Functions
// ===============================================

const char* optimizer_get_pass_name(OptimizationPassType pass) {
    if (pass < num_optimization_passes) {
        return optimization_passes[pass].name;
    }
    return "unknown";
}

const char* optimizer_get_level_name(OptimizationLevel level) {
    switch (level) {
        case OPT_LEVEL_NONE: return "none";
        case OPT_LEVEL_BASIC: return "basic";
        case OPT_LEVEL_STANDARD: return "standard";
        case OPT_LEVEL_AGGRESSIVE: return "aggressive";
        default: return "unknown";
    }
}

void optimizer_print_stats(OptimizerContext* optimizer) {
    if (!optimizer) return;
    
    printf("Optimization Statistics:\n");
    printf("  Level: %s\n", optimizer_get_level_name(optimizer->level));
    printf("  Passes run: %d\n", optimizer->passes_run);
    printf("  Optimizations applied: %d\n", optimizer->optimizations_applied);
    printf("  Original complexity: %zu\n", optimizer->original_size);
    printf("  Optimized complexity: %zu\n", optimizer->optimized_size);
    
    if (optimizer->original_size > 0) {
        double reduction = (double)(optimizer->original_size - optimizer->optimized_size) / optimizer->original_size * 100.0;
        printf("  Complexity reduction: %.1f%%\n", reduction);
    }
    
    printf("  Errors: %d\n", optimizer->error_count);
}

bool optimizer_has_error(OptimizerContext* optimizer) {
    return optimizer && optimizer->has_error;
}

const char* optimizer_get_error(OptimizerContext* optimizer) {
    return optimizer ? optimizer->error_message : "Invalid optimizer context";
}

void optimizer_reset_stats(OptimizerContext* optimizer) {
    if (!optimizer) return;
    
    optimizer->passes_run = 0;
    optimizer->optimizations_applied = 0;
    optimizer->original_size = 0;
    optimizer->optimized_size = 0;
    optimizer->error_count = 0;
    optimizer->has_error = false;
}

// ===============================================
// Configuration Functions
// ===============================================

void optimizer_enable_pass(OptimizerContext* optimizer, OptimizationPassType pass) {
    if (optimizer && pass < OPT_PASS_COUNT) {
        optimizer->enabled_passes[pass] = true;
    }
}

void optimizer_disable_pass(OptimizerContext* optimizer, OptimizationPassType pass) {
    if (optimizer && pass < OPT_PASS_COUNT) {
        optimizer->enabled_passes[pass] = false;
    }
}

void optimizer_set_options(OptimizerContext* optimizer, bool preserve_debug, 
                          bool aggressive_inline, int max_inline_size, int max_unroll) {
    if (!optimizer) return;
    
    optimizer->preserve_debug_info = preserve_debug;
    optimizer->aggressive_inlining = aggressive_inline;
    optimizer->max_inline_size = max_inline_size;
    optimizer->max_unroll_count = max_unroll;
}
