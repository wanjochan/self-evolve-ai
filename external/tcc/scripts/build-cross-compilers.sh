#!/bin/bash
# TCC 交叉编译器构建脚本 - 重新组织版本
# 按架构分类组织构建产物

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TCC_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$TCC_DIR/build"

echo "🔧 TCC 交叉编译器构建脚本 v2.0"
echo "=================================================="

# 清理旧的构建产物
echo "🧹 清理旧的构建产物..."
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

# 定义目标架构配置
declare -A TARGETS=(
    ["x86_64-linux"]="x86_64 linux gnu"
    ["x86_64-windows"]="x86_64 windows pe"
    ["i386-linux"]="i386 linux gnu"
    ["i386-windows"]="i386 windows pe"
    ["aarch64-linux"]="aarch64 linux gnu"
    ["arm-linux"]="arm linux gnu"
    ["riscv64-linux"]="riscv64 linux gnu"
    ["mips-linux"]="mips linux gnu"
    ["mips64-linux"]="mips64 linux gnu"
    ["powerpc-linux"]="powerpc linux gnu"
    ["powerpc64-linux"]="powerpc64 linux gnu"
    ["s390x-linux"]="s390x linux gnu"
)

# 构建主机版本 (host)
build_host() {
    echo "🏠 构建主机版本..."
    local HOST_DIR="$BUILD_DIR/host"
    mkdir -p "$HOST_DIR"
    
    cd "$TCC_DIR"
    ./configure --prefix="$HOST_DIR" \
                --enable-cross \
                --config-musl \
                --strip-binaries
    
    make clean
    make -j$(nproc)
    make install
    
    echo "✅ 主机版本构建完成: $HOST_DIR"
}

# 构建交叉编译器
build_cross_compiler() {
    local target_full="$1"
    local arch="$2"
    local os="$3"
    local abi="$4"
    
    echo "🎯 构建 $target_full 交叉编译器..."
    
    # 创建目标目录结构
    local TARGET_DIR="$BUILD_DIR/cross/$arch-$os"
    mkdir -p "$TARGET_DIR/bin"
    mkdir -p "$TARGET_DIR/lib"
    mkdir -p "$TARGET_DIR/include"
    mkdir -p "$TARGET_DIR/share"
    
    cd "$TCC_DIR"
    
    # 配置交叉编译
    case "$os" in
        "linux")
            ./configure --prefix="$TARGET_DIR" \
                       --cross-prefix="$target_full-" \
                       --cpu="$arch" \
                       --strip-binaries \
                       --sysroot="$TARGET_DIR" \
                       --config-musl
            ;;
        "windows")
            ./configure --prefix="$TARGET_DIR" \
                       --cross-prefix="$target_full-" \
                       --cpu="$arch" \
                       --strip-binaries \
                       --enable-mingw32
            ;;
    esac
    
    # 编译
    make clean
    make cross-$arch 2>/dev/null || make -j$(nproc)
    
    # 复制编译器到目标位置
    if [ -f "tcc" ]; then
        cp tcc "$TARGET_DIR/bin/tcc-$target_full"
        chmod +x "$TARGET_DIR/bin/tcc-$target_full"
    fi
    
    # 复制运行时库
    if [ -d "lib" ]; then
        cp -r lib/* "$TARGET_DIR/lib/" 2>/dev/null || true
    fi
    
    # 复制头文件
    if [ -d "include" ]; then
        cp -r include/* "$TARGET_DIR/include/" 2>/dev/null || true
    fi
    
    echo "✅ $target_full 交叉编译器构建完成"
}

# 构建测试程序
build_test_program() {
    echo "🧪 准备测试程序..."
    
    cat > "$BUILD_DIR/test.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    printf("Hello from TCC cross-compiler!\n");
    printf("Architecture: %s\n", 
#ifdef __x86_64__
        "x86_64"
#elif defined(__i386__)
        "i386"
#elif defined(__aarch64__)
        "aarch64"
#elif defined(__arm__)
        "arm"
#elif defined(__riscv) && __riscv_xlen == 64
        "riscv64"
#elif defined(__mips64)
        "mips64"
#elif defined(__mips__)
        "mips"
#elif defined(__powerpc64__)
        "powerpc64"
#elif defined(__powerpc__)
        "powerpc"
#elif defined(__s390x__)
        "s390x"
#else
        "unknown"
#endif
    );
    printf("Compiler: TCC\n");
    return 0;
}
EOF
}

# 测试交叉编译器
test_cross_compilers() {
    echo "🧪 测试交叉编译器..."
    build_test_program
    
    local success_count=0
    local total_count=0
    
    for compiler_path in "$BUILD_DIR"/cross/*/bin/tcc-*; do
        if [ -x "$compiler_path" ]; then
            local compiler_name=$(basename "$compiler_path")
            local arch_os=$(echo "$compiler_name" | sed 's/tcc-//')
            
            echo "  测试 $compiler_name..."
            total_count=$((total_count + 1))
            
            if "$compiler_path" "$BUILD_DIR/test.c" -o "$BUILD_DIR/test-$arch_os" 2>/dev/null; then
                echo "    ✅ $arch_os: 编译成功"
                success_count=$((success_count + 1))
            else
                echo "    ❌ $arch_os: 编译失败"
            fi
        fi
    done
    
    echo "📊 测试结果: $success_count/$total_count 成功"
}

# 生成使用说明
generate_usage_info() {
    echo "📖 生成使用说明..."
    
    cat > "$BUILD_DIR/README.md" << 'EOF'
# TCC 交叉编译器构建产物

## 目录结构

```
build/
├── host/                   # 主机版本 TCC
│   ├── bin/tcc            # 主机 TCC 编译器
│   ├── lib/               # 运行时库
│   └── include/           # 头文件
├── cross/                 # 交叉编译器
│   ├── x86_64-linux/     # x86_64 Linux 目标
│   │   ├── bin/tcc-x86_64-linux
│   │   ├── lib/
│   │   └── include/
│   ├── x86_64-windows/   # x86_64 Windows 目标
│   ├── aarch64-linux/    # ARM64 Linux 目标
│   └── ...               # 其他架构
└── test.c                # 测试程序
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

### 验证编译器
```bash
# 运行测试
bash scripts/test-tcc-builds.sh
```

## 支持的目标架构

- x86_64-linux
- x86_64-windows  
- i386-linux
- i386-windows
- aarch64-linux (ARM64)
- arm-linux
- riscv64-linux
- mips-linux
- mips64-linux
- powerpc-linux
- powerpc64-linux
- s390x-linux

EOF
}

# 主构建流程
main() {
    cd "$TCC_DIR"
    
    echo "📁 构建目录: $BUILD_DIR"
    echo "🔧 源码目录: $TCC_DIR"
    echo ""
    
    # 1. 构建主机版本
    build_host
    
    echo ""
    echo "🎯 开始构建交叉编译器..."
    
    # 2. 构建交叉编译器 (优先构建常用架构)
    PRIORITY_TARGETS=("x86_64-linux" "aarch64-linux" "x86_64-windows" "i386-linux")
    
    for target in "${PRIORITY_TARGETS[@]}"; do
        if [[ -n "${TARGETS[$target]}" ]]; then
            read -r arch os abi <<< "${TARGETS[$target]}"
            build_cross_compiler "$target" "$arch" "$os" "$abi"
        fi
    done
    
    # 3. 构建其他架构 (如果时间允许)
    echo ""
    echo "🔄 构建额外架构 (可选)..."
    
    for target in "${!TARGETS[@]}"; do
        # 跳过已构建的优先目标
        if [[ " ${PRIORITY_TARGETS[@]} " =~ " $target " ]]; then
            continue
        fi
        
        read -r arch os abi <<< "${TARGETS[$target]}"
        echo "  构建 $target (可选)..."
        if ! build_cross_compiler "$target" "$arch" "$os" "$abi"; then
            echo "    ⚠️ $target 构建失败，跳过"
        fi
    done
    
    # 4. 测试
    echo ""
    test_cross_compilers
    
    # 5. 生成文档
    generate_usage_info
    
    # 6. 总结
    echo ""
    echo "=================================================="
    echo "🎉 TCC 交叉编译器构建完成！"
    echo ""
    echo "📊 构建统计:"
    echo "  主机编译器: $([ -f "$BUILD_DIR/host/bin/tcc" ] && echo "✅" || echo "❌")"
    echo "  交叉编译器: $(find "$BUILD_DIR/cross" -name "tcc-*" -type f | wc -l) 个"
    echo ""
    echo "📁 构建产物位置: $BUILD_DIR"
    echo "📖 使用说明: $BUILD_DIR/README.md"
    echo "🧪 测试脚本: $SCRIPT_DIR/test-tcc-builds.sh"
}

# 运行主流程
main "$@"