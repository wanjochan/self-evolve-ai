# Pipeline模块API使用指南

## 概述

本文档为pipeline_module拆分提供统一的API使用规范和编码指南。

## AST节点API规范

### 1. 节点创建

**API签名**: `struct ASTNode* ast_create_node(ASTNodeType type, int line, int column)`

**正确使用**:
```c
// 创建函数声明节点
ASTNode* func_decl = ast_create_node(ASTC_FUNC_DECL, token->line, token->column);

// 创建常量表达式节点
ASTNode* constant = ast_create_node(ASTC_EXPR_CONSTANT, 1, 8);

// 创建返回语句节点
ASTNode* return_stmt = ast_create_node(ASTC_RETURN_STMT, 1, 1);
```

### 2. 节点类型规范

使用 `ASTC_` 前缀的类型常量：

**语句类型**:
- `ASTC_COMPOUND_STMT` - 复合语句
- `ASTC_RETURN_STMT` - 返回语句
- `ASTC_IF_STMT` - if语句
- `ASTC_WHILE_STMT` - while循环
- `ASTC_FOR_STMT` - for循环
- `ASTC_EXPR_STMT` - 表达式语句

**表达式类型**:
- `ASTC_EXPR_CONSTANT` - 常量表达式
- `ASTC_EXPR_IDENTIFIER` - 标识符表达式
- `ASTC_EXPR_STRING_LITERAL` - 字符串字面量
- `ASTC_BINARY_OP` - 二元操作
- `ASTC_UNARY_OP` - 一元操作
- `ASTC_CALL_EXPR` - 函数调用

**声明类型**:
- `ASTC_FUNC_DECL` - 函数声明
- `ASTC_VAR_DECL` - 变量声明
- `ASTC_TRANSLATION_UNIT` - 翻译单元

**类型说明符**:
- `ASTC_TYPE_INT` - int类型
- `ASTC_TYPE_FLOAT` - float类型
- `ASTC_TYPE_CHAR` - char类型
- `ASTC_TYPE_VOID` - void类型

### 3. 节点数据设置

**函数声明示例**:
```c
ASTNode* func = ast_create_node(ASTC_FUNC_DECL, 1, 1);

// 设置函数名
func->data.func_decl.name = strdup("test_function");

// 设置返回类型
func->data.func_decl.return_type = ast_create_node(ASTC_TYPE_INT, 1, 1);

// 设置函数体
func->data.func_decl.body = ast_create_node(ASTC_COMPOUND_STMT, 1, 1);
```

**常量表达式示例**:
```c
ASTNode* constant = ast_create_node(ASTC_EXPR_CONSTANT, 1, 1);

// 设置常量类型和值
constant->data.constant.type = ASTC_TYPE_INT;
constant->data.constant.int_val = 42;
```

**二元操作示例**:
```c
ASTNode* binary_op = ast_create_node(ASTC_BINARY_OP, 1, 1);

// 设置操作符
binary_op->data.binary_op.op = ASTC_OP_ADD;

// 设置左右操作数
binary_op->data.binary_op.left = left_expr;
binary_op->data.binary_op.right = right_expr;
```

### 4. 内存管理

**释放节点**:
```c
// 使用 ast_free 而不是 ast_free_node
ast_free(node);
```

**字符串处理**:
```c
// 分配字符串内容
node->data.func_decl.name = strdup(name_string);
```

## Token处理规范

### Token类型定义

```c
typedef enum {
    TOKEN_EOF = 0,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_CHAR_LITERAL,
    
    // 运算符
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    // ... 其他
    
    // 关键字
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_RETURN,
    // ... 其他
} TokenType;

typedef struct {
    TokenType type;
    char* value;        // 使用 strdup 分配
    int line;
    int column;
} Token;
```

### Token内存管理

```c
// 创建Token
Token* token = malloc(sizeof(Token));
token->type = type;
token->value = strdup(value_string);  // 分配字符串副本
token->line = line;
token->column = column;

// 释放Token
if (token->value) free(token->value);
free(token);
```

## 错误处理规范

### 统一错误处理模式

```c
// 解析器错误处理
typedef struct {
    Token** tokens;
    int token_count;
    int current;
    char error_msg[256];        // 固定大小错误消息
} Parser;

// 设置错误消息
static void set_parse_error(Parser* parser, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(parser->error_msg, sizeof(parser->error_msg), format, args);
    va_end(args);
}

// 错误检查模式
ASTNode* node = parse_expression(parser);
if (!node) {
    fprintf(stderr, "Parse error: %s\n", parser->error_msg);
    return NULL;
}
```

## 模块接口规范

### 统一的模块接口

```c
// 所有子模块都应该提供的基本接口

// 词法分析器接口
bool frontend_tokenize(const char* source, Token*** tokens, int* token_count);
ASTNode* frontend_parse(Token** tokens, int token_count);
void frontend_free_tokens(Token** tokens, int token_count);
ASTNode* frontend_compile(const char* source);

// 代码生成器接口
bool backend_generate_assembly(ASTNode* ast, char** assembly_code);
bool backend_generate_bytecode(ASTNode* ast, uint8_t** bytecode, size_t* size);

// 优化器接口
bool optimizer_optimize_ast(ASTNode* ast, int optimization_level);

// 虚拟机接口
bool vm_execute_bytecode(const uint8_t* bytecode, size_t size);
```

### 数据传递规范

```c
// 模块间数据传递使用明确的所有权规则

// 返回新分配的数据 - 调用者负责释放
ASTNode* create_node_data(void);

// 接受数据所有权 - 被调用者负责释放  
void consume_node_data(ASTNode* node);

// 借用数据 - 调用者保持所有权
bool process_node_data(const ASTNode* node);
```

## 编码风格规范

### 1. 命名约定

- **函数名**: `module_function_name` (模块前缀 + 下划线分隔)
- **类型名**: `ModuleName` (帕斯卡命名)
- **变量名**: `variable_name` (下划线分隔)
- **常量**: `CONSTANT_NAME` (全大写)

### 2. 文件组织

```c
/**
 * module_name.c - Module Description
 * 
 * 模块功能简介
 */

#include "../module.h"
#include "../astc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ===============================================
// 模块内部类型定义
// ===============================================

// ===============================================
// 模块内部函数声明
// ===============================================

// ===============================================
// 模块实现
// ===============================================

// ===============================================
// 对外接口实现
// ===============================================
```

### 3. 注释规范

```c
/**
 * 函数功能简述
 * 
 * @param param1 参数1描述
 * @param param2 参数2描述
 * @return 返回值描述
 */
```

## 常见陷阱和注意事项

### 1. 内存泄漏

```c
// ❌ 错误：忘记释放字符串
node->data.func_decl.name = "function_name";  

// ✅ 正确：分配字符串副本
node->data.func_decl.name = strdup("function_name");
```

### 2. 空指针检查

```c
// ✅ 总是检查指针有效性
if (!node || !node->data.func_decl.name) {
    return NULL;
}
```

### 3. 数组越界

```c
// ✅ 检查数组边界
if (parser->current >= parser->token_count) {
    set_parse_error(parser, "Unexpected end of input");
    return NULL;
}
```

## 测试规范

### 单元测试模式

```c
void test_module_function(void) {
    // Arrange
    // Act  
    // Assert
    TEST_PASS();
}
```

### 集成测试要求

每个拆分的模块都必须：
1. 独立编译通过
2. 通过单元测试
3. 与其他模块集成测试通过
4. 不影响现有功能

---

**版本**: 1.0  
**更新时间**: 2025-01-14  
**适用范围**: pipeline_module拆分项目