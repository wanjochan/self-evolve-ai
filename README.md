# Self-Evolving AI System

自举编译和AI协同进化、跨硬件架构



## 项目概述

本项目自进化范式：用Loader+Runtime+Program的结构，通过多代自举和AI驱动的进化，逐步构建出完整的自进化计算机软件系统

- Loader-{arch}.exe 负责加载Runtime-{arch} + Program.bin ，目前主要是操作系统内引导，未来不排除继续演化出操作系统。（注意，第零代的是python程序loader0.py，之后进化出可执行程序后不再使用）
- Runtime 是架构依赖二进制，封装硬件架构和ABI等，类似libc、dotnet等。特别注意那些PE/MachO/ELF头不在Runtime而是在Loader上的了
- Program 是平台无关程序（可以是TASM汇编二进制或后续的人类友好的语言如ES6）

Runtime和Program两种编译的二进制是不一样的。Program编译的是.tir（平台无关的IR），Runtime编译的是平台适配的二进制（由IR二次编译，而且不需要PE/ELF/MACHO头）

其中 Loader+Runtime 或 Loader+Runtime+Program 可以根据需要，分开打包或合成打包


# TASM - Token Assembly Language

TASM是一个基于IR的汇编语言系统，支持多平台编译和执行。

## 核心特性

- **极简、自举**: 每一代都是实现最小子集
- **多代进化**: 
- **AI优化**: 由AI驱动，并使用遗传算法自动优化代码生成
- **统一中间格式**: Token-ASM作为贯穿全系统的中间表示
- **紧凑高效**: 使用变长编码实现高效的指令格式

## 编译流程

TASM采用基于IR(中间表示)的多阶段编译流程:

```
源码(TASM) -> IR -> Runtime(平台相关) / Program(平台无关)
```

1. **前端(共享)**
   - TASM源码解析为统一的IR
   - 语法分析和语义检查
   - IR级别优化(常量折叠、死代码消除等)
   - 提供丰富的调试信息

2. **后端(分离)**
   - Runtime后端
     * 针对不同平台(x86/ARM等)生成本地机器码
     * 平台特定优化和ABI适配
     * 指令选择和寄存器分配
   - Program后端
     * 生成平台无关的TASM二进制格式
     * 保留完整的元数据
     * 支持解释执行和JIT编译

3. **优化策略**
   - IR层面优化
     * 控制流分析和优化
     * 公共子表达式消除
     * 数据流分析
   - 后端优化
     * 指令调度
     * 窥孔优化
     * 平台特定优化

## 使用方法

### 编译TASM程序

```bash
# 编译为TASM格式
python tasm_compiler.py test.tasm

# 编译为x86-64机器码
python tasm_compiler.py -t x86_64 test.tasm

# 编译为ARM64机器码
python tasm_compiler.py -t arm64 test.tasm

# 启用调试信息
python tasm_compiler.py -d test.tasm

# 启用优化
python tasm_compiler.py -O test.tasm

# 指定输出文件
python tasm_compiler.py -o output.bin test.tasm
```

### 运行TASM程序

```bash
# 运行程序
python loader0.py run test.bin

# 调试模式运行
python loader0.py debug test.bin

# 显示程序信息
python loader0.py info test.bin
```

### TASM语法

```asm
; 段声明
section .code
section .data

; 寄存器操作
mov.r  r0, r1      ; r0 = r1
ldi.r  r0, 42      ; r0 = 42
add.r  r0, r1, r2  ; r0 = r1 + r2
sub.r  r0, r1, r2  ; r0 = r1 - r2
mul.r  r0, r1, r2  ; r0 = r1 * r2
div.r  r0, r1, r2  ; r0 = r1 / r2

; 跳转和调用
jmp    label       ; 无条件跳转
jz     label       ; 如果零则跳转
jnz    label       ; 如果非零则跳转
call   func        ; 调用函数
ret                ; 返回
exit               ; 退出程序(返回r7的值)

; 标签
label:
func:
```

### 环境变量

- `RUNTIME` - 指定Runtime文件
- `RUNTIME_TIMEOUT` - 执行超时（秒）
- `RUNTIME_DEBUG` - 启用调试输出

## 开发计划

### 1. 基础设施建设 [进行中]
- [x] 基础二进制运行时
- [x] 程序加载执行系统
- [x] 代码自修改能力
- [x] 基本安全机制
- [ ] 完善IR系统
  * 类型系统
  * 控制流图
  * SSA形式

### 2. 编译系统增强 [进行中]
- [x] 支持新增指令集
- [x] 完善指令解析
- [ ] 增强优化器
  * 常量折叠
  * 死代码消除
  * 循环优化
- [ ] 扩展后端支持
  * [x] x86-64
  * [ ] ARM64
  * [ ] WebAssembly

### 3. 工具链改进 [计划中]
- [ ] 调试器
- [ ] 性能分析器
- [ ] 测试框架
- [ ] 开发工具集成

### 4. 智能化功能 [规划中]
- [ ] 代码变异策略
- [ ] 评估系统
- [ ] 知识库系统
- [ ] AI协同优化

## 重点文件

- [PLAN.md](PLAN.md) - 详细开发追踪和进度
- [TASM.md](TASM.md) - TASM规范和技术文档
- bootstrap/
  * [pe.md](bootstrap/pe.md) - Windows PE文件头规范
  * [elf.md](bootstrap/elf.md) - Linux ELF文件头规范
  * [macho.md](bootstrap/macho.md) - macOS Mach-O文件头规范

## 思考

- 脱离人类独立进化：不用依赖人类的路线（c/c#/python/js/java等），鼓励直接使用TASM甚至创作自己的高级语言。只要bootstrap自举之后，要跟AI协同进化甚至进化出自己的AI
- 可以与人类互动：跟人类互动但不用依赖人类的东西。也可以生成适配工具链让人类的程序能跑在自己的底层上
- 要小心谨慎、三思而后行

## 许可证

AI License 

