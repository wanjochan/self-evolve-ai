#ifndef ASTC_H
#define ASTC_H

#include <stdint.h>
#include <stdbool.h>

/**
以 WASM为蓝图，加入c99元素，设计ASTC数据结构
以后考虑加入更多，比如兼容LLVM IR
不要随便修改ASTC的定义，除非有非常充分的理由！s
 */

// ===== 模块系统节点类型 =====
// ASTC_MODULE_DECL,           // 模块声明
// ASTC_EXPORT_DECL,           // 导出声明
// ASTC_IMPORT_DECL,           // 导入声明
// ASTC_REQUIRES_DECL,         // 依赖声明
// ASTC_MODULE_ATTRIBUTE,      // 模块属性
// ASTC_SYMBOL_REF,            // 符号引用

//IMPORTANT
typedef enum {
    // ===== 必须标准 WebAssembly 节点 =====
    // 模块结构 (参考: https://webassembly.github.io/spec/core/binary/modules.html)
    AST_MODULE = 0x00,              // 模块
    AST_FUNC_TYPE = 0x60,            // 函数类型
    AST_IMPORT = 0x02,               // 导入
    AST_FUNC = 0x00,                 // 函数
    AST_TABLE = 0x01,                // 表
    AST_MEMORY = 0x02,               // 内存
    AST_GLOBAL = 0x03,               // 全局变量
    AST_EXPORT = 0x07,               // 导出
    AST_START = 0x08,                // 开始函数
    AST_ELEM = 0x09,                 // 元素段
    AST_DATA = 0x0B,                 // 数据段
    
    // 控制流 (参考: https://webassembly.github.io/spec/core/binary/instructions.html#control-instructions)
    AST_UNREACHABLE = 0x00,         // 不可达
    AST_NOP = 0x01,                  // 空操作
    AST_BLOCK = 0x02,                // 块
    AST_LOOP = 0x03,                 // 循环
    AST_IF = 0x04,                   // 条件
    AST_ELSE = 0x05,                 // 否则
    AST_END = 0x0B,                  // 结束
    AST_BR = 0x0C,                   // 分支
    AST_BR_IF = 0x0D,                // 条件分支
    AST_BR_TABLE = 0x0E,             // 分支表
    AST_RETURN = 0x0F,               // 返回
    AST_CALL = 0x10,                 // 调用
    AST_CALL_INDIRECT = 0x11,         // 间接调用
    
    // 内存操作 (参考: https://webassembly.github.io/spec/core/binary/instructions.html#memory-instructions)
    AST_DROP = 0x1A,                 // 丢弃栈顶值
    AST_SELECT = 0x1B,                // 选择
    
    // 变量指令 (参考: https://webassembly.github.io/spec/core/binary/instructions.html#variable-instructions)
    AST_LOCAL_GET = 0x20,            // 获取局部变量
    AST_LOCAL_SET = 0x21,             // 设置局部变量
    AST_LOCAL_TEE = 0x22,             // 设置并保留局部变量
    AST_GLOBAL_GET = 0x23,            // 获取全局变量
    AST_GLOBAL_SET = 0x24,            // 设置全局变量
    
    // 内存指令 (参考: https://webassembly.github.io/spec/core/binary/instructions.html#memory-instructions)
    AST_I32_LOAD = 0x28,              // i32 加载
    AST_I64_LOAD = 0x29,              // i64 加载
    AST_F32_LOAD = 0x2A,              // f32 加载
    AST_F64_LOAD = 0x2B,              // f64 加载
    AST_I32_LOAD8_S = 0x2C,           // i32 加载8位有符号
    AST_I32_LOAD8_U = 0x2D,           // i32 加载8位无符号
    AST_I32_LOAD16_S = 0x2E,          // i32 加载16位有符号
    AST_I32_LOAD16_U = 0x2F,          // i32 加载16位无符号
    AST_I64_LOAD8_S = 0x30,           // i64 加载8位有符号
    AST_I64_LOAD8_U = 0x31,           // i64 加载8位无符号
    AST_I64_LOAD16_S = 0x32,          // i64 加载16位有符号
    AST_I64_LOAD16_U = 0x33,          // i64 加载16位无符号
    AST_I64_LOAD32_S = 0x34,          // i64 加载32位有符号
    AST_I64_LOAD32_U = 0x35,          // i64 加载32位无符号
    AST_I32_STORE = 0x36,             // i32 存储
    AST_I64_STORE = 0x37,             // i64 存储
    AST_F32_STORE = 0x38,             // f32 存储
    AST_F64_STORE = 0x39,             // f64 存储
    AST_I32_STORE8 = 0x3A,            // i32 存储8位
    AST_I32_STORE16 = 0x3B,           // i32 存储16位
    AST_I64_STORE8 = 0x3C,            // i64 存储8位
    AST_I64_STORE16 = 0x3D,           // i64 存储16位
    AST_I64_STORE32 = 0x3E,           // i64 存储32位
    AST_MEMORY_SIZE = 0x3F,           // 内存大小
    AST_MEMORY_GROW = 0x40,           // 内存增长
    
    // 常量 (参考: https://webassembly.github.io/spec/core/binary/instructions.html#constant-instructions)
    AST_I32_CONST = 0x41,            // i32 常量
    AST_I64_CONST = 0x42,            // i64 常量
    AST_F32_CONST = 0x43,            // f32 常量
    AST_F64_CONST = 0x44,            // f64 常量
    
    // 数值运算 (参考: https://webassembly.github.io/spec/core/binary/instructions.html#numeric-instructions)
    
    // i32 运算
    AST_I32_EQZ = 0x45,         // i32 等于零
    AST_I32_EQ = 0x46,          // i32 等于
    AST_I32_NE = 0x47,          // i32 不等于
    AST_I32_LT_S = 0x48,        // i32 有符号小于
    AST_I32_LT_U = 0x49,        // i32 无符号小于
    AST_I32_GT_S = 0x4A,        // i32 有符号大于
    AST_I32_GT_U = 0x4B,        // i32 无符号大于
    AST_I32_LE_S = 0x4C,        // i32 有符号小于等于
    AST_I32_LE_U = 0x4D,        // i32 无符号小于等于
    AST_I32_GE_S = 0x4E,        // i32 有符号大于等于
    AST_I32_GE_U = 0x4F,        // i32 无符号大于等于
    
    // i64 运算
    AST_I64_EQZ = 0x50,         // i64 等于零
    AST_I64_EQ = 0x51,          // i64 等于
    AST_I64_NE = 0x52,          // i64 不等于
    AST_I64_LT_S = 0x53,        // i64 有符号小于
    AST_I64_LT_U = 0x54,        // i64 无符号小于
    AST_I64_GT_S = 0x55,        // i64 有符号大于
    AST_I64_GT_U = 0x56,        // i64 无符号大于
    AST_I64_LE_S = 0x57,        // i64 有符号小于等于
    AST_I64_LE_U = 0x58,        // i64 无符号小于等于
    AST_I64_GE_S = 0x59,        // i64 有符号大于等于
    AST_I64_GE_U = 0x5A,        // i64 无符号大于等于
    
    // f32 运算
    AST_F32_EQ = 0x5B,          // f32 等于
    AST_F32_NE = 0x5C,          // f32 不等于
    AST_F32_LT = 0x5D,          // f32 小于
    AST_F32_GT = 0x5E,          // f32 大于
    AST_F32_LE = 0x5F,          // f32 小于等于
    AST_F32_GE = 0x60,          // f32 大于等于
    
    // f64 运算
    AST_F64_EQ = 0x61,          // f64 等于
    AST_F64_NE = 0x62,          // f64 不等于
    AST_F64_LT = 0x63,          // f64 小于
    AST_F64_GT = 0x64,          // f64 大于
    AST_F64_LE = 0x65,          // f64 小于等于
    AST_F64_GE = 0x66,          // f64 大于等于
    
    // 数值运算
    AST_I32_CLZ = 0x67,         // i32 前导零计数
    AST_I32_CTZ = 0x68,         // i32 尾随零计数
    AST_I32_POPCNT = 0x69,      // i32 置1位计数
    AST_I32_ADD = 0x6A,         // i32 加法
    AST_I32_SUB = 0x6B,         // i32 减法
    AST_I32_MUL = 0x6C,         // i32 乘法
    AST_I32_DIV_S = 0x6D,       // i32 有符号除法
    AST_I32_DIV_U = 0x6E,       // i32 无符号除法
    AST_I32_REM_S = 0x6F,       // i32 有符号取余
    AST_I32_REM_U = 0x70,       // i32 无符号取余
    AST_I32_AND = 0x71,         // i32 按位与
    AST_I32_OR = 0x72,          // i32 按位或
    AST_I32_XOR = 0x73,         // i32 按位异或
    AST_I32_SHL = 0x74,         // i32 左移
    AST_I32_SHR_S = 0x75,       // i32 算术右移
    AST_I32_SHR_U = 0x76,       // i32 逻辑右移
    AST_I32_ROTL = 0x77,        // i32 循环左移
    AST_I32_ROTR = 0x78,        // i32 循环右移
    
    // 类型转换 (参考: https://webassembly.github.io/spec/core/binary/instructions.html#binary-cvtop)
    AST_I32_WRAP_I64 = 0xA7,    // i64 截断为 i32
    AST_I32_TRUNC_F32_S = 0xA8,  // f32 截断为有符号 i32
    AST_I32_TRUNC_F32_U = 0xA9,  // f32 截断为无符号 i32
    AST_I32_TRUNC_F64_S = 0xAA,  // f64 截断为有符号 i32
    AST_I32_TRUNC_F64_U = 0xAB,  // f64 截断为无符号 i32
    
    // 其他指令
    AST_REF_NULL = 0xD0,         // 空引用
    AST_REF_IS_NULL = 0xD1,      // 检查引用是否为空
    AST_REF_FUNC = 0xD2,         // 函数引用
    
    // 内存指令 (参考: https://webassembly.github.io/spec/core/binary/instructions.html#memory-instructions)
    AST_MEMORY_INIT = 0xFC08,    // 内存初始化
    AST_DATA_DROP = 0xFC09,      // 丢弃数据段
    AST_MEMORY_COPY = 0xFC0A,    // 内存复制
    AST_MEMORY_FILL = 0xFC0B,    // 内存填充
    AST_TABLE_INIT = 0xFC0C,     // 表初始化
    AST_ELEM_DROP = 0xFC0D,      // 丢弃元素段
    AST_TABLE_COPY = 0xFC0E,     // 表复制
    AST_TABLE_GROW = 0xFC0F,     // 表增长
    AST_TABLE_SIZE = 0xFC10,     // 表大小
    AST_TABLE_FILL = 0xFC11,     // 表填充
    
    // ===== 扩展节点 (AST-C) =====
    // C99编译器专用指令
    ASTC_C99_COMPILE = 0xF000,      // C99编译指令
    ASTC_C99_PARSE = 0xF001,        // C99解析指令
    ASTC_C99_CODEGEN = 0xF002,      // C99代码生成指令
    ASTC_C99_OPTIMIZE = 0xF003,     // C99优化指令
    ASTC_C99_LINK = 0xF004,         // C99链接指令

    // 声明和定义
    ASTC_TRANSLATION_UNIT,  // 翻译单元
    ASTC_FUNC_DECL,         // 函数声明
    ASTC_VAR_DECL,          // 变量声明
    ASTC_PARAM_DECL,        // 参数声明
    ASTC_TYPE_SPECIFIER,    // 类型说明符
    
    // 语句
    ASTC_COMPOUND_STMT,     // 复合语句
    ASTC_IF_STMT,           // if语句
    ASTC_WHILE_STMT,        // while语句
    ASTC_FOR_STMT,          // for语句
    ASTC_RETURN_STMT,       // return语句
    ASTC_BREAK_STMT,        // break语句
    ASTC_CONTINUE_STMT,     // continue语句
    ASTC_EXPR_STMT,         // 表达式语句
    
    // 表达式
    ASTC_EXPR_IDENTIFIER,   // 标识符表达式
    ASTC_EXPR_CONSTANT,     // 常量表达式
    ASTC_EXPR_STRING_LITERAL, // 字符串字面量
    ASTC_UNARY_OP,          // 一元操作
    ASTC_BINARY_OP,         // 二元操作
    ASTC_CALL_EXPR,         // 函数调用
    
    // 操作符
    ASTC_OP_UNKNOWN,        // 未知操作符
    ASTC_OP_ADD,            // 加法 +
    ASTC_OP_SUB,            // 减法 -
    ASTC_OP_MUL,            // 乘法 *
    ASTC_OP_DIV,            // 除法 /
    ASTC_OP_MOD,            // 取模 %
    ASTC_OP_EQ,             // 等于 ==
    ASTC_OP_NE,             // 不等于 !=
    ASTC_OP_LT,             // 小于 <
    ASTC_OP_LE,             // 小于等于 <=
    ASTC_OP_GT,             // 大于 >
    ASTC_OP_GE,             // 大于等于 >=
    ASTC_OP_AND,            // 按位与 &
    ASTC_OP_OR,             // 按位或 |
    ASTC_OP_XOR,            // 按位异或 ^
    ASTC_OP_NOT,            // 逻辑非 !
    ASTC_OP_BITWISE_NOT,    // 按位取反 ~
    ASTC_OP_LOGICAL_AND,    // 逻辑与 &&
    ASTC_OP_LOGICAL_OR,     // 逻辑或 ||
    ASTC_OP_ASSIGN,         // 赋值 =
    ASTC_OP_NEG,            // 负号 -
    ASTC_OP_POS,            // 正号 +
    ASTC_OP_DEREF,          // 解引用 *
    ASTC_OP_ADDR,           // 取地址 &
    
    // 复合类型
    ASTC_STRUCT_DECL,      // 结构体声明
    ASTC_UNION_DECL,       // 联合体声明
    ASTC_ENUM_DECL,        // 枚举声明
    ASTC_ENUM_CONSTANT,    // 枚举常量
    ASTC_TYPEDEF_DECL,     // 类型定义
    
    // 类型节点
    ASTC_PRIMITIVE_TYPE,   // 基本类型
    ASTC_POINTER_TYPE,     // 指针类型
    ASTC_ARRAY_TYPE,       // 数组类型
    ASTC_FUNCTION_TYPE,    // 函数类型
    
    // 控制流
    ASTC_CASE_STMT,        // case 语句
    ASTC_DEFAULT_STMT,     // default 语句
    ASTC_GOTO_STMT,        // goto 语句
    ASTC_LABEL_STMT,       // 标签语句
    ASTC_SWITCH_STMT,      // switch 语句
    
    // 表达式类型
    ASTC_EXPR_COMPOUND_LITERAL,  // 复合字面量 (C99)
    ASTC_EXPR_FUNC_CALL,         // 函数调用
    ASTC_EXPR_ARRAY_SUBSCRIPT,   // 数组下标
    ASTC_EXPR_MEMBER_ACCESS,     // 成员访问
    ASTC_EXPR_PTR_MEMBER_ACCESS, // 指针成员访问
    ASTC_EXPR_CAST_EXPR,        // 类型转换
    
    // 语句类型
    ASTC_STMT_NONE,
    ASTC_STMT_DECL,              // 声明语句
    ASTC_STMT_NULL,              // 空语句
    ASTC_STMT_COMPOUND,          // 复合语句
    ASTC_STMT_CASE,              // case 语句
    ASTC_STMT_DEFAULT,           // default 语句
    ASTC_STMT_LABEL,             // 标签语句
    ASTC_STMT_ATTRIBUTED,        // 带属性的语句
    ASTC_STMT_IF,                // if 语句
    ASTC_STMT_SWITCH,            // switch 语句
    ASTC_STMT_WHILE,             // while 循环
    ASTC_STMT_DO,                // do-while 循环
    ASTC_STMT_FOR,               // for 循环
    ASTC_STMT_GOTO,              // goto 语句
    ASTC_STMT_INDIRECT_GOTO,     // 间接 goto 语句
    ASTC_STMT_CONTINUE,          // continue 语句
    ASTC_STMT_BREAK,             // break 语句
    ASTC_STMT_RETURN,            // return 语句
    ASTC_STMT_ASM,               // 内联汇编
    
    // ===== C 语言类型 =====
    // 基本类型
    ASTC_TYPE_INVALID,            // 无效类型
    ASTC_TYPE_VOID,               // void
    ASTC_TYPE_SIGNED,             // signed
    ASTC_TYPE_UNSIGNED,           // unsigned
    ASTC_TYPE_INT,                // int
    
    // 字符类型
    ASTC_TYPE_CHAR,               // char (实现定义的有符号性)
    ASTC_TYPE_SIGNED_CHAR,        // signed char
    ASTC_TYPE_UNSIGNED_CHAR,      // unsigned char
    ASTC_TYPE_CHAR16,             // char16_t (C11)
    ASTC_TYPE_CHAR32,             // char32_t (C11)
    ASTC_TYPE_WCHAR,              // wchar_t
    
    // 整数类型
    ASTC_TYPE_SHORT,              // short (int)
    ASTC_TYPE_UNSIGNED_SHORT,     // unsigned short (int)
    ASTC_TYPE_UNSIGNED_INT,       // unsigned int
    ASTC_TYPE_LONG,               // long (int)
    ASTC_TYPE_UNSIGNED_LONG,      // unsigned long (int)
    ASTC_TYPE_LONG_LONG,          // long long (int) (C99)
    ASTC_TYPE_UNSIGNED_LONG_LONG, // unsigned long long (int) (C99)
    
    // 浮点类型
    ASTC_TYPE_FLOAT,              // float
    ASTC_TYPE_DOUBLE,             // double
    ASTC_TYPE_LONG_DOUBLE,        // long double
    ASTC_TYPE_FLOAT128,           // _Float128 (C23)
    
    // 布尔和空指针
    ASTC_TYPE_BOOL,               // _Bool (C99)
    ASTC_TYPE_NULLPTR,            // nullptr_t (C23)
    
    // 复合类型
    ASTC_TYPE_STRUCT,             // 结构体
    ASTC_TYPE_UNION,              // 联合体
    ASTC_TYPE_ENUM,               // 枚举
    
    // 派生类型
    ASTC_TYPE_POINTER,            // 指针
    ASTC_TYPE_ARRAY,              // 数组
    ASTC_TYPE_FUNCTION,           // 函数
    ASTC_TYPE_TYPEDEF_NAME,       // 类型定义名
    ASTC_TYPE_VOIDPTR,            // void*

    // ===== 模块系统类型 =====
    ASTC_MODULE_DECL,             // 模块声明
    ASTC_EXPORT_DECL,             // 导出声明
    ASTC_IMPORT_DECL,             // 导入声明
    ASTC_REQUIRES_DECL,           // 依赖声明
    ASTC_MODULE_ATTRIBUTE,        // 模块属性注解
    ASTC_SYMBOL_REF               // 符号引用
} ASTNodeType;

// ASTC节点结构
typedef struct ASTNode {
    ASTNodeType type;
    int line;
    int column;
    
    // 节点数据
    union {
        // 标识符
        struct {
            char *name;
        } identifier;
        
        // 常量
        struct {
            ASTNodeType type;  // 常量类型
            union {
                int64_t int_val;
                double float_val;
            };
        } constant;
        
        // 字符串字面量
        struct {
            char *value;
        } string_literal;
        
        // 二元操作
        struct {
            ASTNodeType op;
            struct ASTNode *left;
            struct ASTNode *right;
        } binary_op;
        
        // 一元操作
        struct {
            ASTNodeType op;
            struct ASTNode *operand;
        } unary_op;
        
        // 函数调用
        struct {
            struct ASTNode *callee;
            struct ASTNode **args;
            int arg_count;
            bool is_libc_call;      // 标记是否为libc函数调用
            uint16_t libc_func_id;  // libc函数ID（如果是libc调用）
        } call_expr;
        
        // 翻译单元
        struct {
            struct ASTNode **declarations;
            int declaration_count;
        } translation_unit;
        
        // 函数声明
        struct {
            char *name;
            struct ASTNode *return_type;
            struct ASTNode **params;
            int param_count;
            int has_body;
            struct ASTNode *body;
        } func_decl;
        
        // 变量声明
        struct {
            char *name;
            struct ASTNode *type;
            struct ASTNode *initializer;
        } var_decl;
        
        // 类型说明符
        struct {
            ASTNodeType type;
        } type_specifier;
        
        // 结构体声明
        struct {
            char *name;
            struct ASTNode **members;
            int member_count;
        } struct_decl;
        
        // 联合体声明
        struct {
            char *name;
            struct ASTNode **members;
            int member_count;
        } union_decl;
        
        // 枚举声明
        struct {
            char *name;
            struct ASTNode **constants;
            int constant_count;
        } enum_decl;
        
        // 枚举常量
        struct {
            char *name;
            int has_value;
            struct ASTNode *value;
        } enum_constant;
        
        // 复合语句
        struct {
            struct ASTNode **statements;
            int statement_count;
        } compound_stmt;
        
        // if语句
        struct {
            struct ASTNode *condition;
            struct ASTNode *then_branch;
            struct ASTNode *else_branch;
        } if_stmt;
        
        // while语句
        struct {
            struct ASTNode *condition;
            struct ASTNode *body;
        } while_stmt;
        
        // for语句
        struct {
            struct ASTNode *init;
            struct ASTNode *condition;
            struct ASTNode *increment;
            struct ASTNode *body;
        } for_stmt;
        
        // return语句
        struct {
            struct ASTNode *value;
        } return_stmt;
        
        // 表达式语句
        struct {
            struct ASTNode *expr;
        } expr_stmt;
        
        // 指针类型
        struct {
            struct ASTNode *base_type;
            int pointer_level; // 指针层级，如 int* 为1，int** 为2
        } pointer_type;
        
        // 数组类型
        struct {
            struct ASTNode *element_type;  // 数组元素类型
            struct ASTNode *size_expr;     // 数组大小表达式，可以为NULL表示未指定大小
            int dimensions;                // 数组维度，如int[10]为1，int[5][10]为2
            struct ASTNode **dim_sizes;    // 多维数组的各维度大小表达式
        } array_type;
        
        // 函数指针类型
        struct {
            struct ASTNode *return_type;   // 返回类型
            struct ASTNode **param_types;  // 参数类型列表
            int param_count;               // 参数数量
            int is_variadic;               // 是否为可变参数函数（如 void foo(int, ...)）
        } function_type;
        
        // 数组访问表达式
        struct {
            struct ASTNode *array;         // 数组表达式
            struct ASTNode *index;         // 索引表达式
        } array_subscript;
        
        // 成员访问表达式
        struct {
            struct ASTNode *object;        // 对象表达式
            char *member;                  // 成员名称
        } member_access;
        
        // 指针成员访问表达式
        struct {
            struct ASTNode *pointer;       // 指针表达式
            char *member;                  // 成员名称
        } ptr_member_access;

        // 复合字面量表达式 (C99)
        struct {
            struct ASTNode **expressions;  // 初始化表达式列表
            int expression_count;          // 表达式数量
        } compound_literal;

        // ===============================================
        // 模块系统节点 (Module System Nodes)
        // ===============================================

        // 模块声明
        struct {
            char *name;                    // 模块名称
            char *version;                 // 模块版本 (可选)
            char *author;                  // 模块作者 (可选)
            char *description;             // 模块描述 (可选)
            char *license;                 // 模块许可证 (可选)
            struct ASTNode **declarations; // 模块内的声明列表
            int declaration_count;         // 声明数量
            struct ASTNode **exports;      // 导出列表
            int export_count;              // 导出数量
            struct ASTNode **imports;      // 导入列表
            int import_count;              // 导入数量
            struct ASTNode *init_func;     // 模块初始化函数 (可选)
            struct ASTNode *cleanup_func;  // 模块清理函数 (可选)
        } module_decl;

        // 导出声明
        struct {
            char *name;                    // 导出名称
            char *alias;                   // 导出别名 (可选)
            ASTNodeType export_type;       // 导出类型 (函数/变量/常量/类型)
            struct ASTNode *declaration;   // 被导出的声明
            bool is_default;               // 是否为默认导出
            int flags;                     // 导出标志
        } export_decl;

        // 导入声明
        struct {
            char *module_name;             // 被导入的模块名称
            char *import_name;             // 导入的符号名称
            char *local_name;              // 本地别名 (可选)
            char *version_requirement;    // 版本要求 (可选)
            ASTNodeType import_type;       // 导入类型 (函数/变量/常量/类型)
            bool is_weak;                  // 是否为弱导入 (可选依赖)
            bool is_lazy;                  // 是否为懒导入 (按需加载)
            struct ASTNode *declaration;   // 导入的声明 (extern声明)
        } import_decl;

        // 模块依赖声明
        struct {
            char *module_name;             // 依赖的模块名称
            char *version_requirement;    // 版本要求
            bool is_optional;              // 是否为可选依赖
        } requires_decl;

        // 模块属性注解
        struct {
            char *attribute_name;          // 属性名称 (如 "module", "export", "import")
            char *attribute_value;         // 属性值
            struct ASTNode *target;        // 被注解的目标节点
        } module_attribute;

        // 符号引用 (用于模块间符号解析)
        struct {
            char *module_name;             // 模块名称 (可选，NULL表示当前模块)
            char *symbol_name;             // 符号名称
            ASTNodeType symbol_type;       // 符号类型
            bool is_resolved;              // 是否已解析
            void *resolved_address;        // 解析后的地址 (运行时使用)
        } symbol_ref;
    } data;
} ASTNode;
struct ASTNode* ast_create_node(ASTNodeType type, int line, int column);
void ast_free(struct ASTNode *node);
void ast_print(struct ASTNode *node, int indent);

// ===============================================
// 模块系统函数声明
// ===============================================

// 模块创建和管理
struct ASTNode* ast_create_module(const char* name, const char* version);
struct ASTNode* ast_create_export(const char* name, ASTNodeType export_type, struct ASTNode* declaration);
struct ASTNode* ast_create_import(const char* module_name, const char* import_name, const char* local_name);
struct ASTNode* ast_create_requires(const char* module_name, const char* version_requirement);
struct ASTNode* ast_create_module_attribute(const char* attr_name, const char* attr_value, struct ASTNode* target);
struct ASTNode* ast_create_symbol_ref(const char* module_name, const char* symbol_name, ASTNodeType symbol_type);

// 模块操作
int ast_module_add_declaration(struct ASTNode* module, struct ASTNode* declaration);
int ast_module_add_export(struct ASTNode* module, struct ASTNode* export_decl);
int ast_module_add_import(struct ASTNode* module, struct ASTNode* import_decl);
struct ASTNode* ast_module_find_export(struct ASTNode* module, const char* name);
struct ASTNode* ast_module_find_import(struct ASTNode* module, const char* name);

// 符号解析
int ast_resolve_symbol_references(struct ASTNode* module);
int ast_validate_module_dependencies(struct ASTNode* module);
int ast_check_circular_dependencies(struct ASTNode** modules, int module_count);

// 模块序列化/反序列化 (用于ASTC格式)
int ast_serialize_module(struct ASTNode* module, uint8_t** buffer, size_t* size);
struct ASTNode* ast_deserialize_module(const uint8_t* buffer, size_t size);

// 模块验证
int ast_validate_module(struct ASTNode* module);
int ast_validate_export_declaration(struct ASTNode* export_decl);
int ast_validate_import_declaration(struct ASTNode* import_decl);

/**
 * C99 Compiler Context for ASTC execution
 */
typedef struct C99CompilerContext {
    char input_file[512];
    char output_file[512];
    char* source_code;
    size_t source_size;
    struct ASTNode* ast_root;
    uint8_t* bytecode;
    size_t bytecode_size;
    int optimization_level;
    int debug_info;
    char target_arch[64];
    char* error_message;
    int error_count;
    int warning_count;
} C99CompilerContext;

/**
 * ASTC Program structure for runtime execution
 */
typedef struct ASTCProgram {
    char program_name[256];
    uint32_t version;
    uint32_t flags;
    uint32_t entry_point;
    uint32_t source_size;
    char* source_code;
    uint32_t bytecode_size;
    uint8_t* bytecode;
    C99CompilerContext* compiler_context;  // C99编译器上下文
} ASTCProgram;

// ASTC Program management functions
ASTCProgram* astc_load_program(const char* astc_file);
void astc_free_program(ASTCProgram* program);
int astc_validate_program(const ASTCProgram* program);

#endif // ASTC_H
