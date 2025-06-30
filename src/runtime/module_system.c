/**
 * module_system.c - 程序级模块导入和使用系统实现
 */

#include "module_system.h"
#include "libc_rt_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ===============================================
// 模块系统管理
// ===============================================

ModuleSystem* module_system_init(void) {
    ModuleSystem* system = calloc(1, sizeof(ModuleSystem));
    if (!system) return NULL;
    
    // 初始化配置
    system->auto_resolve_dependencies = true;
    system->lazy_loading = false;
    system->debug_mode = false;
    
    // 添加默认搜索路径
    strcpy(system->search_paths[0], ".");
    strcpy(system->search_paths[1], "bin/");
    strcpy(system->search_paths[2], "lib/");
    strcpy(system->search_paths[3], "modules/");
    system->search_path_count = 4;
    
    printf("Module system initialized\n");
    return system;
}

void module_system_free(ModuleSystem* system) {
    if (!system) return;
    
    // 卸载所有模块
    for (uint32_t i = 0; i < system->module_count; i++) {
        if (system->modules[i]) {
            module_free(system->modules[i]);
        }
    }
    
    free(system);
}

// ===============================================
// 模块操作
// ===============================================

Module* module_create(const char* name, ModuleType type) {
    Module* module = calloc(1, sizeof(Module));
    if (!module) return NULL;
    
    static uint32_t next_id = 1;
    module->id = next_id++;
    
    strncpy(module->name, name, MAX_MODULE_NAME_LEN - 1);
    module->type = type;
    module->state = MODULE_STATE_UNLOADED;
    
    // 分配导入导出数组
    module->exports = calloc(MAX_EXPORTS, sizeof(ModuleExport));
    module->imports = calloc(MAX_IMPORTS, sizeof(ModuleImport));
    
    if (!module->exports || !module->imports) {
        module_free(module);
        return NULL;
    }
    
    module->load_time = (uint64_t)time(NULL);
    
    return module;
}

void module_free(Module* module) {
    if (!module) return;
    
    if (module->exports) free(module->exports);
    if (module->imports) free(module->imports);
    if (module->dependencies) free(module->dependencies);
    if (module->module_data) free(module->module_data);
    
    free(module);
}

int module_add_export(Module* module, const char* name, uint32_t function_id, 
                     void* function_ptr, uint32_t param_count, uint32_t return_type) {
    if (!module || !name || module->export_count >= MAX_EXPORTS) {
        return -1;
    }
    
    ModuleExport* export = &module->exports[module->export_count];
    strncpy(export->name, name, MAX_MODULE_NAME_LEN - 1);
    export->function_id = function_id;
    export->function_ptr = function_ptr;
    export->param_count = param_count;
    export->return_type = return_type;
    export->is_variadic = (param_count == UINT32_MAX);
    
    module->export_count++;
    return 0;
}

int module_add_import(Module* module, const char* module_name, const char* function_name, 
                     uint32_t local_id) {
    if (!module || !module_name || !function_name || module->import_count >= MAX_IMPORTS) {
        return -1;
    }
    
    ModuleImport* import = &module->imports[module->import_count];
    strncpy(import->module_name, module_name, MAX_MODULE_NAME_LEN - 1);
    strncpy(import->function_name, function_name, MAX_MODULE_NAME_LEN - 1);
    import->local_id = local_id;
    import->is_resolved = false;
    
    module->import_count++;
    return 0;
}

ModuleExport* module_find_export(Module* module, const char* name) {
    if (!module || !name) return NULL;
    
    for (uint32_t i = 0; i < module->export_count; i++) {
        if (strcmp(module->exports[i].name, name) == 0) {
            return &module->exports[i];
        }
    }
    
    return NULL;
}

// ===============================================
// 模块系统操作
// ===============================================

Module* module_system_load_module(ModuleSystem* system, const char* name, const char* path) {
    if (!system || !name) return NULL;
    
    // 检查是否已加载
    Module* existing = module_system_find_module(system, name);
    if (existing) {
        existing->reference_count++;
        return existing;
    }
    
    // 检查模块数量限制
    if (system->module_count >= MAX_MODULES) {
        printf("Error: Maximum module count reached\n");
        return NULL;
    }
    
    // 创建新模块
    Module* module = module_create(name, MODULE_TYPE_USER);
    if (!module) return NULL;
    
    if (path) {
        strncpy(module->path, path, MAX_MODULE_PATH_LEN - 1);
    }
    
    module->state = MODULE_STATE_LOADING;
    
    // 添加到系统
    system->modules[system->module_count] = module;
    system->module_count++;
    system->total_loads++;
    
    module->state = MODULE_STATE_LOADED;
    module->reference_count = 1;
    
    if (system->debug_mode) {
        printf("Loaded module: %s (ID: %u)\n", name, module->id);
    }
    
    // 调用回调
    if (system->on_module_loaded) {
        system->on_module_loaded(module);
    }
    
    return module;
}

Module* module_system_find_module(ModuleSystem* system, const char* name) {
    if (!system || !name) return NULL;
    
    for (uint32_t i = 0; i < system->module_count; i++) {
        if (system->modules[i] && strcmp(system->modules[i]->name, name) == 0) {
            return system->modules[i];
        }
    }
    
    return NULL;
}

int module_system_resolve_imports(ModuleSystem* system, Module* module) {
    if (!system || !module) return -1;
    
    int resolved_count = 0;
    
    for (uint32_t i = 0; i < module->import_count; i++) {
        ModuleImport* import = &module->imports[i];
        if (import->is_resolved) continue;
        
        // 查找提供模块
        Module* provider = module_system_find_module(system, import->module_name);
        if (!provider) {
            if (system->debug_mode) {
                printf("Warning: Module '%s' not found for import '%s'\n", 
                       import->module_name, import->function_name);
            }
            continue;
        }
        
        // 查找导出函数
        ModuleExport* export = module_find_export(provider, import->function_name);
        if (!export) {
            if (system->debug_mode) {
                printf("Warning: Function '%s' not found in module '%s'\n", 
                       import->function_name, import->module_name);
            }
            continue;
        }
        
        // 解析导入
        import->resolved_ptr = export->function_ptr;
        import->is_resolved = true;
        resolved_count++;
        
        if (system->on_import_resolved) {
            system->on_import_resolved(import);
        }
    }
    
    if (system->debug_mode) {
        printf("Resolved %d/%u imports for module '%s'\n", 
               resolved_count, module->import_count, module->name);
    }
    
    return resolved_count;
}

// ===============================================
// 标准模块加载
// ===============================================

Module* module_load_libc_rt(ModuleSystem* system) {
    if (!system) return NULL;
    
    // 检查是否已加载
    if (system->libc_module) {
        system->libc_module->reference_count++;
        return system->libc_module;
    }
    
    // 创建libc.rt模块
    Module* module = module_create("libc.rt", MODULE_TYPE_SYSTEM);
    if (!module) return NULL;
    
    // 构建标准libc.rt模块
    LibcRtModule* libc_rt = libc_rt_build_standard_module();
    if (!libc_rt) {
        module_free(module);
        return NULL;
    }
    
    module->libc_rt_module = libc_rt;
    module->state = MODULE_STATE_LOADED;
    
    // 添加导出函数
    for (uint32_t i = 0; i < libc_rt->header.function_count; i++) {
        LibcFunctionSymbol* symbol = &libc_rt->symbols[i];
        void* func_ptr = libc_rt_module_get_function_by_id(libc_rt, symbol->function_id);
        
        module_add_export(module, symbol->name, symbol->function_id, 
                         func_ptr, symbol->param_count, symbol->return_type);
    }
    
    // 添加到系统
    system->modules[system->module_count] = module;
    system->module_count++;
    system->libc_module = module;
    system->total_loads++;
    
    module->reference_count = 1;
    module->is_resident = true;
    
    printf("Loaded system module: libc.rt (%u functions)\n", module->export_count);
    return module;
}

// ===============================================
// 程序级API
// ===============================================

int program_import_module(ModuleSystem* system, const char* module_name) {
    if (!system || !module_name) return -1;
    
    // 特殊处理系统模块
    if (strcmp(module_name, "libc.rt") == 0) {
        Module* module = module_load_libc_rt(system);
        return module ? 0 : -1;
    }
    
    // 尝试加载用户模块
    Module* module = module_system_load_module(system, module_name, NULL);
    return module ? 0 : -1;
}

void* program_get_module_function(ModuleSystem* system,
                                 const char* module_name,
                                 const char* function_name) {
    if (!system || !module_name || !function_name) return NULL;
    
    Module* module = module_system_find_module(system, module_name);
    if (!module) return NULL;
    
    ModuleExport* export = module_find_export(module, function_name);
    return export ? export->function_ptr : NULL;
}

// ===============================================
// 调试和诊断
// ===============================================

void module_system_print_status(ModuleSystem* system) {
    if (!system) return;
    
    printf("=== Module System Status ===\n");
    printf("Loaded modules: %u/%u\n", system->module_count, MAX_MODULES);
    printf("Total loads: %u\n", system->total_loads);
    printf("Failed loads: %u\n", system->failed_loads);
    printf("Auto resolve dependencies: %s\n", system->auto_resolve_dependencies ? "Yes" : "No");
    printf("Lazy loading: %s\n", system->lazy_loading ? "Yes" : "No");
    printf("Debug mode: %s\n", system->debug_mode ? "Yes" : "No");
    
    printf("\nLoaded modules:\n");
    for (uint32_t i = 0; i < system->module_count; i++) {
        Module* module = system->modules[i];
        if (module) {
            printf("  %u: %s (Type: %d, State: %d, Refs: %u)\n", 
                   module->id, module->name, module->type, module->state, module->reference_count);
            printf("      Exports: %u, Imports: %u\n", 
                   module->export_count, module->import_count);
        }
    }
}

void module_print_info(Module* module) {
    if (!module) return;
    
    printf("=== Module Information ===\n");
    printf("ID: %u\n", module->id);
    printf("Name: %s\n", module->name);
    printf("Type: %d\n", module->type);
    printf("State: %d\n", module->state);
    printf("Path: %s\n", module->path);
    printf("Reference count: %u\n", module->reference_count);
    printf("Exports: %u\n", module->export_count);
    printf("Imports: %u\n", module->import_count);
    printf("Is resident: %s\n", module->is_resident ? "Yes" : "No");
    
    if (module->export_count > 0) {
        printf("\nExported functions:\n");
        for (uint32_t i = 0; i < module->export_count; i++) {
            ModuleExport* export = &module->exports[i];
            printf("  %s (ID: 0x%04X, Params: %u)\n", 
                   export->name, export->function_id, export->param_count);
        }
    }
    
    if (module->import_count > 0) {
        printf("\nImported functions:\n");
        for (uint32_t i = 0; i < module->import_count; i++) {
            ModuleImport* import = &module->imports[i];
            printf("  %s::%s (Resolved: %s)\n", 
                   import->module_name, import->function_name, 
                   import->is_resolved ? "Yes" : "No");
        }
    }
}
