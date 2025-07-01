/**
 * libc_rt_module.c - libc.rt模块化实现
 */

#include "libc_rt_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ===============================================
// 模块管理实现
// ===============================================

LibcRtModule* libc_rt_module_create(void) {
    LibcRtModule* module = calloc(1, sizeof(LibcRtModule));
    if (!module) return NULL;
    
    // 初始化头部
    memcpy(module->header.magic, LIBC_RT_MAGIC, 4);
    module->header.version = LIBC_RT_VERSION;
    module->header.function_count = 0;
    
    // 分配符号表
    module->symbols = calloc(MAX_LIBC_FUNCTIONS, sizeof(LibcFunctionSymbol));
    if (!module->symbols) {
        free(module);
        return NULL;
    }
    
    module->is_loaded = false;
    module->is_initialized = false;
    
    return module;
}

void libc_rt_module_free(LibcRtModule* module) {
    if (!module) return;
    
    if (module->symbols) {
        free(module->symbols);
    }
    if (module->code_section) {
        free(module->code_section);
    }
    if (module->data_section) {
        free(module->data_section);
    }
    
    free(module);
}

// ===============================================
// 函数管理实现
// ===============================================

int libc_rt_module_add_function(LibcRtModule* module, 
                               const char* name,
                               LibcFunctionId func_id,
                               void* function_ptr,
                               uint32_t param_count,
                               uint32_t return_type) {
    if (!module || !name || module->header.function_count >= MAX_LIBC_FUNCTIONS) {
        return -1;
    }
    
    LibcFunctionSymbol* symbol = &module->symbols[module->header.function_count];
    
    strncpy(symbol->name, name, MAX_FUNCTION_NAME_LEN - 1);
    symbol->name[MAX_FUNCTION_NAME_LEN - 1] = '\0';
    symbol->function_id = func_id;
    symbol->param_count = param_count;
    symbol->return_type = return_type;
    
    // 存储函数指针
    if (func_id < MAX_LIBC_FUNCTIONS) {
        module->function_table[func_id] = function_ptr;
    }
    
    module->header.function_count++;
    return 0;
}

void* libc_rt_module_get_function(LibcRtModule* module, const char* name) {
    if (!module || !name) return NULL;
    
    for (uint32_t i = 0; i < module->header.function_count; i++) {
        if (strcmp(module->symbols[i].name, name) == 0) {
            return module->function_table[module->symbols[i].function_id];
        }
    }
    
    return NULL;
}

void* libc_rt_module_get_function_by_id(LibcRtModule* module, LibcFunctionId func_id) {
    if (!module || func_id >= MAX_LIBC_FUNCTIONS) return NULL;
    
    return module->function_table[func_id];
}

// ===============================================
// 标准模块构建器
// ===============================================

LibcRtModule* libc_rt_build_standard_module(void) {
    LibcRtModule* module = libc_rt_module_create();
    if (!module) return NULL;
    
    printf("Building standard libc.rt module...\n");
    
    // 添加内存管理函数
    libc_rt_module_add_function(module, "malloc", LIBC_FUNC_MALLOC, malloc, 1, 0);
    libc_rt_module_add_function(module, "free", LIBC_FUNC_FREE, free, 1, 0);
    libc_rt_module_add_function(module, "calloc", LIBC_FUNC_CALLOC, calloc, 2, 0);
    libc_rt_module_add_function(module, "realloc", LIBC_FUNC_REALLOC, realloc, 2, 0);
    
    // 添加内存操作函数
    libc_rt_module_add_function(module, "memcpy", LIBC_FUNC_MEMCPY, memcpy, 3, 0);
    libc_rt_module_add_function(module, "memset", LIBC_FUNC_MEMSET, memset, 3, 0);
    libc_rt_module_add_function(module, "memcmp", LIBC_FUNC_MEMCMP, memcmp, 3, 0);
    libc_rt_module_add_function(module, "memmove", LIBC_FUNC_MEMMOVE, memmove, 3, 0);
    
    // 添加字符串函数
    libc_rt_module_add_function(module, "strlen", LIBC_FUNC_STRLEN, strlen, 1, 0);
    libc_rt_module_add_function(module, "strcpy", LIBC_FUNC_STRCPY, strcpy, 2, 0);
    libc_rt_module_add_function(module, "strncpy", LIBC_FUNC_STRNCPY, strncpy, 3, 0);
    libc_rt_module_add_function(module, "strcmp", LIBC_FUNC_STRCMP, strcmp, 2, 0);
    libc_rt_module_add_function(module, "strncmp", LIBC_FUNC_STRNCMP, strncmp, 3, 0);
    libc_rt_module_add_function(module, "strcat", LIBC_FUNC_STRCAT, strcat, 2, 0);
    libc_rt_module_add_function(module, "strncat", LIBC_FUNC_STRNCAT, strncat, 3, 0);
    libc_rt_module_add_function(module, "strchr", LIBC_FUNC_STRCHR, strchr, 2, 0);
    libc_rt_module_add_function(module, "strrchr", LIBC_FUNC_STRRCHR, strrchr, 2, 0);
    libc_rt_module_add_function(module, "strstr", LIBC_FUNC_STRSTR, strstr, 2, 0);
    
    // 添加I/O函数
    libc_rt_module_add_function(module, "printf", LIBC_FUNC_PRINTF, printf, -1, 0); // 可变参数
    libc_rt_module_add_function(module, "sprintf", LIBC_FUNC_SPRINTF, sprintf, -1, 0);
    libc_rt_module_add_function(module, "fprintf", LIBC_FUNC_FPRINTF, fprintf, -1, 0);
    
    // 添加文件操作函数
    libc_rt_module_add_function(module, "fopen", LIBC_FUNC_FOPEN, fopen, 2, 0);
    libc_rt_module_add_function(module, "fclose", LIBC_FUNC_FCLOSE, fclose, 1, 0);
    libc_rt_module_add_function(module, "fread", LIBC_FUNC_FREAD, fread, 4, 0);
    libc_rt_module_add_function(module, "fwrite", LIBC_FUNC_FWRITE, fwrite, 4, 0);
    libc_rt_module_add_function(module, "fseek", LIBC_FUNC_FSEEK, fseek, 3, 0);
    libc_rt_module_add_function(module, "ftell", LIBC_FUNC_FTELL, ftell, 1, 0);
    
    // 添加转换函数
    libc_rt_module_add_function(module, "atoi", LIBC_FUNC_ATOI, atoi, 1, 0);
    libc_rt_module_add_function(module, "atol", LIBC_FUNC_ATOL, atol, 1, 0);
    libc_rt_module_add_function(module, "atof", LIBC_FUNC_ATOF, atof, 1, 0);
    
    // 添加系统函数
    libc_rt_module_add_function(module, "exit", LIBC_FUNC_EXIT, exit, 1, 0);
    libc_rt_module_add_function(module, "system", LIBC_FUNC_SYSTEM, system, 1, 0);
    libc_rt_module_add_function(module, "getenv", LIBC_FUNC_GETENV, getenv, 1, 0);
    
    module->is_loaded = true;
    
    printf("Standard libc.rt module built with %u functions\n", module->header.function_count);
    return module;
}

LibcRtModule* libc_rt_build_minimal_module(void) {
    LibcRtModule* module = libc_rt_module_create();
    if (!module) return NULL;
    
    printf("Building minimal libc.rt module...\n");
    
    // 仅添加最核心的函数
    libc_rt_module_add_function(module, "malloc", LIBC_FUNC_MALLOC, malloc, 1, 0);
    libc_rt_module_add_function(module, "free", LIBC_FUNC_FREE, free, 1, 0);
    libc_rt_module_add_function(module, "printf", LIBC_FUNC_PRINTF, printf, -1, 0);
    libc_rt_module_add_function(module, "strlen", LIBC_FUNC_STRLEN, strlen, 1, 0);
    libc_rt_module_add_function(module, "memcpy", LIBC_FUNC_MEMCPY, memcpy, 3, 0);
    libc_rt_module_add_function(module, "memset", LIBC_FUNC_MEMSET, memset, 3, 0);
    
    module->is_loaded = true;
    
    printf("Minimal libc.rt module built with %u functions\n", module->header.function_count);
    return module;
}

// ===============================================
// 模块验证和信息
// ===============================================

bool libc_rt_module_validate(LibcRtModule* module) {
    if (!module) return false;
    
    // 检查魔数
    if (memcmp(module->header.magic, LIBC_RT_MAGIC, 4) != 0) {
        return false;
    }
    
    // 检查版本
    if (module->header.version != LIBC_RT_VERSION) {
        return false;
    }
    
    // 检查函数数量
    if (module->header.function_count > MAX_LIBC_FUNCTIONS) {
        return false;
    }
    
    return true;
}

void libc_rt_module_print_info(LibcRtModule* module) {
    if (!module) {
        printf("Module is NULL\n");
        return;
    }
    
    printf("=== libc.rt Module Information ===\n");
    printf("Magic: %.4s\n", module->header.magic);
    printf("Version: %u\n", module->header.version);
    printf("Function count: %u\n", module->header.function_count);
    printf("Loaded: %s\n", module->is_loaded ? "Yes" : "No");
    printf("Initialized: %s\n", module->is_initialized ? "Yes" : "No");
    printf("Total size: %u bytes\n", module->header.total_size);
}

void libc_rt_module_print_symbols(LibcRtModule* module) {
    if (!module) return;
    
    printf("=== libc.rt Symbol Table ===\n");
    for (uint32_t i = 0; i < module->header.function_count; i++) {
        LibcFunctionSymbol* symbol = &module->symbols[i];
        printf("%3u: %-20s ID=0x%04X Params=%u\n", 
               i, symbol->name, symbol->function_id, symbol->param_count);
    }
}

bool libc_rt_module_has_function(LibcRtModule* module, const char* name) {
    return libc_rt_module_get_function(module, name) != NULL;
}

bool libc_rt_module_has_function_id(LibcRtModule* module, LibcFunctionId func_id) {
    return libc_rt_module_get_function_by_id(module, func_id) != NULL;
}

void libc_rt_module_get_stats(LibcRtModule* module, LibcRtModuleStats* stats) {
    if (!module || !stats) return;
    
    memset(stats, 0, sizeof(LibcRtModuleStats));
    
    stats->total_functions = module->header.function_count;
    stats->memory_usage = sizeof(LibcRtModule) + 
                         module->header.function_count * sizeof(LibcFunctionSymbol);
    
    // 计算加载的函数数量
    for (uint32_t i = 0; i < module->header.function_count; i++) {
        if (module->function_table[module->symbols[i].function_id] != NULL) {
            stats->loaded_functions++;
        } else {
            stats->failed_functions++;
        }
    }
    
    stats->code_size = module->header.code_section_offset;
    stats->data_size = module->header.data_section_offset;
}

// ===============================================
// 运行时集成
// ===============================================

int libc_rt_call_function(LibcRtModule* module, 
                         LibcFunctionId func_id,
                         void* args,
                         size_t args_size,
                         void* result,
                         size_t result_size) {
    if (!module) return -1;
    
    void* func_ptr = libc_rt_module_get_function_by_id(module, func_id);
    if (!func_ptr) return -1;
    
    // 这里需要根据具体的调用约定来实现函数调用
    // 简化实现，实际需要更复杂的参数传递机制
    
    printf("Calling function ID 0x%04X\n", func_id);
    return 0;
}
