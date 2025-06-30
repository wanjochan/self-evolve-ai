/**
 * c2astc.h - C语言到ASTC（WebAssembly扩展AST）的转换库头文件
 */

#ifndef C2ASTC_H
#define C2ASTC_H

#include <stdbool.h>
#include "core_astc.h"

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// 公共结构和类型定义
// ===============================================

/**
 * C标准版本枚举
 */
typedef enum {
    C_STD_C99 = 0,
    C_STD_C11 = 1,
    C_STD_C17 = 2
} CStandard;

/**
 * 转换配置选项
 */
typedef struct {
    int optimize_level;         // 优化级别 (0=无优化, 1=基础, 2=高级, 3=激进)
    bool enable_extensions;     // 启用WASX扩展
    bool emit_debug_info;       // 生成调试信息
    bool enable_warnings;       // 启用警告
    bool warnings_as_errors;    // 将警告视为错误
    bool compile_only;          // 仅编译，不链接
    bool generate_assembly;     // 生成汇编输出
    bool preprocess_only;       // 仅预处理
    CStandard c_standard;       // C标准版本
    char include_dirs[16][256]; // 包含目录列表 (最多16个)
    int include_dir_count;      // 包含目录数量
    char macros[32][256];       // 宏定义列表 (最多32个)
    int macro_count;            // 宏定义数量
} C2AstcOptions;

// ===============================================
// API 函数
// ===============================================

/**
 * 获取默认的转换选项
 * 
 * @return 默认配置选项
 */
C2AstcOptions c2astc_default_options(void);

/**
 * 打印C到ASTC转换库版本信息
 */
void c2astc_print_version(void);

/**
 * 将C源代码转换为ASTC表示
 * 
 * @param source C源代码
 * @param options 转换配置选项
 * @return 转换后的ASTC根节点，失败返回NULL
 */
struct ASTNode* c2astc_convert(const char *source, const C2AstcOptions *options);

/**
 * 从文件加载C源代码并转换为ASTC
 * 
 * @param filename C源文件名
 * @param options 转换配置选项
 * @return 转换后的ASTC根节点，失败返回NULL
 */
struct ASTNode* c2astc_convert_file(const char *filename, const C2AstcOptions *options);

/**
 * 获取最后一次错误消息
 * 
 * @return 错误消息字符串，如果没有错误则返回NULL
 */
const char* c2astc_get_error(void);

/**
 * 将ASTC表示序列化为二进制格式
 * 
 * @param node ASTC根节点
 * @param out_size 输出的二进制大小
 * @return 序列化后的二进制数据，调用者负责释放内存
 */
unsigned char* c2astc_serialize(struct ASTNode *node, size_t *out_size);

/**
 * 从二进制格式反序列化为ASTC表示
 * 
 * @param binary 二进制数据
 * @param size 数据大小
 * @return 反序列化后的ASTC根节点，失败返回NULL
 */
struct ASTNode* c2astc_deserialize(const unsigned char *binary, size_t size);

unsigned char* c2astc(struct ASTNode *node, const C2AstcOptions *options, size_t *out_size);

/**
 * 将AST转换为ASTC字节码
 *
 * @param ast AST根节点
 * @param out_size 输出字节码大小
 * @return 生成的ASTC字节码，失败返回NULL
 */
unsigned char* ast_to_astc_bytecode(struct ASTNode* ast, size_t* out_size);

/**
 * 将AST转换为ASTC字节码（带优化选项）
 *
 * @param ast AST根节点
 * @param options 编译选项
 * @param out_size 输出字节码大小
 * @return 生成的ASTC字节码，失败返回NULL
 */
unsigned char* ast_to_astc_bytecode_with_options(struct ASTNode* ast, const C2AstcOptions* options, size_t* out_size);

/**
 * 释放由库分配的内存
 * 
 * @param ptr 指向由库分配的内存的指针
 */
void c2astc_free(void *ptr);

/**
 * 创建AST节点
 * 
 * @param type 节点类型
 * @param line 行号
 * @param column 列号
 * @return 创建的节点
 */
struct ASTNode* ast_create_node(ASTNodeType type, int line, int column);

// ===============================================
// 节点创建函数
// ===============================================

/**
 * 创建标识符节点
 * 
 * @param name 标识符名称
 * @param line 行号
 * @param column 列号
 * @return 创建的节点
 */
struct ASTNode* create_identifier_node(const char *name, int line, int column);

/**
 * 创建整数常量节点
 * 
 * @param value 整数值
 * @param line 行号
 * @param column 列号
 * @return 创建的节点
 */
struct ASTNode* create_int_node(long long value, int line, int column);

/**
 * 创建浮点数常量节点
 * 
 * @param value 浮点数值
 * @param line 行号
 * @param column 列号
 * @return 创建的节点
 */
struct ASTNode* create_float_node(double value, int line, int column);

/**
 * 创建字符串字面量节点
 * 
 * @param value 字符串值
 * @param line 行号
 * @param column 列号
 * @return 创建的节点
 */
struct ASTNode* create_string_node(const char *value, int line, int column);

/**
 * 创建二元操作节点
 * 
 * @param op 操作符类型
 * @param left 左操作数
 * @param right 右操作数
 * @param line 行号
 * @param column 列号
 * @return 创建的节点
 */
struct ASTNode* create_binary_op_node(int op, struct ASTNode *left, struct ASTNode *right, int line, int column);

/**
 * 创建一元操作节点
 * 
 * @param op 操作符类型
 * @param operand 操作数
 * @param line 行号
 * @param column 列号
 * @return 创建的节点
 */
struct ASTNode* create_unary_op_node(int op, struct ASTNode *operand, int line, int column);

/**
 * 创建函数调用节点
 * 
 * @param callee 被调用函数
 * @param args 参数数组
 * @param arg_count 参数数量
 * @param line 行号
 * @param column 列号
 * @return 创建的节点
 */
struct ASTNode* create_call_expr_node(struct ASTNode *callee, struct ASTNode **args, int arg_count, int line, int column);

#ifdef __cplusplus
}
#endif

#endif // C2ASTC_H 
