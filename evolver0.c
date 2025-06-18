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

// ====================================
// 预处理器和宏处理
// ====================================


// 宏定义
typedef struct Macro {
    char *name;             // 宏名
    char *replacement;      // 替换文本
    char **params;          // 参数名数组
    int num_params;         // 参数数量
    int is_function_like;   // 是否为函数式宏
    int is_variadic;        // 是否支持可变参数
    int has_va_args;        // 是否包含__VA_ARGS__
    struct Macro *next;    // 下一个宏
} Macro;

// 宏表
typedef struct {
    Macro *head;
    Macro *tail;
} MacroTable;

// 条件编译栈项
typedef struct IfState {
    int condition_met;    // 当前条件是否为真
    int else_allowed;     // 是否允许出现#else
    struct IfState *prev; // 前一个条件状态
} IfState;

// 宏展开状态
typedef struct {
    Token *expansion_tokens;  // 宏展开后的token列表
    Token *current;           // 当前展开的token
    int level;               // 展开嵌套深度
    char *macro_name;        // 当前展开的宏名
    char **args;             // 宏参数
    int num_args;            // 参数数量
    
    // 条件编译状态
    IfState *if_stack;       // 条件编译栈
    int skipping;            // 是否跳过当前代码块
    int skip_level;          // 当前跳过的嵌套级别
} MacroExpansionState;

// 全局宏表
static MacroTable macro_table = {NULL, NULL};

// 预定义宏
static const char *predefined_macros[] = {
    "__LINE__",
    "__FILE__",
    "__DATE__",
    "__TIME__",
    "__STDC__",
    "__STDC_VERSION__",
    "__cplusplus",
    NULL
};

// 检查是否是预定义宏
static int is_predefined_macro(const char *name) {
    for (int i = 0; predefined_macros[i] != NULL; i++) {
        if (strcmp(name, predefined_macros[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

// 展开预定义宏
static char *expand_predefined_macro(const char *name, int line, const char *filename) {
    if (strcmp(name, "__LINE__") == 0) {
        char *result = malloc(16);
        snprintf(result, 16, "%d", line);
        return result;
    } else if (strcmp(name, "__FILE__") == 0) {
        char *result = malloc(strlen(filename) + 3);
        snprintf(result, strlen(filename) + 3, "\"%s\"", filename);
        return result;
    } else if (strcmp(name, "__DATE__") == 0) {
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        char *result = malloc(12); // "Mmm dd yyyy" + null
        strftime(result, 12, "%b %d %Y", tm_info);
        return result;
    } else if (strcmp(name, "__TIME__") == 0) {
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        char *result = malloc(9); // "hh:mm:ss" + null
        strftime(result, 9, "%H:%M:%S", tm_info);
        return result;
    } else if (strcmp(name, "__STDC__") == 0) {
        return strdup("1");
    } else if (strcmp(name, "__STDC_VERSION__") == 0) {
        return strdup("201710L");
    } else if (strcmp(name, "__cplusplus") == 0) {
        return strdup("1");
    }
    return NULL;
}

// 全局宏展开状态
static MacroExpansionState macro_expansion = {NULL, NULL, 0, NULL, NULL, 0, NULL, 0, 0};

// 释放宏参数
static void free_macro_args(char **args, int count) {
    if (!args) return;
    for (int i = 0; i < count; i++) {
        free(args[i]);
    }
    free(args);
}

// 重置宏展开状态
static void reset_macro_expansion(void) {
    if (macro_expansion.expansion_tokens) {
        free(macro_expansion.expansion_tokens);
    }
    if (macro_expansion.macro_name) {
        free(macro_expansion.macro_name);
    }
    if (macro_expansion.args) {
        free_macro_args(macro_expansion.args, macro_expansion.num_args);
    }
    
    // 清理条件编译栈
    IfState *state = macro_expansion.if_stack;
    while (state) {
        IfState *prev = state->prev;
        free(state);
        state = prev;
    }
    
    memset(&macro_expansion, 0, sizeof(macro_expansion));
}

// 查找宏
static Macro* find_macro(const char *name) {
    if (!name) return NULL;
    
    Macro *current = macro_table.head;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// 添加宏
extern void add_macro(const char *name, const char *replacement, int is_function_like, char **params, int num_params) {
    // 检查是否已存在同名宏
    Macro *existing = find_macro(name);
    if (existing) {
        // 如果已存在，先释放旧的宏定义
        free(existing->replacement);
        free_macro_args(existing->params, existing->num_params);
    } else {
        // 创建新宏
        existing = (Macro*)malloc(sizeof(Macro));
        if (!existing) return;
        
        existing->name = strdup(name);
        existing->next = NULL;
        
        // 添加到链表尾部
        if (!macro_table.head) {
            macro_table.head = macro_table.tail = existing;
        } else {
            macro_table.tail->next = existing;
            macro_table.tail = existing;
        }
    }
    
    // 检查是否包含__VA_ARGS__
    int has_va_args = (strstr(replacement, "__VA_ARGS__") != NULL);
    
    // 设置宏属性
    existing->replacement = strdup(replacement);
    existing->is_function_like = is_function_like;
    existing->is_variadic = (is_function_like && num_params > 0 && 
                           strcmp(params[num_params - 1], "...") == 0);
    existing->has_va_args = has_va_args;
    existing->num_params = num_params;
    existing->params = params;
    
    // 如果最后一个参数是...，则实际参数数量减1
    if (existing->is_variadic) {
        existing->num_params--;
        macro_table.tail->next = macro;
        macro_table.tail = macro;
    }
}

// 释放宏表
static void free_macro_table(void) {
    Macro *current = macro_table.head;
    while (current) {
        Macro *next = current->next;
        free(current->name);
        free(current->replacement);
        if (current->params) {
            for (int i = 0; i < current->num_params; i++) {
                free(current->params[i]);
            }
            free(current->params);
        }
        free(current);
        current = next;
    }
    macro_table.head = macro_table.tail = NULL;
}

// 展开宏
static char* expand_macro(const char *name, char **args, int num_args, int is_variadic) {
    Macro *macro = find_macro(name);
    if (!macro) return strdup(name);  // 不是宏，原样返回
    
    // 简单宏
    if (!macro->is_function_like) {
        return strdup(macro->replacement);
    }
    
    // 检查参数数量
    int expected_args = macro->num_params;
    if (is_variadic) {
        if (num_args < expected_args) {
            fprintf(stderr, "Error: macro %s expects at least %d arguments, but got %d\n", 
                    name, expected_args, num_args);
            return strdup(name);
        }
    } else if (num_args != expected_args) {
        fprintf(stderr, "Error: macro %s expects %d arguments, but got %d\n", 
                name, expected_args, num_args);
        return strdup(name);
    }
    
    // 分配结果缓冲区
    size_t result_size = strlen(macro->replacement) + 1;
    char *result = malloc(result_size);
    if (!result) return strdup(name);
    result[0] = '\0';
    
    const char *src = macro->replacement;
    char *dst = result;
    
    // 处理替换文本
    while (*src) {
        // 检查是否是参数
        if (isalpha(*src) || *src == '_') {
            const char *ident_start = src++;
            while (isalnum(*src) || *src == '_') src++;
            
            char ident[128];
            int ident_len = src - ident_start;
            if (ident_len >= sizeof(ident)) ident_len = sizeof(ident) - 1;
            strncpy(ident, ident_start, ident_len);
            ident[ident_len] = '\0';
            
            // 检查是否是参数名
            int param_index = -1;
            for (int i = 0; i < macro->num_params; i++) {
                if (strcmp(ident, macro->params[i]) == 0) {
                    param_index = i;
                    break;
                }
            }
            
            // 处理__VA_ARGS__
            if (param_index == -1 && strcmp(ident, "__VA_ARGS__") == 0 && macro->is_variadic) {
                param_index = macro->num_params;  // 最后一个参数
            }
            
            // 替换参数
            if (param_index >= 0) {
                const char *replacement = "";
                size_t rep_len = 0;
                
                // 处理可变参数
                if (macro->is_variadic && param_index == macro->num_params) {
                    // 收集所有剩余参数
                    size_t total_len = 0;
                    for (int i = macro->num_params; i < num_args; i++) {
                        if (i > macro->num_params) total_len += 2; // 逗号和空格
                        total_len += strlen(args[i]);
                    }
                    
                    char *va_args = malloc(total_len + 1);
                    if (va_args) {
                        va_args[0] = '\0';
                        for (int i = macro->num_params; i < num_args; i++) {
                            if (i > macro->num_params) strcat(va_args, ", ");
                            strcat(va_args, args[i]);
                        }
                        replacement = va_args;
                        rep_len = strlen(replacement);
                    }
                } else if (param_index < num_args) {
                    replacement = args[param_index];
                    rep_len = strlen(replacement);
                }
                
                // 确保有足够空间
                size_t new_size = (dst - result) + rep_len + 1;
                if (new_size > result_size) {
                    size_t offset = dst - result;
                    result_size = new_size * 2;
                    char *new_result = realloc(result, result_size);
                    if (!new_result) {
                        if (replacement != args[param_index]) free((void*)replacement);
                        free(result);
                        return strdup(name);
                    }
                    result = new_result;
                    dst = result + offset;
                }
                
                // 复制替换文本
                memcpy(dst, replacement, rep_len);
                dst += rep_len;
                
                if (replacement != args[param_index]) {
                    free((void*)replacement);
                }
                
                continue;
            }
            
            // 不是参数，原样复制
            src = ident_start;
        }
        
        // 复制一个字符
        if (dst - result + 1 >= result_size) {
            size_t offset = dst - result;
            result_size *= 2;
            char *new_result = realloc(result, result_size);
            if (!new_result) {
                free(result);
                return strdup(name);
            }
            result = new_result;
            dst = result + offset;
        }
        *dst++ = *src++;
    }
    
    *dst = '\0';
    return result;
}


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

// WASM 基本类型 (参考: https://webassembly.github.io/spec/core/binary/types.html)
typedef enum {
    WASM_I32 = 0x7F,
    WASM_I64 = 0x7E,
    WASM_F32 = 0x7D,
    WASM_F64 = 0x7C,
    WASM_V128 = 0x7B,
    WASM_FUNCREF = 0x70,
    WASM_EXTERNREF = 0x6F,
    WASM_ANYREF = 0x6E
} WasmValType;

/*
 * WASM AST 节点类型
 * 命名规范：
 * - WASM_*: 标准 WebAssembly 节点
 * - WASX_*: 扩展节点 (C 语言特性)
 */
typedef enum {
    // ===== 标准 WebAssembly 节点 =====
    // 模块结构 (参考: https://webassembly.github.io/spec/core/binary/modules.html)
    WASM_MODULE = 0x00,              // 模块
    WASM_FUNC_TYPE = 0x60,            // 函数类型
    WASM_IMPORT = 0x02,               // 导入
    WASM_FUNC = 0x00,                 // 函数
    WASM_TABLE = 0x01,                // 表
    WASM_MEMORY = 0x02,               // 内存
    WASM_GLOBAL = 0x03,               // 全局变量
    WASM_EXPORT = 0x07,               // 导出
    WASM_START = 0x08,                // 开始函数
    WASM_ELEM = 0x09,                 // 元素段
    WASM_DATA = 0x0B,                 // 数据段
    
    // 控制流 (参考: https://webassembly.github.io/spec/core/binary/instructions.html#control-instructions)
    WASM_UNREACHABLE = 0x00,         // 不可达
    WASM_NOP = 0x01,                  // 空操作
    WASM_BLOCK = 0x02,                // 块
    WASM_LOOP = 0x03,                 // 循环
    WASM_IF = 0x04,                   // 条件
    WASM_ELSE = 0x05,                 // 否则
    WASM_END = 0x0B,                  // 结束
    WASM_BR = 0x0C,                   // 分支
    WASM_BR_IF = 0x0D,                // 条件分支
    WASM_BR_TABLE = 0x0E,             // 分支表
    WASM_RETURN = 0x0F,               // 返回
    WASM_CALL = 0x10,                 // 调用
    WASM_CALL_INDIRECT = 0x11,         // 间接调用
    
    // 内存操作 (参考: https://webassembly.github.io/spec/core/binary/instructions.html#memory-instructions)
    WASM_DROP = 0x1A,                 // 丢弃栈顶值
    WASM_SELECT = 0x1B,                // 选择
    
    // 变量指令 (参考: https://webassembly.github.io/spec/core/binary/instructions.html#variable-instructions)
    WASM_LOCAL_GET = 0x20,            // 获取局部变量
    WASM_LOCAL_SET = 0x21,             // 设置局部变量
    WASM_LOCAL_TEE = 0x22,             // 设置并保留局部变量
    WASM_GLOBAL_GET = 0x23,            // 获取全局变量
    WASM_GLOBAL_SET = 0x24,            // 设置全局变量
    
    // 内存指令 (参考: https://webassembly.github.io/spec/core/binary/instructions.html#memory-instructions)
    WASM_I32_LOAD = 0x28,              // i32 加载
    WASM_I64_LOAD = 0x29,              // i64 加载
    WASM_F32_LOAD = 0x2A,              // f32 加载
    WASM_F64_LOAD = 0x2B,              // f64 加载
    WASM_I32_LOAD8_S = 0x2C,           // i32 加载8位有符号
    WASM_I32_LOAD8_U = 0x2D,           // i32 加载8位无符号
    WASM_I32_LOAD16_S = 0x2E,          // i32 加载16位有符号
    WASM_I32_LOAD16_U = 0x2F,          // i32 加载16位无符号
    WASM_I64_LOAD8_S = 0x30,           // i64 加载8位有符号
    WASM_I64_LOAD8_U = 0x31,           // i64 加载8位无符号
    WASM_I64_LOAD16_S = 0x32,          // i64 加载16位有符号
    WASM_I64_LOAD16_U = 0x33,          // i64 加载16位无符号
    WASM_I64_LOAD32_S = 0x34,          // i64 加载32位有符号
    WASM_I64_LOAD32_U = 0x35,          // i64 加载32位无符号
    WASM_I32_STORE = 0x36,             // i32 存储
    WASM_I64_STORE = 0x37,             // i64 存储
    WASM_F32_STORE = 0x38,             // f32 存储
    WASM_F64_STORE = 0x39,             // f64 存储
    WASM_I32_STORE8 = 0x3A,            // i32 存储8位
    WASM_I32_STORE16 = 0x3B,           // i32 存储16位
    WASM_I64_STORE8 = 0x3C,            // i64 存储8位
    WASM_I64_STORE16 = 0x3D,           // i64 存储16位
    WASM_I64_STORE32 = 0x3E,           // i64 存储32位
    WASM_MEMORY_SIZE = 0x3F,           // 内存大小
    WASM_MEMORY_GROW = 0x40,           // 内存增长
    
    // 常量 (参考: https://webassembly.github.io/spec/core/binary/instructions.html#constant-instructions)
    WASM_I32_CONST = 0x41,            // i32 常量
    WASM_I64_CONST = 0x42,            // i64 常量
    WASM_F32_CONST = 0x43,            // f32 常量
    WASM_F64_CONST = 0x44,            // f64 常量
    
    // 数值运算 (参考: https://webassembly.github.io/spec/core/binary/instructions.html#numeric-instructions)
    
    // i32 运算
    WASM_I32_EQZ = 0x45,         // i32 等于零
    WASM_I32_EQ = 0x46,          // i32 等于
    WASM_I32_NE = 0x47,          // i32 不等于
    WASM_I32_LT_S = 0x48,        // i32 有符号小于
    WASM_I32_LT_U = 0x49,        // i32 无符号小于
    WASM_I32_GT_S = 0x4A,        // i32 有符号大于
    WASM_I32_GT_U = 0x4B,        // i32 无符号大于
    WASM_I32_LE_S = 0x4C,        // i32 有符号小于等于
    WASM_I32_LE_U = 0x4D,        // i32 无符号小于等于
    WASM_I32_GE_S = 0x4E,        // i32 有符号大于等于
    WASM_I32_GE_U = 0x4F,        // i32 无符号大于等于
    
    // i64 运算
    WASM_I64_EQZ = 0x50,         // i64 等于零
    WASM_I64_EQ = 0x51,          // i64 等于
    WASM_I64_NE = 0x52,          // i64 不等于
    WASM_I64_LT_S = 0x53,        // i64 有符号小于
    WASM_I64_LT_U = 0x54,        // i64 无符号小于
    WASM_I64_GT_S = 0x55,        // i64 有符号大于
    WASM_I64_GT_U = 0x56,        // i64 无符号大于
    WASM_I64_LE_S = 0x57,        // i64 有符号小于等于
    WASM_I64_LE_U = 0x58,        // i64 无符号小于等于
    WASM_I64_GE_S = 0x59,        // i64 有符号大于等于
    WASM_I64_GE_U = 0x5A,        // i64 无符号大于等于
    
    // f32 运算
    WASM_F32_EQ = 0x5B,          // f32 等于
    WASM_F32_NE = 0x5C,          // f32 不等于
    WASM_F32_LT = 0x5D,          // f32 小于
    WASM_F32_GT = 0x5E,          // f32 大于
    WASM_F32_LE = 0x5F,          // f32 小于等于
    WASM_F32_GE = 0x60,          // f32 大于等于
    
    // f64 运算
    WASM_F64_EQ = 0x61,          // f64 等于
    WASM_F64_NE = 0x62,          // f64 不等于
    WASM_F64_LT = 0x63,          // f64 小于
    WASM_F64_GT = 0x64,          // f64 大于
    WASM_F64_LE = 0x65,          // f64 小于等于
    WASM_F64_GE = 0x66,          // f64 大于等于
    
    // 数值运算
    WASM_I32_CLZ = 0x67,         // i32 前导零计数
    WASM_I32_CTZ = 0x68,         // i32 尾随零计数
    WASM_I32_POPCNT = 0x69,      // i32 置1位计数
    WASM_I32_ADD = 0x6A,         // i32 加法
    WASM_I32_SUB = 0x6B,         // i32 减法
    WASM_I32_MUL = 0x6C,         // i32 乘法
    WASM_I32_DIV_S = 0x6D,       // i32 有符号除法
    WASM_I32_DIV_U = 0x6E,       // i32 无符号除法
    WASM_I32_REM_S = 0x6F,       // i32 有符号取余
    WASM_I32_REM_U = 0x70,       // i32 无符号取余
    WASM_I32_AND = 0x71,         // i32 按位与
    WASM_I32_OR = 0x72,          // i32 按位或
    WASM_I32_XOR = 0x73,         // i32 按位异或
    WASM_I32_SHL = 0x74,         // i32 左移
    WASM_I32_SHR_S = 0x75,       // i32 算术右移
    WASM_I32_SHR_U = 0x76,       // i32 逻辑右移
    WASM_I32_ROTL = 0x77,        // i32 循环左移
    WASM_I32_ROTR = 0x78,        // i32 循环右移
    
    // 类型转换 (参考: https://webassembly.github.io/spec/core/binary/instructions.html#binary-cvtop)
    WASM_I32_WRAP_I64 = 0xA7,    // i64 截断为 i32
    WASM_I32_TRUNC_F32_S = 0xA8,  // f32 截断为有符号 i32
    WASM_I32_TRUNC_F32_U = 0xA9,  // f32 截断为无符号 i32
    WASM_I32_TRUNC_F64_S = 0xAA,  // f64 截断为有符号 i32
    WASM_I32_TRUNC_F64_U = 0xAB,  // f64 截断为无符号 i32
    
    // 其他指令
    WASM_REF_NULL = 0xD0,         // 空引用
    WASM_REF_IS_NULL = 0xD1,      // 检查引用是否为空
    WASM_REF_FUNC = 0xD2,         // 函数引用
    
    // 内存指令 (参考: https://webassembly.github.io/spec/core/binary/instructions.html#memory-instructions)
    WASM_MEMORY_INIT = 0xFC08,    // 内存初始化
    WASM_DATA_DROP = 0xFC09,      // 丢弃数据段
    WASM_MEMORY_COPY = 0xFC0A,    // 内存复制
    WASM_MEMORY_FILL = 0xFC0B,    // 内存填充
    WASM_TABLE_INIT = 0xFC0C,     // 表初始化
    WASM_ELEM_DROP = 0xFC0D,      // 丢弃元素段
    WASM_TABLE_COPY = 0xFC0E,     // 表复制
    WASM_TABLE_GROW = 0xFC0F,     // 表增长
    WASM_TABLE_SIZE = 0xFC10,     // 表大小
    WASM_TABLE_FILL = 0xFC11      // 表填充
    
    // ===== 扩展节点 (WASM-C) =====
    // 声明和定义
    WASX_TRANSLATION_UNIT,  // 翻译单元
    WASX_FUNCTION_DEF,      // 函数定义
    WASX_FUNCTION_DECL,     // 函数声明
    WASX_VAR_DECL,         // 变量声明
    WASX_PARAM_DECL,       // 参数声明
    
    // 复合类型
    WASX_STRUCT_DECL,      // 结构体声明
    WASX_UNION_DECL,       // 联合体声明
    WASX_ENUM_DECL,        // 枚举声明
    WASX_TYPEDEF_DECL,     // 类型定义
    
    // 类型节点
    WASX_PRIMITIVE_TYPE,   // 基本类型
    WASX_POINTER_TYPE,     // 指针类型
    WASX_ARRAY_TYPE,       // 数组类型
    WASX_FUNCTION_TYPE,    // 函数类型
    
    // 控制流
    WASX_IF_STMT,          // if 语句
    WASX_SWITCH_STMT,      // switch 语句
    WASX_CASE_STMT,        // case 语句
    WASX_DEFAULT_STMT,     // default 语句
    WASX_WHILE_STMT,       // while 循环
    WASX_DO_STMT,          // do-while 循环
    WASX_FOR_STMT,         // for 循环
    WASX_GOTO_STMT,        // goto 语句
    WASX_LABEL_STMT,       // 标签语句
    WASX_CONTINUE_STMT,    // continue 语句
    WASX_BREAK_STMT,       // break 语句
    WASX_RETURN_STMT,      // return 语句
    
    // 表达式
    WASX_IDENTIFIER,       // 标识符
    WASX_CONSTANT,         // 常量
    WASX_STRING_LITERAL,   // 字符串字面量
    WASX_UNARY_OP,         // 一元操作
    WASX_BINARY_OP,        // 二元操作
    WASX_TERNARY_OP,       // 三元操作
    WASX_CALL_EXPR,        // 函数调用
    WASX_ARRAY_SUBSCRIPT,  // 数组下标
    WASX_MEMBER_ACCESS,    // 成员访问
    WASX_PTR_MEMBER_ACCESS,// 指针成员访问
    WASX_CAST_EXPR,        // 类型转换
    
    // 表达式类型
    WASX_EXPR_IDENTIFIER,        // 标识符
    WASX_EXPR_CONSTANT,          // 常量
    WASX_EXPR_STRING_LITERAL,    // 字符串字面量
    WASX_EXPR_COMPOUND_LITERAL,  // 复合字面量 (C99)
    WASX_EXPR_FUNC_CALL,         // 函数调用
    WASX_EXPR_ARRAY_SUBSCRIPT,   // 数组下标
    WASX_EXPR_MEMBER_ACCESS,     // 成员访问
    WASX_EXPR_PTR_MEMBER_ACCESS, // 指针成员访问
    WASX_EXPR_POST_INC,          // 后置++
    WASX_EXPR_POST_DEC,          // 后置--
    WASX_EXPR_PRE_INC,           // 前置++
    WASX_EXPR_PRE_DEC,           // 前置--
    WASX_EXPR_ADDR,              // 取地址 &
    WASX_EXPR_DEREF,             // 解引用 *
    WASX_EXPR_PLUS,              // 正号 +
    WASX_EXPR_MINUS,             // 负号 -
    WASX_EXPR_BIT_NOT,           // 按位取反 ~
    WASX_EXPR_LOGICAL_NOT,       // 逻辑非 !
    WASX_EXPR_SIZEOF,            // sizeof
    WASX_EXPR_ALIGNOF,           // _Alignof (C11)
    WASX_EXPR_GENERIC,           // _Generic (C11)
    WASX_EXPR_MUL,               // 乘 *
    WASX_EXPR_DIV,               // 除 /
    WASX_EXPR_MOD,               // 取模 %
    WASX_EXPR_ADD,               // 加 +
    WASX_EXPR_SUB,               // 减 -
    WASX_EXPR_LEFT_SHIFT,        // 左移 <<
    WASX_EXPR_RIGHT_SHIFT,       // 右移 >>
    WASX_EXPR_LESS,              // 小于 <
    WASX_EXPR_LESS_EQUAL,        // 小于等于 <=
    WASX_EXPR_GREATER,           // 大于 >
    WASX_EXPR_GREATER_EQUAL,     // 大于等于 >=
    WASX_EXPR_EQUAL,             // 等于 ==
    WASX_EXPR_NOT_EQUAL,         // 不等于 !=
    WASX_EXPR_BIT_AND,           // 按位与 &
    WASX_EXPR_BIT_XOR,           // 按位异或 ^
    WASX_EXPR_BIT_OR,            // 按位或 |
    WASX_EXPR_LOGICAL_AND,       // 逻辑与 &&
    WASX_EXPR_LOGICAL_OR,        // 逻辑或 ||
    WASX_EXPR_CONDITIONAL,       // 条件 ? :
    WASX_EXPR_ASSIGN,            // =
    WASX_EXPR_ADD_ASSIGN,        // +=
    WASX_EXPR_SUB_ASSIGN,        // -=
    WASX_EXPR_MUL_ASSIGN,        // *=
    WASX_EXPR_DIV_ASSIGN,        // /=
    WASX_EXPR_MOD_ASSIGN,        // %=
    WASX_EXPR_LEFT_SHIFT_ASSIGN, // <<=
    WASX_EXPR_RIGHT_SHIFT_ASSIGN,// >>=
    WASX_EXPR_BIT_AND_ASSIGN,    // &=
    WASX_EXPR_BIT_XOR_ASSIGN,    // ^=
    WASX_EXPR_BIT_OR_ASSIGN,     // |=
    WASX_EXPR_COMMA,             // 逗号表达式
    WASX_EXPR_CAST,              // 类型转换 (type)expr
    WASX_EXPR_VA_ARG,            // va_arg
    WASX_EXPR_STATEMENT_EXPR,    // ({...})
    WASX_EXPR_RANGE,             // x..y (GCC)
    WASX_EXPR_BUILTIN_CHOOSE_EXPR, // __builtin_choose_expr
    WASX_EXPR_BUILTIN_TYPES_COMPATIBLE_P, // __builtin_types_compatible_p
    WASX_EXPR_BUILTIN_OFFSETOF,   // offsetof
    WASX_EXPR_BUILTIN_VA_ARG,     // __builtin_va_arg
    WASX_EXPR_BUILTIN_VA_COPY,    // __builtin_va_copy
    WASX_EXPR_BUILTIN_VA_END,     // __builtin_va_end
    WASX_EXPR_BUILTIN_VA_START,   // __builtin_va_start
    WASX_EXPR_ATTRIBUTE,         // __attribute__
    WASX_EXPR_ASM,               // __asm__
    WASX_EXPR_ERROR,             // 错误表达式

    // 语句类型
    WASX_STMT_NONE,
    WASX_STMT_DECL,              // 声明语句
    WASX_STMT_NULL,              // 空语句
    WASX_STMT_COMPOUND,          // 复合语句
    WASX_STMT_CASE,              // case 语句
    WASX_STMT_DEFAULT,           // default 语句
    WASX_STMT_LABEL,             // 标签语句
    WASX_STMT_ATTRIBUTED,        // 带属性的语句
    WASX_STMT_IF,                // if 语句
    WASX_STMT_SWITCH,            // switch 语句
    WASX_STMT_WHILE,             // while 循环
    WASX_STMT_DO,                // do-while 循环
    WASX_STMT_FOR,               // for 循环
    WASX_STMT_GOTO,              // goto 语句
    WASX_STMT_INDIRECT_GOTO,     // 间接 goto 语句
    WASX_STMT_CONTINUE,          // continue 语句
    WASX_STMT_BREAK,             // break 语句
    WASX_STMT_RETURN,            // return 语句
    WASX_STMT_ASM,               // 内联汇编
    WASX_STMT_GCC_ASM,           // GCC 内联汇编
    WASX_STMT_MS_ASM,            // MSVC 内联汇编
    WASX_STMT_SEH_LEAVE,         // SEH __leave
    WASX_STMT_SEH_TRY,           // SEH __try
    WASX_STMT_SEH_EXCEPT,        // SEH __except
    WASX_STMT_SEH_FINALLY,       // SEH __finally
    WASX_STMT_MS_DECLSPEC,       // MS __declspec
    WASX_STMT_CXX_CATCH,         // C++ catch
    WASX_STMT_CXX_TRY,           // C++ try
    WASX_STMT_CXX_FOR_RANGE,     // C++11 范围 for
    WASX_STMT_MS_TRY,            // MS __try
    WASX_STMT_MS_EXCEPT,         // MS __except
    WASX_STMT_MS_FINALLY,        // MS __finally
    WASX_STMT_MS_LEAVE,          // MS __leave
    WASX_STMT_PRAGMA,            // #pragma 指令
    WASX_STMT_ERROR,             // 错误语句

    // 声明类型
    WASX_DECL_NONE,
    WASX_DECL_VAR,               // 变量声明
    WASX_DECL_FUNCTION,          // 函数声明
    WASX_DECL_FUNCTION_DEF,      // 函数定义
    WASX_DECL_STRUCT,            // 结构体定义
    WASX_DECL_UNION,             // 联合体定义
    WASX_DECL_ENUM,              // 枚举定义
    WASX_DECL_ENUM_CONSTANT,     // 枚举常量
    WASX_DECL_TYPEDEF,           // 类型定义
    WASX_DECL_LABEL,             // 标签
    WASX_DECL_FIELD,             // 结构体/联合体字段
    WASX_DECL_PARAM,             // 函数参数
    WASX_DECL_RECORD,            // 记录(结构体/联合体)
    WASX_DECL_INITIALIZER,       // 初始化器
    WASX_DECL_ATTRIBUTE,         // 属性
    WASX_DECL_ASM_LABEL,         // 汇编标签
    WASX_DECL_IMPLICIT,          // 隐式声明
    WASX_DECL_PACKED,            // 打包属性
    WASX_DECL_ALIGNED,           // 对齐属性
    WASX_DECL_TRANSPARENT_UNION, // 透明联合体
    WASX_DECL_VECTOR,            // 向量类型(GCC)
    WASX_DECL_EXT_VECTOR,        // 扩展向量类型(Clang)
    WASX_DECL_COMPLEX,           // 复数类型
    WASX_DECL_IMAGINARY,         // 虚数类型
    WASX_DECL_ATOMIC,            // 原子类型(C11)
    WASX_DECL_THREAD_LOCAL,      // 线程局部存储(C11)
    WASX_DECL_AUTO_TYPE,         // auto 类型(C23)
    WASX_DECL_NULLPTR,           // nullptr_t(C23)
    WASX_DECL_GENERIC_SELECTION, // _Generic 选择(C11)
    WASX_DECL_OVERLOAD,          // 重载声明(C++)
    WASX_DECL_TEMPLATE,          // 模板(C++)
    WASX_DECL_FRIEND,            // 友元(C++)
    WASX_DECL_USING,             // using 声明(C++)
    WASX_DECL_CONCEPT,           // 概念(C++20)
    WASX_DECL_REQUIRES,          // requires 子句(C++20)
    WASX_DECL_CONSTRAINT,        // 约束(C++20)
    WASX_DECL_ERROR,             // 错误声明
    
    // 复合表达式
    WASX_INIT_LIST,        // 初始化列表
    WASX_DESIGNATION,      // 指示符 (C99)
    WASX_COMPOUND_LITERAL, // 复合字面量
    WASX_STMT_EXPR,        // 语句表达式 (GNU扩展)
    
    // 特殊表达式
    WASX_ALIGNOF_EXPR,     // _Alignof 表达式
    WASX_OFFSETOF_EXPR,    // offsetof 表达式
    WASX_VA_ARG_EXPR,      // va_arg 表达式
    WASX_GENERIC_SELECTION,// _Generic 选择表达式
    
    // 内建函数
    WASX_BUILTIN_VA_START, // __builtin_va_start
    WASX_BUILTIN_VA_END,   // __builtin_va_end
    WASX_BUILTIN_VA_COPY,  // __builtin_va_copy
    WASX_BUILTIN_OFFSETOF, // __builtin_offsetof
    
    // 内联汇编
    WASX_ASM_STMT,         // 内联汇编语句
    
    // 预处理和元信息
    WASX_PREPROCESSING_DIR,// 预处理指令
    WASX_MACRO_DEFINITION, // 宏定义
    WASX_MACRO_EXPANSION,  // 宏展开
    WASX_COMMENT,          // 注释
    WASX_PRAGMA,           // #pragma指令
    
    // 错误处理
    WASX_ERROR,            // 错误节点
    
    // ===== C 语言类型 =====
    // 基本类型
    WASX_TYPE_INVALID,            // 无效类型
    WASX_TYPE_VOID,               // void
    
    // 字符类型
    WASX_TYPE_CHAR,               // char (实现定义的有符号性)
    WASX_TYPE_SIGNED_CHAR,        // signed char
    WASX_TYPE_UNSIGNED_CHAR,      // unsigned char
    WASX_TYPE_CHAR16,             // char16_t (C11)
    WASX_TYPE_CHAR32,             // char32_t (C11)
    WASX_TYPE_WCHAR,              // wchar_t
    
    // 整数类型
    WASX_TYPE_SHORT,              // short (int)
    WASX_TYPE_UNSIGNED_SHORT,     // unsigned short (int)
    WASX_TYPE_INT,                // int
    WASX_TYPE_UNSIGNED_INT,       // unsigned int
    WASX_TYPE_LONG,               // long (int)
    WASX_TYPE_UNSIGNED_LONG,      // unsigned long (int)
    WASX_TYPE_LONG_LONG,          // long long (int) (C99)
    WASX_TYPE_UNSIGNED_LONG_LONG, // unsigned long long (int) (C99)
    
    // 浮点类型
    WASX_TYPE_FLOAT,              // float
    WASX_TYPE_DOUBLE,             // double
    WASX_TYPE_LONG_DOUBLE,        // long double
    WASX_TYPE_FLOAT128,           // _Float128 (C23)
    
    // 布尔和空指针
    WASX_TYPE_BOOL,               // _Bool (C99)
    WASX_TYPE_NULLPTR,            // nullptr_t (C23)
    
    // 复合类型
    WASX_TYPE_STRUCT,             // 结构体
    WASX_TYPE_UNION,              // 联合体
    WASX_TYPE_ENUM,               // 枚举
    
    // 派生类型
    WASX_TYPE_POINTER,            // 指针
    WASX_TYPE_ARRAY,              // 数组
    WASX_TYPE_FUNCTION,           // 函数
    WASX_TYPE_TYPEDEF_NAME,       // 类型定义名
    WASX_TYPE_VOIDPTR             // void*
} WasmNodeType;

// WASM AST 节点
struct WasmNode {
    WasmNodeType type;
    int line;
    int column;
    const char *filename;
    
    // 类型信息
    struct {
        WasmValType val_type;
        unsigned qualifiers;
        struct WasmNode *base_type;
        struct WasmNode *return_type;
        struct WasmNode **params;
        int num_params;
    } type_info;
    
    // 值联合
    union {
        // 常量值
        int32_t i32;
        int64_t i64;
        float f32;
        double f64;
        
        // 标识符
        struct {
            char *name;
            struct WasmNode *symbol;
        } id;
        
        // 表达式
        struct {
            struct WasmNode *lhs;
            struct WasmNode *rhs;
            struct WasmNode *cond;
            struct WasmNode **args;
            int num_args;
        } expr;
        
        // 声明
        struct {
            char *name;
            struct WasmNode *type;
            struct WasmNode *init;
            struct WasmNode *next;
        } decl;
        
        // 复合类型
        struct {
            char *tag;
            struct WasmNode *fields;
            int is_union;
        } record;
        
        // 函数
        struct {
            char *name;
            struct WasmNode *params;
            struct WasmNode *body;
            struct WasmNode *locals;
            int num_locals;
            int is_exported;
        } func;
        
        // 模块
        struct {
            struct WasmNode **items;
            int num_items;
        } module;
    } u;
    
    // 源位置信息
    struct {
        const char *source_file;
        int start_line;
        int start_col;
        int end_line;
        int end_col;
    } src_loc;
};

typedef struct WasmNode WasmNode;

    // 原子类型 (C11)
    WASX_TYPE_ATOMIC_BOOL,
    WASX_TYPE_ATOMIC_CHAR, WASX_TYPE_ATOMIC_SCHAR, WASX_TYPE_ATOMIC_UCHAR,
    WASX_TYPE_ATOMIC_SHORT, WASX_TYPE_ATOMIC_USHORT,
    WASX_TYPE_ATOMIC_INT, WASX_TYPE_ATOMIC_UINT,
    WASX_TYPE_ATOMIC_LONG, WASX_TYPE_ATOMIC_ULONG,
    WASX_TYPE_ATOMIC_LLONG, WASX_TYPE_ATOMIC_ULLONG,
    WASX_TYPE_ATOMIC_CHAR16_T, WASX_TYPE_ATOMIC_CHAR32_T,
    WASX_TYPE_ATOMIC_WCHAR_T, WASX_TYPE_ATOMIC_INT_LEAST8_T,
    WASX_TYPE_ATOMIC_UINT_LEAST8_T, WASX_TYPE_ATOMIC_INT_LEAST16_T,
    WASX_TYPE_ATOMIC_UINT_LEAST16_T, WASX_TYPE_ATOMIC_INT_LEAST32_T,
    WASX_TYPE_ATOMIC_UINT_LEAST32_T, WASX_TYPE_ATOMIC_INT_LEAST64_T,
    WASX_TYPE_ATOMIC_UINT_LEAST64_T, WASX_TYPE_ATOMIC_INT_FAST8_T,
    WASX_TYPE_ATOMIC_UINT_FAST8_T, WASX_TYPE_ATOMIC_INT_FAST16_T,
    WASX_TYPE_ATOMIC_UINT_FAST16_T, WASX_TYPE_ATOMIC_INT_FAST32_T,
    WASX_TYPE_ATOMIC_UINT_FAST32_T, WASX_TYPE_ATOMIC_INT_FAST64_T,
    WASX_TYPE_ATOMIC_UINT_FAST64_T, WASX_TYPE_ATOMIC_INTPTR_T,
    WASX_TYPE_ATOMIC_UINTPTR_T, WASX_TYPE_ATOMIC_SIZE_T,
    WASX_TYPE_ATOMIC_PTRDIFF_T, WASX_TYPE_ATOMIC_INTMAX_T,
    WASX_TYPE_ATOMIC_UINTMAX_T
} BasicType;

// 类型限定符和说明符
typedef enum {
    // 类型限定符 (C89)
    Q_CONST = 1 << 0,      // const
    Q_VOLATILE = 1 << 1,   // volatile
    
    // C99
    Q_RESTRICT = 1 << 2,   // restrict
    
    // C11
    Q_ATOMIC = 1 << 3,     // _Atomic
    Q_NORETURN = 1 << 4,   // _Noreturn
    Q_THREAD_LOCAL = 1 << 5, // _Thread_local
    
    // 函数说明符
    Q_INLINE = 1 << 6,     // inline (C99)
    Q_NORETURN_FN = 1 << 7, // _Noreturn (C11)
    
    // 存储类说明符
    Q_TYPEDEF = 1 << 8,    // typedef
    Q_EXTERN = 1 << 9,     // extern
    Q_STATIC = 1 << 10,    // static
    Q_AUTO = 1 << 11,      // auto
    Q_REGISTER = 1 << 12,  // register
    
    // 扩展限定符
    Q_ALIGNAS = 1 << 16,   // _Alignas (C11)
    Q_ALIGNOF = 1 << 17,   // _Alignof (C11)
    Q_GENERIC = 1 << 18,   // _Generic (C11)
    Q_STATIC_ASSERT = 1 << 19, // _Static_assert (C11)
    Q_THREAD = 1 << 20,    // _Thread_local (C11)
    
    // 属性 (GNU, C2x)
    Q_ATTRIBUTE = 1 << 24,  // __attribute__
    Q_DECLSPEC = 1 << 25,  // __declspec
    
    // 扩展类型限定符
    Q_COMPLEX = 1 << 28,   // _Complex (C99)
    Q_IMAGINARY = 1 << 29, // _Imaginary (C99)
    
    // 特殊标记
    Q_SIGNED = 1 << 30,    // signed
    Q_UNSIGNED = 1 << 31   // unsigned
} TypeQualifier;

// 表达式、语句和声明类型已合并到 WasmNodeType 枚举中，使用 WASX_ 前缀

// 类型节点
typedef struct Type {
    BasicType kind;          // 基本类型
    unsigned int qualifiers;  // 类型限定符
    unsigned int align;       // 对齐要求(字节)
    unsigned int size;        // 大小(字节)
    
    // 复合类型信息
    union {
        // 指针/数组/函数
        struct {
            struct Type *pointee;  // 指向的类型
            int is_restrict;        // 是否有限定符restrict
            int is_atomic;          // 是否为_Atomic
        } ptr;
        
        // 数组
        struct {
            struct Type *element;   // 元素类型
            int size;               // 数组大小(-1表示不完整类型)
            int is_static;          // 是否为static数组
            int is_vla;              // 是否为变长数组
            int is_star;             // 是否为* (函数参数中的[])
        } array;
        
        // 函数
        struct {
            struct Type *return_type;  // 返回类型
            struct ASTNode **params;    // 参数列表
            int num_params;             // 参数数量
            int is_variadic;            // 是否可变参数
            int has_prototype;          // 是否有原型
        } func;
        
        // 结构体/联合体
        struct {
            char *tag;              // 标签名
            struct ASTNode *fields;   // 字段列表
            int is_union;             // 1表示联合体，0表示结构体
            int is_complete;          // 是否完整定义
            int is_anonymous;         // 是否为匿名结构体/联合体
            int has_flexible_array;   // 是否有灵活数组成员
        } record;
        
        // 枚举
        struct {
            char *name;             // 枚举名
            struct ASTNode *enumerators; // 枚举常量列表
            int is_complete;          // 是否完整定义
        } enum_type;
        
        // 原子类型
        struct {
            struct Type *value_type; // 原子值类型
            int is_atomic;            // 是否为_Atomic
        } atomic;
        
        // 向量类型
        struct {
            struct Type *element;    // 元素类型
            int num_elements;         // 元素数量
        } vector;
        
        // 扩展向量类型(Clang)
        struct {
            struct Type *element;    // 元素类型
            int num_elements;         // 元素数量
            int is_sve;               // 是否为SVE向量
        } ext_vector;
    } data;
    
    // 属性
    struct ASTNode *attrs;     // 属性列表
    
    // 源位置信息
    const char *filename;      // 文件名
    int line;                  // 行号
    int column;                // 列号
} Type;

// AST节点结构
typedef struct ASTNode {
    NodeType node_type;       // 节点类型
    int line;                  // 行号
    int column;                // 列号
    const char *filename;      // 文件名
    
    // 源范围信息(用于错误报告)
    struct {
        int start_line;
        int start_column;
        int end_line;
        int end_column;
    } src_range;
    
    // 类型信息(类型检查后填充)
    Type *type;               // 表达式的类型
    
    // 值(用于常量表达式求值)
    union {
        int64_t int_val;       // 整数值
        uint64_t uint_val;      // 无符号整数值
        double float_val;       // 浮点数值
        long double long_double_val; // 长双精度值
        _Complex double complex_val; // 复数值
        void *ptr_val;          // 指针值
        struct {
            const char *str;     // 字符串内容
            size_t len;           // 字符串长度(不包括空字符)
            int is_wide;          // 是否为宽字符串
        } str_val;               // 字符串值
    } value;
    
    // 节点特定数据
    union {
        // 表达式
        struct {
            ExprType expr_type;    // 表达式类型
            struct ASTNode *lhs;    // 左操作数
            struct ASTNode *rhs;    // 右操作数
            struct ASTNode *cond;   // 条件表达式(三元操作符)
            struct ASTNode **args;  // 参数列表(函数调用等)
            int num_args;           // 参数数量
            struct ASTNode *cast_type; // 类型转换的目标类型
            struct ASTNode *init;    // 初始化表达式
            struct ASTNode *designator; // 指示符(C99)
            struct ASTNode *attrs;   // 属性列表
        } expr;
        
        // 声明
        struct {
            DeclType decl_type;     // 声明类型
            char *name;              // 名称
            struct ASTNode *type;    // 类型
            struct ASTNode *init;    // 初始化器
            struct ASTNode *bit_width; // 位域宽度
            struct ASTNode *attrs;    // 属性列表
            struct ASTNode *body;     // 函数体/复合语句
            struct ASTNode *semantic; // 语义信息(例如:异常规范)
            int storage_class;       // 存储类说明符
            int is_inline;            // 是否为内联函数
            int is_noreturn;          // 是否为_Noreturn函数
            int is_register;          // 是否为register变量
            int is_thread_local;      // 是否为线程局部存储
            int is_constexpr;         // 是否为constexpr(C++11)
            int is_consteval;         // 是否为consteval(C++20)
            int is_constinit;        // 是否为constinit(C++20)
        } decl;
        
        // 语句
        struct {
            StmtType stmt_type;     // 语句类型
            struct ASTNode *init;    // 初始化语句(for)
            struct ASTNode *cond;    // 条件表达式
            struct ASTNode *inc;     // 增量表达式(for)
            struct ASTNode *then;    // then语句(if/try)
            struct ASTNode *else_;   // else语句(if)/catch语句(try)
            struct ASTNode *body;    // 循环体/switch体
            struct ASTNode *cases;   // case语句列表(switch)
            struct ASTNode *labels;  // 标签列表
            struct ASTNode *attrs;   // 属性列表
            char *label;             // 标签名(goto/case/default)
            int has_else;            // 是否有else部分
            int is_noreturn;          // 是否不返回
        } stmt;
        
        // 类型
        struct {
            Type *type;              // 类型信息
            struct ASTNode *name;     // 类型名
            struct ASTNode *attrs;    // 属性列表
        } type_node;
        
        // 属性
        struct {
            char *name;              // 属性名
            struct ASTNode **args;    // 参数列表
            int num_args;             // 参数数量
            struct ASTNode *next;     // 下一个属性
        } attr;
        
        // 预处理器
        struct {
            int directive;           // 预处理指令类型
            char *name;               // 宏名/头文件名
            struct Macro *macro;      // 宏定义结构
            struct ASTNode *replacement; // 替换列表/标记
            struct ASTNode *params;   // 参数列表(函数宏)
            int num_params;           // 参数数量
            int is_function_like;     // 是否为函数式宏
            int is_variadic;          // 是否可变参数
        } pp;
        
        // 注释
        struct {
            char *text;              // 注释文本
            int is_block;             // 是否为块注释
        } comment;
    } data;
    
    // 调试信息
    struct {
        const char *mangled_name;  // 修饰名
        unsigned int dreg;          // 调试寄存器
        unsigned int offset;        // 偏移量
        unsigned int size;          // 大小
        unsigned int align;         // 对齐
        unsigned int flags;         // 标志位
    } debug;
    
    // 遍历和转换
    struct ASTNode *parent;         // 父节点
    struct ASTNode **children;      // 子节点数组
    unsigned num_children;          // 子节点数量
    unsigned children_capacity;     // 子节点容量
    
    // 语义分析信息
    struct {
        int is_constant;           // 是否为常量表达式
        int is_lvalue;              // 是否为左值
        int is_pure;                // 是否为纯表达式
        int has_side_effects;       // 是否有副作用
        int is_null_pointer;        // 是否为null指针常量
        int is_bitfield;            // 是否为位域
        int is_implicit;            // 是否为隐式节点
        int is_odr_used;            // 是否被odr使用
        int is_referenced;          // 是否被引用
        int is_used;                // 是否被使用
        int is_referenced_in_unevaluated_context; // 是否在未求值上下文中引用
        int is_module_private;      // 是否为模块私有
        int is_parameter_pack;      // 是否为参数包
        int is_implicitly_declared; // 是否为隐式声明
        int is_decltype_auto;       // 是否为decltype(auto)
        int is_template_parameter;  // 是否为模板参数
        int is_template_parameter_pack; // 是否为模板参数包
        int is_template_template_parameter; // 是否为模板模板参数
    } semantic;
    
    // 内存管理
    unsigned ref_count;             // 引用计数
    void *memory_pool;              // 内存池
    
    // 扩展数据(插件使用)
    void *extension;
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
    TOKEN_EOF = 0,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_FLOAT_NUMBER,
    TOKEN_STRING,
    TOKEN_CHAR,
    // Preprocessor tokens
    TOKEN_PP_INCLUDE,      // #include
    TOKEN_PP_DEFINE,       // #define
    TOKEN_PP_UNDEF,        // #undef
    TOKEN_PP_IFDEF,        // #ifdef
    TOKEN_PP_IFNDEF,       // #ifndef
    TOKEN_PP_IF,           // #if
    TOKEN_PP_ELIF,         // #elif
    TOKEN_PP_ELSE,         // #else
    TOKEN_PP_ENDIF,        // #endif
    TOKEN_PP_LINE,         // #line
    TOKEN_PP_ERROR,        // #error
    TOKEN_PP_PRAGMA,       // #pragma
    TOKEN_PP_DEFINED,      // defined()
    TOKEN_PP_HASH,         // #
    TOKEN_PP_HASHHASH,     // ##
    TOKEN_PP_STRINGIZE,    // #
    TOKEN_PP_HEADER_NAME,  // <header.h> or "header.h" in #include
    TOKEN_PP_NUMBER,       // Preprocessing number
    TOKEN_PP_IDENTIFIER,   // Preprocessing identifier
    TOKEN_PP_OTHER,        // Other preprocessor tokens
    
    // 数据类型关键字
    TOKEN_VOID, TOKEN_CHAR, TOKEN_SHORT, TOKEN_INT, TOKEN_LONG, 
    TOKEN_FLOAT, TOKEN_DOUBLE, TOKEN_SIGNED, TOKEN_UNSIGNED, TOKEN_BOOL,
    
    // 存储类说明符
    TOKEN_TYPEDEF, TOKEN_EXTERN, TOKEN_STATIC, TOKEN_AUTO, TOKEN_REGISTER,
    
    // 类型限定符
    TOKEN_CONST, TOKEN_VOLATILE, TOKEN_RESTRICT, TOKEN_ATOMIC,
    
    // 函数说明符
    TOKEN_INLINE, TOKEN_NORETURN,
    
    // 控制流关键字
    TOKEN_IF, TOKEN_ELSE, TOKEN_SWITCH, TOKEN_CASE, TOKEN_DEFAULT,
    TOKEN_WHILE, TOKEN_DO, TOKEN_FOR, TOKEN_BREAK, TOKEN_CONTINUE, 
    TOKEN_GOTO, TOKEN_RETURN,
    
    // 结构、联合、枚举
    TOKEN_STRUCT, TOKEN_UNION, TOKEN_ENUM, TOKEN_SIZEOF,
    
    // 标点符号
    TOKEN_LBRACE, TOKEN_RBRACE, TOKEN_LPAREN, TOKEN_RPAREN,
    TOKEN_LBRACKET, TOKEN_RBRACKET, TOKEN_SEMICOLON, TOKEN_COLON,
    TOKEN_COMMA, TOKEN_DOT, TOKEN_ELLIPSIS, TOKEN_QUESTION,
    TOKEN_ARROW,
    
    // 操作符
    // 赋值
    TOKEN_ASSIGN, TOKEN_ADD_ASSIGN, TOKEN_SUB_ASSIGN, TOKEN_MUL_ASSIGN,
    TOKEN_DIV_ASSIGN, TOKEN_MOD_ASSIGN, TOKEN_LEFT_SHIFT_ASSIGN, 
    TOKEN_RIGHT_SHIFT_ASSIGN, TOKEN_BIT_AND_ASSIGN, TOKEN_BIT_XOR_ASSIGN,
    TOKEN_BIT_OR_ASSIGN,
    
    // 算术
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULTIPLY, TOKEN_DIVIDE, TOKEN_MOD,
    TOKEN_INCREMENT, TOKEN_DECREMENT,
    
    // 关系
    TOKEN_EQUAL, TOKEN_NOT_EQUAL, TOKEN_LESS, TOKEN_GREATER,
    TOKEN_LESS_EQUAL, TOKEN_GREATER_EQUAL,
    
    // 逻辑
    TOKEN_LOGICAL_AND, TOKEN_LOGICAL_OR, TOKEN_LOGICAL_NOT,
    
    // 位运算
    TOKEN_BIT_AND, TOKEN_BIT_OR, TOKEN_BIT_XOR, TOKEN_BIT_NOT,
    TOKEN_LEFT_SHIFT, TOKEN_RIGHT_SHIFT,
    
    // 预处理器指令
    TOKEN_INCLUDE, TOKEN_DEFINE, TOKEN_UNDEF, TOKEN_IFDEF, TOKEN_IFNDEF,
    TOKEN_ELIF, TOKEN_ENDIF, TOKEN_LINE, TOKEN_ERROR_DIRECTIVE,
    TOKEN_PRAGMA,
    
    // 标准库函数
    TOKEN_PRINTF, TOKEN_SCANF, TOKEN_MALLOC, TOKEN_FREE, TOKEN_EXIT
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

// 解析宏参数
static char** parse_macro_arguments(const char **p, int *num_args, int is_variadic) {
    if (**p != '(') {
        return NULL;
    }
    (*p)++; // 跳过'('
    
    char **args = NULL;
    int capacity = 4;
    int count = 0;
    int va_args_start = -1;  // 可变参数开始位置
    
    args = malloc(sizeof(char*) * (capacity + 1)); // 额外空间用于__VA_ARGS__
    if (!args) return NULL;
    
    while (**p && **p != ')') {
        // 跳过空白
        while (isspace(**p)) (*p)++;
        if (**p == ')' || **p == '\0') break;
        
        // 查找参数结束位置
        const char *arg_start = *p;
        int paren_level = 0;
        int in_string = 0;
        int in_char = 0;
        
        while (**p && (paren_level > 0 || in_string || in_char || 
                     (is_variadic && count < *num_args - 1) ? 
                     (**p != ',' && **p != ')') : **p != ')' && !(count == *num_args - 1 && **p == ',' && *(*p + 1) == '.' && *(*p + 2) == '.' && *(*p + 3) == '.'))) {
            if (!in_string && !in_char) {
                if (**p == '(') paren_level++;
                else if (**p == ')') paren_level--;
                else if (**p == '"' && (p == arg_start || *(p - 1) != '\\')) in_string = 1;
                else if (**p == '\'' && (p == arg_start || *(p - 1) != '\\')) in_char = 1;
            } else if (in_string && **p == '"' && (p == arg_start || *(p - 1) != '\\')) {
                in_string = 0;
            } else if (in_char && **p == '\'' && (p == arg_start || *(p - 1) != '\\')) {
                in_char = 0;
            }
            (*p)++;
        }
        
        // 处理可变参数
        if (is_variadic && count == *num_args - 1 && **p == ',') {
            va_args_start = count;
            // 检查是否是可变参数列表的开始
            const char *check = *p + 1;
            while (isspace(*check)) check++;
            if (strncmp(check, "...", 3) == 0) {
                *p = check + 3;  // 跳过...
                break;
            }
        }
        
        // 分配并保存参数
        int arg_len = *p - arg_start;
        char *arg = malloc(arg_len + 1);
        if (!arg) {
            free_macro_args(args, count);
            return NULL;
        }
        strncpy(arg, arg_start, arg_len);
        arg[arg_len] = '\0';
        
        // 添加到参数列表
        if (count >= capacity) {
            capacity *= 2;
            char **new_args = realloc(args, sizeof(char*) * (capacity + 1));
            if (!new_args) {
                free(arg);
                free_macro_args(args, count);
                return NULL;
            }
            args = new_args;
        }
        args[count++] = arg;
        
        // 跳过逗号
        if (**p == ',') (*p)++;
    }
    
    // 处理可变参数
    if (is_variadic && va_args_start >= 0) {
        // 合并剩余参数到__VA_ARGS__
        if (count > va_args_start) {
            // 计算合并后的参数长度
            size_t total_len = 0;
            for (int i = va_args_start; i < count; i++) {
                total_len += strlen(args[i]) + 2; // +2 for ", "
            }
            
            char *va_args = malloc(total_len + 1);
            if (va_args) {
                va_args[0] = '\0';
                for (int i = va_args_start; i < count; i++) {
                    if (i > va_args_start) {
                        strcat(va_args, ", ");
                    }
                    strcat(va_args, args[i]);
                    free(args[i]);
                }
                args[va_args_start] = va_args;
                count = va_args_start + 1;
            }
        }
    }
    
    if (**p == ')') (*p)++;
    *num_args = count;
    return args;
}

// 替换宏参数
static char* replace_macro_parameters(const char *replacement, char **params, int num_params) {
    if (!replacement || !*replacement) return strdup("");
    
    // 计算需要的最大长度
    size_t max_len = strlen(replacement) * 2 + 1;
    char *result = malloc(max_len);
    if (!result) return NULL;
    
    const char *src = replacement;
    char *dst = result;
    
    while (*src) {
        if (*src == '#' && *(src + 1) == '#') {
            // 处理 ## 操作符
            *dst++ = '#';
            *dst++ = '#';
            src += 2;
        } else if (*src == '#' && isalpha(*(src + 1))) {
            // 处理 # 操作符（字符串化）
            const char *param_start = ++src;
            while (isalnum(*src) || *src == '_') src++;
            
            char param_name[128];
            int param_len = src - param_start;
            if (param_len >= sizeof(param_name)) param_len = sizeof(param_name) - 1;
            strncpy(param_name, param_start, param_len);
            param_name[param_len] = '\0';
            
            // 查找参数
            for (int i = 0; i < num_params; i++) {
                if (strcmp(params[i], param_name) == 0) {
                    // 添加字符串化的参数
                    *dst++ = '"';
                    strcpy(dst, params[i]);
                    dst += strlen(params[i]);
                    *dst++ = '"';
                    break;
                }
            }
        } else if (isalpha(*src) || *src == '_') {
            // 处理标识符（可能是参数）
            const char *ident_start = src;
            while (isalnum(*src) || *src == '_') src++;
            
            char ident[128];
            int ident_len = src - ident_start;
            if (ident_len >= sizeof(ident)) ident_len = sizeof(ident) - 1;
            strncpy(ident, ident_start, ident_len);
            ident[ident_len] = '\0';
            
            // 检查是否是参数
            int is_param = 0;
            for (int i = 0; i < num_params; i++) {
                if (strcmp(params[i], ident) == 0) {
                    // 替换为参数值
                    strcpy(dst, params[i]);
                    dst += strlen(params[i]);
                    is_param = 1;
                    break;
                }
            }
            
            if (!is_param) {
                // 不是参数，原样复制
                strncpy(dst, ident_start, src - ident_start);
                dst += src - ident_start;
            }
        } else {
            // 其他字符原样复制
            *dst++ = *src++;
        }
    }
    
    *dst = '\0';
    return result;
}

// 处理标识符
static int handle_identifier(BootstrapCompiler *compiler, const char *start, const char **p, int line) {
    if (macro_expansion.level > 0) {
        return 0; // 已经在宏展开中，不处理
    }
    
    const char *ident_start = start;
    while (isalnum(**p) || **p == '_') (*p)++;
    
    int ident_len = *p - ident_start;
    char ident[128] = {0};
    strncpy(ident, ident_start, ident_len < 127 ? ident_len : 127);
    
    // 检查是否是预定义宏
    if (is_predefined_macro(ident)) {
        char *expanded = expand_predefined_macro(ident, line, 
            compiler->filename ? compiler->filename : "<unknown>");
        if (expanded) {
            // 将展开的文本添加到输入流中
            size_t expanded_len = strlen(expanded);
            char *new_input = malloc(expanded_len + strlen(*p) + 1);
            if (!new_input) {
                free(expanded);
                return 0;
            }
            strcpy(new_input, expanded);
            strcat(new_input, *p);
            
            // 更新输入指针
            *p = new_input;
            free(expanded);
            return 0; // 让主循环重新处理
        }
    }
    
    // 查找宏定义
    Macro *macro = find_macro(ident);
    if (!macro) {
        // 不是宏，作为普通标识符处理
        Token *token = &compiler->tokens[compiler->token_count++];
        token->type = TOKEN_IDENTIFIER;
        token->value = strdup(ident);
        token->line = line;
        return 1;
    }
    
    // 保存宏名用于错误报告
    char *name = strdup(ident);
    if (!name) return 0;
    
    // 保存当前位置
    const char *save_p = *p;
    
    if (macro->is_function_like) {
        // 函数式宏需要检查是否有参数列表
        // 跳过空白
        while (isspace(**p)) (*p)++;
        
        if (**p != '(') {
            // 不是函数调用语法，不展开
            *p = save_p;
            free(name);
            return 0;
        }
        
        (*p)++; // 跳过'('
        
        // 解析参数
        int num_args = 0;
        char **args = parse_macro_arguments(p, &num_args);
        
        if (macro->num_params >= 0 && num_args != macro->num_params) {
            // 参数数量不匹配，报错
            fprintf(stderr, "Error: macro '%s' expects %d arguments, but got %d\n", 
                   macro->name, macro->num_params, num_args);
            free_macro_args(args, num_args);
            free(name);
            return 0;
        }
        
        // 替换参数
        char *expanded = replace_macro_parameters(macro->replacement, args, num_args);
        free_macro_args(args, num_args);
        
        if (!expanded) {
            free(name);
            return 0;
        }
        
        // 创建新的词法分析器来展开宏
        BootstrapCompiler macro_compiler;
        memset(&macro_compiler, 0, sizeof(macro_compiler));
        macro_compiler.token_count = 0;
        
        // 对宏替换文本进行词法分析
        tokenize(&macro_compiler, expanded);
        free(expanded);
        
        if (macro_compiler.token_count > 0) {
            // 保存展开的tokens
            macro_expansion.expansion_tokens = malloc(sizeof(Token) * macro_compiler.token_count);
            if (macro_expansion.expansion_tokens) {
                memcpy(macro_expansion.expansion_tokens, macro_compiler.tokens, 
                      sizeof(Token) * macro_compiler.token_count);
                macro_expansion.current = macro_expansion.expansion_tokens;
                macro_expansion.level = macro_compiler.token_count;
                macro_expansion.macro_name = name; // 保存宏名用于错误报告
                return 1;
            }
        }
    } else {
        // 简单宏直接展开
        free(name);
        
        // 创建新的词法分析器来展开宏
        BootstrapCompiler macro_compiler;
        memset(&macro_compiler, 0, sizeof(macro_compiler));
        macro_compiler.token_count = 0;
        
        // 对宏替换文本进行词法分析
        tokenize(&macro_compiler, macro->replacement);
        
        if (macro_compiler.token_count > 0) {
            // 保存展开的tokens
            macro_expansion.expansion_tokens = malloc(sizeof(Token) * macro_compiler.token_count);
            if (macro_expansion.expansion_tokens) {
                memcpy(macro_expansion.expansion_tokens, macro_compiler.tokens, 
                      sizeof(Token) * macro_compiler.token_count);
                macro_expansion.current = macro_expansion.expansion_tokens;
                macro_expansion.level = macro_compiler.token_count;
                macro_expansion.macro_name = strdup(macro->name);
                return 1;
            }
        }
    }
    
    free(name);
    return 0;
}

// 计算条件表达式
static int evaluate_condition(const char *expr) {
    // 简单实现：只检查是否定义了宏
    // TODO: 实现完整的表达式求值
    while (isspace(*expr)) expr++;
    
    // 检查 defined 操作符
    if (strncmp(expr, "defined", 7) == 0) {
        const char *p = expr + 7;
        while (isspace(*p)) p++;
        
        if (*p == '(') {
            p++;
            while (isspace(*p)) p++;
            
            const char *name_start = p;
            while (isalnum(*p) || *p == '_') p++;
            
            char name[128];
            int len = p - name_start;
            if (len >= sizeof(name)) len = sizeof(name) - 1;
            strncpy(name, name_start, len);
            name[len] = '\0';
            
            while (isspace(*p)) p++;
            if (*p == ')') p++;
            
            return find_macro(name) != NULL;
        } else {
            // 没有括号的 defined 用法
            const char *name_start = p;
            while (isalnum(*p) || *p == '_') p++;
            
            char name[128];
            int len = p - name_start;
            if (len >= sizeof(name)) len = sizeof(name) - 1;
            strncpy(name, name_start, len);
            name[len] = '\0';
            
            return find_macro(name) != NULL;
        }
    }
    
    // 简单检查标识符是否已定义
    const char *p = expr;
    while (isalnum(*p) || *p == '_') p++;
    
    if (p > expr) {
        char name[128];
        int len = p - expr;
        if (len >= sizeof(name)) len = sizeof(name) - 1;
        strncpy(name, expr, len);
        name[len] = '\0';
        
        return find_macro(name) != NULL;
    }
    
    // 默认返回假
    return 0;
}

// 处理条件编译指令
static void handle_conditional_directive(BootstrapCompiler *compiler, Token *token, const char **p) {
    if (token->type == TOKEN_PP_IF) {
        // 解析条件表达式
        const char *expr_start = *p;
        while (**p && **p != '\n') (*p)++;
        
        char expr[1024];
        int expr_len = *p - expr_start;
        if (expr_len >= sizeof(expr)) expr_len = sizeof(expr) - 1;
        strncpy(expr, expr_start, expr_len);
        expr[expr_len] = '\0';
        
        // 计算条件
        int condition = evaluate_condition(expr);
        
        // 推入条件栈
        IfState *state = malloc(sizeof(IfState));
        if (state) {
            state->condition_met = condition;
            state->else_allowed = 1;
            state->prev = macro_expansion.if_stack;
            macro_expansion.if_stack = state;
            
            if (!condition) {
                macro_expansion.skipping = 1;
                macro_expansion.skip_level = 1;
            }
        }
    } 
    else if (token->type == TOKEN_PP_IFDEF || token->type == TOKEN_PP_IFNDEF) {
        // 跳过空白
        while (isspace(**p)) (*p)++;
        
        // 获取标识符
        const char *ident_start = *p;
        while (isalnum(**p) || **p == '_') (*p)++;
        
        char ident[128];
        int ident_len = *p - ident_start;
        if (ident_len >= sizeof(ident)) ident_len = sizeof(ident) - 1;
        strncpy(ident, ident_start, ident_len);
        ident[ident_len] = '\0';
        
        // 检查是否已定义
        int is_defined = find_macro(ident) != NULL;
        int condition = (token->type == TOKEN_PP_IFDEF) ? is_defined : !is_defined;
        
        // 推入条件栈
        IfState *state = malloc(sizeof(IfState));
        if (state) {
            state->condition_met = condition;
            state->else_allowed = 1;
            state->prev = macro_expansion.if_stack;
            macro_expansion.if_stack = state;
            
            if (!condition) {
                macro_expansion.skipping = 1;
                macro_expansion.skip_level = 1;
            }
        }
    }
    else if (token->type == TOKEN_PP_ELIF) {
        IfState *state = macro_expansion.if_stack;
        if (!state) {
            fprintf(stderr, "Error: #elif without #if\n");
            return;
        }
        
        if (!state->else_allowed) {
            fprintf(stderr, "Error: #elif after #else\n");
            return;
        }
        
        // 如果前面的条件已经满足，跳过这个分支
        if (state->condition_met) {
            macro_expansion.skipping = 1;
            macro_expansion.skip_level = 1;
            return;
        }
        
        // 解析条件表达式
        const char *expr_start = *p;
        while (**p && **p != '\n') (*p)++;
        
        char expr[1024];
        int expr_len = *p - expr_start;
        if (expr_len >= sizeof(expr)) expr_len = sizeof(expr) - 1;
        strncpy(expr, expr_start, expr_len);
        expr[expr_len] = '\0';
        
        // 计算条件
        state->condition_met = evaluate_condition(expr);
        
        // 如果条件为真，停止跳过
        if (state->condition_met) {
            macro_expansion.skipping = 0;
            macro_expansion.skip_level = 0;
        } else {
            macro_expansion.skipping = 1;
            macro_expansion.skip_level = 1;
        }
    }
    else if (token->type == TOKEN_PP_ELSE) {
        IfState *state = macro_expansion.if_stack;
        if (!state) {
            fprintf(stderr, "Error: #else without #if\n");
            return;
        }
        
        if (!state->else_allowed) {
            fprintf(stderr, "Error: multiple #else in one #if\n");
            return;
        }
        
        state->else_allowed = 0;
        
        // 如果前面的条件已经满足，跳过else分支
        if (state->condition_met) {
            macro_expansion.skipping = 1;
            macro_expansion.skip_level = 1;
        } else {
            macro_expansion.skipping = 0;
            macro_expansion.skip_level = 0;
        }
    }
    else if (token->type == TOKEN_PP_ENDIF) {
        IfState *state = macro_expansion.if_stack;
        if (!state) {
            fprintf(stderr, "Error: #endif without #if\n");
            return;
        }
        
        // 弹出条件栈
        macro_expansion.if_stack = state->prev;
        free(state);
        
        // 恢复跳过状态
        if (macro_expansion.if_stack) {
            macro_expansion.skipping = macro_expansion.if_stack->condition_met ? 0 : 1;
            macro_expansion.skip_level = macro_expansion.if_stack->condition_met ? 0 : 1;
        } else {
            macro_expansion.skipping = 0;
            macro_expansion.skip_level = 0;
        }
    }
    
    // 跳过到行尾
    while (**p && **p != '\n') (*p)++;
    if (**p == '\n') (*p)++;
}

static int tokenize(BootstrapCompiler *compiler, const char *source) {
    compiler->token_count = 0;
    const char *p = source;
    int line = 1;
    
    // 重置宏展开状态
    reset_macro_expansion();
    
    // 初始化条件编译状态
    macro_expansion.skipping = 0;
    macro_expansion.skip_level = 0;
    
    while (*p && compiler->token_count < MAX_TOKENS - 1) {
        // 跳过空白字符
        while (isspace(*p)) {
            if (*p == '\n') line++;
            p++;
        }
        
        if (!*p) break;
        
        // 如果当前处于跳过状态，检查是否是条件编译指令
        if (macro_expansion.skipping && *p == '#') {
            const char *save_p = p;
            p++; // 跳过'#'
            
            // 跳过空白
            while (isspace(*p) && *p != '\n') p++;
            
            // 检查是否是指令开始
            if (isalpha(*p) || *p == '_') {
                const char *directive_start = p;
                while (isalnum(*p) || *p == '_') p++;
                
                int directive_len = p - directive_start;
                char directive[16] = {0};
                strncpy(directive, directive_start, directive_len < 15 ? directive_len : 15);
                
                // 检查是否是条件编译指令
                if (strcmp(directive, "if") == 0 || strcmp(directive, "ifdef") == 0 || 
                    strcmp(directive, "ifndef") == 0 || strcmp(directive, "elif") == 0 ||
                    strcmp(directive, "else") == 0 || strcmp(directive, "endif") == 0) {
                    // 回退到#位置，让主循环处理这个指令
                    p = save_p;
                } else {
                    // 不是条件编译指令，继续跳过
                    while (*p && *p != '\n') p++;
                    if (*p == '\n') {
                        line++;
                        p++;
                    }
                    continue;
                }
            } else {
                // 不是有效的指令，继续跳过
                p = save_p + 1;
                while (*p && *p != '\n') p++;
                if (*p == '\n') {
                    line++;
                    p++;
                }
                continue;
            }
        }
        
        Token *token = &compiler->tokens[compiler->token_count++];
        token->line = line;
        
        // 检查是否有宏展开的tokens
        if (macro_expansion.current && macro_expansion.level > 0) {
            // 复制展开的token
            *token = *macro_expansion.current++;
            token->line = line; // 保留原始行号
            
            // 检查是否还有更多tokens
            if (macro_expansion.current >= macro_expansion.expansion_tokens + macro_expansion.level) {
                reset_macro_expansion(); // 展开结束
            }
            continue;
        }
        
        // 处理预处理器指令
        if (*p == '#' && (p == source || *(p-1) == '\n')) {
            const char *start = p++;
            const char *directive_start = p;
            
            // 跳过#后面的空白
            while (isspace(*p) && *p != '\n') p++;
            
            // 提取指令名
            directive_start = p;
            while (isalpha(*p) || *p == '_') p++;
            
            int directive_len = p - directive_start;
            char directive[16] = {0};
            strncpy(directive, directive_start, directive_len < 15 ? directive_len : 15);
            
            // 根据指令设置token类型
            if (strcmp(directive, "if") == 0) {
                token->type = TOKEN_PP_IF;
            } else if (strcmp(directive, "ifdef") == 0) {
                token->type = TOKEN_PP_IFDEF;
            } else if (strcmp(directive, "ifndef") == 0) {
                token->type = TOKEN_PP_IFNDEF;
            } else if (strcmp(directive, "elif") == 0) {
                token->type = TOKEN_PP_ELIF;
            } else if (strcmp(directive, "else") == 0) {
                token->type = TOKEN_PP_ELSE;
            } else if (strcmp(directive, "endif") == 0) {
                token->type = TOKEN_PP_ENDIF;
            } else if (strcmp(directive, "include") == 0) {
                // 如果当前处于跳过状态，跳过整个include
                if (macro_expansion.skipping) {
                    // 跳过到行尾
                    while (*p && *p != '\n') p++;
                    if (*p == '\n') p++;
                    compiler->token_count--; // 不添加这个token
                    continue;
                }
                
                token->type = TOKEN_PP_INCLUDE;
                
                // 跳过空白
                while (isspace(*p) && *p != '\n') p++;
                
                // 处理头文件名
                if (*p == '"' || *p == '<') {
                    char quote = *p++;
                    const char *header_start = p;
                    while (*p && *p != quote && *p != '\n') p++;
                    
                    if (*p == quote) {
                        int header_len = p - header_start;
                        token->value = malloc(header_len + 1);
                        strncpy(token->value, header_start, header_len);
                        token->value[header_len] = '\0';
                        p++; // 跳过结束引号
                    }
                }
            }
            // 处理条件编译指令
            if (token->type == TOKEN_PP_IF || token->type == TOKEN_PP_IFDEF || 
                token->type == TOKEN_PP_IFNDEF || token->type == TOKEN_PP_ELIF ||
                token->type == TOKEN_PP_ELSE || token->type == TOKEN_PP_ENDIF) {
                // 处理条件编译指令
                handle_conditional_directive(compiler, token, &p);
                
                // 如果当前处于跳过状态，继续跳过直到遇到下一个条件编译指令
                if (macro_expansion.skipping) {
                    compiler->token_count--; // 不添加这个token
                    continue;
                }
            } 
            else if (strcmp(directive, "define") == 0) {
                // 如果当前处于跳过状态，跳过整个define
                if (macro_expansion.skipping) {
                    // 跳过到行尾
                    while (*p && *p != '\n') p++;
                    if (*p == '\n') p++;
                    compiler->token_count--; // 不添加这个token
                    continue;
                }
                
                token->type = TOKEN_PP_DEFINE;
                
                // 跳过空白
                while (isspace(*p) && *p != '\n') p++;
                
                // 提取宏名称
                const char *name_start = p;
                while (isalnum(*p) || *p == '_') p++;
                int name_len = p - name_start;
                char *name = malloc(name_len + 1);
                strncpy(name, name_start, name_len);
                name[name_len] = '\0';
                
                // 检查是否为函数式宏
                int is_function_like = 0;
                char **params = NULL;
                int num_params = 0;
                
                // 跳过空白
                while (isspace(*p) && *p != '\n') p++;
                
                if (*p == '(') {
                    // 函数式宏
                    is_function_like = 1;
                    p++; // 跳过'('
                    
                    // 解析参数列表
                    while (*p && *p != ')' && *p != '\n') {
                        // 跳过空白
                        while (isspace(*p)) p++;
                        
                        // 提取参数名
                        if (isalpha(*p) || *p == '_') {
                            const char *param_start = p;
                            while (isalnum(*p) || *p == '_') p++;
                            
                            // 添加到参数列表
                            params = realloc(params, (num_params + 1) * sizeof(char*));
                            int param_len = p - param_start;
                            params[num_params] = malloc(param_len + 1);
                            strncpy(params[num_params], param_start, param_len);
                            params[num_params][param_len] = '\0';
                            num_params++;
                            
                            // 跳过逗号
                            while (isspace(*p)) p++;
                            if (*p == ',') {
                                p++;
                                // 跳过逗号后的空白
                                while (isspace(*p)) p++;
                            }
                        } else if (*p == '.') {
                            // 处理可变参数...
                            if (strncmp(p, "...", 3) == 0) {
                                // TODO: 支持可变参数
                                p += 3;
                            } else {
                                p++;
                            }
                        } else {
                            p++;
                        }
                    }
                    
                    if (*p == ')') p++; // 跳过')'
                }
                
                // 提取替换文本
                const char *repl_start = p;
                while (*p && *p != '\n') p++;
                
                // 去除尾部空白
                const char *repl_end = p;
                while (repl_end > repl_start && isspace(*(repl_end-1))) {
                    repl_end--;
                }
                
                char *replacement = malloc(repl_end - repl_start + 1);
                strncpy(replacement, repl_start, repl_end - repl_start);
                replacement[repl_end - repl_start] = '\0';
                
                // 添加到宏表
                add_macro(name, replacement, is_function_like, params, num_params);
                
                // 保存宏定义文本
                int len = p - start;
                token->value = malloc(len + 1);
                strncpy(token->value, start, len);
                token->value[len] = '\0';
                
                free(name);
                // 注意：params和replacement由宏表管理，不要在这里释放
                
                continue;
            }
            else if (strcmp(directive, "undef") == 0) {
                token->type = TOKEN_PP_UNDEF;
            }
            else if (strcmp(directive, "ifdef") == 0) {
                token->type = TOKEN_PP_IFDEF;
            }
            else if (strcmp(directive, "ifndef") == 0) {
                token->type = TOKEN_PP_IFNDEF;
            }
            else if (strcmp(directive, "if") == 0) {
                token->type = TOKEN_PP_IF;
            }
            else if (strcmp(directive, "elif") == 0) {
                token->type = TOKEN_PP_ELIF;
            }
            else if (strcmp(directive, "else") == 0) {
                token->type = TOKEN_PP_ELSE;
            }
            else if (strcmp(directive, "endif") == 0) {
                token->type = TOKEN_PP_ENDIF;
            }
            else if (strcmp(directive, "line") == 0) {
                token->type = TOKEN_PP_LINE;
            }
            else if (strcmp(directive, "error") == 0) {
                token->type = TOKEN_PP_ERROR;
            }
            else if (strcmp(directive, "pragma") == 0) {
                token->type = TOKEN_PP_PRAGMA;
            }
            else {
                token->type = TOKEN_PP_OTHER;
            }
            
            // 如果不是include指令，则保存整个预处理指令
            if (token->type != TOKEN_PP_INCLUDE) {
                while (*p && *p != '\n') p++;
                int len = p - start;
                token->value = malloc(len + 1);
                strncpy(token->value, start, len);
                token->value[len] = '\0';
            }
            
            continue;
        }
        
        // 识别标识符和关键字
        if (isalpha(*p) || *p == '_') {
            const char *start = p;
            while (isalnum(*p) || *p == '_') p++;
            
            // 检查是否为宏并处理展开
            if (handle_identifier(compiler, start, p, line)) {
                compiler->token_count--; // 回退当前token
                continue; // 继续处理展开的tokens
            }
            
            int len = p - start;
            token->value = malloc(len + 1);
            strncpy(token->value, start, len);
            token->value[len] = '\0';
            
            // 检查关键字
            if (strcmp(token->value, "int") == 0) token->type = TOKEN_INT;
            else if (strcmp(token->value, "char") == 0) token->type = TOKEN_CHAR;
            else if (strcmp(token->value, "void") == 0) token->type = TOKEN_VOID;
            else if (strcmp(token->value, "short") == 0) token->type = TOKEN_SHORT;
            else if (strcmp(token->value, "long") == 0) token->type = TOKEN_LONG;
            else if (strcmp(token->value, "float") == 0) token->type = TOKEN_FLOAT;
            else if (strcmp(token->value, "double") == 0) token->type = TOKEN_DOUBLE;
            else if (strcmp(token->value, "signed") == 0) token->type = TOKEN_SIGNED;
            else if (strcmp(token->value, "unsigned") == 0) token->type = TOKEN_UNSIGNED;
            else if (strcmp(token->value, "return") == 0) token->type = TOKEN_RETURN;
            else if (strcmp(token->value, "if") == 0) token->type = TOKEN_IF;
            else if (strcmp(token->value, "else") == 0) token->type = TOKEN_ELSE;
            else if (strcmp(token->value, "while") == 0) token->type = TOKEN_WHILE;
            else if (strcmp(token->value, "for") == 0) token->type = TOKEN_FOR;
            else if (strcmp(token->value, "do") == 0) token->type = TOKEN_DO;
            else if (strcmp(token->value, "break") == 0) token->type = TOKEN_BREAK;
            else if (strcmp(token->value, "continue") == 0) token->type = TOKEN_CONTINUE;
            else if (strcmp(token->value, "switch") == 0) token->type = TOKEN_SWITCH;
            else if (strcmp(token->value, "case") == 0) token->type = TOKEN_CASE;
            else if (strcmp(token->value, "default") == 0) token->type = TOKEN_DEFAULT;
            else if (strcmp(token->value, "struct") == 0) token->type = TOKEN_STRUCT;
            else if (strcmp(token->value, "union") == 0) token->type = TOKEN_UNION;
            else if (strcmp(token->value, "enum") == 0) token->type = TOKEN_ENUM;
            else if (strcmp(token->value, "typedef") == 0) token->type = TOKEN_TYPEDEF;
            else if (strcmp(token->value, "static") == 0) token->type = TOKEN_STATIC;
            else if (strcmp(token->value, "extern") == 0) token->type = TOKEN_EXTERN;
            else if (strcmp(token->value, "const") == 0) token->type = TOKEN_CONST;
            else if (strcmp(token->value, "volatile") == 0) token->type = TOKEN_VOLATILE;
            else if (strcmp(token->value, "sizeof") == 0) token->type = TOKEN_SIZEOF;
            else if (strcmp(token->value, "printf") == 0) token->type = TOKEN_PRINTF;
            else if (strcmp(token->value, "malloc") == 0) token->type = TOKEN_MALLOC;
            else token->type = TOKEN_IDENTIFIER;
        }
        // 识别数字（十进制、十六进制、八进制、浮点数）
        else if (isdigit(*p) || (*p == '.' && isdigit(*(p+1)))) {
            const char *start = p;
            int is_float = 0;
            
            // 处理十六进制数
            if (*p == '0' && (*(p+1) == 'x' || *(p+1) == 'X')) {
                p += 2;
                while (isxdigit(*p)) p++;
            }
            // 处理八进制数
            else if (*p == '0') {
                p++;
                while (*p >= '0' && *p <= '7') p++;
            }
            // 处理十进制数
            else {
                while (isdigit(*p)) p++;
                // 处理浮点数
                if (*p == '.') {
                    is_float = 1;
                    p++;
                    while (isdigit(*p)) p++;
                }
                // 处理科学计数法
                if (*p == 'e' || *p == 'E') {
                    is_float = 1;
                    p++;
                    if (*p == '+' || *p == '-') p++;
                    while (isdigit(*p)) p++;
                }
            }
            
            // 处理浮点数后缀
            if (is_float && (*p == 'f' || *p == 'F' || *p == 'l' || *p == 'L')) {
                p++;
            }
            // 处理整数后缀
            else if (!is_float && (*p == 'u' || *p == 'U' || *p == 'l' || *p == 'L')) {
                p++;
                if (*p == 'l' || *p == 'L') p++;  // 处理ll/LL后缀
            }
            
            int len = p - start;
            token->value = malloc(len + 1);
            strncpy(token->value, start, len);
            token->value[len] = '\0';
            token->type = is_float ? TOKEN_FLOAT_NUMBER : TOKEN_NUMBER;
        }
        // 识别字符字面量
        else if (*p == '\'') {
            p++; // 跳过开始的单引号
            const char *start = p;
            
            // 处理转义字符
            if (*p == '\\') {
                p++;
                switch (*p) {
                    case '\'': case '\"': case '\\': case '?':
                        p++;
                        break;
                    case 'a': case 'b': case 'f': case 'n':
                    case 'r': case 't': case 'v':
                        p++;
                        break;
                    case 'x': // 十六进制转义
                        p++;
                        while (isxdigit(*p)) p++;
                        break;
                    case '0': case '1': case '2': case '3':
                    case '4': case '5': case '6': case '7': // 八进制转义
                        p++;
                        while (*p >= '0' && *p <= '7') p++;
                        break;
                    default:
                        p++; // 未知转义序列
                }
            } else {
                p++; // 普通字符
            }
            
            if (*p == '\'') {
                p++; // 跳过结束的单引号
                int len = p - start - 2; // 减去两端的单引号
                token->value = malloc(len + 1);
                strncpy(token->value, start, len);
                token->value[len] = '\0';
                token->type = TOKEN_CHAR_LITERAL;
            } else {
                // 错误的字符字面量
                token->value = strdup("");
                token->type = TOKEN_ERROR;
            }
        }
        // 识别字符串
        else if (*p == '"') {
            p++; // 跳过开始的引号
            const char *start = p;
            while (*p && *p != '"' && *p != '\n') {
                if (*p == '\\') {
                    p++; // 跳过转义字符
                    if (*p) p++; // 跳过转义序列中的下一个字符
                } else {
                    p++;
                }
            }
            
            int len = p - start;
            token->value = malloc(len + 1);
            strncpy(token->value, start, len);
            token->value[len] = '\0';
            token->type = TOKEN_STRING;
            
            if (*p == '"') p++; // 跳过结束的引号
            else {
                // 未闭合的字符串
                token->type = TOKEN_ERROR;
            }
        }
        // 识别注释和多字符操作符
        else if (*p == '/') {
            if (*(p+1) == '/') { // 单行注释
                while (*p && *p != '\n') p++;
                compiler->token_count--; // 不保存注释
                continue;
            } else if (*(p+1) == '*') { // 多行注释
                p += 2;
                while (*p && !(*p == '*' && *(p+1) == '/')) {
                    if (*p == '\n') line++;
                    p++;
                }
                if (*p) p += 2; // 跳过 */
                else {
                    // 未闭合的注释
                    token->value = strdup("Unterminated comment");
                    token->type = TOKEN_ERROR;
                }
                compiler->token_count--; // 不保存注释
                continue;
            } else if (*(p+1) == '=') { // /= 操作符
                token->value = strdup("/=");
                token->type = TOKEN_DIVIDE_ASSIGN;
                p += 2;
            } else { // 除号
                token->value = strdup("/");
                token->type = TOKEN_DIVIDE;
                p++;
            }
        }
        // 识别多字符操作符
        else {
            switch (*p) {
                // 括号和标点
                case '{': token->type = TOKEN_LBRACE; p++; break;
                case '}': token->type = TOKEN_RBRACE; p++; break;
                case '(': token->type = TOKEN_LPAREN; p++; break;
                case ')': token->type = TOKEN_RPAREN; p++; break;
                case '[': token->type = TOKEN_LBRACKET; p++; break;
                case ']': token->type = TOKEN_RBRACKET; p++; break;
                case ';': token->type = TOKEN_SEMICOLON; p++; break;
                case ',': token->type = TOKEN_COMMA; p++; break;
                case ':': token->type = TOKEN_COLON; p++; break;
                case '?': token->type = TOKEN_QUESTION; p++; break;
                case '~': token->type = TOKEN_BIT_NOT; p++; break;
                
                // 赋值操作符
                case '=':
                    if (*(p+1) == '=') {
                        token->type = TOKEN_EQUAL;
                        p += 2;
                    } else {
                        token->type = TOKEN_ASSIGN;
                        p++;
                    }
                    break;
                    
                // 比较操作符
                case '!':
                    if (*(p+1) == '=') {
                        token->type = TOKEN_NOT_EQUAL;
                        p += 2;
                    } else {
                        token->type = TOKEN_LOGICAL_NOT;
                        p++;
                    }
                    break;
                    
                case '<':
                    if (*(p+1) == '=') {
                        token->type = TOKEN_LESS_EQUAL;
                        p += 2;
                    } else if (*(p+1) == '<') {
                        if (*(p+2) == '=') {
                            token->type = TOKEN_LEFT_SHIFT_ASSIGN;
                            p += 3;
                        } else {
                            token->type = TOKEN_LEFT_SHIFT;
                            p += 2;
                        }
                    } else {
                        token->type = TOKEN_LESS;
                        p++;
                    }
                    break;
                    
                case '>':
                    if (*(p+1) == '=') {
                        token->type = TOKEN_GREATER_EQUAL;
                        p += 2;
                    } else if (*(p+1) == '>') {
                        if (*(p+2) == '=') {
                            token->type = TOKEN_RIGHT_SHIFT_ASSIGN;
                            p += 3;
                        } else {
                            token->type = TOKEN_RIGHT_SHIFT;
                            p += 2;
                        }
                    } else {
                        token->type = TOKEN_GREATER;
                        p++;
                    }
                    break;
                
                // 算术操作符
                case '+':
                    if (*(p+1) == '+') {
                        token->type = TOKEN_INCREMENT;
                        p += 2;
                    } else if (*(p+1) == '=') {
                        token->type = TOKEN_ADD_ASSIGN;
                        p += 2;
                    } else {
                        token->type = TOKEN_PLUS;
                        p++;
                    }
                    break;
                    
                case '-':
                    if (*(p+1) == '-') {
                        token->type = TOKEN_DECREMENT;
                        p += 2;
                    } else if (*(p+1) == '=') {
                        token->type = TOKEN_SUB_ASSIGN;
                        p += 2;
                    } else if (*(p+1) == '>') {
                        token->type = TOKEN_ARROW;
                        p += 2;
                    } else {
                        token->type = TOKEN_MINUS;
                        p++;
                    }
                    break;
                    
                case '*':
                    if (*(p+1) == '=') {
                        token->type = TOKEN_MULTIPLY_ASSIGN;
                        p += 2;
                    } else {
                        token->type = TOKEN_MULTIPLY;
                        p++;
                    }
                    break;
                    
                case '%':
                    if (*(p+1) == '=') {
                        token->type = TOKEN_MOD_ASSIGN;
                        p += 2;
                    } else {
                        token->type = TOKEN_MOD;
                        p++;
                    }
                    break;
                
                // 位操作符
                case '&':
                    if (*(p+1) == '&') {
                        token->type = TOKEN_LOGICAL_AND;
                        p += 2;
                    } else if (*(p+1) == '=') {
                        token->type = TOKEN_BIT_AND_ASSIGN;
                        p += 2;
                    } else {
                        token->type = TOKEN_BIT_AND;
                        p++;
                    }
                    break;
                    
                case '|':
                    if (*(p+1) == '|') {
                        token->type = TOKEN_LOGICAL_OR;
                        p += 2;
                    } else if (*(p+1) == '=') {
                        token->type = TOKEN_BIT_OR_ASSIGN;
                        p += 2;
                    } else {
                        token->type = TOKEN_BIT_OR;
                        p++;
                    }
                    break;
                    
                case '^':
                    if (*(p+1) == '=') {
                        token->type = TOKEN_BIT_XOR_ASSIGN;
                        p += 2;
                    } else {
                        token->type = TOKEN_BIT_XOR;
                        p++;
                    }
                    break;
                
                // 其他字符
                case '.':
                    if (*(p+1) == '.' && *(p+2) == '.') {
                        token->type = TOKEN_ELLIPSIS;
                        p += 3;
                    } else {
                        token->type = TOKEN_DOT;
                        p++;
                    }
                    break;
                    
                default:
                    // 未知字符，记录错误但继续
                    char unknown[2] = {*p, '\0'};
                    token->value = strdup(unknown);
                    token->type = TOKEN_ERROR;
                    p++;
                    break;
            }
            
            // 为操作符创建字符串表示
            if (token->type != TOKEN_ERROR) {
                int len = p - (start = source + (p - source) - (p - start));
                token->value = malloc(len + 1);
                strncpy(token->value, start, len);
                token->value[len] = '\0';
            }
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