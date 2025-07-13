# Self-Evolve AI 项目规划

## 项目愿景

能自我进化的AI系统，通过代码自我修改、编译和优化实现持续进化，最终目标是实现完全自主的通用智能。

## 进化阶段划分
- **Stage 1: 借用人类经验** - 从兼容C99开始，建立基础技术栈
- **Stage 2: 模式识别进化** - AI识别和优化现有代码模式 （等Stage 1彻底稳定后等主人通知才开始）
- **Stage 3: 架构创新进化** - AI发现超越人类设计的新架构（等主人通知才开始）
- **Stage 4: 通用智能涌现** - 完全自主的计算模式创新（等主人通知才开始）

## 工作模式 - 三条支线独立运作

### 开发支线 (Development Track)
- **职责**: 核心功能开发、架构设计、代码实现
- **工作流**: 基于workflow.md的标准开发流程
- **输出**: 源代码、模块、工具链、技术文档
- **独立性**: 专注于功能实现，不依赖其他支线的完成状态

### 测试支线 (Testing Track)  
- **职责**: 测试脚本开发、质量验证、问题发现
- **工作流**: 基于PRD.md进行测试覆盖度评估和测试脚本补全
- **输出**: 测试脚本、测试报告、问题清单、质量评估
- **独立性**: 可以并行于开发支线进行，提供持续的质量保障

### 审阅支线 (Review Track)
- **职责**: 代码审查、架构评估、持续改进建议
- **工作流**: 定期审计开发支线和测试支线的输出
- **输出**: 审查报告、改进建议、最佳实践、决策记录
- **独立性**: 基于开发和测试支线的输出进行评估，提供客观反馈

### 支线协作机制
- **异步协作**: 三条支线可以异步进行，不互相阻塞
- **定期同步**: 通过报告和文档进行信息同步
- **质量闭环**: 测试支线发现问题 → 开发支线修复 → 审阅支线验证
- **持续改进**: 审阅支线的建议反馈到开发和测试支线

## 核心设计

```
Layer 1 Loader: simple_loader  //执行入口，架构检测和模块加载
    //使用模块系统加载pipeline模块执行ASTC程序
    Module* pipeline = load_module("./pipeline");  // 自动解析为 pipeline_{arch}_{bits}.native
    return pipeline->sym("pipeline_execute")(program.astc, argv[], env[]);  //示意伪代码

Layer 2 Runtime: {module}_{arch}_{bits}.native  // .native原生字节码模块系统
    - pipeline_{arch}_{bits}.native: 编译流水线 + VM执行 (核心运行时)
    - layer0_{arch}_{bits}.native: 基础功能 (内存、工具、libdl)
    - compiler_{arch}_{bits}.native: JIT编译 + FFI接口
    - libc_{arch}_{bits}.native: C99标准库支持
    
Layer 3 Program: {program}.astc //用户程序ASTC字节码，架构无关中间表示
    c99:
        return c99_compile(c_file_name, argv[])
    evolver0:
        return evolve() //基于c99进入stage2的开发,TODO
```

layer-loader:
作为统一入口点,实现架构无关加载器
自动检测当前硬件架构和操作系统
动态选择和加载对应架构的模块
转发命令行参数和环境变量还有结果
未来可以参考cosmopolitan项目实现跨架构的统一loader

layer-runtime:
模块化原生字节码系统,针对特定架构 arch+bits
加载和执行ASTC字节码,转发参数和环境还有结果
//重点核心模块架构 (当前实现):
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
- **模块化架构**: 5个核心模块提供完整功能栈
  - pipeline_module: c2astc, codegen, astc2native, vm执行 (核心)
  - compiler_module: jit编译, ffi接口
  - layer0_module: 基础服务 (内存、工具、标准库、动态加载)
  - libc_module: C99标准库支持
  - module_module: 模块管理器 (自举)
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

注：我们在底层设计了模块化，所以目前底层是 modulized-c，兼容 c99 语法，除非以后进化出更高级的底层之前都以这个为底层，而不是依赖libc（虽然我们模块化c 的底层目前也是libc）

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

#### 3. Pipeline编译流水线模块 (pipeline_module.c)

**作用**: 系统的核心编译和执行模块，整合了完整的编译执行流水线。

**核心功能**:
- **Frontend**: C源码词法分析和语法分析 (c2astc)
- **Backend**: AST转汇编代码 (codegen) + AOT编译 (astc2native)
- **Execution**: 虚拟机执行ASTC字节码 (astc + vm)
- **统一接口**: 编译和执行的一站式服务

**关键API**:
- `pipeline_compile()` - 编译C源码
- `pipeline_execute()` - 执行字节码
- `pipeline_compile_and_run()` - 编译并执行
- `pipeline_get_assembly()` - 获取汇编代码
- `pipeline_get_bytecode()` - 获取字节码
- `pipeline_astc2native()` - AOT编译

#### 4. Compiler编译器集成模块 (compiler_module.c)

**作用**: 提供特殊编译方式的集成，整合了jit+ffi功能。

**核心功能**:
- **JIT编译**: 即时编译ASTC字节码为原生机器码
- **FFI接口**: 外部函数接口，类似libffi
- **优化器**: 代码优化和性能调优
- **调试支持**: 调试信息生成和管理

#### 5. LibC标准库模块 (libc_module.c)

**作用**: 提供C99标准库支持，独立的标准库实现。

**核心功能**:
- **标准I/O**: printf, scanf, fopen等
- **内存管理**: malloc, free, realloc等
- **字符串处理**: strlen, strcpy, strcmp等
- **数学函数**: sin, cos, sqrt等
- **系统调用**: 操作系统接口封装

### 模块依赖关系

```
module_module (核心管理器，无依赖)
    ↓
layer0_module (基础服务层)
    ↓
pipeline_module (编译流水线 + VM执行) ← 依赖 layer0
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

### Stage 1 三条支线路线图

#### 开发支线 (Development Track)
```
dev roadmap (by human master)
- src/core/                    # our modulized system core layer
- layer 1 loader (simple_loader)
- layer 2 native module (pipeline, layer0, compiler, libc), will be loaded by mmap() 
- layer 3 program (c99 windows 64 x86)
- build tcc with c99 // test c99 working good
- layer 3 program c99 supports cross build
- cross build layer 1 loader (linux, macos)
- cross build layer 2 vm (arm, riscv, mips, etc.)
- build loader2 with c99 (then start to be free from tinycc)
```

#### 测试支线 (Testing Track)
```
test roadmap (continuous quality assurance)
- 建立完整的测试脚本体系 ✅ (prd_0_1_0 已完成)
- Layer 1 Loader测试覆盖 ✅
- Layer 2 Native Module测试覆盖 ✅  
- Layer 3 Program测试覆盖 ✅
- 集成测试和端到端测试 ✅
- 性能基准测试建立 ✅
- 错误处理测试完善 ✅
- 跨平台兼容性测试 (进行中)
- 自动化测试流水线建立 (待开始)
```

#### 审阅支线 (Review Track)
```
review roadmap (continuous improvement)
- 代码质量分析框架建立 ✅
- 架构设计审查机制 (待建立)
- 开发流程优化建议 (待建立)
- 测试覆盖度评估 ✅ (prd_0_1_0 已完成)
- 技术债务识别和管理 (待建立)
- 最佳实践文档化 (待建立)
- 决策记录和经验总结 (待建立)
```

## 6. 技术经验总结

### 开发支线经验
- 不要乱建新文件，要尽量改源文件
- VM功能已集成在pipeline_module中，不需要独立的vm_module
- 三层架构：simple_loader -> pipeline_module.native -> program.astc
- 使用模块系统的load_module()和sym()接口进行模块加载和符号解析

### 测试支线经验
- 测试脚本应该覆盖PRD.md的三层架构
- 集成测试比单元测试更能发现系统性问题
- 跨平台兼容性是重要的测试维度
- 测试结果应该生成详细的问题报告和修复建议

### 审阅支线经验
- 代码质量分析需要自动化工具支持
- 架构审查应该关注模块间的依赖关系
- 持续改进需要基于具体的数据和反馈
- 文档化的决策记录有助于项目的长期维护

### 支线协作经验
- 三条支线可以并行工作，提高整体效率
- 定期同步避免支线间的工作重复或冲突
- 问题发现和修复形成闭环，提升质量
