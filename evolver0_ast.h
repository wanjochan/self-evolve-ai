/**
 * evolver0_ast.h - AST节点定义
 */

#ifndef EVOLVER0_AST_H
#define EVOLVER0_AST_H

// AST节点类型
#ifndef EVOLVER0_AST_NODE_TYPE_DEFINED
#define EVOLVER0_AST_NODE_TYPE_DEFINED
typedef enum {
    // 基础节点
    AST_TRANSLATION_UNIT,
    AST_FUNCTION_DEF,
    AST_FUNCTION_DECL,
    AST_PARAM_DECL,
    AST_VAR_DECL,
    AST_TYPE_NAME,
    
    // 语句
    AST_COMPOUND_STMT,
    AST_EXPRESSION_STMT,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_DO_WHILE_STMT,
    AST_FOR_STMT,
    AST_RETURN_STMT,
    AST_BREAK_STMT,
    AST_CONTINUE_STMT,
    AST_GOTO_STMT,
    AST_LABEL_STMT,
    
    // 表达式
    AST_INTEGER_LITERAL,
    AST_STRING_LITERAL,
    AST_CHAR_LITERAL,
    AST_IDENTIFIER,
    AST_BINARY_EXPR,
    AST_UNARY_EXPR,
    AST_ASSIGNMENT_EXPR,
    AST_CALL_EXPR,
    AST_ARRAY_SUBSCRIPT_EXPR,
    AST_MEMBER_EXPR,
    AST_POST_INCREMENT_EXPR,
    AST_POST_DECREMENT_EXPR,
    AST_CAST_EXPR,
    AST_SIZEOF_EXPR,
    AST_CONDITIONAL_EXPR,
    
    // 旧的兼容类型
    AST_PROGRAM,
    AST_FUNCTION,
    AST_PARAMETER,
    AST_RETURN,
    AST_INTEGER,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_COMPOUND,
    AST_DECLARATION,
    AST_ASSIGNMENT,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_CALL,
    AST_ARRAY_ACCESS,
    AST_CAST,
    AST_SIZEOF,
    AST_TYPE,
    AST_BREAK,
    AST_CONTINUE
} ASTNodeTypeDead;

//IMPORTANT
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
} ASTNodeType;
#endif

// AST创建和释放函数声明
struct ASTNode;
struct ASTNode* ast_create_node(ASTNodeType type, int line, int column);
void ast_free(struct ASTNode *node);
void ast_print(struct ASTNode *node, int indent);

#endif // EVOLVER0_AST_H