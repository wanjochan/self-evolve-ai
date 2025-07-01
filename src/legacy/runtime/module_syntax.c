/**
 * module_syntax.c - 模块化程序设计实现
 * 
 * 实现模块导入、导出和使用的语法扩展
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "module_syntax.h"
#include "module_loader.h"

// 全局模块化程序系统状态
static bool g_modular_system_initialized = false;

// 初始化模块化程序系统
int modular_program_init(void) {
    if (g_modular_system_initialized) {
        printf("Warning: Modular program system already initialized\n");
        return 0;
    }
    
    // 初始化模块加载器
    if (module_loader_init(true) != 0) {
        printf("Error: Failed to initialize module loader\n");
        return -1;
    }
    
    // 加载系统模块
    if (module_system_init() != 0) {
        printf("Warning: Failed to load some system modules\n");
    }
    
    g_modular_system_initialized = true;
    printf("Modular program system initialized\n");
    return 0;
}

// 创建模块化程序
ModularProgram* modular_program_create(const char* program_name, const char* program_version) {
    if (!program_name) return NULL;
    
    ModularProgram* program = calloc(1, sizeof(ModularProgram));
    if (!program) return NULL;
    
    strncpy(program->program_name, program_name, sizeof(program->program_name) - 1);
    if (program_version) {
        strncpy(program->program_version, program_version, sizeof(program->program_version) - 1);
    } else {
        strcpy(program->program_version, "1.0.0");
    }
    
    printf("Created modular program: %s v%s\n", program->program_name, program->program_version);
    return program;
}

// 添加模块导入
int modular_program_add_import(ModularProgram* program, const char* module_name, 
                              const char* alias, const char* version) {
    if (!program || !module_name) return -1;
    
    // 扩展导入数组
    if (program->import_count >= 32) { // 最多32个导入
        printf("Error: Maximum imports reached for program %s\n", program->program_name);
        return -1;
    }
    
    if (!program->imports) {
        program->imports = calloc(32, sizeof(ModuleImport));
        if (!program->imports) return -1;
    }
    
    ModuleImport* import = &program->imports[program->import_count];
    strncpy(import->module_name, module_name, sizeof(import->module_name) - 1);
    
    if (alias) {
        strncpy(import->alias, alias, sizeof(import->alias) - 1);
    } else {
        strcpy(import->alias, module_name);
    }
    
    if (version) {
        strncpy(import->version, version, sizeof(import->version) - 1);
    } else {
        strcpy(import->version, "*");
    }
    
    // 检查是否为系统模块
    import->is_system_module = (strcmp(module_name, LIBC_MODULE_NAME) == 0 ||
                               strcmp(module_name, MATH_MODULE_NAME) == 0 ||
                               strcmp(module_name, IO_MODULE_NAME) == 0 ||
                               strcmp(module_name, THREAD_MODULE_NAME) == 0);
    
    import->is_required = true; // 默认为必需
    
    program->import_count++;
    
    printf("Added import %s (alias: %s, version: %s) to program %s\n",
           module_name, import->alias, import->version, program->program_name);
    
    return 0;
}

// 添加模块导出
int modular_program_add_export(ModularProgram* program, const char* symbol_name,
                              uint32_t symbol_type, void* symbol_address) {
    if (!program || !symbol_name) return -1;
    
    // 扩展导出数组
    if (program->export_count >= 32) { // 最多32个导出
        printf("Error: Maximum exports reached for program %s\n", program->program_name);
        return -1;
    }
    
    if (!program->exports) {
        program->exports = calloc(32, sizeof(ModuleExport));
        if (!program->exports) return -1;
    }
    
    ModuleExport* export = &program->exports[program->export_count];
    strncpy(export->symbol_name, symbol_name, sizeof(export->symbol_name) - 1);
    export->symbol_type = symbol_type;
    export->symbol_address = symbol_address;
    export->is_public = true; // 默认公开导出
    
    program->export_count++;
    
    printf("Added export %s (type: %u, addr: %p) to program %s\n",
           symbol_name, symbol_type, symbol_address, program->program_name);
    
    return 0;
}

// 解析模块导入
int modular_program_resolve_imports(ModularProgram* program) {
    if (!program) return -1;
    
    printf("Resolving imports for program %s (%u imports)\n", 
           program->program_name, program->import_count);
    
    int resolved_count = 0;
    
    for (uint32_t i = 0; i < program->import_count; i++) {
        ModuleImport* import = &program->imports[i];
        
        // 尝试加载模块
        Module* module = module_load(import->module_name);
        if (!module) {
            printf("Warning: Failed to load module %s for program %s\n",
                   import->module_name, program->program_name);
            if (import->is_required) {
                return -1;
            }
            continue;
        }
        
        // 解析模块的导入符号
        if (module_resolve_imports(module) == 0) {
            resolved_count++;
            printf("Resolved module %s for program %s\n", 
                   import->module_name, program->program_name);
        } else {
            printf("Warning: Failed to resolve imports for module %s\n", 
                   import->module_name);
        }
    }
    
    printf("Resolved %d/%u imports for program %s\n", 
           resolved_count, program->import_count, program->program_name);
    
    return (resolved_count == program->import_count) ? 0 : -1;
}

// 查找导入的符号
void* modular_program_find_symbol(ModularProgram* program, const char* module_name, 
                                 const char* symbol_name) {
    if (!program || !module_name || !symbol_name) return NULL;
    
    // 查找模块
    Module* module = module_find_by_name(module_name);
    if (!module) {
        printf("Warning: Module %s not found for symbol lookup\n", module_name);
        return NULL;
    }
    
    // 查找符号
    void* symbol = module_find_symbol(module, symbol_name);
    if (symbol) {
        printf("Found symbol %s::%s at %p\n", module_name, symbol_name, symbol);
    } else {
        printf("Warning: Symbol %s::%s not found\n", module_name, symbol_name);
    }
    
    return symbol;
}

// 销毁模块化程序
void modular_program_destroy(ModularProgram* program) {
    if (!program) return;
    
    printf("Destroying modular program: %s\n", program->program_name);
    
    if (program->imports) {
        free(program->imports);
    }
    
    if (program->exports) {
        free(program->exports);
    }
    
    free(program);
}

// ===============================================
// 编译器集成函数
// ===============================================

// 解析模块导入指令
int parse_module_import(const char* source_line, ModuleImport* import) {
    if (!source_line || !import) return -1;
    
    // 查找 #import "module_name"
    const char* import_start = strstr(source_line, "#import");
    if (!import_start) return -1;
    
    // 跳过 #import
    const char* quote_start = strchr(import_start + 7, '"');
    if (!quote_start) return -1;
    
    const char* quote_end = strchr(quote_start + 1, '"');
    if (!quote_end) return -1;
    
    // 提取模块名
    size_t name_len = quote_end - quote_start - 1;
    if (name_len >= sizeof(import->module_name)) return -1;
    
    strncpy(import->module_name, quote_start + 1, name_len);
    import->module_name[name_len] = '\0';
    
    // 设置默认值
    strcpy(import->alias, import->module_name);
    strcpy(import->version, "*");
    import->is_system_module = false;
    import->is_required = true;
    
    printf("Parsed import: %s\n", import->module_name);
    return 0;
}

// 解析模块导出指令
int parse_module_export(const char* source_line, ModuleExport* export) {
    if (!source_line || !export) return -1;
    
    // 查找 #export symbol_name
    const char* export_start = strstr(source_line, "#export");
    if (!export_start) return -1;
    
    // 跳过 #export 和空格
    const char* symbol_start = export_start + 7;
    while (*symbol_start == ' ' || *symbol_start == '\t') symbol_start++;
    
    // 查找符号名结束
    const char* symbol_end = symbol_start;
    while (*symbol_end && *symbol_end != ' ' && *symbol_end != '\t' && 
           *symbol_end != '\n' && *symbol_end != '\r') {
        symbol_end++;
    }
    
    // 提取符号名
    size_t name_len = symbol_end - symbol_start;
    if (name_len >= sizeof(export->symbol_name)) return -1;
    
    strncpy(export->symbol_name, symbol_start, name_len);
    export->symbol_name[name_len] = '\0';
    
    // 设置默认值
    export->symbol_type = 0; // 函数类型
    export->symbol_address = NULL;
    export->is_public = true;
    
    printf("Parsed export: %s\n", export->symbol_name);
    return 0;
}

// 解析模块函数调用
int parse_module_call(const char* source_line, char* module_name, char* function_name) {
    if (!source_line || !module_name || !function_name) return -1;
    
    // 查找 module_name::function_name
    const char* double_colon = strstr(source_line, "::");
    if (!double_colon) return -1;
    
    // 向前查找模块名开始
    const char* module_start = double_colon - 1;
    while (module_start > source_line && 
           ((*module_start >= 'a' && *module_start <= 'z') ||
            (*module_start >= 'A' && *module_start <= 'Z') ||
            (*module_start >= '0' && *module_start <= '9') ||
            *module_start == '_' || *module_start == '.')) {
        module_start--;
    }
    module_start++; // 调整到实际开始位置
    
    // 提取模块名
    size_t module_len = double_colon - module_start;
    if (module_len >= 64) return -1;
    strncpy(module_name, module_start, module_len);
    module_name[module_len] = '\0';
    
    // 向后查找函数名
    const char* function_start = double_colon + 2;
    const char* function_end = function_start;
    while (*function_end && 
           ((*function_end >= 'a' && *function_end <= 'z') ||
            (*function_end >= 'A' && *function_end <= 'Z') ||
            (*function_end >= '0' && *function_end <= '9') ||
            *function_end == '_')) {
        function_end++;
    }
    
    // 提取函数名
    size_t function_len = function_end - function_start;
    if (function_len >= 64) return -1;
    strncpy(function_name, function_start, function_len);
    function_name[function_len] = '\0';
    
    printf("Parsed module call: %s::%s\n", module_name, function_name);
    return 0;
}

// 获取模块化程序统计信息
void modular_program_get_stats(ModularProgram* program, uint32_t* import_count,
                              uint32_t* export_count, size_t* memory_usage) {
    if (!program) {
        if (import_count) *import_count = 0;
        if (export_count) *export_count = 0;
        if (memory_usage) *memory_usage = 0;
        return;
    }
    
    if (import_count) *import_count = program->import_count;
    if (export_count) *export_count = program->export_count;
    
    if (memory_usage) {
        size_t memory = sizeof(ModularProgram);
        if (program->imports) {
            memory += program->import_count * sizeof(ModuleImport);
        }
        if (program->exports) {
            memory += program->export_count * sizeof(ModuleExport);
        }
        *memory_usage = memory;
    }
}
