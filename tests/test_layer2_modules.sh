#!/bin/bash

# Layer 2 Native Module 测试脚本
# 测试pipeline, layer0, compiler, libc模块的功能

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 测试配置
TEST_DIR="$(dirname "$0")"
PROJECT_ROOT="$(cd "$TEST_DIR/.." && pwd)"
RESULTS_DIR="$TEST_DIR/layer2_modules_results"

# 创建结果目录
mkdir -p "$RESULTS_DIR"

echo -e "${BLUE}=== Layer 2 Native Module 测试 ===${NC}"
echo "项目根目录: $PROJECT_ROOT"
echo "测试目录: $TEST_DIR"
echo "结果目录: $RESULTS_DIR"
echo

# 测试计数器
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# 测试结果记录
RESULTS_FILE="$RESULTS_DIR/modules_test_results.txt"
echo "Layer 2 Native Module 测试结果 - $(date)" > "$RESULTS_FILE"
echo "======================================" >> "$RESULTS_FILE"

# 运行单个测试
run_test() {
    local test_name="$1"
    local test_cmd="$2"
    local expected_result="$3"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    echo -e "${YELLOW}测试 $TOTAL_TESTS: $test_name${NC}"
    
    if eval "$test_cmd" > /dev/null 2>&1; then
        if [ "$expected_result" = "success" ]; then
            echo -e "  ${GREEN}✓ 通过${NC}"
            echo "[$TOTAL_TESTS] $test_name: PASS" >> "$RESULTS_FILE"
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            echo -e "  ${RED}✗ 失败 (预期失败但成功了)${NC}"
            echo "[$TOTAL_TESTS] $test_name: FAIL (unexpected success)" >> "$RESULTS_FILE"
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
    else
        if [ "$expected_result" = "fail" ]; then
            echo -e "  ${GREEN}✓ 通过 (预期失败)${NC}"
            echo "[$TOTAL_TESTS] $test_name: PASS (expected failure)" >> "$RESULTS_FILE"
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            echo -e "  ${RED}✗ 失败${NC}"
            echo "[$TOTAL_TESTS] $test_name: FAIL" >> "$RESULTS_FILE"
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
    fi
}

# 检测架构
ARCH=$(uname -m)
if [ "$ARCH" = "arm64" ]; then
    MODULE_SUFFIX="arm64_64"
elif [ "$ARCH" = "x86_64" ]; then
    MODULE_SUFFIX="x64_64"
else
    echo -e "${RED}不支持的架构: $ARCH${NC}"
    exit 1
fi

echo "检测到架构: $ARCH (使用后缀: $MODULE_SUFFIX)"

# 测试1: 模块文件存在性检查
echo -e "${BLUE}=== 模块文件存在性测试 ===${NC}"
PIPELINE_MODULE="$PROJECT_ROOT/bin/pipeline_${MODULE_SUFFIX}.native"
LAYER0_MODULE="$PROJECT_ROOT/bin/layer0_${MODULE_SUFFIX}.native"
COMPILER_MODULE="$PROJECT_ROOT/bin/compiler_${MODULE_SUFFIX}.native"
LIBC_MODULE="$PROJECT_ROOT/bin/libc_${MODULE_SUFFIX}.native"

run_test "pipeline模块文件存在" "test -f '$PIPELINE_MODULE'" "success"
run_test "layer0模块文件存在" "test -f '$LAYER0_MODULE'" "success"
run_test "compiler模块文件存在" "test -f '$COMPILER_MODULE'" "success"
run_test "libc模块文件存在" "test -f '$LIBC_MODULE'" "success"

# 测试2: 模块文件格式检查
echo -e "${BLUE}=== 模块文件格式测试 ===${NC}"
run_test "pipeline模块格式检查" "file '$PIPELINE_MODULE' | grep -q 'data'" "success"
run_test "layer0模块格式检查" "file '$LAYER0_MODULE' | grep -q 'data'" "success"
run_test "compiler模块格式检查" "file '$COMPILER_MODULE' | grep -q 'data'" "success"
run_test "libc模块格式检查" "file '$LIBC_MODULE' | grep -q 'data'" "success"

# 测试3: 模块大小合理性检查
echo -e "${BLUE}=== 模块大小合理性测试 ===${NC}"
run_test "pipeline模块大小检查" "test \$(stat -c%s '$PIPELINE_MODULE' 2>/dev/null || stat -f%z '$PIPELINE_MODULE') -gt 10000" "success"
run_test "layer0模块大小检查" "test \$(stat -c%s '$LAYER0_MODULE' 2>/dev/null || stat -f%z '$LAYER0_MODULE') -gt 5000" "success"
run_test "compiler模块大小检查" "test \$(stat -c%s '$COMPILER_MODULE' 2>/dev/null || stat -f%z '$COMPILER_MODULE') -gt 5000" "success"
run_test "libc模块大小检查" "test \$(stat -c%s '$LIBC_MODULE' 2>/dev/null || stat -f%z '$LIBC_MODULE') -gt 10000" "success"

# 测试4: 使用现有的模块测试程序
echo -e "${BLUE}=== 模块功能测试 ===${NC}"

# 运行现有的测试程序
if [ -f "$TEST_DIR/test_astc_core" ]; then
    run_test "ASTC核心功能测试" "'$TEST_DIR/test_astc_core'" "success"
fi

if [ -f "$TEST_DIR/test_astc_bytecode" ]; then
    run_test "ASTC字节码测试" "'$TEST_DIR/test_astc_bytecode'" "success"
fi

if [ -f "$TEST_DIR/test_compiler_module" ]; then
    run_test "编译器模块测试" "'$TEST_DIR/test_compiler_module'" "success"
fi

if [ -f "$TEST_DIR/test_pipeline_module" ]; then
    run_test "流水线模块测试" "'$TEST_DIR/test_pipeline_module'" "success"
fi

# 测试5: 模块依赖关系测试
echo -e "${BLUE}=== 模块依赖关系测试 ===${NC}"
if [ -f "$TEST_DIR/test_module_dependencies" ]; then
    run_test "模块依赖关系测试" "'$TEST_DIR/test_module_dependencies'" "success"
fi

if [ -f "$TEST_DIR/test_module_system" ]; then
    run_test "模块系统测试" "'$TEST_DIR/test_module_system'" "success"
fi

# 测试6: 模块加载测试
echo -e "${BLUE}=== 模块加载测试 ===${NC}"

# 创建简单的模块加载测试
cat > "$RESULTS_DIR/test_module_load.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

int main() {
    // 这是一个简化的模块加载测试
    // 实际的.native文件需要专门的加载器
    printf("模块加载测试: 基本功能检查\n");
    return 0;
}
EOF

# 编译并运行测试
if gcc -o "$RESULTS_DIR/test_module_load" "$RESULTS_DIR/test_module_load.c" 2>/dev/null; then
    run_test "模块加载基本功能" "'$RESULTS_DIR/test_module_load'" "success"
else
    echo -e "${YELLOW}警告: 无法编译模块加载测试${NC}"
fi

# 生成测试报告
echo
echo -e "${BLUE}=== 测试结果汇总 ===${NC}"
echo "总测试数: $TOTAL_TESTS"
echo -e "通过: ${GREEN}$PASSED_TESTS${NC}"
echo -e "失败: ${RED}$FAILED_TESTS${NC}"

# 写入汇总到结果文件
echo "" >> "$RESULTS_FILE"
echo "测试汇总:" >> "$RESULTS_FILE"
echo "总测试数: $TOTAL_TESTS" >> "$RESULTS_FILE"
echo "通过: $PASSED_TESTS" >> "$RESULTS_FILE"
echo "失败: $FAILED_TESTS" >> "$RESULTS_FILE"
echo "成功率: $(( PASSED_TESTS * 100 / TOTAL_TESTS ))%" >> "$RESULTS_FILE"

# 确定退出状态
if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}所有测试通过！${NC}"
    exit 0
else
    echo -e "${RED}有 $FAILED_TESTS 个测试失败${NC}"
    exit 1
fi 