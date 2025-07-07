/**
 * module_module.c - 模块系统的第一个模块
 * 
 * 作为第一个模块，它管理所有其他模块。
 * 这是系统的核心，所有模块管理功能都在这里实现。
 */

#include "../module.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

// Simple bool type definition to avoid stdbool.h dependency
typedef int bool;
#define true 1
#define false 0

// ===============================================
// 内部常量和配置
// ===============================================

// 最大可加载模块数
#define MAX_MODULES 64

// 每个模块最大依赖数
#define MAX_DEPENDENCIES 16

// 符号缓存哈希表大小
#define SYMBOL_CACHE_SIZE 256

// ===============================================
// .native文件格式定义
// ===============================================

// Native module format (与simple_loader兼容)
typedef struct {
    char magic[4];          // "NATV"
    uint32_t version;       // 版本号
    uint32_t arch;          // 架构类型
    uint32_t module_type;   // 模块类型
    uint32_t flags;         // 标志
    uint32_t header_size;   // 头部大小
    uint32_t code_size;     // 代码大小
    uint32_t data_size;     // 数据大小
    uint32_t export_count;  // 导出函数数量
    uint32_t export_offset; // 导出表偏移
    uint32_t reserved[6];   // 保留字段
} NativeHeader;

typedef struct {
    char name[64];          // 函数名
    uint32_t offset;        // 函数偏移
    uint32_t size;          // 函数大小（可选）
    uint32_t flags;         // 标志
    uint32_t reserved;      // 保留
} ExportEntry;

// 动态加载的模块信息
typedef struct {
    void* base_addr;        // mmap映射的基地址
    size_t file_size;       // 文件大小
    NativeHeader* header;   // 头部指针
    ExportEntry* exports;   // 导出表指针
    void* code_section;     // 代码段指针
    char file_path[256];    // 文件路径
} LoadedNativeModule;

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

// 动态模块缓存 (不是"注册表"，而是"缓存")
static struct {
    Module* loaded_modules[MAX_MODULES];             // 已加载模块缓存
    size_t count;                                    // 已加载模块数量
    bool initialized;                                // 是否已初始化
    SymbolCacheEntry* symbol_cache[SYMBOL_CACHE_SIZE]; // 符号缓存哈希表
    ModuleDependencies dependencies[MAX_MODULES];    // 每个模块的依赖信息
} module_cache = {
    .loaded_modules = {NULL},
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
static Module* find_loaded_module(const char* name);

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
// 模块API实现
// ===============================================

// 模块系统初始化
static int module_init(void) {
    if (module_cache.initialized) {
        return 0;  // 已初始化
    }

    // 清除符号缓存
    memset(module_cache.symbol_cache, 0, sizeof(module_cache.symbol_cache));

    // 初始化依赖信息
    for (size_t i = 0; i < MAX_MODULES; i++) {
        module_cache.dependencies[i].names = NULL;
        module_cache.dependencies[i].modules = NULL;
        module_cache.dependencies[i].count = 0;
    }

    // 将自己作为第一个缓存的模块 (module_module是静态的，不是动态加载的)
    module_cache.loaded_modules[0] = &module_module;
    module_cache.count = 1;
    module_cache.initialized = true;

    // 设置自己的状态为已就绪
    module_module.state = MODULE_READY;

    return 0;
}

// 模块系统清理
static void module_cleanup(void) {
    if (!module_cache.initialized) {
        return;  // 未初始化
    }

    // 按照相反顺序卸载所有缓存的模块（跳过自己）
    for (int i = (int)module_cache.count - 1; i > 0; i--) {
        Module* module = module_cache.loaded_modules[i];
        if (module && module->state == MODULE_READY) {
            // 对于动态加载的模块，直接卸载
            if (module->native_handle) {
                module_unload(module);
            }
            // 对于静态模块，调用cleanup函数
            else if (module->cleanup) {
                module->cleanup();
                module->state = MODULE_UNLOADED;
            }
        }
    }

    // 清除符号缓存
    clear_symbol_cache();

    // 释放依赖信息
    for (size_t i = 0; i < MAX_MODULES; i++) {
        if (module_cache.dependencies[i].names) {
            free(module_cache.dependencies[i].names);
            module_cache.dependencies[i].names = NULL;
        }
        if (module_cache.dependencies[i].modules) {
            free(module_cache.dependencies[i].modules);
            module_cache.dependencies[i].modules = NULL;
        }
        module_cache.dependencies[i].count = 0;
    }

    // 重置缓存但保持初始化状态
    module_cache.count = 1;  // 保留module_module自己
    module_cache.initialized = true;
}

// ===============================================
// 动态模块加载API (无需注册，纯缓存机制)
// ===============================================

/**
 * 动态加载.native模块文件
 */
Module* module_load(const char* name) {
    if (!name) {
        return NULL;
    }

    // 必要时初始化
    if (!module_cache.initialized) {
        if (module_init() != 0) {
            return NULL;
        }
    }

    // 检查缓存中是否已加载
    Module* existing = find_loaded_module(name);
    if (existing && existing->state == MODULE_READY) {
        printf("Module: 从缓存返回模块 %s\n", name);
        return existing;
    }

    // 构建.native文件路径
    char module_path[512];
    snprintf(module_path, sizeof(module_path), "bin/layer2/%s.native", name);

    printf("Module: 尝试加载 %s 从 %s\n", name, module_path);

    // 打开文件
    int fd = open(module_path, O_RDONLY);
    if (fd == -1) {
        printf("Module: 警告: 无法打开模块文件 %s: %s\n", module_path, strerror(errno));
        return NULL;
    }

    // 获取文件大小
    struct stat st;
    if (fstat(fd, &st) == -1) {
        printf("Module: 警告: 获取文件大小失败: %s\n", strerror(errno));
        close(fd);
        return NULL;
    }

    size_t file_size = st.st_size;

    // 映射文件到内存
    void* mapped = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    close(fd);

    if (mapped == MAP_FAILED) {
        printf("Module: 警告: 内存映射失败: %s\n", strerror(errno));
        return NULL;
    }

    // 验证.native文件格式
    NativeHeader* header = (NativeHeader*)mapped;
    if (memcmp(header->magic, "NATV", 4) != 0) {
        printf("Module: 警告: 无效的模块格式 (magic: %.4s)\n", header->magic);
        munmap(mapped, file_size);
        return NULL;
    }

    printf("Module: 成功加载 %s, 导出函数数量: %d\n", name, header->export_count);

    // 创建模块对象
    Module* module = malloc(sizeof(Module));
    if (!module) {
        munmap(mapped, file_size);
        return NULL;
    }

    // 初始化模块信息
    module->name = strdup(name);
    module->state = MODULE_READY;
    module->error = NULL;
    module->native_handle = mapped;
    module->base_addr = mapped;
    module->file_size = file_size;
    module->init = NULL;      // 动态加载的模块不使用这些函数指针
    module->cleanup = NULL;
    module->resolve = NULL;

    // 添加到缓存
    if (module_cache.count < MAX_MODULES) {
        module_cache.loaded_modules[module_cache.count] = module;
        module_cache.count++;
        printf("Module: 模块 %s 已缓存 (缓存数量: %zu)\n", name, module_cache.count);
    } else {
        printf("Module: 警告: 模块缓存已满，无法缓存模块 %s\n", name);
    }

    return module;
}

/**
 * 卸载动态加载的模块
 */
void module_unload(Module* module) {
    if (!module || module->state != MODULE_READY) {
        return;
    }

    // 不允许卸载自己
    if (module == &module_module) {
        return;
    }

    printf("Module: 卸载模块 %s\n", module->name);

    // 如果是动态加载的模块，释放内存映射
    if (module->native_handle && module->base_addr) {
        munmap(module->base_addr, module->file_size);
        module->native_handle = NULL;
        module->base_addr = NULL;
        module->file_size = 0;
    }
    // 如果是静态模块，调用cleanup函数
    else if (module->cleanup) {
        module->cleanup();
    }

    module->state = MODULE_UNLOADED;

    // 清除相关符号缓存
    clear_symbol_cache();

    // 释放模块名称
    if (module->name) {
        free((void*)module->name);
        module->name = NULL;
    }

    // 从缓存中移除
    for (size_t i = 0; i < module_cache.count; i++) {
        if (module_cache.loaded_modules[i] == module) {
            // 移动后续模块
            for (size_t j = i; j < module_cache.count - 1; j++) {
                module_cache.loaded_modules[j] = module_cache.loaded_modules[j + 1];
            }
            module_cache.count--;
            break;
        }
    }

    // 释放模块对象
    free(module);
}

/**
 * 从.native模块解析符号
 */
void* module_resolve(Module* module, const char* symbol) {
    if (!module || !symbol || module->state != MODULE_READY) {
        return NULL;
    }

    // 检查缓存
    void* cached = find_cached_symbol(symbol);
    if (cached) {
        return cached;
    }

    void* addr = NULL;

    // 如果是动态加载的.native模块
    if (module->native_handle && module->base_addr) {
        NativeHeader* header = (NativeHeader*)module->base_addr;

        // 获取导出表
        ExportEntry* exports = (ExportEntry*)((char*)module->base_addr + header->export_offset);

        // 查找符号
        for (uint32_t i = 0; i < header->export_count; i++) {
            if (strcmp(exports[i].name, symbol) == 0) {
                // 计算符号地址 = 基地址 + 代码段偏移 + 符号偏移
                addr = (char*)module->base_addr + header->header_size + exports[i].offset;
                printf("Module: 解析符号 %s -> %p (偏移: 0x%x)\n", symbol, addr, exports[i].offset);
                break;
            }
        }
    }
    // 如果是静态模块，使用传统方式
    else if (module->resolve) {
        addr = module->resolve(symbol);
    }

    // 缓存结果
    if (addr) {
        cache_symbol(symbol, addr);
    }

    return addr;
}

// 从任何已加载模块解析符号
static void* module_resolve_global(const char* symbol) {
    if (!symbol) {
        return NULL;
    }
    
    // 检查缓存
    void* cached = find_cached_symbol(symbol);
    if (cached) {
        return cached;
    }
    
    // 遍历所有已缓存的模块
    for (size_t i = 0; i < module_cache.count; i++) {
        Module* module = module_cache.loaded_modules[i];
        if (module && module->state == MODULE_READY && module->resolve) {
            void* addr = module->resolve(symbol);
            if (addr) {
                // 缓存结果
                cache_symbol(symbol, addr);
                return addr;
            }
        }
    }
    
    return NULL;
}

// 获取模块
Module* module_get(const char* name) {
    return find_module(name);
}

// 注册单个依赖 (暂时禁用，需要重构为缓存机制)
static int module_register_dependency(Module* module, const char* dependency) {
    // TODO: 重构为缓存机制
    return 0;  // 暂时返回成功
}

// 注册多个依赖 (暂时禁用，需要重构为缓存机制)
static int module_register_dependencies(Module* module, const char** dependencies) {
    // TODO: 重构为缓存机制
    return 0;  // 暂时返回成功
}

// 获取模块依赖 (暂时禁用，需要重构为缓存机制)
static const char** module_get_dependencies(const Module* module) {
    // TODO: 重构为缓存机制
    return NULL;  // 暂时返回NULL
}

// 获取模块状态
static ModuleState module_get_state(const Module* module) {
    return module ? module->state : MODULE_ERROR;
}

// 检查模块是否已加载
static bool module_is_loaded(const Module* module) {
    return module && module->state == MODULE_READY;
}

// 获取模块错误信息
static const char* module_get_error(const Module* module) {
    return module ? module->error : "Invalid module";
}

// ===============================================
// 内部辅助函数实现
// ===============================================

// 在缓存中查找已加载的模块
static Module* find_loaded_module(const char* name) {
    if (!name) {
        return NULL;
    }

    for (size_t i = 0; i < module_cache.count; i++) {
        if (module_cache.loaded_modules[i] &&
            strcmp(module_cache.loaded_modules[i]->name, name) == 0) {
            return module_cache.loaded_modules[i];
        }
    }

    return NULL;
}

// 兼容性函数 (为了不破坏现有代码)
static Module* find_module(const char* name) {
    return find_loaded_module(name);
}

// 解析依赖 (暂时禁用，需要重构为缓存机制)
static int resolve_dependencies(Module* module) {
    // TODO: 重构为缓存机制
    return 0;  // 暂时返回成功
}

// 符号哈希函数
static unsigned char symbol_hash(const char* symbol) {
    unsigned char hash = 0;
    for (const char* p = symbol; *p; p++) {
        hash = hash * 31 + *p;
    }
    return hash % SYMBOL_CACHE_SIZE;
}

// 查找缓存的符号
static void* find_cached_symbol(const char* symbol) {
    if (!symbol) {
        return NULL;
    }
    
    unsigned char hash = symbol_hash(symbol);
    SymbolCacheEntry* entry = module_cache.symbol_cache[hash];
    
    while (entry) {
        if (strcmp(entry->symbol, symbol) == 0) {
            return entry->address;
        }
        entry = entry->next;
    }
    
    return NULL;
}

// 缓存符号
static void cache_symbol(const char* symbol, void* address) {
    if (!symbol || !address) {
        return;
    }
    
    unsigned char hash = symbol_hash(symbol);
    
    // 检查是否已存在
    SymbolCacheEntry* entry = module_cache.symbol_cache[hash];
    while (entry) {
        if (strcmp(entry->symbol, symbol) == 0) {
            entry->address = address;  // 更新地址
            return;
        }
        entry = entry->next;
    }
    
    // 创建新条目
    entry = malloc(sizeof(SymbolCacheEntry));
    if (!entry) {
        return;  // 内存分配失败
    }
    
    entry->symbol = strdup(symbol);
    if (!entry->symbol) {
        free(entry);
        return;  // 内存分配失败
    }
    
    entry->address = address;
    entry->next = module_cache.symbol_cache[hash];
    module_cache.symbol_cache[hash] = entry;
}

// 清除符号缓存
static void clear_symbol_cache(void) {
    for (size_t i = 0; i < SYMBOL_CACHE_SIZE; i++) {
        SymbolCacheEntry* entry = module_cache.symbol_cache[i];
        while (entry) {
            SymbolCacheEntry* next = entry->next;
            free((void*)entry->symbol);
            free(entry);
            entry = next;
        }
        module_cache.symbol_cache[i] = NULL;
    }
}

// 注册依赖 (暂时禁用，需要重构为缓存机制)
static int register_dependency(size_t module_index, const char* dep_name) {
    // TODO: 重构为缓存机制
    return 0;  // 暂时返回成功
}

// ===============================================
// 对外暴露的符号表
// ===============================================

static struct {
    const char* name;
    void* symbol;
} module_symbols[] = {
    {"module_load", module_load},
    {"module_unload", module_unload},
    {"module_resolve", module_resolve},
    {"module_resolve_global", module_resolve_global},
    {"module_get", module_get},
    {"module_get_state", module_get_state},
    {"module_is_loaded", module_is_loaded},
    {"module_get_error", module_get_error},
    {NULL, NULL}
};

// 符号解析函数
static void* module_module_resolve(const char* symbol) {
    if (!symbol) {
        return NULL;
    }
    
    // 查找符号
    for (size_t i = 0; module_symbols[i].name; i++) {
        if (strcmp(module_symbols[i].name, symbol) == 0) {
            return module_symbols[i].symbol;
        }
    }
    
    return NULL;
}

// ===============================================
// 全局模块管理API
// ===============================================

/**
 * 初始化模块系统
 */
int module_system_init(void) {
    return module_init();
}

/**
 * 清理模块系统
 */
void module_system_cleanup(void) {
    // 卸载所有动态加载的模块（除了module_module自己）
    for (int i = (int)module_cache.count - 1; i > 0; i--) {
        Module* module = module_cache.loaded_modules[i];
        if (module && module->state == MODULE_READY) {
            module_unload(module);
        }
    }

    // 最后清理模块系统本身
    module_cleanup();
}

// ===============================================
// 模块定义 - 第一个模块
// ===============================================

// 模块管理器模块定义
Module module_module = {
    .name = "module",
    .state = MODULE_UNLOADED,
    .error = NULL,
    .init = module_init,
    .cleanup = module_cleanup,
    .resolve = module_module_resolve
};