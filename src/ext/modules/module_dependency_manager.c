/**
 * module_dependency_manager.c - Module Dependency Management System
 * 
 * Comprehensive dependency management for .native modules including
 * version checking, compatibility verification, and dependency resolution.
 */

#include "../include/native_format.h"
#include "../include/logger.h"
#include "../include/dynamic_module_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

// Dependency manager state
static struct {
    ModuleDependencyInfo modules[256];
    int module_count;
    bool initialized;
    
    // Configuration
    bool strict_version_checking;
    bool allow_prerelease;
    bool auto_resolve_dependencies;
    bool check_platform_compatibility;
    
    // Statistics
    uint64_t dependency_checks;
    uint64_t version_conflicts;
    uint64_t compatibility_failures;
} g_dep_manager = {0};

// Initialize dependency manager
int module_dependency_manager_init(void) {
    if (g_dep_manager.initialized) {
        return 0;
    }
    
    memset(&g_dep_manager, 0, sizeof(g_dep_manager));
    
    // Default configuration
    g_dep_manager.strict_version_checking = true;
    g_dep_manager.allow_prerelease = false;
    g_dep_manager.auto_resolve_dependencies = true;
    g_dep_manager.check_platform_compatibility = true;
    
    g_dep_manager.initialized = true;
    
    LOG_MODULE_INFO("Module dependency manager initialized");
    return 0;
}

// Cleanup dependency manager
void module_dependency_manager_cleanup(void) {
    if (!g_dep_manager.initialized) {
        return;
    }
    
    LOG_MODULE_INFO("Dependency manager statistics:");
    LOG_MODULE_INFO("  Dependency checks: %llu", g_dep_manager.dependency_checks);
    LOG_MODULE_INFO("  Version conflicts: %llu", g_dep_manager.version_conflicts);
    LOG_MODULE_INFO("  Compatibility failures: %llu", g_dep_manager.compatibility_failures);
    
    g_dep_manager.initialized = false;
}

// Parse version string
int parse_version_string(const char* version_str, ModuleVersion* version) {
    if (!version_str || !version) {
        return -1;
    }
    
    memset(version, 0, sizeof(ModuleVersion));
    
    // Parse major.minor.patch format
    int parsed = sscanf(version_str, "%d.%d.%d", &version->major, &version->minor, &version->patch);
    if (parsed < 1) {
        return -1;
    }
    
    // Look for prerelease and build metadata
    const char* dash = strchr(version_str, '-');
    if (dash) {
        const char* plus = strchr(dash, '+');
        if (plus) {
            // Extract prerelease
            size_t prerelease_len = plus - dash - 1;
            if (prerelease_len < sizeof(version->prerelease)) {
                strncpy(version->prerelease, dash + 1, prerelease_len);
                version->prerelease[prerelease_len] = '\0';
            }
            
            // Extract build metadata
            strncpy(version->build, plus + 1, sizeof(version->build) - 1);
        } else {
            // Only prerelease
            strncpy(version->prerelease, dash + 1, sizeof(version->prerelease) - 1);
        }
    } else {
        const char* plus = strchr(version_str, '+');
        if (plus) {
            // Only build metadata
            strncpy(version->build, plus + 1, sizeof(version->build) - 1);
        }
    }
    
    return 0;
}

// Compare two versions
VersionComparison compare_versions(const ModuleVersion* v1, const ModuleVersion* v2) {
    if (!v1 || !v2) {
        return VERSION_INCOMPATIBLE;
    }
    
    // Compare major version
    if (v1->major != v2->major) {
        return (v1->major > v2->major) ? VERSION_NEWER : VERSION_OLDER;
    }
    
    // Compare minor version
    if (v1->minor != v2->minor) {
        return (v1->minor > v2->minor) ? VERSION_NEWER : VERSION_OLDER;
    }
    
    // Compare patch version
    if (v1->patch != v2->patch) {
        return (v1->patch > v2->patch) ? VERSION_NEWER : VERSION_OLDER;
    }
    
    // Compare prerelease (if any)
    bool v1_has_pre = (strlen(v1->prerelease) > 0);
    bool v2_has_pre = (strlen(v2->prerelease) > 0);
    
    if (v1_has_pre && !v2_has_pre) {
        return VERSION_OLDER; // Prerelease is older than release
    } else if (!v1_has_pre && v2_has_pre) {
        return VERSION_NEWER; // Release is newer than prerelease
    } else if (v1_has_pre && v2_has_pre) {
        int cmp = strcmp(v1->prerelease, v2->prerelease);
        if (cmp != 0) {
            return (cmp > 0) ? VERSION_NEWER : VERSION_OLDER;
        }
    }
    
    return VERSION_EQUAL;
}

// Check if version satisfies constraint
bool version_satisfies_constraint(const ModuleVersion* version, const VersionConstraint* constraint) {
    if (!version || !constraint) {
        return false;
    }
    
    // Check prerelease policy
    if (strlen(version->prerelease) > 0 && !constraint->include_prerelease && !g_dep_manager.allow_prerelease) {
        return false;
    }
    
    switch (constraint->type) {
        case VERSION_EXACT:
            return compare_versions(version, &constraint->min_version) == VERSION_EQUAL;
            
        case VERSION_MIN:
            return compare_versions(version, &constraint->min_version) >= VERSION_EQUAL;
            
        case VERSION_MAX:
            return compare_versions(version, &constraint->max_version) <= VERSION_EQUAL;
            
        case VERSION_RANGE:
            return (compare_versions(version, &constraint->min_version) >= VERSION_EQUAL) &&
                   (compare_versions(version, &constraint->max_version) <= VERSION_EQUAL);
            
        case VERSION_COMPATIBLE:
            // Same major version, minor and patch can be newer
            return (version->major == constraint->min_version.major) &&
                   (compare_versions(version, &constraint->min_version) >= VERSION_EQUAL);
            
        default:
            return false;
    }
}

// Register module dependency information
int register_module_dependency_info(const char* module_name, const char* version_str, 
                                   const DependencySpec* dependencies, int dependency_count) {
    if (!module_name || !version_str) {
        return -1;
    }
    
    if (g_dep_manager.module_count >= 256) {
        LOG_MODULE_ERROR("Maximum number of modules reached");
        return -1;
    }
    
    ModuleDependencyInfo* info = &g_dep_manager.modules[g_dep_manager.module_count];
    memset(info, 0, sizeof(ModuleDependencyInfo));
    
    strncpy(info->module_name, module_name, sizeof(info->module_name) - 1);
    
    if (parse_version_string(version_str, &info->version) != 0) {
        LOG_MODULE_ERROR("Invalid version string: %s", version_str);
        return -1;
    }
    
    // Copy dependencies
    if (dependencies && dependency_count > 0) {
        int count = (dependency_count < 32) ? dependency_count : 32;
        memcpy(info->dependencies, dependencies, count * sizeof(DependencySpec));
        info->dependency_count = count;
    }
    
    g_dep_manager.module_count++;
    
    LOG_MODULE_DEBUG("Registered dependency info for module: %s v%s", module_name, version_str);
    return 0;
}

// Check module dependencies
int check_module_dependencies(const char* module_name) {
    if (!module_name) {
        return -1;
    }
    
    g_dep_manager.dependency_checks++;
    
    // Find module dependency info
    ModuleDependencyInfo* module_info = NULL;
    for (int i = 0; i < g_dep_manager.module_count; i++) {
        if (strcmp(g_dep_manager.modules[i].module_name, module_name) == 0) {
            module_info = &g_dep_manager.modules[i];
            break;
        }
    }
    
    if (!module_info) {
        LOG_MODULE_WARN("No dependency information for module: %s", module_name);
        return 0; // Allow loading without dependency info
    }
    
    LOG_MODULE_DEBUG("Checking dependencies for module: %s", module_name);
    
    // Check each dependency
    for (int i = 0; i < module_info->dependency_count; i++) {
        DependencySpec* dep = &module_info->dependencies[i];
        
        // Check if dependency is loaded
        bool dep_loaded = dynamic_module_is_loaded(dep->module_name);
        
        switch (dep->type) {
            case DEP_REQUIRED:
                if (!dep_loaded) {
                    if (g_dep_manager.auto_resolve_dependencies) {
                        LOG_MODULE_INFO("Auto-loading required dependency: %s", dep->module_name);
                        if (dynamic_module_load(dep->module_name) != 0) {
                            LOG_MODULE_ERROR("Failed to load required dependency: %s", dep->module_name);
                            return -1;
                        }
                    } else {
                        LOG_MODULE_ERROR("Required dependency not loaded: %s", dep->module_name);
                        return -1;
                    }
                }
                break;
                
            case DEP_CONFLICTING:
                if (dep_loaded) {
                    LOG_MODULE_ERROR("Conflicting module is loaded: %s", dep->module_name);
                    return -1;
                }
                break;
                
            case DEP_OPTIONAL:
            case DEP_SUGGESTED:
                // These don't block loading
                if (!dep_loaded) {
                    LOG_MODULE_DEBUG("Optional dependency not loaded: %s", dep->module_name);
                }
                break;
        }
        
        // Check version constraints if dependency is loaded
        if (dep_loaded && (dep->type == DEP_REQUIRED || dep->type == DEP_OPTIONAL)) {
            if (check_dependency_version(dep->module_name, &dep->version_constraint) != 0) {
                if (dep->type == DEP_REQUIRED) {
                    LOG_MODULE_ERROR("Version constraint failed for required dependency: %s", dep->module_name);
                    g_dep_manager.version_conflicts++;
                    return -1;
                } else {
                    LOG_MODULE_WARN("Version constraint failed for optional dependency: %s", dep->module_name);
                }
            }
        }
        
        // Check platform compatibility
        if (g_dep_manager.check_platform_compatibility && dep->is_platform_specific) {
            const ASTCPlatformInfo* platform_info = astc_get_platform_info();
            if (dep->required_platform != ASTC_PLATFORM_TYPE_ANY && 
                dep->required_platform != platform_info->platform) {
                LOG_MODULE_ERROR("Platform incompatible dependency: %s requires %d, current is %d",
                               dep->module_name, dep->required_platform, platform_info->platform);
                g_dep_manager.compatibility_failures++;
                return -1;
            }
            
            if (dep->required_architecture != ASTC_ARCH_TYPE_ANY &&
                dep->required_architecture != platform_info->architecture) {
                LOG_MODULE_ERROR("Architecture incompatible dependency: %s requires %d, current is %d",
                               dep->module_name, dep->required_architecture, platform_info->architecture);
                g_dep_manager.compatibility_failures++;
                return -1;
            }
        }
    }
    
    LOG_MODULE_DEBUG("All dependencies satisfied for module: %s", module_name);
    return 0;
}

// Check dependency version constraint
int check_dependency_version(const char* module_name, const VersionConstraint* constraint) {
    if (!module_name || !constraint) {
        return -1;
    }
    
    // Find module dependency info
    ModuleDependencyInfo* module_info = NULL;
    for (int i = 0; i < g_dep_manager.module_count; i++) {
        if (strcmp(g_dep_manager.modules[i].module_name, module_name) == 0) {
            module_info = &g_dep_manager.modules[i];
            break;
        }
    }
    
    if (!module_info) {
        LOG_MODULE_WARN("No version information for module: %s", module_name);
        return g_dep_manager.strict_version_checking ? -1 : 0;
    }
    
    if (!version_satisfies_constraint(&module_info->version, constraint)) {
        LOG_MODULE_ERROR("Version constraint not satisfied for module: %s", module_name);
        return -1;
    }
    
    return 0;
}

// Resolve dependency load order
int resolve_dependency_load_order(const char* modules[], int module_count, char* load_order[], int* order_count) {
    if (!modules || !load_order || !order_count || module_count <= 0) {
        return -1;
    }
    
    // Simple topological sort implementation
    // TODO: Implement proper topological sort with cycle detection
    
    *order_count = 0;
    
    // For now, just copy the modules in order
    for (int i = 0; i < module_count; i++) {
        load_order[*order_count] = malloc(strlen(modules[i]) + 1);
        if (load_order[*order_count]) {
            strcpy(load_order[*order_count], modules[i]);
            (*order_count)++;
        }
    }
    
    LOG_MODULE_DEBUG("Resolved load order for %d modules", *order_count);
    return 0;
}

// Check for circular dependencies
bool has_circular_dependencies(const char* module_name) {
    // TODO: Implement circular dependency detection
    // This would require building a dependency graph and checking for cycles
    
    LOG_MODULE_DEBUG("Checking circular dependencies for: %s", module_name);
    return false; // Placeholder
}

// Get module dependency information
int get_module_dependency_info(const char* module_name, ModuleDependencyInfo* info) {
    if (!module_name || !info) {
        return -1;
    }
    
    for (int i = 0; i < g_dep_manager.module_count; i++) {
        if (strcmp(g_dep_manager.modules[i].module_name, module_name) == 0) {
            *info = g_dep_manager.modules[i];
            return 0;
        }
    }
    
    return -1; // Not found
}

// List all registered modules
void list_registered_modules(void) {
    LOG_MODULE_INFO("Registered modules (%d):", g_dep_manager.module_count);
    for (int i = 0; i < g_dep_manager.module_count; i++) {
        ModuleDependencyInfo* info = &g_dep_manager.modules[i];
        LOG_MODULE_INFO("  %s v%d.%d.%d (%d dependencies)",
                       info->module_name, info->version.major, info->version.minor, info->version.patch,
                       info->dependency_count);
    }
}

// Configure dependency manager
void configure_dependency_manager(bool strict_version_checking, bool allow_prerelease, 
                                 bool auto_resolve_dependencies, bool check_platform_compatibility) {
    g_dep_manager.strict_version_checking = strict_version_checking;
    g_dep_manager.allow_prerelease = allow_prerelease;
    g_dep_manager.auto_resolve_dependencies = auto_resolve_dependencies;
    g_dep_manager.check_platform_compatibility = check_platform_compatibility;
    
    LOG_MODULE_INFO("Dependency manager configured: strict=%s, prerelease=%s, auto_resolve=%s, platform_check=%s",
                   strict_version_checking ? "yes" : "no",
                   allow_prerelease ? "yes" : "no",
                   auto_resolve_dependencies ? "yes" : "no",
                   check_platform_compatibility ? "yes" : "no");
}

// Get dependency manager statistics
void get_dependency_manager_stats(uint64_t* dependency_checks, uint64_t* version_conflicts, uint64_t* compatibility_failures) {
    if (dependency_checks) *dependency_checks = g_dep_manager.dependency_checks;
    if (version_conflicts) *version_conflicts = g_dep_manager.version_conflicts;
    if (compatibility_failures) *compatibility_failures = g_dep_manager.compatibility_failures;
}
