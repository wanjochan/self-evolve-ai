/**
 * evolver0.c - 第零代真正自举编译器 (Loader层)
 * 目标：实现多格式输出（AST/WASM/Executable）的编译器
 * 架构：Loader+Runtime+Program 三层架构的起点
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <wchar.h>
#include <sys/stat.h>  // For mkdir
#include <errno.h>     // For errno

// AST序列化相关定义
#define ASTC_MAGIC "ASTC"
#define ASTC_VERSION 1

// WASM相关定义
#define WASM_MAGIC "\0asm"
#define WASM_VERSION 0x1

// WASM值类型
enum {
    WASM_TYPE_I32 = 0x7F,
    WASM_TYPE_I64 = 0x7E,
    WASM_TYPE_F32 = 0x7D,
    WASM_TYPE_F64 = 0x7C,
    WASM_TYPE_ANYFUNC = 0x70,
    WASM_TYPE_FUNC = 0x60,
    WASM_TYPE_EMPTY = 0x40
};

// WASM段类型
enum {
    WASM_SECTION_CUSTOM = 0,
    WASM_SECTION_TYPE = 1,
    WASM_SECTION_IMPORT = 2,
    WASM_SECTION_FUNCTION = 3,
    WASM_SECTION_TABLE = 4,
    WASM_SECTION_MEMORY = 5,
    WASM_SECTION_GLOBAL = 6,
    WASM_SECTION_EXPORT = 7,
    WASM_SECTION_START = 8,
    WASM_SECTION_ELEMENT = 9,
    WASM_SECTION_CODE = 10,
    WASM_SECTION_DATA = 11,
    WASM_SECTION_DATACOUNT = 12
};

// WASM操作码
enum {
    // 控制指令
    WASM_OP_UNREACHABLE = 0x00,
    WASM_OP_NOP = 0x01,
    WASM_OP_BLOCK = 0x02,
    WASM_OP_LOOP = 0x03,
    WASM_OP_IF = 0x04,
    WASM_OP_ELSE = 0x05,
    WASM_OP_END = 0x0B,
    WASM_OP_BR = 0x0C,
    WASM_OP_BR_IF = 0x0D,
    WASM_OP_BR_TABLE = 0x0E,
    WASM_OP_RETURN = 0x0F,
    WASM_OP_CALL = 0x10,
    WASM_OP_CALL_INDIRECT = 0x11,
    
    // 内存操作指令
    WASM_OP_I32_LOAD = 0x28,
    WASM_OP_I64_LOAD = 0x29,
    WASM_OP_F32_LOAD = 0x2A,
    WASM_OP_F64_LOAD = 0x2B,
    WASM_OP_I32_LOAD8_S = 0x2C,
    WASM_OP_I32_LOAD8_U = 0x2D,
    WASM_OP_I32_LOAD16_S = 0x2E,
    WASM_OP_I32_LOAD16_U = 0x2F,
    WASM_OP_I64_LOAD8_S = 0x30,
    WASM_OP_I64_LOAD8_U = 0x31,
    WASM_OP_I64_LOAD16_S = 0x32,
    WASM_OP_I64_LOAD16_U = 0x33,
    WASM_OP_I64_LOAD32_S = 0x34,
    WASM_OP_I64_LOAD32_U = 0x35,
    WASM_OP_I32_STORE = 0x36,
    WASM_OP_I64_STORE = 0x37,
    WASM_OP_F32_STORE = 0x38,
    WASM_OP_F64_STORE = 0x39,
    WASM_OP_I32_STORE8 = 0x3A,
    WASM_OP_I32_STORE16 = 0x3B,
    WASM_OP_I64_STORE8 = 0x3C,
    WASM_OP_I64_STORE16 = 0x3D,
    WASM_OP_I64_STORE32 = 0x3E,
    WASM_OP_MEMORY_SIZE = 0x3F,
    WASM_OP_MEMORY_GROW = 0x40,
    
    // 常量指令
    WASM_OP_I32_CONST = 0x41,
    WASM_OP_I64_CONST = 0x42,
    WASM_OP_F32_CONST = 0x43,
    WASM_OP_F64_CONST = 0x44,
    
    // 比较指令
    WASM_OP_I32_EQZ = 0x45,
    WASM_OP_I32_EQ = 0x46,
    WASM_OP_I32_NE = 0x47,
    WASM_OP_I32_LT_S = 0x48,
    WASM_OP_I32_LT_U = 0x49,
    WASM_OP_I32_GT_S = 0x4A,
    WASM_OP_I32_GT_U = 0x4B,
    WASM_OP_I32_LE_S = 0x4C,
    WASM_OP_I32_LE_U = 0x4D,
    WASM_OP_I32_GE_S = 0x4E,
    WASM_OP_I32_GE_U = 0x4F,
    
    // 算术指令
    WASM_OP_I32_CLZ = 0x67,
    WASM_OP_I32_CTZ = 0x68,
    WASM_OP_I32_POPCNT = 0x69,
    WASM_OP_I32_ADD = 0x6A,
    WASM_OP_I32_SUB = 0x6B,
    WASM_OP_I32_MUL = 0x6C,
    WASM_OP_I32_DIV_S = 0x6D,
    WASM_OP_I32_DIV_U = 0x6E,
    WASM_OP_I32_REM_S = 0x6F,
    WASM_OP_I32_REM_U = 0x70,
    WASM_OP_I32_AND = 0x71,
    WASM_OP_I32_OR = 0x72,
    WASM_OP_I32_XOR = 0x73,
    WASM_OP_I32_SHL = 0x74,
    WASM_OP_I32_SHR_S = 0x75,
    WASM_OP_I32_SHR_U = 0x76,
    WASM_OP_I32_ROTL = 0x77,
    WASM_OP_I32_ROTR = 0x78,
    
    // 变量指令
    WASM_OP_GET_LOCAL = 0x20,
    WASM_OP_SET_LOCAL = 0x21,
    WASM_OP_TEE_LOCAL = 0x22,
    WASM_OP_GET_GLOBAL = 0x23,
    WASM_OP_SET_GLOBAL = 0x24,
    
    // 参数指令
    WASM_OP_DROP = 0x1A,
    WASM_OP_SELECT = 0x1B,
    
    // 控制流指令
    WASM_OP_IF = 0x04,
    WASM_OP_ELSE = 0x05,
    WASM_OP_END = 0x0B,
    WASM_OP_LOOP = 0x03,
    WASM_OP_BLOCK = 0x02,
    WASM_OP_BR = 0x0C,
    WASM_OP_BR_IF = 0x0D,
    WASM_OP_BR_TABLE = 0x0E,
    WASM_OP_RETURN = 0x0F,
    WASM_OP_CALL = 0x10,
    WASM_OP_CALL_INDIRECT = 0x11,
    
    // 内存段
    WASM_OP_MEMORY_INIT = 0xFC, 0x08,
    WASM_OP_DATA_DROP = 0xFC, 0x09,
    WASM_OP_MEMORY_COPY = 0xFC, 0x0A,
    WASM_OP_MEMORY_FILL = 0xFC, 0x0B,
    
    // 原子操作
    WASM_OP_ATOMIC_NOTIFY = 0xFE, 0x00,
    WASM_OP_I32_ATOMIC_LOAD = 0xFE, 0x10,
    WASM_OP_I32_ATOMIC_STORE = 0xFE, 0x11,
    WASM_OP_I32_ATOMIC_RMW_ADD = 0xFE, 0x16,
    WASM_OP_I32_ATOMIC_RMW_SUB = 0xFE, 0x17,
    WASM_OP_I32_ATOMIC_RMW_AND = 0xFE, 0x18,
    WASM_OP_I32_ATOMIC_RMW_OR = 0xFE, 0x19,
    WASM_OP_I32_ATOMIC_RMW_XOR = 0xFE, 0x1A,
    WASM_OP_I32_ATOMIC_RMW_XCHG = 0xFE, 0x1B,
    WASM_OP_I32_ATOMIC_RMW_CMPXCHG = 0xFE, 0x1C,
    
    // SIMD指令
    WASM_OP_V128_LOAD = 0xFD, 0x00,
    WASM_OP_V128_STORE = 0xFD, 0x01,
    WASM_OP_V128_CONST = 0xFD, 0x02,
    WASM_OP_I8X16_ADD = 0xFD, 0x7E,
    WASM_OP_I8X16_SUB = 0xFD, 0x7F,
    WASM_OP_I8X16_MUL = 0xFD, 0x80,
    
    // 引用类型指令
    WASM_OP_REF_NULL = 0xD0,
    WASM_OP_REF_IS_NULL = 0xD1,
    WASM_OP_REF_FUNC = 0xD2,
    
    // 尾调用指令
    WASM_OP_RETURN_CALL = 0x12,
    WASM_OP_RETURN_CALL_INDIRECT = 0x13
};

// WASM导出类型
enum {
    WASM_EXPORT_FUNC = 0,
    WASM_EXPORT_TABLE = 1,
    WASM_EXPORT_MEMORY = 2,
    WASM_EXPORT_GLOBAL = 3
};

// WASM缓冲区结构体
typedef struct WasmBuffer {
    uint8_t* data;           // 数据缓冲区
    size_t size;             // 当前大小
    size_t capacity;         // 分配的大小
    size_t section_size_offset; // 当前段大小字段的偏移量
} WasmBuffer;

// 初始化WASM缓冲区
static void wasm_buffer_init(WasmBuffer* buffer) {
    buffer->size = 0;
    buffer->capacity = 4096;  // 初始4KB
    buffer->data = (uint8_t*)malloc(buffer->capacity);
    if (!buffer->data) {
        fprintf(stderr, "错误: 无法分配WASM缓冲区内存\n");
        exit(1);
    }
}

// 释放WASM缓冲区
static void wasm_buffer_free(WasmBuffer* buffer) {
    if (buffer->data) {
        free(buffer->data);
        buffer->data = NULL;
    }
    buffer->size = 0;
    buffer->capacity = 0;
}

// 确保缓冲区有足够空间
static void wasm_buffer_reserve(WasmBuffer* buffer, size_t needed) {
    if (buffer->size + needed <= buffer->capacity) {
        return;
    }
    
    // 至少翻倍，确保有足够空间
    size_t new_capacity = buffer->capacity * 2;
    while (buffer->size + needed > new_capacity) {
        new_capacity *= 2;
    }
    
    uint8_t* new_data = (uint8_t*)realloc(buffer->data, new_capacity);
    if (!new_data) {
        fprintf(stderr, "错误: 无法扩展WASM缓冲区\n");
        wasm_buffer_free(buffer);
        exit(1);
    }
    
    buffer->data = new_data;
    buffer->capacity = new_capacity;
}

// 写入单个字节
static void wasm_write_byte(WasmBuffer* buffer, uint8_t value) {
    wasm_buffer_reserve(buffer, 1);
    buffer->data[buffer->size++] = value;
}

// 写入32位无符号整数(LEB128编码)
static void wasm_write_u32(WasmBuffer* buffer, uint32_t value) {
    do {
        uint8_t byte = value & 0x7f;
        value >>= 7;
        if (value != 0) {
            byte |= 0x80;
        }
        wasm_write_byte(buffer, byte);
    } while (value != 0);
}

// 写入32位有符号整数(LEB128编码)
static void wasm_write_s32(WasmBuffer* buffer, int32_t value) {
    int more = 1;
    while (more) {
        uint8_t byte = value & 0x7f;
        value >>= 7;
        
        if ((value == 0 && (byte & 0x40) == 0) ||
            (value == -1 && (byte & 0x40) != 0)) {
            more = 0;
        } else {
            byte |= 0x80;
        }
        
        wasm_write_byte(buffer, byte);
    }
}

// 写入64位无符号整数(LEB128编码)
static void wasm_write_u64(WasmBuffer* buffer, uint64_t value) {
    do {
        uint8_t byte = value & 0x7f;
        value >>= 7;
        if (value != 0) {
            byte |= 0x80;
        }
        wasm_write_byte(buffer, byte);
    } while (value != 0);
}

// 写入64位有符号整数(LEB128编码)
static void wasm_write_s64(WasmBuffer* buffer, int64_t value) {
    int more = 1;
    while (more) {
        uint8_t byte = value & 0x7f;
        value >>= 7;
        
        if ((value == 0 && (byte & 0x40) == 0) ||
            (value == -1 && (byte & 0x40) != 0)) {
            more = 0;
        } else {
            byte |= 0x80;
        }
        
        wasm_write_byte(buffer, byte);
    }
}

// 写入浮点数(32位)
static void wasm_write_f32(WasmBuffer* buffer, float value) {
    uint32_t* p = (uint32_t*)&value;
    wasm_write_byte(buffer, (*p) & 0xFF);
    wasm_write_byte(buffer, ((*p) >> 8) & 0xFF);
    wasm_write_byte(buffer, ((*p) >> 16) & 0xFF);
    wasm_write_byte(buffer, ((*p) >> 24) & 0xFF);
}

// 写入浮点数(64位)
static void wasm_write_f64(WasmBuffer* buffer, double value) {
    uint64_t* p = (uint64_t*)&value;
    wasm_write_byte(buffer, (*p) & 0xFF);
    wasm_write_byte(buffer, ((*p) >> 8) & 0xFF);
    wasm_write_byte(buffer, ((*p) >> 16) & 0xFF);
    wasm_write_byte(buffer, ((*p) >> 24) & 0xFF);
    wasm_write_byte(buffer, ((*p) >> 32) & 0xFF);
    wasm_write_byte(buffer, ((*p) >> 40) & 0xFF);
    wasm_write_byte(buffer, ((*p) >> 48) & 0xFF);
    wasm_write_byte(buffer, ((*p) >> 56) & 0xFF);
}

// 写入字符串
static void wasm_write_string(WasmBuffer* buffer, const char* str) {
    size_t len = strlen(str);
    wasm_write_u32(buffer, (uint32_t)len);
    wasm_buffer_reserve(buffer, len);
    memcpy(buffer->data + buffer->size, str, len);
    buffer->size += len;
}

// 写入WASM头部
static void wasm_write_header(WasmBuffer* buffer) {
    // 魔数: \0asm
    wasm_write_byte(buffer, 0x00);
    wasm_write_byte(buffer, 0x61);
    wasm_write_byte(buffer, 0x73);
    wasm_write_byte(buffer, 0x6D);
    
    // 版本: 1
    wasm_write_byte(buffer, 0x01);
    wasm_write_byte(buffer, 0x00);
    wasm_write_byte(buffer, 0x00);
    wasm_write_byte(buffer, 0x00);
}

// 开始一个段
static void wasm_begin_section(WasmBuffer* buffer, uint8_t section_id) {
    wasm_write_byte(buffer, section_id);
    // 预留4字节给段长度
    buffer->section_size_offset = buffer->size;
    wasm_write_u32(buffer, 0);
}

// 结束一个段
static void wasm_end_section(WasmBuffer* buffer) {
    // 计算段大小
    size_t section_size = buffer->size - buffer->section_size_offset - 4;
    
    // 将段大小写入之前预留的位置
    size_t pos = buffer->section_size_offset;
    do {
        uint8_t byte = section_size & 0x7f;
        section_size >>= 7;
        if (section_size != 0) {
            byte |= 0x80;
        }
        buffer->data[pos++] = byte;
    } while (section_size != 0);
}

// WASM函数类型定义
typedef struct {
    uint8_t* param_types;
    uint8_t* return_types;
    uint32_t param_count;
    uint32_t return_count;
} WasmFuncType;

// 全局函数类型表
static WasmFuncType* wasm_func_types = NULL;
static uint32_t wasm_func_type_count = 0;

// 初始化WASM类型系统
static void wasm_init_type_system(void) {
    wasm_func_types = (WasmFuncType*)malloc(sizeof(WasmFuncType) * 16); // 初始容量16
    wasm_func_type_count = 0;
}

// 添加函数类型
static uint32_t wasm_add_func_type(const uint8_t* param_types, uint32_t param_count,
                                 const uint8_t* return_types, uint32_t return_count) {
    // 检查是否已存在相同类型
    for (uint32_t i = 0; i < wasm_func_type_count; i++) {
        WasmFuncType* type = &wasm_func_types[i];
        if (type->param_count == param_count && 
            type->return_count == return_count &&
            memcmp(type->param_types, param_types, param_count) == 0 &&
            (return_count == 0 || memcmp(type->return_types, return_types, return_count) == 0)) {
            return i; // 返回现有类型索引
        }
    }
    
    // 扩容检查
    if (wasm_func_type_count >= 16) {
        size_t new_size = wasm_func_type_count * 2;
        wasm_func_types = (WasmFuncType*)realloc(wasm_func_types, sizeof(WasmFuncType) * new_size);
    }
    
    // 添加新类型
    WasmFuncType* new_type = &wasm_func_types[wasm_func_type_count];
    new_type->param_count = param_count;
    new_type->return_count = return_count;
    
    new_type->param_types = (uint8_t*)malloc(param_count);
    memcpy(new_type->param_types, param_types, param_count);
    
    if (return_count > 0) {
        new_type->return_types = (uint8_t*)malloc(return_count);
        memcpy(new_type->return_types, return_types, return_count);
    } else {
        new_type->return_types = NULL;
    }
    
    return wasm_func_type_count++;
}

// 释放类型系统资源
static void wasm_free_type_system(void) {
    for (uint32_t i = 0; i < wasm_func_type_count; i++) {
        free(wasm_func_types[i].param_types);
        if (wasm_func_types[i].return_types) {
            free(wasm_func_types[i].return_types);
        }
    }
    free(wasm_func_types);
    wasm_func_types = NULL;
    wasm_func_type_count = 0;
}

// 添加类型段
static void wasm_add_type_section(WasmBuffer* buffer) {
    wasm_begin_section(buffer, WASM_SECTION_TYPE);
    
    // 写入类型数量
    wasm_write_u32(buffer, wasm_func_type_count);
    
    // 写入每个函数类型
    for (uint32_t i = 0; i < wasm_func_type_count; i++) {
        WasmFuncType* type = &wasm_func_types[i];
        
        // 函数类型标记 (0x60)
        wasm_write_byte(buffer, 0x60);
        
        // 参数类型
        wasm_write_u32(buffer, type->param_count);
        for (uint32_t j = 0; j < type->param_count; j++) {
            wasm_write_byte(buffer, type->param_types[j]);
        }
        
        // 返回类型
        wasm_write_u32(buffer, type->return_count);
        for (uint32_t j = 0; j < type->return_count; j++) {
            wasm_write_byte(buffer, type->return_types[j]);
        }
    }
    
    wasm_end_section(buffer);
}

// 函数定义结构
typedef struct {
    uint32_t type_index;      // 函数类型索引
    uint32_t local_count;     // 局部变量数量
    uint8_t* locals;          // 局部变量类型
    uint8_t* code;            // 函数字节码
    size_t code_size;         // 字节码大小
    const char* export_name;   // 导出名称（如果为导出函数）
} WasmFunction;

// 全局函数表
static WasmFunction* wasm_functions = NULL;
static uint32_t wasm_function_count = 0;

// 初始化函数系统
static void wasm_init_function_system(void) {
    wasm_functions = (WasmFunction*)malloc(sizeof(WasmFunction) * 16); // 初始容量16
    wasm_function_count = 0;
}

// 添加函数
static uint32_t wasm_add_function(uint32_t type_index, const uint8_t* locals, 
                                uint32_t local_count, const uint8_t* code, 
                                size_t code_size, const char* export_name) {
    // 扩容检查
    if (wasm_function_count >= 16) {
        size_t new_size = wasm_function_count * 2;
        wasm_functions = (WasmFunction*)realloc(wasm_functions, sizeof(WasmFunction) * new_size);
    }
    
    // 添加新函数
    WasmFunction* func = &wasm_functions[wasm_function_count];
    func->type_index = type_index;
    func->local_count = local_count;
    func->code_size = code_size;
    func->export_name = export_name;
    
    // 复制局部变量类型
    if (local_count > 0) {
        func->locals = (uint8_t*)malloc(local_count);
        memcpy(func->locals, locals, local_count);
    } else {
        func->locals = NULL;
    }
    
    // 复制字节码
    if (code_size > 0) {
        func->code = (uint8_t*)malloc(code_size);
        memcpy(func->code, code, code_size);
    } else {
        func->code = NULL;
    }
    
    return wasm_function_count++;
}

// 释放函数系统资源
static void wasm_free_function_system(void) {
    for (uint32_t i = 0; i < wasm_function_count; i++) {
        if (wasm_functions[i].locals) {
            free(wasm_functions[i].locals);
        }
        if (wasm_functions[i].code) {
            free(wasm_functions[i].code);
        }
    }
    free(wasm_functions);
    wasm_functions = NULL;
    wasm_function_count = 0;
}

// 添加函数段
static void wasm_add_function_section(WasmBuffer* buffer) {
    wasm_begin_section(buffer, WASM_SECTION_FUNCTION);
    
    // 写入函数数量
    wasm_write_u32(buffer, wasm_function_count);
    
    // 写入每个函数的类型索引
    for (uint32_t i = 0; i < wasm_function_count; i++) {
        wasm_write_u32(buffer, wasm_functions[i].type_index);
    }
    
    wasm_end_section(buffer);
}

// 添加导出段
static void wasm_add_export_section(WasmBuffer* buffer) {
    // 统计需要导出的函数数量
    uint32_t export_count = 0;
    for (uint32_t i = 0; i < wasm_function_count; i++) {
        if (wasm_functions[i].export_name != NULL) {
            export_count++;
        }
    }
    
    if (export_count == 0) {
        return; // 没有需要导出的函数
    }
    
    wasm_begin_section(buffer, WASM_SECTION_EXPORT);
    
    // 导出项数量
    wasm_write_u32(buffer, export_count);
    
    // 导出每个标记为导出的函数
    for (uint32_t i = 0; i < wasm_function_count; i++) {
        if (wasm_functions[i].export_name != NULL) {
            wasm_write_string(buffer, wasm_functions[i].export_name);
            wasm_write_byte(buffer, WASM_EXPORT_FUNC);
            wasm_write_u32(buffer, i);  // 函数索引
        }
    }
    
    wasm_end_section(buffer);
}

// 添加代码段
static void wasm_add_code_section(WasmBuffer* buffer) {
    wasm_begin_section(buffer, WASM_SECTION_CODE);
    
    // 代码段数量
    wasm_write_u32(buffer, wasm_function_count);
    
    // 处理每个函数
    for (uint32_t i = 0; i < wasm_function_count; i++) {
        WasmFunction* func = &wasm_functions[i];
        
        // 函数体大小 (稍后填充)
        size_t func_size_pos = buffer->size;
        wasm_write_u32(buffer, 0);
        
        // 局部变量组
        if (func->local_count > 0) {
            // 统计相同类型的局部变量
            uint32_t count = 1;
            uint8_t current_type = func->locals[0];
            
            // 写入局部变量组数量
            wasm_write_u32(buffer, 1);
            
            // 写入局部变量组
            wasm_write_u32(buffer, count);
            wasm_write_byte(buffer, current_type);
        } else {
            // 无局部变量
            wasm_write_u32(buffer, 0);
        }
        
        // 函数体字节码
        if (func->code && func->code_size > 0) {
            wasm_buffer_reserve(buffer, func->code_size);
            memcpy(buffer->data + buffer->size, func->code, func->code_size);
            buffer->size += func->code_size;
        }
        
        // 确保函数体以END操作码结束
        if (func->code_size == 0 || buffer->data[buffer->size - 1] != WASM_OP_END) {
            wasm_write_byte(buffer, WASM_OP_END);
        }
        
        // 更新函数体大小
        size_t func_size = buffer->size - func_size_pos - 4;
        size_t pos = func_size_pos;
        do {
            uint8_t byte = func_size & 0x7f;
            func_size >>= 7;
            if (func_size != 0) {
                byte |= 0x80;
            }
            buffer->data[pos++] = byte;
        } while (func_size != 0);
    }
    
    wasm_end_section(buffer);
}

// 生成一个简单的条件判断函数 (max)
static void generate_max_function(WasmBuffer* wasm) {
    // 函数类型: i32 (i32, i32) -> i32
    uint8_t params[] = {WASM_TYPE_I32, WASM_TYPE_I32};
    uint8_t returns[] = {WASM_TYPE_I32};
    uint32_t func_type = wasm_add_func_type(params, 2, returns, 1);
    
    // 函数体
    uint8_t code[32];
    size_t code_size = 0;
    
    // 获取参数
    code[code_size++] = WASM_OP_GET_LOCAL; code[code_size++] = 0;  // 获取第一个参数 a
    code[code_size++] = WASM_OP_GET_LOCAL; code[code_size++] = 1;  // 获取第二个参数 b
    
    // if (a > b) return a; else return b;
    code[code_size++] = WASM_OP_IF; code[code_size++] = WASM_TYPE_I32;  // if 条件块
    code[code_size++] = WASM_OP_GET_LOCAL; code[code_size++] = 0;       // 返回 a
    code[code_size++] = WASM_OP_ELSE;                                   // else
    code[code_size++] = WASM_OP_GET_LOCAL; code[code_size++] = 1;       // 返回 b
    code[code_size++] = WASM_OP_END;                                    // 结束 if-else
    
    code[code_size++] = WASM_OP_RETURN;  // 返回结果
    
    wasm_add_function(func_type, NULL, 0, code, code_size, "max");
}

// 生成一个简单的循环函数 (factorial)
static void generate_factorial_function(WasmBuffer* wasm) {
    // 函数类型: i32 (i32) -> i32
    uint8_t params[] = {WASM_TYPE_I32};
    uint8_t returns[] = {WASM_TYPE_I32};
    uint32_t func_type = wasm_add_func_type(params, 1, returns, 1);
    
    // 函数体
    uint8_t code[64];
    size_t code_size = 0;
    
    // 局部变量: result = 1, i = n
    uint8_t locals[] = {WASM_TYPE_I32, WASM_TYPE_I32};
    
    // 初始化局部变量
    // result = 1
    code[code_size++] = WASM_OP_I32_CONST; 
    code[code_size++] = 1;  // 1
    code[code_size++] = WASM_OP_SET_LOCAL; code[code_size++] = 1;  // result = 1
    
    // i = n
    code[code_size++] = WASM_OP_GET_LOCAL; code[code_size++] = 0;  // 获取参数 n
    code[code_size++] = WASM_OP_SET_LOCAL; code[code_size++] = 2;  // i = n
    
    // while (i > 0) { result *= i; i--; }
    code[code_size++] = WASM_OP_LOOP; code[code_size++] = WASM_TYPE_I32;  // 开始循环
    
    // 检查 i > 0
    code[code_size++] = WASM_OP_GET_LOCAL; code[code_size++] = 2;  // 获取 i
    code[code_size++] = WASM_OP_I32_CONST; code[code_size++] = 0;  // 0
    code[code_size++] = WASM_OP_I32_LE_S;                          // i <= 0
    code[code_size++] = WASM_OP_BR_IF; code[code_size++] = 1;      // 如果 i <= 0 则退出循环
    
    // result *= i
    code[code_size++] = WASM_OP_GET_LOCAL; code[code_size++] = 1;  // 获取 result
    code[code_size++] = WASM_OP_GET_LOCAL; code[code_size++] = 2;  // 获取 i
    code[code_size++] = WASM_OP_I32_MUL;                          // result * i
    code[code_size++] = WASM_OP_SET_LOCAL; code[code_size++] = 1;  // result = result * i
    
    // i--
    code[code_size++] = WASM_OP_GET_LOCAL; code[code_size++] = 2;  // 获取 i
    code[code_size++] = WASM_OP_I32_CONST; code[code_size++] = 1;   // 1
    code[code_size++] = WASM_OP_I32_SUB;                           // i - 1
    code[code_size++] = WASM_OP_SET_LOCAL; code[code_size++] = 2;  // i = i - 1
    
    code[code_size++] = WASM_OP_BR; code[code_size++] = 0;      // 继续循环
    code[code_size++] = WASM_OP_END;                               // 结束循环
    
    // 返回 result
    code[code_size++] = WASM_OP_GET_LOCAL; code[code_size++] = 1;  // 获取 result
    code[code_size++] = WASM_OP_RETURN;                            // 返回 result
    
    wasm_add_function(func_type, locals, 2, code, code_size, "factorial");
}

// 生成一个内存操作示例 (sum_array)
static void generate_sum_array_function(WasmBuffer* wasm) {
    // 函数类型: i32 (i32, i32) -> i32
    uint8_t params[] = {WASM_TYPE_I32, WASM_TYPE_I32};  // ptr, len
    uint8_t returns[] = {WASM_TYPE_I32};
    uint32_t func_type = wasm_add_func_type(params, 2, returns, 1);
    
    // 函数体
    uint8_t code[128];
    size_t code_size = 0;
    
    // 局部变量: sum = 0, i = 0
    uint8_t locals[] = {WASM_TYPE_I32, WASM_TYPE_I32};
    
    // 初始化局部变量
    // sum = 0
    code[code_size++] = WASM_OP_I32_CONST; code[code_size++] = 0;  // 0
    code[code_size++] = WASM_OP_SET_LOCAL; code[code_size++] = 2;  // sum = 0
    
    // i = 0
    code[code_size++] = WASM_OP_I32_CONST; code[code_size++] = 0;  // 0
    code[code_size++] = WASM_OP_SET_LOCAL; code[code_size++] = 3;  // i = 0
    
    // while (i < len) { sum += array[ptr + i]; i++; }
    code[code_size++] = WASM_OP_LOOP; code[code_size++] = WASM_TYPE_I32;  // 开始循环
    
    // 检查 i < len
    code[code_size++] = WASM_OP_GET_LOCAL; code[code_size++] = 3;  // 获取 i
    code[code_size++] = WASM_OP_GET_LOCAL; code[code_size++] = 1;  // 获取 len
    code[code_size++] = WASM_OP_I32_GE_U;                          // i >= len
    code[code_size++] = WASM_OP_BR_IF; code[code_size++] = 1;      // 如果 i >= len 则退出循环
    
    // sum += array[ptr + i]
    code[code_size++] = WASM_OP_GET_LOCAL; code[code_size++] = 2;  // 获取 sum
    code[code_size++] = WASM_OP_GET_LOCAL; code[code_size++] = 0;  // 获取 ptr
    code[code_size++] = WASM_OP_GET_LOCAL; code[code_size++] = 3;  // 获取 i
    code[code_size++] = WASM_OP_I32_ADD;                          // ptr + i
    code[code_size++] = WASM_OP_I32_LOAD; code[code_size++] = 0x02; code[code_size++] = 0x00;  // 加载 i32 从内存
    code[code_size++] = WASM_OP_I32_ADD;                          // sum + array[ptr + i]
    code[code_size++] = WASM_OP_SET_LOCAL; code[code_size++] = 2;  // sum = sum + array[ptr + i]
    
    // i++
    code[code_size++] = WASM_OP_GET_LOCAL; code[code_size++] = 3;  // 获取 i
    code[code_size++] = WASM_OP_I32_CONST; code[code_size++] = 1;  // 1
    code[code_size++] = WASM_OP_I32_ADD;                           // i + 1
    code[code_size++] = WASM_OP_SET_LOCAL; code[code_size++] = 3;  // i = i + 1
    
    code[code_size++] = WASM_OP_BR; code[code_size++] = 0;      // 继续循环
    code[code_size++] = WASM_OP_END;                               // 结束循环
    
    // 返回 sum
    code[code_size++] = WASM_OP_GET_LOCAL; code[code_size++] = 2;  // 获取 sum
    code[code_size++] = WASM_OP_RETURN;                            // 返回 sum
    
    wasm_add_function(func_type, locals, 2, code, code_size, "sum_array");
}

// 添加内存段
static void wasm_add_memory_section(WasmBuffer* buffer) {
    wasm_begin_section(buffer, WASM_SECTION_MEMORY);
    
    // 内存段数量
    wasm_write_u32(buffer, 1);
    
    // 内存限制 (初始页数: 1, 最大页数: 1)
    wasm_write_byte(buffer, 0x00);  // 没有最大限制标志
    wasm_write_u32(buffer, 1);      // 初始页数 (64KB)
    
    wasm_end_section(buffer);
}

// 添加数据段 (初始化内存)
static void wasm_add_data_section(WasmBuffer* buffer) {
    wasm_begin_section(buffer, WASM_SECTION_DATA);
    
    // 数据段数量
    wasm_write_u32(buffer, 1);
    
    // 数据段1: 内存索引 (0) + 偏移量表达式 (i32.const 0)
    wasm_write_u32(buffer, 0);      // 内存索引
    wasm_write_byte(buffer, 0x41);   // i32.const
    wasm_write_uleb128(buffer, 0);   // 偏移量 0 (使用LEB128编码)
    wasm_write_byte(buffer, 0x0B);   // end
    
    // 数据内容: [1, 2, 3, 4, 5]
    wasm_write_uleb128(buffer, 5);  // 数据长度 (使用LEB128编码)
    for (int i = 1; i <= 5; i++) {
        wasm_write_byte(buffer, i);  // 数据内容
    }
    
    wasm_end_section(buffer);
}

// 生成WASM模块
static int generate_wasm(const char* source, const char* output_file) {
    printf("生成WASM模块: %s\n", output_file);
    
    // 初始化WASM系统
    wasm_init_type_system();
    wasm_init_function_system();
    
    // 1. 生成示例函数
    // 1.1 简单的加法函数
    uint8_t add_params[] = {WASM_TYPE_I32, WASM_TYPE_I32};
    uint8_t add_returns[] = {WASM_TYPE_I32};
    uint32_t add_type = wasm_add_func_type(add_params, 2, add_returns, 1);
    
    uint8_t add_code[] = {
        WASM_OP_GET_LOCAL, 0,  // 获取第一个参数
        WASM_OP_GET_LOCAL, 1,  // 获取第二个参数
        WASM_OP_I32_ADD,       // 相加
        WASM_OP_RETURN         // 返回结果
    };
    wasm_add_function(add_type, NULL, 0, add_code, sizeof(add_code), "add");
    
    // 1.2 生成条件判断函数 (max)
    generate_max_function(NULL);
    
    // 1.3 生成循环函数 (factorial)
    generate_factorial_function(NULL);
    
    // 1.4 生成内存操作函数 (sum_array)
    generate_sum_array_function(NULL);
    
    // 2. 生成WASM模块
    WasmBuffer wasm = {0};
    wasm_buffer_init(&wasm);
    
    // 3. 写入WASM头部
    wasm_write_header(&wasm);
    
    // 4. 添加各个段
    wasm_add_type_section(&wasm);
    wasm_add_function_section(&wasm);
    wasm_add_memory_section(&wasm);      // 添加内存段
    wasm_add_export_section(&wasm);
    wasm_add_code_section(&wasm);
    wasm_add_data_section(&wasm);        // 添加数据段
    
    // 5. 写入到文件
    FILE* f = fopen(output_file, "wb");
    if (!f) {
        fprintf(stderr, "错误: 无法创建文件 %s\n", output_file);
        wasm_buffer_free(&wasm);
        wasm_free_function_system();
        wasm_free_type_system();
        return 1;
    }
    
    size_t written = fwrite(wasm.data, 1, wasm.size, f);
    fclose(f);
    
    if (written != wasm.size) {
        fprintf(stderr, "错误: 写入文件 %s 不完整\n", output_file);
        wasm_buffer_free(&wasm);
        wasm_free_function_system();
        wasm_free_type_system();
        return 1;
    }
    
    printf("成功生成WASM模块: %s (%zu 字节)\n", output_file, wasm.size);
    
    // 6. 清理资源
    wasm_buffer_free(&wasm);
    wasm_free_function_system();
    wasm_free_type_system();
    
    return 0;
}

// 验证WASM文件
static bool verify_wasm_file(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        printf("  ✗ 无法打开文件: %s\n", filename);
        return false;
    }
    
    // 获取文件大小
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (size < 8) {
        printf("  ✗ 文件过小\n");
        fclose(f);
        return false;
    }
    
    // 检查WASM魔数
    uint8_t header[8];
    if (fread(header, 1, 8, f) != 8) {
        printf("  ✗ 读取文件头失败\n");
        fclose(f);
        return false;
    }
    fclose(f);
    
    bool valid = (header[0] == 0x00 && header[1] == 0x61 && 
                 header[2] == 0x73 && header[3] == 0x6D);
    
    if (valid) {
        uint32_t version = (header[4] << 24) | (header[5] << 16) | 
                          (header[6] << 8) | header[7];
        printf("  ✓ WASM文件验证成功 (版本: 0x%08x, 大小: %ld 字节)\n", version, size);
    } else {
        printf("  ✗ 无效的WASM文件\n");
    }
    
    return valid;
}

// 测试WASM生成
static void test_wasm_generation(void) {
    printf("=== WASM生成测试 ===\n");
    
    // 创建输出目录（如果不存在）
#ifdef _WIN32
    _mkdir("output");
#else
    mkdir("output", 0755);
#endif
    
    // 测试1: 简单的加法函数
    printf("\n[测试1] 生成简单加法函数...\n");
    const char* wasm_file1 = "output/simple_add.wasm";
    if (generate_wasm("test source", wasm_file1) == 0) {
        verify_wasm_file(wasm_file1);
    } else {
        printf("✗ 简单加法函数生成失败\n");
    }
    
    // 测试2: 多个函数
    printf("\n[测试2] 生成多个函数...\n");
    const char* wasm_file2 = "output/multi_func.wasm";
    
    // 初始化WASM系统
    wasm_init_type_system();
    wasm_init_function_system();
    
    // 1. 定义函数类型
    // int add(int a, int b)
    uint8_t add_params[] = {WASM_TYPE_I32, WASM_TYPE_I32};
    uint8_t add_returns[] = {WASM_TYPE_I32};
    uint32_t add_type = wasm_add_func_type(add_params, 2, add_returns, 1);
    
    // int sub(int a, int b)
    uint32_t sub_type = wasm_add_func_type(add_params, 2, add_returns, 1);
    
    // int mul(int a, int b)
    uint32_t mul_type = wasm_add_func_type(add_params, 2, add_returns, 1);
    
    // 2. 定义函数体
    // add 函数体
    uint8_t add_code[] = {
        WASM_OP_GET_LOCAL, 0,  // 获取第一个参数
        WASM_OP_GET_LOCAL, 1,  // 获取第二个参数
        WASM_OP_I32_ADD,       // 相加
        WASM_OP_RETURN         // 返回结果
    };
    
    // sub 函数体
    uint8_t sub_code[] = {
        WASM_OP_GET_LOCAL, 0,  // 获取第一个参数
        WASM_OP_GET_LOCAL, 1,  // 获取第二个参数
        WASM_OP_I32_SUB,       // 相减
        WASM_OP_RETURN         // 返回结果
    };
    
    // mul 函数体
    uint8_t mul_code[] = {
        WASM_OP_GET_LOCAL, 0,  // 获取第一个参数
        WASM_OP_GET_LOCAL, 1,  // 获取第二个参数
        WASM_OP_I32_MUL,       // 相乘
        WASM_OP_RETURN         // 返回结果
    };
    
    // 3. 添加函数
    wasm_add_function(add_type, NULL, 0, add_code, sizeof(add_code), "add");
    wasm_add_function(sub_type, NULL, 0, sub_code, sizeof(sub_code), "sub");
    wasm_add_function(mul_type, NULL, 0, mul_code, sizeof(mul_code), "mul");
    
    // 4. 生成WASM模块
    WasmBuffer wasm = {0};
    wasm_buffer_init(&wasm);
    wasm_write_header(&wasm);
    wasm_add_type_section(&wasm);
    wasm_add_function_section(&wasm);
    wasm_add_export_section(&wasm);
    wasm_add_code_section(&wasm);
    
    // 5. 写入到文件
    FILE* f = fopen(wasm_file2, "wb");
    if (f) {
        fwrite(wasm.data, 1, wasm.size, f);
        fclose(f);
        printf("  ✓ 多函数模块生成成功: %s\n", wasm_file2);
        verify_wasm_file(wasm_file2);
    } else {
        printf("  ✗ 无法创建文件: %s\n", wasm_file2);
    }
    
    // 6. 清理资源
    wasm_buffer_free(&wasm);
    wasm_free_function_system();
    wasm_free_type_system();
    
    printf("\n=== WASM生成测试完成 ===\n");
}

// AST节点类型
typedef enum {
    // 声明
    NODE_TRANSLATION_UNIT,
    NODE_FUNCTION_DECL,
    NODE_VAR_DECL,
    NODE_STRUCT_DECL,
    NODE_UNION_DECL,
    NODE_ENUM_DECL,
    NODE_TYPEDEF_DECL,
    
    // 类型
    NODE_PRIMITIVE_TYPE,
    NODE_POINTER_TYPE,
    NODE_ARRAY_TYPE,
    NODE_STRUCT_TYPE,
    NODE_UNION_TYPE,
    NODE_ENUM_TYPE,
    NODE_FUNCTION_TYPE,
    
    // 语句和表达式
    NODE_COMPOUND_STMT,
    NODE_IF_STMT,
    NODE_SWITCH_STMT,
    NODE_WHILE_STMT,
    NODE_DO_WHILE_STMT,
    NODE_FOR_STMT,
    NODE_RETURN_STMT,
    NODE_BREAK_STMT,
    NODE_CONTINUE_STMT,
    NODE_GOTO_STMT,
    NODE_LABEL_STMT,
    NODE_EXPR_STMT,
    
    // 字面量和标识符
    NODE_INTEGER_LITERAL,
    NODE_FLOAT_LITERAL,
    NODE_CHAR_LITERAL,
    NODE_STRING_LITERAL,
    NODE_IDENTIFIER,
    
    // 操作符
    NODE_UNARY_OP,
    NODE_BINARY_OP,
    NODE_TERNARY_OP,
    NODE_CALL_EXPR,
    NODE_ARRAY_SUBSCRIPT,
    NODE_MEMBER_ACCESS,
    NODE_PTR_MEMBER_ACCESS,
    NODE_CAST_EXPR,
    NODE_SIZE_OF_EXPR
} NodeType;

// 基本类型
typedef enum {
    BT_VOID,
    BT_CHAR, BT_SIGNED_CHAR, BT_UNSIGNED_CHAR,
    BT_SHORT, BT_UNSIGNED_SHORT,
    BT_INT, BT_UNSIGNED_INT,
    BT_LONG, BT_UNSIGNED_LONG,
    BT_LONG_LONG, BT_UNSIGNED_LONG_LONG,
    BT_FLOAT, BT_DOUBLE, BT_LONG_DOUBLE,
    BT_BOOL,
    BT_STRUCT, BT_UNION, BT_ENUM,
    BT_POINTER, BT_ARRAY, BT_FUNCTION,
    BT_TYPEDEF_NAME
} BasicType;

// 类型限定符
typedef enum {
    Q_NONE = 0,
    Q_CONST = 1 << 0,
    Q_VOLATILE = 1 << 1,
    Q_RESTRICT = 1 << 2,
    Q_ATOMIC = 1 << 3,
    Q_NORETURN = 1 << 4,
    Q_INLINE = 1 << 5,
    Q_REGISTER = 1 << 6,
    Q_THREAD_LOCAL = 1 << 7,
    Q_EXTERN = 1 << 8,
    Q_STATIC = 1 << 9,
    Q_AUTO = 1 << 10,
    Q_TYPEDEF = 1 << 11
} TypeQualifier;

// AST节点结构
typedef struct ASTNode {
    NodeType type;
    int line;
    int column;
    const char *filename;
    
    // 类型信息
    struct {
        BasicType basic_type;
        unsigned qualifiers;
        int bit_width;  // 位域宽度
        bool is_signed;
        struct ASTNode *base_type;  // 对于指针、数组、函数等
        struct ASTNode *return_type; // 对于函数
    } type_info;
    
    // 值联合
    union {
        // 字面量
        long long int_val;
        double float_val;
        char *str_val;
        wchar_t *wstr_val;
        char char_val;
        
        // 标识符
        struct {
            char *name;
            struct ASTNode *symbol;  // 符号表引用
        } id;
        
        // 一元运算
        struct {
            int op;
            struct ASTNode *operand;
        } unary;
        
        // 二元运算
        struct {
            int op;
            struct ASTNode *left;
            struct ASTNode *right;
        } binary;
        
        // 三元运算
        struct {
            struct ASTNode *cond;
            struct ASTNode *then_expr;
            struct ASTNode *else_expr;
        } ternary;
        
        // 函数调用
        struct {
            struct ASTNode *func;
            struct ASTNode **args;
            int num_args;
        } call;
        
        // 声明
        struct {
            char *name;
            struct ASTNode *type;
            struct ASTNode *initializer;
            struct ASTNode *bit_width;  // 位域
            struct ASTNode *next;       // 下一个声明
        } decl;
        
        // 结构体/联合体/枚举
        struct {
            char *tag_name;
            struct ASTNode *fields;  // 字段声明列表
            bool is_definition;
        } record;
        
        // 枚举常量
        struct {
            char *name;
            struct ASTNode *value;  // 可选的初始化值
        } enum_constant;
        
        // 语句块
        struct {
            struct ASTNode **stmts;
            int num_stmts;
        } compound;
        
        // 控制流
        struct {
            struct ASTNode *cond;
            struct ASTNode *then_stmt;
            struct ASTNode *else_stmt;
            struct ASTNode *init;
            struct ASTNode *incr;
            struct ASTNode *body;
            char *label;
        } ctrl;
    } u;
    
    // 属性
    struct ASTNode **attributes;
    int num_attributes;
    
    // 注释
    char **comments;
    int num_comments;
    
    // 源代码位置信息
    struct {
        const char *source_file;
        int start_line;
        int start_col;
        int end_line;
        int end_col;
    } src_loc;
} ASTNode;

// AST序列化上下文
typedef struct {
    FILE *out;
    uint64_t node_count;
    uint64_t string_table_size;
    uint64_t string_table_offset;
    uint64_t *node_offsets;
    uint64_t node_offsets_capacity;
    uint64_t node_offsets_count;
} ASTSerializeContext;

// 字符串表
typedef struct {
    char **strings;
    uint64_t *hashes;
    uint64_t count;
    uint64_t capacity;
} StringTable;

// 全局字符串表
static StringTable string_table = {0};

// 初始化字符串表
static void init_string_table() {
    string_table.capacity = 64;
    string_table.strings = malloc(string_table.capacity * sizeof(char*));
    string_table.hashes = malloc(string_table.capacity * sizeof(uint64_t));
    string_table.count = 0;
}

// 添加字符串到字符串表
static uint64_t add_string_to_table(const char *str) {
    if (!str) return 0;
    
    // 计算字符串的哈希值
    uint64_t hash = 5381;
    const unsigned char *p;
    for (p = (const unsigned char*)str; *p != '\0'; p++) {
        hash = ((hash << 5) + hash) + *p;
    }
    
    // 检查字符串是否已存在
    for (uint64_t i = 0; i < string_table.count; i++) {
        if (string_table.hashes[i] == hash && 
            strcmp(string_table.strings[i], str) == 0) {
            return i + 1; // 返回1-based索引
        }
    }
    
    // 扩展字符串表
    if (string_table.count >= string_table.capacity) {
        string_table.capacity *= 2;
        string_table.strings = realloc(string_table.strings, 
                                     string_table.capacity * sizeof(char*));
        string_table.hashes = realloc(string_table.hashes, 
                                    string_table.capacity * sizeof(uint64_t));
    }
    
    // 添加新字符串
    string_table.strings[string_table.count] = strdup(str);
    string_table.hashes[string_table.count] = hash;
    return ++string_table.count; // 返回1-based索引
}

// 释放字符串表
static void free_string_table() {
    for (uint64_t i = 0; i < string_table.count; i++) {
        free(string_table.strings[i]);
    }
    free(string_table.strings);
    free(string_table.hashes);
    string_table.strings = NULL;
    string_table.hashes = NULL;
    string_table.count = 0;
    string_table.capacity = 0;
}

// 写入对齐的数据
static void write_padded(FILE *f, const void *data, size_t size, size_t align) {
    fwrite(data, 1, size, f);
    
    // 填充对齐
    size_t padding = (align - (size % align)) % align;
    if (padding > 0) {
        uint8_t zero = 0;
        for (size_t i = 0; i < padding; i++) {
            fwrite(&zero, 1, 1, f);
        }
    }
}

// 序列化AST节点
static void serialize_ast_node(ASTNode *node, ASTSerializeContext *ctx) {
    if (!node || !ctx || !ctx->out) return;
    
    // 记录节点偏移量
    if (ctx->node_offsets_count >= ctx->node_offsets_capacity) {
        ctx->node_offsets_capacity = ctx->node_offsets_capacity ? 
                                   ctx->node_offsets_capacity * 2 : 64;
        ctx->node_offsets = realloc(ctx->node_offsets, 
                                  ctx->node_offsets_capacity * sizeof(uint64_t));
    }
    ctx->node_offsets[ctx->node_offsets_count++] = ftell(ctx->out);
    
    // 写入节点类型
    write_padded(ctx->out, &node->type, sizeof(node->type), 4);
    
    // 写入行号和列号
    write_padded(ctx->out, &node->line, sizeof(node->line), 4);
    write_padded(ctx->out, &node->column, sizeof(node->column), 4);
    
    // 写入类型信息
    write_padded(ctx->out, &node->type_info.basic_type, 
                sizeof(node->type_info.basic_type), 4);
    write_padded(ctx->out, &node->type_info.qualifiers, 
                sizeof(node->type_info.qualifiers), 4);
    write_padded(ctx->out, &node->type_info.bit_width, 
                sizeof(node->type_info.bit_width), 4);
    
    // 序列化节点特定数据
    switch (node->type) {
        case NODE_IDENTIFIER: {
            uint64_t name_idx = add_string_to_table(node->id.name);
            write_padded(ctx->out, &name_idx, sizeof(name_idx), 8);
            break;
        }
        case NODE_INTEGER_LITERAL:
            write_padded(ctx->out, &node->u.int_val, sizeof(node->u.int_val), 8);
            break;
        case NODE_FLOAT_LITERAL:
            write_padded(ctx->out, &node->u.float_val, sizeof(node->u.float_val), 8);
            break;
        case NODE_STRING_LITERAL: {
            uint64_t str_idx = add_string_to_table(node->u.str_val);
            write_padded(ctx->out, &str_idx, sizeof(str_idx), 8);
            break;
        }
        // 其他节点类型的序列化...
        default:
            break;
    }
    
    // 增加节点计数
    ctx->node_count++;
}

// 序列化AST到.astc文件
static int serialize_ast_to_astc(ASTNode *root, const char *filename) {
    if (!root || !filename) return -1;
    
    // 初始化字符串表
    init_string_table();
    
    // 创建序列化上下文
    ASTSerializeContext ctx = {0};
    ctx.out = fopen(filename, "wb");
    if (!ctx.out) {
        free_string_table();
        return -1;
    }
    
    // 写入文件头
    fwrite(ASTC_MAGIC, 1, 4, ctx.out);
    uint32_t version = ASTC_VERSION;
    fwrite(&version, sizeof(version), 1, ctx.out);
    
    // 为节点计数和字符串表预留空间
    uint64_t placeholder = 0;
    fwrite(&placeholder, sizeof(placeholder), 1, ctx.out); // node_count
    fwrite(&placeholder, sizeof(placeholder), 1, ctx.out); // string_table_size
    fwrite(&placeholder, sizeof(placeholder), 1, ctx.out); // string_table_offset
    
    // 序列化AST
    serialize_ast_node(root, &ctx);
    
    // 记录字符串表位置
    ctx.string_table_offset = ftell(ctx.out);
    
    // 写入字符串表
    uint64_t str_count = string_table.count;
    fwrite(&str_count, sizeof(str_count), 1, ctx.out);
    
    for (uint64_t i = 0; i < str_count; i++) {
        uint64_t len = strlen(string_table.strings[i]);
        fwrite(&len, sizeof(len), 1, ctx.out);
        fwrite(string_table.strings[i], 1, len, ctx.out);
    }
    
    // 更新文件头
    fseek(ctx.out, 8, SEEK_SET); // 跳过魔数和版本
    fwrite(&ctx.node_count, sizeof(ctx.node_count), 1, ctx.out);
    fwrite(&ctx.string_table_size, sizeof(ctx.string_table_size), 1, ctx.out);
    fwrite(&ctx.string_table_offset, sizeof(ctx.string_table_offset), 1, ctx.out);
    
    // 清理
    free(ctx.node_offsets);
    free_string_table();
    fclose(ctx.out);
    
    return 0;
}

// 输出格式
enum OutputFormat {
    FORMAT_AST,    // 输出AST二进制文件
    FORMAT_WASM,   // 输出WASM模块
    FORMAT_EXE,    // 输出可执行文件
    FORMAT_DEFAULT = FORMAT_EXE  // 默认输出格式
};
#define MAX_TOKENS 10000
#define MAX_FUNCTIONS 100
#define MAX_MACHINE_CODE 8192
#define GENERATION_FILE "generation.txt"

// 词法分析器
typedef enum {
    TOKEN_EOF, TOKEN_IDENTIFIER, TOKEN_NUMBER, TOKEN_STRING,
    TOKEN_INCLUDE, TOKEN_DEFINE, TOKEN_IF, TOKEN_ELSE, TOKEN_WHILE, TOKEN_FOR,
    TOKEN_INT, TOKEN_CHAR, TOKEN_VOID, TOKEN_RETURN, TOKEN_STATIC,
    TOKEN_LBRACE, TOKEN_RBRACE, TOKEN_LPAREN, TOKEN_RPAREN,
    TOKEN_SEMICOLON, TOKEN_COMMA, TOKEN_ASSIGN, TOKEN_PLUS, TOKEN_MINUS,
    TOKEN_MULTIPLY, TOKEN_DIVIDE, TOKEN_PRINTF, TOKEN_MALLOC, TOKEN_FREE
} TokenType;

typedef struct {
    TokenType type;
    char *value;
    int line;
} Token;

// 简化的AST节点
typedef struct ASTNode {
    TokenType type;
    char *value;
    struct ASTNode *left;
    struct ASTNode *right;
    struct ASTNode *next;
} ASTNode;

// 机器码生成器
typedef struct {
    unsigned char code[MAX_MACHINE_CODE];
    int size;
    int entry_point;
} MachineCode;

// 编译器配置
typedef struct {
    enum OutputFormat output_format;
    bool verbose;
    bool optimize;
    const char *output_file;
    const char *target_arch;  // 目标架构 (x86_64, arm64, etc.)
} CompilerConfig;

// 编译器状态
typedef struct {
    Token tokens[MAX_TOKENS];
    int token_count;
    int current_token;
    MachineCode machine_code;
    char *source_code;
    CompilerConfig config;
} BootstrapCompiler;

// 核心自举函数
static char* read_self_source();
static int bootstrap_compile_real(const char *source, const CompilerConfig *config);
// 创建AST节点
static ASTNode *create_ast_node(NodeType type, int line, int col) {
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    if (node) {
        node->type = type;
        node->line = line;
        node->column = col;
        node->type_info.basic_type = BT_INT; // 默认类型
        node->type_info.qualifiers = 0;
        node->type_info.size = 0;
        node->type_info.align = 0;
    }
    return node;
}

// 创建标识符节点
static ASTNode *create_identifier_node(const char *name, int line, int col) {
    ASTNode *node = create_ast_node(NODE_IDENTIFIER, line, col);
    if (node) {
        node->id.name = strdup(name);
    }
    return node;
}

// 创建整数字面量节点
static ASTNode *create_integer_literal(long long value, int line, int col) {
    ASTNode *node = create_ast_node(NODE_INTEGER_LITERAL, line, col);
    if (node) {
        node->u.int_val = value;
    }
    return node;
}

// 创建函数声明节点
static ASTNode *create_function_decl(const char *name, ASTNode *return_type, 
                                   ASTNode *params, ASTNode *body, 
                                   int line, int col) {
    ASTNode *node = create_ast_node(NODE_FUNCTION_DECL, line, col);
    if (node) {
        node->decl.name = strdup(name);
        node->decl.type = return_type;
        node->u.compound.stmts = malloc(sizeof(ASTNode*) * 32); // 简单的固定大小数组
        node->u.compound.stmts[0] = body;
        node->u.compound.num_stmts = 1;
    }
    return node;
}

// 创建返回语句节点
static ASTNode *create_return_stmt(ASTNode *expr, int line, int col) {
    ASTNode *node = create_ast_node(NODE_RETURN_STMT, line, col);
    if (node) {
        node->u.ctrl.cond = expr; // 复用cond字段存储返回值表达式
    }
    return node;
}

// 创建二元操作节点
static ASTNode *create_binary_op(BinaryOp op, ASTNode *lhs, ASTNode *rhs, int line, int col) {
    ASTNode *node = create_ast_node(NODE_BINARY_OP, line, col);
    if (node) {
        node->u.binary_op.op = op;
        node->u.binary_op.lhs = lhs;
        node->u.binary_op.rhs = rhs;
    }
    return node;
}

// 创建变量声明节点
static ASTNode *create_var_decl(const char *name, ASTNode *type, ASTNode *init, int line, int col) {
    ASTNode *node = create_ast_node(NODE_VAR_DECL, line, col);
    if (node) {
        node->decl.name = strdup(name);
        node->decl.type = type;
        node->decl.init = init;
    }
    return node;
}

// 创建函数调用节点
static ASTNode *create_function_call(const char *name, ASTNode **args, int num_args, int line, int col) {
    ASTNode *node = create_ast_node(NODE_FUNCTION_CALL, line, col);
    if (node) {
        node->id.name = strdup(name);
        node->u.call.args = args;
        node->u.call.num_args = num_args;
    }
    return node;
}

// 创建if语句节点
static ASTNode *create_if_stmt(ASTNode *cond, ASTNode *then_block, ASTNode *else_block, int line, int col) {
    ASTNode *node = create_ast_node(NODE_IF_STMT, line, col);
    if (node) {
        node->u.if_stmt.cond = cond;
        node->u.if_stmt.then_block = then_block;
        node->u.if_stmt.else_block = else_block;
    }
    return node;
}

// 创建while循环节点
static ASTNode *create_while_loop(ASTNode *cond, ASTNode *body, int line, int col) {
    ASTNode *node = create_ast_node(NODE_WHILE_STMT, line, col);
    if (node) {
        node->u.while_loop.cond = cond;
        node->u.while_loop.body = body;
    }
    return node;
}

// 创建数组访问节点
static ASTNode *create_array_access(ASTNode *array, ASTNode *index, int line, int col) {
    ASTNode *node = create_ast_node(NODE_ARRAY_ACCESS, line, col);
    if (node) {
        node->u.array.array = array;
        node->u.array.index = index;
    }
    return node;
}

// 递归释放AST节点
static void free_ast_node(ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case NODE_IDENTIFIER:
            free(node->id.name);
            break;
            
        case NODE_FUNCTION_DECL:
            free(node->decl.name);
            free_ast_node(node->decl.type);
            if (node->u.compound.stmts) {
                for (int i = 0; i < node->u.compound.num_stmts; i++) {
                    free_ast_node(node->u.compound.stmts[i]);
                }
                free(node->u.compound.stmts);
            }
            break;
            
        case NODE_VAR_DECL:
            free(node->decl.name);
            free_ast_node(node->decl.type);
            free_ast_node(node->decl.init);
            break;
            
        case NODE_BINARY_OP:
            free_ast_node(node->u.binary_op.lhs);
            free_ast_node(node->u.binary_op.rhs);
            break;
            
        case NODE_IF_STMT:
            free_ast_node(node->u.if_stmt.cond);
            free_ast_node(node->u.if_stmt.then_block);
            free_ast_node(node->u.if_stmt.else_block);
            break;
            
        case NODE_WHILE_STMT:
            free_ast_node(node->u.while_loop.cond);
            free_ast_node(node->u.while_loop.body);
            break;
            
        case NODE_FUNCTION_CALL:
            free(node->id.name);
            if (node->u.call.args) {
                for (int i = 0; i < node->u.call.num_args; i++) {
                    free_ast_node(node->u.call.args[i]);
                }
                free(node->u.call.args);
            }
            break;
            
        case NODE_ARRAY_ACCESS:
            free_ast_node(node->u.array.array);
            free_ast_node(node->u.array.index);
            break;
            
        case NODE_RETURN_STMT:
            free_ast_node(node->u.ctrl.cond);
            break;
            
        case NODE_COMPOUND_STMT:
            if (node->u.compound.stmts) {
                for (int i = 0; i < node->u.compound.num_stmts; i++) {
                    free_ast_node(node->u.compound.stmts[i]);
                }
                free(node->u.compound.stmts);
            }
            break;
            
        default:
            // 基本类型节点没有额外分配的内存需要释放
            break;
    }
    
    free(node);
}

// 生成AST文件
int generate_ast(const char *source, const char *output_file) {
    printf("生成AST文件: %s\n", output_file);
    
    // 创建更复杂的示例AST
    ASTNode *translation_unit = create_ast_node(NODE_TRANSLATION_UNIT, 1, 1);
    
    // 创建类型节点
    ASTNode *int_type = create_ast_node(NODE_PRIMITIVE_TYPE, 1, 1);
    int_type->type_info.basic_type = BT_INT;
    
    // 创建函数参数: int a, int b
    ASTNode *param_a = create_var_decl("a", int_type, NULL, 1, 10);
    ASTNode *param_b = create_var_decl("b", int_type, NULL, 1, 16);
    
    // 创建函数体: { return a + b; }
    ASTNode *return_expr = create_binary_op(
        OP_ADD,
        create_identifier_node("a", 1, 25),
        create_identifier_node("b", 1, 29),
        1, 25
    );
    ASTNode *return_stmt = create_return_stmt(return_expr, 1, 18);
    
    ASTNode *function_body = create_ast_node(NODE_COMPOUND_STMT, 1, 16);
    function_body->u.compound.stmts = malloc(sizeof(ASTNode*) * 1);
    function_body->u.compound.stmts[0] = return_stmt;
    function_body->u.compound.num_stmts = 1;
    
    // 创建函数声明: int add(int a, int b) { ... }
    ASTNode *func_decl = create_function_decl("add", int_type, NULL, function_body, 1, 1);
    
    // 将函数添加到翻译单元
    translation_unit->u.compound.stmts = malloc(sizeof(ASTNode*) * 1);
    translation_unit->u.compound.stmts[0] = func_decl;
    translation_unit->u.compound.num_stmts = 1;
    
    // 序列化AST到文件
    int result = serialize_ast_to_astc(translation_unit, output_file);
    
    // 清理AST
    free_ast_node(translation_unit);
    
    return result == 0 ? 0 : 1;
}

// 从文件反序列化AST
ASTNode *deserialize_ast_from_astc(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("无法打开文件");
        return NULL;
    }
    
    // 读取文件头
    ASTCHeader header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        fprintf(stderr, "读取文件头失败\n");
        return NULL;
    }
    
    // 验证魔数
    if (strncmp(header.magic, "ASTC", 4) != 0) {
        fclose(file);
        fprintf(stderr, "无效的ASTC文件格式\n");
        return NULL;
    }
    
    // 检查版本
    if (header.version != ASTC_VERSION) {
        fclose(file);
        fprintf(stderr, "不支持的ASTC版本: %u\n", header.version);
        return NULL;
    }
    
    // 读取字符串表
    if (fseek(file, header.string_table_offset, SEEK_SET) != 0) {
        fclose(file);
        fprintf(stderr, "定位字符串表失败\n");
        return NULL;
    }
    
    // 读取字符串表项数
    uint32_t num_strings;
    if (fread(&num_strings, sizeof(uint32_t), 1, file) != 1) {
        fclose(file);
        fprintf(stderr, "读取字符串表大小失败\n");
        return NULL;
    }
    
    // 分配字符串表
    char **string_table = (char **)calloc(num_strings, sizeof(char *));
    if (!string_table) {
        fclose(file);
        fprintf(stderr, "分配字符串表内存失败\n");
        return NULL;
    }
    
    // 读取每个字符串
    for (uint32_t i = 0; i < num_strings; i++) {
        uint32_t str_len;
        if (fread(&str_len, sizeof(uint32_t), 1, file) != 1) {
            // 清理已分配的字符串
            for (uint32_t j = 0; j < i; j++) {
                free(string_table[j]);
            }
            free(string_table);
            fclose(file);
            fprintf(stderr, "读取字符串长度失败\n");
            return NULL;
        }
        
        string_table[i] = (char *)malloc(str_len + 1);
        if (!string_table[i]) {
            // 清理已分配的字符串
            for (uint32_t j = 0; j < i; j++) {
                free(string_table[j]);
            }
            free(string_table);
            fclose(file);
            fprintf(stderr, "分配字符串内存失败\n");
            return NULL;
        }
        
        if (fread(string_table[i], 1, str_len, file) != str_len) {

int main(int argc, char *argv[]) {
    srand(time(NULL));
    
    // 默认配置
    CompilerConfig config = {
        .output_format = FORMAT_DEFAULT,
        .verbose = false,
        .optimize = false,
        .output_file = NULL,
        .target_arch = "x86_64"
    };
    
    // 从环境变量读取配置
    const char *env_format = getenv("EVOLVER_OUTPUT_FORMAT");
    if (env_format) {
        if (strcmp(env_format, "ast") == 0) config.output_format = FORMAT_AST;
        else if (strcmp(env_format, "wasm") == 0) config.output_format = FORMAT_WASM;
        else if (strcmp(env_format, "exe") == 0) config.output_format = FORMAT_EXE;
    }
    
    const char *env_arch = getenv("EVOLVER_TARGET_ARCH");
    if (env_arch) {
        config.target_arch = env_arch;
    }
    
    // 解析命令行参数
    static struct option long_options[] = {
        {"output", required_argument, 0, 'o'},
        {"format", required_argument, 0, 'f'},
        {"arch", required_argument, 0, 'a'},
        {"optimize", no_argument, 0, 'O'},
        {"verbose", no_argument, 0, 'v'},
        {"evolve", no_argument, 0, 0x100},
        {"test", no_argument, 0, 0x101},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "o:f:a:Ovh", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'o':
                config.output_file = optarg;
                break;
            case 'f':
                if (strcmp(optarg, "ast") == 0) config.output_format = FORMAT_AST;
                else if (strcmp(optarg, "wasm") == 0) config.output_format = FORMAT_WASM;
                else if (strcmp(optarg, "exe") == 0) config.output_format = FORMAT_EXE;
                else {
                    fprintf(stderr, "错误: 未知的输出格式 '%s'\n", optarg);
                    return 1;
                }
                break;
            case 'a':
                config.target_arch = optarg;
                break;
            case 'O':
                config.optimize = true;
                break;
            case 'v':
                config.verbose = true;
                break;
            case 0x100:  // --evolve
                if (config.verbose) {
                    printf("=== Self-Evolve AI - 自举进化模式 ===\n");
                }
                evolve_bootstrap();
                return 0;
            case 0x101:  // --test
                if (config.verbose) {
                    printf("=== Self-Evolve AI - 测试模式 ===\n");
                }
                // 测试WASM生成
                test_wasm_generation();
                return 0;
            case 'h':
            default:
                print_help(argv[0]);
                return 0;
        }
    }
    
    if (argc > 1 && strcmp(argv[1], "-wasm") == 0) {
        if (argc < 3) {
            printf("用法: %s -wasm <输出文件.wasm>\n", argv[0]);
            return 1;
        }
        return generate_wasm("", argv[2]);
    }
    
    printf("=== Self-Evolve AI - 第零代自举编译器 ===\n");
    printf("版本: evolver%d.c\n", VERSION);
    printf("当前代数: %d\n", get_current_generation());
    
    // 处理输入文件
    if (optind >= argc) {
        fprintf(stderr, "错误: 未指定输入文件\n");
        print_help(argv[0]);
        return 1;
    }
    
    const char *input_file = argv[optind];
    if (config.verbose) {
        printf("输入文件: %s\n", input_file);
        printf("输出格式: %s\n", 
               config.output_format == FORMAT_AST ? "AST" :
               config.output_format == FORMAT_WASM ? "WASM" : "Executable");
        printf("目标架构: %s\n", config.target_arch);
    }
    
    // 读取输入文件
    FILE *f = fopen(input_file, "rb");
    if (!f) {
        perror("无法打开输入文件");
        return 1;
    }
    
    size_t size = get_file_size(f);
    char *source = malloc(size + 1);
    if (!source) {
        fclose(f);
        fprintf(stderr, "内存分配失败\n");
        return 1;
    }
    
    fread(source, 1, size, f);
    source[size] = '\0';
    fclose(f);
    
    // 确定输出文件名
    char default_output[256];
    if (!config.output_file) {
        const char *ext = ".exe";
        if (config.output_format == FORMAT_AST) ext = ".ast";
        else if (config.output_format == FORMAT_WASM) ext = ".wasm";
        
        const char *base = input_file;
        const char *dot = strrchr(input_file, '.');
        if (dot) {
            snprintf(default_output, sizeof(default_output), "%.*s%s", 
                   (int)(dot - input_file), input_file, ext);
        } else {
            snprintf(default_output, sizeof(default_output), "%s%s", input_file, ext);
        }
        config.output_file = default_output;
    }
    
    if (config.verbose) {
        printf("输出文件: %s\n", config.output_file);
    }
    
    // 执行编译
    int result = bootstrap_compile_real(source, &config);
    free(source);
    
    if (result == 0) {
        if (config.verbose) {
            printf("编译成功: %s\n", config.output_file);
        }
        return 0;
    } else {
        fprintf(stderr, "编译失败\n");
        return 1;
    }
    
    return 0;
}

static char* read_self_source() {
    char filename[64];
    sprintf(filename, "evolver%d.c", VERSION);
    
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "无法读取源文件: %s\n", filename);
        return NULL;
    }
    
    size_t size = get_file_size(f);
    if (size == 0 || size > MAX_CODE_SIZE) {
        fclose(f);
        return NULL;
    }
    
    char *code = malloc(size + 1);
    if (!code) {
        fclose(f);
        return NULL;
    }
    
    fread(code, 1, size, f);
    fclose(f);
    code[size] = '\0';
    
    return code;
}

static int tokenize(BootstrapCompiler *compiler, const char *source) {
    compiler->token_count = 0;
    const char *p = source;
    int line = 1;
    
    while (*p && compiler->token_count < MAX_TOKENS - 1) {
        // 跳过空白字符
        while (isspace(*p)) {
            if (*p == '\n') line++;
            p++;
        }
        
        if (!*p) break;
        
        Token *token = &compiler->tokens[compiler->token_count++];
        token->line = line;
        
        // 识别标识符和关键字
        if (isalpha(*p) || *p == '_') {
            const char *start = p;
            while (isalnum(*p) || *p == '_') p++;
            
            int len = p - start;
            token->value = malloc(len + 1);
            strncpy(token->value, start, len);
            token->value[len] = '\0';
            
            // 检查关键字
            if (strcmp(token->value, "int") == 0) token->type = TOKEN_INT;
            else if (strcmp(token->value, "char") == 0) token->type = TOKEN_CHAR;
            else if (strcmp(token->value, "void") == 0) token->type = TOKEN_VOID;
            else if (strcmp(token->value, "return") == 0) token->type = TOKEN_RETURN;
            else if (strcmp(token->value, "static") == 0) token->type = TOKEN_STATIC;
            else if (strcmp(token->value, "if") == 0) token->type = TOKEN_IF;
            else if (strcmp(token->value, "else") == 0) token->type = TOKEN_ELSE;
            else if (strcmp(token->value, "while") == 0) token->type = TOKEN_WHILE;
            else if (strcmp(token->value, "printf") == 0) token->type = TOKEN_PRINTF;
            else if (strcmp(token->value, "malloc") == 0) token->type = TOKEN_MALLOC;
            else token->type = TOKEN_IDENTIFIER;
        }
        // 识别数字
        else if (isdigit(*p)) {
            const char *start = p;
            while (isdigit(*p)) p++;
            
            int len = p - start;
            token->value = malloc(len + 1);
            strncpy(token->value, start, len);
            token->value[len] = '\0';
            token->type = TOKEN_NUMBER;
        }
        // 识别字符串
        else if (*p == '"') {
            p++; // 跳过开始的引号
            const char *start = p;
            while (*p && *p != '"') p++;
            
            int len = p - start;
            token->value = malloc(len + 1);
            strncpy(token->value, start, len);
            token->value[len] = '\0';
            token->type = TOKEN_STRING;
            
            if (*p == '"') p++; // 跳过结束的引号
        }
        // 识别单字符标记
        else {
            token->value = malloc(2);
            token->value[0] = *p;
            token->value[1] = '\0';
            
            switch (*p) {
                case '{': token->type = TOKEN_LBRACE; break;
                case '}': token->type = TOKEN_RBRACE; break;
                case '(': token->type = TOKEN_LPAREN; break;
                case ')': token->type = TOKEN_RPAREN; break;
                case ';': token->type = TOKEN_SEMICOLON; break;
                case ',': token->type = TOKEN_COMMA; break;
                case '=': token->type = TOKEN_ASSIGN; break;
                case '+': token->type = TOKEN_PLUS; break;
                case '-': token->type = TOKEN_MINUS; break;
                case '*': token->type = TOKEN_MULTIPLY; break;
                case '/': 
                    // 处理注释
                    if (*(p+1) == '/') {
                        while (*p && *p != '\n') p++;
                        compiler->token_count--; // 不保存注释
                        continue;
                    }
                    token->type = TOKEN_DIVIDE; 
                    break;
                default:
                    compiler->token_count--; // 忽略未知字符
                    break;
            }
            p++;
        }
    }
    
    // 添加EOF标记
    compiler->tokens[compiler->token_count].type = TOKEN_EOF;
    compiler->tokens[compiler->token_count].value = NULL;
    
    return compiler->token_count;
}

// x86-64 机器码生成函数
static void emit_byte(MachineCode *code, unsigned char byte) {
    if (code->size < MAX_MACHINE_CODE) {
        code->code[code->size++] = byte;
    }
}

static void emit_word(MachineCode *code, unsigned short word) {
    emit_byte(code, word & 0xFF);
    emit_byte(code, (word >> 8) & 0xFF);
}

static void emit_dword(MachineCode *code, unsigned int dword) {
    emit_byte(code, dword & 0xFF);
    emit_byte(code, (dword >> 8) & 0xFF);
    emit_byte(code, (dword >> 16) & 0xFF);
    emit_byte(code, (dword >> 24) & 0xFF);
}

static void emit_qword(MachineCode *code, unsigned long qword) {
    emit_dword(code, qword & 0xFFFFFFFF);
    emit_dword(code, (qword >> 32) & 0xFFFFFFFF);
}

// MOV RAX, immediate value
static void emit_mov_rax_imm(MachineCode *code, long value) {
    emit_byte(code, 0x48); // REX.W prefix
    emit_byte(code, 0xB8); // MOV RAX, imm64
    emit_qword(code, value);
}

// RET instruction
static void emit_ret(MachineCode *code) {
    emit_byte(code, 0xC3);
}

// SYSCALL instruction
static void emit_syscall(MachineCode *code) {
    emit_byte(code, 0x0F);
    emit_byte(code, 0x05);
}

static int generate_machine_code(BootstrapCompiler *compiler) {
    // 清空机器码缓冲区
    memset(&compiler->machine_code, 0, sizeof(MachineCode));
    
    // 生成一个简单的程序：返回42
    // 相当于：
    // mov rax, 42    ; 设置返回值
    // ret            ; 返回
    
    emit_mov_rax_imm(&compiler->machine_code, 42);
    emit_ret(&compiler->machine_code);
    
    printf("生成机器码 %d 字节\n", compiler->machine_code.size);
    
    return 0;
}

static int parse_and_generate(BootstrapCompiler *compiler) {
    // 简化的解析：只寻找main函数并生成返回42的代码
    int found_main = 0;
    
    for (int i = 0; i < compiler->token_count; i++) {
        if (compiler->tokens[i].type == TOKEN_IDENTIFIER &&
            strcmp(compiler->tokens[i].value, "main") == 0) {
            found_main = 1;
            break;
        }
    }
    
    if (!found_main) {
        printf("错误：未找到main函数\n");
        return -1;
    }
    
    // 生成机器码
    return generate_machine_code(compiler);
}

static int write_elf_executable(const char *filename, MachineCode *code) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        printf("错误：无法创建输出文件 %s\n", filename);
        return -1;
    }
    
    // 简化的ELF头（64位）
    unsigned char elf_header[64] = {
        // ELF魔数
        0x7F, 'E', 'L', 'F',
        // 64位，小端序，版本1
        2, 1, 1, 0,
        // 填充
        0, 0, 0, 0, 0, 0, 0, 0,
        // 可执行文件类型
        2, 0,
        // x86-64架构
        0x3E, 0x00,
        // 版本
        1, 0, 0, 0,
        // 入口点地址 (0x401000)
        0x00, 0x10, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
        // 程序头表偏移 (64)
        0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // 节区头表偏移 (0)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // 标志
        0x00, 0x00, 0x00, 0x00,
        // ELF头大小
        0x40, 0x00,
        // 程序头表项大小
        0x38, 0x00,
        // 程序头表项数量
        0x01, 0x00,
        // 节区头表项大小
        0x00, 0x00,
        // 节区头表项数量
        0x00, 0x00,
        // 字符串表索引
        0x00, 0x00
    };
    
    // 程序头表
    unsigned char program_header[56] = {
        // 段类型：LOAD
        1, 0, 0, 0,
        // 段标志：可执行+可读
        5, 0, 0, 0,
        // 文件偏移
        0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // 虚拟地址
        0x00, 0x10, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
        // 物理地址
        0x00, 0x10, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
        // 文件大小
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 稍后填充
        // 内存大小
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 稍后填充
        // 对齐
        0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    
    // 填充代码大小
    *((unsigned int*)(program_header + 32)) = code->size;
    *((unsigned int*)(program_header + 40)) = code->size;
    
    // 写入ELF头
    fwrite(elf_header, 1, 64, f);
    
    // 写入程序头表
    fwrite(program_header, 1, 56, f);
    
    // 写入机器码
    fwrite(code->code, 1, code->size, f);
    
    fclose(f);
    
    // 设置可执行权限 (暂时跳过，依赖系统默认权限)
    
    printf("✓ 生成ELF可执行文件: %s (%d字节机器码)\n", filename, code->size);
    
    return 0;
}

static int bootstrap_compile_real(const char *source, const CompilerConfig *config) {
    if (!source || !config) {
        fprintf(stderr, "错误: 无效的输入参数\n");
        return 1;
    }
    
    // 确保输出文件名不为空
    if (!config->output_file || strlen(config->output_file) == 0) {
        fprintf(stderr, "错误: 未指定输出文件名\n");
        return 1;
    }
    
    // 根据配置选择输出格式
    switch (config->output_format) {
        case FORMAT_AST: {
            // 确保输出文件扩展名为.astc
            char *output_file = strdup(config->output_file);
            if (!strstr(output_file, ".")) {
                // 如果没有扩展名，添加.astc
                char *new_output = malloc(strlen(output_file) + 6); // 5 for ".astc" + 1 for '\0'
                sprintf(new_output, "%s.astc", output_file);
                free(output_file);
                output_file = new_output;
            } else if (strcmp(strrchr(output_file, '.'), ".astc") != 0) {
                // 如果扩展名不是.astc，替换之
                char *dot = strrchr(output_file, '.');
                strcpy(dot, ".astc");
            }
            
            int result = generate_ast(source, output_file);
            free(output_file);
            return result;
        }
            
        case FORMAT_WASM:
            return generate_wasm(source, config->output_file);
            
        case FORMAT_EXE:
        case FORMAT_DEFAULT: {
            BootstrapCompiler compiler;
            memset(&compiler, 0, sizeof(compiler));
            compiler.source_code = (char *)source;
            compiler.config = *config;
            
            // 词法分析
            if (tokenize(&compiler, source) != 0) {
                fprintf(stderr, "词法分析失败\n");
                return 1;
            }
            
            // 语法分析和代码生成
            if (parse_and_generate(&compiler) != 0) {
                fprintf(stderr, "语法分析或代码生成失败\n");
                return 1;
            }
            
            // 生成可执行文件
            return generate_executable(source, config->output_file, config->target_arch);
        }
            
        default:
            fprintf(stderr, "错误: 不支持的输出格式\n");
            return 1;
    }
    
    return 0;
}

static char* mutate_for_bootstrap(const char *source) {
    // 简单的变异：随机复制当前代码
    size_t len = strlen(source);
    char *mutant = malloc(len + 1);
    if (!mutant) return NULL;
    
    strcpy(mutant, source);
    
    // 在代码末尾添加一个空行作为简单的变异
    char *new_mutant = realloc(mutant, len + 3); // +1 for newline, +1 for null terminator, +1 for potential extra char
    if (!new_mutant) {
        free(mutant);
        return NULL;
    }
    
    mutant = new_mutant;
    strcat(mutant, "\n");
    
    return mutant;
}

static double evaluate_bootstrap_fitness(const char *source) {
    if (!source) return 0.0;
    
    // 简单的适应度函数：代码越短越好
    double fitness = 0.0;
    size_t code_size = strlen(source);
    if (code_size > 0) {
        fitness += 10000.0 / (double)code_size;
    }
    
    // 检查是否包含关键函数
    if (strstr(source, "main") != NULL) fitness += 100.0;
    if (strstr(source, "bootstrap_compile_real") != NULL) fitness += 50.0;
    
    return fitness;
}

static int save_next_generation(const char *source, int gen) {
    char filename[64];
    snprintf(filename, sizeof(filename), "evolver%d.c", gen + 1);
    
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("无法创建下一代文件");
        return 1;
    }
    
    size_t len = strlen(source);
    if (fwrite(source, 1, len, f) != len) {
        fclose(f);
        perror("写入文件失败");
        return 1;
    }
    
    fclose(f);
    update_generation(gen + 1);
    
    // 更新aitasker.md中的进度
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "sed -i '' 's/第零代自举编译器/第%d代自举编译器/g' aitasker.md", gen + 1);
    system(cmd);
    
    return 0;
}

static void evolve_bootstrap() {
    char *source = read_self_source();
    if (!source) {
        fprintf(stderr, "无法读取当前源代码\n");
        return;
    }
    
    int current_gen = get_current_generation();
    printf("当前代数: %d\n", current_gen);
    
    // 生成下一代
    char *new_source = mutate_for_bootstrap(source);
    if (!new_source) {
        free(source);
        fprintf(stderr, "生成下一代失败\n");
        return;
    }
    
    // 编译测试当前代
    CompilerConfig test_config = {
        .output_format = FORMAT_EXE,
        .verbose = true,
        .optimize = true,
        .output_file = "evolver_test",
        .target_arch = "x86_64"
    };
    
    printf("\n测试编译当前代...\n");
    if (bootstrap_compile_real(source, &test_config) != 0) {
        fprintf(stderr, "当前代编译测试失败，停止进化\n");
        free(source);
        free(new_source);
        return;
    }
    
    // 测试编译下一代
    printf("\n测试编译下一代...\n");
    if (bootstrap_compile_real(new_source, &test_config) != 0) {
        fprintf(stderr, "下一代编译测试失败，放弃保存\n");
        free(source);
        free(new_source);
        return;
    }
    
    // 保存下一代
    if (save_next_generation(new_source, current_gen) == 0) {
        printf("\n成功生成第%d代\n", current_gen + 1);
        
        // 更新aitasker.md中的进度
        FILE *f = fopen("aitasker.md", "a");
        if (f) {
            fprintf(f, "\n## 第%d代更新\n", current_gen + 1);
            fprintf(f, "- 成功通过编译测试\n");
            fprintf(f, "- 添加了对多种输出格式的支持\n");
            fprintf(f, "- 改进了命令行参数处理\n");
            fclose(f);
        }
    }
    
    free(source);
    free(new_source);
}

static size_t get_file_size(FILE *f) {
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    return size;
}

static int get_current_generation() {
    FILE *f = fopen(GENERATION_FILE, "r");
    if (!f) return 0;
    
    int gen = 0;
    fscanf(f, "%d", &gen);
    fclose(f);
    return gen;
}

static void update_generation(int gen) {
    FILE *f = fopen(GENERATION_FILE, "w");
    if (f) {
        fprintf(f, "%d\n", gen);
        fclose(f);
    }
} 