# Self-Evolve AI 项目规划

## 项目愿景

能自我进化的AI系统，通过代码自我修改、编译和优化实现持续进化，最终目标是实现完全自主的通用智能。

## 进化阶段划分
- **Stage 1: 借用人类经验** - 从兼容C99开始，建立基础技术栈
- **Stage 2: 模式识别进化** - AI识别和优化现有代码模式 （等Stage 1彻底稳定后等主人通知才开始）
- **Stage 3: 架构创新进化** - AI发现超越人类设计的新架构（等主人通知才开始）
- **Stage 4: 通用智能涌现** - 完全自主的计算模式创新（等主人通知才开始）

## 核心设计

```
Layer 1 Loader: simple_loader  //执行入口，架构检测和模块加载
    //使用模块系统加载pipeline模块执行ASTC程序
    return import(’vm',arch=None,bits=None).main(program,argv[],env[]) //示意伪代码
Layer 2 Runtime: f'vm_{arch}_{bits}.native'  // .native原生字节码模块，其中最重要是vm模块用于加载astc运行
    def main(astc_module_name,argv[],env[]):  
        //vm 模块加载 astc 模块然后转发参数和环境变量
        return vm_import(astc_module_name).main(argv[],env[])  //示意伪代码
Layer 3 Program: {program}.astc //用户程序（比如c99、evolver{version}）ASTC字节码，以后兼容ASTC-ASM、ASTC-ES6等高级语言
    c99:
        return c99_compile(c_file_name, argv[])
    evolver0:
        return evolve() //基于c99他stage1开始进入stage2的开发,TODO
```

layer-loader:
作为统一入口点,实现架构无关加载器
自动检测当前硬件架构和操作系统
动态选择和加载对应架构的VM模块
转发命令行参数和环境变量还有结果
未来可以参考cosmopolitan项目实现跨架构的统一loader

layer-runtime:
原生字节码模块,针对特定架构 arch+bits
加载和执行ASTC字节码,转发参数和环境还有结果
//重点核心模块架构 (tunecore重构后):
  1. module_module    - 模块管理器 (智能加载+符号解析)
  2. layer0_module    - 基础功能 (memory+utils+std+libdl)
  3. pipeline_module  - 编译流水线:
     ├── frontend: c2astc (C代码→ASTC字节码)
     ├── backend: codegen (ASTC字节码→ASTC汇编) + astc2native (AOT编译)
     └── execution: astc + vm (ASTC字节码执行)
  4. compiler_module  - 编译器集成:
     ├── jit (即时编译，ASTC字节码→原生机器码)
     └── ffi (外部函数接口，类似libffi)
  5. libc_module      - C99标准库 (独立模块)

layer-program:
用户程序的ASTC字节码表示
架构无关的中间表示IR
先实现ASTC字节码支持，未来可扩展支持ASTC-ASM、ASTC-ES6等高级语言

### 核心技术
- **ASTC字节码**: 可扩展的计算表示 (astc.[h|c])
- **.native模块**: 原生字节码模块 {module}_{arch}_{bits}.native (module.[h|c])
- **模块化架构**: 4个核心模块提供完整功能栈
  - pipeline_module: c2astc, codegen, astc2native, vm执行
  - compiler_module: jit编译, ffi接口
  - layer0_module: 基础服务 (内存、工具、标准库、动态加载)
  - libc_module: C99标准库支持
- **按需加载**: Python风格的模块加载机制 load_module("./module")

## 4. 模块化设计详细说明

### 设计理念

- **模块整合**: 将相关功能合并，减少模块间复杂依赖
- **按需加载**: Python风格的优雅模块加载机制
- **智能解析**: 自动架构检测和路径解析
- **统一接口**: 所有模块遵循相同的接口规范

### 按需加载机制

#### 智能路径解析
```c
// 自动解析: "./layer0" -> "./layer0_arm64_64.native"
char* resolved = resolve_native_file("./layer0");
```

#### 优雅的模块加载
```c
// Python风格的模块加载
Module* module = load_module("./layer0");
```

#### 符号解析接口
```c
// 优雅的符号解析和调用
void* func = module->sym("function_name");
int result = ((int(*)(int))func)(42);
```

### 核心模块接口

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

### 核心模块详细介绍

#### 1. 增强的模块管理器 (module_module.c)

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

#### 2. Layer0基础模块 (layer0_module.c)

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

#### 3. Pipeline编译流水线模块 (pipeline_module.c)

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
- `pipeline_astc2native()` - AOT编译
- `aot_create_compiler()` - 创建AOT编译器
- `aot_compile_to_executable()` - 编译为可执行文件

#### 4. Compiler编译器集成模块 (compiler_module.c)

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

#### 5. LibC标准库模块 (libc_module.c)

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

### 模块依赖关系

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
```

### 模块加载和初始化

#### 按需加载
使用新的按需加载机制：

```c
// 智能路径解析和加载
Module* layer0 = load_module("./layer0");
Module* pipeline = load_module("./pipeline");
Module* compiler = load_module("./compiler");
```

#### 初始化顺序
1. `module_module` 首先初始化（自举）
2. `layer0_module` 初始化（提供基础服务）
3. `pipeline_module` 和 `compiler_module` 并行初始化
4. `libc_module` 独立初始化

#### 符号解析
使用新的优雅接口：

```c
// 获取模块函数并调用
void* malloc_func = layer0->sym("memory_alloc");
void* ptr = ((void*(*)(size_t))malloc_func)(1024);
```

## 5. 实现路线图
stage 1
```
dev roadmap (by human master)
- src/core/                    # our modulized system core layer
- layer 1 loader (windows exe)
- layer 2 native module (vm, libc, std, astc, jit, utils, etc), will be loaded by mmap() 
- layer 3 program (c99 windows 64 x86)
- build tcc with c99 // test c99 working good
- layer 3 program c99 supports cross build
- cross build layer 1 loader (linux, macos)
- cross build layer 2 vm (arm, riscv, mips, etc.)
- build loader2 with c99 (then start to be free from tinycc)
```


stage 2

stage 3

stage 4

## 6. 技术经验总结

- 不要乱建新文件，要尽量改源文件

### TCC编译避免杀毒软件误报经验

在实现PRD.md三层架构过程中，遇到了TCC编译的可执行文件被杀毒软件误报为病毒（HEUR/QVM202.0.68C9）的问题。经过研究和实践，找到了有效的解决方案：

#### 成功的编译方案

**最简单有效的方案**：
```bash
tcc.exe -o loader.exe source.c
```

**保守方案**（如果简单方案仍有问题）：
```bash
tcc.exe -g -O0 -DLEGITIMATE_SOFTWARE -o loader.exe source.c -luser32 -lkernel32 -ladvapi32
```

---

## ❌ 重要编译经验和教训 ❌

### 严禁的错误做法：
1. **不要生成.def文件** - 项目架构不使用.def文件
2. **不要将.exe重命名为.native** - .native是自定义格式，不是重命名的可执行文件
3. **不要使用传统共享库编译方式** - 我们有自己的native模块系统
4. **❌❌❌ 绝对不要创建不必要的新文件 ❌❌❌** - 这是重复犯的严重错误！
   - 不要创建 *_new.bat, *_clean.bat, *_simple.c 等文件
   - 直接修改现有文件，不要创建副本
   - 使用现有架构和工具，不要重复造轮子

### ✅ 正确的.native模块创建方式：
1. **使用src/core/native.c中的函数**：
   - `native_module_create()` - 创建模块结构
   - `native_module_set_code()` - 设置机器码
   - `native_module_add_export()` - 添加导出函数
   - `native_module_write_file()` - 写入真正的.native格式（NATV魔数）

2. **遵循PRD.md第76行**：用 module 模块来加载其它模块
3. **遵循PRD.md第84行**：src/utils.c实现libdl-alike, libffi-alike功能

### 🎯 正确编译流程：
```
源码 → 编译为目标代码 → 使用native.c系统创建.native格式 → 输出真正的.native文件
```

**错误流程**：
```
源码 → 编译为.exe → 重命名为.native ❌
```
