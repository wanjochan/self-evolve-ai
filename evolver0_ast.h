/**
 * evolver0_ast.h - AST节点定义
 */

#ifndef EVOLVER0_AST_H
#define EVOLVER0_AST_H

// AST节点类型
#ifndef EVOLVER0_AST_NODE_TYPE_DEFINED
#define EVOLVER0_AST_NODE_TYPE_DEFINED

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
    AST_TABLE_FILL = 0xFC11      // 表填充
    
    // ===== 扩展节点 (AST-C) =====
    // 声明和定义
    ASTC_TRANSLATION_UNIT,  // 翻译单元
    ASTC_FUNCTION_DEF,      // 函数定义
    ASTC_FUNCTION_DECL,     // 函数声明
    ASTC_VAR_DECL,         // 变量声明
    ASTC_PARAM_DECL,       // 参数声明
    
    // 复合类型
    ASTC_STRUCT_DECL,      // 结构体声明
    ASTC_UNION_DECL,       // 联合体声明
    ASTC_ENUM_DECL,        // 枚举声明
    ASTC_TYPEDEF_DECL,     // 类型定义
    
    // 类型节点
    ASTC_PRIMITIVE_TYPE,   // 基本类型
    ASTC_POINTER_TYPE,     // 指针类型
    ASTC_ARRAY_TYPE,       // 数组类型
    ASTC_FUNCTION_TYPE,    // 函数类型
    
    // 控制流
    ASTC_IF_STMT,          // if 语句
    ASTC_SWITCH_STMT,      // switch 语句
    ASTC_CASE_STMT,        // case 语句
    ASTC_DEFAULT_STMT,     // default 语句
    ASTC_WHILE_STMT,       // while 循环
    ASTC_DO_STMT,          // do-while 循环
    ASTC_FOR_STMT,         // for 循环
    ASTC_GOTO_STMT,        // goto 语句
    ASTC_LABEL_STMT,       // 标签语句
    ASTC_CONTINUE_STMT,    // continue 语句
    ASTC_BREAK_STMT,       // break 语句
    ASTC_RETURN_STMT,      // return 语句
    
    // 表达式
    ASTC_IDENTIFIER,       // 标识符
    ASTC_CONSTANT,         // 常量
    ASTC_STRING_LITERAL,   // 字符串字面量
    ASTC_UNARY_OP,         // 一元操作
    ASTC_BINARY_OP,        // 二元操作
    ASTC_TERNARY_OP,       // 三元操作
    ASTC_CALL_EXPR,        // 函数调用
    ASTC_ARRAY_SUBSCRIPT,  // 数组下标
    ASTC_MEMBER_ACCESS,    // 成员访问
    ASTC_PTR_MEMBER_ACCESS,// 指针成员访问
    ASTC_CAST_EXPR,        // 类型转换
    
    // 表达式类型
    ASTC_EXPR_IDENTIFIER,        // 标识符
    ASTC_EXPR_CONSTANT,          // 常量
    ASTC_EXPR_STRING_LITERAL,    // 字符串字面量
    ASTC_EXPR_COMPOUND_LITERAL,  // 复合字面量 (C99)
    ASTC_EXPR_FUNC_CALL,         // 函数调用
    ASTC_EXPR_ARRAY_SUBSCRIPT,   // 数组下标
    ASTC_EXPR_MEMBER_ACCESS,     // 成员访问
    ASTC_EXPR_PTR_MEMBER_ACCESS, // 指针成员访问
    ASTC_EXPR_POST_INC,          // 后置++
    ASTC_EXPR_POST_DEC,          // 后置--
    ASTC_EXPR_PRE_INC,           // 前置++
    ASTC_EXPR_PRE_DEC,           // 前置--
    ASTC_EXPR_ADDR,              // 取地址 &
    ASTC_EXPR_DEREF,             // 解引用 *
    ASTC_EXPR_PLUS,              // 正号 +
    ASTC_EXPR_MINUS,             // 负号 -
    ASTC_EXPR_BIT_NOT,           // 按位取反 ~
    ASTC_EXPR_LOGICAL_NOT,       // 逻辑非 !
    ASTC_EXPR_SIZEOF,            // sizeof
    ASTC_EXPR_ALIGNOF,           // _Alignof (C11)
    ASTC_EXPR_GENERIC,           // _Generic (C11)
    ASTC_EXPR_MUL,               // 乘 *
    ASTC_EXPR_DIV,               // 除 /
    ASTC_EXPR_MOD,               // 取模 %
    ASTC_EXPR_ADD,               // 加 +
    ASTC_EXPR_SUB,               // 减 -
    ASTC_EXPR_LEFT_SHIFT,        // 左移 <<
    ASTC_EXPR_RIGHT_SHIFT,       // 右移 >>
    ASTC_EXPR_LESS,              // 小于 <
    ASTC_EXPR_LESS_EQUAL,        // 小于等于 <=
    ASTC_EXPR_GREATER,           // 大于 >
    ASTC_EXPR_GREATER_EQUAL,     // 大于等于 >=
    ASTC_EXPR_EQUAL,             // 等于 ==
    ASTC_EXPR_NOT_EQUAL,         // 不等于 !=
    ASTC_EXPR_BIT_AND,           // 按位与 &
    ASTC_EXPR_BIT_XOR,           // 按位异或 ^
    ASTC_EXPR_BIT_OR,            // 按位或 |
    ASTC_EXPR_LOGICAL_AND,       // 逻辑与 &&
    ASTC_EXPR_LOGICAL_OR,        // 逻辑或 ||
    ASTC_EXPR_CONDITIONAL,       // 条件 ? :
    ASTC_EXPR_ASSIGN,            // =
    ASTC_EXPR_ADD_ASSIGN,        // +=
    ASTC_EXPR_SUB_ASSIGN,        // -=
    ASTC_EXPR_MUL_ASSIGN,        // *=
    ASTC_EXPR_DIV_ASSIGN,        // /=
    ASTC_EXPR_MOD_ASSIGN,        // %=
    ASTC_EXPR_LEFT_SHIFT_ASSIGN, // <<=
    ASTC_EXPR_RIGHT_SHIFT_ASSIGN,// >>=
    ASTC_EXPR_BIT_AND_ASSIGN,    // &=
    ASTC_EXPR_BIT_XOR_ASSIGN,    // ^=
    ASTC_EXPR_BIT_OR_ASSIGN,     // |=
    ASTC_EXPR_COMMA,             // 逗号表达式
    ASTC_EXPR_CAST,              // 类型转换 (type)expr
    ASTC_EXPR_VA_ARG,            // va_arg
    ASTC_EXPR_STATEMENT_EXPR,    // ({...})
    ASTC_EXPR_RANGE,             // x..y (GCC)
    ASTC_EXPR_BUILTIN_CHOOSE_EXPR, // __builtin_choose_expr
    ASTC_EXPR_BUILTIN_TYPES_COMPATIBLE_P, // __builtin_types_compatible_p
    ASTC_EXPR_BUILTIN_OFFSETOF,   // offsetof
    ASTC_EXPR_BUILTIN_VA_ARG,     // __builtin_va_arg
    ASTC_EXPR_BUILTIN_VA_COPY,    // __builtin_va_copy
    ASTC_EXPR_BUILTIN_VA_END,     // __builtin_va_end
    ASTC_EXPR_BUILTIN_VA_START,   // __builtin_va_start
    ASTC_EXPR_ATTRIBUTE,         // __attribute__
    ASTC_EXPR_ASM,               // __asm__
    ASTC_EXPR_ERROR,             // 错误表达式

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
    ASTC_STMT_GCC_ASM,           // GCC 内联汇编
    ASTC_STMT_MS_ASM,            // MSVC 内联汇编
    ASTC_STMT_SEH_LEAVE,         // SEH __leave
    ASTC_STMT_SEH_TRY,           // SEH __try
    ASTC_STMT_SEH_EXCEPT,        // SEH __except
    ASTC_STMT_SEH_FINALLY,       // SEH __finally
    ASTC_STMT_MS_DECLSPEC,       // MS __declspec
    ASTC_STMT_CXX_CATCH,         // C++ catch
    ASTC_STMT_CXX_TRY,           // C++ try
    ASTC_STMT_CXX_FOR_RANGE,     // C++11 范围 for
    ASTC_STMT_MS_TRY,            // MS __try
    ASTC_STMT_MS_EXCEPT,         // MS __except
    ASTC_STMT_MS_FINALLY,        // MS __finally
    ASTC_STMT_MS_LEAVE,          // MS __leave
    ASTC_STMT_PRAGMA,            // #pragma 指令
    ASTC_STMT_ERROR,             // 错误语句

    // 声明类型
    ASTC_DECL_NONE,
    ASTC_DECL_VAR,               // 变量声明
    ASTC_DECL_FUNCTION,          // 函数声明
    ASTC_DECL_FUNCTION_DEF,      // 函数定义
    ASTC_DECL_STRUCT,            // 结构体定义
    ASTC_DECL_UNION,             // 联合体定义
    ASTC_DECL_ENUM,              // 枚举定义
    ASTC_DECL_ENUM_CONSTANT,     // 枚举常量
    ASTC_DECL_TYPEDEF,           // 类型定义
    ASTC_DECL_LABEL,             // 标签
    ASTC_DECL_FIELD,             // 结构体/联合体字段
    ASTC_DECL_PARAM,             // 函数参数
    ASTC_DECL_RECORD,            // 记录(结构体/联合体)
    ASTC_DECL_INITIALIZER,       // 初始化器
    ASTC_DECL_ATTRIBUTE,         // 属性
    ASTC_DECL_ASM_LABEL,         // 汇编标签
    ASTC_DECL_IMPLICIT,          // 隐式声明
    ASTC_DECL_PACKED,            // 打包属性
    ASTC_DECL_ALIGNED,           // 对齐属性
    ASTC_DECL_TRANSPARENT_UNION, // 透明联合体
    ASTC_DECL_VECTOR,            // 向量类型(GCC)
    ASTC_DECL_EXT_VECTOR,        // 扩展向量类型(Clang)
    ASTC_DECL_COMPLEX,           // 复数类型
    ASTC_DECL_IMAGINARY,         // 虚数类型
    ASTC_DECL_ATOMIC,            // 原子类型(C11)
    ASTC_DECL_THREAD_LOCAL,      // 线程局部存储(C11)
    ASTC_DECL_AUTO_TYPE,         // auto 类型(C23)
    ASTC_DECL_NULLPTR,           // nullptr_t(C23)
    ASTC_DECL_GENERIC_SELECTION, // _Generic 选择(C11)
    ASTC_DECL_OVERLOAD,          // 重载声明(C++)
    ASTC_DECL_TEMPLATE,          // 模板(C++)
    ASTC_DECL_FRIEND,            // 友元(C++)
    ASTC_DECL_USING,             // using 声明(C++)
    ASTC_DECL_CONCEPT,           // 概念(C++20)
    ASTC_DECL_REQUIRES,          // requires 子句(C++20)
    ASTC_DECL_CONSTRAINT,        // 约束(C++20)
    ASTC_DECL_ERROR,             // 错误声明
    
    // 复合表达式
    ASTC_INIT_LIST,        // 初始化列表
    ASTC_DESIGNATION,      // 指示符 (C99)
    ASTC_COMPOUND_LITERAL, // 复合字面量
    ASTC_STMT_EXPR,        // 语句表达式 (GNU扩展)
    
    // 特殊表达式
    ASTC_ALIGNOF_EXPR,     // _Alignof 表达式
    ASTC_OFFSETOF_EXPR,    // offsetof 表达式
    ASTC_VA_ARG_EXPR,      // va_arg 表达式
    ASTC_GENERIC_SELECTION,// _Generic 选择表达式
    
    // 内建函数
    ASTC_BUILTIN_VA_START, // __builtin_va_start
    ASTC_BUILTIN_VA_END,   // __builtin_va_end
    ASTC_BUILTIN_VA_COPY,  // __builtin_va_copy
    ASTC_BUILTIN_OFFSETOF, // __builtin_offsetof
    
    // 内联汇编
    ASTC_ASM_STMT,         // 内联汇编语句
    
    // 预处理和元信息
    ASTC_PREPROCESSING_DIR,// 预处理指令
    ASTC_MACRO_DEFINITION, // 宏定义
    ASTC_MACRO_EXPANSION,  // 宏展开
    ASTC_COMMENT,          // 注释
    ASTC_PRAGMA,           // #pragma指令
    
    // 错误处理
    ASTC_ERROR,            // 错误节点
    
    // ===== C 语言类型 =====
    // 基本类型
    ASTC_TYPE_INVALID,            // 无效类型
    ASTC_TYPE_VOID,               // void
    
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
    ASTC_TYPE_INT,                // int
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
    ASTC_TYPE_VOIDPTR             // void*
} ASTNodeType;
#endif

// AST创建和释放函数声明
struct ASTNode;
struct ASTNode* ast_create_node(ASTNodeType type, int line, int column);
void ast_free(struct ASTNode *node);
void ast_print(struct ASTNode *node, int indent);

#endif // EVOLVER0_AST_H
