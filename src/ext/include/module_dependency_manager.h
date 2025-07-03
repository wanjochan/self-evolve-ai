/**
 * module_dependency_manager.h - Module Dependency Management System
 * 
 * Header for comprehensive dependency management for .native modules
 */

#ifndef MODULE_DEPENDENCY_MANAGER_H
#define MODULE_DEPENDENCY_MANAGER_H

#include "astc_platform_compat.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Version comparison result
typedef enum {
    VERSION_EQUAL = 0,
    VERSION_NEWER = 1,
    VERSION_OLDER = -1,
    VERSION_INCOMPATIBLE = -2
} VersionComparison;

// Dependency requirement types
typedef enum {
    DEP_REQUIRED = 0,       // Must be present
    DEP_OPTIONAL = 1,       // Optional dependency
    DEP_CONFLICTING = 2,    // Must not be present
    DEP_SUGGESTED = 3       // Suggested but not required
} DependencyType;

// Version constraint types
typedef enum {
    VERSION_EXACT = 0,      // Exact version match
    VERSION_MIN = 1,        // Minimum version
    VERSION_MAX = 2,        // Maximum version
    VERSION_RANGE = 3,      // Version range
    VERSION_COMPATIBLE = 4  // Compatible version (same major)
} VersionConstraintType;

// Version structure
typedef struct {
    int major;
    int minor;
    int patch;
    char prerelease[32];
    char build[32];
} ModuleVersion;

// Version constraint
typedef struct {
    VersionConstraintType type;
    ModuleVersion min_version;
    ModuleVersion max_version;
    bool include_prerelease;
} VersionConstraint;

// Dependency specification
typedef struct {
    char module_name[128];
    DependencyType type;
    VersionConstraint version_constraint;
    char description[256];
    bool is_platform_specific;
    ASTCPlatformType required_platform;
    ASTCArchitectureType required_architecture;
} DependencySpec;

// Module dependency information
typedef struct {
    char module_name[128];
    ModuleVersion version;
    DependencySpec dependencies[32];
    int dependency_count;
    
    // Compatibility information
    char abi_version[32];
    char api_version[32];
    uint32_t compatibility_flags;
    
    // Platform requirements
    ASTCPlatformType supported_platforms[8];
    int supported_platform_count;
    ASTCArchitectureType supported_architectures[8];
    int supported_arch_count;
    
    // Load information
    bool is_loaded;
    time_t load_time;
    char load_path[256];
} ModuleDependencyInfo;

// Dependency manager configuration
typedef struct {
    bool strict_version_checking;
    bool allow_prerelease;
    bool auto_resolve_dependencies;
    bool check_platform_compatibility;
} DependencyManagerConfig;

// Core dependency management functions

/**
 * Initialize dependency manager
 * @return 0 on success, -1 on error
 */
int module_dependency_manager_init(void);

/**
 * Cleanup dependency manager
 */
void module_dependency_manager_cleanup(void);

/**
 * Configure dependency manager
 * @param strict_version_checking Enable strict version checking
 * @param allow_prerelease Allow prerelease versions
 * @param auto_resolve_dependencies Automatically resolve dependencies
 * @param check_platform_compatibility Check platform compatibility
 */
void configure_dependency_manager(bool strict_version_checking, bool allow_prerelease, 
                                 bool auto_resolve_dependencies, bool check_platform_compatibility);

// Module registration and information

/**
 * Register module dependency information
 * @param module_name Name of the module
 * @param version_str Version string (e.g., "1.2.3-alpha+build")
 * @param dependencies Array of dependency specifications
 * @param dependency_count Number of dependencies
 * @return 0 on success, -1 on error
 */
int register_module_dependency_info(const char* module_name, const char* version_str, 
                                   const DependencySpec* dependencies, int dependency_count);

/**
 * Get module dependency information
 * @param module_name Name of the module
 * @param info Pointer to store dependency information
 * @return 0 on success, -1 on error
 */
int get_module_dependency_info(const char* module_name, ModuleDependencyInfo* info);

/**
 * List all registered modules
 */
void list_registered_modules(void);

// Version management

/**
 * Parse version string
 * @param version_str Version string to parse
 * @param version Pointer to store parsed version
 * @return 0 on success, -1 on error
 */
int parse_version_string(const char* version_str, ModuleVersion* version);

/**
 * Compare two versions
 * @param v1 First version
 * @param v2 Second version
 * @return VersionComparison result
 */
VersionComparison compare_versions(const ModuleVersion* v1, const ModuleVersion* v2);

/**
 * Check if version satisfies constraint
 * @param version Version to check
 * @param constraint Version constraint
 * @return true if satisfied, false otherwise
 */
bool version_satisfies_constraint(const ModuleVersion* version, const VersionConstraint* constraint);

/**
 * Create version constraint
 * @param type Constraint type
 * @param min_version Minimum version (for range/min constraints)
 * @param max_version Maximum version (for range/max constraints)
 * @param include_prerelease Include prerelease versions
 * @param constraint Pointer to store created constraint
 * @return 0 on success, -1 on error
 */
int create_version_constraint(VersionConstraintType type, const ModuleVersion* min_version,
                             const ModuleVersion* max_version, bool include_prerelease,
                             VersionConstraint* constraint);

// Dependency checking and resolution

/**
 * Check module dependencies
 * @param module_name Name of module to check
 * @return 0 if all dependencies satisfied, -1 on error
 */
int check_module_dependencies(const char* module_name);

/**
 * Check dependency version constraint
 * @param module_name Name of dependency module
 * @param constraint Version constraint to check
 * @return 0 if constraint satisfied, -1 on error
 */
int check_dependency_version(const char* module_name, const VersionConstraint* constraint);

/**
 * Resolve dependency load order
 * @param modules Array of module names
 * @param module_count Number of modules
 * @param load_order Array to store load order (caller must free)
 * @param order_count Pointer to store number of modules in order
 * @return 0 on success, -1 on error
 */
int resolve_dependency_load_order(const char* modules[], int module_count, char* load_order[], int* order_count);

/**
 * Check for circular dependencies
 * @param module_name Name of module to check
 * @return true if circular dependency detected, false otherwise
 */
bool has_circular_dependencies(const char* module_name);

// Dependency specification helpers

/**
 * Create dependency specification
 * @param module_name Name of dependency module
 * @param type Dependency type
 * @param version_constraint Version constraint
 * @param description Human-readable description
 * @param spec Pointer to store created specification
 * @return 0 on success, -1 on error
 */
int create_dependency_spec(const char* module_name, DependencyType type,
                          const VersionConstraint* version_constraint, const char* description,
                          DependencySpec* spec);

/**
 * Add platform requirement to dependency
 * @param spec Dependency specification to modify
 * @param platform Required platform
 * @param architecture Required architecture
 * @return 0 on success, -1 on error
 */
int add_platform_requirement(DependencySpec* spec, ASTCPlatformType platform, ASTCArchitectureType architecture);

/**
 * Validate dependency specification
 * @param spec Dependency specification to validate
 * @return true if valid, false otherwise
 */
bool validate_dependency_spec(const DependencySpec* spec);

// Compatibility checking

/**
 * Check ABI compatibility
 * @param module1 First module name
 * @param module2 Second module name
 * @return true if ABI compatible, false otherwise
 */
bool check_abi_compatibility(const char* module1, const char* module2);

/**
 * Check API compatibility
 * @param module1 First module name
 * @param module2 Second module name
 * @return true if API compatible, false otherwise
 */
bool check_api_compatibility(const char* module1, const char* module2);

/**
 * Check platform compatibility
 * @param module_name Name of module
 * @param platform Target platform
 * @param architecture Target architecture
 * @return true if compatible, false otherwise
 */
bool check_platform_compatibility(const char* module_name, ASTCPlatformType platform, ASTCArchitectureType architecture);

// Statistics and monitoring

/**
 * Get dependency manager statistics
 * @param dependency_checks Pointer to store dependency check count
 * @param version_conflicts Pointer to store version conflict count
 * @param compatibility_failures Pointer to store compatibility failure count
 */
void get_dependency_manager_stats(uint64_t* dependency_checks, uint64_t* version_conflicts, uint64_t* compatibility_failures);

/**
 * Reset dependency manager statistics
 */
void reset_dependency_manager_stats(void);

// Utility functions

/**
 * Version to string
 * @param version Version to convert
 * @param buffer Buffer to store string
 * @param buffer_size Size of buffer
 * @return 0 on success, -1 on error
 */
int version_to_string(const ModuleVersion* version, char* buffer, size_t buffer_size);

/**
 * Dependency type to string
 * @param type Dependency type
 * @return String representation
 */
const char* dependency_type_to_string(DependencyType type);

/**
 * Version constraint type to string
 * @param type Constraint type
 * @return String representation
 */
const char* version_constraint_type_to_string(VersionConstraintType type);

/**
 * Dump dependency graph to file
 * @param filename File to write to
 * @return 0 on success, -1 on error
 */
int dump_dependency_graph(const char* filename);

/**
 * Load dependency information from file
 * @param filename File to read from
 * @return 0 on success, -1 on error
 */
int load_dependency_info_from_file(const char* filename);

/**
 * Save dependency information to file
 * @param filename File to write to
 * @return 0 on success, -1 on error
 */
int save_dependency_info_to_file(const char* filename);

// Error codes
#define DEP_MANAGER_SUCCESS           0
#define DEP_MANAGER_ERROR_INVALID     -1
#define DEP_MANAGER_ERROR_NOT_FOUND   -2
#define DEP_MANAGER_ERROR_VERSION     -3
#define DEP_MANAGER_ERROR_CIRCULAR    -4
#define DEP_MANAGER_ERROR_PLATFORM    -5
#define DEP_MANAGER_ERROR_ABI         -6

#ifdef __cplusplus
}
#endif

#endif // MODULE_DEPENDENCY_MANAGER_H
