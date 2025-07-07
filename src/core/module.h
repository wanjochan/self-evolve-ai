/**
 * module.h - 核心模块系统
 * 
 * 这是整个系统的"创世纪"，定义了什么是一个模块。
 * 设计理念：极简主义 + 高度灵活性 + 自我进化能力
 */

#ifndef MODULE_H
#define MODULE_H

// #include <stdbool.h>

#include <stdint.h>
#include <stddef.h>  // for size_t

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
// 模块接口 - 支持动态加载
// ===============================================
typedef struct Module {
    // 基本信息
    const char* name;           // 模块名称
    ModuleState state;          // 当前状态
    const char* error;          // 最后错误信息

    // 动态加载信息
    void* native_handle;        // .native文件的加载句柄
    void* base_addr;           // 内存映射基地址
    size_t file_size;          // 文件大小

    // 核心功能 (对于动态加载的模块，这些可能为NULL)
    int (*init)(void);          // 初始化模块
    void (*cleanup)(void);      // 清理模块
    void* (*resolve)(const char* symbol);  // 解析符号
} Module;

// 模块管理器模块 - 系统的第一个模块，类似上帝模块。是否一定要 extern，这里待讨论
extern Module module_module;

// ===============================================
// 动态模块加载系统
// ===============================================

/**
 * 动态加载.native模块文件 (类似Python的import)
 * 第一次加载时从文件系统读取，后续调用返回缓存的模块
 * @param name 模块名称 (如 "memory", "vm", "astc")
 * @return 加载成功的模块指针，失败返回NULL
 */
extern Module* module_load(const char* name);

/**
 * 获取已缓存的模块 (不会触发新的加载)
 * @param name 模块名称
 * @return 模块指针，未找到返回NULL
 */
extern Module* module_get(const char* name);

/**
 * 从模块解析符号
 * @param module 模块指针
 * @param symbol 符号名称
 * @return 符号地址，未找到返回NULL
 */
extern void* module_resolve(Module* module, const char* symbol);

/**
 * 卸载模块并从缓存中移除
 * @param module 模块指针
 */
extern void module_unload(Module* module);

/**
 * 初始化模块系统
 * @return 0成功，-1失败
 */
extern int module_system_init(void);

/**
 * 清理模块系统，卸载所有缓存的模块
 */
extern void module_system_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // MODULE_H