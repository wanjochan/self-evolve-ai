/**
 * module_loader.h - Dynamic .native Module Loader
 * 
 * Header for dynamic loading and management of .native modules
 */

#ifndef MODULE_LOADER_H
#define MODULE_LOADER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Module information structure
typedef struct {
    char name[256];
    uint32_t architecture;
    uint32_t module_type;
    uint64_t code_size;
    uint64_t data_size;
    uint32_t export_count;
    bool is_loaded;
} ModuleInfo;

// Module loader functions

/**
 * Initialize the module loader system
 * @return 0 on success, -1 on error
 */
int module_loader_init(void);

/**
 * Cleanup the module loader system
 */
void module_loader_cleanup(void);

/**
 * Load a .native module
 * @param module_name Name to register the module under
 * @param file_path Path to the .native file
 * @return 0 on success, -1 on error
 */
int module_loader_load(const char* module_name, const char* file_path);

/**
 * Unload a module
 * @param module_name Name of the module to unload
 * @return 0 on success, -1 on error
 */
int module_loader_unload(const char* module_name);

/**
 * Resolve a symbol from a specific module
 * @param module_name Name of the module
 * @param symbol_name Name of the symbol to resolve
 * @return Pointer to the symbol, or NULL if not found
 */
void* module_loader_resolve_symbol(const char* module_name, const char* symbol_name);

/**
 * Resolve a symbol from any loaded module
 * @param symbol_name Name of the symbol to resolve
 * @return Pointer to the symbol, or NULL if not found
 */
void* module_loader_resolve_symbol_global(const char* symbol_name);

/**
 * List all loaded modules (for debugging)
 */
void module_loader_list_modules(void);

/**
 * Get information about a loaded module
 * @param module_name Name of the module
 * @param info Pointer to ModuleInfo structure to fill
 * @return 0 on success, -1 on error
 */
int module_loader_get_info(const char* module_name, ModuleInfo* info);

/**
 * Auto-load platform-specific modules
 * @return Number of modules loaded
 */
int module_loader_auto_load_platform_modules(void);

// Convenience macros for common operations

#define MODULE_LOAD(name, path) module_loader_load(name, path)
#define MODULE_UNLOAD(name) module_loader_unload(name)
#define MODULE_RESOLVE(module, symbol) module_loader_resolve_symbol(module, symbol)
#define MODULE_RESOLVE_GLOBAL(symbol) module_loader_resolve_symbol_global(symbol)

// Error codes
#define MODULE_SUCCESS           0
#define MODULE_ERROR_INVALID     -1
#define MODULE_ERROR_NOT_FOUND   -2
#define MODULE_ERROR_NO_MEMORY   -3
#define MODULE_ERROR_IO          -4
#define MODULE_ERROR_MAX_MODULES -5

#ifdef __cplusplus
}
#endif

#endif // MODULE_LOADER_H
