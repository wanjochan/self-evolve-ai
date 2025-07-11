#!/bin/bash
#
# build_core.sh - 构建新的core模块系统
# 

# 确保脚本在错误时退出
set -e

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 定义路径
SRC_DIR="$SCRIPT_DIR/src/core"
MODULES_DIR="$SRC_DIR/modules"
BIN_DIR="$SCRIPT_DIR/bin"

echo "=== 构建新的Core模块系统 ==="

# 创建输出目录
mkdir -p "$BIN_DIR"

# 检测架构
if [[ "$(uname -m)" == "arm64" ]]; then
    ARCH="arm64"
    BITS="64"
elif [[ "$(uname -m)" == "x86_64" ]]; then
    ARCH="x64"
    BITS="64"
elif [[ "$(uname -m)" == "i386" ]] || [[ "$(uname -m)" == "i686" ]]; then
    ARCH="x86"
    BITS="32"
else
    echo "警告: 未知架构 $(uname -m)，使用默认 x64_64"
    ARCH="x64"
    BITS="64"
fi

echo "检测到架构: ${ARCH}_${BITS}"

# 编译核心模块
echo ""
echo "1. 编译核心模块..."

# 编译layer0模块
echo "编译 layer0_module.c..."
"$SCRIPT_DIR/cc.sh" -c "$MODULES_DIR/layer0_module.c" -I "$SRC_DIR" -o "$MODULES_DIR/layer0_module.o"

# 编译pipeline模块
echo "编译 pipeline_module.c..."
"$SCRIPT_DIR/cc.sh" -c "$MODULES_DIR/pipeline_module.c" -I "$SRC_DIR" -o "$MODULES_DIR/pipeline_module.o"

# 编译compiler模块
echo "编译 compiler_module.c..."
"$SCRIPT_DIR/cc.sh" -c "$MODULES_DIR/compiler_module.c" -I "$SRC_DIR" -o "$MODULES_DIR/compiler_module.o"

# 编译module_module
echo "编译 module_module.c..."
"$SCRIPT_DIR/cc.sh" -c "$MODULES_DIR/module_module.c" -I "$SRC_DIR" -o "$MODULES_DIR/module_module.o"

# 编译libc模块
echo "编译 libc_module.c..."
"$SCRIPT_DIR/cc.sh" -c "$MODULES_DIR/libc_module.c" -I "$SRC_DIR" -o "$MODULES_DIR/libc_module.o"

echo "所有模块编译完成"

# 生成.native文件
echo ""
echo "2. 生成.native模块文件..."

# 确保c2native工具存在 (直接构建，不依赖外部脚本)
if [ ! -x "$BIN_DIR/c2native" ]; then
    echo "构建 c2native 工具..."
    "$SCRIPT_DIR/cc.sh" -o "$BIN_DIR/c2native" "tools/c2native.c" -std=c99 -O2 -Wall
    if [ $? -ne 0 ]; then
        echo "错误: 构建 c2native 工具失败"
        exit 1
    fi
fi

# 使用c2native工具生成.native文件
create_native_module() {
    local module_name="$1"
    local source_file="$2"
    local output_file="$3"

    echo "生成 $output_file..."

    # 使用c2native工具生成正确的.native文件
    "$BIN_DIR/c2native" "$source_file" "$output_file"

    if [ $? -ne 0 ]; then
        echo "错误: 生成 $output_file 失败"
        exit 1
    fi
}

# 生成各个模块的.native文件
create_native_module "layer0" "$MODULES_DIR/layer0_module.c" "$BIN_DIR/layer0_${ARCH}_${BITS}.native"
create_native_module "pipeline" "$MODULES_DIR/pipeline_module.c" "$BIN_DIR/pipeline_${ARCH}_${BITS}.native"
create_native_module "compiler" "$MODULES_DIR/compiler_module.c" "$BIN_DIR/compiler_${ARCH}_${BITS}.native"
create_native_module "libc" "$MODULES_DIR/libc_module.c" "$BIN_DIR/libc_${ARCH}_${BITS}.native"

# 构建简单的模块加载器
echo ""
echo "3. 构建模块加载器..."

cat > "$BIN_DIR/test_module_loader.c" << 'EOF'
#include "../src/core/module.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("=== 测试新模块系统 ===\n");
    
    // 测试智能路径解析
    char* resolved = resolve_native_file("./layer0");
    if (resolved) {
        printf("路径解析测试: './layer0' -> '%s'\n", resolved);
        free(resolved);
    }
    
    printf("模块系统构建完成！\n");
    return 0;
}
EOF

"$SCRIPT_DIR/cc.sh" "$BIN_DIR/test_module_loader.c" "$MODULES_DIR/module_module.o" -I "$SRC_DIR" -o "$BIN_DIR/test_module_loader"

echo "模块加载器已构建: $BIN_DIR/test_module_loader"

# 运行测试
echo ""
echo "4. 运行模块系统测试..."
"$BIN_DIR/test_module_loader"

# 清理临时文件
rm -f "$BIN_DIR/test_module_loader.c"

echo ""
echo "=== Core模块系统构建完成 ==="
echo "生成的.native模块文件:"
ls -la "$BIN_DIR"/*.native 2>/dev/null || echo "  (未找到.native文件)"
echo ""
echo "可用的模块:"
echo "  - layer0_${ARCH}_${BITS}.native: 基础功能模块"
echo "  - pipeline_${ARCH}_${BITS}.native: 编译流水线模块 + VM运行时"
echo "  - compiler_${ARCH}_${BITS}.native: 编译器集成模块"
echo "  - libc_${ARCH}_${BITS}.native: C99标准库模块"
echo ""
echo "使用方法:"
echo "  module = load_module('./layer0')  // 自动解析为 ./layer0_${ARCH}_${BITS}.native"
echo "  rt = module->sym('function_name')(args)" 