/**
 * c99_optimizer.h - C99 Code Optimizer
 * 
 * Code optimization passes for C99 compiler including constant folding,
 * dead code elimination, loop optimization, and other standard optimizations.
 */

#ifndef C99_OPTIMIZER_H
#define C99_OPTIMIZER_H

#include "c99_codegen.h"
#include "../../core/astc.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// Optimization Levels
// ===============================================

typedef enum {
    OPT_LEVEL_NONE = 0,         // No optimization (-O0)
    OPT_LEVEL_BASIC = 1,        // Basic optimization (-O1)
    OPT_LEVEL_STANDARD = 2,     // Standard optimization (-O2)
    OPT_LEVEL_AGGRESSIVE = 3    // Aggressive optimization (-O3)
} OptimizationLevel;

// ===============================================
// Optimization Pass Types
// ===============================================

typedef enum {
    OPT_PASS_CONSTANT_FOLDING,      // Constant folding
    OPT_PASS_DEAD_CODE_ELIMINATION, // Dead code elimination
    OPT_PASS_COMMON_SUBEXPRESSION,  // Common subexpression elimination
    OPT_PASS_LOOP_INVARIANT,        // Loop invariant code motion
    OPT_PASS_LOOP_UNROLLING,        // Loop unrolling
    OPT_PASS_INLINE_EXPANSION,      // Function inlining
    OPT_PASS_TAIL_CALL,             // Tail call optimization
    OPT_PASS_PEEPHOLE,              // Peephole optimization
    OPT_PASS_REGISTER_ALLOCATION,   // Register allocation
    OPT_PASS_INSTRUCTION_SELECTION, // Instruction selection
    OPT_PASS_COUNT
} OptimizationPassType;

// ===============================================
// Optimization Context
// ===============================================

typedef struct {
    OptimizationLevel level;        // Optimization level
    bool enabled_passes[OPT_PASS_COUNT]; // Enabled optimization passes
    
    // Statistics
    int passes_run;                 // Number of passes run
    int optimizations_applied;      // Number of optimizations applied
    size_t original_size;           // Original code size
    size_t optimized_size;          // Optimized code size
    
    // Options
    bool preserve_debug_info;       // Preserve debug information
    bool aggressive_inlining;       // Aggressive function inlining
    int max_inline_size;            // Maximum function size for inlining
    int max_unroll_count;           // Maximum loop unroll count
    
    // Error handling
    char error_message[512];
    bool has_error;
    int error_count;
} OptimizerContext;

// ===============================================
// Optimization Pass Structure
// ===============================================

typedef struct OptimizationPass {
    OptimizationPassType type;      // Pass type
    const char* name;               // Pass name
    const char* description;        // Pass description
    bool (*run)(OptimizerContext* ctx, struct ASTNode* ast); // Pass function
    bool requires_ssa;              // Requires SSA form
    bool modifies_cfg;              // Modifies control flow graph
} OptimizationPass;

// ===============================================
// Optimizer Functions
// ===============================================

/**
 * Create optimizer context
 */
OptimizerContext* optimizer_create(OptimizationLevel level);

/**
 * Destroy optimizer context
 */
void optimizer_destroy(OptimizerContext* optimizer);

/**
 * Optimize AST
 */
bool optimizer_optimize_ast(OptimizerContext* optimizer, struct ASTNode* ast);

/**
 * Optimize bytecode
 */
bool optimizer_optimize_bytecode(OptimizerContext* optimizer, uint8_t* bytecode, size_t size);

/**
 * Run specific optimization pass
 */
bool optimizer_run_pass(OptimizerContext* optimizer, OptimizationPassType pass, struct ASTNode* ast);

/**
 * Enable optimization pass
 */
void optimizer_enable_pass(OptimizerContext* optimizer, OptimizationPassType pass);

/**
 * Disable optimization pass
 */
void optimizer_disable_pass(OptimizerContext* optimizer, OptimizationPassType pass);

// ===============================================
// Optimization Passes
// ===============================================

/**
 * Constant folding pass
 */
bool opt_pass_constant_folding(OptimizerContext* optimizer, struct ASTNode* ast);

/**
 * Dead code elimination pass
 */
bool opt_pass_dead_code_elimination(OptimizerContext* optimizer, struct ASTNode* ast);

/**
 * Common subexpression elimination pass
 */
bool opt_pass_common_subexpression(OptimizerContext* optimizer, struct ASTNode* ast);

/**
 * Loop optimization pass
 */
bool opt_pass_loop_optimization(OptimizerContext* optimizer, struct ASTNode* ast);

/**
 * Function inlining pass
 */
bool opt_pass_inline_expansion(OptimizerContext* optimizer, struct ASTNode* ast);

/**
 * Peephole optimization pass
 */
bool opt_pass_peephole(OptimizerContext* optimizer, uint8_t* bytecode, size_t size);

// ===============================================
// Analysis Functions
// ===============================================

/**
 * Analyze code complexity
 */
int optimizer_analyze_complexity(struct ASTNode* ast);

/**
 * Estimate optimization benefit
 */
double optimizer_estimate_benefit(OptimizerContext* optimizer, OptimizationPassType pass, struct ASTNode* ast);

/**
 * Check if optimization is safe
 */
bool optimizer_is_safe(OptimizerContext* optimizer, OptimizationPassType pass, struct ASTNode* ast);

// ===============================================
// Utility Functions
// ===============================================

/**
 * Get optimization pass name
 */
const char* optimizer_get_pass_name(OptimizationPassType pass);

/**
 * Get optimization level name
 */
const char* optimizer_get_level_name(OptimizationLevel level);

/**
 * Print optimization statistics
 */
void optimizer_print_stats(OptimizerContext* optimizer);

/**
 * Check if optimizer has error
 */
bool optimizer_has_error(OptimizerContext* optimizer);

/**
 * Get error message
 */
const char* optimizer_get_error(OptimizerContext* optimizer);

/**
 * Reset optimizer statistics
 */
void optimizer_reset_stats(OptimizerContext* optimizer);

// ===============================================
// Configuration Functions
// ===============================================

/**
 * Set optimization options
 */
void optimizer_set_options(OptimizerContext* optimizer, bool preserve_debug, 
                          bool aggressive_inline, int max_inline_size, int max_unroll);

/**
 * Load optimization configuration
 */
bool optimizer_load_config(OptimizerContext* optimizer, const char* config_file);

/**
 * Save optimization configuration
 */
bool optimizer_save_config(OptimizerContext* optimizer, const char* config_file);

#ifdef __cplusplus
}
#endif

#endif // C99_OPTIMIZER_H
