# TinyCC (TCC) 使用文档

## 概述

TinyCC 是一个轻量级的 C 编译器，支持 C99 标准的大部分特性。在我们自己的 C99 编译器完成之前，TinyCC 作为临时解决方案提供交叉编译能力。

## 目录结构

```
external/tcc/
├── dist/bin/           # 编译好的可执行文件
├── src/                # TinyCC 源代码
├── build/              # 构建产物
├── scripts/            # 构建脚本
└── cross/              # 交叉编译相关
```

## 可用的编译器版本

### 1. 主要可执行文件

| 文件名 | 架构 | 平台 | 状态 | 说明 |
|--------|------|------|------|------|
| `tcc-macos-arm64` | ARM64 | macOS | ✅ 可用 | Apple Silicon Mac 原生版本 |
| `x86_64-tcc` | x86_64 | macOS | ⚠️ 部分可用 | Intel Mac 原生版本（需要运行时库） |
| `tcc-x86_64-linux` | x86_64 | Linux | ⚠️ 部分可用 | Linux 64位版本（需要运行时库） |
| `tcc-host` | x86_64 | Linux | ⚠️ 部分可用 | 主机版本（通用） |
| `tcc-i386-win32` | i386 | Windows | ✅ 可用 | **Windows 32位版本**（新增，完全配置） |

### 2. 智能选择脚本

| 脚本名 | 功能 |
|--------|------|
| `tcc` | 根据当前系统自动选择合适的版本 |
| `tcc-win32` | Windows 版本选择（优先32位） |
| `tcc-linux` | Linux 版本选择脚本 |
| `tcc-macos` | macOS 版本选择脚本 |

### 3. 推荐使用

根据当前配置状态，推荐使用以下版本：

**🥇 首选版本**:
- **Windows 交叉编译**: `tcc-i386-win32` 或 `tcc-win32` (完全配置，包含所需库文件)
- **macOS 开发**: `tcc-macos-arm64` (在 Apple Silicon Mac 上，适合 JIT 模式)

**⚠️ 需要额外配置**:
- 其他版本可能需要额外的运行时库配置才能正常工作

## 快速开始

### 验证安装

```bash
# 进入 TCC 目录
cd external/tcc/dist/bin

# 检查可用版本
ls -la tcc-*

# 测试 Windows 32位编译器
./tcc-i386-win32 -v
# 输出: tcc version 0.9.28rc 2025-07-08 main_local@89c6e60* (i386 Windows)

# 测试 macOS ARM64 编译器
./tcc-macos-arm64 -v
# 输出: tcc version 0.9.28rc 2025-07-08 main_local@89c6e60* (AArch64 Darwin)
```

### 第一个程序

```bash
# 创建测试文件
cat > hello.c << 'EOF'
#include <stdio.h>
int main() {
    printf("Hello from TinyCC!\n");
    return 0;
}
EOF

# Windows 32位交叉编译
./tcc-i386-win32 -B.. hello.c -o hello.exe
file hello.exe  # 验证: PE32 executable (console) Intel 80386

# macOS 编译（JIT 模式，直接运行）
./tcc-macos-arm64 -run hello.c
```

## 使用方法

### 基本用法

```bash
# 查看版本信息
./tcc-macos-arm64 -v

# 编译 C 文件
./tcc-macos-arm64 hello.c -o hello

# 直接运行（JIT 模式）
./tcc-macos-arm64 -run hello.c
```

### 交叉编译

#### 编译 Windows 32位程序

```bash
# 使用 Windows 32位编译器（需要指定基础路径）
cd external/tcc/dist/bin
./tcc-i386-win32 -B.. ../../examples/test_cross_compile.c -o hello.exe

# 使用智能选择脚本
./tcc-win32 -B.. ../../examples/test_cross_compile.c -o hello.exe

# 验证生成的文件
file hello.exe
# 输出: hello.exe: PE32 executable (console) Intel 80386, for MS Windows
```

#### 编译 Linux 程序

```bash
# 在 macOS 上编译 Linux 程序
./tcc-x86_64-linux hello.c -o hello_linux
```

#### 重要说明

**基础路径设置**: 交叉编译时必须使用 `-B` 参数指定基础路径，以便编译器找到头文件和库文件：
- `-B..` : 当在 `dist/bin/` 目录中时使用
- `-B../..` : 当在其他位置时可能需要调整

## 版本信息对照

通过 `-v` 参数可以查看详细的版本信息：

```bash
# macOS ARM64
$ ./tcc-macos-arm64 -v
tcc version 0.9.28rc 2025-07-08 main_local@89c6e60* (AArch64 Darwin)

# Windows 32位
$ ./tcc-i386-win32 -v  
tcc version 0.9.28rc 2025-07-08 main_local@89c6e60* (i386 Windows)

# Linux x86_64
$ ./tcc-x86_64-linux -v
tcc version 0.9.28rc 2025-07-08 main_local@89c6e60* (x86_64 Linux)
```

## 构建新版本

### 前提条件

- 已有的 TinyCC 编译器（用作宿主编译器）
- 源代码目录：`external/tcc/src/`

### 构建步骤

1. **配置构建环境**
   ```bash
   cd external/tcc/src
   
   # 修复换行符问题（如果需要）
   sed -i 's/\r$//' configure  # Linux 上使用 sed -i
   # sed -i '' 's/\r$//' configure  # macOS 上使用 sed -i ''
   
   # 运行配置
   ./configure --enable-cross
   ```

2. **生成必要文件**
   ```bash
   # 复制 conftest.c（如果不存在）
   if [ ! -f conftest.c ]; then
       cp ../build/host/conftest.c . || echo "需要手动创建 conftest.c"
   fi
   
   # 生成 tccdefs_.h
   # Linux 上通常使用 gcc
   gcc -DC2STR conftest.c -o c2str.exe  # Linux
   # clang -DC2STR conftest.c -o c2str.exe  # macOS
   ./c2str.exe include/tccdefs.h tccdefs_.h
   ```

3. **构建特定架构版本**
   ```bash
   # 构建 i386 Linux 版本
   make cross-i386
   
   # 构建 Windows 32位版本
   clang -o i386-win32-tcc.o -c tcc.c \
     -DTCC_TARGET_I386 -DTCC_TARGET_PE \
     -DCONFIG_TCC_CROSSPREFIX="\"i386-win32-\"" \
     -I. -DTCC_GITHASH="\"$(git describe --always)\"" \
     -Wall -O2 -Wdeclaration-after-statement
   
   # Linux 上的链接命令
   gcc -o i386-win32-tcc i386-win32-tcc.o -lm -ldl -lpthread  # Linux
   # clang -o i386-win32-tcc i386-win32-tcc.o \
   #   -lm -ldl -lpthread -flat_namespace -undefined warning  # macOS
   ```

### Linux 特定注意事项

在 Linux 系统上构建和使用 TinyCC 时的特殊考虑：

1. **编译器选择**
   ```bash
   # Linux 上推荐使用 gcc
   which gcc || sudo apt-get install gcc  # Ubuntu/Debian
   which gcc || sudo yum install gcc      # CentOS/RHEL
   ```

2. **依赖库**
   ```bash
   # 安装必要的开发工具
   sudo apt-get install build-essential  # Ubuntu/Debian
   sudo yum groupinstall "Development Tools"  # CentOS/RHEL
   ```

3. **链接参数差异**
   - Linux: 不需要 `-flat_namespace -undefined warning`
   - macOS: 需要这些参数来处理动态链接

4. **路径分隔符**
   - Linux: 使用 `sed -i` (不需要空字符串参数)
   - macOS: 使用 `sed -i ''` (需要空字符串参数)

### 添加新架构支持

要添加新的目标架构，需要：

1. **修改 Makefile**
   - 在 `TARGETS` 数组中添加新架构
   - 定义相应的编译参数

2. **编译目标文件**
   ```bash
   clang -o <arch>-<platform>-tcc.o -c tcc.c \
     -DTCC_TARGET_<ARCH> -DTCC_TARGET_<PLATFORM> \
     -DCONFIG_TCC_CROSSPREFIX="\"<arch>-<platform>-\"" \
     -I. -DTCC_GITHASH="\"$(git describe --always)\"" \
     -Wall -O2 -Wdeclaration-after-statement
   ```

3. **链接生成可执行文件**
   ```bash
   # Linux
   gcc -o <arch>-<platform>-tcc <arch>-<platform>-tcc.o -lm -ldl -lpthread
   
   # macOS
   clang -o <arch>-<platform>-tcc <arch>-<platform>-tcc.o \
     -lm -ldl -lpthread -flat_namespace -undefined warning
   ```

### Linux 环境验证

在 Linux 上首次设置后，建议运行以下验证：

```bash
# 检查系统环境
echo "系统信息:"
uname -a
gcc --version
make --version

# 检查 TinyCC 状态
cd external/tcc/dist/bin
echo -e "\n可用的 TinyCC 版本:"
ls -la tcc-* 2>/dev/null || echo "没有找到 TinyCC 可执行文件"

# 运行测试脚本
echo -e "\n运行自动化测试:"
cd ../..
if [ -f test_all_versions.sh ]; then
    ./test_all_versions.sh
else
    echo "测试脚本不存在，需要从 macOS 环境复制"
fi
```

## 常见问题

### 1. 找不到头文件

**问题**: `error: include file 'stdio.h' not found`

**解决**: 使用 `-B` 参数指定基础路径
```bash
# 正确的方式
./tcc-i386-win32 -B.. hello.c -o hello.exe

# 或者手动指定头文件路径
./tcc-macos-arm64 -I./include hello.c -o hello
```

### 2. 找不到库文件

**问题**: `error: file 'libtcc1.a' not found` 或 `error: library 'msvcrt' not found`

**解决**: 
- 对于 Windows 交叉编译：确保使用 `-B` 参数，并且 `dist/lib/` 目录包含必要的 `.def` 文件
- 对于 macOS/Linux：确保运行时库在正确位置

### 3. 交叉编译失败

**问题**: `can't cross compile long double constants`

**解决**: 这是已知限制，某些复杂的长双精度常量无法交叉编译。可以：
- 简化代码中的长双精度使用
- 使用目标平台的原生编译器

### 4. 权限问题

**问题**: `Permission denied`

**解决**: 确保可执行文件有执行权限
```bash
chmod +x tcc-*
```

### 5. Windows 可执行文件无法运行

**问题**: 生成的 `.exe` 文件在 macOS/Linux 上无法运行

**解决**: 这是正常现象。交叉编译生成的是目标平台的可执行文件：
- Windows 32位程序只能在 Windows 系统上运行
- 可以使用 Wine 或虚拟机进行测试

### 6. Linux 特定问题

**问题**: `configure: command not found` 或权限错误

**解决**: 
```bash
# 确保文件有执行权限
chmod +x configure

# 如果是换行符问题
sed -i 's/\r$//' configure
```

**问题**: `gcc: command not found`

**解决**: 安装开发工具
```bash
# Ubuntu/Debian
sudo apt-get update && sudo apt-get install build-essential

# CentOS/RHEL/Fedora
sudo yum groupinstall "Development Tools"
# 或新版本
sudo dnf groupinstall "Development Tools"
```

**问题**: 在 Linux 上编译的程序在 macOS 上无法运行

**解决**: 这是正常的交叉编译行为：
- Linux 编译的程序生成 ELF 格式，只能在 Linux 上运行
- macOS 编译的程序生成 Mach-O 格式，只能在 macOS 上运行
- 使用对应平台的编译器版本

## 支持的 C 特性

TinyCC 支持 C99 标准的大部分特性：

- ✅ 基本数据类型和操作符
- ✅ 函数和变量声明
- ✅ 结构体和联合体
- ✅ 指针和数组
- ✅ 预处理器指令
- ✅ 内联汇编（限制性支持）
- ⚠️ 某些 C11 特性（部分支持）
- ❌ C++ 语法（不支持）

## 性能特点

- **编译速度**: 极快，适合快速原型开发
- **生成代码**: 优化程度一般，但足够日常使用
- **内存占用**: 很小，适合嵌入式环境
- **启动时间**: 几乎瞬时启动

## 与其他编译器的比较

| 特性 | TinyCC | GCC | Clang |
|------|--------|-----|-------|
| 编译速度 | 极快 | 慢 | 中等 |
| 代码优化 | 基本 | 强 | 强 |
| 错误信息 | 简单 | 详细 | 详细 |
| 标准支持 | C99 | C11/C17 | C11/C17 |
| 平台支持 | 多平台 | 最广泛 | 广泛 |

## 迁移计划

> **注意**: TinyCC 是临时解决方案。一旦我们自己的 C99 编译器完成，将逐步迁移：
> 
> 1. **阶段1**: 使用 TinyCC 进行原型开发和测试
> 2. **阶段2**: 并行开发自己的 C99 编译器
> 3. **阶段3**: 逐步替换 TinyCC 的使用场景
> 4. **阶段4**: 完全迁移到自研编译器

## 参考资源

- [TinyCC 官方文档](http://fabrice.bellard.free.fr/tcc/)
- [TinyCC GitHub 仓库](https://github.com/TinyCC/tinycc)
- [C99 标准参考](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf)

## 测试验证

项目包含自动化测试脚本来验证所有版本：

```bash
# 运行完整测试
./test_all_versions.sh

# 预期输出示例
🔧 TinyCC 版本测试脚本
=======================
🧪 测试 tcc-i386-win32...
   ✅ 版本: tcc version 0.9.28rc (i386 Windows)
   ✅ 编译成功: test_win32.exe
   📄 文件类型: PE32 executable (console) Intel 80386

📊 测试结果: 3/5 成功
```

测试脚本会：
- 检查所有编译器的版本信息
- 尝试编译测试程序
- 验证生成文件的格式
- 提供详细的成功/失败报告

## 更新日志

- **2025-07-08**: 
  - 添加 Windows 32位版本支持 (`tcc-i386-win32`)
  - 创建完整的使用文档和测试脚本
  - 配置 Windows 交叉编译环境（头文件和库文件）
- **2025-06-28**: 初始版本，支持 macOS ARM64/x86_64 和 Linux x86_64

---

*文档维护者: 开发团队*  
*最后更新: 2025-07-08* 