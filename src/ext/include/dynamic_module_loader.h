/**
 * dynamic_module_loader.h - Dynamic Module Loading System
 * 
 * Header for complete dynamic loading mechanism for .native modules
 */

#ifndef DYNAMIC_MODULE_LOADER_H
#define DYNAMIC_MODULE_LOADER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Module load states
typedef enum {
    MODULE_STATE_UNLOADED = 0,
    MODULE_STATE_LOADING = 1,
    MODULE_STATE_LOADED = 2,
    MODULE_STATE_INITIALIZING = 3,
    MODULE_STATE_READY = 4,
    MODULE_STATE_ERROR = 5,
    MODULE_STATE_UNLOADING = 6
} ModuleLoadState;

// Module information structure
typedef struct {
    char module_name[128];
    char module_path[256];
    char version[32];
    ModuleLoadState state;
    int reference_count;
    time_t load_time;
    time_t last_access_time;
    uint64_t call_count;
    bool hot_swappable;
    char last_error[256];
    
    // Dependencies
    char dependencies[16][128];
    int dependency_count;
} ModuleInfo;

// Loader configuration
typedef struct {
    bool enable_hot_swap;
    bool enable_lazy_loading;
    bool enable_dependency_checking;
    int max_reference_count;
    char search_paths[16][256];
    int search_path_count;
} LoaderConfig;

// Loader statistics
typedef struct {
    uint64_t total_loads;
    uint64_t total_unloads;
    uint64_t failed_loads;
    int current_module_count;
    int max_modules_loaded;
    double average_load_time;
} LoaderStats;

// Core loader functions

/**
 * Initialize dynamic module loader
 * @return 0 on success, -1 on error
 */
int dynamic_module_loader_init(void);

/**
 * Cleanup dynamic module loader
 */
void dynamic_module_loader_cleanup(void);

/**
 * Configure module loader
 * @param config Loader configuration
 * @return 0 on success, -1 on error
 */
int dynamic_module_loader_configure(const LoaderConfig* config);

/**
 * Load native module
 * @param module_name Name of module to load
 * @return 0 on success, -1 on error
 */
int dynamic_module_load(const char* module_name);

/**
 * Load module from specific path
 * @param module_name Name to assign to module
 * @param module_path Path to module file
 * @return 0 on success, -1 on error
 */
int dynamic_module_load_from_path(const char* module_name, const char* module_path);

/**
 * Unload native module
 * @param module_name Name of module to unload
 * @return 0 on success, -1 on error
 */
int dynamic_module_unload(const char* module_name);

/**
 * Get function from loaded module
 * @param module_name Name of module
 * @param function_name Name of function
 * @return Function pointer, NULL if not found
 */
void* dynamic_module_get_function(const char* module_name, const char* function_name);

/**
 * Check if module is loaded
 * @param module_name Name of module
 * @return true if loaded, false otherwise
 */
bool dynamic_module_is_loaded(const char* module_name);

/**
 * Get module state
 * @param module_name Name of module
 * @return Module state, MODULE_STATE_UNLOADED if not found
 */
ModuleLoadState dynamic_module_get_state(const char* module_name);

// Hot-swap functionality

/**
 * Hot-swap module with new version
 * @param module_name Name of module to swap
 * @param new_module_path Path to new module version
 * @return 0 on success, -1 on error
 */
int dynamic_module_hot_swap(const char* module_name, const char* new_module_path);

/**
 * Enable/disable hot-swap for module
 * @param module_name Name of module
 * @param enable true to enable, false to disable
 * @return 0 on success, -1 on error
 */
int dynamic_module_set_hot_swappable(const char* module_name, bool enable);

/**
 * Create backup of module for rollback
 * @param module_name Name of module
 * @return 0 on success, -1 on error
 */
int dynamic_module_create_backup(const char* module_name);

/**
 * Rollback module to backup version
 * @param module_name Name of module
 * @return 0 on success, -1 on error
 */
int dynamic_module_rollback(const char* module_name);

// Dependency management

/**
 * Add module dependency
 * @param module_name Name of module
 * @param dependency_name Name of dependency
 * @return 0 on success, -1 on error
 */
int dynamic_module_add_dependency(const char* module_name, const char* dependency_name);

/**
 * Remove module dependency
 * @param module_name Name of module
 * @param dependency_name Name of dependency
 * @return 0 on success, -1 on error
 */
int dynamic_module_remove_dependency(const char* module_name, const char* dependency_name);

/**
 * Get module dependencies
 * @param module_name Name of module
 * @param dependencies Array to store dependency names
 * @param max_dependencies Maximum number of dependencies to return
 * @return Number of dependencies found, -1 on error
 */
int dynamic_module_get_dependencies(const char* module_name, char dependencies[][128], int max_dependencies);

/**
 * Check dependency chain for cycles
 * @param module_name Name of module to check
 * @return true if cycle detected, false otherwise
 */
bool dynamic_module_has_dependency_cycle(const char* module_name);

// Module discovery and search

/**
 * Add module search path
 * @param path Path to add
 * @return 0 on success, -1 on error
 */
int dynamic_module_add_search_path(const char* path);

/**
 * Remove module search path
 * @param path Path to remove
 * @return 0 on success, -1 on error
 */
int dynamic_module_remove_search_path(const char* path);

/**
 * Search for available modules
 * @param pattern Search pattern (e.g., "*.native")
 * @param results Array to store found module names
 * @param max_results Maximum number of results
 * @return Number of modules found, -1 on error
 */
int dynamic_module_search(const char* pattern, char results[][128], int max_results);

/**
 * Resolve module path
 * @param module_name Name of module
 * @param resolved_path Buffer to store resolved path
 * @param path_size Size of path buffer
 * @return 0 on success, -1 on error
 */
int dynamic_module_resolve_path(const char* module_name, char* resolved_path, size_t path_size);

// Information and statistics

/**
 * Get module information
 * @param module_name Name of module
 * @param info Pointer to store module information
 * @return 0 on success, -1 on error
 */
int dynamic_module_get_info(const char* module_name, ModuleInfo* info);

/**
 * List all loaded modules
 */
void dynamic_module_list_loaded(void);

/**
 * Get loader statistics
 * @param total_loads Pointer to store total loads
 * @param total_unloads Pointer to store total unloads
 * @param failed_loads Pointer to store failed loads
 * @param current_count Pointer to store current module count
 */
void dynamic_module_get_stats(uint64_t* total_loads, uint64_t* total_unloads, uint64_t* failed_loads, int* current_count);

/**
 * Get detailed loader statistics
 * @param stats Pointer to store detailed statistics
 */
void dynamic_module_get_detailed_stats(LoaderStats* stats);

/**
 * Reset loader statistics
 */
void dynamic_module_reset_stats(void);

// Advanced features

/**
 * Set module load timeout
 * @param timeout_seconds Timeout in seconds
 */
void dynamic_module_set_load_timeout(int timeout_seconds);

/**
 * Enable/disable lazy loading
 * @param enable true to enable, false to disable
 */
void dynamic_module_set_lazy_loading(bool enable);

/**
 * Force garbage collection of unused modules
 * @return Number of modules unloaded
 */
int dynamic_module_garbage_collect(void);

/**
 * Preload module without initializing
 * @param module_name Name of module to preload
 * @return 0 on success, -1 on error
 */
int dynamic_module_preload(const char* module_name);

/**
 * Validate module before loading
 * @param module_path Path to module file
 * @return true if valid, false otherwise
 */
bool dynamic_module_validate(const char* module_path);

/**
 * Get module load order for dependencies
 * @param modules Array of module names
 * @param module_count Number of modules
 * @param load_order Array to store load order
 * @return 0 on success, -1 on error (e.g., circular dependency)
 */
int dynamic_module_get_load_order(const char* modules[], int module_count, int* load_order);

// Error handling

/**
 * Get last loader error
 * @return Error message string
 */
const char* dynamic_module_get_last_error(void);

/**
 * Clear loader error state
 */
void dynamic_module_clear_error(void);

/**
 * Set error callback
 * @param callback Function to call on errors
 */
void dynamic_module_set_error_callback(void (*callback)(const char* module_name, const char* error));

// Debugging and monitoring

/**
 * Enable/disable debug logging
 * @param enable true to enable, false to disable
 */
void dynamic_module_set_debug_logging(bool enable);

/**
 * Dump loader state to file
 * @param filename File to write state to
 * @return 0 on success, -1 on error
 */
int dynamic_module_dump_state(const char* filename);

/**
 * Monitor module for changes
 * @param module_name Name of module to monitor
 * @param callback Function to call when module changes
 * @return 0 on success, -1 on error
 */
int dynamic_module_monitor(const char* module_name, void (*callback)(const char* module_name));

/**
 * Stop monitoring module
 * @param module_name Name of module to stop monitoring
 * @return 0 on success, -1 on error
 */
int dynamic_module_stop_monitoring(const char* module_name);

// Error codes
#define MODULE_LOADER_SUCCESS           0
#define MODULE_LOADER_ERROR_INVALID     -1
#define MODULE_LOADER_ERROR_NOT_FOUND   -2
#define MODULE_LOADER_ERROR_ALREADY_LOADED -3
#define MODULE_LOADER_ERROR_DEPENDENCY  -4
#define MODULE_LOADER_ERROR_MEMORY      -5
#define MODULE_LOADER_ERROR_TIMEOUT     -6
#define MODULE_LOADER_ERROR_VALIDATION  -7

#ifdef __cplusplus
}
#endif

#endif // DYNAMIC_MODULE_LOADER_H
