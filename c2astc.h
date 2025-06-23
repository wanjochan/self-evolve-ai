/**
 * c2astc.h - C语言到ASTC（WebAssembly扩展AST）的转换库头文件
 */

#ifndef C2ASTC_H
#define C2ASTC_H

#include <stdbool.h>
#include "astc.h"

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// 公共结构和类型定义
// ===============================================

/**
 * 转换配置选项
 */
typedef struct {
    bool optimize_level;        // 优化级别
    bool enable_extensions;     // 启用WASX扩展
    bool emit_debug_info;       // 生成调试信息
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
 * 释放由库分配的内存
 * 
 * @param ptr 指向由库分配的内存的指针
 */
void c2astc_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif // C2ASTC_H 
