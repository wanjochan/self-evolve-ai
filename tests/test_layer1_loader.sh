#!/bin/bash

# Layer 1 Loader 测试脚本
# 测试simple_loader的架构检测和模块加载功能

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
SIMPLE_LOADER="$PROJECT_ROOT/bin/simple_loader"
RESULTS_DIR="$TEST_DIR/layer1_loader_results"

# 创建结果目录
mkdir -p "$RESULTS_DIR"

echo -e "${BLUE}=== Layer 1 Loader 测试 ===${NC}"
echo "项目根目录: $PROJECT_ROOT"
echo "测试目录: $TEST_DIR"
echo "结果目录: $RESULTS_DIR"
echo

# 测试计数器
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# 测试结果记录
RESULTS_FILE="$RESULTS_DIR/loader_test_results.txt"
echo "Layer 1 Loader 测试结果 - $(date)" > "$RESULTS_FILE"
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

# 检查simple_loader是否存在
if [ ! -f "$SIMPLE_LOADER" ]; then
    echo -e "${RED}错误: simple_loader不存在: $SIMPLE_LOADER${NC}"
    echo "请先运行 bash build_core.sh 构建系统"
    exit 1
fi

# 测试1: 检查架构检测
echo -e "${BLUE}=== 架构检测测试 ===${NC}"
run_test "架构检测功能" "file -L '$SIMPLE_LOADER' | grep -q 'ELF\\|Mach-O\\|PE32'" "success"

# 测试2: 检查模块加载能力
echo -e "${BLUE}=== 模块加载测试 ===${NC}"

# 创建测试用的简单ASTC程序
TEST_ASTC="$RESULTS_DIR/test_simple.astc"
cat > "$TEST_ASTC" << 'EOF'
ASTC
version 1
flags 0x00000000
instruction_count 2
code_size 1024
entry_point 0
instructions:
  LOAD_CONST 42
  RETURN
EOF

run_test "ASTC程序文件存在性检查" "test -f '$TEST_ASTC'" "success"

# 测试3: 检查模块文件存在性
echo -e "${BLUE}=== 模块文件检测测试 ===${NC}"
run_test "pipeline模块存在性" "test -f '$PROJECT_ROOT/bin/pipeline_x64_64.native' -o -f '$PROJECT_ROOT/bin/pipeline_arm64_64.native'" "success"
run_test "layer0模块存在性" "test -f '$PROJECT_ROOT/bin/layer0_x64_64.native' -o -f '$PROJECT_ROOT/bin/layer0_arm64_64.native'" "success"
run_test "compiler模块存在性" "test -f '$PROJECT_ROOT/bin/compiler_x64_64.native' -o -f '$PROJECT_ROOT/bin/compiler_arm64_64.native'" "success"
run_test "libc模块存在性" "test -f '$PROJECT_ROOT/bin/libc_x64_64.native' -o -f '$PROJECT_ROOT/bin/libc_arm64_64.native'" "success"

# 测试4: 错误处理测试
echo -e "${BLUE}=== 错误处理测试 ===${NC}"
run_test "不存在的ASTC文件处理" "'$SIMPLE_LOADER' /nonexistent/file.astc" "fail"
run_test "无效的ASTC文件处理" "echo 'invalid' > '$RESULTS_DIR/invalid.astc' && '$SIMPLE_LOADER' '$RESULTS_DIR/invalid.astc'" "fail"

# 测试5: 基本功能测试
echo -e "${BLUE}=== 基本功能测试 ===${NC}"

# 使用现有的测试程序
if [ -f "$PROJECT_ROOT/examples/test_c99.astc" ]; then
    run_test "示例ASTC程序加载" "'$SIMPLE_LOADER' '$PROJECT_ROOT/examples/test_c99.astc'" "success"
fi

if [ -f "$PROJECT_ROOT/tests/test_minimal.astc" ]; then
    run_test "最小ASTC程序加载" "'$SIMPLE_LOADER' '$PROJECT_ROOT/tests/test_minimal.astc'" "success"
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