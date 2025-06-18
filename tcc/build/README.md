# TCC 交叉编译器构建产物 (重新组织版)

## 目录结构

```
build/
├── host/                   # 主机版本 TCC
│   ├── bin/tcc            # 主机 TCC 编译器 ✅
│   ├── lib/               # 运行时库
│   ├── include/           # 头文件
│   └── share/             # 文档
├── cross/                 # 交叉编译器
│   ├── x86_64-linux/     # x86_64 Linux 目标 ✅
│   │   ├── bin/tcc-x86_64-linux
│   │   ├── lib/
│   │   └── include/
│   ├── aarch64-linux/    # ARM64 Linux 目标 (库文件和头文件)
│   └── arm-linux/        # ARM Linux 目标 (库文件和头文件)
├── test.c                # 测试程序
└── README.md             # 本文件
```

## 可用的编译器

### ✅ 主机编译器
- **位置**: `build/host/bin/tcc`
- **状态**: 完全可用
- **用法**: 编译本机 x86_64 程序

### ✅ x86_64-linux 交叉编译器
- **位置**: `build/cross/x86_64-linux/bin/tcc-x86_64-linux`
- **状态**: 完全可用，已测试
- **用法**: 编译 x86_64 Linux 程序

### ⚠️ 其他架构
- **aarch64-linux**: 有库文件和头文件，但缺少编译器可执行文件
- **arm-linux**: 有库文件和头文件，但缺少编译器可执行文件

## 使用方法

### 编译 x86_64 Linux 程序
```bash
# 使用交叉编译器
build/cross/x86_64-linux/bin/tcc-x86_64-linux hello.c -o hello

# 或者使用主机编译器 (如果在 x86_64 系统上)
build/host/bin/tcc hello.c -o hello
```

### 快速测试
```bash
# 测试主机编译器
build/host/bin/tcc build/test.c -o test_host && ./test_host

# 测试 x86_64 交叉编译器
build/cross/x86_64-linux/bin/tcc-x86_64-linux build/test.c -o test_cross && ./test_cross
```

## 验证编译器

运行测试脚本：
```bash
bash scripts/test-tcc-organized.sh
```

## 构建状态

- **重新组织时间**: $(date)
- **主机编译器**: ✅ 可用
- **x86_64-linux 交叉编译器**: ✅ 可用并已测试
- **其他架构**: ⚠️ 部分可用 (需要重新构建编译器可执行文件)

## 目录结构优化完成

✅ **问题已解决**: TCC 的交叉编译产物已经按架构分类重新组织，目录结构清晰：

- **主机编译器** → `build/host/`
- **交叉编译器** → `build/cross/{架构}/`
- **按架构分离** → 每个架构有独立的 `bin/`, `lib/`, `include/` 目录
- **命名规范** → 编译器命名为 `tcc-{架构}`

## 下一步建议

如需完整的多架构支持，可以：

1. 运行完整的交叉编译构建脚本:
   ```bash
   bash scripts/build-cross-compilers.sh
   ```

2. 或针对特定架构重新构建:
   ```bash
   # 进入TCC源码目录
   cd /path/to/tcc/src
   ./configure --cross-prefix=aarch64-linux-gnu-
   make clean && make
   cp tcc ../build/cross/aarch64-linux/bin/tcc-aarch64-linux
   ```

当前可用的编译器已足够进行基本的开发和测试工作。
