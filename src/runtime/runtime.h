/**
 * runtime.h - ASTC虚拟机运行时头文件
 * 
 * 该文件定义了ASTC虚拟机的基本结构和接口，用于执行ASTC格式的程序。
 * Runtime模块是连接Loader和Program的关键组件。
 */

#ifndef RUNTIME_H
#define RUNTIME_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>
#include "astc.h"

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// 常量定义
// ===============================================

#define RUNTIME_VERSION "0.1.0"
#define RUNTIME_STACK_SIZE 1024 * 1024  // 1MB栈大小
#define RUNTIME_HEAP_INITIAL_SIZE 4 * 1024 * 1024  // 4MB初始堆大小
#define RUNTIME_MAX_PERF_EVENTS 100  // 最大性能事件数量

// ===============================================
// 类型定义
// ===============================================

// 虚拟机值类型
typedef enum {
    RT_VAL_I32,      // 32位整数
    RT_VAL_I64,      // 64位整数
    RT_VAL_F32,      // 32位浮点数
    RT_VAL_F64,      // 64位浮点数
    RT_VAL_PTR,      // 指针
    RT_VAL_FUNC_REF  // 函数引用
} RuntimeValueType;

// 虚拟机值
typedef struct {
    RuntimeValueType type;
    union {
        int32_t i32;
        int64_t i64;
        float f32;
        double f64;
        void* ptr;
        struct ASTNode* func_ref;
    } value;
} RuntimeValue;

// 内存管理器
typedef struct {
    uint8_t* stack;          // 栈内存
    size_t stack_size;       // 栈大小
    size_t stack_pointer;    // 栈指针
    
    uint8_t* heap;           // 堆内存
    size_t heap_size;        // 堆大小
    size_t heap_used;        // 已使用堆大小
} RuntimeMemory;

// 函数表项
typedef struct {
    const char* name;        // 函数名
    struct ASTNode* node;    // 函数节点
    bool is_native;          // 是否为原生函数
    void* native_func;       // 原生函数指针
} RuntimeFunctionEntry;

// 函数表
typedef struct {
    RuntimeFunctionEntry* entries;  // 函数表项
    size_t count;                   // 函数数量
    size_t capacity;                // 函数表容量
} RuntimeFunctionTable;

// 全局变量表项
typedef struct {
    const char* name;        // 变量名
    RuntimeValue value;      // 变量值
    bool is_mutable;         // 是否可变
} RuntimeGlobalEntry;

// 全局变量表
typedef struct {
    RuntimeGlobalEntry* entries;    // 全局变量表项
    size_t count;                   // 全局变量数量
    size_t capacity;                // 全局变量表容量
} RuntimeGlobalTable;

// 局部变量映射项
typedef struct {
    const char* name;               // 变量名
    size_t index;                   // 在locals数组中的索引
} RuntimeLocalEntry;

// 性能事件
typedef struct {
    char name[64];                  // 事件名称
    time_t timestamp;               // 时间戳
    size_t instruction_count;       // 事件发生时的指令数
} RuntimePerfEvent;

// 性能统计
typedef struct {
    size_t instruction_count;       // 执行的指令数量
    size_t function_call_count;     // 函数调用次数
    time_t total_execution_time;    // 总执行时间(秒)
} RuntimeStats;

// 调用帧
typedef struct RuntimeCallFrame {
    struct ASTNode* func;           // 当前执行的函数
    RuntimeValue* locals;           // 局部变量
    size_t local_count;             // 局部变量数量
    RuntimeLocalEntry* local_map;   // 局部变量名到索引的映射
    size_t local_map_count;         // 映射项数量
    size_t local_map_capacity;      // 映射表容量
    size_t bp;                      // 基址指针
    size_t ip;                      // 指令指针
    struct RuntimeCallFrame* prev;  // 前一帧
    RuntimeValue return_value;      // 返回值
    bool return_value_set;          // 返回值是否已设置
} RuntimeCallFrame;

// 虚拟机实例
typedef struct {
    RuntimeMemory memory;           // 内存管理器
    RuntimeFunctionTable functions; // 函数表
    RuntimeGlobalTable globals;     // 全局变量表
    RuntimeCallFrame* current_frame;// 当前调用帧
    int exit_code;                  // 退出码
    bool running;                   // 运行状态
    char error_message[256];        // 错误信息
    
    // 调试和性能相关
    bool debug_mode;                // 调试模式
    size_t instruction_count;       // 执行的指令数量
    size_t function_call_count;     // 函数调用次数
    time_t perf_start_time;         // 性能统计开始时间
    RuntimePerfEvent* perf_events;  // 性能事件数组
    size_t perf_event_count;        // 性能事件数量
    size_t perf_event_capacity;     // 性能事件数组容量
} RuntimeVM;

// ===============================================
// 函数声明
// ===============================================

/**
 * 初始化虚拟机
 * 
 * @param vm 虚拟机实例
 * @return 成功返回true，失败返回false
 */
bool runtime_init(RuntimeVM* vm);

/**
 * 销毁虚拟机
 * 
 * @param vm 虚拟机实例
 */
void runtime_destroy(RuntimeVM* vm);

/**
 * 加载ASTC程序
 * 
 * @param vm 虚拟机实例
 * @param root ASTC根节点
 * @return 成功返回true，失败返回false
 */
bool runtime_load_program(RuntimeVM* vm, struct ASTNode* root);

/**
 * 执行ASTC程序
 * 
 * @param vm 虚拟机实例
 * @param entry_point 入口函数名，通常为"main"
 * @return 程序退出码
 */
int runtime_execute(RuntimeVM* vm, const char* entry_point);

/**
 * 注册原生函数
 * 
 * @param vm 虚拟机实例
 * @param name 函数名
 * @param func 函数指针
 * @return 成功返回true，失败返回false
 */
bool runtime_register_native_function(RuntimeVM* vm, const char* name, void* func);

/**
 * Runtime系统调用接口 - 文件操作
 */
int runtime_syscall_read_file(RuntimeVM* vm, const char* filename, char** content, size_t* size);
int runtime_syscall_write_file(RuntimeVM* vm, const char* filename, const char* content, size_t size);
int runtime_syscall_copy_file(RuntimeVM* vm, const char* src, const char* dst);

/**
 * 获取最后一次错误信息
 * 
 * @param vm 虚拟机实例
 * @return 错误信息字符串
 */
const char* runtime_get_error(RuntimeVM* vm);

/**
 * 分配堆内存
 * 
 * @param vm 虚拟机实例
 * @param size 需要分配的字节数
 * @return 分配的内存指针，失败返回NULL
 */
void* runtime_allocate(RuntimeVM* vm, size_t size);

/**
 * 释放堆内存
 * 
 * @param vm 虚拟机实例
 * @param ptr 要释放的内存指针
 */
void runtime_free(RuntimeVM* vm, void* ptr);

/**
 * 创建RuntimeValue
 */
RuntimeValue runtime_value_i32(int32_t value);
RuntimeValue runtime_value_i64(int64_t value);
RuntimeValue runtime_value_f32(float value);
RuntimeValue runtime_value_f64(double value);
RuntimeValue runtime_value_ptr(void* value);
RuntimeValue runtime_value_func_ref(struct ASTNode* value);

/**
 * 设置虚拟机调试模式
 * 
 * @param vm 虚拟机实例
 * @param debug_mode 是否启用调试模式
 */
void runtime_set_debug_mode(RuntimeVM* vm, bool debug_mode);

/**
 * 获取性能统计信息
 * 
 * @param vm 虚拟机实例
 * @return 性能统计信息结构体指针，vm为NULL时返回NULL
 */
const RuntimeStats* runtime_get_stats(RuntimeVM* vm);

#ifdef __cplusplus
}
#endif

#endif // RUNTIME_H 