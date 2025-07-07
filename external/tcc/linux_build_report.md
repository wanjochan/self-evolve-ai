# TinyCC Linux 环境构建报告

## 环境信息

**系统**: Linux cursor 6.8.0-1024-aws #26-Ubuntu SMP (x86_64 GNU/Linux)  
**编译器**: gcc (Ubuntu 14.2.0-19ubuntu2) 14.2.0  
**构建工具**: GNU Make 4.4.1  
**日期**: 2025-07-08

## 构建结果

### ✅ 成功构建的版本

1. **Windows 32位交叉编译器** (`i386-win32-tcc`)
   - 状态: ✅ 完全可用
   - 版本: tcc version 0.9.28rc bc31319 (i386 Windows)
   - 文件类型: ELF 64-bit LSB pie executable, x86-64 (Linux 原生)
   - 交叉编译测试: ✅ 成功生成 PE32 executable

2. **Linux 原生编译器** (`tcc-linux-native`)
   - 状态: ⚠️ 部分可用 (主程序正常，缺少运行时库)
   - 版本: tcc version 0.9.28rc cursor/test-tcc-compilation-on-linux-5192@bc31319* (x86_64 Linux)
   - 文件类型: ELF 64-bit LSB pie executable, x86-64

### ⚠️ 从 macOS 复制的版本状态

- `tcc-macos-arm64`: Mach-O ARM64 格式，在 Linux 上无法运行
- `tcc-i386-win32` (旧版): Mach-O ARM64 格式，已被新构建版本替换
- `tcc-x86_64-linux`: ✅ 可运行，但编译时缺少运行时库

## 交叉编译测试

### Windows 32位交叉编译

```bash
# 测试命令
./i386-win32-tcc -B.. ../../examples/test_cross_compile.c -o test.exe

# 结果
✅ 编译成功
📄 文件类型: PE32 executable (console) Intel 80386, for MS Windows
🎯 目标平台: Windows 32位
```

### Linux 原生编译

```bash
# 测试命令  
./tcc-linux-native hello.c -o hello

# 结果
❌ 缺少运行时库 (crt1.o, crti.o)
❌ 系统头文件兼容性问题
```

## 构建过程

### 1. 环境准备

```bash
# 已安装的工具
gcc --version          # ✅ 14.2.0
make --version         # ✅ 4.4.1  
sudo apt-get install file  # ✅ 安装文件类型检测工具
```

### 2. Windows 32位版本构建

```bash
cd external/tcc/src
./configure --enable-cross
cp ../build/host/conftest.c .
gcc -DC2STR conftest.c -o c2str.exe
./c2str.exe include/tccdefs.h tccdefs_.h

# 编译 Windows 32位版本
gcc -o i386-win32-tcc.o -c tcc.c \
  -DTCC_TARGET_I386 -DTCC_TARGET_PE \
  -DCONFIG_TCC_CROSSPREFIX="\"i386-win32-\"" \
  -I. -DTCC_GITHASH="\"$(git describe --always)\"" \
  -Wall -O2 -Wdeclaration-after-statement

gcc -o i386-win32-tcc i386-win32-tcc.o -lm -ldl -lpthread
```

### 3. Linux 原生版本构建

```bash
make  # 主程序构建成功，运行时库构建失败
```

## 问题和解决方案

### 1. 文件权限问题

**问题**: `Permission denied`

**解决**: 
```bash
chmod +x tcc-x86_64-linux
```

### 2. 运行时库构建失败

**问题**: 
```
/usr/include/stdio.h:28: error: include file 'bits/libc-header-start.h' not found
```

**原因**: TinyCC 与系统 glibc 头文件不完全兼容

**当前状态**: 主编译器可用，但需要运行时库支持才能编译完整程序

### 3. 跨平台文件格式

**问题**: macOS 编译的可执行文件在 Linux 上无法运行

**解决**: 在 Linux 上重新构建，生成正确的 ELF 格式

## 推荐使用

基于当前构建结果，推荐使用顺序：

1. **🥇 Windows 交叉编译**: `i386-win32-tcc` (完全可用)
2. **🥈 Linux 兼容版本**: `tcc-x86_64-linux` (基本可用)  
3. **🥉 Linux 原生版本**: `tcc-linux-native` (需要额外配置)

## 下一步计划

1. **解决运行时库问题**: 
   - 配置正确的 C 运行时库路径
   - 或使用 TinyCC 自带的最小运行时

2. **完善 Linux 原生支持**:
   - 解决系统头文件兼容性
   - 构建完整的 libtcc1.a

3. **更新测试脚本**:
   - 包含新构建的版本
   - 添加 Linux 特定的测试用例

## 文件清单

### 新生成的文件

- `src/i386-win32-tcc` - Windows 32位交叉编译器源码版本
- `dist/bin/i386-win32-tcc` - Windows 32位交叉编译器 (Linux 构建)
- `dist/bin/tcc-linux-native` - Linux 原生编译器
- `dist/bin/test_linux_to_win32.exe` - 交叉编译测试结果

### 支持文件

- `dist/lib/i386-win32-libtcc1.a` - Windows 32位运行时库
- `dist/lib/*.def` - Windows API 定义文件
- `dist/include/*` - Windows 头文件

---

**总结**: Linux 环境下的 TinyCC 构建基本成功，Windows 32位交叉编译功能完全可用，Linux 原生编译需要进一步配置运行时库支持。