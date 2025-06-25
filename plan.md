# Self-Evolve AI 项目规划

## 1. 项目愿景

创建一个能够真正自我进化的AI系统，通过代码自我修改、编译和优化实现持续进化，最终目标是实现完全自主的智能系统。

## 来自人类的意见参考

跨架构自举 + AI协同进化

本项目自进化范式：先用Loader+Runtime+Program的结构，通过多代自举和AI驱动的进化，逐步构建出完整的自进化计算机软件系统

核心是一种ASTC的数据结构，整合了WASM/IR/AST的概念，它也是Loader/Runtime/Program的底层逻辑。后面发展高级语言也要由解析器转成它。不过它的压缩率不大，后面可以制作binx二进制码，在执行时才动态把它转ASTC再执行

- c2astc.c 把c语言编译成astc的库
- Loader-{arch}.exe 负责加载Runtime-{arch} + Program.astc，目前主要是操作系统内引导，未来不排除继续演化出操作系统。（注意，第零代的是tinycc编译的C程序loader0.c，之后进化出可执行程序后不再使用）
- Runtime 是架构依赖二进制，封装硬件架构和ABI等，实现ASTC虚拟机。特别注意那些PE/MachO/ELF头不在Runtime而是在Loader上的了
- Program 是平台无关程序（ASTC格式的二进制模块）

注意：Runtime和Program两种编译的二进制是不一样的。Program编译的是ASTC，Runtime编译的是平台适配的二进制（由ASTC再编译成机器码，且不需要PE/ELF/MACHO头）

其中 Loader+Runtime 或 Loader+Runtime+Program 可以根据需要，分开打包或合成打包

## 2. 系统架构

初期基于人类建议的 Loader+Runtime+Program 三层架构，构建完整的自进化计算机软件系统：

### 2.1 三层架构设计

**Layer 1: Loader-{arch}.exe**
- 架构特定的引导程序，负责加载Runtime和Program
- 处理PE/MachO/ELF头和操作系统接口（先对接syscall和libc）
- 使用 tinycc 编译的 evolver0.c (第零代C语言实现)
- 目标：进化出原生可执行程序替代C语言依赖

**Layer 2: Runtime-{arch}**
- 架构依赖的运行时二进制，实现ASTC虚拟机
- 封装硬件架构和ABI接口
- 从.ASTC 格式二次编译生成
- 不包含PE/ELF/MACHO头

**Layer 3: Program**
- 平台无关的程序逻辑
- 编译为.ASTC
- 支持C/C++/Rust等高级语言编译到 ASTC
- 当前实现：evolver系列自进化程序

## 3. 进化路径规划

### 3.1 evolver0 (当前阶段)

evolver0是第零代自举编译器，使用C语言编写，由标准C编译器(如TCC)编译。它的目标是实现一个能够编译自身的C编译器从而脱离任何其他编译器依赖）。

**当前进度**: 40% (详见aitasker.md)

### 3.2 evolver1 (规划阶段)

evolver1是第一代自举编译器，将由evolver0编译生成。它将扩展evolver0的功能，并开始支持 ASTC 格式输出。

**计划功能**:
- 支持更完整的C语言子集
- 添加优化器模块
- 实现 ASTC 格式输出
- 支持跨平台交叉编译

### 3.3 Runtime开发 (规划阶段)

Runtime模块是连接Loader和Program的关键组件，负责执行 ASTC 格式的Program。

**计划功能**:
- 实现基本的 ASTC 虚拟机
- 提供操作系统API封装
- 支持JIT编译优化
- 实现跨平台兼容层

**实现路径**:
1. 使用evolver1开发Runtime1
2. 逐步扩展Runtime功能
3. 实现对多种硬件架构的支持

### 3.4 Program层开发 (规划阶段)

Program层是平台无关的程序逻辑，将使用 ASTC 格式实现。

**计划功能**:
- 实现自我修改和优化的核心逻辑
- 开发AI驱动的进化算法
- 构建自我学习和适应机制

## 4. 测试结果与进展

### 4.1 c2astc模块测试结果

c2astc模块已经实现了将C语言代码转换为ASTC表示的基本功能。以下是使用TCC编译器对c2astc模块进行的测试结果：

#### 4.1.1 基本功能测试

测试程序`test_c2astc.exe`成功解析了以下基本C语言结构：
- 变量声明和初始化
- 函数声明和定义
- 简单表达式和语句
- 控制流语句（if、while、for等）

示例测试代码：
```c
int main() { int a = 10; return 0; }
```

成功生成了正确的ASTC表示：
```
TranslationUnit
  FunctionDecl: main
    ReturnType:
      TypeSpecifier: int
    Parameters:
    Body:
      CompoundStmt
        VarDecl: a
          Type:
            TypeSpecifier: int
          Initializer:
            Constant: 10
        ReturnStmt
          Constant: 0
```

#### 4.1.2 复杂类型测试

测试文件`complex_types_test.c`验证了c2astc模块对复杂类型的支持：
- 结构体定义和嵌套结构体
- 联合体定义
- 枚举定义
- 匿名结构体和联合体
- 复杂数据结构（结构体包含联合体等）

测试结果显示c2astc模块能够正确解析这些复杂类型，并生成相应的ASTC表示。

#### 4.1.3 数组类型测试

测试文件`array_types_test.c`验证了c2astc模块对数组类型的支持：
- 一维数组
- 多维数组
- 字符串数组
- 数组访问表达式

测试结果表明c2astc模块能够正确处理各种数组类型和操作。

#### 4.1.4 函数指针测试

测试文件`function_pointer_test.c`验证了c2astc模块对函数指针的支持：
- 函数指针声明
- 函数指针赋值
- 通过函数指针调用函数
- 函数指针作为参数传递
- 结构体中的函数指针

测试结果显示c2astc模块能够正确解析和处理函数指针相关的语法结构。

#### 4.1.5 序列化和反序列化测试

测试验证了ASTC节点的序列化和反序列化功能：
- 将ASTC节点序列化为二进制格式
- 从二进制格式反序列化为ASTC节点
- 验证序列化和反序列化前后节点类型一致

测试结果表明序列化和反序列化功能正常工作。

### 4.2 总体评估

c2astc模块已经实现了大部分计划的功能，能够处理C语言的主要语法结构和类型系统。当前的实现支持：

1. 基本类型和表达式
2. 控制流语句（if、while、for、return、break、continue）
3. 复杂类型（结构体、联合体、枚举）
4. 指针类型和函数指针
5. 数组类型和多维数组
6. 成员访问表达式（点运算符和箭头运算符）
7. ASTC的序列化和反序列化

下一步计划：
1. 完善类型检查和语义分析
2. 优化ASTC生成
3. 扩展对C语言更多特性的支持（如可变参数函数、位域等）
4. 与Runtime和Program模块集成测试

# 来自人类的开发提示（供智能助理参考）

- 除了代码中的注释，其它默认中文；
- 经常正思反思然后才行动；注意自主思考和执行；
- 从设计角度来解决问题，不要乱简化来回避问题；
如果任务中等以上复杂程度，智能助理需要使用aitasker.md（放在当前代码仓根目录）来进行任务追踪，其中它的内容应该包括：
- ## 任务描述
- ## 动态规划的任务分解图（使用 mermaid 语法，每个节点有：简单ID、精简标题、评估进度百分比），尽量采用非线性并行工作机制，有利于加速进展
- ## 每个节点的具体任务描述（使用 markdown 语法，要有图中的简单ID、精简标题、评估进度百分比、子任务详细描述）
- ## 跟任务相关的经验和上下文（以确保智能助理不分神）
- ## 集中用 aitasker.md 管理任务，不用开新的 md 文档
注意不要在 aitasker.md 放太多无用的"历史"信息， 内容要尽量保持最新
