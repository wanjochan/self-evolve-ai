# API Reference

## 概述

Self-Evolving AI 项目提供了一套完整的API，包括核心模块系统、编译器接口、错误处理机制等。本文档详细介绍了所有可用的API接口。

## 核心模块系统 API

### 模块管理

#### `module_load(const char* module_name)`
加载指定的模块。

**参数:**
- `module_name`: 模块名称

**返回值:**
- 成功: 模块句柄指针
- 失败: NULL

**示例:**
```c
#include "core/module.h"

Module* layer0 = module_load("layer0");
if (layer0) {
    printf("模块加载成功\n");
} else {
    printf("模块加载失败\n");
}
```

#### `module_unload(Module* module)`
卸载指定的模块。

**参数:**
- `module`: 模块句柄

**返回值:**
- 成功: 0
- 失败: -1

#### `module_resolve(Module* module, const char* symbol_name)`
解析模块中的符号。

**参数:**
- `module`: 模块句柄
- `symbol_name`: 符号名称

**返回值:**
- 成功: 符号地址
- 失败: NULL

### 模块稳定性 API

#### `stable_module_load(const char* module_name)`
使用稳定性增强的模块加载。

**特性:**
- 自动重试机制
- 错误恢复
- 性能统计
- 健康监控

**示例:**
```c
#include "core/module_stability.h"

// 初始化稳定性系统
module_stability_init(NULL);

// 加载模块
void* handle = stable_module_load("layer0");
if (handle) {
    printf("模块加载成功\n");
    
    // 获取模块统计信息
    ModuleStats* stats = module_get_stats("layer0");
    printf("加载次数: %lu\n", stats->load_count);
}

// 清理
module_stability_cleanup();
```

## 统一错误处理 API

### 错误管理器

#### `unified_error_manager_create()`
创建错误管理器实例。

**返回值:**
- 成功: 错误管理器指针
- 失败: NULL

#### `unified_error_system_init()`
初始化全局错误系统。

**返回值:**
- 成功: 0
- 失败: -1

### 错误报告

#### `ERROR_REPORT(manager, code, severity, message)`
报告错误的便利宏。

**参数:**
- `manager`: 错误管理器
- `code`: 错误代码
- `severity`: 错误严重性
- `message`: 错误消息

**示例:**
```c
#include "core/unified_error_handler.h"

// 初始化全局错误系统
unified_error_system_init();

// 报告错误
ERROR_REPORT(g_unified_error_manager, 
            ERROR_CORE_INVALID_PARAM, 
            ERROR_SEVERITY_ERROR, 
            "参数验证失败");

// 报告带详细信息的错误
ERROR_REPORT_FULL(g_unified_error_manager,
                 ERROR_MODULE_NOT_FOUND,
                 ERROR_SEVERITY_ERROR,
                 "模块未找到",
                 "指定的模块文件不存在",
                 "检查模块路径是否正确");

// 清理
unified_error_system_cleanup();
```

### 错误恢复

#### `unified_error_attempt_recovery(manager, error)`
尝试错误恢复。

**参数:**
- `manager`: 错误管理器
- `error`: 错误对象

**返回值:**
- 成功: true
- 失败: false

## C99编译器 API

### 编译接口

#### `c99_compile_file(const char* source_file, const char* output_file)`
编译C99源文件。

**参数:**
- `source_file`: 源文件路径
- `output_file`: 输出文件路径

**返回值:**
- 成功: 0
- 失败: 非0错误代码

**示例:**
```c
#include "c99/c99_compiler.h"

int result = c99_compile_file("hello.c", "hello");
if (result == 0) {
    printf("编译成功\n");
} else {
    printf("编译失败，错误代码: %d\n", result);
}
```

#### `c99_compile_to_object(const char* source_file, const char* object_file)`
编译C99源文件到目标文件。

**参数:**
- `source_file`: 源文件路径
- `object_file`: 目标文件路径

**返回值:**
- 成功: 0
- 失败: 非0错误代码

## ASTC字节码 API

### ASTC程序操作

#### `astc_program_create()`
创建ASTC程序对象。

**返回值:**
- 成功: ASTC程序指针
- 失败: NULL

#### `astc_program_add_instruction(program, opcode, operand)`
向ASTC程序添加指令。

**参数:**
- `program`: ASTC程序对象
- `opcode`: 操作码
- `operand`: 操作数

**返回值:**
- 成功: 0
- 失败: -1

**示例:**
```c
#include "core/astc.h"

ASTCProgram* program = astc_program_create();
if (program) {
    // 添加加载常量指令
    ASTCOperand operand;
    operand.i64 = 42;
    astc_program_add_instruction(program, ASTC_OP_LOAD_CONST, operand);
    
    // 添加返回指令
    astc_program_add_instruction(program, ASTC_OP_RET, operand);
    
    // 执行程序
    int result = astc_program_execute(program);
    printf("程序执行结果: %d\n", result);
    
    // 清理
    astc_program_destroy(program);
}
```

## AI代码分析 API

### 代码分析

#### `ai_analyze_file(const char* file_path)`
分析单个源文件。

**参数:**
- `file_path`: 源文件路径

**返回值:**
- 成功: 分析结果指针
- 失败: NULL

**示例:**
```c
#include "ai/code_analyzer.h"

// 初始化AI分析器
ai_analyzer_init();

// 分析文件
CodeAnalysisResult* result = ai_analyze_file("example.c");
if (result) {
    printf("复杂度评分: %d/100\n", result->complexity_score);
    printf("质量评分: %d/100\n", result->quality_score);
    printf("性能评分: %d/100\n", result->performance_score);
    printf("改进建议数量: %d\n", result->improvement_count);
    
    // 打印改进建议
    for (int i = 0; i < result->improvement_count; i++) {
        CodeImprovement* imp = &result->improvements[i];
        printf("建议 %d: %s\n", i+1, imp->description);
        printf("  修复建议: %s\n", imp->suggested_fix);
        printf("  置信度: %d%%\n", imp->confidence_score);
    }
    
    // 清理
    ai_analysis_result_free(result);
}
```

## 错误代码参考

### 核心系统错误
- `ERROR_CORE_INIT_FAILED` (0x1001): 核心系统初始化失败
- `ERROR_CORE_INVALID_PARAM` (0x1002): 无效参数
- `ERROR_CORE_OUT_OF_MEMORY` (0x1003): 内存不足
- `ERROR_CORE_RESOURCE_BUSY` (0x1004): 资源忙
- `ERROR_CORE_TIMEOUT` (0x1005): 操作超时

### 模块系统错误
- `ERROR_MODULE_NOT_FOUND` (0x2001): 模块未找到
- `ERROR_MODULE_LOAD_FAILED` (0x2002): 模块加载失败
- `ERROR_MODULE_SYMBOL_NOT_FOUND` (0x2003): 符号未找到
- `ERROR_MODULE_VERSION_MISMATCH` (0x2004): 版本不匹配
- `ERROR_MODULE_DEPENDENCY_FAILED` (0x2005): 依赖失败

### 编译器错误
- `ERROR_COMPILER_SYNTAX` (0x3001): 语法错误
- `ERROR_COMPILER_SEMANTIC` (0x3002): 语义错误
- `ERROR_COMPILER_TYPE_MISMATCH` (0x3003): 类型不匹配
- `ERROR_COMPILER_UNDEFINED_SYMBOL` (0x3004): 未定义符号
- `ERROR_COMPILER_INTERNAL` (0x3005): 编译器内部错误

## 最佳实践

### 错误处理
1. 始终检查函数返回值
2. 使用统一错误处理系统
3. 提供有意义的错误消息
4. 实现适当的错误恢复机制

### 模块使用
1. 在程序开始时初始化模块系统
2. 使用稳定性增强的模块加载接口
3. 正确处理模块加载失败的情况
4. 在程序结束时清理模块资源

### 性能优化
1. 使用模块缓存避免重复加载
2. 监控模块加载性能
3. 实现适当的资源管理
4. 使用性能分析工具识别瓶颈

## 示例项目

完整的示例项目请参考：
- `examples/basic_usage/`: 基础使用示例
- `examples/advanced_features/`: 高级功能示例
- `examples/error_handling/`: 错误处理示例
- `examples/performance_optimization/`: 性能优化示例
