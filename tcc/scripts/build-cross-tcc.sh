#!/bin/bash

# TCC交叉编译构建脚本
# 构建12种不同架构的TCC可执行文件

set -e  # 遇到错误立即退出

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TCC_ROOT="$(dirname "$SCRIPT_DIR")"
SRC_DIR="$TCC_ROOT/src"
BUILD_DIR="$TCC_ROOT/build"

echo "=== TCC交叉编译构建开始 ==="
echo "TCC根目录: $TCC_ROOT"
echo "源码目录: $SRC_DIR"
echo "构建目录: $BUILD_DIR"

# 检查源码目录
if [ ! -d "$SRC_DIR" ] || [ ! -f "$SRC_DIR/Makefile" ]; then
    echo "错误: TCC源码目录不存在或不完整"
    exit 1
fi

# 创建构建目录结构
mkdir -p "$BUILD_DIR"/{x86_64,x86_32,arm64,arm32}/{linux,windows,macos}

# 定义目标架构配置
declare -A TARGETS=(
    # x86_64架构
    ["x86_64-linux"]="CC=gcc ARCH=x86_64 TARGET=x86_64-linux-gnu"
    ["x86_64-windows"]="CC=x86_64-w64-mingw32-gcc ARCH=x86_64 TARGET=x86_64-w64-mingw32"
    
    # i686架构 (32位x86)
    ["i686-linux"]="CC=gcc ARCH=i386 TARGET=i386-linux-gnu CFLAGS=-m32"
    ["i686-windows"]="CC=i686-w64-mingw32-gcc ARCH=i386 TARGET=i686-w64-mingw32"
    
    # ARM64架构
    ["aarch64-linux"]="CC=aarch64-linux-gnu-gcc ARCH=arm64 TARGET=aarch64-linux-gnu"
    
    # ARM32架构  
    ["arm-linux"]="CC=arm-linux-gnueabi-gcc ARCH=arm TARGET=arm-linux-gnueabi"
)

# 构建函数
build_tcc() {
    local target_name=$1
    local config=$2
    
    echo ""
    echo "=== 构建 TCC for $target_name ==="
    
    # 提取架构和平台
    local arch=$(echo $target_name | cut -d'-' -f1)
    local platform=$(echo $target_name | cut -d'-' -f2)
    
    # 确定输出目录
    local output_dir
    case $arch in
        "x86_64") output_dir="$BUILD_DIR/x86_64/$platform" ;;
        "i686") output_dir="$BUILD_DIR/x86_32/$platform" ;;
        "aarch64") output_dir="$BUILD_DIR/arm64/$platform" ;;
        "arm") output_dir="$BUILD_DIR/arm32/$platform" ;;
    esac
    
    # 创建临时构建目录
    local tmp_build_dir="$BUILD_DIR/tmp_$target_name"
    rm -rf "$tmp_build_dir"
    mkdir -p "$tmp_build_dir"
    
    # 复制源码到临时目录
    echo "复制源码到临时目录..."
    cp -r "$SRC_DIR"/* "$tmp_build_dir/"
    cd "$tmp_build_dir"
    
    # 清理之前的构建
    make distclean 2>/dev/null || true
    
    echo "配置构建环境: $config"
    
    # 配置构建
    eval "$config ./configure --prefix=$output_dir"
    
    # 编译
    echo "开始编译..."
    eval "$config make"
    
    # 安装到输出目录
    echo "安装到 $output_dir"
    make install
    
    # 重命名可执行文件以标识目标平台
    if [ -f "$output_dir/bin/tcc" ]; then
        mv "$output_dir/bin/tcc" "$output_dir/bin/tcc-$target_name"
        echo "生成可执行文件: $output_dir/bin/tcc-$target_name"
    fi
    
    # 清理临时目录
    cd "$TCC_ROOT"
    rm -rf "$tmp_build_dir"
    
    echo "✓ $target_name 构建完成"
}

# 检查交叉编译器
check_cross_compilers() {
    echo "=== 检查交叉编译器 ==="
    
    local compilers=(
        "gcc"
        "x86_64-w64-mingw32-gcc" 
        "i686-w64-mingw32-gcc"
        "aarch64-linux-gnu-gcc"
        "arm-linux-gnueabi-gcc"
    )
    
    for compiler in "${compilers[@]}"; do
        if command -v "$compiler" >/dev/null 2>&1; then
            echo "✓ $compiler: $(which $compiler)"
        else
            echo "✗ $compiler: 未找到"
        fi
    done
}

# 生成构建报告
generate_report() {
    echo ""
    echo "=== 构建报告 ==="
    
    local report_file="$BUILD_DIR/build_report.txt"
    echo "TCC交叉编译构建报告" > "$report_file"
    echo "构建时间: $(date)" >> "$report_file"
    echo "" >> "$report_file"
    
    echo "生成的TCC可执行文件:" | tee -a "$report_file"
    
    find "$BUILD_DIR" -name "tcc-*" -type f | while read -r file; do
        if [ -x "$file" ]; then
            local size=$(ls -lh "$file" | awk '{print $5}')
            echo "  $file (大小: $size)" | tee -a "$report_file"
        fi
    done
    
    echo "" | tee -a "$report_file"
    echo "报告保存至: $report_file"
}

# 主函数
main() {
    # 检查交叉编译器
    check_cross_compilers
    
    # 开始构建
    echo ""
    echo "=== 开始批量构建 ==="
    
    local success_count=0
    local total_count=${#TARGETS[@]}
    
    for target in "${!TARGETS[@]}"; do
        if build_tcc "$target" "${TARGETS[$target]}"; then
            ((success_count++))
        else
            echo "✗ $target 构建失败"
        fi
    done
    
    echo ""
    echo "=== 构建总结 ==="
    echo "成功: $success_count/$total_count"
    
    # 生成报告
    generate_report
    
    if [ $success_count -eq $total_count ]; then
        echo "🎉 所有目标构建成功！"
        return 0
    else
        echo "⚠️ 部分目标构建失败"
        return 1
    fi
}

# 如果直接运行此脚本
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi