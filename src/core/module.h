/**
 * module.h - Core Module System
 * 
 * Minimalistic module system that serves as the foundation
 * for all functionality in the core system.
 */

#ifndef MODULE_H
#define MODULE_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Module states
typedef enum {
    MODULE_UNLOADED,
    MODULE_LOADING,
    MODULE_READY,
    MODULE_ERROR
} ModuleState;

// Module interface
typedef struct Module {
    const char* name;           // Module name
    void* handle;              // Module handle (implementation specific)
    ModuleState state;         // Current state
    const char* error;         // Last error message
    
    // Core functions
    int (*load)(void);         // Load and initialize the module
    void (*unload)(void);      // Unload and cleanup the module
    void* (*resolve)(const char* symbol);  // Resolve a symbol
    
    // Optional functions
    void (*on_init)(void);     // Called after successful load
    void (*on_exit)(void);     // Called before unload
    void (*on_error)(const char* msg);  // Called on error
} Module;

// Module registration
#define REGISTER_MODULE(name) \
    __attribute__((constructor)) \
    static void _register_##name(void) { \
        module_register(&module_##name); \
    }

// Module dependency declaration
#define MODULE_DEPENDS_ON(name) \
    static const char* _module_deps[] = { \
        #name, \
        NULL \
    }; \
    __attribute__((constructor)) \
    static void _register_deps_##name(void) { \
        module_register_dependencies(&module_##name, _module_deps); \
    }

// Core API
int module_init(void);  // Initialize module system
void module_cleanup(void);  // Cleanup module system

Module* module_load(const char* name);  // Load a module
void module_unload(Module* module);  // Unload a module
void* module_resolve(Module* module, const char* symbol);  // Resolve a symbol from a specific module
void* module_resolve_global(const char* symbol);  // Resolve a symbol from any loaded module
int module_register(Module* module);  // Register a module
Module* module_get(const char* name);  // Get a module by name

// Dependency management
int module_register_dependency(Module* module, const char* dependency);  // Register a single dependency
int module_register_dependencies(Module* module, const char** dependencies);  // Register multiple dependencies
const char** module_get_dependencies(const Module* module);  // Get module dependencies

// Utility functions
const char* module_get_error(const Module* module);  // Get last error
ModuleState module_get_state(const Module* module);  // Get module state
bool module_is_loaded(const Module* module);  // Check if module is loaded

#ifdef __cplusplus
}
#endif

#endif // MODULE_H 