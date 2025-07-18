#!/bin/bash
#
# run_arm64_performance_tests.sh - 运行ARM64性能测试
#

set -e

TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$TEST_DIR/../.." && pwd)"

echo "=== ARM64性能测试套件 ==="

# 检查是否在ARM64系统上
if [ "$(uname -m)" != "arm64" ] && [ "$(uname -m)" != "aarch64" ]; then
    echo "警告: 当前不在ARM64系统上，跳过实际性能测试"
    exit 0
fi

# 编译测试程序
echo "编译ARM64性能测试程序..."

# 加载ARM64优化标志
if [ -f "$PROJECT_ROOT/build/arm64_optimization_flags.mk" ]; then
    source <(grep -E '^[A-Z_]+ =' "$PROJECT_ROOT/build/arm64_optimization_flags.mk" | sed 's/ = /="/; s/$/"/')
fi

# 编译向量化测试
gcc $ARM64_CFLAGS "$TEST_DIR/arm64_vector_test.c" -o "$TEST_DIR/arm64_vector_test"

# 编译对齐测试
gcc $ARM64_CFLAGS "$TEST_DIR/arm64_alignment_test.c" -o "$TEST_DIR/arm64_alignment_test"

# 运行测试
echo
echo "运行ARM64向量化测试..."
"$TEST_DIR/arm64_vector_test"

echo
echo "运行ARM64内存对齐测试..."
"$TEST_DIR/arm64_alignment_test"

echo
echo "ARM64性能测试完成！"
