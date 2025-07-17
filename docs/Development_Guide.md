# Development Guide

## 开发环境搭建

### 系统要求

**最低要求:**
- Linux/Unix 系统 (Ubuntu 18.04+, CentOS 7+, macOS 10.14+)
- GCC 4.8+ 或 Clang 6.0+
- GNU Make 3.81+
- Bash 4.0+
- 至少 2GB 可用磁盘空间
- 至少 1GB 可用内存

**推荐配置:**
- Ubuntu 20.04+ 或 CentOS 8+
- GCC 9.0+ 或 Clang 10.0+
- 4GB+ 可用内存
- SSD 存储

### 依赖安装

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install build-essential gcc make bash git
sudo apt install libdl-dev  # 动态链接库支持
```

**CentOS/RHEL:**
```bash
sudo yum groupinstall "Development Tools"
sudo yum install gcc make bash git
sudo yum install glibc-devel
```

**macOS:**
```bash
# 安装 Xcode Command Line Tools
xcode-select --install

# 或使用 Homebrew
brew install gcc make bash git
```

## 项目结构

```
self-evolve-ai/
├── src/                    # 源代码
│   ├── core/              # 核心系统
│   │   ├── astc.h/c       # ASTC字节码系统
│   │   ├── module.h/c     # 模块系统
│   │   ├── module_stability.h/c  # 模块稳定性
│   │   └── unified_error_handler.h/c  # 统一错误处理
│   ├── c99/               # C99编译器
│   │   ├── frontend/      # 前端 (词法/语法/语义分析)
│   │   ├── backend/       # 后端 (代码生成/优化)
│   │   └── stdlib/        # C99标准库
│   └── ai/                # AI功能
│       ├── code_analyzer.h/c     # 代码分析
│       └── evolution_core_loop.h/c  # 进化循环
├── bin/                   # 编译输出
│   └── layer2/           # 模块文件 (*.so)
├── tests/                # 测试套件
├── examples/             # 示例代码
├── docs/                 # 文档
├── tools/                # 工具脚本
└── helpers/              # 辅助工具
```

## 开发工作流

### 1. 代码开发

**分支策略:**
- `main`: 稳定版本
- `develop`: 开发版本
- `feature/*`: 功能分支
- `hotfix/*`: 紧急修复

**开发步骤:**
```bash
# 1. 创建功能分支
git checkout -b feature/new-feature

# 2. 开发代码
# ... 编写代码 ...

# 3. 构建和测试
./build_modules_gcc.sh
./tests/run_all_tests.sh

# 4. 提交代码
git add .
git commit -m "feat: add new feature"

# 5. 推送分支
git push origin feature/new-feature

# 6. 创建 Pull Request
```

### 2. 代码规范

**命名约定:**
```c
// 函数: snake_case
int module_load(const char* name);

// 变量: snake_case
int error_count = 0;

// 常量: UPPER_CASE
#define MAX_MODULES 100

// 类型: PascalCase
typedef struct ModuleInfo {
    char* name;
    void* handle;
} ModuleInfo;

// 宏: UPPER_CASE
#define ERROR_REPORT(manager, code, severity, msg) \
    unified_error_report(manager, code, severity, __FILE__, __LINE__, __func__, msg, NULL, NULL)
```

**代码风格:**
```c
// 1. 缩进使用4个空格
if (condition) {
    do_something();
}

// 2. 大括号换行
int function_name(int param)
{
    // 函数体
    return 0;
}

// 3. 指针声明
char* ptr;          // 推荐
char *ptr;          // 不推荐

// 4. 注释风格
/**
 * 函数说明
 * @param param 参数说明
 * @return 返回值说明
 */
int function(int param);

// 单行注释
int value = 42;     // 行末注释
```

### 3. 错误处理

**统一错误处理:**
```c
#include "core/unified_error_handler.h"

int my_function(const char* param) {
    // 参数验证
    if (!param) {
        ERROR_REPORT(g_unified_error_manager,
                    ERROR_CORE_INVALID_PARAM,
                    ERROR_SEVERITY_ERROR,
                    "参数不能为空");
        return -1;
    }
    
    // 业务逻辑
    if (some_operation_failed()) {
        ERROR_REPORT_WITH_SUGGESTION(g_unified_error_manager,
                                     ERROR_CORE_OPERATION_FAILED,
                                     ERROR_SEVERITY_ERROR,
                                     "操作失败",
                                     "检查系统资源是否充足");
        return -1;
    }
    
    return 0;
}
```

**错误恢复:**
```c
// 设置恢复策略
UnifiedError* error = ERROR_REPORT(manager, code, severity, message);
if (error) {
    unified_error_set_recovery_strategy(error, ERROR_RECOVERY_RETRY, 3);
}

// 尝试恢复
bool recovered = unified_error_attempt_recovery(manager, error);
```

### 4. 内存管理

**基本原则:**
```c
// 1. 每个 malloc 都要有对应的 free
char* buffer = malloc(size);
if (!buffer) {
    ERROR_REPORT(manager, ERROR_CORE_OUT_OF_MEMORY, 
                ERROR_SEVERITY_CRITICAL, "内存分配失败");
    return -1;
}
// ... 使用 buffer ...
free(buffer);
buffer = NULL;  // 避免悬空指针

// 2. 检查返回值
FILE* file = fopen("test.txt", "r");
if (!file) {
    ERROR_REPORT(manager, ERROR_IO_FILE_NOT_FOUND,
                ERROR_SEVERITY_ERROR, "文件打开失败");
    return -1;
}
// ... 使用 file ...
fclose(file);

// 3. 使用安全的字符串函数
char dest[100];
strncpy(dest, src, sizeof(dest) - 1);
dest[sizeof(dest) - 1] = '\0';
```

## 测试开发

### 1. 单元测试

**测试框架使用:**
```c
#include "tests/core_test_framework.h"

TEST_CASE(test_module_loading) {
    // 测试模块加载
    void* handle = stable_module_load("layer0");
    ASSERT_TRUE(handle != NULL, "模块加载成功");
    
    // 测试符号解析
    void* symbol = stable_module_resolve("layer0", "some_function");
    ASSERT_TRUE(symbol != NULL, "符号解析成功");
    
    return true;
}

int main() {
    test_framework_init(false);
    
    TEST_SUITE_START("Module Tests");
    RUN_TEST(test_module_loading);
    TEST_SUITE_END();
    
    test_framework_print_summary();
    return test_framework_all_passed() ? 0 : 1;
}
```

### 2. 集成测试

**Shell脚本测试:**
```bash
#!/bin/bash
# test_integration.sh

set -e

echo "=== 集成测试 ==="

# 构建系统
./build_modules_gcc.sh

# 测试编译
echo "测试编译功能..."
echo 'int main() { return 42; }' > test.c
./c99bin.sh test.c -o test
./test
result=$?
if [ $result -eq 42 ]; then
    echo "✅ 编译测试通过"
else
    echo "❌ 编译测试失败"
    exit 1
fi

# 清理
rm -f test.c test

echo "✅ 集成测试完成"
```

### 3. 性能测试

**性能基准测试:**
```c
#include <time.h>

void benchmark_module_loading() {
    const int iterations = 1000;
    clock_t start = clock();
    
    for (int i = 0; i < iterations; i++) {
        void* handle = stable_module_load("layer0");
        if (!handle) {
            printf("模块加载失败\n");
            return;
        }
    }
    
    clock_t end = clock();
    double time_spent = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("模块加载性能:\n");
    printf("  迭代次数: %d\n", iterations);
    printf("  总时间: %.3f 秒\n", time_spent);
    printf("  平均时间: %.3f 毫秒\n", (time_spent * 1000) / iterations);
}
```

## 调试技巧

### 1. 使用 GDB

```bash
# 编译时添加调试信息
gcc -g -O0 source.c -o program

# 启动 GDB
gdb ./program

# GDB 常用命令
(gdb) break main          # 设置断点
(gdb) run                 # 运行程序
(gdb) step                # 单步执行
(gdb) print variable      # 打印变量
(gdb) backtrace          # 查看调用栈
(gdb) continue           # 继续执行
```

### 2. 内存检查

```bash
# 使用 Valgrind 检查内存泄漏
valgrind --leak-check=full ./program

# 使用 AddressSanitizer
gcc -fsanitize=address -g source.c -o program
./program
```

### 3. 日志调试

```c
// 使用统一错误系统进行调试
#ifdef DEBUG
    ERROR_REPORT(g_unified_error_manager,
                ERROR_CORE_DEBUG_INFO,
                ERROR_SEVERITY_DEBUG,
                "调试信息: 变量值为 %d", value);
#endif

// 条件编译调试代码
#ifdef DEBUG
    printf("DEBUG: 进入函数 %s\n", __func__);
#endif
```

## 性能优化

### 1. 编译优化

```bash
# 发布版本编译选项
gcc -O3 -DNDEBUG -march=native source.c -o program

# 链接时优化
gcc -O3 -flto source.c -o program
```

### 2. 代码优化

**避免频繁内存分配:**
```c
// 不好的做法
for (int i = 0; i < 1000; i++) {
    char* buffer = malloc(100);
    // ... 使用 buffer ...
    free(buffer);
}

// 好的做法
char* buffer = malloc(100);
for (int i = 0; i < 1000; i++) {
    // ... 重复使用 buffer ...
}
free(buffer);
```

**使用缓存:**
```c
// 模块缓存示例
static void* cached_modules[MAX_MODULES];
static char* cached_names[MAX_MODULES];
static int cache_count = 0;

void* get_cached_module(const char* name) {
    for (int i = 0; i < cache_count; i++) {
        if (strcmp(cached_names[i], name) == 0) {
            return cached_modules[i];
        }
    }
    return NULL;
}
```

## 发布流程

### 1. 版本管理

```bash
# 创建发布分支
git checkout -b release/v1.0.0

# 更新版本号
echo "1.0.0" > VERSION

# 运行完整测试
./tests/run_all_tests.sh

# 构建发布版本
make clean
make release

# 创建标签
git tag -a v1.0.0 -m "Release version 1.0.0"
git push origin v1.0.0
```

### 2. 文档更新

- 更新 CHANGELOG.md
- 更新 API 文档
- 更新示例代码
- 检查文档链接

### 3. 质量检查

```bash
# 代码质量检查
./tools/code_quality_check.sh

# 性能回归测试
./tests/performance_regression_test.sh

# 安全扫描
./tests/security_scan.sh
```

## 贡献指南

### 1. 提交规范

**提交消息格式:**
```
<type>(<scope>): <subject>

<body>

<footer>
```

**类型说明:**
- `feat`: 新功能
- `fix`: 错误修复
- `docs`: 文档更新
- `style`: 代码格式
- `refactor`: 重构
- `test`: 测试相关
- `chore`: 构建/工具相关

**示例:**
```
feat(core): add unified error handling system

- Implement comprehensive error classification
- Add intelligent error recovery mechanisms
- Support custom error and recovery handlers
- Include detailed error statistics and analysis

Closes #123
```

### 2. 代码审查

**审查要点:**
- 代码功能正确性
- 错误处理完整性
- 内存管理安全性
- 性能影响评估
- 文档完整性
- 测试覆盖率

### 3. 问题报告

**Bug 报告模板:**
```markdown
## 问题描述
简要描述问题

## 重现步骤
1. 步骤1
2. 步骤2
3. 步骤3

## 预期行为
描述预期的正确行为

## 实际行为
描述实际发生的错误行为

## 环境信息
- 操作系统: Ubuntu 20.04
- 编译器: GCC 9.4.0
- 项目版本: v1.0.0

## 附加信息
其他相关信息
```

---

**祝您开发愉快！** 如有问题，请查看 [FAQ](FAQ.md) 或提交 [Issue](https://github.com/wanjochan/self-evolve-ai/issues)。
