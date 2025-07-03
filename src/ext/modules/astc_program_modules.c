/**
 * astc_program_modules.c - Program-Level Module System for ASTC
 * 
 * Implements program-level module import and usage system,
 * supporting system modules like libc.rt and user-defined modules.
 */

#include "include/astc_program_modules.h"
#include "include/astc_native_bridge.h"
#include "include/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Maximum number of imported modules per program
#define MAX_PROGRAM_MODULES 64

// Program module registry
typedef struct {
    char module_name[128];
    char module_path[256];
    char version[32];
    ASTCModuleType module_type;
    bool is_loaded;
    bool is_system_module;
    void* module_handle;
    ASTCModuleInterface interface;
} ProgramModule;

// Program module state
static struct {
    ProgramModule modules[MAX_PROGRAM_MODULES];
    int module_count;
    bool initialized;
    char program_name[128];
    char program_path[256];
} g_program_state = {0};

// Initialize program module system
int astc_program_modules_init(const char* program_name, const char* program_path) {
    if (g_program_state.initialized) {
        return 0;
    }

    memset(&g_program_state, 0, sizeof(g_program_state));
    
    if (program_name) {
        strncpy(g_program_state.program_name, program_name, sizeof(g_program_state.program_name) - 1);
    }
    if (program_path) {
        strncpy(g_program_state.program_path, program_path, sizeof(g_program_state.program_path) - 1);
    }
    
    g_program_state.initialized = true;

    LOG_RUNTIME_INFO("Program module system initialized for: %s", program_name ? program_name : "unnamed");
    return 0;
}

// Cleanup program module system
void astc_program_modules_cleanup(void) {
    if (!g_program_state.initialized) {
        return;
    }

    // Unload all modules
    for (int i = 0; i < g_program_state.module_count; i++) {
        if (g_program_state.modules[i].is_loaded) {
            astc_program_unload_module(g_program_state.modules[i].module_name);
        }
    }

    memset(&g_program_state, 0, sizeof(g_program_state));
    LOG_RUNTIME_INFO("Program module system cleaned up");
}

// Load a system module (like libc.rt)
static int load_system_module(const char* module_name, ProgramModule* module) {
    LOG_RUNTIME_INFO("Loading system module: %s", module_name);

    // Handle libc.rt system module
    if (strcmp(module_name, "libc.rt") == 0) {
        module->module_type = ASTC_MODULE_SYSTEM;
        module->is_system_module = true;
        strncpy(module->version, "1.0.0", sizeof(module->version) - 1);
        
        // Register standard libc functions
        module->interface.function_count = 8;
        
        // printf
        strncpy(module->interface.functions[0].name, "printf", sizeof(module->interface.functions[0].name) - 1);
        module->interface.functions[0].param_count = 1;
        module->interface.functions[0].param_types[0] = ASTC_TYPE_STRING;
        module->interface.functions[0].return_type = ASTC_TYPE_I32;
        
        // malloc
        strncpy(module->interface.functions[1].name, "malloc", sizeof(module->interface.functions[1].name) - 1);
        module->interface.functions[1].param_count = 1;
        module->interface.functions[1].param_types[0] = ASTC_TYPE_I64;
        module->interface.functions[1].return_type = ASTC_TYPE_PTR;
        
        // free
        strncpy(module->interface.functions[2].name, "free", sizeof(module->interface.functions[2].name) - 1);
        module->interface.functions[2].param_count = 1;
        module->interface.functions[2].param_types[0] = ASTC_TYPE_PTR;
        module->interface.functions[2].return_type = ASTC_TYPE_VOID;
        
        // strlen
        strncpy(module->interface.functions[3].name, "strlen", sizeof(module->interface.functions[3].name) - 1);
        module->interface.functions[3].param_count = 1;
        module->interface.functions[3].param_types[0] = ASTC_TYPE_STRING;
        module->interface.functions[3].return_type = ASTC_TYPE_I64;
        
        // strcpy
        strncpy(module->interface.functions[4].name, "strcpy", sizeof(module->interface.functions[4].name) - 1);
        module->interface.functions[4].param_count = 2;
        module->interface.functions[4].param_types[0] = ASTC_TYPE_PTR;
        module->interface.functions[4].param_types[1] = ASTC_TYPE_STRING;
        module->interface.functions[4].return_type = ASTC_TYPE_PTR;
        
        // strcmp
        strncpy(module->interface.functions[5].name, "strcmp", sizeof(module->interface.functions[5].name) - 1);
        module->interface.functions[5].param_count = 2;
        module->interface.functions[5].param_types[0] = ASTC_TYPE_STRING;
        module->interface.functions[5].param_types[1] = ASTC_TYPE_STRING;
        module->interface.functions[5].return_type = ASTC_TYPE_I32;
        
        // memcpy
        strncpy(module->interface.functions[6].name, "memcpy", sizeof(module->interface.functions[6].name) - 1);
        module->interface.functions[6].param_count = 3;
        module->interface.functions[6].param_types[0] = ASTC_TYPE_PTR;
        module->interface.functions[6].param_types[1] = ASTC_TYPE_PTR;
        module->interface.functions[6].param_types[2] = ASTC_TYPE_I64;
        module->interface.functions[6].return_type = ASTC_TYPE_PTR;
        
        // memset
        strncpy(module->interface.functions[7].name, "memset", sizeof(module->interface.functions[7].name) - 1);
        module->interface.functions[7].param_count = 3;
        module->interface.functions[7].param_types[0] = ASTC_TYPE_PTR;
        module->interface.functions[7].param_types[1] = ASTC_TYPE_I32;
        module->interface.functions[7].param_types[2] = ASTC_TYPE_I64;
        module->interface.functions[7].return_type = ASTC_TYPE_PTR;
        
        LOG_RUNTIME_INFO("System module libc.rt loaded with %d functions", module->interface.function_count);
        return 0;
    }
    
    // Handle math.rt system module
    if (strcmp(module_name, "math.rt") == 0) {
        module->module_type = ASTC_MODULE_SYSTEM;
        module->is_system_module = true;
        strncpy(module->version, "1.0.0", sizeof(module->version) - 1);
        
        module->interface.function_count = 6;
        
        // sin
        strncpy(module->interface.functions[0].name, "sin", sizeof(module->interface.functions[0].name) - 1);
        module->interface.functions[0].param_count = 1;
        module->interface.functions[0].param_types[0] = ASTC_TYPE_F64;
        module->interface.functions[0].return_type = ASTC_TYPE_F64;
        
        // cos
        strncpy(module->interface.functions[1].name, "cos", sizeof(module->interface.functions[1].name) - 1);
        module->interface.functions[1].param_count = 1;
        module->interface.functions[1].param_types[0] = ASTC_TYPE_F64;
        module->interface.functions[1].return_type = ASTC_TYPE_F64;
        
        // sqrt
        strncpy(module->interface.functions[2].name, "sqrt", sizeof(module->interface.functions[2].name) - 1);
        module->interface.functions[2].param_count = 1;
        module->interface.functions[2].param_types[0] = ASTC_TYPE_F64;
        module->interface.functions[2].return_type = ASTC_TYPE_F64;
        
        // pow
        strncpy(module->interface.functions[3].name, "pow", sizeof(module->interface.functions[3].name) - 1);
        module->interface.functions[3].param_count = 2;
        module->interface.functions[3].param_types[0] = ASTC_TYPE_F64;
        module->interface.functions[3].param_types[1] = ASTC_TYPE_F64;
        module->interface.functions[3].return_type = ASTC_TYPE_F64;
        
        // log
        strncpy(module->interface.functions[4].name, "log", sizeof(module->interface.functions[4].name) - 1);
        module->interface.functions[4].param_count = 1;
        module->interface.functions[4].param_types[0] = ASTC_TYPE_F64;
        module->interface.functions[4].return_type = ASTC_TYPE_F64;
        
        // exp
        strncpy(module->interface.functions[5].name, "exp", sizeof(module->interface.functions[5].name) - 1);
        module->interface.functions[5].param_count = 1;
        module->interface.functions[5].param_types[0] = ASTC_TYPE_F64;
        module->interface.functions[5].return_type = ASTC_TYPE_F64;
        
        LOG_RUNTIME_INFO("System module math.rt loaded with %d functions", module->interface.function_count);
        return 0;
    }
    
    LOG_RUNTIME_ERROR("Unknown system module: %s", module_name);
    return -1;
}

// Load a user module
static int load_user_module(const char* module_name, const char* module_path, ProgramModule* module) {
    LOG_RUNTIME_INFO("Loading user module: %s from %s", module_name, module_path);
    
    // TODO: Implement actual .astc module loading
    // For now, just set up basic structure
    module->module_type = ASTC_MODULE_USER;
    module->is_system_module = false;
    strncpy(module->version, "0.1.0", sizeof(module->version) - 1);
    strncpy(module->module_path, module_path, sizeof(module->module_path) - 1);
    
    // Placeholder interface
    module->interface.function_count = 0;
    
    LOG_RUNTIME_INFO("User module %s loaded (placeholder)", module_name);
    return 0;
}

// Import a module into the program
int astc_program_import_module(const char* module_name, const char* module_path, const char* version_requirement) {
    if (!module_name) {
        SET_ERROR(ERROR_INVALID_ARGUMENT, "Module name cannot be null");
        return -1;
    }

    if (g_program_state.module_count >= MAX_PROGRAM_MODULES) {
        SET_ERROR(ERROR_MEMORY_ALLOCATION, "Maximum program modules reached");
        return -1;
    }

    // Check if module is already imported
    for (int i = 0; i < g_program_state.module_count; i++) {
        if (strcmp(g_program_state.modules[i].module_name, module_name) == 0) {
            LOG_RUNTIME_WARN("Module %s already imported", module_name);
            return 0;
        }
    }

    ProgramModule* module = &g_program_state.modules[g_program_state.module_count];
    memset(module, 0, sizeof(ProgramModule));
    
    strncpy(module->module_name, module_name, sizeof(module->module_name) - 1);

    // Determine if this is a system module
    if (strstr(module_name, ".rt") != NULL) {
        // System module
        if (load_system_module(module_name, module) != 0) {
            LOG_RUNTIME_ERROR("Failed to load system module: %s", module_name);
            return -1;
        }
    } else {
        // User module
        if (!module_path) {
            SET_ERROR(ERROR_INVALID_ARGUMENT, "Module path required for user modules");
            return -1;
        }
        if (load_user_module(module_name, module_path, module) != 0) {
            LOG_RUNTIME_ERROR("Failed to load user module: %s", module_name);
            return -1;
        }
    }

    module->is_loaded = true;
    g_program_state.module_count++;

    LOG_RUNTIME_INFO("Module imported successfully: %s", module_name);
    return 0;
}

// Unload a module from the program
int astc_program_unload_module(const char* module_name) {
    if (!module_name) {
        return -1;
    }

    for (int i = 0; i < g_program_state.module_count; i++) {
        if (strcmp(g_program_state.modules[i].module_name, module_name) == 0) {
            if (g_program_state.modules[i].is_loaded) {
                // TODO: Cleanup module resources
                g_program_state.modules[i].is_loaded = false;
                
                // Compact array
                for (int j = i; j < g_program_state.module_count - 1; j++) {
                    g_program_state.modules[j] = g_program_state.modules[j + 1];
                }
                g_program_state.module_count--;
                
                LOG_RUNTIME_INFO("Module unloaded: %s", module_name);
                return 0;
            }
        }
    }

    return -1; // Not found
}

// Find a function in imported modules
const ASTCFunctionInfo* astc_program_find_function(const char* module_name, const char* function_name) {
    if (!module_name || !function_name) {
        return NULL;
    }

    for (int i = 0; i < g_program_state.module_count; i++) {
        if (g_program_state.modules[i].is_loaded &&
            strcmp(g_program_state.modules[i].module_name, module_name) == 0) {
            
            ProgramModule* module = &g_program_state.modules[i];
            for (int j = 0; j < module->interface.function_count; j++) {
                if (strcmp(module->interface.functions[j].name, function_name) == 0) {
                    return &module->interface.functions[j];
                }
            }
        }
    }

    return NULL;
}

// Call a function from an imported module
int astc_program_call_function(const char* module_name, const char* function_name, 
                              const ASTCValue* args, int arg_count, ASTCValue* result) {
    if (!module_name || !function_name || !args || !result) {
        SET_ERROR(ERROR_INVALID_ARGUMENT, "Invalid arguments to call_function");
        return -1;
    }

    const ASTCFunctionInfo* func_info = astc_program_find_function(module_name, function_name);
    if (!func_info) {
        SET_ERROR(ERROR_SYMBOL_NOT_FOUND, "Function not found: %s.%s", module_name, function_name);
        return -1;
    }

    // Validate argument count
    if (arg_count != func_info->param_count) {
        SET_ERROR(ERROR_INVALID_ARGUMENT, "Argument count mismatch for %s.%s: expected %d, got %d",
                 module_name, function_name, func_info->param_count, arg_count);
        return -1;
    }

    // For system modules, delegate to native bridge
    ProgramModule* module = NULL;
    for (int i = 0; i < g_program_state.module_count; i++) {
        if (strcmp(g_program_state.modules[i].module_name, module_name) == 0) {
            module = &g_program_state.modules[i];
            break;
        }
    }

    if (!module) {
        SET_ERROR(ERROR_SYMBOL_NOT_FOUND, "Module not found: %s", module_name);
        return -1;
    }

    if (module->is_system_module) {
        // Build interface name for native bridge
        char interface_name[256];
        snprintf(interface_name, sizeof(interface_name), "%s.%s", module_name, function_name);
        
        // Remove .rt suffix for native bridge
        char* rt_suffix = strstr(interface_name, ".rt.");
        if (rt_suffix) {
            memmove(rt_suffix, rt_suffix + 3, strlen(rt_suffix + 3) + 1);
        }
        
        return astc_native_call(interface_name, args, arg_count, result);
    } else {
        // TODO: Handle user module function calls
        LOG_RUNTIME_ERROR("User module function calls not yet implemented");
        return -1;
    }
}

// List all imported modules
void astc_program_list_modules(void) {
    LOG_RUNTIME_INFO("Imported modules (%d):", g_program_state.module_count);
    for (int i = 0; i < g_program_state.module_count; i++) {
        if (g_program_state.modules[i].is_loaded) {
            ProgramModule* module = &g_program_state.modules[i];
            LOG_RUNTIME_INFO("  %s v%s (%s, %d functions)",
                           module->module_name, module->version,
                           module->is_system_module ? "system" : "user",
                           module->interface.function_count);
        }
    }
}

// Get module information
int astc_program_get_module_info(const char* module_name, ASTCProgramModuleInfo* info) {
    if (!module_name || !info) {
        return -1;
    }

    for (int i = 0; i < g_program_state.module_count; i++) {
        if (g_program_state.modules[i].is_loaded &&
            strcmp(g_program_state.modules[i].module_name, module_name) == 0) {
            
            ProgramModule* module = &g_program_state.modules[i];
            strncpy(info->module_name, module->module_name, sizeof(info->module_name) - 1);
            strncpy(info->version, module->version, sizeof(info->version) - 1);
            strncpy(info->module_path, module->module_path, sizeof(info->module_path) - 1);
            info->module_type = module->module_type;
            info->is_system_module = module->is_system_module;
            info->function_count = module->interface.function_count;
            info->is_loaded = module->is_loaded;
            
            return 0;
        }
    }

    return -1; // Not found
}
