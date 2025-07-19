# Work Plan: stage1crossbuild

**work_id**: stage1crossbuild  
**目标**: 完成Stage 1的跨平台交叉编译支持  
**优先级**: Critical  
**预期工期**: 4-6周  

## 🎯 **项目目标**

彻底完成Stage 1的跨平台支持，实现四个目标架构的完整编译和运行能力：
- ✅ linux-x64 (已完成)
- 🎯 windows-x64 (新增)
- 🎯 macos-arm64 (新增)
- 🎯 windows-x86 (新增)

## 📋 **核心任务分解**

### **Task 1: C99Bin交叉编译器扩展 (2周)**

#### **T1.1 Windows PE文件格式支持**
- **目标**: C99Bin能够生成Windows PE可执行文件
- **技术要求**:
  - PE32/PE32+文件格式生成器
  - Windows系统调用映射
  - 动态链接库(DLL)支持
  - Windows运行时链接

**交付物**:
- `src/core/modules/pe_generator.c` - PE文件生成器
- `src/core/modules/windows_runtime.c` - Windows运行时支持
- 扩展的x86_64_codegen.c支持Windows ABI

#### **T1.2 macOS Mach-O文件格式支持**
- **目标**: C99Bin能够生成macOS Mach-O可执行文件
- **技术要求**:
  - Mach-O文件格式生成器
  - macOS系统调用映射
  - 动态链接库(.dylib)支持
  - macOS运行时链接

**交付物**:
- `src/core/modules/macho_generator.c` - Mach-O文件生成器
- `src/core/modules/macos_runtime.c` - macOS运行时支持
- 扩展的arm64_codegen.c支持macOS ABI

#### **T1.3 多架构代码生成器增强**
- **目标**: 支持x86_32架构代码生成
- **技术要求**:
  - x86_32汇编代码生成器
  - 32位ABI支持
  - 内存模型适配

**交付物**:
- `src/core/modules/x86_32_codegen.c` - x86 32位代码生成器

### **Task 2: Layer 1跨平台Simple Loader (1周)**

#### **T2.1 Windows版Simple Loader**
- **目标**: 构建Windows平台的ASTC字节码加载器
- **技术要求**:
  - Windows API集成
  - PE模块加载支持
  - Windows路径和文件系统适配

**交付物**:
- `bin/simple_loader_windows_x64.exe`
- `bin/simple_loader_windows_x86.exe`

#### **T2.2 macOS版Simple Loader**
- **目标**: 构建macOS平台的ASTC字节码加载器
- **技术要求**:
  - macOS API集成
  - Mach-O模块加载支持
  - macOS路径和文件系统适配

**交付物**:
- `bin/simple_loader_macos_arm64`

#### **T2.3 统一跨平台检测逻辑**
- **目标**: 实现自动平台检测和模块加载
- **技术要求**:
  - 运行时平台识别
  - 自动选择对应架构的native模块
  - 统一的加载接口

**交付物**:
- 更新的`src/layer1/simple_loader.c`支持跨平台

### **Task 3: Layer 2跨平台Native模块 (2周)**

#### **T3.1 Pipeline模块跨平台构建**
- **目标**: 为所有目标平台构建pipeline模块

**交付物**:
- `bin/pipeline_windows_x64_64.native`
- `bin/pipeline_windows_x86_32.native`
- `bin/pipeline_macos_arm64_64.native`

#### **T3.2 Layer0模块跨平台构建**
- **目标**: 为所有目标平台构建基础功能模块

**交付物**:
- `bin/layer0_windows_x64_64.native`
- `bin/layer0_windows_x86_32.native`
- `bin/layer0_macos_arm64_64.native`

#### **T3.3 其他核心模块跨平台构建**
- **目标**: 构建compiler、libc、module模块的跨平台版本

**交付物**:
- `bin/compiler_*_各平台.native`
- `bin/libc_*_各平台.native`
- `bin/module_*_各平台.native`

#### **T3.4 平台特定优化**
- **目标**: 针对不同平台进行性能优化
- **技术要求**:
  - Windows MSVC ABI兼容性
  - macOS Apple Silicon优化
  - x86平台特定优化

### **Task 4: 跨平台构建系统 (1周)**

#### **T4.1 Windows构建脚本**
- **目标**: 创建Windows平台的自动化构建

**交付物**:
- `build_windows.bat` - Windows批处理构建脚本
- `build_windows.ps1` - PowerShell构建脚本

#### **T4.2 macOS构建脚本**
- **目标**: 创建macOS平台的自动化构建

**交付物**:
- `build_macos.sh` - macOS构建脚本
- Xcode项目文件支持(可选)

#### **T4.3 跨平台CI/CD集成**
- **目标**: 建立多平台持续集成

**交付物**:
- GitHub Actions工作流
- 多平台自动测试和构建

### **Task 5: 跨平台测试与验证 (1周)**

#### **T5.1 跨平台功能测试**
- **目标**: 验证所有平台的基础功能

**交付物**:
- `tests/test_crossplatform_comprehensive.sh`
- 四个平台的端到端测试

#### **T5.2 兼容性测试**
- **目标**: 验证ASTC字节码跨平台兼容性

**测试场景**:
- Linux编译的ASTC在Windows上运行
- Windows编译的ASTC在macOS上运行
- 等等...

#### **T5.3 性能基准测试**
- **目标**: 建立跨平台性能基线

**交付物**:
- 四个平台的性能报告
- 平台间性能对比分析

## 📊 **里程碑计划**

### **Week 1-2: 核心编译器扩展**
- ✅ PE文件格式生成器完成
- ✅ Mach-O文件格式生成器完成
- ✅ x86_32代码生成器完成
- ✅ 基础跨平台编译测试通过

### **Week 3: Layer 1&2跨平台构建**
- ✅ 所有平台的simple_loader构建完成
- ✅ 核心native模块跨平台构建完成
- ✅ 基础加载和执行测试通过

### **Week 4: 构建系统和集成**
- ✅ Windows/macOS构建脚本完成
- ✅ CI/CD多平台集成完成
- ✅ 自动化构建测试通过

### **Week 5-6: 测试验证和优化**
- ✅ 跨平台兼容性测试完成
- ✅ 性能基准测试完成
- ✅ 文档和示例完善
- ✅ 最终验收测试通过

## 🔧 **技术架构设计**

### **跨平台抽象层**
```c
// 平台抽象接口
typedef struct PlatformInterface {
    int (*file_open)(const char* path, int mode);
    int (*file_read)(int fd, void* buffer, size_t size);
    int (*file_write)(int fd, const void* buffer, size_t size);
    void (*file_close)(int fd);
    
    void* (*memory_alloc)(size_t size);
    void (*memory_free)(void* ptr);
    
    int (*process_create)(const char* path, char* const argv[]);
    int (*process_wait)(int pid);
} PlatformInterface;
```

### **文件格式生成器架构**
```c
// 统一的可执行文件生成接口
typedef struct ExecutableGenerator {
    int (*initialize)(const char* target_platform);
    int (*add_section)(const char* name, const void* data, size_t size);
    int (*add_symbol)(const char* name, size_t offset);
    int (*generate)(const char* output_path);
    void (*cleanup)(void);
} ExecutableGenerator;

// 平台特定实现
extern ExecutableGenerator elf_generator;    // Linux
extern ExecutableGenerator pe_generator;     // Windows  
extern ExecutableGenerator macho_generator;  // macOS
```

### **多架构代码生成架构**
```c
// 统一的代码生成器接口
typedef struct CodeGenerator {
    const char* architecture;  // "x64", "arm64", "x86"
    const char* platform;      // "linux", "windows", "macos"
    
    int (*generate_function_prologue)(FILE* output, const char* func_name);
    int (*generate_function_epilogue)(FILE* output);
    int (*generate_instruction)(FILE* output, const char* instruction);
    int (*generate_data_section)(FILE* output, const void* data, size_t size);
} CodeGenerator;
```

## 🧪 **测试策略**

### **单元测试**
- 每个新模块的独立功能测试
- PE/Mach-O文件格式正确性验证
- 代码生成器输出验证

### **集成测试**
- 跨平台编译流程测试
- Layer 1/2/3完整流程测试
- 平台间兼容性测试

### **端到端测试**
- 真实程序的跨平台编译和执行
- 复杂ASTC程序的跨平台运行
- 性能和稳定性测试

## 🚨 **风险评估**

### **技术风险**
1. **文件格式复杂性** 
   - 风险: PE/Mach-O格式实现复杂
   - 缓解: 分阶段实现，先支持基础功能

2. **ABI兼容性**
   - 风险: 不同平台的ABI差异
   - 缓解: 参考成熟编译器实现，充分测试

3. **系统调用差异**
   - 风险: 平台间系统调用接口不同
   - 缓解: 建立平台抽象层

### **项目风险**
1. **开发时间**
   - 风险: 跨平台开发可能超期
   - 缓解: 并行开发，重点先支持Windows x64

2. **测试复杂性**
   - 风险: 多平台测试环境复杂
   - 缓解: 使用CI/CD自动化测试

## 📈 **成功标准**

### **最低成功标准 (MVP)**
- ✅ Windows x64平台完全支持
- ✅ 基础ASTC程序跨平台运行
- ✅ 核心工具链跨平台构建

### **完全成功标准**
- ✅ 四个目标平台完全支持
- ✅ 跨平台性能基本相当
- ✅ 完整的跨平台构建和测试体系

### **超越目标**
- 🚀 性能优于现有跨平台编译器
- 🚀 支持额外的架构 (如RISC-V)
- 🚀 完全无外部依赖的跨平台工具链

## 📚 **参考资源**

### **文件格式规范**
- PE/COFF格式规范 (Microsoft)
- Mach-O文件格式规范 (Apple)
- ELF格式规范 (System V ABI)

### **跨平台编译器参考**
- LLVM跨平台架构
- GCC跨平台实现
- TinyCC跨平台支持

### **系统调用参考**
- Windows API文档
- macOS/iOS系统调用
- Linux系统调用表

## 🎯 **启动决策**

**建议董事会决策**:
1. ✅ **批准work_id=stage1crossbuild启动**
2. ✅ **暂停Stage 2开发资源分配**
3. ✅ **优先级设为Critical级别**

**预期收益**:
- 项目获得真正的跨平台能力
- 满足企业级多平台部署需求
- 为Stage 2提供坚实的技术基础
- 显著提升项目的商业价值和技术竞争力

**🚀 work_id=stage1crossbuild 准备就绪，等待董事会批准启动！**