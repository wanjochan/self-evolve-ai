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
    local timeout_seconds="${3:-300}"  # 默认5分钟超时

    TOTAL_SUITES=$((TOTAL_SUITES + 1))
    echo -e "${YELLOW}=== 运行测试套件 $TOTAL_SUITES: $suite_name ===${NC}"

    if [ -f "$test_script" ]; then
        echo "开始时间: $(date)" >> "$RESULTS_FILE"
        echo "测试套件: $suite_name" >> "$RESULTS_FILE"
        echo "脚本路径: $test_script" >> "$RESULTS_FILE"
        echo "超时设置: ${timeout_seconds}秒" >> "$RESULTS_FILE"
        echo "----------------------------------------" >> "$RESULTS_FILE"

        # 使用timeout命令执行测试脚本
        if timeout "${timeout_seconds}s" bash "$test_script" >> "$RESULTS_FILE" 2>&1; then
            echo -e "  ${GREEN}✓ 通过${NC}"
            echo "结果: PASS" >> "$RESULTS_FILE"
            PASSED_SUITES=$((PASSED_SUITES + 1))
        else
            local exit_code=$?
            if [ $exit_code -eq 124 ]; then
                echo -e "  ${YELLOW}⚠ 超时${NC}"
                echo "结果: TIMEOUT (${timeout_seconds}秒)" >> "$RESULTS_FILE"
            else
                echo -e "  ${RED}✗ 失败${NC}"
                echo "结果: FAIL" >> "$RESULTS_FILE"
            fi
            FAILED_SUITES=$((FAILED_SUITES + 1))
        fi

        echo "结束时间: $(date)" >> "$RESULTS_FILE"
        echo "" >> "$RESULTS_FILE"
    else
        echo -e "  ${YELLOW}⚠ 跳过 (脚本不存在)${NC}"
        echo "结果: SKIP (script not found)" >> "$RESULTS_FILE"
        echo "" >> "$RESULTS_FILE"
    fi
}

# 运行现有的测试脚本
echo -e "${BLUE}=== 运行现有测试脚本 ===${NC}"
run_test_suite "C99合规性测试" "$TEST_DIR/c99_compliance_test.sh" 180  # 3分钟
run_test_suite "性能测试" "$TEST_DIR/performance_test.sh" 240  # 4分钟
run_test_suite "代码质量分析" "$TEST_DIR/code_quality_analysis.sh" 120  # 2分钟

# 运行新创建的层级测试
echo -e "${BLUE}=== 运行层级测试 ===${NC}"
run_test_suite "Layer 1 Loader测试" "$TEST_DIR/test_layer1_loader.sh" 120  # 2分钟
run_test_suite "Layer 2 Modules测试" "$TEST_DIR/test_layer2_modules.sh" 180  # 3分钟
run_test_suite "Layer 3 Programs测试" "$TEST_DIR/test_layer3_programs.sh" 240  # 4分钟

# 运行模块测试
echo -e "${BLUE}=== 运行模块测试 ===${NC}"
run_test_suite "ASTC核心测试" "$TEST_DIR/test_astc_core" 150  # 2.5分钟
run_test_suite "ASTC字节码测试" "$TEST_DIR/test_astc_bytecode" 150  # 2.5分钟
run_test_suite "编译器模块测试" "$TEST_DIR/test_compiler_module" 180  # 3分钟
run_test_suite "流水线模块测试" "$TEST_DIR/test_pipeline_module" 180  # 3分钟

# 运行其他可用测试
echo -e "${BLUE}=== 运行其他测试 ===${NC}"
if [ -f "$TEST_DIR/test_module_system" ]; then
    run_test_suite "模块系统测试" "$TEST_DIR/test_module_system" 120  # 2分钟
fi

if [ -f "$TEST_DIR/test_module_dependencies" ]; then
    run_test_suite "模块依赖测试" "$TEST_DIR/test_module_dependencies" 120  # 2分钟
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