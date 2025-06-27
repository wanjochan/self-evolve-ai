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

#ifdef __cplusplus
}
#endif

#endif // LOADER_H 