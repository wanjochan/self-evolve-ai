#!/bin/bash

# TinyCC 版本测试脚本
# 用于验证所有可用版本的功能

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="$SCRIPT_DIR/dist/bin"
TEST_FILE="$SCRIPT_DIR/examples/test_cross_compile.c"

echo "🔧 TinyCC 版本测试脚本"
echo "======================="

# 检查测试文件是否存在
if [ ! -f "$TEST_FILE" ]; then
    echo "❌ 测试文件不存在: $TEST_FILE"
    exit 1
fi

cd "$BIN_DIR"

# 测试函数
test_compiler() {
    local compiler="$1"
    local output_name="$2"
    local extra_args="$3"
    
    echo "🧪 测试 $compiler..."
    
    if [ ! -f "$compiler" ]; then
        echo "   ⚠️  编译器不存在: $compiler"
        return 1
    fi
    
    # 测试版本信息
    if ./"$compiler" -v > /dev/null 2>&1; then
        local version_info=$(./"$compiler" -v 2>&1 | head -1)
        echo "   ✅ 版本: $version_info"
    else
        echo "   ❌ 无法获取版本信息"
        return 1
    fi
    
    # 测试编译
    if ./"$compiler" $extra_args "$TEST_FILE" -o "$output_name" > /dev/null 2>&1; then
        echo "   ✅ 编译成功: $output_name"
        
        # 检查生成的文件
        if [ -f "$output_name" ]; then
            local file_info=$(file "$output_name" | cut -d: -f2-)
            echo "   📄 文件类型:$file_info"
            rm -f "$output_name"  # 清理
        fi
    else
        echo "   ❌ 编译失败"
        return 1
    fi
    
    return 0
}

# 测试所有版本
echo ""
echo "开始测试所有版本..."
echo ""

success_count=0
total_count=0

# Windows 32位版本
total_count=$((total_count + 1))
if test_compiler "tcc-i386-win32" "test_win32.exe" "-B.."; then
    success_count=$((success_count + 1))
fi

echo ""

# macOS ARM64 版本
total_count=$((total_count + 1))
if test_compiler "tcc-macos-arm64" "test_macos_arm64" ""; then
    success_count=$((success_count + 1))
fi

echo ""

# macOS x86_64 版本
total_count=$((total_count + 1))
if test_compiler "x86_64-tcc" "test_macos_x64" ""; then
    success_count=$((success_count + 1))
fi

echo ""

# Linux x86_64 版本
total_count=$((total_count + 1))
if test_compiler "tcc-x86_64-linux" "test_linux_x64" ""; then
    success_count=$((success_count + 1))
fi

echo ""

# 智能选择脚本测试
echo "🔍 测试智能选择脚本..."
total_count=$((total_count + 1))
if test_compiler "tcc-win32" "test_win32_script.exe" "-B.."; then
    success_count=$((success_count + 1))
fi

echo ""

# 总结
echo "📊 测试结果"
echo "==========="
echo "成功: $success_count/$total_count"

if [ $success_count -eq $total_count ]; then
    echo "🎉 所有测试通过！"
    exit 0
else
    echo "⚠️  有 $((total_count - success_count)) 个测试失败"
    exit 1
fi 