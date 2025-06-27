#!/bin/bash

# 从源码构建真正的交叉编译版本的 TCC
# 这个脚本会构建 TCC 的交叉编译器，能够生成不同架构和操作系统的可执行文件

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TCC_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$TCC_ROOT/build"
DIST_DIR="$TCC_ROOT/dist"

echo "=== 构建 TCC 交叉编译器 ==="
echo "TCC根目录: $TCC_ROOT"
echo "构建目录: $BUILD_DIR"
echo "安装目录: $DIST_DIR"

# 创建构建目录
mkdir -p "$BUILD_DIR"

# 检查是否已经构建了本地版本的 TCC
if [ ! -f "$DIST_DIR/bin/tcc-macos-arm64" ]; then
    echo "错误: 本地版本的 TCC 不存在，请先构建 macOS ARM64 版本的 TCC"
    exit 1
fi

# 构建交叉编译器
build_cross_compiler() {
    local target=$1
    local build_subdir="$BUILD_DIR/$target"
    
    echo "构建 $target 交叉编译器..."
    
    # 创建构建子目录
    mkdir -p "$build_subdir"
    
    # 进入 TCC 源码目录
    cd "$TCC_ROOT/src"
    
    # 配置和构建
    case "$target" in
        "x86_64")
            echo "配置 x86_64-linux 交叉编译器..."
            ./configure --prefix="$build_subdir" --cpu=x86_64 --enable-cross
            ;;
        "i386")
            echo "配置 i386-linux 交叉编译器..."
            ./configure --prefix="$build_subdir" --cpu=i386 --enable-cross
            ;;
        "x86_64-win32")
            echo "配置 x86_64-win32 交叉编译器..."
            ./configure --prefix="$build_subdir" --cpu=x86_64 --targetos=WIN32 --enable-cross
            ;;
        "i386-win32")
            echo "配置 i386-win32 交叉编译器..."
            ./configure --prefix="$build_subdir" --cpu=i386 --targetos=WIN32 --enable-cross
            ;;
        *)
            echo "错误: 不支持的目标平台 $target"
            return 1
            ;;
    esac
    
    # 编译
    make cross
    
    # 安装
    make install
    
    # 复制到 dist 目录
    mkdir -p "$DIST_DIR/bin"
    if [ -f "$build_subdir/bin/$target-tcc" ]; then
        cp "$build_subdir/bin/$target-tcc" "$DIST_DIR/bin/"
        echo "已安装 $target-tcc 到 $DIST_DIR/bin/"
    else
        echo "警告: $target-tcc 未构建成功"
    fi
    
    echo "完成 $target 交叉编译器构建"
    echo ""
}

# 测试交叉编译器
test_cross_compiler() {
    local target=$1
    local test_file="$TCC_ROOT/test_programs/hello.c"
    local output_dir="$TCC_ROOT/cross_test"
    
    echo "测试 $target 交叉编译器..."
    
    # 创建测试目录
    mkdir -p "$output_dir"
    
    # 创建简单的测试程序
    if [ ! -f "$test_file" ]; then
        echo '#include <stdio.h>' > "$test_file"
        echo 'int main() {' >> "$test_file"
        echo '    printf("Hello from %s!\n", "Cross-Compiled TCC");' >> "$test_file"
        echo '    return 0;' >> "$test_file"
        echo '}' >> "$test_file"
    fi
    
    # 使用交叉编译器编译测试程序
    if [ -f "$DIST_DIR/bin/$target-tcc" ]; then
        local output_file="$output_dir/hello-$target"
        
        # 为 Windows 目标添加 .exe 后缀
        if [[ "$target" == *-win32 ]]; then
            output_file="$output_file.exe"
        fi
        
        echo "编译 $test_file 到 $output_file..."
        "$DIST_DIR/bin/$target-tcc" -o "$output_file" "$test_file" || echo "警告: $target 编译失败"
        
        # 检查文件类型
        if [ -f "$output_file" ]; then
            echo "生成文件: $output_file"
            file "$output_file" || echo "无法检查文件类型"
        else
            echo "错误: 未生成输出文件"
        fi
    else
        echo "错误: $target-tcc 不存在，无法测试"
    fi
    
    echo "完成 $target 交叉编译器测试"
    echo ""
}

# 构建所有交叉编译器
build_all() {
    # Linux x86_64
    build_cross_compiler "x86_64"
    
    # Linux i386
    build_cross_compiler "i386"
    
    # Windows x86_64
    build_cross_compiler "x86_64-win32"
    
    # Windows i386
    build_cross_compiler "i386-win32"
}

# 测试所有交叉编译器
test_all() {
    # Linux x86_64
    test_cross_compiler "x86_64"
    
    # Linux i386
    test_cross_compiler "i386"
    
    # Windows x86_64
    test_cross_compiler "x86_64-win32"
    
    # Windows i386
    test_cross_compiler "i386-win32"
}

# 显示结果
show_results() {
    echo "=== 交叉编译器构建结果 ==="
    ls -la "$DIST_DIR/bin" | grep -E "tcc|i386|x86_64"
    
    echo ""
    echo "=== 交叉编译测试结果 ==="
    ls -la "$TCC_ROOT/cross_test" || echo "没有测试结果"
}

# 主函数
main() {
    # 构建所有交叉编译器
    build_all
    
    # 测试所有交叉编译器
    test_all
    
    # 显示结果
    show_results
    
    echo ""
    echo "🎉 TCC 交叉编译器构建完成！"
}

# 运行主函数
main "$@"