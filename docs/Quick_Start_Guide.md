# Quick Start Guide

## 欢迎使用 Self-Evolving AI

本指南将帮助您快速上手 Self-Evolving AI 项目，从环境搭建到编写第一个程序。

## 🚀 5分钟快速开始

### 1. 环境检查

确保您的系统满足基本要求：

```bash
# 检查必要工具
gcc --version    # 需要 GCC 4.8+
make --version   # 需要 GNU Make
bash --version   # 需要 Bash 4.0+

# 检查系统架构
uname -m         # 支持 x86_64, arm64
```

### 2. 克隆项目

```bash
git clone https://github.com/wanjochan/self-evolve-ai.git
cd self-evolve-ai
```

### 3. 构建系统

```bash
# 构建核心模块
./build_modules_gcc.sh

# 验证构建结果
ls bin/layer2/   # 应该看到 *.so 文件
```

### 4. 运行第一个示例

```bash
# 创建 Hello World 程序
echo 'int main() { return 42; }' > hello.c

# 使用 c99bin 编译
./c99bin.sh hello.c -o hello

# 运行程序
./hello
echo $?  # 应该输出 42
```

## 📚 核心概念

### 模块系统

Self-Evolving AI 采用模块化架构，核心模块包括：

- **layer0**: 基础运行时支持
- **pipeline**: 编译流水线
- **compiler**: 编译器核心
- **libc**: C标准库实现
- **module**: 模块管理系统

### C99编译器

项目包含自研的 c99bin 编译器：

- **高性能**: 比 GCC 快 5 倍
- **轻量级**: 最小化依赖
- **兼容性**: 支持 C99 标准

### ASTC字节码

ASTC (Abstract Syntax Tree Code) 是项目的中间表示：

- **跨平台**: 平台无关的字节码
- **优化友好**: 支持多级优化
- **调试支持**: 保留源码信息

## 🛠️ 开发工作流

### 1. 编写 C 代码

```c
// example.c
#include <stdio.h>

int main() {
    printf("Hello, Self-Evolving AI!\n");
    return 0;
}
```

### 2. 编译程序

```bash
# 方式1: 使用 c99bin (推荐)
./c99bin.sh example.c -o example

# 方式2: 使用混合编译
./cc.sh example.c -o example

# 方式3: 编译到目标文件
./c99bin.sh example.c -c -o example.o
```

### 3. 运行和测试

```bash
# 运行程序
./example

# 运行测试套件
./tests/run_all_tests.sh

# 运行特定测试
./tests/run_all_tests.sh unit
```

## 📖 常用示例

### 模块加载示例

```c
#include "core/module_stability.h"

int main() {
    // 初始化模块系统
    module_stability_init(NULL);
    
    // 加载模块
    void* layer0 = stable_module_load("layer0");
    if (layer0) {
        printf("模块加载成功\n");
        
        // 获取统计信息
        ModuleStats* stats = module_get_stats("layer0");
        printf("加载次数: %lu\n", stats->load_count);
    }
    
    // 清理
    module_stability_cleanup();
    return 0;
}
```

### 错误处理示例

```c
#include "core/unified_error_handler.h"

int main() {
    // 初始化错误系统
    unified_error_system_init();
    
    // 报告错误
    ERROR_REPORT(g_unified_error_manager,
                ERROR_CORE_INVALID_PARAM,
                ERROR_SEVERITY_ERROR,
                "参数验证失败");
    
    // 打印错误摘要
    unified_error_print_summary(g_unified_error_manager);
    
    // 清理
    unified_error_system_cleanup();
    return 0;
}
```

### AI代码分析示例

```c
#include "ai/code_analyzer.h"

int main() {
    // 初始化AI分析器
    ai_analyzer_init();
    
    // 分析代码文件
    CodeAnalysisResult* result = ai_analyze_file("example.c");
    if (result) {
        printf("代码质量评分: %d/100\n", result->quality_score);
        printf("改进建议数量: %d\n", result->improvement_count);
        
        // 清理
        ai_analysis_result_free(result);
    }
    
    return 0;
}
```

## 🔧 常用工具

### 构建工具

```bash
# 构建所有模块
./build_modules_gcc.sh

# 构建核心系统
./build_core.sh

# 清理构建文件
make clean
```

### 测试工具

```bash
# 运行所有测试
./tests/run_all_tests.sh

# 运行压力测试
./tests/test_stress_module_system.sh

# 运行安全测试
./tests/test_security_validation.sh

# 分析测试覆盖率
./tests/run_coverage_analysis.sh
```

### 调试工具

```bash
# 启用详细输出
export DEBUG=1
./c99bin.sh example.c -o example

# 查看模块信息
./tools/module_info.sh layer0

# 性能分析
./tools/performance_profiler.sh example.c
```

## 🎯 最佳实践

### 代码风格

1. **使用一致的命名约定**
   - 函数: `snake_case`
   - 变量: `snake_case`
   - 常量: `UPPER_CASE`
   - 类型: `PascalCase`

2. **错误处理**
   - 始终检查返回值
   - 使用统一错误处理系统
   - 提供有意义的错误消息

3. **内存管理**
   - 每个 `malloc` 都要有对应的 `free`
   - 使用 `NULL` 检查避免段错误
   - 考虑使用内存池优化性能

### 性能优化

1. **模块使用**
   - 使用模块缓存避免重复加载
   - 预加载常用模块
   - 监控模块性能指标

2. **编译优化**
   - 使用 c99bin 获得最佳性能
   - 启用编译器优化选项
   - 使用链接时优化 (LTO)

### 调试技巧

1. **使用调试工具**
   - GDB 用于深度调试
   - Valgrind 检查内存问题
   - 项目内置的错误处理系统

2. **日志记录**
   - 使用统一的日志系统
   - 设置适当的日志级别
   - 在关键点添加日志

## 🆘 常见问题

### Q: 编译失败怎么办？

A: 检查以下几点：
1. 确保所有依赖已安装
2. 检查源码语法是否正确
3. 查看详细错误信息
4. 尝试使用混合编译模式

### Q: 模块加载失败？

A: 可能的原因：
1. 模块文件不存在或损坏
2. 权限问题
3. 依赖模块未加载
4. 架构不匹配

### Q: 性能不如预期？

A: 优化建议：
1. 使用 c99bin 编译器
2. 启用编译优化
3. 检查内存使用情况
4. 使用性能分析工具

## 📚 进一步学习

- [API Reference](API_Reference.md) - 完整的API文档
- [Architecture Guide](Architecture_Guide.md) - 系统架构详解
- [Performance Guide](Performance_Guide.md) - 性能优化指南
- [Examples](../examples/) - 更多示例代码

## 🤝 获得帮助

- 查看 [FAQ](FAQ.md)
- 阅读 [Troubleshooting Guide](Troubleshooting_Guide.md)
- 提交 [GitHub Issues](https://github.com/wanjochan/self-evolve-ai/issues)
- 参与 [讨论区](https://github.com/wanjochan/self-evolve-ai/discussions)

---

**恭喜！** 您已经完成了快速入门。现在可以开始探索 Self-Evolving AI 的强大功能了！
