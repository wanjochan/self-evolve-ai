#!/bin/bash
#
# build_tools.sh - 构建多架构工具
#

set -e

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="$SCRIPT_DIR/bin"
TOOLS_DIR="$SCRIPT_DIR/tools"

echo "=== 构建多架构工具 ==="

# 创建输出目录
mkdir -p "$BIN_DIR"

# 检测当前架构
if [[ "$(uname -m)" == "arm64" ]]; then
    CURRENT_ARCH="arm64"
    CURRENT_BITS="64"
elif [[ "$(uname -m)" == "x86_64" ]]; then
    CURRENT_ARCH="x64"
    CURRENT_BITS="64"
elif [[ "$(uname -m)" == "i386" ]] || [[ "$(uname -m)" == "i686" ]]; then
    CURRENT_ARCH="x86"
    CURRENT_BITS="32"
else
    echo "警告: 未知架构 $(uname -m)，使用默认 x64_64"
    CURRENT_ARCH="x64"
    CURRENT_BITS="64"
fi

echo "当前架构: ${CURRENT_ARCH}_${CURRENT_BITS}"

# 构建当前架构的工具
echo ""
echo "1. 构建当前架构的工具..."

# 首先编译必要的模块文件
echo "编译模块依赖..."
"$SCRIPT_DIR/cc.sh" -c "$SCRIPT_DIR/src/core/astc.c" -I "$SCRIPT_DIR/src/core" -o "$BIN_DIR/astc.o"
"$SCRIPT_DIR/cc.sh" -c "$SCRIPT_DIR/src/core/modules/module_module.c" -I "$SCRIPT_DIR/src/core" -o "$BIN_DIR/module_module.o"

# 构建 c2astc 工具
echo "构建 c2astc_${CURRENT_ARCH}_${CURRENT_BITS}..."
"$SCRIPT_DIR/cc.sh" -o "$BIN_DIR/c2astc_${CURRENT_ARCH}_${CURRENT_BITS}" "$TOOLS_DIR/c2astc.c" "$BIN_DIR/astc.o" "$BIN_DIR/module_module.o" -I "$SCRIPT_DIR/src/core" -std=c99 -O2 -Wall -ldl

# 构建 c2native 工具
echo "构建 c2native_${CURRENT_ARCH}_${CURRENT_BITS}..."
"$SCRIPT_DIR/cc.sh" -o "$BIN_DIR/c2native_${CURRENT_ARCH}_${CURRENT_BITS}" "$TOOLS_DIR/c2native.c" "$BIN_DIR/astc.o" "$BIN_DIR/module_module.o" -I "$SCRIPT_DIR/src/core" -std=c99 -O2 -Wall -ldl

# 构建 simple_loader 工具
echo "构建 simple_loader_${CURRENT_ARCH}_${CURRENT_BITS}..."
"$SCRIPT_DIR/cc.sh" -o "$BIN_DIR/simple_loader_${CURRENT_ARCH}_${CURRENT_BITS}" "$SCRIPT_DIR/src/layer1/simple_loader.c" -I "$SCRIPT_DIR/src/core" -std=c99 -O2 -Wall -ldl

# 创建符号链接到当前架构
echo ""
echo "2. 创建符号链接..."
ln -sf "c2astc_${CURRENT_ARCH}_${CURRENT_BITS}" "$BIN_DIR/c2astc"
ln -sf "c2native_${CURRENT_ARCH}_${CURRENT_BITS}" "$BIN_DIR/c2native"
ln -sf "simple_loader_${CURRENT_ARCH}_${CURRENT_BITS}" "$BIN_DIR/simple_loader"

echo ""
echo "=== 工具构建完成 ==="
echo "生成的工具文件:"
ls -la "$BIN_DIR"/c2astc_* "$BIN_DIR"/c2native_* "$BIN_DIR"/simple_loader_* 2>/dev/null || echo "  (未找到工具文件)"
echo ""
echo "当前架构工具:"
echo "  - c2astc -> c2astc_${CURRENT_ARCH}_${CURRENT_BITS}"
echo "  - c2native -> c2native_${CURRENT_ARCH}_${CURRENT_BITS}"
echo "  - simple_loader -> simple_loader_${CURRENT_ARCH}_${CURRENT_BITS}"
echo ""
echo "使用方法:"
echo "  bin/c2astc source.c output.astc"
echo "  bin/c2native source.c output_{arch}_{bits}.native"
echo "  bin/simple_loader program.astc"
echo ""
echo "架构支持:"
echo "  - c2native 会根据输出文件名自动检测目标架构"
echo "  - 支持的架构: x86_64, arm64, x86_32"
echo "  - 示例: bin/c2native src.c pipeline_arm64_64.native" 