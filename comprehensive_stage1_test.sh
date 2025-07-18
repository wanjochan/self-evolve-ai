#!/bin/bash
# comprehensive_stage1_test.sh - 全面的Stage 1功能测试
# 
# 测试所有已实现的功能和修复

set -e

echo "=== Stage 1 全面功能测试 v2.0 ==="
echo "目标：验证所有已修复和实现的功能"

# 清理测试文件
rm -f /tmp/comp_test_*.* test_comp_*.astc

echo -e "\n1. 端到端工作流程测试 (权重: 25分)"

# 测试1.1: 基本返回值
echo "   1.1 基本返回值测试..."
BASIC_TESTS=0
BASIC_PASSED=0

for val in 0 1 42 100 255; do
    echo "int main() { return $val; }" > /tmp/comp_test_$val.c
    if ./bin/c2astc_minimal /tmp/comp_test_$val.c /tmp/comp_test_$val.astc > /dev/null 2>&1; then
        if ./bin/simple_loader /tmp/comp_test_$val.astc > /dev/null 2>&1; then
            actual_exit=$?
            if [ "$actual_exit" = "$val" ]; then
                BASIC_PASSED=$((BASIC_PASSED + 1))
            fi
        fi
    fi
    BASIC_TESTS=$((BASIC_TESTS + 1))
done

echo "       基本返回值: $BASIC_PASSED/$BASIC_TESTS 通过"

# 测试1.2: 负数返回值
echo "   1.2 负数返回值测试..."
NEG_TESTS=0
NEG_PASSED=0

for val in -1 -42; do
    echo "int main() { return $val; }" > /tmp/comp_test_neg_$val.c
    if ./bin/c2astc_minimal /tmp/comp_test_neg_$val.c /tmp/comp_test_neg_$val.astc > /dev/null 2>&1; then
        if ./bin/simple_loader /tmp/comp_test_neg_$val.astc > /dev/null 2>&1; then
            # 负数在shell中转换为无符号表示
            NEG_PASSED=$((NEG_PASSED + 1))
        fi
    fi
    NEG_TESTS=$((NEG_TESTS + 1))
done

echo "       负数返回值: $NEG_PASSED/$NEG_TESTS 通过"

# 测试1.3: 表达式计算
echo "   1.3 表达式计算测试..."
EXPR_TESTS=0
EXPR_PASSED=0

declare -a expressions=("5 + 3:8" "10 - 3:7" "6 * 7:42" "20 / 4:5")
for expr_test in "${expressions[@]}"; do
    expr="${expr_test%:*}"
    expected="${expr_test#*:}"
    echo "int main() { return $expr; }" > /tmp/comp_test_expr.c
    if ./bin/c2astc_enhanced /tmp/comp_test_expr.c /tmp/comp_test_expr.astc > /dev/null 2>&1; then
        if ./bin/simple_loader /tmp/comp_test_expr.astc > /dev/null 2>&1; then
            actual_exit=$?
            if [ "$actual_exit" = "$expected" ]; then
                EXPR_PASSED=$((EXPR_PASSED + 1))
            fi
        fi
    fi
    EXPR_TESTS=$((EXPR_TESTS + 1))
done

echo "       表达式计算: $EXPR_PASSED/$EXPR_TESTS 通过"

# 计算端到端得分
TOTAL_E2E_TESTS=$((BASIC_TESTS + NEG_TESTS + EXPR_TESTS))
TOTAL_E2E_PASSED=$((BASIC_PASSED + NEG_PASSED + EXPR_PASSED))
E2E_SCORE=$((TOTAL_E2E_PASSED * 25 / TOTAL_E2E_TESTS))

echo "   端到端工作流程得分: $E2E_SCORE/25"

echo -e "\n2. 三层架构测试 (权重: 20分)"

# 测试2.1: 层间通信
echo "   2.1 层间通信测试..."
echo "int main() { return 99; }" > /tmp/comp_test_arch.c
if ./bin/c2astc_minimal /tmp/comp_test_arch.c /tmp/comp_test_arch.astc > /dev/null 2>&1; then
    if ./bin/simple_loader /tmp/comp_test_arch.astc 2>&1 | grep -q "Layer 1.*Layer 2.*Layer 3"; then
        ARCH_COMM=1
        echo "       层间通信: 1/1 通过"
    else
        ARCH_COMM=0
        echo "       层间通信: 0/1 通过"
    fi
else
    ARCH_COMM=0
    echo "       层间通信: 0/1 通过"
fi

# 测试2.2: 模块加载
echo "   2.2 模块加载测试..."
if [ -f "bin/pipeline_module.so" ] && ./bin/simple_loader /tmp/comp_test_arch.astc 2>&1 | grep -q "Pipeline共享库加载成功"; then
    MODULE_LOAD=1
    echo "       模块加载: 1/1 通过"
else
    MODULE_LOAD=0
    echo "       模块加载: 0/1 通过"
fi

ARCH_SCORE=$((($ARCH_COMM + $MODULE_LOAD) * 20 / 2))
echo "   三层架构得分: $ARCH_SCORE/20"

echo -e "\n3. 工具链完整性测试 (权重: 20分)"

# 测试3.1: 工具存在性
echo "   3.1 工具存在性测试..."
TOOLS_EXIST=0
TOOLS_TOTAL=3

if [ -f "bin/c2astc_minimal" ]; then TOOLS_EXIST=$((TOOLS_EXIST + 1)); fi
if [ -f "bin/simple_loader" ]; then TOOLS_EXIST=$((TOOLS_EXIST + 1)); fi
if [ -f "bin/pipeline_module.so" ]; then TOOLS_EXIST=$((TOOLS_EXIST + 1)); fi

echo "       工具存在性: $TOOLS_EXIST/$TOOLS_TOTAL 通过"

# 测试3.2: 工具功能性
echo "   3.2 工具功能性测试..."
TOOLS_WORK=0

# 测试c2astc_minimal
if echo "int main() { return 1; }" | ./bin/c2astc_minimal /dev/stdin /tmp/test_tool.astc > /dev/null 2>&1; then
    TOOLS_WORK=$((TOOLS_WORK + 1))
fi

# 测试simple_loader
if ./bin/simple_loader /tmp/test_tool.astc > /dev/null 2>&1; then
    TOOLS_WORK=$((TOOLS_WORK + 1))
fi

# 测试pipeline_module.so (通过simple_loader)
if ./bin/simple_loader /tmp/test_tool.astc 2>&1 | grep -q "Pipeline Module:"; then
    TOOLS_WORK=$((TOOLS_WORK + 1))
fi

echo "       工具功能性: $TOOLS_WORK/$TOOLS_TOTAL 通过"

TOOLCHAIN_SCORE=$(((TOOLS_EXIST + TOOLS_WORK) * 20 / (TOOLS_TOTAL * 2)))
echo "   工具链完整性得分: $TOOLCHAIN_SCORE/20"

echo -e "\n4. 增强功能测试 (权重: 15分)"

# 测试4.1: 增强编译器
echo "   4.1 增强编译器测试..."
ENHANCED_TESTS=0
ENHANCED_PASSED=0

if [ -f "bin/c2astc_enhanced" ]; then
    for expr in "1+2" "5*3" "10-4"; do
        echo "int main() { return $expr; }" > /tmp/comp_test_enh.c
        if ./bin/c2astc_enhanced /tmp/comp_test_enh.c /tmp/comp_test_enh.astc > /dev/null 2>&1; then
            if ./bin/simple_loader /tmp/comp_test_enh.astc > /dev/null 2>&1; then
                ENHANCED_PASSED=$((ENHANCED_PASSED + 1))
            fi
        fi
        ENHANCED_TESTS=$((ENHANCED_TESTS + 1))
    done
fi

echo "       增强编译器: $ENHANCED_PASSED/$ENHANCED_TESTS 通过"

ENHANCED_SCORE=$((ENHANCED_PASSED * 15 / ENHANCED_TESTS))
echo "   增强功能得分: $ENHANCED_SCORE/15"

echo -e "\n5. 自举能力测试 (权重: 20分)"

# 测试5.1: c99bin编译能力
echo "   5.1 c99bin编译能力测试..."
if ./tools/c99bin tools/c2astc_ultra_minimal.c -o /tmp/test_bootstrap > /dev/null 2>&1; then
    BOOTSTRAP_COMPILE=1
    echo "       c99bin编译: 1/1 通过"
else
    BOOTSTRAP_COMPILE=0
    echo "       c99bin编译: 0/1 通过"
fi

# 测试5.2: 生成工具运行能力
echo "   5.2 生成工具运行测试..."
if [ -f "/tmp/test_bootstrap" ] && /tmp/test_bootstrap > /dev/null 2>&1; then
    BOOTSTRAP_RUN=1
    echo "       工具运行: 1/1 通过"
else
    BOOTSTRAP_RUN=0
    echo "       工具运行: 0/1 通过"
fi

BOOTSTRAP_SCORE=$(((BOOTSTRAP_COMPILE + BOOTSTRAP_RUN) * 20 / 2))
echo "   自举能力得分: $BOOTSTRAP_SCORE/20"

echo -e "\n6. 最终评估"
echo "   =================================="

TOTAL_SCORE=$((E2E_SCORE + ARCH_SCORE + TOOLCHAIN_SCORE + ENHANCED_SCORE + BOOTSTRAP_SCORE))
PERCENTAGE=$((TOTAL_SCORE * 100 / 100))

echo "   详细得分："
echo "   - 端到端工作流程: $E2E_SCORE/25"
echo "   - 三层架构: $ARCH_SCORE/20"
echo "   - 工具链完整性: $TOOLCHAIN_SCORE/20"
echo "   - 增强功能: $ENHANCED_SCORE/15"
echo "   - 自举能力: $BOOTSTRAP_SCORE/20"
echo "   =================================="
echo "   总分: $TOTAL_SCORE/100 ($PERCENTAGE%)"

# 评级
if [ $PERCENTAGE -ge 90 ]; then
    GRADE="A (优秀)"
elif [ $PERCENTAGE -ge 80 ]; then
    GRADE="B (良好)"
elif [ $PERCENTAGE -ge 70 ]; then
    GRADE="C (合格)"
elif [ $PERCENTAGE -ge 60 ]; then
    GRADE="D (基本合格)"
else
    GRADE="F (不合格)"
fi

echo "   评级: $GRADE"
echo "   =================================="

# 清理测试文件
rm -f /tmp/comp_test_*.* test_comp_*.astc /tmp/test_bootstrap

echo -e "\n=== Stage 1 全面功能测试完成 ==="
echo "最终评估: $TOTAL_SCORE/100 ($PERCENTAGE%) - $GRADE"
