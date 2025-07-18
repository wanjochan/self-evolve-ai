/**
 * module_module.c - 模块系统的第一个模块
 * 
 * 作为第一个模块，它管理所有其他模块。
 * 这是系统的核心，所有模块管理功能都在这里实现。
 */

#include "../module.h"
#include "../module_loading_optimizer.h"  // T3.1 新增：性能优化器
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>  // T3.1 新增：时间测量

// Simple bool type definition to avoid stdbool.h dependency
typedef int bool;
#define true 1
#define false 0

// ===============================================
// 内部常量和配置
// ===============================================

// 最大可加载模块数
#define MAX_MODULES 128                 // 优化：从64增加到128

// 每个模块最大依赖数
#define MAX_DEPENDENCIES 32             // 优化：从16增加到32

// 符号缓存哈希表大小
#define SYMBOL_CACHE_SIZE 512           // 优化：从256增加到512

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

// 按需加载相关函数
static const char* detect_architecture_string(void);
static int detect_architecture_bits(void);
static void* module_sym_impl(Module* self, const char* symbol_name);
static Module* load_native_file_direct(const char* file_path, const char* module_path);

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

    // T3.1 性能优化：初始化模块加载优化器
    if (!module_optimizer_is_initialized()) {
        ModuleLoadingOptimizerConfig config = module_optimizer_get_default_config();
        if (module_optimizer_init(&config) == 0) {
            printf("Module: T3.1模块加载优化器已启动\n");
        } else {
            printf("Module: 警告: T3.1模块加载优化器启动失败\n");
        }
    }

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
 * 动态加载.native模块文件 (T3.1 优化版本)
 */
Module* module_load(const char* name) {
    if (!name) {
        return NULL;
    }

    // T3.1 性能优化：记录开始时间
    double start_time = 0.0;
    if (module_optimizer_is_initialized()) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        start_time = tv.tv_sec + tv.tv_usec / 1000000.0;

        // 尝试从优化器缓存加载
        void* optimized_module = module_optimizer_lookup_symbol(name);
        if (optimized_module) {
            printf("Module: 从优化器缓存返回模块 %s\n", name);
            return (Module*)optimized_module;
        }
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

        // T3.1 性能优化：缓存到优化器
        if (module_optimizer_is_initialized()) {
            module_optimizer_cache_symbol(name, name, existing);
        }

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
    void* mapped = mmap(NULL, file_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE, fd, 0);
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
    module->path = strdup(name);  // 对于传统加载，path和name相同
    module->state = MODULE_READY;
    module->error = NULL;
    module->native_handle = mapped;
    module->base_addr = mapped;
    module->file_size = file_size;
    module->init = NULL;      // 动态加载的模块不使用这些函数指针
    module->cleanup = NULL;
    module->resolve = NULL;
    module->sym = module_sym_impl;  // 设置符号解析接口

    // 添加到缓存
    if (module_cache.count < MAX_MODULES) {
        module_cache.loaded_modules[module_cache.count] = module;
        module_cache.count++;
        printf("Module: 模块 %s 已缓存 (缓存数量: %zu)\n", name, module_cache.count);
    } else {
        printf("Module: 警告: 模块缓存已满，无法缓存模块 %s\n", name);
    }

    // T3.1 性能优化：记录加载统计和缓存到优化器
    if (module_optimizer_is_initialized() && start_time > 0.0) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        double end_time = tv.tv_sec + tv.tv_usec / 1000000.0;
        double load_time = end_time - start_time;

        // 更新优化器统计
        g_module_optimizer.stats.total_loads++;
        g_module_optimizer.stats.total_load_time += load_time;
        g_module_optimizer.stats.cache_misses++;  // 这是一次缓存未命中

        // 缓存模块到优化器
        module_optimizer_cache_symbol(name, name, module);

        printf("Module: T3.1优化 - 模块 %s 加载时间: %.6f 秒\n", name, load_time);
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

    // 释放模块名称和路径
    if (module->name) {
        free((void*)module->name);
        module->name = NULL;
    }
    if (module->path) {
        free((void*)module->path);
        module->path = NULL;
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
 * 从模块解析符号
 * @param module 模块指针
 * @param symbol 符号名称
 * @return 符号地址，未找到返回NULL
 */
void* module_resolve(Module* module, const char* symbol) {
    if (!module || !symbol || module->state != MODULE_READY) {
        printf("Module: module_resolve 参数检查失败 (module: %p, symbol: %s, state: %d)\n", 
               module, symbol, module ? module->state : -1);
        return NULL;
    }

    printf("Module: 开始解析符号 %s 在模块 %s\n", symbol, module->name);

    // 检查缓存
    void* cached = find_cached_symbol(symbol);
    if (cached) {
        printf("Module: 从缓存找到符号 %s -> %p\n", symbol, cached);
        return cached;
    }

    void* addr = NULL;

    // 如果是动态加载的.native模块
    if (module->native_handle && module->base_addr) {
        printf("Module: 处理动态加载的.native模块\n");
        NativeHeader* header = (NativeHeader*)module->base_addr;
        printf("Module: 头部信息 - 导出数量: %d, 导出偏移: 0x%x\n", 
               header->export_count, header->export_offset);

        // 获取导出表
        ExportEntry* exports = (ExportEntry*)((char*)module->base_addr + header->export_offset);
        printf("Module: 导出表地址: %p\n", exports);

        // 查找符号
        for (uint32_t i = 0; i < header->export_count; i++) {
            printf("Module: 检查导出 %d: '%s' (寻找 '%s')\n", i, exports[i].name, symbol);
            if (strcmp(exports[i].name, symbol) == 0) {
                // 计算符号地址 = 基地址 + 代码段偏移 + 符号偏移
                addr = (char*)module->base_addr + header->header_size + exports[i].offset;
                printf("Module: 解析符号 %s -> %p (偏移: 0x%x)\n", symbol, addr, exports[i].offset);
                break;
            }
        }
        
        if (!addr) {
            printf("Module: 符号 %s 未在导出表中找到\n", symbol);
        }
    }
    // 如果是静态模块，使用传统方式
    else if (module->resolve) {
        printf("Module: 处理静态模块，使用传统resolve方式\n");
        addr = module->resolve(symbol);
    }

    // 缓存结果
    if (addr) {
        cache_symbol(symbol, addr);
        printf("Module: 符号 %s 已缓存\n", symbol);
    } else {
        printf("Module: 符号 %s 解析失败\n", symbol);
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

// 注册单个依赖
static int module_register_dependency(Module* module, const char* dependency) {
    if (!module || !dependency) return -1;

    // 查找模块在缓存中的索引
    size_t module_index = MAX_MODULES;
    for (size_t i = 0; i < module_cache.count; i++) {
        if (module_cache.loaded_modules[i] == module) {
            module_index = i;
            break;
        }
    }

    if (module_index >= MAX_MODULES) {
        printf("Module: 错误: 模块未在缓存中找到\n");
        return -1;
    }

    ModuleDependencies* deps = &module_cache.dependencies[module_index];

    // 检查是否已经存在该依赖
    for (size_t i = 0; i < deps->count; i++) {
        if (strcmp(deps->names[i], dependency) == 0) {
            return 0; // 已存在，返回成功
        }
    }

    // 检查依赖数量限制
    if (deps->count >= MAX_DEPENDENCIES) {
        printf("Module: 错误: 模块 %s 依赖数量超过限制 %d\n", module->name, MAX_DEPENDENCIES);
        return -1;
    }

    // 扩展依赖数组
    if (deps->names == NULL) {
        deps->names = malloc(MAX_DEPENDENCIES * sizeof(char*));
        deps->modules = malloc(MAX_DEPENDENCIES * sizeof(Module*));
        if (!deps->names || !deps->modules) {
            printf("Module: 错误: 内存分配失败\n");
            return -1;
        }
    }

    // 添加新依赖
    deps->names[deps->count] = strdup(dependency);
    deps->modules[deps->count] = NULL; // 稍后解析
    deps->count++;

    printf("Module: 为模块 %s 注册依赖: %s\n", module->name, dependency);
    return 0;
}

// 注册多个依赖
static int module_register_dependencies(Module* module, const char** dependencies) {
    if (!module || !dependencies) return -1;

    int success_count = 0;
    int total_count = 0;

    // 遍历依赖数组
    for (const char** dep = dependencies; *dep != NULL; dep++) {
        total_count++;
        if (module_register_dependency(module, *dep) == 0) {
            success_count++;
        } else {
            printf("Module: 警告: 注册依赖 %s 失败\n", *dep);
        }
    }

    printf("Module: 为模块 %s 注册了 %d/%d 个依赖\n",
           module->name, success_count, total_count);

    return (success_count == total_count) ? 0 : -1;
}

// 获取模块依赖
static const char** module_get_dependencies(const Module* module) {
    if (!module) return NULL;

    // 查找模块在缓存中的索引
    size_t module_index = MAX_MODULES;
    for (size_t i = 0; i < module_cache.count; i++) {
        if (module_cache.loaded_modules[i] == module) {
            module_index = i;
            break;
        }
    }

    if (module_index >= MAX_MODULES) {
        return NULL;
    }

    ModuleDependencies* deps = &module_cache.dependencies[module_index];

    if (deps->count == 0 || !deps->names) {
        return NULL;
    }

    // 创建以NULL结尾的依赖名称数组
    const char** result = malloc((deps->count + 1) * sizeof(char*));
    if (!result) return NULL;

    for (size_t i = 0; i < deps->count; i++) {
        result[i] = deps->names[i];
    }
    result[deps->count] = NULL;

    return result;
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

// 手动添加模块到缓存 (用于测试)
static int module_add_to_cache(Module* module) {
    if (!module) return -1;
    
    // 必要时初始化
    if (!module_cache.initialized) {
        if (module_init() != 0) {
            return -1;
        }
    }
    
    // 检查是否已经在缓存中
    for (size_t i = 0; i < module_cache.count; i++) {
        if (module_cache.loaded_modules[i] == module) {
            return 0; // 已在缓存中
        }
    }
    
    // 检查缓存容量
    if (module_cache.count >= MAX_MODULES) {
        printf("Module: 错误: 模块缓存已满\n");
        return -1;
    }
    
    // 添加到缓存
    module_cache.loaded_modules[module_cache.count] = module;
    module_cache.count++;
    
    printf("Module: 模块 %s 已手动添加到缓存 (缓存数量: %zu)\n", 
           module->name, module_cache.count);
    
    return 0;
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

// 解析依赖
static int resolve_dependencies(Module* module) {
    if (!module) return -1;

    // 查找模块在缓存中的索引
    size_t module_index = MAX_MODULES;
    for (size_t i = 0; i < module_cache.count; i++) {
        if (module_cache.loaded_modules[i] == module) {
            module_index = i;
            break;
        }
    }

    if (module_index >= MAX_MODULES) {
        printf("Module: 错误: 模块未在缓存中找到\n");
        return -1;
    }

    ModuleDependencies* deps = &module_cache.dependencies[module_index];

    if (deps->count == 0) {
        return 0; // 无依赖，成功
    }

    printf("Module: 解析模块 %s 的 %zu 个依赖\n", module->name, deps->count);

    int resolved_count = 0;
    for (size_t i = 0; i < deps->count; i++) {
        const char* dep_name = deps->names[i];

        // 尝试查找已加载的依赖模块
        Module* dep_module = find_loaded_module(dep_name);

        if (!dep_module) {
            // 尝试动态加载依赖模块
            printf("Module: 尝试加载依赖模块: %s\n", dep_name);
            dep_module = load_module(dep_name);
        }

        if (dep_module) {
            deps->modules[i] = dep_module;
            resolved_count++;
            printf("Module: 依赖 %s 解析成功\n", dep_name);
        } else {
            printf("Module: 警告: 无法解析依赖 %s\n", dep_name);
        }
    }

    printf("Module: 模块 %s 成功解析了 %d/%zu 个依赖\n",
           module->name, resolved_count, deps->count);

    return (resolved_count == (int)deps->count) ? 0 : -1;
}

// 符号哈希函数
// 优化：使用更好的哈希函数 (djb2算法)
static unsigned char symbol_hash(const char* symbol) {
    uint32_t hash = 5381;
    for (const char* p = symbol; *p; p++) {
        hash = ((hash << 5) + hash) + *p; // hash * 33 + c
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

// 注册依赖 (通过模块索引)
static int register_dependency(size_t module_index, const char* dep_name) {
    if (module_index >= MAX_MODULES || !dep_name) return -1;

    if (module_index >= module_cache.count) {
        printf("Module: 错误: 模块索引 %zu 超出范围\n", module_index);
        return -1;
    }

    Module* module = module_cache.loaded_modules[module_index];
    if (!module) {
        printf("Module: 错误: 模块索引 %zu 对应的模块为空\n", module_index);
        return -1;
    }

    return module_register_dependency(module, dep_name);
}

// ===============================================
// 按需加载功能实现
// ===============================================

/**
 * 检测当前架构
 */
static const char* detect_architecture_string(void) {
#if defined(__x86_64__) || defined(_M_X64)
    return "x64";
#elif defined(__aarch64__) || defined(_M_ARM64)
    return "arm64";
#elif defined(__i386__) || defined(_M_IX86)
    return "x86";
#elif defined(__arm__) || defined(_M_ARM)
    return "arm";
#else
    return "unknown";
#endif
}

/**
 * 检测当前架构位数
 */
static int detect_architecture_bits(void) {
#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__) || defined(_M_ARM64)
    return 64;
#else
    return 32;
#endif
}

/**
 * 智能路径解析 - 自动添加架构后缀
 */
char* resolve_native_file(const char* module_path) {
    if (!module_path) {
        return NULL;
    }

    const char* arch = detect_architecture_string();
    int bits = detect_architecture_bits();
    
    // 计算需要的内存大小
    size_t path_len = strlen(module_path);
    size_t arch_len = strlen(arch);
    size_t suffix_len = 32; // 足够容纳 "_x64_64.native\0"
    
    char* resolved = malloc(path_len + arch_len + suffix_len);
    if (!resolved) {
        return NULL;
    }
    
    // 构建完整路径: "./module" -> "./module_x64_64.native"
    snprintf(resolved, path_len + arch_len + suffix_len, 
             "%s_%s_%d.native", module_path, arch, bits);
    
    return resolved;
}

/**
 * 模块符号解析包装函数 - 用于函数指针
 */
static Module* current_resolving_module = NULL;

static void* module_resolve_wrapper(const char* symbol_name) {
    // 使用全局的当前解析模块
    if (current_resolving_module) {
        return module_resolve(current_resolving_module, symbol_name);
    }
    
    // 如果没有当前模块，尝试全局解析
    return module_resolve_global(symbol_name);
}

// 为每个模块创建专用的resolve函数的结构
typedef struct {
    Module* module;
    void* (*resolve_func)(const char*);
} ModuleResolveWrapper;

// 存储模块resolve包装器
static ModuleResolveWrapper module_wrappers[MAX_MODULES];
static size_t wrapper_count = 0;

/**
 * 为特定模块创建resolve包装器的实现
 */
static void* module_resolve_impl_0(const char* symbol) { return module_resolve(module_wrappers[0].module, symbol); }
static void* module_resolve_impl_1(const char* symbol) { return module_resolve(module_wrappers[1].module, symbol); }
static void* module_resolve_impl_2(const char* symbol) { return module_resolve(module_wrappers[2].module, symbol); }
static void* module_resolve_impl_3(const char* symbol) { return module_resolve(module_wrappers[3].module, symbol); }
static void* module_resolve_impl_4(const char* symbol) { return module_resolve(module_wrappers[4].module, symbol); }
static void* module_resolve_impl_5(const char* symbol) { return module_resolve(module_wrappers[5].module, symbol); }
static void* module_resolve_impl_6(const char* symbol) { return module_resolve(module_wrappers[6].module, symbol); }
static void* module_resolve_impl_7(const char* symbol) { return module_resolve(module_wrappers[7].module, symbol); }

static void* (*resolve_impls[])(const char*) = {
    module_resolve_impl_0, module_resolve_impl_1, module_resolve_impl_2, module_resolve_impl_3,
    module_resolve_impl_4, module_resolve_impl_5, module_resolve_impl_6, module_resolve_impl_7
};

/**
 * 为特定模块创建resolve包装器
 */
static void* (*create_module_resolve_wrapper(Module* module))(const char*) {
    if (wrapper_count >= MAX_MODULES || wrapper_count >= 8) {
        return NULL; // 太多模块了
    }
    
    module_wrappers[wrapper_count].module = module;
    module_wrappers[wrapper_count].resolve_func = resolve_impls[wrapper_count];
    
    return resolve_impls[wrapper_count++];
}

/**
 * 模块符号解析接口实现
 */
static void* module_sym_impl(Module* self, const char* symbol_name) {
    if (!self || !symbol_name) {
        return NULL;
    }
    
    return module_resolve(self, symbol_name);
}

/**
 * 按需加载模块 - 优雅的加载接口
 */
Module* load_module(const char* path) {
    if (!path) {
        return NULL;
    }
    
    // 解析完整的.native文件路径
    char* native_file = resolve_native_file(path);
    if (!native_file) {
        return NULL;
    }
    
    printf("Module: 按需加载 %s -> %s\n", path, native_file);
    
    // 检查是否已经加载过（基于路径）
    Module* existing = NULL;
    for (size_t i = 0; i < module_cache.count; i++) {
        Module* module = module_cache.loaded_modules[i];
        if (module && module->path && strcmp(module->path, path) == 0) {
            existing = module;
            break;
        }
    }
    
    if (existing && existing->state == MODULE_READY) {
        printf("Module: 从缓存返回模块 %s\n", path);
        free(native_file);
        return existing;
    }
    
    // 使用现有的module_load加载.native文件
    // 但我们需要直接加载文件而不是通过name查找
    Module* module = load_native_file_direct(native_file, path);
    
    free(native_file);
    return module;
}

/**
 * 直接加载.native文件（内部函数）
 */
static Module* load_native_file_direct(const char* file_path, const char* module_path) {
    if (!file_path || !module_path) {
        return NULL;
    }
    
    // 必要时初始化
    if (!module_cache.initialized) {
        if (module_init() != 0) {
            return NULL;
        }
    }
    
    printf("Module: 直接加载 %s\n", file_path);
    
    // 打开文件
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        printf("Module: 警告: 无法打开模块文件 %s: %s\n", file_path, strerror(errno));
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
    void* mapped = mmap(NULL, file_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE, fd, 0);
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
    
    printf("Module: 成功加载 %s, 导出函数数量: %d\n", module_path, header->export_count);
    
    // 创建模块对象
    Module* module = malloc(sizeof(Module));
    if (!module) {
        munmap(mapped, file_size);
        return NULL;
    }
    
    // 初始化模块信息
    module->name = strdup(module_path);
    module->path = strdup(module_path);
    module->state = MODULE_READY;
    module->error = NULL;
    module->native_handle = mapped;
    module->base_addr = mapped;
    module->file_size = file_size;
    module->init = NULL;
    module->cleanup = NULL;
    module->resolve = create_module_resolve_wrapper(module);  // 为这个模块创建专用的resolve包装器
    module->sym = module_sym_impl;  // 设置符号解析接口
    
    // 添加到缓存
    if (module_cache.count < MAX_MODULES) {
        module_cache.loaded_modules[module_cache.count] = module;
        module_cache.count++;
        printf("Module: 模块 %s 已缓存 (缓存数量: %zu)\n", module_path, module_cache.count);
    } else {
        printf("Module: 警告: 模块缓存已满，无法缓存模块 %s\n", module_path);
    }
    
    return module;
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
    {"resolve_native_file", resolve_native_file},
    {"load_module", load_module},
    
    // 测试辅助函数
    {"module_add_to_cache", module_add_to_cache},

    // 依赖管理接口
    {"module_register_dependency", module_register_dependency},
    {"module_register_dependencies", module_register_dependencies},
    {"module_get_dependencies", module_get_dependencies},
    {"resolve_dependencies", resolve_dependencies},
    {"register_dependency", register_dependency},

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
    .path = "module",
    .state = MODULE_UNLOADED,
    .error = NULL,
    .native_handle = NULL,
    .base_addr = NULL,
    .file_size = 0,
    .init = module_init,
    .cleanup = module_cleanup,
    .resolve = module_module_resolve,
    .sym = module_sym_impl
};