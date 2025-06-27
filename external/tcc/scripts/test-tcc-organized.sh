#!/bin/bash
# TCC 构建产物测试脚本 - 重新组织版本
# 测试重新组织后的交叉编译器

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TCC_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$TCC_DIR/build"

echo "🧪 TCC 构建产物测试脚本 v2.0"
echo "=================================================="

# 检查构建目录结构
check_directory_structure() {
    echo "📁 检查目录结构..."
    
    local required_dirs=(
        "$BUILD_DIR/host"
        "$BUILD_DIR/host/bin"
        "$BUILD_DIR/cross"
    )
    
    for dir in "${required_dirs[@]}"; do
        if [ -d "$dir" ]; then
            echo "  ✅ $dir"
        else
            echo "  ❌ $dir (缺失)"
            return 1
        fi
    done
}

# 检查主机编译器
check_host_compiler() {
    echo "🏠 检查主机编译器..."
    
    local host_tcc="$BUILD_DIR/host/bin/tcc"
    
    if [ -x "$host_tcc" ]; then
        echo "  ✅ 主机 TCC: $host_tcc"
        
        # 测试版本信息
        if "$host_tcc" -v 2>&1 | grep -q "tcc"; then
            echo "  ✅ 版本信息正常"
        else
            echo "  ⚠️  版本信息异常"
        fi
        
        return 0
    else
        echo "  ❌ 主机编译器不存在或不可执行"
        return 1
    fi
}

# 检查交叉编译器
check_cross_compilers() {
    echo "🎯 检查交叉编译器..."
    
    local found_compilers=0
    local working_compilers=0
    
    # 遍历交叉编译器目录
    for arch_dir in "$BUILD_DIR/cross"/*; do
        if [ -d "$arch_dir" ]; then
            local arch_name=$(basename "$arch_dir")
            local compiler_path="$arch_dir/bin/tcc-$arch_name"
            
            if [ -x "$compiler_path" ]; then
                found_compilers=$((found_compilers + 1))
                echo "  📦 发现: $arch_name"
                echo "    位置: $compiler_path"
                
                # 测试编译器是否工作
                if test_compiler "$compiler_path" "$arch_name"; then
                    working_compilers=$((working_compilers + 1))
                    echo "    ✅ 工作正常"
                else
                    echo "    ❌ 编译测试失败"
                fi
            else
                echo "  ⚠️  $arch_name: 编译器文件缺失"
            fi
        fi
    done
    
    echo "📊 交叉编译器统计:"
    echo "  发现: $found_compilers 个"
    echo "  可用: $working_compilers 个"
    
    return 0
}

# 测试单个编译器
test_compiler() {
    local compiler_path="$1"
    local arch_name="$2"
    
    # 创建临时测试文件
    local test_file="/tmp/test_${arch_name}.c"
    local output_file="/tmp/test_${arch_name}"
    
    cat > "$test_file" << 'EOF'
#include <stdio.h>

int main() {
    printf("Hello from %s!\n", 
#ifdef __x86_64__
        "x86_64"
#elif defined(__i386__)
        "i386"
#elif defined(__aarch64__)
        "aarch64"
#elif defined(__arm__)
        "arm"
#else
        "unknown"
#endif
    );
    return 0;
}
EOF
    
    # 尝试编译
    if "$compiler_path" "$test_file" -o "$output_file" 2>/dev/null; then
        # 清理临时文件
        rm -f "$test_file" "$output_file"
        return 0
    else
        rm -f "$test_file" "$output_file"
        return 1
    fi
}

# 性能基准测试
benchmark_compilers() {
    echo "⚡ 性能基准测试..."
    
    # 创建较大的测试程序
    local benchmark_file="/tmp/benchmark.c"
    cat > "$benchmark_file" << 'EOF'
#include <stdio.h>
#include <stdlib.h>

// 简单的计算密集型函数
long fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n-1) + fibonacci(n-2);
}

int main() {
    printf("计算 fibonacci(30) = %ld\n", fibonacci(30));
    return 0;
}
EOF
    
    echo "  测试文件: fibonacci 计算"
    
    # 测试主机编译器
    local host_tcc="$BUILD_DIR/host/bin/tcc"
    if [ -x "$host_tcc" ]; then
        echo -n "  主机编译器: "
        local start_time=$(date +%s.%N)
        if "$host_tcc" "$benchmark_file" -o "/tmp/benchmark_host" 2>/dev/null; then
            local end_time=$(date +%s.%N)
            local duration=$(echo "$end_time - $start_time" | bc 2>/dev/null || echo "N/A")
            echo "✅ ${duration}s"
        else
            echo "❌ 编译失败"
        fi
    fi
    
    # 测试几个交叉编译器
    local test_targets=("x86_64-linux" "aarch64-linux" "i386-linux")
    
    for target in "${test_targets[@]}"; do
        local compiler_path="$BUILD_DIR/cross/$target/bin/tcc-$target"
        if [ -x "$compiler_path" ]; then
            echo -n "  $target: "
            local start_time=$(date +%s.%N)
            if "$compiler_path" "$benchmark_file" -o "/tmp/benchmark_$target" 2>/dev/null; then
                local end_time=$(date +%s.%N)
                local duration=$(echo "$end_time - $start_time" | bc 2>/dev/null || echo "N/A")
                echo "✅ ${duration}s"
            else
                echo "❌ 编译失败"
            fi
        fi
    done
    
    # 清理
    rm -f "$benchmark_file" /tmp/benchmark_*
}

# 生成测试报告
generate_test_report() {
    echo "📄 生成测试报告..."
    
    local report_file="$BUILD_DIR/test-report.md"
    
    cat > "$report_file" << EOF
# TCC 构建产物测试报告

生成时间: $(date)

## 目录结构检查

$(if check_directory_structure >/dev/null 2>&1; then echo "✅ 目录结构正确"; else echo "❌ 目录结构有问题"; fi)

## 编译器统计

### 主机编译器
$(if [ -x "$BUILD_DIR/host/bin/tcc" ]; then echo "✅ 可用"; else echo "❌ 不可用"; fi)

### 交叉编译器
$(find "$BUILD_DIR/cross" -name "tcc-*" -type f | wc -l) 个交叉编译器

#### 详细列表
$(for compiler in "$BUILD_DIR"/cross/*/bin/tcc-*; do
    if [ -x "$compiler" ]; then
        echo "- $(basename "$compiler"): ✅"
    fi
done)

## 构建产物大小

\`\`\`
$(du -sh "$BUILD_DIR"/* 2>/dev/null | sort -hr || echo "无法获取大小信息")
\`\`\`

## 推荐使用

### 常用命令
\`\`\`bash
# 编译 x86_64 Linux 程序
$BUILD_DIR/cross/x86_64-linux/bin/tcc-x86_64-linux hello.c -o hello

# 编译 ARM64 程序  
$BUILD_DIR/cross/aarch64-linux/bin/tcc-aarch64-linux hello.c -o hello-arm64
\`\`\`

测试完成时间: $(date)
EOF
    
    echo "  报告已保存: $report_file"
}

# 清理旧的构建产物（可选）
cleanup_old_build() {
    echo "🧹 清理旧的混乱构建产物..."
    
    # 保留新的规范化目录结构，清理旧的混乱文件
    if [ -d "$BUILD_DIR" ]; then
        # 删除旧的临时目录
        rm -rf "$BUILD_DIR"/tmp_* 2>/dev/null || true
        rm -rf "$BUILD_DIR"/work_* 2>/dev/null || true
        rm -rf "$BUILD_DIR"/simple_tests 2>/dev/null || true
        
        # 移动旧的 x86_64 目录到新位置（如果存在且不在 cross 下）
        if [ -d "$BUILD_DIR/x86_64" ] && [ ! -d "$BUILD_DIR/cross" ]; then
            echo "  迁移旧的 x86_64 构建产物..."
            mkdir -p "$BUILD_DIR/cross"
            mv "$BUILD_DIR/x86_64" "$BUILD_DIR/cross/" 2>/dev/null || true
        fi
        
        echo "  ✅ 清理完成"
    fi
}

# 主测试流程
main() {
    echo "🏃 开始测试流程..."
    echo ""
    
    # 1. 清理旧产物
    cleanup_old_build
    
    # 2. 检查目录结构
    if ! check_directory_structure; then
        echo "❌ 目录结构检查失败，请先运行构建脚本"
        exit 1
    fi
    
    echo ""
    
    # 3. 检查主机编译器
    check_host_compiler
    
    echo ""
    
    # 4. 检查交叉编译器
    check_cross_compilers
    
    echo ""
    
    # 5. 性能测试
    if command -v bc >/dev/null 2>&1; then
        benchmark_compilers
        echo ""
    else
        echo "⚠️  跳过性能测试 (需要 bc 命令)"
        echo ""
    fi
    
    # 6. 生成报告
    generate_test_report
    
    echo "=================================================="
    echo "🎉 测试完成！"
    echo ""
    echo "📁 构建目录: $BUILD_DIR"
    echo "📄 测试报告: $BUILD_DIR/test-report.md"
    echo ""
    echo "🚀 可以开始使用 TCC 交叉编译器了！"
}

# 运行测试
main "$@"