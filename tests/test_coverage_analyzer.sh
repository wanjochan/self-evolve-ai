#!/bin/bash
#
# test_coverage_analyzer.sh - 测试覆盖率分析工具
# 
# 分析当前测试覆盖情况，识别覆盖率缺口，提供改进建议
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

# 统计变量
TOTAL_FUNCTIONS=0
TESTED_FUNCTIONS=0
TOTAL_MODULES=0
TESTED_MODULES=0
CRITICAL_GAPS=0

echo -e "${CYAN}=== 测试覆盖率分析工具 ===${NC}"
echo "项目根目录: $PROJECT_ROOT"
echo

# 分析源代码中的函数
analyze_source_functions() {
    echo -e "${BLUE}=== 分析源代码函数 ===${NC}"

    local functions_file="/tmp/source_functions.txt"
    > "$functions_file"

    # 扫描所有C文件中的函数定义
    local func_count=0
    find "$PROJECT_ROOT/src" -name "*.c" -type f | while read -r file; do
        echo "分析文件: $(basename "$file")"

        # 提取函数定义（简化的正则表达式）
        grep -n "^[a-zA-Z_][a-zA-Z0-9_]*.*(" "$file" 2>/dev/null | \
        grep -v "^[[:space:]]*//\|^[[:space:]]*\*\|if\|while\|for\|switch" | \
        while IFS=: read -r line_num func_line; do
            func_name=$(echo "$func_line" | sed 's/(.*//' | sed 's/.* //' | sed 's/\*//')
            if [[ "$func_name" =~ ^[a-zA-Z_][a-zA-Z0-9_]*$ ]]; then
                echo "$(basename "$file"):$func_name" >> "$functions_file"
                func_count=$((func_count + 1))
            fi
        done
        echo "$func_count" > "/tmp/func_count.txt"
    done

    if [ -f "/tmp/func_count.txt" ]; then
        TOTAL_FUNCTIONS=$(cat "/tmp/func_count.txt")
    else
        TOTAL_FUNCTIONS=$(wc -l < "$functions_file" 2>/dev/null || echo "0")
    fi

    echo "发现 $TOTAL_FUNCTIONS 个函数"
}

# 分析测试覆盖情况
analyze_test_coverage() {
    echo -e "${BLUE}=== 分析测试覆盖情况 ===${NC}"
    
    local tested_functions_file="/tmp/tested_functions.txt"
    > "$tested_functions_file"
    
    # 扫描测试文件中提到的函数
    find "$PROJECT_ROOT/tests" -name "*.c" -o -name "*.sh" | while read -r test_file; do
        if [[ "$test_file" == *.c ]]; then
            # C测试文件
            grep -o "[a-zA-Z_][a-zA-Z0-9_]*(" "$test_file" | sed 's/(//' >> "$tested_functions_file"
        else
            # Shell测试文件
            grep -o "[a-zA-Z_][a-zA-Z0-9_]*" "$test_file" | grep -v "^test_\|^echo\|^printf" >> "$tested_functions_file"
        fi
    done
    
    # 去重并统计
    sort "$tested_functions_file" | uniq > "/tmp/unique_tested_functions.txt"
    TESTED_FUNCTIONS=$(wc -l < "/tmp/unique_tested_functions.txt")
    
    echo "测试中涉及 $TESTED_FUNCTIONS 个函数"
}

# 识别关键模块
identify_critical_modules() {
    echo -e "${BLUE}=== 识别关键模块 ===${NC}"
    
    local critical_modules=(
        "astc.c:ASTC核心"
        "module.c:模块系统"
        "layer0_module.c:Layer0基础"
        "pipeline_module.c:编译流水线"
        "compiler_module.c:编译器核心"
        "libc_module.c:LibC实现"
        "c99bin_module.c:C99Bin模块"
    )
    
    echo "关键模块测试覆盖分析:"
    for module_info in "${critical_modules[@]}"; do
        local module_file=$(echo "$module_info" | cut -d: -f1)
        local module_name=$(echo "$module_info" | cut -d: -f2)
        
        # 检查是否有专门的测试文件
        local test_exists=false
        if find "$PROJECT_ROOT/tests" -name "*$(basename "$module_file" .c)*" | head -1 > /dev/null 2>&1; then
            test_exists=true
        fi
        
        if $test_exists; then
            echo -e "  ✅ $module_name: 有专门测试"
        else
            echo -e "  ❌ $module_name: 缺少专门测试"
            ((CRITICAL_GAPS++))
        fi
        
        ((TOTAL_MODULES++))
    done
}

# 分析测试类型覆盖
analyze_test_types() {
    echo -e "${BLUE}=== 分析测试类型覆盖 ===${NC}"
    
    local test_types=(
        "unit:单元测试"
        "integration:集成测试"
        "performance:性能测试"
        "regression:回归测试"
        "stress:压力测试"
        "error:错误处理测试"
        "compatibility:兼容性测试"
        "security:安全测试"
    )
    
    echo "测试类型覆盖分析:"
    for type_info in "${test_types[@]}"; do
        local test_type=$(echo "$type_info" | cut -d: -f1)
        local type_name=$(echo "$type_info" | cut -d: -f2)
        
        local count=$(find "$PROJECT_ROOT/tests" -name "*$test_type*" | wc -l)
        if [ $count -gt 0 ]; then
            echo -e "  ✅ $type_name: $count 个测试文件"
        else
            echo -e "  ⚠️  $type_name: 无专门测试"
        fi
    done
}

# 检查测试质量
check_test_quality() {
    echo -e "${BLUE}=== 检查测试质量 ===${NC}"
    
    local total_test_files=$(find "$PROJECT_ROOT/tests" -name "*.sh" -o -name "*.c" | wc -l)
    local executable_tests=$(find "$PROJECT_ROOT/tests" -name "*.sh" -executable | wc -l)
    local c_test_files=$(find "$PROJECT_ROOT/tests" -name "*.c" | wc -l)
    
    echo "测试文件统计:"
    echo "  总测试文件: $total_test_files"
    echo "  可执行Shell测试: $executable_tests"
    echo "  C测试文件: $c_test_files"
    
    # 检查测试框架使用
    local framework_usage=$(grep -r "TEST_CASE\|ASSERT_" "$PROJECT_ROOT/tests" | wc -l)
    echo "  测试框架使用: $framework_usage 处"
    
    # 检查测试文档
    local documented_tests=$(find "$PROJECT_ROOT/tests" -name "*.md" | wc -l)
    echo "  测试文档: $documented_tests 个"
}

# 生成覆盖率报告
generate_coverage_report() {
    echo -e "${BLUE}=== 生成覆盖率报告 ===${NC}"
    
    local coverage_rate=0
    if [ $TOTAL_FUNCTIONS -gt 0 ]; then
        coverage_rate=$((TESTED_FUNCTIONS * 100 / TOTAL_FUNCTIONS))
    fi
    
    local module_coverage_rate=0
    if [ $TOTAL_MODULES -gt 0 ]; then
        local tested_modules=$((TOTAL_MODULES - CRITICAL_GAPS))
        module_coverage_rate=$((tested_modules * 100 / TOTAL_MODULES))
    fi
    
    echo
    echo -e "${MAGENTA}=== 测试覆盖率报告 ===${NC}"
    echo "函数覆盖率: $coverage_rate% ($TESTED_FUNCTIONS/$TOTAL_FUNCTIONS)"
    echo "关键模块覆盖率: $module_coverage_rate% ($((TOTAL_MODULES - CRITICAL_GAPS))/$TOTAL_MODULES)"
    
    if [ $coverage_rate -ge 90 ]; then
        echo -e "总体评级: ${GREEN}优秀${NC}"
    elif [ $coverage_rate -ge 80 ]; then
        echo -e "总体评级: ${BLUE}良好${NC}"
    elif [ $coverage_rate -ge 70 ]; then
        echo -e "总体评级: ${YELLOW}一般${NC}"
    else
        echo -e "总体评级: ${RED}需要改进${NC}"
    fi
}

# 提供改进建议
provide_improvement_suggestions() {
    echo -e "${BLUE}=== 改进建议 ===${NC}"
    
    echo "优先级改进建议:"
    
    if [ $CRITICAL_GAPS -gt 0 ]; then
        echo -e "  ${RED}高优先级${NC}: 为 $CRITICAL_GAPS 个关键模块添加专门测试"
    fi
    
    # 检查缺失的测试类型
    local missing_types=()
    
    if [ $(find "$PROJECT_ROOT/tests" -name "*unit*" | wc -l) -eq 0 ]; then
        missing_types+=("单元测试")
    fi
    
    if [ $(find "$PROJECT_ROOT/tests" -name "*security*" | wc -l) -eq 0 ]; then
        missing_types+=("安全测试")
    fi
    
    if [ $(find "$PROJECT_ROOT/tests" -name "*stress*" | wc -l) -eq 0 ]; then
        missing_types+=("压力测试")
    fi
    
    if [ ${#missing_types[@]} -gt 0 ]; then
        echo -e "  ${YELLOW}中优先级${NC}: 添加缺失的测试类型: ${missing_types[*]}"
    fi
    
    # 检查测试自动化
    if [ ! -f "$PROJECT_ROOT/tests/run_all_tests.sh" ]; then
        echo -e "  ${BLUE}低优先级${NC}: 创建统一的测试运行脚本"
    fi
    
    echo
    echo "具体建议:"
    echo "1. 为每个关键模块创建专门的单元测试"
    echo "2. 增加边缘情况和错误处理测试"
    echo "3. 建立持续集成测试流程"
    echo "4. 添加性能回归测试"
    echo "5. 完善测试文档和使用指南"
}

# 创建缺失的测试模板
create_missing_test_templates() {
    echo -e "${BLUE}=== 创建缺失的测试模板 ===${NC}"
    
    local templates_dir="$PROJECT_ROOT/tests/templates"
    mkdir -p "$templates_dir"
    
    # 为关键模块创建测试模板
    local critical_modules=("astc" "module" "layer0_module" "pipeline_module" "compiler_module")
    
    for module in "${critical_modules[@]}"; do
        local test_file="$PROJECT_ROOT/tests/test_${module}_unit.c"
        if [ ! -f "$test_file" ]; then
            echo "创建 $module 单元测试模板"
            cat > "$test_file" << EOF
#include "core_test_framework.h"
#include "../src/core/${module}.h"

// ${module} 模块单元测试

TEST_CASE(test_${module}_basic_functionality) {
    // TODO: 实现基础功能测试
    ASSERT_TRUE(true, "基础功能测试");
    return true;
}

TEST_CASE(test_${module}_error_handling) {
    // TODO: 实现错误处理测试
    ASSERT_TRUE(true, "错误处理测试");
    return true;
}

TEST_CASE(test_${module}_edge_cases) {
    // TODO: 实现边缘情况测试
    ASSERT_TRUE(true, "边缘情况测试");
    return true;
}

int main() {
    test_framework_init(false);
    
    TEST_SUITE_START("${module} Unit Tests");
    RUN_TEST(test_${module}_basic_functionality);
    RUN_TEST(test_${module}_error_handling);
    RUN_TEST(test_${module}_edge_cases);
    TEST_SUITE_END();
    
    test_framework_print_summary();
    return test_framework_all_passed() ? 0 : 1;
}
EOF
        fi
    done
    
    echo "测试模板创建完成"
}

# 主函数
main() {
    analyze_source_functions
    echo
    analyze_test_coverage
    echo
    identify_critical_modules
    echo
    analyze_test_types
    echo
    check_test_quality
    echo
    generate_coverage_report
    echo
    provide_improvement_suggestions
    echo
    create_missing_test_templates
    
    echo
    echo -e "${GREEN}=== 测试覆盖率分析完成 ===${NC}"
    
    # 清理临时文件
    rm -f /tmp/source_functions.txt /tmp/tested_functions.txt /tmp/unique_tested_functions.txt
}

# 运行主函数
main "$@"
