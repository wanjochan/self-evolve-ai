# C99bin 工作任务 (work_id=c99bin)

## 任务概述

基于现有src/core/模块化C设计，开发c99bin编译器，采用类似tinycc的JIT执行思路，直接生成可执行文件，不走astc+vm路线。

## 可行性评估: ✅ 高度可行

### 现有优势
- **模块化架构**: src/core/的.native模块系统已实现动态加载机制
- **JIT基础**: compiler_module.c已有JIT编译器框架，包含缓存和优化机制  
- **编译流水线**: pipeline_module.c已实现完整的c2astc编译前端
- **多架构支持**: 支持x64_64、arm64等架构的.native模块

## 技术路径对比

```
现有astc路径: C源码 → c2astc → .astc字节码 → astc vm执行
c99bin路径:   C源码 → c2astc → AST → 直接机器码 → 可执行文件
```

## 实现方案

### 推荐方案: 创建c99bin_module.c

**核心设计思路:**
- 新建独立的c99bin_module.c作为.native模块
- 最大化复用现有pipeline前端(c2astc)
- 集成现有JIT技术用于AOT编译
- 输出标准ELF/PE可执行文件

**架构集成:**
```
Layer 1: simple_loader
    ↓
Layer 2: c99bin_x64_64.native (新增模块)
    ├── 复用: pipeline前端 (词法/语法分析)
    ├── 复用: compiler JIT技术
    ├── 新增: AOT代码生成器
    └── 新增: 可执行文件生成器
    ↓
Layer 3: 标准可执行文件 (.exe/.elf)
```

## 详细实现计划

### 阶段1: 模块框架搭建
- [ ] 创建 `src/core/modules/c99bin_module.c`
- [ ] 实现Module接口 (init/cleanup/resolve/sym)
- [ ] 集成到现有模块加载系统
- [ ] 基础架构检测和多平台支持

### 阶段2: 复用现有组件
- [ ] 集成pipeline前端: 复用c2astc词法/语法分析器
- [ ] 集成compiler JIT: 复用现有JIT编译框架
- [ ] 适配AST到机器码的直接转换路径
- [ ] 绕过.astc中间表示，直接处理AST

### 阶段3: AOT代码生成
- [ ] 实现AST到机器码的直接生成器
- [ ] 支持x86_64和ARM64架构
- [ ] 集成现有的优化和缓存机制
- [ ] 处理函数调用、变量访问、控制流

### 阶段4: 可执行文件生成
- [ ] 实现ELF文件格式生成 (Linux)
- [ ] 实现PE文件格式生成 (Windows)
- [ ] 系统库链接处理
- [ ] 程序入口点设置

### 阶段5: 系统集成
- [ ] 创建c99bin命令行工具
- [ ] 集成到build_tools.sh
- [ ] 支持常用编译选项 (-o, -I, -L等)
- [ ] 错误处理和诊断信息

### 阶段6: 测试验证
- [ ] 基础功能测试
- [ ] 与tinycc兼容性测试
- [ ] 性能基准测试
- [ ] 跨平台测试

## 关键技术挑战

### 1. 链接器实现
**问题**: 需要生成标准的ELF/PE文件格式
**方案**: 
- 复用现有的symbol resolution机制
- 实现简化版的静态链接器
- 支持标准C库链接

### 2. 系统调用处理
**问题**: 直接可执行文件需要处理系统库依赖
**方案**:
- 复用现有libc_module的系统调用封装
- 支持动态链接标准库
- 实现runtime启动代码

### 3. 调试信息支持
**问题**: 可执行文件的调试信息生成
**方案**:
- 保留源码位置信息
- 生成DWARF调试格式
- 集成到现有错误处理系统

## 复用策略详情

### 直接复用组件
```c
// 复用pipeline前端
Module* pipeline = load_module("./pipeline");
void* c2astc = pipeline->sym("frontend_parse");

// 复用JIT编译技术
Module* compiler = load_module("./compiler");  
void* jit_compile = compiler->sym("jit_compile_ast");

// 复用基础设施
Module* layer0 = load_module("./layer0");
void* memory_alloc = layer0->sym("memory_alloc");
```

### 新增核心功能
```c
// c99bin特有功能
int c99bin_compile_to_executable(const char* source_file, const char* output_file);
int c99bin_generate_elf(ASTNode* ast, const char* output_file);
int c99bin_generate_pe(ASTNode* ast, const char* output_file);
```

## 预期效果

### 功能对标
- **编译速度**: 接近tinycc的快速编译
- **可执行性能**: 生成高效的原生机器码
- **兼容性**: 支持主要C99语法特性
- **集成度**: 无缝集成到现有模块化架构

### 使用体验
```bash
# 类似tinycc的使用方式
./c99bin hello.c -o hello
./hello

# 集成到现有工具链
c99.sh --use-c99bin hello.c -o hello
```

## 风险评估

### 低风险
- ✅ 现有模块架构成熟稳定
- ✅ JIT编译技术已验证
- ✅ 前端parser可直接复用

### 中等风险  
- ⚠️ 可执行文件格式生成复杂度
- ⚠️ 系统库链接兼容性
- ⚠️ 跨平台调试信息标准化

### 缓解措施
- 渐进式开发，先支持基础功能
- 复用成熟的开源链接器代码
- 参考tinycc的实现经验

## 开发优先级

**P0 (核心功能)**:
- c99bin_module基础框架
- AST到机器码直接生成
- 基础可执行文件输出

**P1 (重要特性)**:
- 多架构支持
- 系统库链接
- 错误诊断

**P2 (增强功能)**:
- 调试信息支持
- 优化特性
- 高级链接选项

## 成功标准

1. **功能完整**: 支持基础C99语法编译到可执行文件
2. **性能目标**: 编译速度不低于tinycc的80%
3. **架构一致**: 完全兼容现有.native模块系统
4. **复用充分**: 90%以上复用现有core组件

---

**状态**: 📋 待开工
**预计工期**: 4-6周
**负责人**: 待分配