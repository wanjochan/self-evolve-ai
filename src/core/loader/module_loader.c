/**
 * module_loader.c - Dynamic .native Module Loader
 * 
 * Implements dynamic loading and management of .native modules
 * with symbol resolution and address relocation.
 */

#include "module_loader.h"
#include "../include/native_format.h"
#include "../include/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>  // For dynamic loading on Unix-like systems
#endif

// Maximum number of loaded modules
#define MAX_LOADED_MODULES 64

// Module registry
typedef struct {
    char name[256];
    NativeModule* module;
    void* handle;  // Platform-specific handle
    bool is_loaded;
} LoadedModule;

static LoadedModule g_loaded_modules[MAX_LOADED_MODULES];
static int g_module_count = 0;

// Initialize module loader
int module_loader_init(void) {
    memset(g_loaded_modules, 0, sizeof(g_loaded_modules));
    g_module_count = 0;
    return 0;
}

// Cleanup module loader
void module_loader_cleanup(void) {
    for (int i = 0; i < g_module_count; i++) {
        if (g_loaded_modules[i].is_loaded) {
            module_loader_unload(g_loaded_modules[i].name);
        }
    }
    g_module_count = 0;
}

// Load a .native module
int module_loader_load(const char* module_name, const char* file_path) {
    if (!module_name || !file_path) {
        return -1;
    }

    // Check if already loaded
    for (int i = 0; i < g_module_count; i++) {
        if (strcmp(g_loaded_modules[i].name, module_name) == 0) {
            return 0; // Already loaded
        }
    }

    // Check capacity
    if (g_module_count >= MAX_LOADED_MODULES) {
        LOG_MODULE_ERROR("Maximum number of modules reached (%d)", MAX_LOADED_MODULES);
        SET_ERROR(ERROR_MEMORY_ALLOCATION, "Maximum number of modules reached");
        return -1;
    }

    // Load the .native module
    LOG_MODULE_INFO("Loading .native module: %s from %s", module_name, file_path);
    NativeModule* module = native_module_load_file(file_path);
    if (!module) {
        LOG_MODULE_ERROR("Failed to load .native module: %s", file_path);
        SET_ERROR(ERROR_MODULE_LOAD_FAILED, "Failed to load .native module: %s", file_path);
        return -1;
    }

    // Validate module
    if (native_module_validate(module) != NATIVE_SUCCESS) {
        LOG_MODULE_ERROR("Invalid .native module: %s", file_path);
        SET_ERROR(ERROR_MODULE_LOAD_FAILED, "Invalid .native module: %s", file_path);
        native_module_free(module);
        return -1;
    }

    // Add to registry
    LoadedModule* loaded = &g_loaded_modules[g_module_count];
    strncpy(loaded->name, module_name, sizeof(loaded->name) - 1);
    loaded->name[sizeof(loaded->name) - 1] = '\0';
    loaded->module = module;
    loaded->handle = NULL;
    loaded->is_loaded = true;

    g_module_count++;

    LOG_MODULE_INFO("Module loaded successfully: %s (%s)", module_name, file_path);
    return 0;
}

// Unload a module
int module_loader_unload(const char* module_name) {
    if (!module_name) {
        return -1;
    }

    for (int i = 0; i < g_module_count; i++) {
        if (strcmp(g_loaded_modules[i].name, module_name) == 0) {
            if (g_loaded_modules[i].is_loaded) {
                native_module_free(g_loaded_modules[i].module);
                g_loaded_modules[i].module = NULL;
                g_loaded_modules[i].is_loaded = false;
                
                // Compact array
                for (int j = i; j < g_module_count - 1; j++) {
                    g_loaded_modules[j] = g_loaded_modules[j + 1];
                }
                g_module_count--;
                
                printf("Module unloaded: %s\n", module_name);
                return 0;
            }
        }
    }

    return -1; // Not found
}

// Find a loaded module
LoadedModule* find_loaded_module(const char* module_name) {
    for (int i = 0; i < g_module_count; i++) {
        if (strcmp(g_loaded_modules[i].name, module_name) == 0 && 
            g_loaded_modules[i].is_loaded) {
            return &g_loaded_modules[i];
        }
    }
    return NULL;
}

// Resolve symbol from a specific module
void* module_loader_resolve_symbol(const char* module_name, const char* symbol_name) {
    if (!module_name || !symbol_name) {
        return NULL;
    }

    LoadedModule* loaded = find_loaded_module(module_name);
    if (!loaded) {
        return NULL;
    }

    return native_module_get_export_address(loaded->module, symbol_name);
}

// Resolve symbol from any loaded module
void* module_loader_resolve_symbol_global(const char* symbol_name) {
    if (!symbol_name) {
        return NULL;
    }

    for (int i = 0; i < g_module_count; i++) {
        if (g_loaded_modules[i].is_loaded) {
            void* addr = native_module_get_export_address(g_loaded_modules[i].module, symbol_name);
            if (addr) {
                return addr;
            }
        }
    }

    return NULL;
}

// List all loaded modules
void module_loader_list_modules(void) {
    printf("Loaded modules (%d):\n", g_module_count);
    for (int i = 0; i < g_module_count; i++) {
        if (g_loaded_modules[i].is_loaded) {
            NativeModule* mod = g_loaded_modules[i].module;
            printf("  %s: arch=%d, type=%d, exports=%d\n", 
                   g_loaded_modules[i].name,
                   mod->header.architecture,
                   mod->header.module_type,
                   mod->header.export_count);
        }
    }
}

// Get module information
int module_loader_get_info(const char* module_name, ModuleInfo* info) {
    if (!module_name || !info) {
        return -1;
    }

    LoadedModule* loaded = find_loaded_module(module_name);
    if (!loaded) {
        return -1;
    }

    NativeModule* mod = loaded->module;
    strncpy(info->name, module_name, sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0';
    info->architecture = mod->header.architecture;
    info->module_type = mod->header.module_type;
    info->code_size = mod->header.code_size;
    info->data_size = mod->header.data_size;
    info->export_count = mod->header.export_count;
    info->is_loaded = true;

    return 0;
}

// Auto-load required modules based on platform
int module_loader_auto_load_platform_modules(void) {
    // Detect platform and load appropriate modules
    // This would typically load vm_{arch}_{bits}.native and libc_{arch}_{bits}.native
    
    printf("Auto-loading platform modules...\n");
    
    // For now, try to load common modules
    const char* common_modules[] = {
        "vm_x64_64.native",
        "libc_x64_64.native"
    };
    
    int loaded_count = 0;
    for (int i = 0; i < 2; i++) {
        char path[512];
        snprintf(path, sizeof(path), "bin/%s", common_modules[i]);
        
        if (module_loader_load(common_modules[i], path) == 0) {
            loaded_count++;
        }
    }
    
    printf("Auto-loaded %d platform modules\n", loaded_count);
    return loaded_count;
}
