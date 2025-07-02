/**
 * program.h - ASTC程序模块头文件
 * 
 * 该文件定义了ASTC程序模块的基本结构和接口，用于创建和管理ASTC格式的程序。
 * Program模块是平台无关的程序逻辑部分。
 */

#ifndef PROGRAM_H
#define PROGRAM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "astc.h"

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// 常量定义
// ===============================================

#define PROGRAM_VERSION "0.1.0"

// ===============================================
// 类型定义
// ===============================================

// 程序配置
typedef struct {
    const char* name;           // 程序名称
    const char* version;        // 程序版本
    const char* author;         // 程序作者
    const char* description;    // 程序描述
    bool debug_mode;            // 调试模式
} ProgramConfig;

// 程序实例
typedef struct {
    ProgramConfig config;       // 配置
    struct ASTNode* root;       // 程序根节点
    char error_message[256];    // 错误信息
} Program;

// ===============================================
// 函数声明
// ===============================================

/**
 * 初始化程序
 * 
 * @param program 程序实例
 * @param config 配置
 * @return 成功返回true，失败返回false
 */
bool program_init(Program* program, const ProgramConfig* config);

/**
 * 销毁程序
 * 
 * @param program 程序实例
 */
void program_destroy(Program* program);

/**
 * 创建空程序
 * 
 * @param program 程序实例
 * @return 成功返回true，失败返回false
 */
bool program_create_empty(Program* program);

/**
 * 从C源代码创建程序
 * 
 * @param program 程序实例
 * @param source C源代码
 * @return 成功返回true，失败返回false
 */
bool program_create_from_c(Program* program, const char* source);

/**
 * 从C源文件创建程序
 * 
 * @param program 程序实例
 * @param path C源文件路径
 * @return 成功返回true，失败返回false
 */
bool program_create_from_file(Program* program, const char* path);

/**
 * 添加函数到程序
 * 
 * @param program 程序实例
 * @param name 函数名
 * @param return_type 返回类型
 * @param param_types 参数类型数组
 * @param param_names 参数名称数组
 * @param param_count 参数数量
 * @param body 函数体
 * @return 成功返回true，失败返回false
 */
bool program_add_function(Program* program, const char* name, struct ASTNode* return_type,
                          struct ASTNode** param_types, const char** param_names, int param_count,
                          struct ASTNode* body);

/**
 * 添加全局变量到程序
 * 
 * @param program 程序实例
 * @param name 变量名
 * @param type 变量类型
 * @param initializer 初始化表达式
 * @return 成功返回true，失败返回false
 */
bool program_add_global(Program* program, const char* name, struct ASTNode* type,
                        struct ASTNode* initializer);

/**
 * 序列化程序
 * 
 * @param program 程序实例
 * @param out_size 输出大小
 * @return 序列化后的数据，失败返回NULL
 */
uint8_t* program_serialize(Program* program, size_t* out_size);

/**
 * 保存程序到文件
 * 
 * @param program 程序实例
 * @param path 文件路径
 * @return 成功返回true，失败返回false
 */
bool program_save(Program* program, const char* path);

/**
 * 获取最后一次错误信息
 * 
 * @param program 程序实例
 * @return 错误信息字符串
 */
const char* program_get_error(Program* program);

/**
 * 创建类型节点
 * 
 * @param type_name 类型名称
 * @return 类型节点
 */
struct ASTNode* program_create_type(const char* type_name);

/**
 * 创建指针类型节点
 * 
 * @param base_type 基础类型
 * @return 指针类型节点
 */
struct ASTNode* program_create_pointer_type(struct ASTNode* base_type);

/**
 * 创建数组类型节点
 * 
 * @param element_type 元素类型
 * @param size 数组大小
 * @return 数组类型节点
 */
struct ASTNode* program_create_array_type(struct ASTNode* element_type, int size);

#ifdef __cplusplus
}
#endif

#endif // PROGRAM_H 