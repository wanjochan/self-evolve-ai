# Self-Evolve AI 项目执行计划

## 🚨 重要指导原则 - 避免文件混乱

### ❌ 严禁的行为模式
1. **创建重复文件** - 绝对不要创建类似功能的新文件
2. **绕过现有代码** - 不要因为代码复杂就创建简化版本
3. **临时文件泛滥** - 避免创建过多临时测试文件
4. **忽视现有架构** - 必须在现有8553行核心代码基础上工作

### ✅ 必须遵循的原则
1. **修改而非新建** - 优先修改现有文件而非创建新文件
2. **理解后再动作** - 深入理解现有代码功能后再修改
3. **保持架构完整** - 维护现有三层架构的完整性
4. **实事求是评估** - 准确评估现有代码的完成度

## 📁 核心文件清单 - 绝对不要重复创建

### Layer 3: Program层 (95%完成)
- **`src/runtime/compiler_c2astc.c`** (6056行) - 完整C99编译器 ✅
- **`src/runtime/core_libc.c`** (411行) - libc转发接口 ✅  
- **`src/c99_program.c`** (482行) - C99程序层实现 ✅
- **`src/tool_c2astc.c`** (108行) - C到ASTC转换工具 ⚠️命令行参数待修复

### Layer 2: Runtime层 (90%完成)
- **`src/runtime/compiler_astc2rt.c`** (803行) - JIT编译器 ✅
- **`src/runtime/vm_astc.c`** (1694行) - ASTC虚拟机 ✅
- **`src/runtime/compiler_codegen_x64.c`** (246行) - x64后端 ✅
- **`src/runtime/compiler_codegen_arm64.c`** (216行) - ARM64框架 ✅
- **`src/runtime/ffi_interface.c`** (300行) - FFI系统 ✅

### Layer 1: Loader层 (90%完成)
- **`src/runtime/core_loader.c`** (643行) - 核心加载器 ✅
- **`src/enhanced_loader.c`** (331行) - 增强加载器 ✅
- **`src/universal_loader.c`** (260行) - 通用加载器 ✅

### 核心数据结构 (100%完成)
- **`src/runtime/core_astc.h`** (536行) - ASTC架构定义 ✅完美符合PRD.md

### 构建脚本 (待修正)
- **8个.bat文件** - 包含TinyCC依赖，需要清理 ⚠️

## 🎯 立即执行任务 - 基于现有文件

### 🔥 第一优先级：修复工具可用性
```bash
任务：修复src/tool_c2astc.c的命令行参数解析
文件：src/tool_c2astc.c (第30-45行)
问题：--version和--help参数未处理
解决：在现有参数解析逻辑中添加这些选项
时间：30分钟
```

### 🔥 第二优先级：验证工具链完整性
```bash
任务：端到端测试c2astc→astc2rt→loader流程
测试文件：tests/simple_test.c (如不存在则创建)
验证文件：
  - bin/tool_c2astc.exe ✅存在
  - bin/tool_astc2rt.exe ✅存在  
  - bin/enhanced_loader.exe ✅存在
流程：tool_c2astc → tool_astc2rt → enhanced_loader
```

### 🔥 第三优先级：消除TinyCC依赖
```bash
任务：分析并修正8个构建脚本中的TinyCC依赖
影响文件：
  - build_true_self_hosted.bat
  - build_completely_independent.bat
  - build_bootstrap_independent.bat
  - (等5个其他脚本)
策略：用现有的23个.exe文件替代TinyCC调用
```

## 📊 项目真实状态评估

### ✅ 技术实现状态 (远超预期)
- **核心代码量**: 8553行 (c2astc:6056 + vm:1694 + astc2rt:803)
- **ASTC架构**: 完美符合PRD.md"整合WASM/IR/AST/JIT概念"
- **三层架构**: 完全按PRD.md设计实现
- **可执行文件**: 23个.exe文件已生成
- **ASTC/RT文件**: 30+个已生成
- **技术完整度**: 约85%

### ⚠️ 关键瓶颈 (阻碍自举)
- **TinyCC依赖**: 8个构建脚本仍依赖external/tcc-win
- **自举验证**: 缺乏端到端独立编译循环验证
- **声称vs实际**: 多个脚本声称"TinyCC Independence"但仍有依赖

### 🎯 核心矛盾
**技术已就绪，但自举验证未完成**，直接违反PRD.md"摆脱其它cc依赖"要求

## 🚀 具体执行步骤 - 避免新建文件

### 步骤1：修复工具可用性 (今天)
```bash
# 修改现有文件，不要新建
编辑文件：src/tool_c2astc.c
修改位置：main函数参数处理部分
添加内容：--version, --help选项支持
测试命令：bin\tool_c2astc.exe --version
预期结果：显示版本信息而不是报错
```

### 步骤2：验证现有工具链 (今天)
```bash
# 使用现有可执行文件，不要重新编译
测试序列：
1. bin\tool_c2astc.exe tests/simple_test.c tests/simple_test.astc
2. bin\tool_astc2rt.exe tests/simple_test.astc tests/simple_test_x64_64.rt
3. bin\enhanced_loader.exe tests/simple_test_x64_64.rt tests/simple_test.astc

记录结果：每个步骤的成功/失败状态
识别问题：缺失的功能或集成问题
```

### 步骤3：分析TinyCC依赖 (明天)
```bash
# 分析现有脚本，不要创建新脚本
分析文件：8个.bat构建脚本
搜索内容：external\tcc-win\tcc\tcc.exe调用
替换策略：使用bin\目录下的23个现有工具
验证方法：运行修改后的脚本，确保无TinyCC调用
```

### 步骤4：建立独立循环 (后续)
```bash
# 基于现有工具，不要重新实现
使用工具：现有23个.exe文件
编译目标：
  - src/runtime/compiler_c2astc.c → 新的tool_c2astc.exe
  - src/runtime/compiler_astc2rt.c → 新的tool_astc2rt.exe  
  - src/runtime/core_loader.c → 新的loader.exe
验证标准：新工具能编译自身，形成完全独立循环
```

## 📋 文件操作规范

### ✅ 允许的操作
1. **修改现有文件** - 在现有代码基础上改进
2. **删除临时文件** - 清理tests/目录下的临时文件
3. **重命名混乱文件** - 整理命名不规范的文件
4. **修复现有构建脚本** - 去除TinyCC依赖

### ❌ 禁止的操作  
1. **创建新的编译器** - 已有compiler_c2astc.c完整实现
2. **重新实现JIT** - 已有compiler_astc2rt.c完整实现
3. **新建虚拟机** - 已有vm_astc.c完整实现
4. **复制现有功能** - 任何重复功能的新文件

### 🔧 特殊情况处理
- **如需测试文件** - 只在tests/目录创建，用完即删
- **如需临时脚本** - 只在当前会话使用，不保存到仓库
- **如需配置文件** - 修改现有配置而非新建

## 🎯 成功标准

### 短期目标 (本周)
- [ ] tool_c2astc.exe支持--version, --help参数
- [ ] 完成一个简单C程序的端到端编译验证
- [ ] 识别并记录所有TinyCC依赖点

### 中期目标 (2周内)
- [ ] 所有构建脚本完全不依赖TinyCC
- [ ] 使用现有工具成功编译自身的核心组件
- [ ] 建立完全独立的编译循环

### 长期目标 (1个月内)
- [ ] 符合PRD.md"摆脱其它cc依赖"核心要求
- [ ] 系统能够完全自举，无任何外部编译器依赖
- [ ] 为AI进化框架奠定坚实基础

## 🚨 风险警告

### 高风险操作
- **重新实现已有功能** - 浪费时间且容易引入Bug
- **忽视现有架构** - 破坏已有的精心设计
- **创建平行实现** - 导致代码分裂和维护困难

### 应对策略
- **深入理解后再动手** - 仔细阅读现有代码再修改
- **小步快跑迭代** - 每次只改动小部分，及时验证
- **保持架构完整** - 确保修改不破坏三层架构设计

---

**记住：我们已经有了8553行高质量的核心代码，现在需要的是整合和验证，而不是重新发明轮子！** 