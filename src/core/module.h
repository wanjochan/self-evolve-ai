/**
 * module.h - 核心模块系统
 * 
 * 这是整个系统的核心骨架，连接所有底层组件。
 * 设计理念：极简主义 + 高度灵活性 + 自我进化能力
 */

#ifndef MODULE_H
#define MODULE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 系统架构设计 (根据PRD.md)
 * 
 * Layer 1: Loader - 架构特定加载器，负责加载Layer 2
 * Layer 2: Runtime - 原生模块(.native)，提供核心功能
 * Layer 3: Program - ASTC字节码程序(.astc)，用户级程序
 * 
 * 模块系统是Layer 2的核心，负责管理和连接所有功能模块
 */

// ===============================================
// 核心模块状态
// ===============================================
typedef enum {
    MODULE_UNLOADED = 0,    // 已注册但未加载
    MODULE_LOADING = 1,     // 正在加载中
    MODULE_READY = 2,       // 已加载完成，可以使用
    MODULE_ERROR = 3        // 加载过程中出错
} ModuleState;

// ===============================================
// 模块接口 - 核心抽象
// ===============================================
typedef struct Module {
    // 基本信息
    const char* name;           // 模块名称
    void* handle;               // 模块句柄（实现特定）
    ModuleState state;          // 当前状态
    const char* error;          // 最后错误信息
    
    // 核心功能
    int (*load)(void);          // 加载并初始化模块
    void (*unload)(void);       // 卸载并清理模块
    void* (*resolve)(const char* symbol);  // 解析符号
    
    // 可选回调
    void (*on_init)(void);      // 成功加载后调用
    void (*on_exit)(void);      // 卸载前调用
    void (*on_error)(const char* msg);  // 错误时调用
} Module;

// ===============================================
// 模块注册和依赖声明宏
// ===============================================

/**
 * 注册模块 - 使用构造函数确保模块在main()之前注册
 * 
 * 用法: REGISTER_MODULE(module_name)
 */
#define REGISTER_MODULE(name) \
    __attribute__((constructor)) \
    static void _register_##name(void) { \
        module_register(&module_##name); \
    }

/**
 * 声明模块依赖 - 确保依赖模块在本模块之前加载
 * 
 * 用法: MODULE_DEPENDS_ON("memory", "utils", ...)
 */
#define MODULE_DEPENDS_ON(...) \
    static const char* _module_deps[] = { \
        __VA_ARGS__, \
        NULL \
    }; \
    __attribute__((constructor)) \
    static void _register_deps(void) { \
        extern Module* module_get(const char*); \
        extern int module_register_dependencies(Module*, const char**); \
        Module* self = module_get(MODULE_NAME); \
        if (self) { \
            module_register_dependencies(self, _module_deps); \
        } \
    }

// ===============================================
// 核心API - 模块系统基础
// ===============================================

/**
 * 初始化模块系统
 * 
 * 返回: 0表示成功，非0表示失败
 */
int module_init(void);

/**
 * 清理模块系统
 */
void module_cleanup(void);

/**
 * 注册模块
 * 
 * @param module 要注册的模块
 * @return 0表示成功，非0表示失败
 */
int module_register(Module* module);

/**
 * 加载模块
 * 
 * @param name 模块名称
 * @return 加载的模块，如果失败返回NULL
 */
Module* module_load(const char* name);

/**
 * 卸载模块
 * 
 * @param module 要卸载的模块
 */
void module_unload(Module* module);

/**
 * 从特定模块解析符号
 * 
 * @param module 模块
 * @param symbol 符号名称
 * @return 符号地址，如果未找到返回NULL
 */
void* module_resolve(Module* module, const char* symbol);

/**
 * 从任何已加载模块解析符号
 * 
 * @param symbol 符号名称
 * @return 符号地址，如果未找到返回NULL
 */
void* module_resolve_global(const char* symbol);

/**
 * 获取模块
 * 
 * @param name 模块名称
 * @return 模块，如果未找到返回NULL
 */
Module* module_get(const char* name);

// ===============================================
// 依赖管理API
// ===============================================

/**
 * 注册单个依赖
 * 
 * @param module 模块
 * @param dependency 依赖模块名称
 * @return 0表示成功，非0表示失败
 */
int module_register_dependency(Module* module, const char* dependency);

/**
 * 注册多个依赖
 * 
 * @param module 模块
 * @param dependencies 依赖模块名称数组，以NULL结尾
 * @return 0表示成功，非0表示失败
 */
int module_register_dependencies(Module* module, const char** dependencies);

/**
 * 获取模块依赖
 * 
 * @param module 模块
 * @return 依赖模块名称数组，以NULL结尾
 */
const char** module_get_dependencies(const Module* module);

// ===============================================
// 工具函数
// ===============================================

/**
 * 获取模块最后错误
 * 
 * @param module 模块
 * @return 错误信息
 */
const char* module_get_error(const Module* module);

/**
 * 获取模块状态
 * 
 * @param module 模块
 * @return 模块状态
 */
ModuleState module_get_state(const Module* module);

/**
 * 检查模块是否已加载
 * 
 * @param module 模块
 * @return true表示已加载，false表示未加载
 */
bool module_is_loaded(const Module* module);

// ===============================================
// 内置模块声明
// ===============================================

/**
 * 内置核心模块（暂定，还要优化调整的）:
 * 
 * memory: 内存管理模块
 * utils: 工具函数模块
 * native: 原生模块管理
 * astc: ASTC字节码定义和工具
 * vm: 虚拟机模块，加载ASTC字节码
 * jit: JIT编译器，将ASTC转为原生代码
 * c2astc: C到ASTC转换
 * astc2native: ASTC到原生模块转换
 * codegen: 代码生成辅助
 * std: 标准库函数
 * libc: C标准库封装
 */

// 每个模块应该在其实现文件中定义自己的Module实例
// 例如: Module module_memory = { "memory", ... };

// 模块使用示例:
// 
// 1. 加载模块:
//    Module* memory = module_load("memory");
// 
// 2. 解析符号:
//    void* (*alloc)(size_t) = module_resolve(memory, "memory_alloc");
// 
// 3. 使用符号:
//    void* ptr = alloc(1024);
//
// 4. 全局解析:
//    void* (*alloc)(size_t) = module_resolve_global("memory_alloc");

#ifdef __cplusplus
}
#endif

#endif // MODULE_H 