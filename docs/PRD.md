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
Layer 1 Loader: loader_{arch}_{bits}.exe  //执行入口，未来参考cosmopolitan等制作跨架构统一入口 loader.exe
    //导入vm模块 f'vm_{arch}_{bits}.native' 转发参数和环境变量给程序f'{program}.astc'
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
//重点核心模块主要是vm、astc、libc、std

layer-program:
用户程序的ASTC字节码表示
架构无关的中间表示IR
先实现ASTC字节码支持，未来可扩展支持ASTC-ASM、ASTC-ES6等高级语言

### 核心技术
- **ASTC字节码**: 可扩展的计算表示 (astc.[h|c])
- **.native模块**: 原生字节码模块 {module}_{arch}_{bits}.native (native.[h|c])
- **多种转换工具**: c2astc,astc2native,c2native等

## 5. 实现路线图
stage 1
```
dev roadmap (by human master)
- src/core/                    # the real core
- layer 1 loader (windows exe)
- layer 2 native module (vm, libc, std, astc, jit, utils, etc), will be loaded by mmap() alike. (not libdl or ffi)
- layer 3 program (c99 windows 64 x86)
- build tcc with c99 // test c99 working good
- layer 3 program c99 supports cross build
- cross build layer 1 loader (linux, macos)
- cross build layer 2 vm (arm, riscv, mips, etc.)
- build loader2 with c99 (then start to be free from tinycc)

- src/utils.c:: libdl-alike, libffi-alike ? to discuss further
```

下面是临时笔记请忽略：
```
这几个模块似乎应该合并成同一个模块

src/core/modules/astc2native_module.c
src/core/modules/c2astc_module.c
src/core/modules/codegen_module.c

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

2. **遵循PRD.md第76行**：使用mmap()加载，不是libdl或ffi
3. **遵循PRD.md第84行**：src/utils.c实现libdl-alike, libffi-alike功能

### 🎯 正确编译流程：
```
源码 → 编译为目标代码 → 使用native.c系统创建.native格式 → 输出真正的.native文件
```

**错误流程**：
```
源码 → 编译为.exe → 重命名为.native ❌
```
