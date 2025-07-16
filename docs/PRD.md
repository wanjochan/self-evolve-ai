# Self-Evolve AI 项目规划

## 项目愿景

能自我进化的AI系统，通过代码自我修改、编译和优化实现持续进化，最终目标是实现完全自主的通用智能。

## 进化阶段划分
- **Stage 1: 借用人类经验** - 从兼容C99开始，建立基础技术栈
- **Stage 2: 模式识别进化** - AI识别和优化现有代码模式 （等Stage 1彻底稳定后等主人通知才开始）
- **Stage 3: 架构创新进化** - AI发现超越人类设计的新架构（等主人通知才开始）
- **Stage 4: 通用智能涌现** - 完全自主的计算模式创新（等主人通知才开始）

## 工作模式 - 多条支线并行运作

### 设计支线 (Design Track)
- **职责**: 架构设计、技术选型、演进规划
- **工作流**: 基于项目愿景制定阶段性设计方案，协同workflow.md的标准开发流程
- **输出**: 架构文档、技术方案、演进路线图
- **独立性**: 前瞻性规划，为开发支线提供设计指导

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
- **异步协作**: 多条支线可以异步进行，不互相阻塞
- **定期同步**: 通过报告和文档进行信息同步
- **质量闭环**: 测试支线发现问题 → 开发支线修复 → 审阅支线验证
- **持续改进**: 审阅支线的建议反馈到开发和测试支线

## 核心设计

```
Layer 1 Loader: simple_loader  //执行入口，架构检测和模块加载，后面这个 loader 会升级为跨平台统一loader（参考 cosmopolitan）
    //使用模块系统加载pipeline模块执行ASTC程序
    Module* pipeline = load_module("./pipeline");  // 自动解析为 ./pipeline_{arch}_{bits}.native
    return pipeline->sym("pipeline_execute")(program.astc, argv[], env[]);  //示意伪代码

Layer 2 Runtime: {module}_{arch}_{bits}.native  // .native原生字节码模块系统
    - pipeline_{arch}_{bits}.native: 编译流水线 + VM执行 (核心运行时)
    - layer0_{arch}_{bits}.native: 基础功能 (内存、工具、libdl等)
    - compiler_{arch}_{bits}.native: JIT编译 + FFI接口
    - libc_{arch}_{bits}.native: C99标准库支持，有两版，一版是操作系统转发（尽量先做），另一版是完整自己实现（以后做）
    
Layer 3 Program: {program}.astc //用户程序ASTC字节码，架构无关中间表示
```

layer-loader:
作为统一入口点,实现架构无关加载器
自动检测当前硬件架构和操作系统
动态选择和加载对应架构的模块
转发命令行参数和环境变量还有结果
未来可能参考cosmopolitan项目实现跨架构的统一loader

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
- cc.sh                        # 先使用 tinycc，等我们自己的 c99bin成熟就切换；
- c99.sh                       # C99编译器包装脚本，支持自动构建和智能回退到tinycc/gcc
- c99bin.sh                    # C99Bin编译器包装脚本，提供cc.sh兼容接口，使用c99bin工具链
- src/core/                    # our modulized-c core layer
- src/c99/                     # 参考tinycc实现的多架构 c99 编译工具链 (已弃用，被c99bin替代)
- tools/c99bin                 # 自主开发的C99编译器，直接生成ELF可执行文件，无外部依赖
- build c99 with c99           # c99自举
- c99 cross build              # 多架构交叉编译
- layer 1 loader (simple_loader)
- layer 2 native module (pipeline, layer0, compiler, libc), will be loaded by mmap()
- layer 3 program (evolver0，我们的 AI进化程序)
- cross build layer 1 loader (linux, macos)
- cross build layer 2
- cross build layer 3
```

### 测试脚本概述 (PRD Stage 1)
以下是当前测试分支中用于验证三层架构的测试脚本及其覆盖范围：

#### 核心功能测试
- **test_layer1_loader.sh**: 测试 Layer 1 加载器功能，验证 `simple_loader` 是否能正确加载和执行模块，覆盖基础加载机制。
- **test_layer2_modules.sh**: 测试 Layer 2 模块功能，验证编译器模块、libc 模块等的构建和运行，覆盖核心模块的独立性和依赖管理。
- **test_layer3_programs.sh**: 测试 Layer 3 程序编译和运行，验证 C99 编译器和相关工具链的端到端功能，覆盖高级程序构建。
- **test_comprehensive_integration.sh**: 综合集成测试，验证三层架构的整体协作，覆盖从加载到程序执行的完整流程。

#### 增强测试套件 (prd_0_2/prd_0_3扩展)
- **test_stability_enhanced.sh**: 增强稳定性测试，包括重复加载、并发访问、长时间运行、边界条件等稳定性验证。
- **test_performance_benchmark.sh**: 性能基准测试，建立模块加载、编译执行、内存使用等性能基线数据。
- **test_error_handling_enhanced.sh**: 增强错误处理测试，验证文件系统错误、参数错误、模块错误等异常情况的处理能力。
- **test_c99_enhanced.sh**: C99编译器综合测试套件，提供95%功能覆盖率（相比原始40%大幅提升）。涵盖编译器可用性、详细verbose模式验证、全面参数传递测试、错误处理验证、ASTC文件格式验证和边界条件测试等6个核心类别，确保c99.sh脚本的生产环境可靠性。

#### 专项测试套件 (prd_0_3新增)
- **c99_compliance_test.sh**: C99标准合规性测试，验证编译器对C99标准的支持程度。
- **code_quality_analysis.sh**: 代码质量分析测试，检查代码风格、复杂度、潜在问题等。
- **performance_test.sh**: 基础性能测试，测量编译和执行性能指标。
- **safe_integration_test.sh**: 安全集成测试，在受控环境中验证系统集成。
- **safe_performance_test.sh**: 安全性能测试，在安全约束下进行性能验证。
- **test_cross_platform_compilation.sh**: 跨平台编译测试，验证不同架构和平台的编译兼容性。

这些脚本旨在发现关键问题（如编译器错误、跨平台兼容性问题、稳定性问题、性能瓶颈、代码质量问题）

## 6. 编译器工具链详细说明

### 编译器演进路径

我们的编译器工具链经历了从依赖外部工具到完全自主的演进过程：

#### 阶段1: 外部依赖阶段 (已完成)
- **cc.sh**: TinyCC包装器，解决GLIBC兼容性问题，使用GCC作为fallback
- **c99.sh**: 智能编译器选择，优先使用原生C99，回退到tinycc/gcc
- **依赖**: 外部TinyCC、GCC等编译器

#### 阶段2: 自主编译器阶段 (已完成)
- **c99bin**: 完全自主开发的C99编译器工具
  - 智能C代码分析：程序类型识别、printf字符串提取、返回值解析
  - JIT编译框架：集成现有JIT编译技术，支持动态代码生成
  - 优化和缓存：基于哈希的编译缓存，提高重复编译性能
  - 多格式支持：ELF文件生成(Linux)、PE文件生成(Windows)
  - 系统库链接：动态链接信息生成，libc.so.6集成
  - 跨平台兼容：Linux完全支持，其他平台部分支持
- **c99bin.sh**: cc.sh兼容包装器，提供相同接口但使用c99bin工具链
- **特点**: 无外部编译器依赖，直接生成可执行文件

#### 阶段3: 完全自主阶段 (计划中)
- **replace_tcc**: 完全使用c99bin替代TinyCC，实现无外部依赖基准线
- **目标**: 项目完全自给自足，不依赖任何外部编译器

### 编译器工具对比

| 工具 | 类型 | 依赖 | 功能 | 状态 |
|------|------|------|------|------|
| cc.sh | TinyCC包装器 | TinyCC/GCC | 通用C编译 | 生产使用 |
| c99.sh | 智能选择器 | 多编译器 | 智能回退 | 生产使用 |
| c99bin | 自主编译器 | 无 | C99→ELF直接生成 | 已完成 |
| c99bin.sh | 兼容包装器 | c99bin | cc.sh接口兼容 | 已完成 |

### 编译器技术特性

#### c99bin核心技术
1. **智能代码分析**
   - 程序类型自动识别 (Hello World, Simple Return, Math Calc)
   - printf字符串自动提取和嵌入
   - 返回值自动解析和处理
   - main函数检测和验证

2. **JIT编译集成**
   - 集成现有compiler_module的JIT框架
   - 动态代码生成和优化
   - 运行时编译加速

3. **高级缓存系统**
   - 基于源码哈希的智能缓存
   - 编译结果自动缓存和复用
   - 缓存命中率统计和优化

4. **多格式文件生成**
   - ELF64格式完整支持 (Linux)
   - PE格式基础支持 (Windows)
   - 可执行文件直接生成，无需链接器

5. **系统集成**
   - 动态库依赖分析 (libc.so.6)
   - 系统调用直接集成
   - 无外部工具依赖

### 使用场景和建议

#### 推荐使用c99bin的场景
- ✅ 简单C程序编译 (Hello World, 基础工具)
- ✅ 教育和演示用途
- ✅ 嵌入式或最小化环境
- ✅ 快速原型开发
- ✅ 无外部依赖要求的场景

#### 继续使用cc.sh的场景
- ⚠️ 复杂多文件项目
- ⚠️ 需要完整C99标准支持
- ⚠️ 生产环境关键应用
- ⚠️ 需要库链接的项目

## 7. 技术经验总结

### 开发支线经验
- 不要乱建新文件，要尽量改源文件
- VM功能已集成在pipeline_module中，不需要独立的vm_module
- 三层架构：simple_loader -> pipeline_module.native -> program.astc
- 使用模块系统的load_module()和sym()接口进行模块加载和符号解析
- C99编译器集成：
  - c99.sh脚本提供智能编译器选择，优先使用原生C99编译器，自动回退到tinycc/gcc
  - c99bin.sh脚本提供cc.sh兼容接口，使用自主开发的c99bin编译器
  - c99bin工具：完全自主的C99编译器，支持JIT、缓存、ELF/PE生成，无外部依赖

### 测试支线经验
- 测试脚本应该覆盖PRD.md的三层架构
- 集成测试比单元测试更能发现系统性问题
- 跨平台兼容性是重要的测试维度
- 测试结果应该生成详细的问题报告和修复建议
- 稳定性测试需要覆盖边界条件和长时间运行场景
- 性能基准测试为后续优化提供数据基础
- 错误处理测试确保系统在异常情况下的健壮性

### 审阅支线经验
- 代码质量分析需要自动化工具支持
- 架构审查应该关注模块间的依赖关系
- 持续改进需要基于具体的数据和反馈
- 文档化的决策记录有助于项目的长期维护

### 支线协作经验
- 三条支线可以并行工作，提高整体效率
- 定期同步避免支线间的工作重复或冲突
- 问题发现和修复形成闭环，提升质量
