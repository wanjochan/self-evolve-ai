/**
 * loader.h - ASTC程序加载器头文件
 * 
 * 该文件定义了ASTC程序加载器的基本结构和接口，用于加载和运行ASTC格式的程序。
 * Loader模块是连接操作系统和Runtime的桥梁。
 */

#ifndef LOADER_H
#define LOADER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "astc.h"
#include "runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// 常量定义
// ===============================================

#define LOADER_VERSION "0.1.0"
#define ASTC_MAGIC "ASTC"  // ASTC文件魔数

// ===============================================
// 类型定义
// ===============================================

// Runtime二进制格式头部
typedef struct {
    char magic[4];       // 魔数，固定为 "RTME"
    uint32_t version;    // 版本
    uint32_t size;       // 代码大小
    uint32_t entry_point;// 入口点偏移
    uint32_t reserved;   // 保留
} RuntimeHeader;

// Loader命令行选项
typedef struct {
    const char* runtime_file;  // Runtime文件路径
    const char* program_file;  // Program文件路径
    bool verbose;              // 详细输出
    bool debug;                // 调试模式
} LoaderOptions;

// 加载器配置
typedef struct {
    bool debug_mode;           // 调试模式
    const char* program_path;  // 程序路径
    const char* entry_point;   // 入口点函数名
    int argc;                  // 命令行参数数量
    char** argv;               // 命令行参数
} LoaderConfig;

// 加载器实例
typedef struct {
    LoaderConfig config;       // 配置
    struct ASTNode* program;   // 加载的程序
    RuntimeVM vm;              // 虚拟机实例
    char error_message[256];   // 错误信息
} Loader;

// ===============================================
// 函数声明
// ===============================================

/**
 * 初始化加载器
 * 
 * @param loader 加载器实例
 * @param config 配置
 * @return 成功返回true，失败返回false
 */
bool loader_init(Loader* loader, const LoaderConfig* config);

/**
 * 销毁加载器
 * 
 * @param loader 加载器实例
 */
void loader_destroy(Loader* loader);

/**
 * 加载ASTC程序
 * 
 * @param loader 加载器实例
 * @param path ASTC文件路径
 * @return 成功返回true，失败返回false
 */
bool loader_load_program(Loader* loader, const char* path);

/**
 * 从内存加载ASTC程序
 * 
 * @param loader 加载器实例
 * @param data ASTC数据
 * @param size 数据大小
 * @return 成功返回true，失败返回false
 */
bool loader_load_program_from_memory(Loader* loader, const uint8_t* data, size_t size);

/**
 * 运行加载的程序
 * 
 * @param loader 加载器实例
 * @return 程序退出码
 */
int loader_run(Loader* loader);

/**
 * 注册标准库函数
 * 
 * @param loader 加载器实例
 * @return 成功返回true，失败返回false
 */
bool loader_register_stdlib(Loader* loader);

/**
 * 获取最后一次错误信息
 * 
 * @param loader 加载器实例
 * @return 错误信息字符串
 */
const char* loader_get_error(Loader* loader);

/**
 * 设置命令行参数
 * 
 * @param loader 加载器实例
 * @param argc 参数数量
 * @param argv 参数数组
 * @return 成功返回true，失败返回false
 */
bool loader_set_args(Loader* loader, int argc, char** argv);

/**
 * 加载文件到内存
 * 
 * @param filename 文件名
 * @param size 返回文件大小
 * @return 文件内容指针，需手动free
 */
void* load_file(const char* filename, size_t* size);

/**
 * 使用Runtime执行Program
 * 
 * @param runtime_data Runtime二进制数据
 * @param runtime_size Runtime大小
 * @param program_data Program数据
 * @param program_size Program大小
 * @param options 加载器选项
 * @return 执行结果
 */
int execute_runtime_with_program(void* runtime_data, size_t runtime_size, 
                                 void* program_data, size_t program_size, 
                                 const LoaderOptions* options);

/**
 * 解析加载器命令行参数
 * 
 * @param argc 参数数量
 * @param argv 参数数组
 * @param options 加载器选项
 * @return 成功返回true，失败返回false
 */
bool parse_loader_arguments(int argc, char* argv[], LoaderOptions* options);

#ifdef __cplusplus
}
#endif

#endif // LOADER_H 