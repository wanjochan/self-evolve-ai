/**
 * module_loader.c - .rt模块动态加载器
 * 
 * 实现.rt模块的动态加载和符号解析
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "rt_format_standard.h"
#include "module_loader.h"

// 模块加载器状态
typedef struct {
    Module** loaded_modules;        // 已加载模块列表
    uint32_t module_count;          // 模块数量
    uint32_t module_capacity;       // 模块容量
    char** search_paths;            // 搜索路径
    uint32_t search_path_count;     // 搜索路径数量
    bool verbose;                   // 详细输出
} ModuleLoader;

static ModuleLoader* g_loader = NULL;

// 初始化模块加载器
int module_loader_init(bool verbose) {
    if (g_loader) {
        printf("Warning: Module loader already initialized\n");
        return 0;
    }
    
    g_loader = calloc(1, sizeof(ModuleLoader));
    if (!g_loader) {
        printf("Error: Failed to allocate module loader\n");
        return -1;
    }
    
    g_loader->verbose = verbose;
    g_loader->module_capacity = 16;
    g_loader->loaded_modules = calloc(g_loader->module_capacity, sizeof(Module*));
    
    if (!g_loader->loaded_modules) {
        free(g_loader);
        g_loader = NULL;
        return -1;
    }
    
    // 添加默认搜索路径
    module_loader_add_search_path(".");
    module_loader_add_search_path("bin");
    module_loader_add_search_path("lib");
    module_loader_add_search_path("modules");
    
    if (verbose) {
        printf("Module loader initialized with %d search paths\n", g_loader->search_path_count);
    }
    
    return 0;
}

// 清理模块加载器
void module_loader_cleanup(void) {
    if (!g_loader) return;
    
    // 卸载所有模块
    for (uint32_t i = 0; i < g_loader->module_count; i++) {
        if (g_loader->loaded_modules[i]) {
            module_unload(g_loader->loaded_modules[i]);
        }
    }
    
    // 释放搜索路径
    for (uint32_t i = 0; i < g_loader->search_path_count; i++) {
        free(g_loader->search_paths[i]);
    }
    free(g_loader->search_paths);
    
    // 释放模块列表
    free(g_loader->loaded_modules);
    
    free(g_loader);
    g_loader = NULL;
    
    printf("Module loader cleaned up\n");
}

// 添加搜索路径
int module_loader_add_search_path(const char* path) {
    if (!g_loader || !path) return -1;
    
    // 扩展搜索路径数组
    if (g_loader->search_path_count >= 16) { // 最多16个搜索路径
        printf("Warning: Maximum search paths reached\n");
        return -1;
    }
    
    if (!g_loader->search_paths) {
        g_loader->search_paths = calloc(16, sizeof(char*));
    }
    
    g_loader->search_paths[g_loader->search_path_count] = strdup(path);
    g_loader->search_path_count++;
    
    if (g_loader->verbose) {
        printf("Added search path: %s\n", path);
    }
    
    return 0;
}

// 查找模块文件
char* find_module_file(const char* module_name) {
    if (!g_loader || !module_name) return NULL;
    
    char* full_path = malloc(512);
    if (!full_path) return NULL;
    
    // 尝试各种文件扩展名
    const char* extensions[] = {".rt", ".native", ".so", ".dll", NULL};
    
    for (uint32_t i = 0; i < g_loader->search_path_count; i++) {
        for (int j = 0; extensions[j]; j++) {
            snprintf(full_path, 512, "%s/%s%s", 
                    g_loader->search_paths[i], module_name, extensions[j]);
            
            FILE* fp = fopen(full_path, "rb");
            if (fp) {
                fclose(fp);
                if (g_loader->verbose) {
                    printf("Found module file: %s\n", full_path);
                }
                return full_path;
            }
        }
    }
    
    free(full_path);
    return NULL;
}

// 加载.rt模块
Module* module_load_rt(const char* filename) {
    if (!filename) return NULL;
    
    if (g_loader->verbose) {
        printf("Loading RT module: %s\n", filename);
    }
    
    // 读取.rt文件
    RTFileHeader* header;
    void* code;
    size_t code_size;
    void* data;
    size_t data_size;
    RTMetadata* metadata;
    
    int result = rt_read_file(filename, &header, &code, &code_size, 
                             &data, &data_size, &metadata);
    if (result != 0) {
        printf("Error: Failed to read RT file %s\n", filename);
        return NULL;
    }
    
    // 验证文件头
    if (!rt_validate_header(header)) {
        printf("Error: Invalid RT file header in %s\n", filename);
        free(header);
        free(code);
        free(data);
        free(metadata);
        return NULL;
    }
    
    // 创建模块对象
    Module* module = calloc(1, sizeof(Module));
    if (!module) {
        printf("Error: Failed to allocate module\n");
        free(header);
        free(code);
        free(data);
        free(metadata);
        return NULL;
    }
    
    // 设置模块信息
    module->id = g_loader->module_count;
    strncpy(module->name, filename, sizeof(module->name) - 1);
    strncpy(module->path, filename, sizeof(module->path) - 1);
    module->type = MODULE_TYPE_RUNTIME;
    module->state = MODULE_STATE_LOADED;
    
    // 设置版本信息
    module->version_major = header->version_major;
    module->version_minor = header->version_minor;
    module->version_patch = header->version_patch;
    
    // 设置模块数据
    module->module_data = code;
    module->module_size = code_size;
    module->entry_point = (void*)((uintptr_t)code + header->entry_point);
    
    // 设置运行时信息
    module->is_resident = true;
    module->reference_count = 1;
    module->load_time = (uint64_t)time(NULL);
    
    if (g_loader->verbose) {
        printf("RT module loaded: %s (%zu bytes, entry=0x%p)\n", 
               module->name, module->module_size, module->entry_point);
        printf("  Version: %d.%d.%d\n", 
               module->version_major, module->version_minor, module->version_patch);
        printf("  Architecture: %d, OS: %d, ABI: %d\n", 
               header->architecture, header->os, header->abi);
    }
    
    // 添加到已加载模块列表
    if (g_loader->module_count >= g_loader->module_capacity) {
        g_loader->module_capacity *= 2;
        g_loader->loaded_modules = realloc(g_loader->loaded_modules, 
                                          g_loader->module_capacity * sizeof(Module*));
    }
    
    g_loader->loaded_modules[g_loader->module_count] = module;
    g_loader->module_count++;
    
    // 清理临时数据
    free(header);
    free(data);
    free(metadata);
    
    return module;
}

// 通过名称查找模块
Module* module_find_by_name(const char* name) {
    if (!g_loader || !name) return NULL;
    
    for (uint32_t i = 0; i < g_loader->module_count; i++) {
        Module* module = g_loader->loaded_modules[i];
        if (module && strstr(module->name, name)) {
            return module;
        }
    }
    
    return NULL;
}

// 加载模块（自动查找）
Module* module_load(const char* module_name) {
    if (!module_name) return NULL;
    
    // 检查是否已加载
    Module* existing = module_find_by_name(module_name);
    if (existing) {
        existing->reference_count++;
        if (g_loader->verbose) {
            printf("Module %s already loaded (ref_count=%d)\n", 
                   module_name, existing->reference_count);
        }
        return existing;
    }
    
    // 查找模块文件
    char* filename = find_module_file(module_name);
    if (!filename) {
        printf("Error: Module %s not found in search paths\n", module_name);
        return NULL;
    }
    
    // 加载模块
    Module* module = module_load_rt(filename);
    free(filename);
    
    return module;
}

// 卸载模块
void module_unload(Module* module) {
    if (!module) return;
    
    module->reference_count--;
    if (module->reference_count > 0) {
        if (g_loader->verbose) {
            printf("Module %s reference count decreased to %d\n", 
                   module->name, module->reference_count);
        }
        return;
    }
    
    if (g_loader->verbose) {
        printf("Unloading module: %s\n", module->name);
    }
    
    // 释放模块数据
    if (module->module_data) {
        free(module->module_data);
    }
    
    // 释放导入导出表
    if (module->exports) {
        free(module->exports);
    }
    if (module->imports) {
        free(module->imports);
    }
    if (module->dependencies) {
        free(module->dependencies);
    }
    
    // 从已加载列表中移除
    for (uint32_t i = 0; i < g_loader->module_count; i++) {
        if (g_loader->loaded_modules[i] == module) {
            // 移动后续元素
            for (uint32_t j = i; j < g_loader->module_count - 1; j++) {
                g_loader->loaded_modules[j] = g_loader->loaded_modules[j + 1];
            }
            g_loader->module_count--;
            break;
        }
    }
    
    free(module);
}

// 列出已加载的模块
void module_list_loaded(void) {
    if (!g_loader) {
        printf("Module loader not initialized\n");
        return;
    }

    printf("Loaded modules (%d):\n", g_loader->module_count);
    for (uint32_t i = 0; i < g_loader->module_count; i++) {
        Module* module = g_loader->loaded_modules[i];
        if (module) {
            printf("  [%d] %s (v%d.%d.%d, refs=%d, size=%zu)\n",
                   module->id, module->name,
                   module->version_major, module->version_minor, module->version_patch,
                   module->reference_count, module->module_size);
        }
    }
}

// ===============================================
// 符号解析功能
// ===============================================

// 在模块中查找符号
void* module_find_symbol(Module* module, const char* symbol_name) {
    if (!module || !symbol_name) return NULL;

    // 查找导出符号
    for (uint32_t i = 0; i < module->export_count; i++) {
        if (strcmp(module->exports[i].name, symbol_name) == 0) {
            if (g_loader && g_loader->verbose) {
                printf("Found symbol %s in module %s at 0x%p\n",
                       symbol_name, module->name, module->exports[i].function_ptr);
            }
            return module->exports[i].function_ptr;
        }
    }

    // 如果是libc模块，尝试查找标准函数
    if (module->type == MODULE_TYPE_LIBC) {
        // 简化的libc符号查找
        if (strcmp(symbol_name, "printf") == 0) {
            return (void*)printf;
        } else if (strcmp(symbol_name, "malloc") == 0) {
            return (void*)malloc;
        } else if (strcmp(symbol_name, "free") == 0) {
            return (void*)free;
        } else if (strcmp(symbol_name, "strlen") == 0) {
            return (void*)strlen;
        } else if (strcmp(symbol_name, "strcmp") == 0) {
            return (void*)strcmp;
        } else if (strcmp(symbol_name, "strcpy") == 0) {
            return (void*)strcpy;
        }
    }

    return NULL;
}

// 添加模块导出符号
int module_add_export(Module* module, const char* name, uint32_t function_id,
                     void* function_ptr, uint32_t param_count, uint32_t return_type) {
    if (!module || !name || !function_ptr) return -1;

    // 扩展导出表
    if (module->export_count >= 64) { // 最多64个导出
        printf("Warning: Maximum exports reached for module %s\n", module->name);
        return -1;
    }

    if (!module->exports) {
        module->exports = calloc(64, sizeof(ModuleExport));
        if (!module->exports) return -1;
    }

    // 添加导出
    ModuleExport* export = &module->exports[module->export_count];
    strncpy(export->name, name, sizeof(export->name) - 1);
    export->function_id = function_id;
    export->function_ptr = function_ptr;
    export->param_count = param_count;
    export->return_type = return_type;
    export->is_variadic = false;

    module->export_count++;

    if (g_loader && g_loader->verbose) {
        printf("Added export %s to module %s (id=0x%x, ptr=0x%p)\n",
               name, module->name, function_id, function_ptr);
    }

    return 0;
}

// 添加模块导入符号
int module_add_import(Module* module, const char* module_name,
                     const char* function_name, uint32_t local_id) {
    if (!module || !module_name || !function_name) return -1;

    // 扩展导入表
    if (module->import_count >= 64) { // 最多64个导入
        printf("Warning: Maximum imports reached for module %s\n", module->name);
        return -1;
    }

    if (!module->imports) {
        module->imports = calloc(64, sizeof(ModuleImport));
        if (!module->imports) return -1;
    }

    // 添加导入
    ModuleImport* import = &module->imports[module->import_count];
    strncpy(import->module_name, module_name, sizeof(import->module_name) - 1);
    strncpy(import->function_name, function_name, sizeof(import->function_name) - 1);
    import->local_id = local_id;
    import->resolved_ptr = NULL;
    import->is_resolved = false;

    module->import_count++;

    if (g_loader && g_loader->verbose) {
        printf("Added import %s.%s to module %s (local_id=%d)\n",
               module_name, function_name, module->name, local_id);
    }

    return 0;
}

// 解析模块的所有导入符号
int module_resolve_imports(Module* module) {
    if (!module) return -1;

    if (g_loader && g_loader->verbose) {
        printf("Resolving imports for module %s (%d imports)\n",
               module->name, module->import_count);
    }

    int resolved_count = 0;

    for (uint32_t i = 0; i < module->import_count; i++) {
        ModuleImport* import = &module->imports[i];
        if (import->is_resolved) {
            resolved_count++;
            continue;
        }

        // 查找依赖模块
        Module* dep_module = module_find_by_name(import->module_name);
        if (!dep_module) {
            // 尝试加载依赖模块
            dep_module = module_load(import->module_name);
            if (!dep_module) {
                printf("Warning: Failed to load dependency %s for module %s\n",
                       import->module_name, module->name);
                continue;
            }
        }

        // 查找符号
        void* symbol_ptr = module_find_symbol(dep_module, import->function_name);
        if (symbol_ptr) {
            import->resolved_ptr = symbol_ptr;
            import->is_resolved = true;
            resolved_count++;

            if (g_loader && g_loader->verbose) {
                printf("Resolved %s.%s -> 0x%p\n",
                       import->module_name, import->function_name, symbol_ptr);
            }
        } else {
            printf("Warning: Symbol %s not found in module %s\n",
                   import->function_name, import->module_name);
        }
    }

    if (g_loader && g_loader->verbose) {
        printf("Resolved %d/%d imports for module %s\n",
               resolved_count, module->import_count, module->name);
    }

    return (resolved_count == module->import_count) ? 0 : -1;
}

// ===============================================
// 系统模块管理
// ===============================================

// 初始化系统模块
int module_system_init(void) {
    if (!g_loader) {
        printf("Error: Module loader not initialized\n");
        return -1;
    }

    printf("Initializing system modules...\n");

    // 加载核心系统模块
    return module_system_load_core_modules();
}

// 加载系统模块
int module_system_load_core_modules(void) {
    int loaded_count = 0;

    // 尝试加载libc模块
    Module* libc_module = module_load("libc_x64_64");
    if (!libc_module) {
        libc_module = module_load("libc");
    }

    if (libc_module) {
        libc_module->type = MODULE_TYPE_LIBC;
        libc_module->is_resident = true;

        // 添加标准libc导出
        module_add_export(libc_module, "printf", 0x0030, (void*)printf, 1, 0);
        module_add_export(libc_module, "malloc", 0x0031, (void*)malloc, 1, 0);
        module_add_export(libc_module, "free", 0x0032, (void*)free, 1, 0);
        module_add_export(libc_module, "strlen", 0x0033, (void*)strlen, 1, 0);
        module_add_export(libc_module, "strcmp", 0x0034, (void*)strcmp, 2, 0);
        module_add_export(libc_module, "strcpy", 0x0035, (void*)strcpy, 2, 0);
        module_add_export(libc_module, "strcat", 0x0036, (void*)strcat, 2, 0);
        module_add_export(libc_module, "memcpy", 0x0037, (void*)memcpy, 3, 0);
        module_add_export(libc_module, "memset", 0x0038, (void*)memset, 3, 0);
        module_add_export(libc_module, "fopen", 0x0039, (void*)fopen, 2, 0);
        module_add_export(libc_module, "fclose", 0x003A, (void*)fclose, 1, 0);
        module_add_export(libc_module, "fread", 0x003B, (void*)fread, 4, 0);
        module_add_export(libc_module, "fwrite", 0x003C, (void*)fwrite, 4, 0);

        loaded_count++;
        printf("Loaded libc module with %d exports\n", libc_module->export_count);
    } else {
        printf("Warning: Failed to load libc module\n");
    }

    // 尝试加载VM运行时模块
    Module* vm_module = module_load("vm_x64_64");
    if (!vm_module) {
        vm_module = module_load("runtime");
    }

    if (vm_module) {
        vm_module->type = MODULE_TYPE_RUNTIME;
        vm_module->is_resident = true;
        loaded_count++;
        printf("Loaded VM runtime module\n");
    } else {
        printf("Warning: Failed to load VM runtime module\n");
    }

    printf("Loaded %d core system modules\n", loaded_count);
    return (loaded_count > 0) ? 0 : -1;
}

// 获取模块统计信息
void module_get_statistics(uint32_t* total_modules, uint32_t* loaded_modules,
                          size_t* total_memory) {
    if (!g_loader) {
        if (total_modules) *total_modules = 0;
        if (loaded_modules) *loaded_modules = 0;
        if (total_memory) *total_memory = 0;
        return;
    }

    if (total_modules) *total_modules = g_loader->module_count;
    if (loaded_modules) *loaded_modules = g_loader->module_count;

    if (total_memory) {
        size_t memory = 0;
        for (uint32_t i = 0; i < g_loader->module_count; i++) {
            Module* module = g_loader->loaded_modules[i];
            if (module) {
                memory += module->module_size;
                memory += sizeof(Module);
                if (module->exports) {
                    memory += module->export_count * sizeof(ModuleExport);
                }
                if (module->imports) {
                    memory += module->import_count * sizeof(ModuleImport);
                }
            }
        }
        *total_memory = memory;
    }
}
