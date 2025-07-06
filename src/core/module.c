/**
 * module.c - 核心模块系统实现
 * 
 * 这是整个系统的核心骨架实现，连接所有底层组件。
 * 设计理念：极简主义 + 高度灵活性 + 自我进化能力
 */

#include "module.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ===============================================
// 内部常量和配置
// ===============================================

// 最大可注册模块数
#define MAX_MODULES 64

// 每个模块最大依赖数
#define MAX_DEPENDENCIES 16

// 符号缓存哈希表大小
#define SYMBOL_CACHE_SIZE 256

// ===============================================
// 内部数据结构
// ===============================================

// 符号缓存条目
typedef struct SymbolCacheEntry {
    const char* symbol;             // 符号名称
    void* address;                  // 符号地址
    struct SymbolCacheEntry* next;  // 链表下一项
} SymbolCacheEntry;

// 模块依赖信息
typedef struct {
    const char** names;    // 依赖模块名称数组
    Module** modules;      // 解析后的依赖模块数组
    size_t count;          // 依赖数量
} ModuleDependencies;

// 模块注册表
static struct {
    Module* modules[MAX_MODULES];                    // 已注册模块数组
    size_t count;                                    // 已注册模块数量
    bool initialized;                                // 是否已初始化
    SymbolCacheEntry* symbol_cache[SYMBOL_CACHE_SIZE]; // 符号缓存哈希表
    ModuleDependencies dependencies[MAX_MODULES];    // 每个模块的依赖信息
} module_registry = {
    .modules = {NULL},
    .count = 0,
    .initialized = false,
    .symbol_cache = {NULL},
    .dependencies = {{NULL}}
};

// ===============================================
// 内部函数声明
// ===============================================

// 查找模块
static Module* find_module(const char* name);

// 解析依赖
static int resolve_dependencies(Module* module);

// 符号哈希函数
static unsigned char symbol_hash(const char* symbol);

// 查找缓存的符号
static void* find_cached_symbol(const char* symbol);

// 缓存符号
static void cache_symbol(const char* symbol, void* address);

// 清除符号缓存
static void clear_symbol_cache(void);

// 注册依赖
static int register_dependency(size_t module_index, const char* dep_name);

// ===============================================
// 核心API实现
// ===============================================

/**
 * 初始化模块系统
 */
int module_init(void) {
    if (module_registry.initialized) {
        return 0;  // 已初始化
    }
    
    // 清除符号缓存
    memset(module_registry.symbol_cache, 0, sizeof(module_registry.symbol_cache));
    
    // 初始化依赖信息
    for (size_t i = 0; i < MAX_MODULES; i++) {
        module_registry.dependencies[i].names = NULL;
        module_registry.dependencies[i].modules = NULL;
        module_registry.dependencies[i].count = 0;
    }
    
    module_registry.initialized = true;
    return 0;
}

/**
 * 清理模块系统
 */
void module_cleanup(void) {
    if (!module_registry.initialized) {
        return;  // 未初始化
    }
    
    // 按照相反顺序卸载所有模块
    for (int i = (int)module_registry.count - 1; i >= 0; i--) {
        Module* module = module_registry.modules[i];
        if (module && module_is_loaded(module)) {
            module_unload(module);
        }
    }
    
    // 清除符号缓存
    clear_symbol_cache();
    
    // 释放依赖信息
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
 * 注册模块
 */
int module_register(Module* module) {
    if (!module || !module->name) {
        return -1;
    }
    
    // 检查模块是否已注册
    if (find_module(module->name)) {
        return 0;  // 已注册
    }
    
    // 添加到注册表
    if (module_registry.count >= MAX_MODULES) {
        return -1;  // 注册表已满
    }
    
    size_t module_index = module_registry.count;
    module_registry.modules[module_index] = module;
    module_registry.count++;
    module->state = MODULE_UNLOADED;
    
    return 0;
}

/**
 * 注册单个依赖
 */
int module_register_dependency(Module* module, const char* dependency) {
    if (!module || !dependency) {
        return -1;
    }
    
    // 查找模块索引
    size_t module_index = MAX_MODULES;
    for (size_t i = 0; i < module_registry.count; i++) {
        if (module_registry.modules[i] == module) {
            module_index = i;
            break;
        }
    }
    
    if (module_index == MAX_MODULES) {
        return -1;  // 未找到模块
    }
    
    return register_dependency(module_index, dependency);
}

/**
 * 注册多个依赖
 */
int module_register_dependencies(Module* module, const char** dependencies) {
    if (!module || !dependencies) {
        return -1;
    }
    
    // 查找模块索引
    size_t module_index = MAX_MODULES;
    for (size_t i = 0; i < module_registry.count; i++) {
        if (module_registry.modules[i] == module) {
            module_index = i;
            break;
        }
    }
    
    if (module_index == MAX_MODULES) {
        return -1;  // 未找到模块
    }
    
    // 注册每个依赖
    for (size_t i = 0; dependencies[i]; i++) {
        if (register_dependency(module_index, dependencies[i]) != 0) {
            return -1;
        }
    }
    
    return 0;
}

/**
 * 加载模块
 */
Module* module_load(const char* name) {
    if (!name) {
        return NULL;
    }
    
    // 必要时初始化模块系统
    if (!module_registry.initialized) {
        if (module_init() != 0) {
            return NULL;
        }
    }
    
    // 查找模块
    Module* module = find_module(name);
    if (!module) {
        return NULL;
    }
    
    // 检查是否已加载
    if (module_is_loaded(module)) {
        return module;
    }
    
    // 设置状态为加载中
    module->state = MODULE_LOADING;
    
    // 解析依赖
    if (resolve_dependencies(module) != 0) {
        module->state = MODULE_ERROR;
        module->error = "无法解析依赖";
        return NULL;
    }
    
    // 加载模块
    if (module->load && module->load() != 0) {
        module->state = MODULE_ERROR;
        module->error = "加载模块失败";
        return NULL;
    }
    
    // 调用初始化回调
    if (module->on_init) {
        module->on_init();
    }
    
    // 设置状态为就绪
    module->state = MODULE_READY;
    
    return module;
}

/**
 * 卸载模块
 */
void module_unload(Module* module) {
    if (!module || !module_is_loaded(module)) {
        return;
    }
    
    // 调用退出回调
    if (module->on_exit) {
        module->on_exit();
    }
    
    // 卸载模块
    if (module->unload) {
        module->unload();
    }
    
    // 设置状态为未加载
    module->state = MODULE_UNLOADED;
    
    // 清除相关的符号缓存
    // 注意：这里可以优化为只清除与此模块相关的缓存
    clear_symbol_cache();
}

/**
 * 从特定模块解析符号
 */
void* module_resolve(Module* module, const char* symbol) {
    if (!module || !symbol || !module_is_loaded(module)) {
        return NULL;
    }
    
    // 检查缓存
    void* cached = find_cached_symbol(symbol);
    if (cached) {
        return cached;
    }
    
    // 解析符号
    void* address = NULL;
    if (module->resolve) {
        address = module->resolve(symbol);
    }
    
    // 缓存结果
    if (address) {
        cache_symbol(symbol, address);
    }
    
    return address;
}

/**
 * 从任何已加载模块解析符号
 */
void* module_resolve_global(const char* symbol) {
    if (!symbol) {
        return NULL;
    }
    
    // 检查缓存
    void* cached = find_cached_symbol(symbol);
    if (cached) {
        return cached;
    }
    
    // 在所有已加载模块中查找
    for (size_t i = 0; i < module_registry.count; i++) {
        Module* module = module_registry.modules[i];
        if (module && module_is_loaded(module)) {
            void* address = module_resolve(module, symbol);
            if (address) {
                return address;
            }
        }
    }
    
    return NULL;
}

/**
 * 获取模块
 */
Module* module_get(const char* name) {
    return find_module(name);
}

/**
 * 获取模块依赖
 */
const char** module_get_dependencies(const Module* module) {
    if (!module) {
        return NULL;
    }
    
    // 查找模块索引
    size_t module_index = MAX_MODULES;
    for (size_t i = 0; i < module_registry.count; i++) {
        if (module_registry.modules[i] == module) {
            module_index = i;
            break;
        }
    }
    
    if (module_index == MAX_MODULES) {
        return NULL;  // 未找到模块
    }
    
    return module_registry.dependencies[module_index].names;
}

/**
 * 获取模块最后错误
 */
const char* module_get_error(const Module* module) {
    return module ? module->error : NULL;
}

/**
 * 获取模块状态
 */
ModuleState module_get_state(const Module* module) {
    return module ? module->state : MODULE_UNLOADED;
}

/**
 * 检查模块是否已加载
 */
bool module_is_loaded(const Module* module) {
    return module && module->state == MODULE_READY;
}

// ===============================================
// 内部函数实现
// ===============================================

/**
 * 查找模块
 */
static Module* find_module(const char* name) {
    if (!name) {
        return NULL;
    }
    
    for (size_t i = 0; i < module_registry.count; i++) {
        if (module_registry.modules[i] && 
            strcmp(module_registry.modules[i]->name, name) == 0) {
            return module_registry.modules[i];
        }
    }
    
    return NULL;
}

/**
 * 解析依赖
 */
static int resolve_dependencies(Module* module) {
    if (!module) {
        return -1;
    }
    
    // 查找模块索引
    size_t module_index = MAX_MODULES;
    for (size_t i = 0; i < module_registry.count; i++) {
        if (module_registry.modules[i] == module) {
            module_index = i;
            break;
        }
    }
    
    if (module_index == MAX_MODULES) {
        return -1;  // 未找到模块
    }
    
    // 获取依赖信息
    ModuleDependencies* deps = &module_registry.dependencies[module_index];
    if (!deps->names || deps->count == 0) {
        return 0;  // 无依赖
    }
    
    // 确保模块数组已分配
    if (!deps->modules) {
        deps->modules = (Module**)calloc(deps->count, sizeof(Module*));
        if (!deps->modules) {
            return -1;  // 内存分配失败
        }
    }
    
    // 加载每个依赖
    for (size_t i = 0; i < deps->count; i++) {
        const char* dep_name = deps->names[i];
        Module* dep_module = module_load(dep_name);
        if (!dep_module) {
            return -1;  // 加载依赖失败
        }
        deps->modules[i] = dep_module;
    }
    
    return 0;
}

/**
 * 符号哈希函数
 */
static unsigned char symbol_hash(const char* symbol) {
    unsigned char hash = 0;
    while (*symbol) {
        hash = hash * 31 + *symbol;
        symbol++;
    }
    return hash;
}

/**
 * 查找缓存的符号
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
 * 缓存符号
 */
static void cache_symbol(const char* symbol, void* address) {
    unsigned char hash = symbol_hash(symbol);
    
    // 创建新条目
    SymbolCacheEntry* entry = (SymbolCacheEntry*)malloc(sizeof(SymbolCacheEntry));
    if (!entry) {
        return;  // 内存分配失败
    }
    
    // 复制符号名称
    char* symbol_copy = strdup(symbol);
    if (!symbol_copy) {
        free(entry);
        return;  // 内存分配失败
    }
    
    // 设置条目
    entry->symbol = symbol_copy;
    entry->address = address;
    
    // 添加到哈希表
    entry->next = module_registry.symbol_cache[hash];
    module_registry.symbol_cache[hash] = entry;
}

/**
 * 清除符号缓存
 */
static void clear_symbol_cache(void) {
    for (size_t i = 0; i < SYMBOL_CACHE_SIZE; i++) {
        SymbolCacheEntry* entry = module_registry.symbol_cache[i];
        while (entry) {
            SymbolCacheEntry* next = entry->next;
            free((void*)entry->symbol);  // 释放符号名称
            free(entry);                 // 释放条目
            entry = next;
        }
        module_registry.symbol_cache[i] = NULL;
    }
}

/**
 * 注册依赖
 */
static int register_dependency(size_t module_index, const char* dep_name) {
    if (module_index >= MAX_MODULES || !dep_name) {
        return -1;
    }
    
    ModuleDependencies* deps = &module_registry.dependencies[module_index];
    
    // 检查是否已经有此依赖
    if (deps->names) {
        for (size_t i = 0; i < deps->count; i++) {
            if (strcmp(deps->names[i], dep_name) == 0) {
                return 0;  // 已存在
            }
        }
    }
    
    // 检查是否达到最大依赖数
    if (deps->count >= MAX_DEPENDENCIES) {
        return -1;  // 太多依赖
    }
    
    // 首次添加依赖
    if (!deps->names) {
        deps->names = (const char**)calloc(MAX_DEPENDENCIES, sizeof(const char*));
        if (!deps->names) {
            return -1;  // 内存分配失败
        }
    }
    
    // 复制依赖名称
    char* name_copy = strdup(dep_name);
    if (!name_copy) {
        return -1;  // 内存分配失败
    }
    
    // 添加依赖
    deps->names[deps->count] = name_copy;
    deps->count++;
    
    return 0;
} 