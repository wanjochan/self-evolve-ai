#!/bin/bash

# macOS交叉编译环境设置脚本
# 设置osxcross工具链来支持macOS目标

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TCC_ROOT="$(dirname "$SCRIPT_DIR")"
TOOLS_DIR="$TCC_ROOT/tools"

echo "=== 设置macOS交叉编译环境 ==="

# 创建工具目录
mkdir -p "$TOOLS_DIR"

# 检查是否已安装osxcross
if [ -d "$TOOLS_DIR/osxcross" ]; then
    echo "osxcross已存在，跳过安装"
    exit 0
fi

echo "注意: macOS交叉编译需要macOS SDK"
echo "由于许可证限制，本脚本仅设置基础框架"
echo ""

# 安装依赖
echo "安装osxcross依赖..."
sudo apt update
sudo apt install -y clang libbz2-dev libxml2-dev

# 下载osxcross
echo "下载osxcross..."
cd "$TOOLS_DIR"
git clone https://github.com/tpoechtrager/osxcross.git

cd osxcross

echo ""
echo "=== macOS交叉编译环境设置完成 ==="
echo ""
echo "⚠️  重要提示:"
echo "1. 需要合法获取macOS SDK (如MacOSX11.3.sdk.tar.xz)"
echo "2. 将SDK文件放置到: $TOOLS_DIR/osxcross/tarballs/"
echo "3. 运行: cd $TOOLS_DIR/osxcross && ./build.sh"
echo "4. 设置环境变量: export PATH=$TOOLS_DIR/osxcross/target/bin:\$PATH"
echo ""
echo "由于SDK许可证限制，建议在有合法macOS系统的机器上构建macOS版本"