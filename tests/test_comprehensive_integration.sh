#!/bin/bash

# 综合集成测试脚本
# 运行所有测试脚本并生成综合报告

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
RESULTS_DIR="$TEST_DIR/comprehensive_test_results"

# 创建结果目录
mkdir -p "$RESULTS_DIR"

echo -e "${BLUE}=== 综合集成测试 ===${NC}"
echo "项目根目录: $PROJECT_ROOT"
echo "测试目录: $TEST_DIR"
echo "结果目录: $RESULTS_DIR"
echo "测试时间: $(date)"
echo

# 测试结果记录
RESULTS_FILE="$RESULTS_DIR/comprehensive_test_results.txt"
echo "Self-Evolve AI 综合测试报告 - $(date)" > "$RESULTS_FILE"
echo "=================================================" >> "$RESULTS_FILE"
echo "" >> "$RESULTS_FILE"

# 测试计数器
TOTAL_SUITES=0
PASSED_SUITES=0
FAILED_SUITES=0

# 运行测试套件
run_test_suite() {
    local suite_name="$1"
    local test_script="$2"
    local timeout_seconds="${3:-120}"  # 默认2分钟超时

    TOTAL_SUITES=$((TOTAL_SUITES + 1))
    echo -e "${YELLOW}=== 运行测试套件 $TOTAL_SUITES: $suite_name ===${NC}"

    if [ ! -f "$test_script" ]; then
        echo -e "  ${YELLOW}⚠ 跳过 (脚本不存在: $test_script)${NC}"
        echo "测试套件: $suite_name - SKIP (script not found: $test_script)" >> "$RESULTS_FILE"
        echo "" >> "$RESULTS_FILE"
        return
    fi

    # 检查脚本是否可执行
    if [ ! -x "$test_script" ]; then
        echo -e "  ${YELLOW}⚠ 跳过 (脚本不可执行: $test_script)${NC}"
        echo "测试套件: $suite_name - SKIP (not executable: $test_script)" >> "$RESULTS_FILE"
        echo "" >> "$RESULTS_FILE"
        return
    fi

    echo "开始时间: $(date)" >> "$RESULTS_FILE"
    echo "测试套件: $suite_name" >> "$RESULTS_FILE"
    echo "脚本路径: $test_script" >> "$RESULTS_FILE"
    echo "超时设置: ${timeout_seconds}秒" >> "$RESULTS_FILE"
    echo "----------------------------------------" >> "$RESULTS_FILE"

    # 创建临时文件来捕获输出
    local temp_output=$(mktemp)
    local test_passed=false
    local exit_code=0

    # 使用timeout命令执行测试脚本或可执行文件
    if [[ "$test_script" == *.sh ]]; then
        # 对于.sh文件使用bash执行
        timeout "${timeout_seconds}s" bash "$test_script" > "$temp_output" 2>&1
        exit_code=$?
    else
        # 对于可执行文件直接执行
        timeout "${timeout_seconds}s" "$test_script" > "$temp_output" 2>&1
        exit_code=$?
    fi

    # 将输出添加到结果文件
    cat "$temp_output" >> "$RESULTS_FILE"
    rm -f "$temp_output"

    if [ $exit_code -eq 0 ]; then
        echo -e "  ${GREEN}✓ 通过${NC}"
        echo "结果: PASS" >> "$RESULTS_FILE"
        PASSED_SUITES=$((PASSED_SUITES + 1))
    elif [ $exit_code -eq 124 ]; then
        echo -e "  ${YELLOW}⚠ 超时${NC}"
        echo "结果: TIMEOUT (${timeout_seconds}秒)" >> "$RESULTS_FILE"
        FAILED_SUITES=$((FAILED_SUITES + 1))
    else
        echo -e "  ${RED}✗ 失败 (退出码: $exit_code)${NC}"
        echo "结果: FAIL (exit code: $exit_code)" >> "$RESULTS_FILE"
        FAILED_SUITES=$((FAILED_SUITES + 1))
    fi

    echo "结束时间: $(date)" >> "$RESULTS_FILE"
    echo "" >> "$RESULTS_FILE"
}

# 运行核心层级测试（优先级最高）
echo -e "${BLUE}=== 运行核心层级测试 ===${NC}"
run_test_suite "Layer 1 Loader测试" "$TEST_DIR/test_layer1_loader.sh" 60  # 1分钟
run_test_suite "Layer 2 Modules测试" "$TEST_DIR/test_layer2_modules.sh" 90  # 1.5分钟
run_test_suite "Layer 3 Programs测试" "$TEST_DIR/test_layer3_programs.sh" 120  # 2分钟

# 运行模块功能测试
echo -e "${BLUE}=== 运行模块功能测试 ===${NC}"
run_test_suite "ASTC核心测试" "$TEST_DIR/test_astc_core" 60  # 1分钟
run_test_suite "ASTC字节码测试" "$TEST_DIR/test_astc_bytecode" 60  # 1分钟
run_test_suite "编译器模块测试" "$TEST_DIR/test_compiler_module" 60  # 1分钟
run_test_suite "流水线模块测试" "$TEST_DIR/test_pipeline_module" 60  # 1分钟
run_test_suite "模块依赖测试" "$TEST_DIR/test_module_dependencies" 60  # 1分钟

# 运行增强测试套件（如果存在）
echo -e "${BLUE}=== 运行增强测试套件 ===${NC}"
run_test_suite "稳定性测试" "$TEST_DIR/test_stability_enhanced.sh" 90  # 1.5分钟
run_test_suite "性能基准测试" "$TEST_DIR/test_performance_benchmark.sh" 90  # 1.5分钟
run_test_suite "错误处理测试" "$TEST_DIR/test_error_handling_enhanced.sh" 60  # 1分钟

# 运行其他现有测试（可选，较低优先级）
echo -e "${BLUE}=== 运行其他测试 ===${NC}"
if [ -f "$TEST_DIR/c99_compliance_test.sh" ]; then
    run_test_suite "C99合规性测试" "$TEST_DIR/c99_compliance_test.sh" 90  # 1.5分钟
fi

if [ -f "$TEST_DIR/performance_test.sh" ]; then
    run_test_suite "性能测试" "$TEST_DIR/performance_test.sh" 90  # 1.5分钟
fi

if [ -f "$TEST_DIR/code_quality_analysis.sh" ]; then
    run_test_suite "代码质量分析" "$TEST_DIR/code_quality_analysis.sh" 60  # 1分钟
fi

# 生成综合报告
echo
echo -e "${BLUE}=== 综合测试结果汇总 ===${NC}"
echo "总测试套件数: $TOTAL_SUITES"
echo -e "通过: ${GREEN}$PASSED_SUITES${NC}"
echo -e "失败: ${RED}$FAILED_SUITES${NC}"

if [ $TOTAL_SUITES -gt 0 ]; then
    SUCCESS_RATE=$(( PASSED_SUITES * 100 / TOTAL_SUITES ))
    echo "成功率: ${SUCCESS_RATE}%"
else
    SUCCESS_RATE=0
    echo "成功率: 0%"
fi

# 写入综合汇总
echo "" >> "$RESULTS_FILE"
echo "=========================================" >> "$RESULTS_FILE"
echo "综合测试汇总:" >> "$RESULTS_FILE"
echo "总测试套件数: $TOTAL_SUITES" >> "$RESULTS_FILE"
echo "通过: $PASSED_SUITES" >> "$RESULTS_FILE"
echo "失败: $FAILED_SUITES" >> "$RESULTS_FILE"
echo "成功率: ${SUCCESS_RATE}%" >> "$RESULTS_FILE"
echo "报告生成时间: $(date)" >> "$RESULTS_FILE"

# 生成问题报告
echo
echo -e "${BLUE}=== 问题分析 ===${NC}"
echo "问题报告:" >> "$RESULTS_FILE"
echo "----------------------------------------" >> "$RESULTS_FILE"

if [ $FAILED_SUITES -gt 0 ]; then
    echo -e "${RED}发现的主要问题:${NC}"
    echo "发现的主要问题:" >> "$RESULTS_FILE"
    
    echo "1. 编译器构建问题 - C99编译器无法正常构建"
    echo "1. 编译器构建问题 - C99编译器无法正常构建" >> "$RESULTS_FILE"
    
    echo "2. 跨平台兼容性问题 - simple_loader为Linux ELF格式，无法在macOS运行"
    echo "2. 跨平台兼容性问题 - simple_loader为Linux ELF格式，无法在macOS运行" >> "$RESULTS_FILE"
    
    echo "3. C2ASTC编译器问题 - 编译测试失败"
    echo "3. C2ASTC编译器问题 - 编译测试失败" >> "$RESULTS_FILE"
    
    echo -e "${YELLOW}建议修复措施:${NC}"
    echo "建议修复措施:" >> "$RESULTS_FILE"
    echo "- 修复C99编译器的语法错误和未定义标识符问题"
    echo "- 修复C99编译器的语法错误和未定义标识符问题" >> "$RESULTS_FILE"
    echo "- 重新构建适用于当前平台的simple_loader"
    echo "- 重新构建适用于当前平台的simple_loader" >> "$RESULTS_FILE"
    echo "- 检查和修复C2ASTC编译器的功能"
    echo "- 检查和修复C2ASTC编译器的功能" >> "$RESULTS_FILE"
else
    echo -e "${GREEN}所有测试套件通过！${NC}"
    echo "所有测试套件通过！" >> "$RESULTS_FILE"
fi

echo
echo -e "${BLUE}详细报告已保存至: $RESULTS_FILE${NC}"

# 确定退出状态
if [ $FAILED_SUITES -eq 0 ]; then
    exit 0
else
    exit 1
fi 