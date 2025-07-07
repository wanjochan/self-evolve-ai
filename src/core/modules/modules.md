# Self-Evolve AI 模块化设计文档

## 概述

本文档描述了 Self-Evolve AI 项目的模块化设计架构。该系统基于三层架构（Layer 1 Loader、Layer 2 Runtime、Layer 3 Program）构建，其中模块系统是 Layer 2 Runtime 的核心组成部分。

## 设计理念

- **极简主义**: 每个模块职责单一、接口清晰
- **高度灵活性**: 模块间松耦合，支持动态加载/卸载
- **自我进化能力**: 支持运行时模块替换和升级
- **统一接口**: 所有模块遵循相同的接口规范

## 系统架构

```
Layer 1: Loader (loader_{arch}_{bits}.exe)
    ↓ 加载
Layer 2: Runtime (vm_{arch}_{bits}.native + 其他模块)
    ├── 模块管理器 (module_module)
    ├── 虚拟机模块 (vm_module)     //加载 astc 模块，在内存中转成原生字节并运行
    ├── 内存管理模块 (memory_module)
    ├── ASTC模块 (astc_module)  // astc bytecode data structure
    ├── 编译器模块 (c2astc_module)  //convert c source code to astc bytecode
    ├── 代码生成模块 (codegen_module)  // AST to ASM
    ├── JIT编译模块 (jit_module)
    ├── 原生模块系统 (native_module)
    ├── 标准库模块 (std_module)
    ├── LibC模块 (libc_module)
    ├── 工具模块 (utils_module)
    └── ASTC转换模块 (astc2native_module)  //ASTC bytecode to native bytecode, is aot!
    ↓ 执行
Layer 3: Program ({program}.astc)
```

注意，准备调整：
```
1. module     - 模块管理器, for layer 1 loader
2. layer0     - std + memory + utils, for layer 0 (see PRD.md)；add libdl later
3. libc       - C标准库转发, for c99 (our replacement to cc)
4. pipeline/   - for layer 2 runtime
   ├── frontend           - c2astc
   ├── backend            - codegen + astc2native
   ├── compiler           - jit + aot + ffi
   └── execution          - astc + vm
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

## 模块详细介绍

### 1. 模块管理器 (module_module.c)

**作用**: 系统的第一个模块，管理所有其他模块的生命周期。

**核心功能**:
- 模块注册和发现
- 依赖关系管理
- 符号解析和缓存
- 模块加载/卸载
- 生命周期管理

**关键API**:
- `module_register()` - 注册模块
- `module_load()` - 加载模块
- `module_resolve()` - 解析符号
- `modules_init_all()` - 初始化所有模块
- `modules_cleanup_all()` - 清理所有模块

**特点**: 
- 作为"上帝模块"，自己管理自己
- 支持最多64个模块
- 提供符号缓存机制提高性能
- 支持模块依赖关系管理

### 2. 内存管理模块 (memory_module.c)

**作用**: 提供统一的内存管理服务，支持多种内存池。

**核心功能**:
- 多内存池管理（通用、字节码、JIT、模块、临时、C99相关）
- 内存分配/释放/重分配
- 内存统计和监控
- 内存泄漏检测

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
- `memory_alloc_pool()` - 从指定池分配
- `memory_get_stats()` - 获取统计信息

### 3. ASTC模块 (astc_module.c)

**作用**: 提供抽象语法树编译器（ASTC）核心功能。

**核心功能**:
- AST节点创建和管理
- AST遍历和操作
- ASTC程序结构管理
- 程序验证和序列化

**支持的节点类型**:
- 基本表达式（常量、标识符、二元/一元操作）
- 控制结构（if、while、for）
- 函数和变量声明
- 复合语句
- 模块系统节点

**关键API**:
- `ast_create_node()` - 创建AST节点
- `ast_free()` - 释放AST
- `ast_print()` - 打印AST
- `astc_create_program()` - 创建ASTC程序
- `astc_validate_program()` - 验证程序

### 4. 虚拟机模块 (vm_module.c)

**作用**: 提供ASTC字节码的执行环境。

**核心功能**:
- 虚拟机上下文管理
- 字节码加载和执行
- 指令集实现
- 运行时状态管理
- 调试和性能统计

**指令集**:
- 算术运算（ADD、SUB、MUL、DIV）
- 内存操作（LOAD、STORE）
- 控制流（JUMP、CALL、RETURN）
- 系统调用（LIBC_CALL）

**关键API**:
- `vm_create_context()` - 创建VM上下文
- `vm_load_program()` - 加载程序
- `vm_execute()` - 执行程序
- `vm_step()` - 单步执行
- `vm_get_stats()` - 获取统计信息

### 5. C到ASTC编译器模块 (c2astc_module.c)

**作用**: 将C语言源代码转换为ASTC字节码。

**核心功能**:
- C语言词法分析
- 语法分析
- AST构建
- 字节码生成
- 错误处理和诊断

**支持的C语言特性**:
- 基本数据类型（int、float、char等）
- 控制结构（if、while、for）
- 函数定义和调用
- 变量声明和赋值
- 结构体和数组
- 模块系统扩展

**关键API**:
- `c2astc_convert()` - 转换源代码
- `c2astc_convert_file()` - 转换文件
- `c2astc_default_options()` - 默认选项

### 6. 代码生成模块 (codegen_module.c)

**作用**: 将AST转换为目标平台的汇编代码。

**核心功能**:
- 多架构代码生成
- 汇编代码输出
- 寄存器分配
- 指令选择

**支持的架构**:
- x86_64
- ARM64
- x86_32
- ARM32

**关键API**:
- `codegen_create()` - 创建代码生成器
- `codegen_generate_function()` - 生成函数代码
- `codegen_get_assembly()` - 获取汇编代码

### 7. JIT编译模块 (jit_module.c)

**作用**: 提供即时编译功能，将字节码编译为机器码。

**核心功能**:
- 运行时编译
- 机器码生成
- 优化处理
- 可执行内存管理

**优化级别**:
- `JIT_OPT_NONE` - 无优化
- `JIT_OPT_BASIC` - 基础优化
- `JIT_OPT_AGGRESSIVE` - 激进优化

**关键API**:
- `jit_init_compiler()` - 初始化编译器
- `jit_compile_bytecode()` - 编译字节码
- `jit_execute()` - 执行编译后代码
- `jit_check_availability()` - 检查JIT可用性

### 8. 原生模块系统 (native_module.c)

**作用**: 管理.native格式的原生模块文件。

**核心功能**:
- .native文件格式处理
- 模块加载和链接
- 符号导出/导入
- 重定位处理
- 数字签名验证

**.native文件格式**:
- 魔数: "NATV" (0x5654414E)
- 版本控制
- 架构信息
- 代码和数据段
- 导出表
- 元数据

**关键API**:
- `native_module_create()` - 创建模块
- `native_module_write_file()` - 写入文件
- `native_module_load_file()` - 加载文件
- `native_module_add_export()` - 添加导出

### 9. 标准库模块 (std_module.c)

**作用**: 提供标准C库函数的实现。

**核心功能**:
- 内存管理函数（malloc、free等）
- 字符串处理函数（strlen、strcpy等）
- 输入输出函数（printf、puts等）
- 数学函数（sin、cos、sqrt等）
- 类型转换函数（atoi、atof等）

**关键API**:
- `std_malloc()` - 内存分配
- `std_printf()` - 格式化输出
- `std_strlen()` - 字符串长度
- `std_sin()` - 正弦函数

### 10. LibC模块 (libc_module.c)

**作用**: 提供完整的C标准库转发功能。

**核心功能**:
- 系统调用转发
- 文件I/O操作
- 内存管理增强
- 错误处理
- 数学函数库
- 字符串操作

**增强特性**:
- 内存使用统计
- 错误跟踪
- 安全检查
- 性能监控

**关键API**:
- `libc_fopen()` - 文件打开
- `libc_malloc_enhanced()` - 增强内存分配
- `libc_get_memory_stats()` - 获取内存统计
- `libc_strerror()` - 错误信息

### 11. 工具模块 (utils_module.c)

**作用**: 提供系统工具和辅助功能。

**核心功能**:
- 架构检测
- 平台检测
- 文件操作
- 可执行内存分配
- 时间和延时函数
- 错误输出

**架构支持**:
- x86_64、x86_32
- ARM64、ARM32
- 自动检测当前架构

**关键API**:
- `detect_architecture()` - 检测架构
- `allocate_executable_memory()` - 分配可执行内存
- `read_file_to_buffer()` - 读取文件
- `print_error()` - 错误输出

### 12. ASTC转换模块 (astc2native_module.c)

**作用**: 将ASTC字节码转换为原生机器码。

**核心功能**:
- 字节码解析
- 指令翻译
- 目标代码生成
- 优化处理

**支持的转换**:
- ASTC → x86_64 机器码
- ASTC → ARM64 机器码
- ASTC → x86_32 机器码

**关键API**:
- `astc2native_convert()` - 转换字节码
- `astc2native_optimize()` - 优化代码
- `astc2native_write_native()` - 写入原生模块

## 模块依赖关系

```
module_module (核心，无依赖)
    ↓
memory_module (被所有模块依赖)
    ↓
utils_module (基础工具)
    ↓
astc_module (依赖 memory)
    ↓
c2astc_module (依赖 memory, astc, utils)
    ↓
codegen_module (依赖 memory, astc, utils)
    ↓
jit_module (依赖 memory, utils)
    ↓
vm_module (依赖 memory)
    ↓
native_module (依赖 memory)
    ↓
std_module (独立实现)
    ↓
libc_module (独立实现)
    ↓
astc2native_module (依赖 memory, astc, utils, c2astc)
```

## 模块注册和初始化

### 1. 自动注册

每个模块使用 `REGISTER_MODULE` 宏自动注册：

```c
// 在模块文件末尾
REGISTER_MODULE(module_name);
```

### 2. 初始化顺序

1. `module_module` 首先初始化（自举）
2. `memory_module` 初始化（提供内存服务）
3. 其他模块按依赖顺序初始化
4. 调用 `modules_init_all()` 完成全部初始化

### 3. 符号解析

模块间通过符号名称进行通信：

```c
// 获取其他模块的函数
void* malloc_func = module_resolve(memory_module, "alloc");
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