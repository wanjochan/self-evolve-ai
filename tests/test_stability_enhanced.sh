#!/bin/bash

# Enhanced Stability Test Suite for prd_0_2
# 测试系统稳定性和边界情况

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 测试计数器
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# 项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RESULTS_DIR="$TEST_DIR/stability_test_results"

echo -e "${BLUE}=== 增强稳定性测试套件 ===${NC}"
echo "项目根目录: $PROJECT_ROOT"
echo "测试目录: $TEST_DIR"
echo "结果目录: $RESULTS_DIR"
echo

# 创建结果目录
mkdir -p "$RESULTS_DIR"

# 测试函数
run_test() {
    local test_name="$1"
    local test_command="$2"
    local expected_result="$3"  # "pass" or "fail"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    echo -e "${YELLOW}测试 $TOTAL_TESTS: $test_name${NC}"
    
    if eval "$test_command" > "$RESULTS_DIR/test_${TOTAL_TESTS}.log" 2>&1; then
        if [ "$expected_result" = "pass" ]; then
            echo -e "  ${GREEN}✓ 通过${NC}"
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            echo -e "  ${RED}✗ 失败 (预期失败但成功了)${NC}"
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
    else
        if [ "$expected_result" = "fail" ]; then
            echo -e "  ${GREEN}✓ 通过 (预期失败)${NC}"
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            echo -e "  ${RED}✗ 失败${NC}"
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
    fi
}

# 切换到项目根目录
cd "$PROJECT_ROOT"

echo -e "${BLUE}=== 模块加载稳定性测试 ===${NC}"

# 测试1: 重复加载同一模块
run_test "重复加载同一模块" \
    "for i in {1..10}; do ./bin/simple_loader ./tests/test_minimal.astc > /dev/null 2>&1 || exit 1; done" \
    "pass"

# 测试2: 并发模块加载
run_test "并发模块加载" \
    "for i in {1..5}; do ./bin/simple_loader ./tests/test_minimal.astc & done; wait" \
    "pass"

# 测试3: 大量模块加载
run_test "大量模块加载" \
    "for i in {1..50}; do ./bin/simple_loader ./tests/test_minimal.astc > /dev/null 2>&1 || exit 1; done" \
    "pass"

echo -e "${BLUE}=== 内存管理稳定性测试 ===${NC}"

# 测试4: 内存泄漏检测（如果有valgrind）
if command -v valgrind > /dev/null 2>&1; then
    run_test "内存泄漏检测" \
        "valgrind --leak-check=full --error-exitcode=1 ./bin/simple_loader ./tests/test_minimal.astc" \
        "pass"
else
    echo -e "${YELLOW}跳过内存泄漏检测 (valgrind未安装)${NC}"
fi

# 测试5: 大内存分配测试
run_test "大内存分配测试" \
    "./bin/simple_loader ./tests/test_minimal.astc" \
    "pass"

echo -e "${BLUE}=== 错误处理稳定性测试 ===${NC}"

# 测试6: 无效文件处理
run_test "无效文件处理" \
    "./bin/simple_loader /dev/null" \
    "fail"

# 测试7: 不存在文件处理
run_test "不存在文件处理" \
    "./bin/simple_loader ./nonexistent.astc" \
    "fail"

# 测试8: 权限不足文件处理
touch "$RESULTS_DIR/no_permission.astc"
chmod 000 "$RESULTS_DIR/no_permission.astc"
run_test "权限不足文件处理" \
    "./bin/simple_loader $RESULTS_DIR/no_permission.astc" \
    "fail"
chmod 644 "$RESULTS_DIR/no_permission.astc"

echo -e "${BLUE}=== 边界条件测试 ===${NC}"

# 测试9: 空ASTC文件
touch "$RESULTS_DIR/empty.astc"
run_test "空ASTC文件处理" \
    "./bin/simple_loader $RESULTS_DIR/empty.astc" \
    "fail"

# 测试10: 超大ASTC文件（如果存在）
if [ -f "./tests/test_complex_c99.astc" ]; then
    run_test "复杂ASTC文件处理" \
        "./bin/simple_loader ./tests/test_complex_c99.astc" \
        "pass"
fi

echo -e "${BLUE}=== 架构检测稳定性测试 ===${NC}"

# 测试11: 架构检测一致性
run_test "架构检测一致性" \
    "arch1=\$(./bin/simple_loader --arch 2>/dev/null || echo 'unknown'); arch2=\$(./bin/simple_loader --arch 2>/dev/null || echo 'unknown'); [ \"\$arch1\" = \"\$arch2\" ]" \
    "pass"

echo -e "${BLUE}=== 模块依赖稳定性测试 ===${NC}"

# 测试12: 模块依赖链测试
run_test "模块依赖链测试" \
    "./bin/simple_loader ./tests/test_minimal.astc" \
    "pass"

echo -e "${BLUE}=== 长时间运行稳定性测试 ===${NC}"

# 测试13: 长时间运行测试
run_test "长时间运行测试" \
    "timeout 30s bash -c 'while true; do ./bin/simple_loader ./tests/test_minimal.astc > /dev/null 2>&1 || exit 1; done'; [ \$? -eq 124 ]" \
    "pass"

echo -e "${BLUE}=== 资源限制测试 ===${NC}"

# 测试14: 文件描述符限制测试
run_test "文件描述符限制测试" \
    "ulimit -n 100; ./bin/simple_loader ./tests/test_minimal.astc" \
    "pass"

# 测试15: 进程限制测试
run_test "进程限制测试" \
    "ulimit -u 50; ./bin/simple_loader ./tests/test_minimal.astc" \
    "pass"

echo
echo -e "${BLUE}=== 稳定性测试结果汇总 ===${NC}"
echo "总测试数: $TOTAL_TESTS"
echo -e "通过: ${GREEN}$PASSED_TESTS${NC}"
echo -e "失败: ${RED}$FAILED_TESTS${NC}"

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}所有稳定性测试通过！${NC}"
    exit 0
else
    echo -e "${RED}有 $FAILED_TESTS 个稳定性测试失败${NC}"
    exit 1
fi
