/**
 * astc2native.h - ASTC到Native转换库头文件
 *
 * 将ASTC格式转译为轻量化的.native Runtime格式
 * 符合PRD.md规范的专注于libc转发封装的轻量化设计
 */

#ifndef ASTC2NATIVE_H
#define ASTC2NATIVE_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "c2astc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ASTC指令操作码定义
 */
typedef enum {
    ASTC_OP_NOP = 0x00,         // 空操作
    ASTC_OP_CONST_I32 = 0x10,   // 32位整数常量
    ASTC_OP_ADD = 0x20,         // 加法
    ASTC_OP_SUB = 0x21,         // 减法
    ASTC_OP_MUL = 0x22,         // 乘法
    ASTC_OP_DIV = 0x23,         // 除法
    ASTC_OP_LOAD_LOCAL = 0x30,  // 加载局部变量
    ASTC_OP_STORE_LOCAL = 0x31, // 存储局部变量
    ASTC_OP_JUMP = 0x40,        // 无条件跳转
    ASTC_OP_JUMP_IF_FALSE = 0x41, // 条件跳转
    ASTC_OP_CALL_USER = 0x50,   // 调用用户函数
    ASTC_OP_LIBC_CALL = 0xF0,   // 调用libc函数
    ASTC_OP_RETURN = 0xFF       // 函数返回
} ASTCOpcode;

/**
 * libc函数ID定义
 */
typedef enum {
    LIBC_PRINTF = 0x0030,  // printf函数
    LIBC_MALLOC = 0x0031,  // malloc函数
    LIBC_FREE = 0x0032,    // free函数
    LIBC_FOPEN = 0x0033,   // fopen函数
    LIBC_FCLOSE = 0x0034,  // fclose函数
    LIBC_FREAD = 0x0035,   // fread函数
    LIBC_FWRITE = 0x0036   // fwrite函数
} LibcFuncId;

/**
 * ASTC指令操作数联合体
 */
typedef union {
    int32_t i32_val;           // 32位整数值
    uint32_t var_index;        // 变量索引
    uint32_t target;           // 跳转目标
    uint32_t func_addr;        // 函数地址
    struct {
        uint16_t func_id;      // libc函数ID
        uint16_t arg_count;    // 参数数量
    } libc_call;
} ASTCOperands;

/**
 * ASTC指令结构
 */
typedef struct {
    ASTCOpcode opcode;        // 操作码
    ASTCOperands operands;    // 操作数
} ASTCInstruction;

/**
 * 目标架构枚举
 */
typedef enum {
    TARGET_ARCH_X86_32,    // x86 32位
    TARGET_ARCH_X86_64,    // x86 64位
    TARGET_ARCH_ARM32,     // ARM 32位
    TARGET_ARCH_ARM64,     // ARM 64位
    TARGET_ARCH_UNKNOWN    // 未知架构
} TargetArch;

/**
 * 代码生成器结构
 */
typedef struct {
    uint8_t* code;          // 生成的机器码缓冲区
    size_t code_size;       // 当前代码大小
    size_t code_capacity;   // 代码缓冲区容量
    TargetArch target_arch; // 目标架构
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
 * 检测当前运行时架构
 *
 * @return 检测到的架构类型
 */
TargetArch detect_runtime_architecture(void);

/**
 * 获取架构名称字符串
 *
 * @param arch 架构类型
 * @return 架构名称字符串
 */
const char* get_architecture_name(TargetArch arch);

/**
 * 从字符串解析目标架构
 *
 * @param arch_str 架构字符串
 * @return 解析的架构类型
 */
TargetArch parse_target_architecture(const char* arch_str);

/**
 * 检查架构是否支持
 *
 * @param arch 架构类型
 * @return 是否支持
 */
bool is_architecture_supported(TargetArch arch);

/**
 * 初始化ASTC代码生成器
 *
 * @param target_arch 目标架构，如果为ARCH_UNKNOWN则自动检测
 * @return 初始化的代码生成器
 */
CodeGen* astc_codegen_init(TargetArch target_arch);

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

/**
 * 将ASTC指令转换为目标架构的机器码
 *
 * @param instr ASTC指令
 * @param gen 代码生成器
 * @return 成功返回0，失败返回-1
 */
int convert_astc_to_machine_code(ASTCInstruction* instr, CodeGen* gen);

#ifdef __cplusplus
}
#endif

#endif /* ASTC2NATIVE_H */