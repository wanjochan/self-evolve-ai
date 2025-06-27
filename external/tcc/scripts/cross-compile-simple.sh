#!/bin/bash

# TCC 简单交叉编译脚本
# 从 macOS ARM64 环境交叉编译到 Linux 和 Windows

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TCC_ROOT="$(dirname "$SCRIPT_DIR")"
DIST_DIR="$TCC_ROOT/dist"
TCC_BIN="$DIST_DIR/bin/tcc-macos-arm64"
OUTPUT_DIR="$TCC_ROOT/cross_output"

echo "=== TCC 简单交叉编译脚本 ==="
echo "TCC根目录: $TCC_ROOT"
echo "TCC可执行文件: $TCC_BIN"
echo "输出目录: $OUTPUT_DIR"

# 检查 TCC 是否存在
if [ ! -f "$TCC_BIN" ]; then
    echo "错误: TCC 可执行文件不存在，请先构建 macOS ARM64 版本的 TCC"
    exit 1
fi

# 创建输出目录
mkdir -p "$OUTPUT_DIR"

# 编译测试程序
compile_test_program() {
    local target=$1
    local arch=$2
    local format=$3
    local suffix=$4
    local output_file="$OUTPUT_DIR/cross_test-${target}-${arch}${suffix}"
    
    echo "编译 $target $arch 版本..."
    
    # 构建命令
    local cmd="$TCC_BIN"
    
    # 添加架构选项
    if [ "$arch" = "64" ]; then
        cmd="$cmd -m64"
    else
        cmd="$cmd -m32"
    fi
    
    # 添加输出格式选项
    cmd="$cmd -b $format"
    
    # 添加输出文件
    cmd="$cmd -o $output_file"
    
    # 添加源文件
    cmd="$cmd $TCC_ROOT/test_programs/cross_test.c"
    
    # 执行编译
    echo "执行: $cmd"
    $cmd || echo "警告: $target $arch 编译失败"
    
    # 检查文件类型
    if [ -f "$output_file" ]; then
        echo "生成文件: $output_file"
        file "$output_file" || echo "无法检查文件类型"
    else
        echo "错误: 未生成输出文件"
    fi
    
    echo ""
}

# 编译所有目标平台
compile_all() {
    # Linux ELF 64位
    compile_test_program "linux" "64" "elf" ""
    
    # Linux ELF 32位
    compile_test_program "linux" "32" "elf" ""
    
    # Windows PE 64位
    compile_test_program "windows" "64" "pe" ".exe"
    
    # Windows PE 32位
    compile_test_program "windows" "32" "pe" ".exe"
}

# 显示结果
show_results() {
    echo "=== 交叉编译结果 ==="
    ls -la "$OUTPUT_DIR"
}

# 主函数
main() {
    # 编译所有目标平台
    compile_all
    
    # 显示结果
    show_results
    
    echo ""
    echo "🎉 TCC 交叉编译完成！"
}

# 运行主函数
main "$@" 