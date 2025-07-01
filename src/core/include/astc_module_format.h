/**
 * astc_module_format.h - Enhanced ASTC Module Format
 * 
 * Header for enhanced ASTC bytecode format with comprehensive module support
 */

#ifndef ASTC_MODULE_FORMAT_H
#define ASTC_MODULE_FORMAT_H

#include "core_astc.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ASTC Module Format Version
#define ASTC_FORMAT_VERSION_MAJOR 1
#define ASTC_FORMAT_VERSION_MINOR 0
#define ASTC_FORMAT_VERSION_PATCH 0

// Module flags
typedef enum {
    ASTC_MODULE_FLAG_NONE = 0,
    ASTC_MODULE_FLAG_DEBUG = 1,
    ASTC_MODULE_FLAG_OPTIMIZED = 2,
    ASTC_MODULE_FLAG_RELOCATABLE = 4,
    ASTC_MODULE_FLAG_POSITION_INDEPENDENT = 8
} ASTCModuleFlags;

// Import/Export types
typedef enum {
    ASTC_SYMBOL_FUNCTION = 1,
    ASTC_SYMBOL_VARIABLE = 2,
    ASTC_SYMBOL_CONSTANT = 3,
    ASTC_SYMBOL_TYPE = 4,
    ASTC_SYMBOL_MODULE = 5
} ASTCSymbolType;

// Import flags
typedef enum {
    ASTC_IMPORT_FLAG_NONE = 0,
    ASTC_IMPORT_FLAG_WEAK = 1,      // Optional import
    ASTC_IMPORT_FLAG_LAZY = 2       // Load on demand
} ASTCImportFlags;

// Export flags
typedef enum {
    ASTC_EXPORT_FLAG_NONE = 0,
    ASTC_EXPORT_FLAG_DEFAULT = 1,   // Default export
    ASTC_EXPORT_FLAG_CONST = 2      // Constant export
} ASTCExportFlags;

// Dependency flags
typedef enum {
    ASTC_DEP_FLAG_NONE = 0,
    ASTC_DEP_FLAG_OPTIONAL = 1,     // Optional dependency
    ASTC_DEP_FLAG_DEV_ONLY = 2      // Development-only dependency
} ASTCDependencyFlags;

// Module validation result
typedef struct {
    bool is_valid;
    int error_count;
    char errors[16][256];           // Up to 16 error messages
} ASTCValidationResult;

// Module dependency information
typedef struct {
    char module_name[128];
    char version_requirement[32];
    bool is_satisfied;
    bool is_optional;
} ASTCDependencyInfo;

// Module statistics
typedef struct {
    int function_count;
    int variable_count;
    int constant_count;
    int import_count;
    int export_count;
    int dependency_count;
    size_t code_size;
    size_t data_size;
    size_t total_size;
} ASTCModuleStats;

// Enhanced module serialization functions

/**
 * Serialize module with enhanced format
 * @param module Module AST node to serialize
 * @param buffer Output buffer (allocated by function)
 * @param size Output buffer size
 * @return 0 on success, -1 on error
 */
int ast_serialize_module_enhanced(ASTNode* module, uint8_t** buffer, size_t* size);

/**
 * Deserialize module from enhanced format
 * @param buffer Input buffer containing serialized module
 * @param size Buffer size
 * @return Module AST node on success, NULL on error
 */
ASTNode* ast_deserialize_module_enhanced(const uint8_t* buffer, size_t size);

/**
 * Validate module format and dependencies
 * @param module Module AST node to validate
 * @param result Validation result structure
 * @return 0 on success, -1 on error
 */
int ast_validate_module_enhanced(const ASTNode* module, ASTCValidationResult* result);

/**
 * Get module statistics
 * @param module Module AST node
 * @param stats Statistics structure to fill
 * @return 0 on success, -1 on error
 */
int ast_get_module_stats(const ASTNode* module, ASTCModuleStats* stats);

/**
 * Check if module satisfies dependency requirements
 * @param module Module to check
 * @param dependency_name Name of required dependency
 * @param version_requirement Version requirement string
 * @return true if satisfied, false otherwise
 */
bool ast_module_satisfies_dependency(const ASTNode* module, 
                                    const char* dependency_name,
                                    const char* version_requirement);

/**
 * Resolve all symbol references in module
 * @param module Module AST node
 * @param available_modules Array of available modules for resolution
 * @param module_count Number of available modules
 * @return 0 on success, -1 on error
 */
int ast_resolve_module_symbols(ASTNode* module, ASTNode** available_modules, int module_count);

/**
 * Create module dependency graph
 * @param modules Array of modules
 * @param module_count Number of modules
 * @param dependencies Output array of dependency information
 * @param max_dependencies Maximum number of dependencies to return
 * @return Number of dependencies found, -1 on error
 */
int ast_create_dependency_graph(ASTNode** modules, int module_count,
                               ASTCDependencyInfo* dependencies, int max_dependencies);

/**
 * Check for circular dependencies
 * @param modules Array of modules to check
 * @param module_count Number of modules
 * @return 0 if no circular dependencies, -1 if found
 */
int ast_check_circular_dependencies_enhanced(ASTNode** modules, int module_count);

/**
 * Write module to ASTC file
 * @param module Module AST node
 * @param filename Output filename
 * @return 0 on success, -1 on error
 */
int ast_write_module_file(const ASTNode* module, const char* filename);

/**
 * Read module from ASTC file
 * @param filename Input filename
 * @return Module AST node on success, NULL on error
 */
ASTNode* ast_read_module_file(const char* filename);

/**
 * Optimize module for size or speed
 * @param module Module AST node to optimize
 * @param optimize_for_speed true for speed, false for size
 * @return 0 on success, -1 on error
 */
int ast_optimize_module(ASTNode* module, bool optimize_for_speed);

/**
 * Generate module documentation
 * @param module Module AST node
 * @param output_buffer Output buffer for documentation
 * @param buffer_size Size of output buffer
 * @return 0 on success, -1 on error
 */
int ast_generate_module_docs(const ASTNode* module, char* output_buffer, size_t buffer_size);

// Utility functions for module manipulation

/**
 * Clone a module AST node
 * @param module Module to clone
 * @return Cloned module on success, NULL on error
 */
ASTNode* ast_clone_module(const ASTNode* module);

/**
 * Merge two modules
 * @param target Target module to merge into
 * @param source Source module to merge from
 * @return 0 on success, -1 on error
 */
int ast_merge_modules(ASTNode* target, const ASTNode* source);

/**
 * Extract submodule from module
 * @param module Source module
 * @param export_names Array of export names to extract
 * @param export_count Number of exports to extract
 * @return New module containing only specified exports, NULL on error
 */
ASTNode* ast_extract_submodule(const ASTNode* module, const char** export_names, int export_count);

// Version comparison utilities

/**
 * Compare two version strings
 * @param version1 First version string (e.g., "1.2.3")
 * @param version2 Second version string
 * @return -1 if version1 < version2, 0 if equal, 1 if version1 > version2
 */
int ast_compare_versions(const char* version1, const char* version2);

/**
 * Check if version satisfies requirement
 * @param version Version string to check
 * @param requirement Requirement string (e.g., ">=1.0.0", "~1.2.0")
 * @return true if satisfied, false otherwise
 */
bool ast_version_satisfies(const char* version, const char* requirement);

// Error handling

/**
 * Get last module operation error message
 * @return Error message string
 */
const char* ast_get_last_module_error(void);

/**
 * Clear last module operation error
 */
void ast_clear_module_error(void);

#ifdef __cplusplus
}
#endif

#endif // ASTC_MODULE_FORMAT_H
