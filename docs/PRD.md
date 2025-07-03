# Self-Evolve AI 项目规划

## 项目愿景

能自我进化的AI系统，通过代码自我修改、编译和优化实现持续进化，最终目标是实现完全自主的通用智能。

## 进化阶段划分
- **Stage 1: 借用人类经验** - 从兼容C99开始，建立基础技术栈 （Layer 1 2 3）
- **Stage 2: 模式识别进化** - AI识别和优化现有代码模式 （等Stage 1彻底稳定后等主人通知才开始）
- **Stage 3: 架构创新进化** - AI发现超越人类设计的新架构（等主人通知才开始）
- **Stage 4: 通用智能涌现** - 完全自主的计算模式创新（等主人通知才开始）

## 核心设计

```
Layer 1 Loader: loader_{arch}_{bits}.exe  //执行入口，未来参考cosmopolitan等制作跨架构统一入口loader.exe
    return import(f'vm_{arch}_{bits}.native').main(f'{program}.astc',argv[],env[]) //示意伪代码
Layer 2 Runtime: vm_{arch}_{bits}.native  // .native原生字节码模块，其中最重要是vm模块用于加载astc运行
    main(astc_module_name,argv[],env[]):
        return vm_import(astc_module_name).main(argv[],env[])  //示意伪代码
Layer 3 Program: {program}.astc     //用户程序（比如c99、evolver{version}）ASTC字节码，后面兼容ASTC-ASM、ASTC-ES6等高级语言
    c99:
        return c99_compile(c_file_name, argv[])
    evolver0:
        return evolve() //基于c99他stage1开始进入stage2的开发,TODO
```

layer-loader:
作为统一入口点,实现了架构无关的加载器
自动检测当前硬件架构和操作系统
动态选择和加载对应架构的VM模块
转发命令行参数和环境变量
未来可以参考cosmopolitan项目实现跨架构的统一loader

layer-runtime:
原生字节码模块,针对特定架构优化
负责加载和执行ASTC字节码
if module=='vm'，提供VM运行时环境for astc，处理内存管理、JIT编译等底层功能

layer-program:
用户程序的ASTC字节码表示
完全架构无关的中间表示
支持多种程序类型(如c99编译器、evolver等)
未来可扩展支持ASTC-ASM、ASTC-ES6等高级语言


## dev roadmap (by human master)
- src/core/   # the real core with module system
    - astc.h     # core def about ASTC
    - native_format.h  # our native module format def
    - utils.[ch]                       # core utils for our system
        - arch and bits dector (must) not using macro)
        - mmap-alike utils (copy and chmod exec)
        - build and write .native bytecodes modules
        - loader for .native
- src/ext/    # todo next
    - astc.h      # core def about astc
    - astc_module.c            # native module that convert C to ASTC vise versa
    - vm_module.c              # native module that vm that load .astc
    - libc_module.c            # native module that of libc forwader 
    - 

- layer 1 loader (windows exe)
- layer 2 native module (vm, libc), will be loaded by mmap() alike. (not libdl or ffi)
- layer 3 program (c99 windows 64 x86)
- build tcc with c99 // test c99 working good
- layer 3 program c99 supports cross build
- cross build layer 1 loader (linux, macos)
- cross build layer 2 vm (arm, riscv, mips, etc.)
- build loader2 with c99

- src/utils.c:: libdl-alike, libffi-alike

## stage1 dev flow
loop: update .c and run build_layer1.bat + build_layer2.bat + bild_layer3.bat and then 
.\bin\layer1\loader_x64_64.exe -m bin\layer2\vm_x64_64.native bin\layer3\c99.astc -- test_hello.c -o hello.exe

### 核心技术
- **ASTC字节码**: 可扩展的计算表示 (core_astc.h)
- **.native模块**: 原生字节码模块 {module}_{arch}_{bits}.native (native_format.h)
- **JIT编译**: 动态代码生成引擎 (astc2native.c)

## 5. 实现路线图

### Phase 1: 自举

### Phase 2: 生态完善

### Phase 3: AI进化

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
