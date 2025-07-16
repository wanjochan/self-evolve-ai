#!/bin/bash

# TCC 最终版本交叉编译脚本
# 从 macOS ARM64 环境交叉编译到 Linux 和 Windows

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TCC_ROOT="$(dirname "$SCRIPT_DIR")"
DIST_DIR="$TCC_ROOT/dist"
TCC_BIN="$DIST_DIR/bin/tcc-macos-arm64"
OUTPUT_DIR="$TCC_ROOT/cross_output"
LIBTCC1_DIR="$DIST_DIR/lib/macos-arm64/tcc"

echo "=== TCC 最终版本交叉编译脚本 ==="
echo "TCC根目录: $TCC_ROOT"
echo "TCC可执行文件: $TCC_BIN"
echo "libtcc1.a目录: $LIBTCC1_DIR"
echo "输出目录: $OUTPUT_DIR"

# 检查 TCC 是否存在
if [ ! -f "$TCC_BIN" ]; then
    echo "错误: TCC 可执行文件不存在，请先构建 macOS ARM64 版本的 TCC"
    exit 1
fi

# 检查 libtcc1.a 是否存在
if [ ! -f "$LIBTCC1_DIR/libtcc1.a" ]; then
    echo "错误: libtcc1.a 不存在，请先构建 macOS ARM64 版本的 TCC"
    exit 1
fi

# 创建输出目录
mkdir -p "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR/lib/linux-x86_64/tcc"
mkdir -p "$OUTPUT_DIR/lib/linux-i386/tcc"
mkdir -p "$OUTPUT_DIR/lib/windows-x86_64/tcc"
mkdir -p "$OUTPUT_DIR/lib/windows-i386/tcc"

# 复制 libtcc1.a 到各个目标平台目录
cp "$LIBTCC1_DIR/libtcc1.a" "$OUTPUT_DIR/lib/linux-x86_64/tcc/"
cp "$LIBTCC1_DIR/libtcc1.a" "$OUTPUT_DIR/lib/linux-i386/tcc/"
cp "$LIBTCC1_DIR/libtcc1.a" "$OUTPUT_DIR/lib/windows-x86_64/tcc/"
cp "$LIBTCC1_DIR/libtcc1.a" "$OUTPUT_DIR/lib/windows-i386/tcc/"

# 编译测试程序
compile_test_program() {
    local target_os=$1
    local target_cpu=$2
    local output_file="$OUTPUT_DIR/cross_test-${target_os}-${target_cpu}"
    
    echo "编译 $target_os $target_cpu 版本..."
    
    # 为 Windows 目标添加 .exe 后缀
    if [ "$target_os" = "windows" ]; then
        output_file="${output_file}.exe"
    fi
    
    # 构建命令
    local cmd="$TCC_BIN"
    
    # 添加库路径
    cmd="$cmd -B$OUTPUT_DIR/lib/${target_os}-${target_cpu}/tcc"
    
    # 添加特定的编译选项
    case "$target_os-$target_cpu" in
        "linux-x86_64")
            cmd="$cmd -DTCC_TARGET_X86_64 -DTCC_TARGET_LINUX"
            ;;
        "linux-i386")
            cmd="$cmd -DTCC_TARGET_I386 -DTCC_TARGET_LINUX"
            ;;
        "windows-x86_64")
            cmd="$cmd -DTCC_TARGET_X86_64 -DTCC_TARGET_PE"
            ;;
        "windows-i386")
            cmd="$cmd -DTCC_TARGET_I386 -DTCC_TARGET_PE"
            ;;
        *)
            echo "错误: 不支持的目标平台 $target_os-$target_cpu"
            return 1
            ;;
    esac
    
    # 添加输出文件
    cmd="$cmd -o $output_file"
    
    # 添加源文件
    cmd="$cmd $TCC_ROOT/test_programs/cross_test.c"
    
    # 执行编译
    echo "执行: $cmd"
    $cmd || echo "警告: $target_os $target_cpu 编译失败"
    
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
    # Linux x86_64
    compile_test_program "linux" "x86_64"
    
    # Linux i386
    compile_test_program "linux" "i386"
    
    # Windows x86_64
    compile_test_program "windows" "x86_64"
    
    # Windows i386
    compile_test_program "windows" "i386"
}

# 显示结果
show_results() {
    echo "=== 交叉编译结果 ==="
    ls -la "$OUTPUT_DIR"
    
    echo ""
    echo "=== 检查生成的可执行文件 ==="
    file "$OUTPUT_DIR"/cross_test-* 2>/dev/null || echo "没有生成可执行文件"
}

# 显示 TCC 版本和支持的目标平台
show_tcc_info() {
    echo "=== TCC 版本信息 ==="
    "$TCC_BIN" -v
    
    echo ""
    echo "=== TCC 库路径 ==="
    "$TCC_BIN" -vv | grep -A 10 "libraries:"
}

# 主函数
main() {
    # 显示 TCC 信息
    show_tcc_info
    
    # 编译所有目标平台
    compile_all
    
    # 显示结果
    show_results
    
    echo ""
    echo "🎉 TCC 交叉编译完成！"
}

# 运行主函数
main "$@" 