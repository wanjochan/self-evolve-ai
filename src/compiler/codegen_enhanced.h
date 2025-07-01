/**
 * codegen_enhanced.h - Enhanced Code Generation Utility
 * 
 * Header for advanced code generation framework
 */

#ifndef CODEGEN_ENHANCED_H
#define CODEGEN_ENHANCED_H

#include "../core/include/core_astc.h"
#include "../core/include/astc_platform_compat.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Code generation optimization levels
typedef enum {
    CODEGEN_OPT_NONE = 0,       // No optimization
    CODEGEN_OPT_BASIC = 1,      // Basic optimizations
    CODEGEN_OPT_ADVANCED = 2,   // Advanced optimizations
    CODEGEN_OPT_AGGRESSIVE = 3  // Aggressive optimizations
} CodegenOptLevel;

// Code generation configuration
typedef struct {
    ASTCArchitectureType target_arch;
    ASTCPlatformType target_platform;
    CodegenOptLevel optimization_level;
    bool enable_debug;
    bool enable_profiling;
    bool enable_ai_optimization;
    bool generate_metadata;
    size_t initial_buffer_size;
} CodegenConfig;

// Symbol information
typedef struct {
    char name[128];
    uint32_t offset;
    uint32_t size;
    int symbol_type;
} CodegenSymbol;

// Relocation information
typedef struct {
    uint32_t offset;
    uint32_t target_symbol;
    int reloc_type;
} CodegenRelocation;

// Code generation statistics
typedef struct {
    uint64_t instructions_generated;
    uint64_t optimizations_applied;
    uint64_t bytes_saved;
    double code_quality_score;
    size_t final_code_size;
    int symbol_count;
    int relocation_count;
} CodegenStats;

// Enhanced code generation functions

/**
 * Initialize enhanced code generator
 * @param target_arch Target architecture
 * @param target_platform Target platform
 * @param opt_level Optimization level
 * @return 0 on success, -1 on error
 */
int codegen_enhanced_init(ASTCArchitectureType target_arch, ASTCPlatformType target_platform, int opt_level);

/**
 * Cleanup enhanced code generator
 */
void codegen_enhanced_cleanup(void);

/**
 * Configure code generator
 * @param config Configuration structure
 * @return 0 on success, -1 on error
 */
int codegen_enhanced_configure(const CodegenConfig* config);

/**
 * Emit instruction for target architecture
 * @param instruction ASTC instruction type
 * @param operands Instruction operands
 * @param operand_count Number of operands
 * @return 0 on success, -1 on error
 */
int codegen_enhanced_emit_instruction(ASTNodeType instruction, const ASTCValue* operands, int operand_count);

/**
 * Generate function prologue
 * @param function_name Name of the function
 * @return 0 on success, -1 on error
 */
int codegen_enhanced_emit_function_prologue(const char* function_name);

/**
 * Generate function epilogue
 * @return 0 on success, -1 on error
 */
int codegen_enhanced_emit_function_epilogue(void);

/**
 * Generate complete function
 * @param function_ast Function AST node
 * @return 0 on success, -1 on error
 */
int codegen_enhanced_generate_function(ASTNode* function_ast);

/**
 * Generate complete module
 * @param module_ast Module AST node
 * @return 0 on success, -1 on error
 */
int codegen_enhanced_generate_module(ASTNode* module_ast);

/**
 * Apply optimizations to generated code
 * @return 0 on success, -1 on error
 */
int codegen_enhanced_optimize(void);

/**
 * Get generated code
 * @param size Pointer to store code size
 * @return Pointer to generated code buffer
 */
const uint8_t* codegen_enhanced_get_code(size_t* size);

/**
 * Get symbol table
 * @param symbols Buffer to store symbols
 * @param max_symbols Maximum number of symbols to return
 * @return Number of symbols returned
 */
int codegen_enhanced_get_symbols(void* symbols, int max_symbols);

/**
 * Get relocation table
 * @param relocations Buffer to store relocations
 * @param max_relocations Maximum number of relocations to return
 * @return Number of relocations returned
 */
int codegen_enhanced_get_relocations(void* relocations, int max_relocations);

/**
 * Get code generation statistics
 * @param instructions Pointer to store instruction count
 * @param optimizations Pointer to store optimization count
 * @param bytes_saved Pointer to store bytes saved
 */
void codegen_enhanced_get_stats(uint64_t* instructions, uint64_t* optimizations, uint64_t* bytes_saved);

/**
 * Get detailed statistics
 * @param stats Pointer to store detailed statistics
 */
void codegen_enhanced_get_detailed_stats(CodegenStats* stats);

// Optimization functions

/**
 * Remove redundant NOP instructions
 */
void codegen_remove_redundant_nops(void);

/**
 * Apply peephole optimizations
 */
void codegen_apply_peephole_optimizations(void);

/**
 * Apply advanced optimizations
 */
void codegen_apply_advanced_optimizations(void);

/**
 * Apply AI-driven optimizations
 */
void codegen_apply_ai_optimizations(void);

/**
 * Optimize for size
 * @return Number of bytes saved
 */
size_t codegen_optimize_for_size(void);

/**
 * Optimize for speed
 * @return Number of optimizations applied
 */
int codegen_optimize_for_speed(void);

// Architecture-specific functions

/**
 * Check if instruction is supported on target architecture
 * @param instruction ASTC instruction type
 * @return true if supported, false otherwise
 */
bool codegen_is_instruction_supported(ASTNodeType instruction);

/**
 * Get instruction encoding size for target architecture
 * @param instruction ASTC instruction type
 * @return Size in bytes, -1 if unsupported
 */
int codegen_get_instruction_size(ASTNodeType instruction);

/**
 * Get optimal register allocation for target architecture
 * @param variables Array of variable names
 * @param var_count Number of variables
 * @param allocation Array to store register allocations
 * @return 0 on success, -1 on error
 */
int codegen_get_register_allocation(const char** variables, int var_count, int* allocation);

// Debugging and analysis functions

/**
 * Dump generated code to file
 * @param filename Output filename
 * @param format Output format (binary, assembly, hex)
 * @return 0 on success, -1 on error
 */
int codegen_dump_code(const char* filename, const char* format);

/**
 * Analyze code quality
 * @return Quality score (0.0 to 100.0)
 */
double codegen_analyze_code_quality(void);

/**
 * Validate generated code
 * @return true if valid, false otherwise
 */
bool codegen_validate_code(void);

/**
 * Get disassembly of generated code
 * @param buffer Buffer to store disassembly
 * @param buffer_size Size of buffer
 * @return 0 on success, -1 on error
 */
int codegen_get_disassembly(char* buffer, size_t buffer_size);

// Utility functions

/**
 * Calculate code complexity
 * @return Complexity score
 */
int codegen_calculate_complexity(void);

/**
 * Estimate execution time
 * @return Estimated cycles
 */
uint64_t codegen_estimate_execution_time(void);

/**
 * Get memory usage
 * @param code_size Pointer to store code size
 * @param data_size Pointer to store data size
 * @param stack_size Pointer to store stack size
 */
void codegen_get_memory_usage(size_t* code_size, size_t* data_size, size_t* stack_size);

/**
 * Check for potential issues
 * @param issues Buffer to store issue descriptions
 * @param max_issues Maximum number of issues to return
 * @return Number of issues found
 */
int codegen_check_issues(char issues[][256], int max_issues);

// Error codes
#define CODEGEN_SUCCESS           0
#define CODEGEN_ERROR_INVALID     -1
#define CODEGEN_ERROR_MEMORY      -2
#define CODEGEN_ERROR_UNSUPPORTED -3
#define CODEGEN_ERROR_BUFFER_FULL -4
#define CODEGEN_ERROR_SYMBOL_NOT_FOUND -5

// Symbol types
#define CODEGEN_SYMBOL_FUNCTION   1
#define CODEGEN_SYMBOL_VARIABLE   2
#define CODEGEN_SYMBOL_LABEL      3
#define CODEGEN_SYMBOL_CONSTANT   4

// Relocation types
#define CODEGEN_RELOC_ABSOLUTE    1
#define CODEGEN_RELOC_RELATIVE    2
#define CODEGEN_RELOC_FUNCTION    3
#define CODEGEN_RELOC_DATA        4

#ifdef __cplusplus
}
#endif

#endif // CODEGEN_ENHANCED_H
