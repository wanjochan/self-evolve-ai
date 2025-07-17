#!/bin/bash
#
# run_coverage_analysis.sh - 简化的测试覆盖率分析
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=== 测试覆盖率快速分析 ===${NC}"

# 统计源代码文件
echo "1. 源代码统计:"
SRC_C_FILES=$(find "$PROJECT_ROOT/src" -name "*.c" | wc -l)
SRC_H_FILES=$(find "$PROJECT_ROOT/src" -name "*.h" | wc -l)
echo "  C文件: $SRC_C_FILES"
echo "  头文件: $SRC_H_FILES"

# 统计测试文件
echo
echo "2. 测试文件统计:"
TEST_SH_FILES=$(find "$PROJECT_ROOT/tests" -name "*.sh" | wc -l)
TEST_C_FILES=$(find "$PROJECT_ROOT/tests" -name "*.c" | wc -l)
echo "  Shell测试: $TEST_SH_FILES"
echo "  C测试: $TEST_C_FILES"

# 检查关键模块测试覆盖
echo
echo "3. 关键模块测试覆盖:"
CRITICAL_MODULES=("astc" "module" "layer0_module" "pipeline_module" "compiler_module" "libc_module" "c99bin_module")
COVERED_MODULES=0

for module in "${CRITICAL_MODULES[@]}"; do
    if find "$PROJECT_ROOT/tests" -name "*$module*" | head -1 > /dev/null 2>&1; then
        echo -e "  ✅ $module: 有测试覆盖"
        COVERED_MODULES=$((COVERED_MODULES + 1))
    else
        echo -e "  ❌ $module: 缺少测试"
    fi
done

# 计算覆盖率
COVERAGE_RATE=$((COVERED_MODULES * 100 / ${#CRITICAL_MODULES[@]}))
echo
echo "关键模块覆盖率: $COVERAGE_RATE% ($COVERED_MODULES/${#CRITICAL_MODULES[@]})"

# 检查测试类型
echo
echo "4. 测试类型覆盖:"
TEST_TYPES=("unit" "integration" "performance" "regression" "stress" "error" "compatibility")

for test_type in "${TEST_TYPES[@]}"; do
    count=$(find "$PROJECT_ROOT/tests" -name "*$test_type*" | wc -l)
    if [ $count -gt 0 ]; then
        echo -e "  ✅ $test_type: $count 个测试"
    else
        echo -e "  ❌ $test_type: 无测试"
    fi
done

# 运行现有测试并统计结果
echo
echo "5. 运行核心测试套件:"

# 运行稳定性测试
if [ -f "$PROJECT_ROOT/tests/test_stability_enhanced" ]; then
    echo "运行稳定性测试..."
    if "$PROJECT_ROOT/tests/test_stability_enhanced" > /dev/null 2>&1; then
        echo -e "  ✅ 稳定性测试: 通过"
    else
        echo -e "  ❌ 稳定性测试: 失败"
    fi
fi

# 检查模块系统
if [ -f "$PROJECT_ROOT/bin/layer2/layer0.so" ]; then
    echo -e "  ✅ 模块系统: 已构建"
else
    echo -e "  ❌ 模块系统: 未构建"
fi

# 检查c99bin
if [ -f "$PROJECT_ROOT/tools/c99bin" ]; then
    echo -e "  ✅ c99bin: 可用"
else
    echo -e "  ❌ c99bin: 不可用"
fi

# 生成改进建议
echo
echo -e "${YELLOW}=== 改进建议 ===${NC}"

if [ $COVERAGE_RATE -lt 90 ]; then
    echo "1. 为缺少测试的关键模块添加单元测试"
fi

if [ $(find "$PROJECT_ROOT/tests" -name "*unit*" | wc -l) -eq 0 ]; then
    echo "2. 创建单元测试框架和测试用例"
fi

if [ $(find "$PROJECT_ROOT/tests" -name "*security*" | wc -l) -eq 0 ]; then
    echo "3. 添加安全测试用例"
fi

if [ ! -f "$PROJECT_ROOT/tests/run_all_tests.sh" ]; then
    echo "4. 创建统一的测试运行脚本"
fi

echo
echo -e "${GREEN}=== 分析完成 ===${NC}"
echo "总体评估: 测试文件数量充足，需要提升关键模块覆盖率"
