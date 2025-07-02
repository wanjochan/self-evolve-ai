/**
 * module_loader.c - Dynamic Module Loading Infrastructure (Layer 2)
 * 
 * Implements dynamic loading system for .native modules.
 * Supports cross-platform loading of vm_{arch}_{bits}.native and libc_{arch}_{bits}.native.
 * Follows PRD.md Layer 2 specification.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif

#include "../core/include/native_format.h"

// ===============================================
// Module Loading Infrastructure
// ===============================================

typedef struct LoadedModule {
    char module_path[512];
    void* handle;                    // HMODULE (Windows) or void* (Unix)
    NativeModule* native_module;     // Parsed .native module
    bool is_dynamic_library;        // True if loaded as DLL/SO, false if .native
    
    // Function pointers (cached)
    void* main_function;
    void* get_interface_function;
    
    // Module info
    char name[128];
    char version[32];
    char arch[16];
    int bits;
    
    struct LoadedModule* next;       // Linked list
} LoadedModule;

// Global module registry
static LoadedModule* loaded_modules = NULL;
static bool module_loader_initialized = false;

// ===============================================
// Module Loader Functions
// ===============================================

int module_loader_init(void) {
    if (module_loader_initialized) {
        return 0;
    }
    
    printf("Module Loader: Initializing dynamic loading infrastructure\n");
    
    loaded_modules = NULL;
    module_loader_initialized = true;
    
    return 0;
}

void module_loader_cleanup(void) {
    if (!module_loader_initialized) {
        return;
    }
    
    printf("Module Loader: Cleaning up loaded modules\n");
    
    // Unload all modules
    LoadedModule* current = loaded_modules;
    while (current) {
        LoadedModule* next = current->next;
        unload_native_module(current);
        current = next;
    }
    
    loaded_modules = NULL;
    module_loader_initialized = false;
}

// ===============================================
// Cross-Platform Dynamic Loading
// ===============================================

void* load_dynamic_library(const char* path) {
#ifdef _WIN32
    HMODULE handle = LoadLibraryA(path);
    if (!handle) {
        DWORD error = GetLastError();
        fprintf(stderr, "Module Loader Error: Failed to load %s (Error: %lu)\n", path, error);
    }
    return handle;
#else
    void* handle = dlopen(path, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Module Loader Error: Failed to load %s (%s)\n", path, dlerror());
    }
    return handle;
#endif
}

void unload_dynamic_library(void* handle) {
    if (!handle) return;
    
#ifdef _WIN32
    FreeLibrary((HMODULE)handle);
#else
    dlclose(handle);
#endif
}

void* get_symbol_address(void* handle, const char* symbol_name) {
    if (!handle || !symbol_name) return NULL;
    
#ifdef _WIN32
    return GetProcAddress((HMODULE)handle, symbol_name);
#else
    return dlsym(handle, symbol_name);
#endif
}

// ===============================================
// .native Module Loading
// ===============================================

LoadedModule* load_native_module(const char* module_path) {
    if (!module_loader_initialized) {
        module_loader_init();
    }
    
    if (!module_path) {
        fprintf(stderr, "Module Loader Error: NULL module path\n");
        return NULL;
    }
    
    printf("Module Loader: Loading module: %s\n", module_path);
    
    // Check if already loaded
    LoadedModule* existing = find_loaded_module(module_path);
    if (existing) {
        printf("Module Loader: Module already loaded: %s\n", module_path);
        return existing;
    }
    
    // Allocate module structure
    LoadedModule* module = calloc(1, sizeof(LoadedModule));
    if (!module) {
        fprintf(stderr, "Module Loader Error: Memory allocation failed\n");
        return NULL;
    }
    
    strncpy(module->module_path, module_path, sizeof(module->module_path) - 1);
    
    // Try to load as .native file first
    module->native_module = native_module_load_file(module_path);
    if (module->native_module) {
        printf("Module Loader: Loaded as .native format\n");
        module->is_dynamic_library = false;
        
        // Extract module info from .native format
        if (module->native_module->metadata) {
            strncpy(module->name, module->native_module->metadata->module_name, sizeof(module->name) - 1);
            strncpy(module->version, module->native_module->metadata->version, sizeof(module->version) - 1);
        }
        
        // Get architecture info
        switch (module->native_module->header.architecture) {
            case NATIVE_ARCH_X86_64:
                strcpy(module->arch, "x64");
                module->bits = 64;
                break;
            case NATIVE_ARCH_ARM64:
                strcpy(module->arch, "arm64");
                module->bits = 64;
                break;
            case NATIVE_ARCH_X86_32:
                strcpy(module->arch, "x86");
                module->bits = 32;
                break;
            default:
                strcpy(module->arch, "unknown");
                module->bits = 0;
                break;
        }
        
        // TODO: Map .native code sections into executable memory
        // For now, we'll fall back to dynamic library loading
    }
    
    // Try to load as dynamic library (.dll/.so)
    if (!module->native_module) {
        module->handle = load_dynamic_library(module_path);
        if (!module->handle) {
            free(module);
            return NULL;
        }
        
        printf("Module Loader: Loaded as dynamic library\n");
        module->is_dynamic_library = true;
        
        // Try to get module info from exports
        typedef const char* (*get_name_func_t)(void);
        get_name_func_t get_name = (get_name_func_t)get_symbol_address(module->handle, "get_module_name");
        if (get_name) {
            strncpy(module->name, get_name(), sizeof(module->name) - 1);
        } else {
            // Extract name from filename
            const char* filename = strrchr(module_path, '/');
            if (!filename) filename = strrchr(module_path, '\\');
            if (!filename) filename = module_path;
            else filename++;
            
            strncpy(module->name, filename, sizeof(module->name) - 1);
            // Remove extension
            char* dot = strrchr(module->name, '.');
            if (dot) *dot = '\0';
        }
    }
    
    // Cache common function pointers
    if (module->is_dynamic_library) {
        module->main_function = get_symbol_address(module->handle, "vm_native_main");
        if (!module->main_function) {
            module->main_function = get_symbol_address(module->handle, "libc_native_main");
        }
        
        module->get_interface_function = get_symbol_address(module->handle, "vm_get_interface");
        if (!module->get_interface_function) {
            module->get_interface_function = get_symbol_address(module->handle, "libc_native_get_info");
        }
    }
    
    // Add to loaded modules list
    module->next = loaded_modules;
    loaded_modules = module;
    
    printf("Module Loader: Successfully loaded module: %s (%s)\n", module->name, module->arch);
    
    return module;
}

void unload_native_module(LoadedModule* module) {
    if (!module) return;
    
    printf("Module Loader: Unloading module: %s\n", module->module_path);
    
    // Remove from loaded modules list
    if (loaded_modules == module) {
        loaded_modules = module->next;
    } else {
        LoadedModule* current = loaded_modules;
        while (current && current->next != module) {
            current = current->next;
        }
        if (current) {
            current->next = module->next;
        }
    }
    
    // Cleanup module
    if (module->is_dynamic_library && module->handle) {
        unload_dynamic_library(module->handle);
    }
    
    if (module->native_module) {
        native_module_free(module->native_module);
    }
    
    free(module);
}

LoadedModule* find_loaded_module(const char* module_path) {
    LoadedModule* current = loaded_modules;
    while (current) {
        if (strcmp(current->module_path, module_path) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void* get_module_function(LoadedModule* module, const char* function_name) {
    if (!module || !function_name) return NULL;
    
    if (module->is_dynamic_library) {
        return get_symbol_address(module->handle, function_name);
    } else if (module->native_module) {
        // Search in export table
        for (uint32_t i = 0; i < module->native_module->header.export_count; i++) {
            NativeExport* export = &module->native_module->export_table->exports[i];
            if (strcmp(export->name, function_name) == 0) {
                // Return pointer to function in code section
                return (void*)(module->native_module->code_section + export->offset);
            }
        }
    }
    
    return NULL;
}

// ===============================================
// Module Search and Resolution
// ===============================================

char* resolve_module_path(const char* module_name, const char* arch, int bits) {
    static char resolved_path[512];
    
    // Try different search paths
    const char* search_paths[] = {
        "bin/layer2",
        "bin",
        ".",
        NULL
    };
    
    for (int i = 0; search_paths[i]; i++) {
        // Try .native format first
        snprintf(resolved_path, sizeof(resolved_path), "%s/%s_%s_%d.native", 
                search_paths[i], module_name, arch, bits);
        
        FILE* test = fopen(resolved_path, "rb");
        if (test) {
            fclose(test);
            return resolved_path;
        }
        
        // Try dynamic library format
#ifdef _WIN32
        snprintf(resolved_path, sizeof(resolved_path), "%s/%s_%s_%d.dll", 
                search_paths[i], module_name, arch, bits);
#else
        snprintf(resolved_path, sizeof(resolved_path), "%s/lib%s_%s_%d.so", 
                search_paths[i], module_name, arch, bits);
#endif
        
        test = fopen(resolved_path, "rb");
        if (test) {
            fclose(test);
            return resolved_path;
        }
    }
    
    return NULL;
}

LoadedModule* load_module_by_name(const char* module_name, const char* arch, int bits) {
    char* path = resolve_module_path(module_name, arch, bits);
    if (!path) {
        fprintf(stderr, "Module Loader Error: Cannot find module: %s_%s_%d\n", 
                module_name, arch, bits);
        return NULL;
    }
    
    return load_native_module(path);
}

// ===============================================
// Module Information and Statistics
// ===============================================

void print_loaded_modules(void) {
    printf("Loaded Modules:\n");
    printf("===============\n");
    
    if (!loaded_modules) {
        printf("No modules loaded.\n");
        return;
    }
    
    LoadedModule* current = loaded_modules;
    int count = 0;
    
    while (current) {
        count++;
        printf("%d. %s (%s %d-bit)\n", count, current->name, current->arch, current->bits);
        printf("   Path: %s\n", current->module_path);
        printf("   Type: %s\n", current->is_dynamic_library ? "Dynamic Library" : ".native Module");
        printf("   Version: %s\n", current->version[0] ? current->version : "Unknown");
        printf("\n");
        current = current->next;
    }
    
    printf("Total modules loaded: %d\n", count);
}

int get_loaded_module_count(void) {
    int count = 0;
    LoadedModule* current = loaded_modules;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}

// ===============================================
// Module Loader Interface
// ===============================================

typedef struct {
    int (*init)(void);
    void (*cleanup)(void);
    LoadedModule* (*load_module)(const char* path);
    void (*unload_module)(LoadedModule* module);
    LoadedModule* (*find_module)(const char* path);
    void* (*get_function)(LoadedModule* module, const char* name);
    LoadedModule* (*load_by_name)(const char* name, const char* arch, int bits);
    void (*print_modules)(void);
    int (*get_module_count)(void);
} ModuleLoaderInterface;

static ModuleLoaderInterface module_loader_interface = {
    .init = module_loader_init,
    .cleanup = module_loader_cleanup,
    .load_module = load_native_module,
    .unload_module = unload_native_module,
    .find_module = find_loaded_module,
    .get_function = get_module_function,
    .load_by_name = load_module_by_name,
    .print_modules = print_loaded_modules,
    .get_module_count = get_loaded_module_count
};

const ModuleLoaderInterface* get_module_loader_interface(void) {
    return &module_loader_interface;
}
