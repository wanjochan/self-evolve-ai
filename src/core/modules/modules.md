# Self-Evolve AI 模块化设计文档 (重构版)

## 概述

本文档描述了 Self-Evolve AI 项目经过 tunecore 工作包重构后的新模块化设计架构。原有的12个分散模块已整合为4个核心模块，实现了更优雅的按需加载机制。

## 设计理念

- **模块整合**: 将相关功能合并，减少模块间复杂依赖
- **按需加载**: Python风格的优雅模块加载机制
- **智能解析**: 自动架构检测和路径解析
- **统一接口**: 所有模块遵循相同的接口规范

## 系统架构

```
Layer 1: Loader (loader_{arch}_{bits}.exe)
    ↓ 加载
Layer 2: Runtime (5个核心模块)
    1. module_module       - 增强的模块管理器 (智能加载+符号解析)
    2. layer0_module       - 基础功能 (memory+utils+std+libdl)
    3. pipeline_module/     - 编译流水线:
       ├── frontend: c2astc (C代码→ASTC字节码)
       ├── backend: codegen (ASTC字节码→ASTC汇编) + astc2native (AOT编译，实现 astc 字节码转成原生机器码)
       └── execution: astc + vm (ASTC字节码执行)
    4. compiler_module     - 编译器集成:
       ├── jit (即时编译，ASTC字节码→原生机器码)
       └── ffi (外部函数接口，类似libffi)
    5. libc_module/         - C99标准库；注：这个不着急开工，这个是给后面其它应用包括 c99 编译器替代 ssh.sh用的
    ↓ 执行
Layer 3: Program ({program}.astc)
```

### 功能分布说明

**pipeline_module.c** (编译流水线):
- ✅ frontend: c2astc (C代码转ASTC字节码)
- ✅ backend: codegen (ASTC字节码转ASTC汇编)
- ✅ backend: astc2native (AOT编译) - 已从compiler_module移入
- ✅ execution: astc + vm (ASTC字节码执行)

**compiler_module.c** (编译器集成):
- ✅ jit (即时编译，ASTC字节码→原生机器码)
- ✅ ffi (外部函数接口，类似libffi)
- ✅ aot功能已移至pipeline_module，保持架构清晰

## 按需加载机制

### 智能路径解析
```c
// 自动解析: "./layer0" -> "./layer0_arm64_64.native"
char* resolved = resolve_native_file("./layer0");
```

### 优雅的模块加载
```c
// Python风格的模块加载
Module* module = load_module("./layer0");
```

### 符号解析接口
```c
// 优雅的符号解析和调用
void* func = module->sym("function_name");
int result = ((int(*)(int))func)(42);
```


## 核心模块接口

每个模块都实现以下统一接口：

```c
typedef struct Module {
    const char* name;           // 模块名称
    ModuleState state;          // 当前状态
    const char* error;          // 最后错误信息
    
    int (*init)(void);          // 初始化模块
    void (*cleanup)(void);      // 清理模块
    void* (*resolve)(const char* symbol);  // 解析符号
} Module;
```

## 新模块详细介绍

### 1. 增强的模块管理器 (module_module.c)

**作用**: 系统的核心模块，提供智能的模块加载和管理功能。

**核心功能**:
- **智能路径解析**: 自动添加架构后缀
- **按需加载**: Python风格的模块加载
- **符号解析**: 优雅的函数调用接口
- **模块缓存**: 避免重复加载
- **依赖管理**: 自动处理模块依赖

**关键API**:
- `resolve_native_file()` - 智能路径解析
- `load_module()` - 按需加载模块
- `module->sym()` - 符号解析接口
- `module_resolve()` - 解析符号
- `module_load()` - 传统加载接口

**特点**: 
- 支持自动架构检测 (arm64_64, x64_64等)
- 提供Python风格的优雅接口
- 集成.native文件格式验证
- 支持模块生命周期管理

### 2. Layer0基础模块 (layer0_module.c)

**作用**: 提供系统基础服务，整合了memory+utils+std+libdl功能。

**核心功能**:
- **内存管理**: 多内存池管理和统计
- **工具函数**: 架构检测、文件操作、字符串处理
- **标准库**: 基础C标准库函数
- **动态加载**: dlopen/dlsym/dlclose/dlerror包装

**内存池类型**:
- `MEMORY_POOL_GENERAL` - 通用内存
- `MEMORY_POOL_BYTECODE` - 字节码存储
- `MEMORY_POOL_JIT` - JIT编译代码
- `MEMORY_POOL_MODULES` - 模块数据
- `MEMORY_POOL_TEMP` - 临时数据
- `MEMORY_POOL_C99_*` - C99编译器相关内存池

**关键API**:
- `memory_alloc()` - 分配内存
- `memory_free()` - 释放内存
- `detect_architecture()` - 检测架构
- `dlopen_wrapper()` - 动态库加载
- `safe_strncpy()` - 安全字符串操作

### 3. Pipeline编译流水线模块 (pipeline_module.c)

**作用**: 提供完整的编译执行流水线，整合了frontend+backend+execution功能。

**核心功能**:
- **Frontend**: C源码词法分析和语法分析 (c2astc)
- **Backend**: AST转汇编代码 (codegen) + AOT编译 (astc2native)
- **Execution**: 虚拟机执行ASTC字节码 (astc + vm)
- **统一接口**: 编译和执行的一站式服务

**支持的功能**:
- Token识别和解析 (frontend)
- AST构建和验证 (frontend)
- 代码生成和优化 (backend)
- ASTC字节码转原生代码 (backend - astc2native)
- 虚拟机执行环境 (execution)
- 错误处理和诊断

**关键API**:
- `pipeline_compile()` - 编译C源码
- `pipeline_execute()` - 执行字节码
- `pipeline_compile_and_run()` - 编译并执行
- `pipeline_get_assembly()` - 获取汇编代码
- `pipeline_get_bytecode()` - 获取字节码
- ✅ `pipeline_astc2native()` - AOT编译 (已从compiler_module移入)
- `aot_create_compiler()` - 创建AOT编译器
- `aot_compile_to_executable()` - 编译为可执行文件

### 4. Compiler编译器集成模块 (compiler_module.c)

**作用**: 提供特殊编译方式的集成，整合了jit+ffi功能。

**核心功能**:
- **JIT**: 运行时即时编译，支持x86-64机器码生成
- **FFI**: 外部函数接口，支持动态库加载和函数调用
- **统一接口**: 特殊编译模式的统一管理

**支持的编译模式**:
- 即时编译（JIT）- 运行时生成机器码
- 外部函数接口（FFI）- 调用外部库

**关键API**:
- `compiler_jit_compile()` - JIT编译
- `compiler_ffi_call()` - FFI调用
- `compiler_create_context()` - 创建编译上下文
- `compiler_set_optimization()` - 设置优化级别
- ⚠️ `compiler_aot_compile()` - AOT编译 (应移到pipeline_module作为astc2native)

### 5. LibC标准库模块 (libc_module.c) - 保持独立

**作用**: 提供完整的C99标准库支持，为C99开发提供标准库替代。

**核心功能**:
- **文件I/O**: 文件读写、流操作
- **字符串处理**: 字符串操作、格式化
- **数学函数**: 数学运算、三角函数
- **内存管理**: malloc/free/realloc
- **错误处理**: errno、错误信息

**支持的C99特性**:
- 标准输入输出（stdio.h）
- 字符串操作（string.h）
- 数学函数（math.h）
- 内存管理（stdlib.h）
- 字符处理（ctype.h）
- 时间处理（time.h）

**关键API**:
- `libc_printf()` - 格式化输出
- `libc_malloc()` - 内存分配
- `libc_fopen()` - 文件操作
- `libc_strlen()` - 字符串长度
- `libc_sin()` - 数学函数

## 旧模块整合说明

以下旧模块已被整合到新的四个核心模块中：

### 已整合的模块：
- `memory_module.c` → `layer0_module.c` (内存管理)
- `utils_module.c` → `layer0_module.c` (工具函数)
- `std_module.c` → `layer0_module.c` (标准库基础)
- `c2astc_module.c` → `pipeline_module.c` (前端编译)
- `codegen_module.c` → `pipeline_module.c` (后端代码生成)
- `astc2native_module.c` → `pipeline_module.c` (字节码转换)
- `astc_module.c` → `pipeline_module.c` (AST处理)
- `vm_module.c` → `pipeline_module.c` (虚拟机执行)
- `jit_module.c` → `compiler_module.c` (即时编译)
- `native_module.c` → `module_module.c` (原生模块管理)

### 保持独立的模块：
- `libc_module.c` - 完整的C99标准库支持

## 新模块依赖关系

```
module_module (核心管理器，无依赖)
    ↓
layer0_module (基础服务层)
    ↓
pipeline_module (编译流水线) ← 依赖 layer0
    ↓
compiler_module (编译器集成) ← 依赖 layer0
    ↓
libc_module (C99标准库，独立)

## 新模块加载和初始化

### 1. 按需加载

使用新的按需加载机制：

```c
// 智能路径解析和加载
Module* layer0 = load_module("./layer0");
Module* pipeline = load_module("./pipeline");
Module* compiler = load_module("./compiler");
```

### 2. 初始化顺序

1. `module_module` 首先初始化（自举）
2. `layer0_module` 初始化（提供基础服务）
3. `pipeline_module` 和 `compiler_module` 并行初始化
4. `libc_module` 独立初始化

### 3. 符号解析

使用新的优雅接口：

```c
// 获取模块函数并调用
void* malloc_func = layer0->sym("memory_alloc");
void* ptr = ((void*(*)(size_t))malloc_func)(1024);
```

## 错误处理

### 1. 模块状态

- `MODULE_UNLOADED` - 已注册但未加载
- `MODULE_LOADING` - 正在加载中
- `MODULE_READY` - 已加载完成
- `MODULE_ERROR` - 加载出错

### 2. 错误信息

每个模块维护最后的错误信息：

```c
if (module->state == MODULE_ERROR) {
    printf("模块错误: %s\n", module->error);
}
```

## 性能优化

### 1. 符号缓存

模块管理器使用哈希表缓存符号解析结果，避免重复查找。

### 2. 内存池

不同类型的内存分配使用专门的内存池，提高分配效率。

### 3. 懒加载

模块支持按需加载，减少启动时间。

## 扩展性

### 1. 新模块添加

1. 实现 `Module` 接口
2. 添加符号导出表
3. 使用 `REGISTER_MODULE` 注册
4. 声明依赖关系

### 2. 模块升级

支持运行时模块替换，实现热更新功能。

### 3. 跨平台支持

模块系统设计为跨平台，支持不同架构和操作系统。

## 总结

Self-Evolve AI 的模块化设计提供了：

- **统一的模块接口** - 所有模块遵循相同规范
- **灵活的依赖管理** - 支持复杂的模块依赖关系
- **高效的符号解析** - 缓存机制提高性能
- **完整的生命周期管理** - 从注册到清理的全过程管理
- **强大的扩展能力** - 易于添加新模块和功能

这个模块化架构为系统的自我进化提供了坚实的基础，支持运行时的模块替换和升级，是实现通用人工智能的重要技术基础。 