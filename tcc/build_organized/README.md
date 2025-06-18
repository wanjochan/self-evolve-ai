# TCC 重新组织的构建产物

## 目录结构

```
build/
├── host/                   # 主机版本 TCC
│   ├── bin/tcc            # 主机 TCC 编译器
│   ├── lib/               # 运行时库
│   ├── include/           # 头文件
│   └── share/             # 文档
├── cross/                 # 交叉编译器
│   ├── x86_64-linux/     # x86_64 Linux 目标
│   │   ├── bin/tcc-x86_64-linux
│   │   ├── lib/
│   │   └── include/
│   ├── aarch64-linux/    # ARM64 Linux 目标
│   ├── arm-linux/        # ARM Linux 目标
│   └── ...               # 其他架构
└── README.md             # 本文件
```

## 使用方法

### 编译 x86_64 Linux 程序
```bash
build/cross/x86_64-linux/bin/tcc-x86_64-linux hello.c -o hello
```

### 编译 ARM64 程序
```bash
build/cross/aarch64-linux/bin/tcc-aarch64-linux hello.c -o hello-arm64
```

### 使用主机编译器
```bash
build/host/bin/tcc hello.c -o hello
```

## 验证编译器

运行测试脚本：
```bash
bash scripts/test-tcc-organized.sh
```

## 支持的架构

- x86_64-linux

重新组织时间: Wed Jun 18 11:53:02 AM UTC 2025
