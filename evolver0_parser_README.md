# evolver0 Parser 模块说明

## 概述

`evolver0_parser.c` 是第零代自举编译器的语法分析器模块，实现了完整的C语言解析功能。这个模块可以将C源代码解析成抽象语法树（AST），为后续的代码生成阶段提供基础。

## 功能特性

### 已实现的功能

1. **完整的表达式解析**
   - 所有C语言操作符的优先级和结合性
   - 一元、二元、三元操作符
   - 函数调用、数组下标、成员访问
   - 类型转换、sizeof操作符

2. **所有控制流语句**
   - if/else 条件语句
   - while、do-while 循环
   - for 循环（包括C99风格的声明）
   - switch/case/default
   - break、continue、goto、return
   - 标签语句

3. **声明和定义**
   - 函数定义和声明
   - 变量声明（带初始化）
   - 类型说明符（基本类型、指针、数组、函数）
   - 存储类说明符（static、extern、typedef等）
   - 类型限定符（const、volatile、restrict）

4. **调试和测试**
   - AST打印功能
   - 完整的测试程序
   - 错误位置报告

### 待实现的功能

1. struct/union/enum 的完整支持
2. typedef 名称的符号表查找
3. 预处理器指令处理
4. 更完善的错误恢复机制

## 使用方法

### 基本使用

```c
#include "evolver0_parser.c"

// 1. 创建token数组（通过词法分析器）
Token *tokens;
int token_count = tokenize(source_code, &tokens);

// 2. 创建解析器
Parser *parser = create_parser(tokens, token_count);

// 3. 执行解析
ASTNode *ast = parse(parser);

// 4. 处理结果
if (ast) {
    // 解析成功，使用AST
    print_ast(ast);  // 打印AST用于调试
    
    // ... 进行代码生成等后续处理
    
    // 释放AST
    free_ast_node(ast);
} else {
    // 解析失败，处理错误
    fprintf(stderr, "Parse error: %s at line %d, column %d\n",
            parser->error_msg, parser->error_line, parser->error_column);
}

// 5. 清理资源
free_parser(parser);
```

### 运行测试程序

```bash
# 编译测试程序
gcc -o test_evolver0_parser test_evolver0_parser.c -w

# 运行测试
./test_evolver0_parser
```

## AST节点类型

解析器生成的AST包含以下主要节点类型：

- **AST_TRANSLATION_UNIT**: 整个翻译单元（源文件）
- **AST_FUNCTION_DEF**: 函数定义
- **AST_VAR_DECL**: 变量声明
- **AST_IF_STMT**: if语句
- **AST_WHILE_STMT**: while语句
- **AST_FOR_STMT**: for语句
- **AST_RETURN_STMT**: return语句
- **AST_BINARY_EXPR**: 二元表达式
- **AST_UNARY_EXPR**: 一元表达式
- **AST_CALL_EXPR**: 函数调用
- **AST_IDENTIFIER**: 标识符
- **AST_INT_LITERAL**: 整数字面量
- **AST_STRING_LITERAL**: 字符串字面量

完整的节点类型定义请参考 `evolver0_parser.c` 中的 `ASTNodeType` 枚举。

## 示例

### 解析简单函数

输入代码：
```c
int add(int a, int b) {
    return a + b;
}
```

生成的AST：
```
TranslationUnit
  FunctionDef: add
    ReturnType:
      TypeSpec: int
    Parameters:
      ParamList
        Param: a
          TypeSpec: int
        Param: b
          TypeSpec: int
    Body:
      CompoundStmt
        StmtList
          ReturnStmt
            BinaryExpr: +
              Identifier: a
              Identifier: b
```

### 解析控制流

输入代码：
```c
int max(int a, int b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}
```

生成的AST会包含 `IfStmt` 节点，其中包含条件表达式、then分支和else分支。

## 与evolver0.c的集成

要将解析器集成到evolver0.c中，需要：

1. 替换当前的简单 `parse_and_generate` 函数
2. 使用解析器生成的AST进行代码生成
3. 实现从AST到机器码的转换

示例集成代码：
```c
// 在evolver0.c中
static int parse_and_generate(BootstrapCompiler *compiler) {
    // 创建解析器
    Parser *parser = create_parser(compiler->tokens, compiler->token_count);
    
    // 解析生成AST
    ASTNode *ast = parse(parser);
    if (!ast) {
        fprintf(stderr, "解析错误: %s\n", parser->error_msg);
        free_parser(parser);
        return -1;
    }
    
    // TODO: 遍历AST生成机器码
    // generate_code_from_ast(ast, &compiler->machine_code);
    
    free_ast_node(ast);
    free_parser(parser);
    return 0;
}
```

## 下一步工作

1. **完善解析器**
   - 添加struct/union/enum支持
   - 实现完整的类型系统
   - 改进符号表管理

2. **实现代码生成**
   - 设计AST到中间表示的转换
   - 实现x86-64指令选择
   - 添加寄存器分配

3. **生成可执行文件**
   - 实现ELF文件格式
   - 支持代码段和数据段
   - 处理符号和重定位

## 贡献指南

如果要为解析器添加新功能：

1. 在 `ASTNodeType` 中添加新的节点类型
2. 在 `ASTNode` 结构中添加相应的数据字段
3. 实现相应的解析函数
4. 添加AST打印支持
5. 编写测试用例

## 许可证

本模块是evolver0项目的一部分，遵循项目的整体许可证。