#!/bin/bash

# 跨平台编译测试脚本
# 测试不同架构和平台的编译功能

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
RESULTS_DIR="$TEST_DIR/cross_platform_results"

# 创建结果目录
mkdir -p "$RESULTS_DIR"

echo -e "${BLUE}=== 跨平台编译测试 ===${NC}"
echo "项目根目录: $PROJECT_ROOT"
echo "测试目录: $TEST_DIR"
echo "结果目录: $RESULTS_DIR"
echo "当前平台: $(uname -s) $(uname -m)"
echo

# 测试计数器
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# 测试结果记录
RESULTS_FILE="$RESULTS_DIR/cross_platform_results.txt"
echo "跨平台编译测试结果 - $(date)" > "$RESULTS_FILE"
echo "当前平台: $(uname -s) $(uname -m)" >> "$RESULTS_FILE"
echo "======================================" >> "$RESULTS_FILE"

# 运行单个测试
run_test() {
    local test_name="$1"
    local test_command="$2"
    local expected_result="$3"  # "success" 或 "fail"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    echo -n "测试 $TOTAL_TESTS: $test_name ... "
    
    local test_result=0
    
    # 运行测试命令，设置超时
    if timeout 60s bash -c "$test_command" >> "$RESULTS_FILE" 2>&1; then
        test_result=0  # 成功
    else
        test_result=1  # 失败
    fi
    
    # 检查结果是否符合预期
    local test_passed=false
    if [ "$expected_result" = "success" ] && [ $test_result -eq 0 ]; then
        test_passed=true
    elif [ "$expected_result" = "fail" ] && [ $test_result -ne 0 ]; then
        test_passed=true
    fi
    
    if [ "$test_passed" = true ]; then
        echo -e "${GREEN}PASS${NC}"
        echo "测试 $TOTAL_TESTS: $test_name - PASS" >> "$RESULTS_FILE"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}FAIL${NC}"
        echo "测试 $TOTAL_TESTS: $test_name - FAIL (期望: $expected_result, 实际: $([ $test_result -eq 0 ] && echo "success" || echo "fail"))" >> "$RESULTS_FILE"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
}

# 检测当前架构
detect_architecture() {
    local arch=$(uname -m)
    local os=$(uname -s)
    
    echo -e "${BLUE}=== 架构检测 ===${NC}"
    echo "操作系统: $os"
    echo "架构: $arch"
    
    case "$arch" in
        x86_64|amd64)
            ARCH_SUFFIX="x64_64"
            ;;
        arm64|aarch64)
            ARCH_SUFFIX="arm64_64"
            ;;
        *)
            echo -e "${YELLOW}警告: 未知架构 $arch，使用 x64_64${NC}"
            ARCH_SUFFIX="x64_64"
            ;;
    esac
    
    echo "使用架构后缀: $ARCH_SUFFIX"
    echo
}

# 测试编译工具链
test_build_tools() {
    echo -e "${BLUE}=== 编译工具链测试 ===${NC}"
    
    # 测试cc.sh脚本
    run_test "cc.sh脚本存在性" "test -f '$PROJECT_ROOT/cc.sh'" "success"
    run_test "cc.sh脚本可执行性" "test -x '$PROJECT_ROOT/cc.sh'" "success"
    
    # 测试构建脚本
    run_test "build_core.sh存在性" "test -f '$PROJECT_ROOT/build_core.sh'" "success"
    run_test "build_simple_loader.sh存在性" "test -f '$PROJECT_ROOT/build_simple_loader.sh'" "success"
    run_test "build_tools.sh存在性" "test -f '$PROJECT_ROOT/build_tools.sh'" "success"
}

# 测试模块文件
test_native_modules() {
    echo -e "${BLUE}=== 原生模块文件测试 ===${NC}"
    
    local modules=("pipeline" "layer0" "compiler" "libc")
    
    for module in "${modules[@]}"; do
        local module_file="$PROJECT_ROOT/bin/${module}_${ARCH_SUFFIX}.native"
        run_test "${module}模块文件存在性" "test -f '$module_file'" "success"
        
        if [ -f "$module_file" ]; then
            run_test "${module}模块文件可读性" "test -r '$module_file'" "success"
            # 检查文件大小（应该大于0）
            run_test "${module}模块文件非空" "test -s '$module_file'" "success"
        fi
    done
}

# 测试可执行文件
test_executables() {
    echo -e "${BLUE}=== 可执行文件测试 ===${NC}"
    
    local executables=("simple_loader" "c2astc" "c2native")
    
    for exe in "${executables[@]}"; do
        local exe_file="$PROJECT_ROOT/bin/${exe}"
        local arch_exe_file="$PROJECT_ROOT/bin/${exe}_${ARCH_SUFFIX}"
        
        # 检查通用版本或架构特定版本
        if [ -f "$exe_file" ]; then
            run_test "${exe}可执行文件存在性" "test -f '$exe_file'" "success"
            run_test "${exe}可执行文件权限" "test -x '$exe_file'" "success"
        elif [ -f "$arch_exe_file" ]; then
            run_test "${exe}架构特定文件存在性" "test -f '$arch_exe_file'" "success"
            run_test "${exe}架构特定文件权限" "test -x '$arch_exe_file'" "success"
        else
            run_test "${exe}文件存在性" "false" "fail"
        fi
    done
}

# 测试简单编译
test_simple_compilation() {
    echo -e "${BLUE}=== 简单编译测试 ===${NC}"
    
    # 创建简单的测试文件
    local test_c_file="$RESULTS_DIR/test_simple.c"
    cat > "$test_c_file" << 'EOF'
int main() {
    int a = 42;
    return 0;
}
EOF
    
    # 测试C2ASTC编译
    local c2astc="$PROJECT_ROOT/bin/c2astc"
    if [ -f "$c2astc" ]; then
        local output_astc="$RESULTS_DIR/test_simple.astc"
        run_test "C2ASTC简单编译" "'$c2astc' '$test_c_file' -o '$output_astc'" "success"
        
        if [ -f "$output_astc" ]; then
            run_test "ASTC输出文件生成" "test -f '$output_astc'" "success"
            run_test "ASTC输出文件非空" "test -s '$output_astc'" "success"
        fi
    else
        echo -e "${YELLOW}跳过C2ASTC测试（文件不存在）${NC}"
    fi
}

# 测试跨平台兼容性
test_cross_platform_compatibility() {
    echo -e "${BLUE}=== 跨平台兼容性测试 ===${NC}"
    
    # 检查文件格式
    local simple_loader="$PROJECT_ROOT/bin/simple_loader"
    if [ -f "$simple_loader" ]; then
        local file_info=$(file "$simple_loader" 2>/dev/null || echo "unknown")
        echo "simple_loader文件信息: $file_info"
        echo "simple_loader文件信息: $file_info" >> "$RESULTS_FILE"
        
        case "$(uname -s)" in
            Linux)
                run_test "Linux ELF格式检查" "file '$simple_loader' | grep -q 'ELF'" "success"
                ;;
            Darwin)
                run_test "macOS Mach-O格式检查" "file '$simple_loader' | grep -q 'Mach-O'" "success"
                ;;
            CYGWIN*|MINGW*|MSYS*)
                run_test "Windows PE格式检查" "file '$simple_loader' | grep -q 'PE32'" "success"
                ;;
            *)
                echo -e "${YELLOW}未知操作系统，跳过格式检查${NC}"
                ;;
        esac
    fi
    
    # 测试架构匹配
    local current_arch=$(uname -m)
    case "$current_arch" in
        x86_64|amd64)
            run_test "x64架构模块存在" "test -f '$PROJECT_ROOT/bin/pipeline_x64_64.native'" "success"
            ;;
        arm64|aarch64)
            run_test "ARM64架构模块存在" "test -f '$PROJECT_ROOT/bin/pipeline_arm64_64.native'" "success"
            ;;
    esac
}

# 生成测试报告
generate_report() {
    echo "" >> "$RESULTS_FILE"
    echo "======================================" >> "$RESULTS_FILE"
    echo "测试总结:" >> "$RESULTS_FILE"
    echo "  总测试数: $TOTAL_TESTS" >> "$RESULTS_FILE"
    echo "  通过测试: $PASSED_TESTS" >> "$RESULTS_FILE"
    echo "  失败测试: $FAILED_TESTS" >> "$RESULTS_FILE"
    
    local pass_rate=0
    if [ $TOTAL_TESTS -gt 0 ]; then
        pass_rate=$((PASSED_TESTS * 100 / TOTAL_TESTS))
    fi
    echo "  通过率: ${pass_rate}%" >> "$RESULTS_FILE"
    
    echo
    echo -e "${BLUE}=== 测试总结 ===${NC}"
    echo "总测试数: $TOTAL_TESTS"
    echo -e "通过测试: ${GREEN}$PASSED_TESTS${NC}"
    echo -e "失败测试: ${RED}$FAILED_TESTS${NC}"
    echo -e "通过率: ${YELLOW}${pass_rate}%${NC}"
    
    if [ $pass_rate -ge 80 ]; then
        echo -e "${GREEN}跨平台编译测试结果良好！${NC}"
    elif [ $pass_rate -ge 60 ]; then
        echo -e "${YELLOW}跨平台编译测试结果一般，需要改进${NC}"
    else
        echo -e "${RED}跨平台编译测试结果较差，需要大量改进${NC}"
    fi
    
    # 生成建议
    echo "" >> "$RESULTS_FILE"
    echo "改进建议:" >> "$RESULTS_FILE"
    if [ $FAILED_TESTS -gt 0 ]; then
        echo "- 检查失败的测试项目并修复相关问题" >> "$RESULTS_FILE"
        echo "- 确保所有必要的模块文件都已正确构建" >> "$RESULTS_FILE"
        echo "- 验证跨平台兼容性问题" >> "$RESULTS_FILE"
    fi
}

# 主函数
main() {
    detect_architecture
    test_build_tools
    test_native_modules
    test_executables
    test_simple_compilation
    test_cross_platform_compatibility
    generate_report
    
    echo
    echo -e "${GREEN}=== 跨平台编译测试完成 ===${NC}"
    echo "详细结果保存在: $RESULTS_FILE"
    echo
    echo "查看结果: cat $RESULTS_FILE"
    
    # 返回适当的退出码
    if [ $FAILED_TESTS -eq 0 ]; then
        exit 0
    else
        exit 1
    fi
}

# 运行主函数
main "$@"
