/**
 * module_syntax.h - 模块化程序设计语法扩展
 * 
 * 定义模块导入、导出和使用的语法扩展
 */

#ifndef MODULE_SYNTAX_H
#define MODULE_SYNTAX_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// 模块语法扩展
// ===============================================

// 模块导入语法：#import "module_name"
// 示例：#import "libc.rt"
//       #import "math.rt"
//       #import "graphics.rt"

// 模块导出语法：#export function_name
// 示例：#export my_function
//       #export my_variable

// 模块使用语法：module_name::function_name
// 示例：libc::printf("Hello, World!\n");
//       math::sqrt(16.0);

// ===============================================
// 模块导入声明
// ===============================================

typedef struct {
    char module_name[64];        // 模块名称
    char alias[64];              // 模块别名（可选）
    bool is_system_module;       // 是否为系统模块
    bool is_required;            // 是否为必需模块
    char version[16];            // 版本要求（可选）
} ModuleImport;

// ===============================================
// 模块导出声明
// ===============================================

typedef struct {
    char symbol_name[64];        // 符号名称
    uint32_t symbol_type;        // 符号类型
    void* symbol_address;        // 符号地址
    bool is_public;              // 是否公开导出
} ModuleExport;

// ===============================================
// 模块程序结构
// ===============================================

typedef struct {
    ModuleImport* imports;       // 导入列表
    uint32_t import_count;       // 导入数量
    ModuleExport* exports;       // 导出列表
    uint32_t export_count;       // 导出数量
    char program_name[64];       // 程序名称
    char program_version[16];    // 程序版本
} ModularProgram;

// ===============================================
// 标准系统模块定义
// ===============================================

// libc.rt - 标准C库模块
#define LIBC_MODULE_NAME "libc.rt"
#define LIBC_MODULE_FUNCTIONS \
    "printf", "scanf", "malloc", "free", "strlen", "strcmp", "strcpy", \
    "strcat", "memcpy", "memset", "fopen", "fclose", "fread", "fwrite", \
    "atoi", "atof", "sprintf", "sscanf", "exit", "abort"

// math.rt - 数学库模块
#define MATH_MODULE_NAME "math.rt"
#define MATH_MODULE_FUNCTIONS \
    "sin", "cos", "tan", "asin", "acos", "atan", "atan2", \
    "sinh", "cosh", "tanh", "exp", "log", "log10", "pow", \
    "sqrt", "ceil", "floor", "fabs", "fmod"

// io.rt - 输入输出模块
#define IO_MODULE_NAME "io.rt"
#define IO_MODULE_FUNCTIONS \
    "open", "close", "read", "write", "seek", "tell", \
    "flush", "sync", "dup", "dup2", "pipe", "select"

// thread.rt - 线程模块
#define THREAD_MODULE_NAME "thread.rt"
#define THREAD_MODULE_FUNCTIONS \
    "thread_create", "thread_join", "thread_detach", "thread_exit", \
    "mutex_init", "mutex_lock", "mutex_unlock", "mutex_destroy", \
    "cond_init", "cond_wait", "cond_signal", "cond_broadcast"

// ===============================================
// 模块化程序API
// ===============================================

/**
 * 初始化模块化程序系统
 *
 * @return 0成功，-1失败
 */
int modular_program_init(void);

/**
 * 创建模块化程序
 *
 * @param program_name 程序名称
 * @param program_version 程序版本
 * @return 程序指针，失败返回NULL
 */
ModularProgram* modular_program_create(const char* program_name, const char* program_version);

/**
 * 添加模块导入
 *
 * @param program 程序指针
 * @param module_name 模块名称
 * @param alias 模块别名（可选）
 * @param version 版本要求（可选）
 * @return 0成功，-1失败
 */
int modular_program_add_import(ModularProgram* program, const char* module_name, 
                              const char* alias, const char* version);

/**
 * 添加模块导出
 *
 * @param program 程序指针
 * @param symbol_name 符号名称
 * @param symbol_type 符号类型
 * @param symbol_address 符号地址
 * @return 0成功，-1失败
 */
int modular_program_add_export(ModularProgram* program, const char* symbol_name,
                              uint32_t symbol_type, void* symbol_address);

/**
 * 解析模块导入
 *
 * @param program 程序指针
 * @return 0成功，-1失败
 */
int modular_program_resolve_imports(ModularProgram* program);

/**
 * 查找导入的符号
 *
 * @param program 程序指针
 * @param module_name 模块名称
 * @param symbol_name 符号名称
 * @return 符号地址，失败返回NULL
 */
void* modular_program_find_symbol(ModularProgram* program, const char* module_name, 
                                 const char* symbol_name);

/**
 * 销毁模块化程序
 *
 * @param program 程序指针
 */
void modular_program_destroy(ModularProgram* program);

// ===============================================
// 编译器集成API
// ===============================================

/**
 * 解析模块导入指令
 *
 * @param source_line 源代码行
 * @param import 导入结构指针
 * @return 0成功，-1失败
 */
int parse_module_import(const char* source_line, ModuleImport* import);

/**
 * 解析模块导出指令
 *
 * @param source_line 源代码行
 * @param export 导出结构指针
 * @return 0成功，-1失败
 */
int parse_module_export(const char* source_line, ModuleExport* export);

/**
 * 解析模块函数调用
 *
 * @param source_line 源代码行
 * @param module_name 模块名称缓冲区
 * @param function_name 函数名称缓冲区
 * @return 0成功，-1失败
 */
int parse_module_call(const char* source_line, char* module_name, char* function_name);

/**
 * 生成模块化程序的ASTC字节码
 *
 * @param program 程序指针
 * @param output_buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 生成的字节码大小，失败返回-1
 */
int generate_modular_astc(ModularProgram* program, uint8_t* output_buffer, size_t buffer_size);

// ===============================================
// 运行时集成API
// ===============================================

/**
 * 加载模块化程序
 *
 * @param astc_data ASTC字节码
 * @param astc_size 字节码大小
 * @return 程序指针，失败返回NULL
 */
ModularProgram* runtime_load_modular_program(uint8_t* astc_data, size_t astc_size);

/**
 * 执行模块化程序
 *
 * @param program 程序指针
 * @param argc 命令行参数数量
 * @param argv 命令行参数
 * @return 程序返回值
 */
int runtime_execute_modular_program(ModularProgram* program, int argc, char* argv[]);

/**
 * 获取模块化程序统计信息
 *
 * @param program 程序指针
 * @param import_count 导入数量
 * @param export_count 导出数量
 * @param memory_usage 内存使用量
 */
void modular_program_get_stats(ModularProgram* program, uint32_t* import_count,
                              uint32_t* export_count, size_t* memory_usage);

#ifdef __cplusplus
}
#endif

#endif // MODULE_SYNTAX_H
