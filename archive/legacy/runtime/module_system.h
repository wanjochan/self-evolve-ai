/**
 * module_system.h - 程序级模块导入和使用系统
 * 
 * 实现程序级别的模块导入，支持libc.rt等系统模块
 */

#ifndef MODULE_SYSTEM_H
#define MODULE_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// 模块系统常量
// ===============================================

#define MAX_MODULES 64
#define MAX_MODULE_NAME_LEN 64
#define MAX_MODULE_PATH_LEN 256
#define MAX_IMPORTS 256
#define MAX_EXPORTS 256

// ===============================================
// 模块类型定义
// ===============================================

typedef enum {
    MODULE_TYPE_SYSTEM = 1,     // 系统模块 (libc.rt, runtime.rt)
    MODULE_TYPE_USER = 2,       // 用户模块
    MODULE_TYPE_LIBRARY = 3,    // 库模块
    MODULE_TYPE_PLUGIN = 4      // 插件模块
} ModuleType;

typedef enum {
    MODULE_STATE_UNLOADED = 0,
    MODULE_STATE_LOADING = 1,
    MODULE_STATE_LOADED = 2,
    MODULE_STATE_INITIALIZED = 3,
    MODULE_STATE_ERROR = 4
} ModuleState;

// ===============================================
// 模块导入/导出定义
// ===============================================

typedef struct {
    char name[MAX_MODULE_NAME_LEN];
    uint32_t function_id;
    void* function_ptr;
    uint32_t param_count;
    uint32_t return_type;
    bool is_variadic;
} ModuleExport;

typedef struct {
    char module_name[MAX_MODULE_NAME_LEN];
    char function_name[MAX_MODULE_NAME_LEN];
    uint32_t local_id;          // 本地函数ID
    void* resolved_ptr;         // 解析后的函数指针
    bool is_resolved;
} ModuleImport;

// ===============================================
// 模块描述符
// ===============================================

typedef struct Module {
    uint32_t id;
    char name[MAX_MODULE_NAME_LEN];
    char path[MAX_MODULE_PATH_LEN];
    ModuleType type;
    ModuleState state;
    
    // 版本信息
    uint32_t version_major;
    uint32_t version_minor;
    uint32_t version_patch;
    
    // 导入导出
    ModuleExport* exports;
    uint32_t export_count;
    ModuleImport* imports;
    uint32_t import_count;
    
    // 模块数据
    void* module_data;
    size_t module_size;
    void* entry_point;
    
    // 依赖关系
    uint32_t* dependencies;
    uint32_t dependency_count;
    
    // 运行时信息
    bool is_resident;           // 是否常驻内存
    uint32_t reference_count;   // 引用计数
    uint64_t load_time;         // 加载时间
    
    // 模块特定数据
    union {
        void* libc_rt_module;   // libc.rt模块数据
        void* runtime_module;   // runtime模块数据
        void* user_data;        // 用户数据
    };
} Module;

// ===============================================
// 模块系统管理器
// ===============================================

typedef struct ModuleSystem {
    Module* modules[MAX_MODULES];
    uint32_t module_count;
    
    // 系统模块
    Module* libc_module;
    Module* runtime_module;
    
    // 模块搜索路径
    char search_paths[8][MAX_MODULE_PATH_LEN];
    uint32_t search_path_count;
    
    // 配置选项
    bool auto_resolve_dependencies;
    bool lazy_loading;
    bool debug_mode;
    
    // 统计信息
    uint32_t total_loads;
    uint32_t total_unloads;
    uint32_t failed_loads;
    
    // 回调函数
    void (*on_module_loaded)(Module* module);
    void (*on_module_unloaded)(Module* module);
    void (*on_import_resolved)(ModuleImport* import);
} ModuleSystem;

// ===============================================
// 模块系统API
// ===============================================

/**
 * 初始化模块系统
 */
ModuleSystem* module_system_init(void);

/**
 * 释放模块系统
 */
void module_system_free(ModuleSystem* system);

/**
 * 加载模块
 */
Module* module_system_load_module(ModuleSystem* system, const char* name, const char* path);

/**
 * 卸载模块
 */
int module_system_unload_module(ModuleSystem* system, const char* name);

/**
 * 查找模块
 */
Module* module_system_find_module(ModuleSystem* system, const char* name);

/**
 * 解析模块依赖
 */
int module_system_resolve_dependencies(ModuleSystem* system, Module* module);

/**
 * 解析导入
 */
int module_system_resolve_imports(ModuleSystem* system, Module* module);

/**
 * 添加搜索路径
 */
int module_system_add_search_path(ModuleSystem* system, const char* path);

/**
 * 注册系统模块
 */
int module_system_register_libc(ModuleSystem* system, void* libc_rt_module);
int module_system_register_runtime(ModuleSystem* system, void* runtime_module);

// ===============================================
// 模块操作API
// ===============================================

/**
 * 创建模块
 */
Module* module_create(const char* name, ModuleType type);

/**
 * 释放模块
 */
void module_free(Module* module);

/**
 * 添加导出函数
 */
int module_add_export(Module* module, const char* name, uint32_t function_id, 
                     void* function_ptr, uint32_t param_count, uint32_t return_type);

/**
 * 添加导入函数
 */
int module_add_import(Module* module, const char* module_name, const char* function_name, 
                     uint32_t local_id);

/**
 * 查找导出函数
 */
ModuleExport* module_find_export(Module* module, const char* name);

/**
 * 获取模块信息
 */
void module_get_info(Module* module, char* info_buffer, size_t buffer_size);

// ===============================================
// 程序级模块使用API
// ===============================================

/**
 * 程序导入模块
 */
int program_import_module(ModuleSystem* system, const char* module_name);

/**
 * 程序调用模块函数
 */
int program_call_module_function(ModuleSystem* system, 
                                const char* module_name,
                                const char* function_name,
                                void* args, size_t args_size,
                                void* result, size_t result_size);

/**
 * 程序获取模块函数指针
 */
void* program_get_module_function(ModuleSystem* system,
                                 const char* module_name,
                                 const char* function_name);

// ===============================================
// 标准模块加载器
// ===============================================

/**
 * 加载libc.rt模块
 */
Module* module_load_libc_rt(ModuleSystem* system);

/**
 * 加载runtime.rt模块
 */
Module* module_load_runtime_rt(ModuleSystem* system);

/**
 * 加载用户模块
 */
Module* module_load_user_module(ModuleSystem* system, const char* path);

// ===============================================
// 模块文件格式支持
// ===============================================

/**
 * 从.rt文件加载模块
 */
Module* module_load_from_rt_file(const char* path);

/**
 * 保存模块到.rt文件
 */
int module_save_to_rt_file(Module* module, const char* path);

/**
 * 验证.rt文件格式
 */
bool module_validate_rt_file(const char* path);

// ===============================================
// 调试和诊断API
// ===============================================

/**
 * 打印模块系统状态
 */
void module_system_print_status(ModuleSystem* system);

/**
 * 打印模块信息
 */
void module_print_info(Module* module);

/**
 * 打印模块依赖图
 */
void module_system_print_dependency_graph(ModuleSystem* system);

/**
 * 检查模块完整性
 */
bool module_check_integrity(Module* module);

/**
 * 获取模块统计信息
 */
typedef struct {
    uint32_t total_modules;
    uint32_t loaded_modules;
    uint32_t system_modules;
    uint32_t user_modules;
    uint32_t total_exports;
    uint32_t total_imports;
    uint32_t resolved_imports;
    uint32_t unresolved_imports;
    size_t total_memory_usage;
} ModuleSystemStats;

void module_system_get_stats(ModuleSystem* system, ModuleSystemStats* stats);

#ifdef __cplusplus
}
#endif

#endif // MODULE_SYSTEM_H
