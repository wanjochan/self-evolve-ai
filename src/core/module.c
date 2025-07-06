/**
 * module.c - Core Module System Implementation
 * 
 * Implementation of the minimalistic module system that serves as the foundation
 * for all functionality in the core system.
 */

#include "module.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Maximum number of modules that can be registered
#define MAX_MODULES 64
// Maximum number of dependencies per module
#define MAX_DEPENDENCIES 16

// Symbol cache entry
typedef struct SymbolCacheEntry {
    const char* symbol;
    void* address;
    struct SymbolCacheEntry* next;
} SymbolCacheEntry;

// Module dependency information
typedef struct {
    const char** names;  // Array of dependency module names
    Module** modules;    // Array of resolved dependency modules
    size_t count;        // Number of dependencies
} ModuleDependencies;

// Module registry
static struct {
    Module* modules[MAX_MODULES];
    size_t count;
    bool initialized;
    SymbolCacheEntry* symbol_cache[256]; // Simple hash table for symbol cache
    ModuleDependencies dependencies[MAX_MODULES]; // Dependencies for each module
} module_registry = {
    .modules = {NULL},
    .count = 0,
    .initialized = false,
    .symbol_cache = {NULL},
    .dependencies = {{NULL}}
};

// Forward declarations
static Module* find_module(const char* name);
static int resolve_dependencies(Module* module);
static unsigned char symbol_hash(const char* symbol);
static void* find_cached_symbol(const char* symbol);
static void cache_symbol(const char* symbol, void* address);
static void clear_symbol_cache(void);
static int register_dependency(size_t module_index, const char* dep_name);

/**
 * Initialize the module system
 */
int module_init(void) {
    if (module_registry.initialized) {
        return 0;  // Already initialized
    }
    
    // Clear symbol cache
    memset(module_registry.symbol_cache, 0, sizeof(module_registry.symbol_cache));
    
    // Initialize dependencies
    for (size_t i = 0; i < MAX_MODULES; i++) {
        module_registry.dependencies[i].names = NULL;
        module_registry.dependencies[i].modules = NULL;
        module_registry.dependencies[i].count = 0;
    }
    
    module_registry.initialized = true;
    return 0;
}

/**
 * Clean up the module system
 */
void module_cleanup(void) {
    if (!module_registry.initialized) {
        return;  // Not initialized
    }
    
    // Unload all modules in reverse order
    for (int i = (int)module_registry.count - 1; i >= 0; i--) {
        Module* module = module_registry.modules[i];
        if (module && module_is_loaded(module)) {
            module_unload(module);
        }
    }
    
    // Clear symbol cache
    clear_symbol_cache();
    
    // Free dependency information
    for (size_t i = 0; i < MAX_MODULES; i++) {
        if (module_registry.dependencies[i].names) {
            free(module_registry.dependencies[i].names);
            module_registry.dependencies[i].names = NULL;
        }
        if (module_registry.dependencies[i].modules) {
            free(module_registry.dependencies[i].modules);
            module_registry.dependencies[i].modules = NULL;
        }
        module_registry.dependencies[i].count = 0;
    }
    
    module_registry.initialized = false;
}

/**
 * Register a module with the module system
 */
int module_register(Module* module) {
    if (!module || !module->name) {
        return -1;
    }
    
    // Check if module is already registered
    if (find_module(module->name)) {
        return 0;  // Already registered
    }
    
    // Add to registry
    if (module_registry.count >= MAX_MODULES) {
        return -1;  // Registry full
    }
    
    size_t module_index = module_registry.count;
    module_registry.modules[module_index] = module;
    module_registry.count++;
    module->state = MODULE_UNLOADED;
    
    return 0;
}

/**
 * Register a dependency for a module
 */
int module_register_dependency(Module* module, const char* dependency) {
    if (!module || !dependency) {
        return -1;
    }
    
    // Find module index
    size_t module_index = MAX_MODULES;
    for (size_t i = 0; i < module_registry.count; i++) {
        if (module_registry.modules[i] == module) {
            module_index = i;
            break;
        }
    }
    
    if (module_index == MAX_MODULES) {
        return -1;  // Module not found
    }
    
    return register_dependency(module_index, dependency);
}

/**
 * Register multiple dependencies for a module
 */
int module_register_dependencies(Module* module, const char** dependencies) {
    if (!module || !dependencies) {
        return -1;
    }
    
    // Find module index
    size_t module_index = MAX_MODULES;
    for (size_t i = 0; i < module_registry.count; i++) {
        if (module_registry.modules[i] == module) {
            module_index = i;
            break;
        }
    }
    
    if (module_index == MAX_MODULES) {
        return -1;  // Module not found
    }
    
    // Register each dependency
    for (size_t i = 0; dependencies[i]; i++) {
        if (register_dependency(module_index, dependencies[i]) != 0) {
            return -1;
        }
    }
    
    return 0;
}

/**
 * Load a module by name
 */
Module* module_load(const char* name) {
    if (!name) {
        return NULL;
    }
    
    // Initialize module system if needed
    if (!module_registry.initialized) {
        if (module_init() != 0) {
            return NULL;
        }
    }
    
    // Find module
    Module* module = find_module(name);
    if (!module) {
        return NULL;
    }
    
    // Check if already loaded
    if (module_is_loaded(module)) {
        return module;
    }
    
    // Set state to loading
    module->state = MODULE_LOADING;
    
    // Resolve dependencies
    if (resolve_dependencies(module) != 0) {
        module->state = MODULE_ERROR;
        module->error = "Failed to resolve dependencies";
        return NULL;
    }
    
    // Load module
    if (module->load && module->load() != 0) {
        module->state = MODULE_ERROR;
        module->error = "Failed to load module";
        return NULL;
    }
    
    // Call init callback
    if (module->on_init) {
        module->on_init();
    }
    
    // Set state to ready
    module->state = MODULE_READY;
    
    return module;
}

/**
 * Unload a module
 */
void module_unload(Module* module) {
    if (!module || !module_is_loaded(module)) {
        return;
    }
    
    // Call exit callback
    if (module->on_exit) {
        module->on_exit();
    }
    
    // Unload module
    if (module->unload) {
        module->unload();
    }
    
    // Set state to unloaded
    module->state = MODULE_UNLOADED;
}

/**
 * Resolve a symbol from a module
 */
void* module_resolve(Module* module, const char* symbol) {
    if (!module || !symbol || !module_is_loaded(module)) {
        return NULL;
    }
    
    // Check symbol cache first
    void* cached = find_cached_symbol(symbol);
    if (cached) {
        return cached;
    }
    
    // Use module's resolve function
    if (module->resolve) {
        void* address = module->resolve(symbol);
        if (address) {
            // Cache the result
            cache_symbol(symbol, address);
            return address;
        }
    }
    
    return NULL;
}

/**
 * Resolve a symbol from any loaded module
 */
void* module_resolve_global(const char* symbol) {
    if (!symbol || !module_registry.initialized) {
        return NULL;
    }
    
    // Check symbol cache first
    void* cached = find_cached_symbol(symbol);
    if (cached) {
        return cached;
    }
    
    // Try to resolve from each loaded module
    for (size_t i = 0; i < module_registry.count; i++) {
        Module* module = module_registry.modules[i];
        if (module && module_is_loaded(module) && module->resolve) {
            void* address = module->resolve(symbol);
            if (address) {
                // Cache the result
                cache_symbol(symbol, address);
                return address;
            }
        }
    }
    
    return NULL;
}

/**
 * Get the last error from a module
 */
const char* module_get_error(const Module* module) {
    return module ? module->error : "Invalid module";
}

/**
 * Get the state of a module
 */
ModuleState module_get_state(const Module* module) {
    return module ? module->state : MODULE_ERROR;
}

/**
 * Check if a module is loaded
 */
bool module_is_loaded(const Module* module) {
    return module && module->state == MODULE_READY;
}

/**
 * Find a module by name
 */
static Module* find_module(const char* name) {
    for (size_t i = 0; i < module_registry.count; i++) {
        if (module_registry.modules[i] && 
            strcmp(module_registry.modules[i]->name, name) == 0) {
            return module_registry.modules[i];
        }
    }
    return NULL;
}

/**
 * Resolve dependencies for a module
 */
static int resolve_dependencies(Module* module) {
    if (!module) {
        return -1;
    }
    
    // Find module index
    size_t module_index = MAX_MODULES;
    for (size_t i = 0; i < module_registry.count; i++) {
        if (module_registry.modules[i] == module) {
            module_index = i;
            break;
        }
    }
    
    if (module_index == MAX_MODULES) {
        return -1;  // Module not found
    }
    
    // No dependencies
    if (module_registry.dependencies[module_index].count == 0) {
        return 0;
    }
    
    // Load each dependency
    for (size_t i = 0; i < module_registry.dependencies[module_index].count; i++) {
        const char* dep_name = module_registry.dependencies[module_index].names[i];
        Module* dep_module = module_load(dep_name);
        
        if (!dep_module || !module_is_loaded(dep_module)) {
            return -1;  // Failed to load dependency
        }
        
        module_registry.dependencies[module_index].modules[i] = dep_module;
    }
    
    return 0;
}

/**
 * Get a module by name (public API)
 */
Module* module_get(const char* name) {
    return find_module(name);
}

/**
 * Simple hash function for symbols
 */
static unsigned char symbol_hash(const char* symbol) {
    unsigned char hash = 0;
    while (*symbol) {
        hash = (hash * 31) + *symbol++;
    }
    return hash;
}

/**
 * Find a cached symbol
 */
static void* find_cached_symbol(const char* symbol) {
    unsigned char hash = symbol_hash(symbol);
    SymbolCacheEntry* entry = module_registry.symbol_cache[hash];
    
    while (entry) {
        if (strcmp(entry->symbol, symbol) == 0) {
            return entry->address;
        }
        entry = entry->next;
    }
    
    return NULL;
}

/**
 * Cache a resolved symbol
 */
static void cache_symbol(const char* symbol, void* address) {
    unsigned char hash = symbol_hash(symbol);
    
    // Create new cache entry
    SymbolCacheEntry* entry = malloc(sizeof(SymbolCacheEntry));
    if (!entry) {
        return;
    }
    
    // Duplicate symbol name
    char* symbol_copy = strdup(symbol);
    if (!symbol_copy) {
        free(entry);
        return;
    }
    
    // Initialize entry
    entry->symbol = symbol_copy;
    entry->address = address;
    
    // Add to cache
    entry->next = module_registry.symbol_cache[hash];
    module_registry.symbol_cache[hash] = entry;
}

/**
 * Clear the symbol cache
 */
static void clear_symbol_cache(void) {
    for (int i = 0; i < 256; i++) {
        SymbolCacheEntry* entry = module_registry.symbol_cache[i];
        while (entry) {
            SymbolCacheEntry* next = entry->next;
            free((void*)entry->symbol);
            free(entry);
            entry = next;
        }
        module_registry.symbol_cache[i] = NULL;
    }
}

/**
 * Register a dependency for a module
 */
static int register_dependency(size_t module_index, const char* dep_name) {
    if (module_index >= MAX_MODULES || !dep_name) {
        return -1;
    }
    
    // Check if dependency is already registered
    for (size_t i = 0; i < module_registry.dependencies[module_index].count; i++) {
        if (strcmp(module_registry.dependencies[module_index].names[i], dep_name) == 0) {
            return 0;  // Already registered
        }
    }
    
    // Check if we have room for more dependencies
    if (module_registry.dependencies[module_index].count >= MAX_DEPENDENCIES) {
        return -1;  // Too many dependencies
    }
    
    // Allocate arrays if needed
    if (!module_registry.dependencies[module_index].names) {
        module_registry.dependencies[module_index].names = malloc(MAX_DEPENDENCIES * sizeof(const char*));
        if (!module_registry.dependencies[module_index].names) {
            return -1;  // Out of memory
        }
    }
    
    if (!module_registry.dependencies[module_index].modules) {
        module_registry.dependencies[module_index].modules = malloc(MAX_DEPENDENCIES * sizeof(Module*));
        if (!module_registry.dependencies[module_index].modules) {
            free(module_registry.dependencies[module_index].names);
            module_registry.dependencies[module_index].names = NULL;
            return -1;  // Out of memory
        }
    }
    
    // Duplicate dependency name
    char* dep_name_copy = strdup(dep_name);
    if (!dep_name_copy) {
        return -1;  // Out of memory
    }
    
    // Add dependency
    size_t index = module_registry.dependencies[module_index].count;
    module_registry.dependencies[module_index].names[index] = dep_name_copy;
    module_registry.dependencies[module_index].modules[index] = NULL;
    module_registry.dependencies[module_index].count++;
    
    return 0;
}

/**
 * Get module dependencies
 */
const char** module_get_dependencies(const Module* module) {
    if (!module) {
        return NULL;
    }
    
    // Find module index
    size_t module_index = MAX_MODULES;
    for (size_t i = 0; i < module_registry.count; i++) {
        if (module_registry.modules[i] == module) {
            module_index = i;
            break;
        }
    }
    
    if (module_index == MAX_MODULES) {
        return NULL;  // Module not found
    }
    
    return (const char**)module_registry.dependencies[module_index].names;
} 