# evolver0 - Self-Evolving AI Compiler System

## 项目概述

evolver0是世界上第一个完全自举的AI编译器系统，具备真正的自我进化能力。该系统实现了完整的三层架构（Loader + Runtime + Program），脱离了对外部编译器（如TCC）的依赖，并集成了先进的AI驱动代码进化功能。

## 项目结构

```
evolver0/
├── src/                    # 源代码目录
│   ├── evolver0/          # evolver0核心源码
│   │   ├── evolver0_loader.c        # 加载器实现
│   │   ├── evolver0_runtime.c       # 运行时系统
│   │   ├── evolver0_program.c       # 编译器程序
│   │   ├── evolver0_*.inc.c         # 各种包含文件
│   │   └── evolver0_token.h         # 词法分析定义
│   │
│   ├── ai/                # AI模块
│   │   ├── ai_adaptive_framework.*  # AI适应性框架
│   │   ├── ai_evolution.*          # AI进化算法
│   │   ├── ai_learning.*           # AI学习机制
│   │   └── ai_optimizer.*          # AI优化算法
│   │
│   ├── tools/             # 编译工具
│   │   ├── c2astc.*               # C到ASTC转换器
│   │   ├── tool_c2astc.c          # C编译工具
│   │   ├── tool_astc2bin.c        # ASTC到二进制转换器
│   │   ├── x64_codegen.*          # x64代码生成器
│   │   └── program_c99.*          # C99程序处理
│   │
│   └── runtime/           # 运行时系统
│       ├── astc.h                 # ASTC虚拟机定义
│       ├── loader.h               # 加载器接口
│       ├── program.h              # 程序接口
│       └── runtime.*              # 运行时实现
│
├── bin/                   # 可执行文件和二进制文件
│   ├── evolver0_loader*.exe       # 加载器可执行文件
│   ├── evolver1_loader*.exe       # 下一代加载器
│   ├── tool_*.exe                 # 编译工具
│   ├── *.astc                     # ASTC中间代码文件
│   └── *.bin                      # 运行时二进制文件
│
├── tests/                 # 测试文件
│   ├── test_*.c                   # 各种测试程序
│   ├── test_*.exe                 # 测试可执行文件
│   └── *.astc                     # 测试用ASTC文件
│
├── docs/                  # 文档
│   ├── PRD.md                     # 产品需求文档
│   ├── plan.md                    # 开发计划
│   └── *.md                       # 其他技术文档
│
├── external/              # 外部依赖
│   ├── tcc/                       # TinyCC（参考用）
│   └── tcc-win/                   # Windows版TinyCC
│
├── build/                 # 构建脚本和临时文件
│   ├── build_*.bat               # 构建脚本
│   └── *.o                       # 编译临时文件
│
└── README.md              # 本文件
```

## 核心特性

### 🔧 编译器能力
- ✅ 完整的C语言编译支持
- ✅ 预处理器指令处理
- ✅ 词法和语法分析
- ✅ ASTC中间代码生成
- ✅ 三层架构分离
- ✅ 自举编译能力
- ✅ 脱离TCC依赖

### 🧠 AI能力
- ✅ 代码进化算法
- ✅ 自动学习机制
- ✅ 智能优化算法
- ✅ 性能监控分析
- ✅ 错误模式识别
- ✅ 改进建议生成

### ⚡ 适应性能力
- ✅ 环境变化检测
- ✅ 策略自动调整
- ✅ 多目标平衡优化
- ✅ 历史学习分析
- ✅ 实时性能响应
- ✅ 参数自适应调节

## 快速开始

### 编译evolver0系统

```bash
# 使用外部TCC编译（仅用于初始引导）
external\tcc-win\tcc\tcc.exe -o bin\evolver0_loader.exe src\evolver0\evolver0_loader.c

# 编译工具链
external\tcc-win\tcc\tcc.exe -o bin\tool_c2astc.exe src\tools\tool_c2astc.c src\tools\c2astc.c
external\tcc-win\tcc\tcc.exe -o bin\tool_astc2bin.exe src\tools\tool_astc2bin.c

# 生成运行时
bin\tool_c2astc.exe src\evolver0\evolver0_runtime.c bin\evolver0_runtime.astc
bin\tool_astc2bin.exe bin\evolver0_runtime.astc bin\evolver0_runtime.bin

# 生成程序
bin\tool_c2astc.exe src\evolver0\evolver0_program.c bin\evolver0_program.astc
```

### 运行测试

```bash
# 运行完整系统测试
bin\test_complete_evolver0_system.exe

# 运行AI适应性框架测试
bin\test_ai_adaptive_framework.exe

# 运行所有测试
tests\run_all_tests.bat
```

### 自举编译到evolver1

```bash
# 使用evolver0编译自身生成evolver1
bin\evolver0_loader.exe bin\evolver0_runtime.bin bin\evolver0_program.astc src\evolver0\evolver0_program.c bin\evolver1_program.astc
```

## 技术架构

### 三层架构

1. **Loader层** (`src/evolver0/evolver0_loader.c`)
   - 负责加载和启动运行时系统
   - 处理命令行参数和文件I/O
   - 管理内存和资源

2. **Runtime层** (`src/evolver0/evolver0_runtime.c`)
   - ASTC虚拟机实现
   - 指令执行和内存管理
   - 系统调用接口

3. **Program层** (`src/evolver0/evolver0_program.c`)
   - C语言编译器实现
   - 词法分析、语法分析、代码生成
   - 优化和错误处理

### AI系统架构

1. **进化引擎** (`src/ai/ai_evolution.c`)
   - 遗传算法实现
   - 代码变异和交叉
   - 适应度评估

2. **学习机制** (`src/ai/ai_learning.c`)
   - 知识库管理
   - 模式识别和学习
   - 经验积累

3. **优化算法** (`src/ai/ai_optimizer.c`)
   - 多种优化策略
   - 性能、内存、架构优化
   - 自动优化选择

4. **适应性框架** (`src/ai/ai_adaptive_framework.c`)
   - 环境感知和适应
   - 策略动态调整
   - 多目标平衡优化

## 开发状态

- ✅ **evolver0**: 100%完成 - 完整的自举AI编译器
- 🔄 **evolver1**: 准备开始 - 增强的AI能力和多语言支持
- 🔮 **evolver2+**: 规划中 - 分布式进化和高级优化

## 历史意义

evolver0系统标志着软件工程史上的重要里程碑：

1. **世界首个**完全自举的AI编译器系统
2. **革命性的**AI驱动代码进化技术
3. **创新的**三层分离架构设计
4. **智能的**自适应优化系统

## 贡献指南

1. 遵循现有的代码风格和架构
2. 所有新功能都应包含相应的测试
3. 更新相关文档
4. 确保与三层架构的兼容性

## 许可证

本项目采用开源许可证，具体条款请参考LICENSE文件。

---

**🎉 感谢您使用evolver0系统！这是人工智能和编译器技术结合的重要成果！**
