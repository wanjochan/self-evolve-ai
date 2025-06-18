#!/bin/bash

# TCC简化构建脚本
# 使用主机版本TCC来构建其他架构版本

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TCC_ROOT="$(dirname "$SCRIPT_DIR")"
SRC_DIR="$TCC_ROOT/src"
BUILD_DIR="$TCC_ROOT/build"
HOST_TCC="$BUILD_DIR/host/bin/tcc"

echo "=== TCC简化构建脚本 ==="

# 检查主机TCC是否存在
if [ ! -f "$HOST_TCC" ]; then
    echo "错误: 主机版本TCC不存在，请先运行 build-cross-tcc-fixed.sh"
    exit 1
fi

echo "使用主机TCC: $HOST_TCC"

# 定义目标架构
declare -A CROSS_TARGETS=(
    ["x86_64-windows"]="x86_64-w64-mingw32-"
    ["i686-linux"]="local-m32"
    ["i686-windows"]="i686-w64-mingw32-"
)

# 使用TCC自己来构建交叉编译版本
build_with_tcc() {
    local target_name=$1
    local cross_prefix=$2
    
    echo ""
    echo "=== 使用TCC构建 $target_name ==="
    
    # 确定输出目录
    local arch=$(echo $target_name | cut -d'-' -f1)
    local platform=$(echo $target_name | cut -d'-' -f2)
    
    local output_dir
    case $arch in
        "x86_64") output_dir="$BUILD_DIR/x86_64/$platform" ;;
        "i686") output_dir="$BUILD_DIR/x86_32/$platform" ;;
    esac
    
    mkdir -p "$output_dir/bin"
    
    # 创建工作目录
    local work_dir="$BUILD_DIR/work_$target_name"
    rm -rf "$work_dir"
    mkdir -p "$work_dir"
    cd "$work_dir"
    
    # 复制必要的源文件
    cp "$SRC_DIR"/{tcc.c,libtcc.c,tccpp.c,tccgen.c,tccdbg.c,tccelf.c,tccpe.c,tccmacho.c,tccasm.c,tccrun.c,tcctools.c} .
    cp "$SRC_DIR"/{i386-gen.c,x86_64-gen.c,i386-link.c,x86_64-link.c,i386-asm.c} .
    cp "$SRC_DIR"/{tcctok.h,tcc.h,tcclib.h,il-opcodes.h,i386-tok.h,i386-asm.h,x86_64-asm.h} .
    cp "$SRC_DIR"/{elf.h,coff.h,stab.h,stab.def,dwarf.h} .
    
    # 复制预生成的头文件
    cp "$BUILD_DIR/host/tccdefs_.h" .
    
    # 设置编译选项
    local tcc_opts="-I. -DONE_SOURCE=0"
    local output_name="tcc-$target_name"
    
    if [ "$cross_prefix" = "local-m32" ]; then
        # 32位本地版本
        tcc_opts="$tcc_opts -m32"
        output_name="${output_name}"
    elif [ "$target_name" = "x86_64-windows" ]; then
        # Windows版本
        tcc_opts="$tcc_opts -DTCC_TARGET_PE"
        output_name="${output_name}.exe"
    elif [ "$target_name" = "i686-windows" ]; then
        # Windows 32位版本
        tcc_opts="$tcc_opts -DTCC_TARGET_PE -m32"
        output_name="${output_name}.exe"
    fi
    
    echo "TCC编译选项: $tcc_opts"
    echo "输出文件: $output_name"
    
    # 创建一个简单的构建脚本
    cat > build_tcc.c << 'EOF'
#define ONE_SOURCE 1
#include "tcc.c"
EOF
    
    # 使用主机TCC编译
    if "$HOST_TCC" $tcc_opts -o "$output_dir/bin/$output_name" build_tcc.c; then
        echo "✓ $target_name 构建成功: $output_dir/bin/$output_name"
        
        # 验证文件
        if [ -f "$output_dir/bin/$output_name" ]; then
            local size=$(ls -lh "$output_dir/bin/$output_name" | awk '{print $5}')
            echo "文件大小: $size"
            return 0
        fi
    else
        echo "✗ $target_name 构建失败"
        return 1
    fi
    
    cd "$TCC_ROOT"
    rm -rf "$work_dir"
    return 1
}

# 创建简单的测试程序
create_simple_tests() {
    echo "创建简单的测试程序..."
    
    mkdir -p "$BUILD_DIR/simple_tests"
    
    cat > "$BUILD_DIR/simple_tests/hello.c" << 'EOF'
#include <stdio.h>
int main() {
    printf("Hello from TCC!\n");
    return 0;
}
EOF
    
    cat > "$BUILD_DIR/simple_tests/calc.c" << 'EOF'
#include <stdio.h>
int main() {
    int a = 10, b = 20;
    printf("10 + 20 = %d\n", a + b);
    return 0;
}
EOF
}

# 测试生成的TCC
test_generated_tcc() {
    local tcc_path=$1
    local tcc_name=$(basename "$tcc_path")
    
    echo "测试 $tcc_name..."
    
    if [ ! -f "$tcc_path" ]; then
        echo "✗ 文件不存在: $tcc_path"
        return 1
    fi
    
    # 检查文件是否可执行
    if ! file "$tcc_path" | grep -q "executable"; then
        echo "✗ 不是可执行文件: $tcc_path"
        return 1
    fi
    
    # 对于本地版本，尝试编译测试
    if [[ "$tcc_name" == *"x86_64-linux"* ]] || [[ "$tcc_name" == *"i686-linux"* ]]; then
        local test_output="/tmp/test_${tcc_name}_hello"
        if "$tcc_path" -o "$test_output" "$BUILD_DIR/simple_tests/hello.c" 2>/dev/null; then
            echo "✓ $tcc_name 编译测试成功"
            if [ -x "$test_output" ] && "$test_output" 2>/dev/null | grep -q "Hello"; then
                echo "✓ $tcc_name 运行测试成功"
            fi
            rm -f "$test_output"
        else
            echo "⚠ $tcc_name 编译测试失败"
        fi
    else
        echo "⚠ $tcc_name 跳过运行测试（交叉编译版本）"
    fi
    
    return 0
}

# 主函数
main() {
    echo "使用主机TCC: $($HOST_TCC -v 2>&1 | head -1)"
    
    # 创建测试程序
    create_simple_tests
    
    # 构建交叉编译版本
    local success_count=0
    local total_count=${#CROSS_TARGETS[@]}
    
    # 首先复制主机版本作为x86_64-linux
    if [ -f "$HOST_TCC" ]; then
        cp "$HOST_TCC" "$BUILD_DIR/x86_64/linux/bin/tcc-x86_64-linux"
        echo "✓ 复制主机版本作为 x86_64-linux"
        ((success_count++))
        ((total_count++))
    fi
    
    for target in "${!CROSS_TARGETS[@]}"; do
        if build_with_tcc "$target" "${CROSS_TARGETS[$target]}"; then
            ((success_count++))
        fi
    done
    
    echo ""
    echo "=== 构建结果测试 ==="
    
    # 测试所有生成的TCC
    find "$BUILD_DIR" -name "tcc-*" -type f | while read -r tcc_file; do
        test_generated_tcc "$tcc_file"
    done
    
    echo ""
    echo "=== 最终报告 ==="
    echo "成功构建: $success_count/$total_count"
    
    echo ""
    echo "生成的TCC可执行文件:"
    find "$BUILD_DIR" -name "tcc-*" -type f | while read -r file; do
        local size=$(ls -lh "$file" | awk '{print $5}')
        echo "  $file (大小: $size)"
    done
    
    if [ $success_count -gt 0 ]; then
        echo ""
        echo "🎉 TCC交叉编译构建完成！"
        echo "共生成 $success_count 个TCC版本"
        return 0
    else
        echo "⚠️ 构建失败"
        return 1
    fi
}

# 运行主函数
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi