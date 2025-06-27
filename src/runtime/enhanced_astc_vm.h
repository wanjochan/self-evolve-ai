/**
 * enhanced_astc_vm.h - 增强的ASTC虚拟机
 * 
 * 集成libc转发系统的完整ASTC虚拟机实现
 * 目标：支持完整的C语言程序执行，为脱离TinyCC做准备
 */

#ifndef ENHANCED_ASTC_VM_H
#define ENHANCED_ASTC_VM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "libc_forward.h"

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// ASTC虚拟机配置
// ===============================================

#define ASTC_VM_STACK_SIZE      4096    // 操作数栈大小
#define ASTC_VM_LOCALS_SIZE     1024    // 局部变量数量
#define ASTC_VM_GLOBALS_SIZE    2048    // 全局变量数量
#define ASTC_VM_CALL_STACK_SIZE 256     // 调用栈深度

// ===============================================
// ASTC指令集定义
// ===============================================

// 基础指令
#define ASTC_NOP            0x00    // 空操作
#define ASTC_HALT           0x01    // 停机

// 常量加载
#define ASTC_CONST_I32      0x10    // 加载32位整数常量
#define ASTC_CONST_I64      0x11    // 加载64位整数常量
#define ASTC_CONST_F32      0x12    // 加载32位浮点常量
#define ASTC_CONST_F64      0x13    // 加载64位浮点常量
#define ASTC_CONST_STR      0x14    // 加载字符串常量

// 变量操作
#define ASTC_LOAD_LOCAL     0x20    // 加载局部变量
#define ASTC_STORE_LOCAL    0x21    // 存储局部变量
#define ASTC_LOAD_GLOBAL    0x22    // 加载全局变量
#define ASTC_STORE_GLOBAL   0x23    // 存储全局变量

// 栈操作
#define ASTC_POP            0x30    // 弹出栈顶
#define ASTC_DUP            0x31    // 复制栈顶
#define ASTC_SWAP           0x32    // 交换栈顶两个元素

// 算术运算
#define ASTC_ADD_I32        0x40    // 32位整数加法
#define ASTC_SUB_I32        0x41    // 32位整数减法
#define ASTC_MUL_I32        0x42    // 32位整数乘法
#define ASTC_DIV_I32        0x43    // 32位整数除法
#define ASTC_MOD_I32        0x44    // 32位整数取模

// 比较运算
#define ASTC_EQ_I32         0x50    // 32位整数相等比较
#define ASTC_NE_I32         0x51    // 32位整数不等比较
#define ASTC_LT_I32         0x52    // 32位整数小于比较
#define ASTC_LE_I32         0x53    // 32位整数小于等于比较
#define ASTC_GT_I32         0x54    // 32位整数大于比较
#define ASTC_GE_I32         0x55    // 32位整数大于等于比较

// 逻辑运算
#define ASTC_AND            0x60    // 逻辑与
#define ASTC_OR             0x61    // 逻辑或
#define ASTC_NOT            0x62    // 逻辑非

// 控制流
#define ASTC_JUMP           0x70    // 无条件跳转
#define ASTC_JUMP_IF        0x71    // 条件跳转（真）
#define ASTC_JUMP_IF_NOT    0x72    // 条件跳转（假）
#define ASTC_CALL           0x73    // 函数调用
#define ASTC_RETURN         0x74    // 函数返回

// 内存操作
#define ASTC_LOAD_MEM       0x80    // 从内存加载
#define ASTC_STORE_MEM      0x81    // 存储到内存
#define ASTC_ALLOC          0x82    // 分配内存
#define ASTC_FREE           0x83    // 释放内存

// 系统调用
#define ASTC_LIBC_CALL      0xF0    // libc函数调用
#define ASTC_SYSCALL        0xF1    // 系统调用
#define ASTC_DEBUG_PRINT    0xF2    // 调试输出

// ===============================================
// 虚拟机状态结构
// ===============================================

typedef struct {
    uint64_t value;     // 值
    uint8_t type;       // 类型（0=int32, 1=int64, 2=float32, 3=float64, 4=pointer）
} ASTCValue;

typedef struct {
    uint32_t pc;        // 程序计数器
    uint32_t sp;        // 栈指针
    uint32_t fp;        // 帧指针
} CallFrame;

typedef struct {
    // 代码和数据
    uint8_t* code;                          // ASTC字节码
    size_t code_size;                       // 代码大小
    uint8_t* data;                          // 数据段
    size_t data_size;                       // 数据段大小
    
    // 执行状态
    uint32_t pc;                            // 程序计数器
    bool running;                           // 运行状态
    int32_t exit_code;                      // 退出码
    
    // 栈和变量
    ASTCValue stack[ASTC_VM_STACK_SIZE];    // 操作数栈
    int32_t stack_top;                      // 栈顶指针
    ASTCValue locals[ASTC_VM_LOCALS_SIZE];  // 局部变量
    ASTCValue globals[ASTC_VM_GLOBALS_SIZE]; // 全局变量
    
    // 调用栈
    CallFrame call_stack[ASTC_VM_CALL_STACK_SIZE]; // 调用栈
    int32_t call_stack_top;                 // 调用栈顶
    
    // libc转发
    LibcStats libc_stats;                   // libc调用统计
    
    // 调试信息
    bool debug_mode;                        // 调试模式
    uint64_t instruction_count;             // 执行指令计数
} EnhancedASTCVM;

// ===============================================
// 虚拟机接口
// ===============================================

/**
 * 初始化增强ASTC虚拟机
 * @param vm 虚拟机实例
 * @param code ASTC字节码
 * @param code_size 代码大小
 * @param data 数据段
 * @param data_size 数据段大小
 * @return 0成功，非0失败
 */
int enhanced_astc_vm_init(EnhancedASTCVM* vm, uint8_t* code, size_t code_size, 
                         uint8_t* data, size_t data_size);

/**
 * 执行ASTC程序
 * @param vm 虚拟机实例
 * @return 程序退出码
 */
int enhanced_astc_vm_run(EnhancedASTCVM* vm);

/**
 * 单步执行一条指令
 * @param vm 虚拟机实例
 * @return 0继续，1停机，负数错误
 */
int enhanced_astc_vm_step(EnhancedASTCVM* vm);

/**
 * 重置虚拟机状态
 * @param vm 虚拟机实例
 */
void enhanced_astc_vm_reset(EnhancedASTCVM* vm);

/**
 * 清理虚拟机资源
 * @param vm 虚拟机实例
 */
void enhanced_astc_vm_cleanup(EnhancedASTCVM* vm);

/**
 * 设置调试模式
 * @param vm 虚拟机实例
 * @param debug 是否启用调试
 */
void enhanced_astc_vm_set_debug(EnhancedASTCVM* vm, bool debug);

/**
 * 获取虚拟机状态信息
 * @param vm 虚拟机实例
 */
void enhanced_astc_vm_print_status(EnhancedASTCVM* vm);

// ===============================================
// 栈操作辅助函数
// ===============================================

/**
 * 压入值到栈
 */
int astc_vm_push(EnhancedASTCVM* vm, ASTCValue value);

/**
 * 从栈弹出值
 */
ASTCValue astc_vm_pop(EnhancedASTCVM* vm);

/**
 * 查看栈顶值（不弹出）
 */
ASTCValue astc_vm_peek(EnhancedASTCVM* vm);

#ifdef __cplusplus
}
#endif

#endif // ENHANCED_ASTC_VM_H
