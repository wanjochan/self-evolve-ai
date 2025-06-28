/**
 * astc2rt.h - ASTC到Runtime转换库头文件
 *
 * 将ASTC格式转译为轻量化的.rt Runtime格式
 * 符合PRD.md规范的专注于libc转发封装的轻量化设计
 */

#ifndef ASTC2RT_H
#define ASTC2RT_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "compiler_c2astc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 代码生成器结构
 */
typedef struct {
    uint8_t* code;          // 生成的机器码缓冲区
    size_t code_size;       // 当前代码大小
    size_t code_capacity;   // 代码缓冲区容量
} CodeGen;

/**
 * Runtime文件头结构
 */
typedef struct {
    char magic[4];          // "RTME"
    uint32_t version;       // 版本号
    uint32_t size;          // 代码大小
    uint32_t entry_point;   // 入口点偏移
} RuntimeHeader;

/**
 * 初始化ASTC代码生成器
 *
 * @return 初始化的代码生成器
 */
CodeGen* astc_codegen_init(void);

/**
 * 释放ASTC代码生成器资源
 *
 * @param gen 代码生成器
 */
void astc_codegen_free(CodeGen* gen);

/**
 * 输出一个字节到代码缓冲区
 *
 * @param gen 代码生成器
 * @param byte 要输出的字节
 */
void emit_byte(CodeGen* gen, uint8_t byte);

/**
 * 输出32位立即数
 *
 * @param gen 代码生成器
 * @param value 32位整数值
 */
void emit_int32(CodeGen* gen, int32_t value);

/**
 * 将C源文件编译为Runtime二进制文件
 *
 * @param c_file C源文件路径
 * @param output_file 输出文件路径
 * @return 成功返回0，失败返回非0值
 */
int compile_c_to_runtime_bin(const char* c_file, const char* output_file);

/**
 * 将ASTC文件编译为Runtime二进制文件
 *
 * @param astc_file ASTC文件路径
 * @param output_file 输出文件路径
 * @return 成功返回0，失败返回非0值
 */
int compile_astc_to_runtime_bin(const char* astc_file, const char* output_file);

/**
 * 编译完整的ASTC虚拟机到机器码
 *
 * @param gen 代码生成器
 */
void compile_complete_runtime_vm(CodeGen* gen);

/**
 * 从二进制数据生成Runtime文件
 *
 * @param code 二进制代码数据
 * @param code_size 代码大小
 * @param output_file 输出文件路径
 * @return 成功返回0，失败返回非0值
 */
int generate_runtime_file(uint8_t* code, size_t code_size, const char* output_file);

#ifdef __cplusplus
}
#endif

#endif /* ASTC2RT_H */ 