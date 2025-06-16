# Self-Evolve AI 项目规划

## 1. 项目愿景

创建一个能够真正自我进化的AI系统，通过代码自我修改、编译和优化实现持续进化，最终目标是实现完全自主的智能系统。

## 来自人类的意见参考
自举编译和AI协同进化、跨硬件架构

本项目自进化范式：用Loader+Runtime+Program的结构，通过多代自举和AI驱动的进化，逐步构建出完整的自进化计算机软件系统

- Loader-{arch}.exe 负责加载Runtime-{arch} + Program.wasm，目前主要是操作系统内引导，未来不排除继续演化出操作系统。（注意，第零代的是tinycc编译的C程序loader0.c，之后进化出可执行程序后不再使用）
- Runtime 是架构依赖二进制，封装硬件架构和ABI等，实现WASM虚拟机。特别注意那些PE/MachO/ELF头不在Runtime而是在Loader上的了
- Program 是平台无关程序（WASM格式的二进制模块）

Runtime和Program两种编译的二进制是不一样的。Program编译的是.wasm（WebAssembly标准格式），Runtime编译的是平台适配的二进制（由WASM二次编译，而且不需要PE/ELF/MACHO头）

其中 Loader+Runtime 或 Loader+Runtime+Program 可以根据需要，分开打包或合成打包


## 2. 系统架构

基于人类建议的 Loader+Runtime+Program 三层架构，构建完整的自进化计算机软件系统：

### 2.1 三层架构设计

**Layer 1: Loader-{arch}.exe** [规划中]
- 架构特定的引导程序，负责加载Runtime和Program
- 处理PE/MachO/ELF头和操作系统接口
- 当前使用 tinycc 编译的 loader0.c (第零代C语言实现)
- 目标：进化出原生可执行程序替代C语言依赖

**Layer 2: Runtime-{arch}** [规划中] 
- 架构依赖的运行时二进制，实现WASM虚拟机
- 封装硬件架构和ABI接口
- 从.wasm标准格式二次编译生成
- 不包含PE/ELF/MACHO头（由Loader处理）

**Layer 3: Program** [部分实现]
- 平台无关的程序逻辑
- 编译为.wasm（WebAssembly标准格式）
- 支持C/C++/Rust等高级语言编译到WASM
- 当前实现：evolver系列自进化程序

# 来自人类的提示

- 除非是代码中的注释，否则默认中文；
- 经常正思反思然后才行动；注意自主思考和执行；
- 从设计角度来解决问题，不要乱简化来回避问题；
如果任务中等以上复杂程度，智能助理需要使用aitasker.md（放在当前代码仓根目录）来进行任务追踪，其中它的内容应该包括：
- ## 任务描述
- ## 动态规划的任务分解图（使用 mermaid 语法，每个节点有：简单ID、精简标题、评估进度百分比），尽量采用非线性并行工作机制，有利于加速进展
- ## 每个节点的具体任务描述（使用 markdown 语法，要有图中的简单ID、精简标题、评估进度百分比、子任务详细描述）
- ## 跟任务相关的经验和上下文累积（以确保智能助理不分神）
- ## 集中用 aitasker.md 管理任务，不用开新的 md 文档
注意不要在 aitasker.md 放太多无用的“历史”信息， 内容要尽量保持 up-to-date