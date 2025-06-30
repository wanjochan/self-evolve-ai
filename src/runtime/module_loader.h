/**
 * module_loader.h - .rt模块动态加载器头文件
 * 
 * 定义模块加载和符号解析的接口
 */

#ifndef MODULE_LOADER_H
#define MODULE_LOADER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// 常量定义
// ===============================================

#define MAX_MODULE_NAME_LEN 64
#define MAX_MODULE_PATH_LEN 256
#define MAX_SYMBOL_NAME_LEN 64
#define MAX_SEARCH_PATHS 16

// ===============================================
// 模块类型和状态
// ===============================================

typedef enum {
    MODULE_TYPE_UNKNOWN = 0,
    MODULE_TYPE_RUNTIME = 1,        // 运行时模块 (vm_*.native)
    MODULE_TYPE_LIBC = 2,           // 标准库模块 (libc_*.native)
    MODULE_TYPE_USER = 3,           // 用户模块
    MODULE_TYPE_SYSTEM = 4          // 系统模块
} ModuleType;

typedef enum {
    MODULE_STATE_UNLOADED = 0,
    MODULE_STATE_LOADING = 1,
    MODULE_STATE_LOADED = 2,
    MODULE_STATE_INITIALIZED = 3,
    MODULE_STATE_ERROR = 4
} ModuleState;

// ===============================================
// 符号定义
// ===============================================

typedef enum {
    SYMBOL_TYPE_FUNCTION = 0,
    SYMBOL_TYPE_VARIABLE = 1,
    SYMBOL_TYPE_CONSTANT = 2,
    SYMBOL_TYPE_TYPE = 3
} SymbolType;

typedef struct {
    char name[MAX_SYMBOL_NAME_LEN];
    SymbolType type;
    void* address;
    uint32_t size;
    uint32_t flags;
} Symbol;

// ===============================================
// 模块导入导出
// ===============================================

typedef struct {
    char name[MAX_SYMBOL_NAME_LEN];
    uint32_t function_id;
    void* function_ptr;
    uint32_t param_count;
    uint32_t return_type;
    bool is_variadic;
} ModuleExport;

typedef struct {
    char module_name[MAX_MODULE_NAME_LEN];
    char function_name[MAX_SYMBOL_NAME_LEN];
    uint32_t local_id;
    void* resolved_ptr;
    bool is_resolved;
} ModuleImport;

// ===============================================
// 模块结构
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
    bool is_resident;
    uint32_t reference_count;
    uint64_t load_time;
    
    // 模块特定数据
    union {
        void* libc_rt_module;
        void* runtime_module;
        void* user_data;
    };
} Module;

// ===============================================
// 模块加载器API
// ===============================================

/**
 * 初始化模块加载器
 *
 * @param verbose 是否启用详细输出
 * @return 0成功，-1失败
 */
int module_loader_init(bool verbose);

/**
 * 清理模块加载器
 */
void module_loader_cleanup(void);

/**
 * 添加模块搜索路径
 *
 * @param path 搜索路径
 * @return 0成功，-1失败
 */
int module_loader_add_search_path(const char* path);

/**
 * 加载模块
 *
 * @param module_name 模块名称
 * @return 模块指针，失败返回NULL
 */
Module* module_load(const char* module_name);

/**
 * 加载.rt模块文件
 *
 * @param filename .rt文件路径
 * @return 模块指针，失败返回NULL
 */
Module* module_load_rt(const char* filename);

/**
 * 卸载模块
 *
 * @param module 模块指针
 */
void module_unload(Module* module);

/**
 * 通过名称查找模块
 *
 * @param name 模块名称
 * @return 模块指针，未找到返回NULL
 */
Module* module_find_by_name(const char* name);

/**
 * 列出已加载的模块
 */
void module_list_loaded(void);

// ===============================================
// 符号解析API
// ===============================================

/**
 * 在模块中查找符号
 *
 * @param module 模块指针
 * @param symbol_name 符号名称
 * @return 符号地址，未找到返回NULL
 */
void* module_find_symbol(Module* module, const char* symbol_name);

/**
 * 解析模块的所有导入符号
 *
 * @param module 模块指针
 * @return 0成功，-1失败
 */
int module_resolve_imports(Module* module);

/**
 * 添加模块导出符号
 *
 * @param module 模块指针
 * @param name 符号名称
 * @param function_id 函数ID
 * @param function_ptr 函数指针
 * @param param_count 参数数量
 * @param return_type 返回类型
 * @return 0成功，-1失败
 */
int module_add_export(Module* module, const char* name, uint32_t function_id,
                     void* function_ptr, uint32_t param_count, uint32_t return_type);

/**
 * 添加模块导入符号
 *
 * @param module 模块指针
 * @param module_name 依赖模块名称
 * @param function_name 函数名称
 * @param local_id 本地ID
 * @return 0成功，-1失败
 */
int module_add_import(Module* module, const char* module_name, 
                     const char* function_name, uint32_t local_id);

// ===============================================
// 模块系统API
// ===============================================

/**
 * 初始化系统模块
 *
 * @return 0成功，-1失败
 */
int module_system_init(void);

/**
 * 加载系统模块 (libc.rt, vm.rt等)
 *
 * @return 0成功，-1失败
 */
int module_system_load_core_modules(void);

/**
 * 获取模块统计信息
 *
 * @param total_modules 总模块数
 * @param loaded_modules 已加载模块数
 * @param total_memory 总内存使用
 */
void module_get_statistics(uint32_t* total_modules, uint32_t* loaded_modules, 
                          size_t* total_memory);

#ifdef __cplusplus
}
#endif

#endif // MODULE_LOADER_H
