/**
 * libc_rt_module.h - libc.rt模块化接口定义
 * 
 * 实现PRD.md要求的libc.rt模块分离架构
 */

#ifndef LIBC_RT_MODULE_H
#define LIBC_RT_MODULE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// 模块化常量定义
// ===============================================

#define LIBC_RT_MAGIC "LBRT"
#define LIBC_RT_VERSION 1
#define MAX_LIBC_FUNCTIONS 256
#define MAX_FUNCTION_NAME_LEN 64

// ===============================================
// libc.rt模块头部结构
// ===============================================

typedef struct {
    char magic[4];              // "LBRT"
    uint32_t version;           // 版本号
    uint32_t function_count;    // 导出函数数量
    uint32_t symbol_table_offset; // 符号表偏移
    uint32_t code_section_offset; // 代码段偏移
    uint32_t data_section_offset; // 数据段偏移
    uint32_t total_size;        // 模块总大小
    uint32_t checksum;          // 校验和
} LibcRtHeader;

// ===============================================
// 函数符号表项
// ===============================================

typedef struct {
    char name[MAX_FUNCTION_NAME_LEN]; // 函数名
    uint32_t function_id;             // 函数ID
    uint32_t code_offset;             // 代码偏移
    uint32_t code_size;               // 代码大小
    uint32_t param_count;             // 参数数量
    uint32_t return_type;             // 返回类型
    uint32_t flags;                   // 函数标志
} LibcFunctionSymbol;

// ===============================================
// libc.rt模块接口
// ===============================================

typedef struct LibcRtModule {
    LibcRtHeader header;
    LibcFunctionSymbol* symbols;
    uint8_t* code_section;
    uint8_t* data_section;
    
    // 运行时状态
    bool is_loaded;
    bool is_initialized;
    void* native_handle;  // 本地库句柄（如果需要）
    
    // 函数查找表
    void* function_table[MAX_LIBC_FUNCTIONS];
} LibcRtModule;

// ===============================================
// 标准libc函数ID定义
// ===============================================

typedef enum {
    LIBC_FUNC_MALLOC = 0x0001,
    LIBC_FUNC_FREE = 0x0002,
    LIBC_FUNC_CALLOC = 0x0003,
    LIBC_FUNC_REALLOC = 0x0004,
    
    LIBC_FUNC_MEMCPY = 0x0010,
    LIBC_FUNC_MEMSET = 0x0011,
    LIBC_FUNC_MEMCMP = 0x0012,
    LIBC_FUNC_MEMMOVE = 0x0013,
    
    LIBC_FUNC_STRLEN = 0x0020,
    LIBC_FUNC_STRCPY = 0x0021,
    LIBC_FUNC_STRNCPY = 0x0022,
    LIBC_FUNC_STRCMP = 0x0023,
    LIBC_FUNC_STRNCMP = 0x0024,
    LIBC_FUNC_STRCAT = 0x0025,
    LIBC_FUNC_STRNCAT = 0x0026,
    LIBC_FUNC_STRCHR = 0x0027,
    LIBC_FUNC_STRRCHR = 0x0028,
    LIBC_FUNC_STRSTR = 0x0029,
    
    LIBC_FUNC_PRINTF = 0x0030,
    LIBC_FUNC_SPRINTF = 0x0031,
    LIBC_FUNC_FPRINTF = 0x0032,
    LIBC_FUNC_SCANF = 0x0033,
    LIBC_FUNC_SSCANF = 0x0034,
    LIBC_FUNC_FSCANF = 0x0035,
    
    LIBC_FUNC_FOPEN = 0x0040,
    LIBC_FUNC_FCLOSE = 0x0041,
    LIBC_FUNC_FREAD = 0x0042,
    LIBC_FUNC_FWRITE = 0x0043,
    LIBC_FUNC_FSEEK = 0x0044,
    LIBC_FUNC_FTELL = 0x0045,
    LIBC_FUNC_FEOF = 0x0046,
    LIBC_FUNC_FERROR = 0x0047,
    
    LIBC_FUNC_ATOI = 0x0050,
    LIBC_FUNC_ATOL = 0x0051,
    LIBC_FUNC_ATOF = 0x0052,
    LIBC_FUNC_STRTOL = 0x0053,
    LIBC_FUNC_STRTOD = 0x0054,
    
    LIBC_FUNC_EXIT = 0x0060,
    LIBC_FUNC_ABORT = 0x0061,
    LIBC_FUNC_SYSTEM = 0x0062,
    LIBC_FUNC_GETENV = 0x0063,
    
    LIBC_FUNC_MAX = 0x00FF
} LibcFunctionId;

// ===============================================
// 模块管理API
// ===============================================

/**
 * 创建新的libc.rt模块
 */
LibcRtModule* libc_rt_module_create(void);

/**
 * 释放libc.rt模块
 */
void libc_rt_module_free(LibcRtModule* module);

/**
 * 从文件加载libc.rt模块
 */
LibcRtModule* libc_rt_module_load_from_file(const char* filename);

/**
 * 从内存加载libc.rt模块
 */
LibcRtModule* libc_rt_module_load_from_memory(const uint8_t* data, size_t size);

/**
 * 保存libc.rt模块到文件
 */
int libc_rt_module_save_to_file(LibcRtModule* module, const char* filename);

/**
 * 初始化模块（准备执行）
 */
int libc_rt_module_initialize(LibcRtModule* module);

/**
 * 查找函数
 */
void* libc_rt_module_get_function(LibcRtModule* module, const char* name);
void* libc_rt_module_get_function_by_id(LibcRtModule* module, LibcFunctionId func_id);

/**
 * 添加函数到模块
 */
int libc_rt_module_add_function(LibcRtModule* module, 
                               const char* name,
                               LibcFunctionId func_id,
                               void* function_ptr,
                               uint32_t param_count,
                               uint32_t return_type);

/**
 * 验证模块完整性
 */
bool libc_rt_module_validate(LibcRtModule* module);

/**
 * 获取模块信息
 */
void libc_rt_module_get_info(LibcRtModule* module, char* info_buffer, size_t buffer_size);

// ===============================================
// 标准libc.rt模块构建器
// ===============================================

/**
 * 构建标准的libc.rt模块（转发到系统libc）
 */
LibcRtModule* libc_rt_build_standard_module(void);

/**
 * 构建最小的libc.rt模块（仅核心函数）
 */
LibcRtModule* libc_rt_build_minimal_module(void);

/**
 * 构建操作系统开发用的libc.rt模块（自实现）
 */
LibcRtModule* libc_rt_build_os_module(void);

// ===============================================
// 运行时集成API
// ===============================================

/**
 * 将libc.rt模块集成到runtime中
 */
int libc_rt_integrate_with_runtime(LibcRtModule* module, void* runtime_context);

/**
 * 从runtime中卸载libc.rt模块
 */
int libc_rt_unload_from_runtime(LibcRtModule* module, void* runtime_context);

/**
 * 运行时函数调用接口
 */
int libc_rt_call_function(LibcRtModule* module, 
                         LibcFunctionId func_id,
                         void* args,
                         size_t args_size,
                         void* result,
                         size_t result_size);

// ===============================================
// 调试和诊断API
// ===============================================

/**
 * 打印模块信息
 */
void libc_rt_module_print_info(LibcRtModule* module);

/**
 * 打印符号表
 */
void libc_rt_module_print_symbols(LibcRtModule* module);

/**
 * 检查函数是否存在
 */
bool libc_rt_module_has_function(LibcRtModule* module, const char* name);
bool libc_rt_module_has_function_id(LibcRtModule* module, LibcFunctionId func_id);

/**
 * 获取模块统计信息
 */
typedef struct {
    uint32_t total_functions;
    uint32_t loaded_functions;
    uint32_t failed_functions;
    uint32_t memory_usage;
    uint32_t code_size;
    uint32_t data_size;
} LibcRtModuleStats;

void libc_rt_module_get_stats(LibcRtModule* module, LibcRtModuleStats* stats);

#ifdef __cplusplus
}
#endif

#endif // LIBC_RT_MODULE_H
