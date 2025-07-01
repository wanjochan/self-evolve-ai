/**
 * multi_arch_support.h - Multi-Architecture Support System
 * 
 * Header for comprehensive multi-architecture support
 */

#ifndef MULTI_ARCH_SUPPORT_H
#define MULTI_ARCH_SUPPORT_H

#include "astc_platform_compat.h"
#include "core_astc.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Architecture-specific configuration
typedef struct {
    ASTCArchitectureType arch_type;
    const char* arch_name;
    const char* arch_id;
    int pointer_size;           // 4 or 8 bytes
    int register_count;         // Number of general-purpose registers
    int vector_register_count;  // Number of vector registers
    bool has_fpu;              // Has floating-point unit
    bool has_vector_unit;      // Has vector/SIMD unit
    bool has_atomic_ops;       // Has atomic operations
    bool is_little_endian;     // Byte order
    
    // Code generation capabilities
    bool supports_jit;         // Supports JIT compilation
    bool supports_inline_asm;  // Supports inline assembly
    bool supports_hot_patching; // Supports hot code patching
    
    // Performance characteristics
    int cache_line_size;       // Cache line size in bytes
    int branch_prediction_accuracy; // 0-100 percentage
    int instruction_latency_avg;    // Average instruction latency
    
    // Architecture-specific limits
    int max_immediate_size;    // Maximum immediate value size
    int max_displacement;      // Maximum displacement for addressing
    int alignment_requirement; // Memory alignment requirement
} ArchitectureConfig;

// Architecture capabilities structure
typedef struct {
    bool has_jit;
    bool has_vector;
    bool has_atomic;
    bool has_fpu;
    bool supports_cross_compilation;
    int pointer_size;
    int register_count;
} ArchitectureCapabilities;

// Multi-architecture statistics
typedef struct {
    uint64_t arch_specific_optimizations;
    uint64_t cross_arch_translations;
    uint64_t runtime_adaptations;
    int supported_architectures;
    ASTCArchitectureType current_arch;
    ASTCArchitectureType target_arch;
} MultiArchStats;

// Core multi-architecture functions

/**
 * Initialize multi-architecture support
 * @return 0 on success, -1 on error
 */
int multi_arch_support_init(void);

/**
 * Cleanup multi-architecture support
 */
void multi_arch_support_cleanup(void);

/**
 * Initialize architecture configurations
 * @return 0 on success, -1 on error
 */
int init_architecture_configs(void);

/**
 * Detect runtime architecture
 * @return 0 on success, -1 on error
 */
int detect_runtime_architecture(void);

// Architecture configuration functions

/**
 * Get architecture configuration
 * @param arch Architecture type
 * @return Pointer to configuration, NULL if not found
 */
const ArchitectureConfig* get_architecture_config(ASTCArchitectureType arch);

/**
 * Get current architecture configuration
 * @return Pointer to current architecture configuration
 */
const ArchitectureConfig* get_current_architecture_config(void);

/**
 * Set target architecture for cross-compilation
 * @param target_arch Target architecture
 * @return 0 on success, -1 on error
 */
int set_target_architecture(ASTCArchitectureType target_arch);

/**
 * Get target architecture
 * @return Current target architecture
 */
ASTCArchitectureType get_target_architecture(void);

/**
 * Check if cross-compilation is enabled
 * @return true if enabled, false otherwise
 */
bool is_cross_compilation_enabled(void);

// Code generation functions

/**
 * Generate architecture-specific code
 * @param instruction ASTC instruction type
 * @param operands Instruction operands
 * @param operand_count Number of operands
 * @param target_arch Target architecture
 * @param code_buffer Buffer to store generated code
 * @param buffer_size Size of code buffer
 * @param code_size Pointer to store actual code size
 * @return 0 on success, -1 on error
 */
int generate_arch_specific_code(ASTNodeType instruction, const ASTCValue* operands, int operand_count,
                               ASTCArchitectureType target_arch, uint8_t* code_buffer, size_t buffer_size, size_t* code_size);

/**
 * Generate x64 code
 * @param instruction ASTC instruction type
 * @param operands Instruction operands
 * @param operand_count Number of operands
 * @param code_buffer Buffer to store generated code
 * @param buffer_size Size of code buffer
 * @param code_size Pointer to store actual code size
 * @return 0 on success, -1 on error
 */
int generate_x64_code(ASTNodeType instruction, const ASTCValue* operands, int operand_count,
                     uint8_t* code_buffer, size_t buffer_size, size_t* code_size);

/**
 * Generate ARM64 code
 * @param instruction ASTC instruction type
 * @param operands Instruction operands
 * @param operand_count Number of operands
 * @param code_buffer Buffer to store generated code
 * @param buffer_size Size of code buffer
 * @param code_size Pointer to store actual code size
 * @return 0 on success, -1 on error
 */
int generate_arm64_code(ASTNodeType instruction, const ASTCValue* operands, int operand_count,
                       uint8_t* code_buffer, size_t buffer_size, size_t* code_size);

/**
 * Generate x86 code
 * @param instruction ASTC instruction type
 * @param operands Instruction operands
 * @param operand_count Number of operands
 * @param code_buffer Buffer to store generated code
 * @param buffer_size Size of code buffer
 * @param code_size Pointer to store actual code size
 * @return 0 on success, -1 on error
 */
int generate_x86_code(ASTNodeType instruction, const ASTCValue* operands, int operand_count,
                     uint8_t* code_buffer, size_t buffer_size, size_t* code_size);

/**
 * Generate ARM32 code
 * @param instruction ASTC instruction type
 * @param operands Instruction operands
 * @param operand_count Number of operands
 * @param code_buffer Buffer to store generated code
 * @param buffer_size Size of code buffer
 * @param code_size Pointer to store actual code size
 * @return 0 on success, -1 on error
 */
int generate_arm32_code(ASTNodeType instruction, const ASTCValue* operands, int operand_count,
                       uint8_t* code_buffer, size_t buffer_size, size_t* code_size);

// Optimization functions

/**
 * Apply architecture-specific optimizations
 * @param arch Target architecture
 * @param code Code buffer to optimize
 * @param code_size Size of code
 * @return Number of optimizations applied, -1 on error
 */
int apply_arch_optimizations(ASTCArchitectureType arch, uint8_t* code, size_t code_size);

/**
 * Optimize x64 code
 * @param code Code buffer to optimize
 * @param code_size Size of code
 * @return Number of optimizations applied
 */
int optimize_x64_code(uint8_t* code, size_t code_size);

/**
 * Optimize ARM64 code
 * @param code Code buffer to optimize
 * @param code_size Size of code
 * @return Number of optimizations applied
 */
int optimize_arm64_code(uint8_t* code, size_t code_size);

/**
 * Optimize x86 code
 * @param code Code buffer to optimize
 * @param code_size Size of code
 * @return Number of optimizations applied
 */
int optimize_x86_code(uint8_t* code, size_t code_size);

/**
 * Optimize ARM32 code
 * @param code Code buffer to optimize
 * @param code_size Size of code
 * @return Number of optimizations applied
 */
int optimize_arm32_code(uint8_t* code, size_t code_size);

// Compatibility and capability functions

/**
 * Check architecture compatibility
 * @param arch1 First architecture
 * @param arch2 Second architecture
 * @return true if compatible, false otherwise
 */
bool is_architecture_compatible(ASTCArchitectureType arch1, ASTCArchitectureType arch2);

/**
 * Get architecture name
 * @param arch Architecture type
 * @return Architecture name string
 */
const char* get_architecture_name(ASTCArchitectureType arch);

/**
 * Get architecture capabilities
 * @param arch Architecture type
 * @param has_jit Pointer to store JIT capability
 * @param has_vector Pointer to store vector capability
 * @param has_atomic Pointer to store atomic capability
 */
void get_architecture_capabilities(ASTCArchitectureType arch, bool* has_jit, bool* has_vector, bool* has_atomic);

/**
 * Get detailed architecture capabilities
 * @param arch Architecture type
 * @param capabilities Pointer to store capabilities
 * @return 0 on success, -1 on error
 */
int get_detailed_architecture_capabilities(ASTCArchitectureType arch, ArchitectureCapabilities* capabilities);

/**
 * Check if architecture supports feature
 * @param arch Architecture type
 * @param feature Feature name
 * @return true if supported, false otherwise
 */
bool architecture_supports_feature(ASTCArchitectureType arch, const char* feature);

// Information and statistics

/**
 * Get multi-architecture statistics
 * @param optimizations Pointer to store optimization count
 * @param translations Pointer to store translation count
 * @param adaptations Pointer to store adaptation count
 */
void get_multi_arch_stats(uint64_t* optimizations, uint64_t* translations, uint64_t* adaptations);

/**
 * Get detailed multi-architecture statistics
 * @param stats Pointer to store detailed statistics
 */
void get_detailed_multi_arch_stats(MultiArchStats* stats);

/**
 * List supported architectures
 * @param architectures Array to store architecture types
 * @param max_architectures Maximum number of architectures to return
 * @return Number of supported architectures
 */
int list_supported_architectures(ASTCArchitectureType* architectures, int max_architectures);

/**
 * Get architecture count
 * @return Number of supported architectures
 */
int get_supported_architecture_count(void);

// Utility functions

/**
 * Convert architecture string to type
 * @param arch_string Architecture string
 * @return Architecture type, ASTC_ARCH_TYPE_UNKNOWN if not found
 */
ASTCArchitectureType string_to_architecture_type(const char* arch_string);

/**
 * Convert architecture type to string
 * @param arch Architecture type
 * @return Architecture string
 */
const char* architecture_type_to_string(ASTCArchitectureType arch);

/**
 * Get instruction size for architecture
 * @param arch Architecture type
 * @param instruction Instruction type
 * @return Instruction size in bytes, -1 if unsupported
 */
int get_instruction_size_for_arch(ASTCArchitectureType arch, ASTNodeType instruction);

/**
 * Check if instruction is supported on architecture
 * @param arch Architecture type
 * @param instruction Instruction type
 * @return true if supported, false otherwise
 */
bool is_instruction_supported_on_arch(ASTCArchitectureType arch, ASTNodeType instruction);

/**
 * Get optimal calling convention for architecture
 * @param arch Architecture type
 * @return Calling convention identifier
 */
int get_calling_convention_for_arch(ASTCArchitectureType arch);

/**
 * Get register allocation strategy for architecture
 * @param arch Architecture type
 * @return Register allocation strategy identifier
 */
int get_register_allocation_strategy(ASTCArchitectureType arch);

// Error codes
#define MULTI_ARCH_SUCCESS           0
#define MULTI_ARCH_ERROR_INVALID     -1
#define MULTI_ARCH_ERROR_UNSUPPORTED -2
#define MULTI_ARCH_ERROR_BUFFER_FULL -3
#define MULTI_ARCH_ERROR_NOT_FOUND   -4

// Feature names for architecture_supports_feature()
#define ARCH_FEATURE_JIT             "jit"
#define ARCH_FEATURE_VECTOR          "vector"
#define ARCH_FEATURE_ATOMIC          "atomic"
#define ARCH_FEATURE_FPU             "fpu"
#define ARCH_FEATURE_INLINE_ASM      "inline_asm"
#define ARCH_FEATURE_HOT_PATCHING    "hot_patching"

#ifdef __cplusplus
}
#endif

#endif // MULTI_ARCH_SUPPORT_H
