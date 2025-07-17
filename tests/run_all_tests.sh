#!/bin/bash
#
# run_all_tests.sh - 统一的测试运行脚本
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

# 测试统计
TOTAL_SUITES=0
PASSED_SUITES=0
FAILED_SUITES=0

# 测试套件列表
declare -A TEST_SUITES=(
    ["unit"]="单元测试"
    ["integration"]="集成测试"
    ["performance"]="性能测试"
    ["stress"]="压力测试"
    ["security"]="安全测试"
    ["regression"]="回归测试"
    ["compatibility"]="兼容性测试"
)

print_header() {
    echo -e "${CYAN}================================${NC}"
    echo -e "${CYAN}$1${NC}"
    echo -e "${CYAN}================================${NC}"
}

print_suite_result() {
    local suite_name="$1"
    local result="$2"
    
    TOTAL_SUITES=$((TOTAL_SUITES + 1))
    if [ "$result" = "PASS" ]; then
        echo -e "${GREEN}✅ PASS${NC}: $suite_name"
        PASSED_SUITES=$((PASSED_SUITES + 1))
    else
        echo -e "${RED}❌ FAIL${NC}: $suite_name"
        FAILED_SUITES=$((FAILED_SUITES + 1))
    fi
}

# 运行单元测试
run_unit_tests() {
    print_header "运行单元测试"
    
    local unit_tests_passed=true
    
    # 编译并运行ASTC单元测试
    if [ -f "$PROJECT_ROOT/tests/test_unit_astc.c" ]; then
        echo "编译ASTC单元测试..."
        if gcc "$PROJECT_ROOT/tests/test_unit_astc.c" "$PROJECT_ROOT/tests/core_test_framework.c" \
               -I"$PROJECT_ROOT" -o "$PROJECT_ROOT/tests/test_unit_astc" 2>/dev/null; then
            echo "运行ASTC单元测试..."
            if "$PROJECT_ROOT/tests/test_unit_astc" >/dev/null 2>&1; then
                echo "✅ ASTC单元测试通过"
            else
                echo "❌ ASTC单元测试失败"
                unit_tests_passed=false
            fi
            rm -f "$PROJECT_ROOT/tests/test_unit_astc"
        else
            echo "❌ ASTC单元测试编译失败"
            unit_tests_passed=false
        fi
    else
        echo "⚠️  ASTC单元测试文件不存在"
    fi
    
    if $unit_tests_passed; then
        print_suite_result "单元测试" "PASS"
    else
        print_suite_result "单元测试" "FAIL"
    fi
}

# 运行集成测试
run_integration_tests() {
    print_header "运行集成测试"
    
    local integration_tests=(
        "test_c99bin_functionality.sh"
        "test_module_stability.sh"
        "test_stability_enhanced"
    )
    
    local integration_passed=true
    
    for test in "${integration_tests[@]}"; do
        local test_path="$PROJECT_ROOT/tests/$test"
        if [ -f "$test_path" ]; then
            echo "运行 $test..."
            if [ "${test##*.}" = "sh" ]; then
                chmod +x "$test_path"
                if "$test_path" >/dev/null 2>&1; then
                    echo "✅ $test 通过"
                else
                    echo "❌ $test 失败"
                    integration_passed=false
                fi
            else
                if [ -x "$test_path" ] && "$test_path" >/dev/null 2>&1; then
                    echo "✅ $test 通过"
                else
                    echo "❌ $test 失败"
                    integration_passed=false
                fi
            fi
        else
            echo "⚠️  $test 不存在"
        fi
    done
    
    if $integration_passed; then
        print_suite_result "集成测试" "PASS"
    else
        print_suite_result "集成测试" "FAIL"
    fi
}

# 运行性能测试
run_performance_tests() {
    print_header "运行性能测试"
    
    local performance_tests=(
        "test_performance_c99bin.sh"
        "test_performance_comparison.sh"
    )
    
    local performance_passed=true
    
    for test in "${performance_tests[@]}"; do
        local test_path="$PROJECT_ROOT/tests/$test"
        if [ -f "$test_path" ]; then
            echo "运行 $test..."
            chmod +x "$test_path"
            if "$test_path" >/dev/null 2>&1; then
                echo "✅ $test 通过"
            else
                echo "❌ $test 失败"
                performance_passed=false
            fi
        else
            echo "⚠️  $test 不存在，跳过"
        fi
    done
    
    if $performance_passed; then
        print_suite_result "性能测试" "PASS"
    else
        print_suite_result "性能测试" "FAIL"
    fi
}

# 运行压力测试
run_stress_tests() {
    print_header "运行压力测试"
    
    local stress_test="$PROJECT_ROOT/tests/test_stress_module_system.sh"
    if [ -f "$stress_test" ]; then
        echo "运行模块系统压力测试..."
        chmod +x "$stress_test"
        if "$stress_test" >/dev/null 2>&1; then
            print_suite_result "压力测试" "PASS"
        else
            print_suite_result "压力测试" "FAIL"
        fi
    else
        echo "⚠️  压力测试不存在，跳过"
        print_suite_result "压力测试" "PASS"
    fi
}

# 运行安全测试
run_security_tests() {
    print_header "运行安全测试"
    
    local security_test="$PROJECT_ROOT/tests/test_security_validation.sh"
    if [ -f "$security_test" ]; then
        echo "运行安全验证测试..."
        chmod +x "$security_test"
        if "$security_test" >/dev/null 2>&1; then
            print_suite_result "安全测试" "PASS"
        else
            print_suite_result "安全测试" "FAIL"
        fi
    else
        echo "⚠️  安全测试不存在，跳过"
        print_suite_result "安全测试" "PASS"
    fi
}

# 运行回归测试
run_regression_tests() {
    print_header "运行回归测试"
    
    local regression_tests=(
        "test_regression_basic.sh"
    )
    
    local regression_passed=true
    
    for test in "${regression_tests[@]}"; do
        local test_path="$PROJECT_ROOT/tests/$test"
        if [ -f "$test_path" ]; then
            echo "运行 $test..."
            chmod +x "$test_path"
            if "$test_path" >/dev/null 2>&1; then
                echo "✅ $test 通过"
            else
                echo "❌ $test 失败"
                regression_passed=false
            fi
        else
            echo "⚠️  $test 不存在，跳过"
        fi
    done
    
    if $regression_passed; then
        print_suite_result "回归测试" "PASS"
    else
        print_suite_result "回归测试" "FAIL"
    fi
}

# 运行兼容性测试
run_compatibility_tests() {
    print_header "运行兼容性测试"
    
    local compatibility_tests=(
        "test_compatibility_gcc.sh"
        "test_compatibility_platforms.sh"
    )
    
    local compatibility_passed=true
    
    for test in "${compatibility_tests[@]}"; do
        local test_path="$PROJECT_ROOT/tests/$test"
        if [ -f "$test_path" ]; then
            echo "运行 $test..."
            chmod +x "$test_path"
            if "$test_path" >/dev/null 2>&1; then
                echo "✅ $test 通过"
            else
                echo "❌ $test 失败"
                compatibility_passed=false
            fi
        else
            echo "⚠️  $test 不存在，跳过"
        fi
    done
    
    if $compatibility_passed; then
        print_suite_result "兼容性测试" "PASS"
    else
        print_suite_result "兼容性测试" "FAIL"
    fi
}

# 检查环境
check_environment() {
    print_header "检查测试环境"
    
    # 检查必要的工具
    local tools=("gcc" "make" "bash")
    local tools_ok=true
    
    for tool in "${tools[@]}"; do
        if command -v "$tool" >/dev/null 2>&1; then
            echo "✅ $tool: 可用"
        else
            echo "❌ $tool: 不可用"
            tools_ok=false
        fi
    done
    
    # 检查项目结构
    local required_dirs=("src" "tests" "bin")
    for dir in "${required_dirs[@]}"; do
        if [ -d "$PROJECT_ROOT/$dir" ]; then
            echo "✅ $dir/: 存在"
        else
            echo "❌ $dir/: 不存在"
            tools_ok=false
        fi
    done
    
    if ! $tools_ok; then
        echo -e "${RED}环境检查失败，无法运行测试${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}环境检查通过${NC}"
}

# 生成测试报告
generate_report() {
    print_header "测试报告"
    
    echo "测试套件统计:"
    echo "  总套件数: $TOTAL_SUITES"
    echo -e "  通过: ${GREEN}$PASSED_SUITES${NC}"
    echo -e "  失败: ${RED}$FAILED_SUITES${NC}"
    
    local success_rate=0
    if [ $TOTAL_SUITES -gt 0 ]; then
        success_rate=$((PASSED_SUITES * 100 / TOTAL_SUITES))
    fi
    
    echo "  成功率: $success_rate%"
    
    if [ $FAILED_SUITES -eq 0 ]; then
        echo -e "${GREEN}🎉 所有测试套件通过！${NC}"
        return 0
    else
        echo -e "${RED}⚠️  有 $FAILED_SUITES 个测试套件失败${NC}"
        return 1
    fi
}

# 主函数
main() {
    local test_type="${1:-all}"
    
    print_header "自动化测试套件"
    echo "项目根目录: $PROJECT_ROOT"
    echo "测试类型: $test_type"
    echo
    
    # 检查环境
    check_environment
    echo
    
    # 切换到项目根目录
    cd "$PROJECT_ROOT"
    
    # 根据参数运行特定测试
    case "$test_type" in
        "unit")
            run_unit_tests
            ;;
        "integration")
            run_integration_tests
            ;;
        "performance")
            run_performance_tests
            ;;
        "stress")
            run_stress_tests
            ;;
        "security")
            run_security_tests
            ;;
        "regression")
            run_regression_tests
            ;;
        "compatibility")
            run_compatibility_tests
            ;;
        "all"|*)
            run_unit_tests
            echo
            run_integration_tests
            echo
            run_performance_tests
            echo
            run_stress_tests
            echo
            run_security_tests
            echo
            run_regression_tests
            echo
            run_compatibility_tests
            ;;
    esac
    
    echo
    generate_report
}

# 显示帮助信息
show_help() {
    echo "用法: $0 [test_type]"
    echo
    echo "测试类型:"
    echo "  all          - 运行所有测试 (默认)"
    echo "  unit         - 单元测试"
    echo "  integration  - 集成测试"
    echo "  performance  - 性能测试"
    echo "  stress       - 压力测试"
    echo "  security     - 安全测试"
    echo "  regression   - 回归测试"
    echo "  compatibility- 兼容性测试"
    echo
    echo "示例:"
    echo "  $0           # 运行所有测试"
    echo "  $0 unit      # 只运行单元测试"
    echo "  $0 stress    # 只运行压力测试"
}

# 处理命令行参数
if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    show_help
    exit 0
fi

# 运行主函数
main "$@"
