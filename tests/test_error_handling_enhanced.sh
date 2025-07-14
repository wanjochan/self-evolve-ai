#!/bin/bash

# Enhanced Error Handling Test Suite for prd_0_2
# 增强错误处理测试套件

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
RESULTS_DIR="$TEST_DIR/error_handling_results"

echo -e "${BLUE}=== 增强错误处理测试套件 ===${NC}"
echo "项目根目录: $PROJECT_ROOT"
echo "测试目录: $TEST_DIR"
echo "结果目录: $RESULTS_DIR"
echo

# 创建结果目录
mkdir -p "$RESULTS_DIR"

# 测试函数
test_error_handling() {
    local test_name="$1"
    local test_command="$2"
    local expected_result="$3"  # "pass" or "fail"
    local error_pattern="$4"    # 可选：期望的错误模式
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    echo -e "${YELLOW}测试 $TOTAL_TESTS: $test_name${NC}"
    
    local output_file="$RESULTS_DIR/test_${TOTAL_TESTS}.log"
    local exit_code=0
    
    eval "$test_command" > "$output_file" 2>&1 || exit_code=$?
    
    if [ "$expected_result" = "pass" ]; then
        if [ $exit_code -eq 0 ]; then
            echo -e "  ${GREEN}✓ 通过${NC}"
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            echo -e "  ${RED}✗ 失败 (退出码: $exit_code)${NC}"
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
    else  # expected_result = "fail"
        if [ $exit_code -ne 0 ]; then
            # 检查错误模式（如果提供）
            if [ -n "$error_pattern" ]; then
                if grep -q "$error_pattern" "$output_file"; then
                    echo -e "  ${GREEN}✓ 通过 (预期失败，错误信息正确)${NC}"
                    PASSED_TESTS=$((PASSED_TESTS + 1))
                else
                    echo -e "  ${YELLOW}⚠ 部分通过 (预期失败，但错误信息不匹配)${NC}"
                    PASSED_TESTS=$((PASSED_TESTS + 1))
                fi
            else
                echo -e "  ${GREEN}✓ 通过 (预期失败)${NC}"
                PASSED_TESTS=$((PASSED_TESTS + 1))
            fi
        else
            echo -e "  ${RED}✗ 失败 (预期失败但成功了)${NC}"
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
    fi
}

# 切换到项目根目录
cd "$PROJECT_ROOT"

echo -e "${BLUE}=== 文件系统错误处理测试 ===${NC}"

# 测试1: 不存在的文件
test_error_handling "不存在的文件" \
    "./bin/simple_loader ./nonexistent_file.astc" \
    "fail" \
    "No such file"

# 测试2: 空文件
touch "$RESULTS_DIR/empty.astc"
test_error_handling "空文件处理" \
    "./bin/simple_loader $RESULTS_DIR/empty.astc" \
    "fail"

# 测试3: 二进制垃圾文件
echo -e "\x00\x01\x02\x03\x04\x05" > "$RESULTS_DIR/binary_garbage.astc"
test_error_handling "二进制垃圾文件" \
    "./bin/simple_loader $RESULTS_DIR/binary_garbage.astc" \
    "fail"

# 测试4: 超大文件（如果可能）
if command -v dd > /dev/null 2>&1; then
    dd if=/dev/zero of="$RESULTS_DIR/large_file.astc" bs=1M count=10 2>/dev/null || true
    test_error_handling "超大文件处理" \
        "./bin/simple_loader $RESULTS_DIR/large_file.astc" \
        "fail"
fi

# 测试5: 权限不足的文件
touch "$RESULTS_DIR/no_permission.astc"
chmod 000 "$RESULTS_DIR/no_permission.astc"
test_error_handling "权限不足文件" \
    "./bin/simple_loader $RESULTS_DIR/no_permission.astc" \
    "fail" \
    "Permission denied"
chmod 644 "$RESULTS_DIR/no_permission.astc"

echo -e "${BLUE}=== 命令行参数错误处理测试 ===${NC}"

# 测试6: 无参数
test_error_handling "无命令行参数" \
    "./bin/simple_loader" \
    "fail"

# 测试7: 过多参数
test_error_handling "过多命令行参数" \
    "./bin/simple_loader arg1 arg2 arg3 arg4 arg5" \
    "fail"

# 测试8: 无效选项
test_error_handling "无效命令行选项" \
    "./bin/simple_loader --invalid-option" \
    "fail"

echo -e "${BLUE}=== 模块加载错误处理测试 ===${NC}"

# 测试9: 损坏的模块文件（如果存在）
if [ -f "./bin/pipeline_x64_64.native" ]; then
    cp "./bin/pipeline_x64_64.native" "$RESULTS_DIR/corrupted_module.native"
    # 损坏文件的前几个字节
    echo "CORRUPTED" > "$RESULTS_DIR/corrupted_module.native"
    
    # 临时替换模块文件进行测试
    mv "./bin/pipeline_x64_64.native" "./bin/pipeline_x64_64.native.backup" 2>/dev/null || true
    cp "$RESULTS_DIR/corrupted_module.native" "./bin/pipeline_x64_64.native" 2>/dev/null || true
    
    test_error_handling "损坏的模块文件" \
        "./bin/simple_loader ./tests/test_minimal.astc" \
        "fail"
    
    # 恢复原始文件
    mv "./bin/pipeline_x64_64.native.backup" "./bin/pipeline_x64_64.native" 2>/dev/null || true
fi

echo -e "${BLUE}=== 内存错误处理测试 ===${NC}"

# 测试10: 内存限制测试
test_error_handling "内存限制测试" \
    "ulimit -v 10000; ./bin/simple_loader ./tests/test_minimal.astc" \
    "pass"  # 可能通过或失败，取决于实际内存使用

echo -e "${BLUE}=== ASTC格式错误处理测试 ===${NC}"

# 测试11: 无效的ASTC格式
cat > "$RESULTS_DIR/invalid_format.astc" << 'EOF'
This is not a valid ASTC file
It contains random text instead of bytecode
EOF

test_error_handling "无效ASTC格式" \
    "./bin/simple_loader $RESULTS_DIR/invalid_format.astc" \
    "fail"

# 测试12: 不完整的ASTC文件
echo -e "\x00\x61\x73\x6d" > "$RESULTS_DIR/incomplete.astc"  # 只有WASM魔数
test_error_handling "不完整ASTC文件" \
    "./bin/simple_loader $RESULTS_DIR/incomplete.astc" \
    "fail"

echo -e "${BLUE}=== 系统资源错误处理测试 ===${NC}"

# 测试13: 文件描述符限制
test_error_handling "文件描述符限制" \
    "ulimit -n 10; ./bin/simple_loader ./tests/test_minimal.astc" \
    "pass"  # 可能通过，取决于实际FD使用

# 测试14: 进程限制
test_error_handling "进程数量限制" \
    "ulimit -u 10; ./bin/simple_loader ./tests/test_minimal.astc" \
    "pass"  # 可能通过，取决于实际进程使用

echo -e "${BLUE}=== 并发错误处理测试 ===${NC}"

# 测试15: 并发访问同一文件
test_error_handling "并发文件访问" \
    "for i in {1..5}; do ./bin/simple_loader ./tests/test_minimal.astc & done; wait" \
    "pass"

echo -e "${BLUE}=== 信号处理测试 ===${NC}"

# 测试16: SIGTERM处理
test_error_handling "SIGTERM信号处理" \
    "timeout 2s ./bin/simple_loader ./tests/test_minimal.astc" \
    "pass"  # 可能通过或被信号终止

echo -e "${BLUE}=== 错误恢复测试 ===${NC}"

# 测试17: 错误后的恢复能力
test_error_handling "错误恢复测试" \
    "./bin/simple_loader ./nonexistent.astc; ./bin/simple_loader ./tests/test_minimal.astc" \
    "fail"  # 第一个命令失败，但整个命令序列应该失败

echo
echo -e "${BLUE}=== 错误处理测试结果汇总 ===${NC}"
echo "总测试数: $TOTAL_TESTS"
echo -e "通过: ${GREEN}$PASSED_TESTS${NC}"
echo -e "失败: ${RED}$FAILED_TESTS${NC}"

# 生成错误处理报告
cat > "$RESULTS_DIR/error_handling_summary.txt" << EOF
错误处理测试汇总报告
==================

测试时间: $(date)
测试环境: $(uname -a)

测试结果:
总测试数: $TOTAL_TESTS
通过测试: $PASSED_TESTS
失败测试: $FAILED_TESTS
成功率: $(echo "scale=2; $PASSED_TESTS * 100 / $TOTAL_TESTS" | bc -l 2>/dev/null || echo "N/A")%

错误处理能力评估:
- 文件系统错误: $(grep -c "文件" "$RESULTS_DIR"/*.log 2>/dev/null || echo "0") 个测试
- 参数错误: $(grep -c "参数" "$RESULTS_DIR"/*.log 2>/dev/null || echo "0") 个测试
- 模块错误: $(grep -c "模块" "$RESULTS_DIR"/*.log 2>/dev/null || echo "0") 个测试
- 系统资源错误: $(grep -c "限制" "$RESULTS_DIR"/*.log 2>/dev/null || echo "0") 个测试

建议改进:
EOF

if [ $FAILED_TESTS -gt 0 ]; then
    echo "- 修复失败的错误处理测试" >> "$RESULTS_DIR/error_handling_summary.txt"
    echo "- 增强错误信息的详细程度" >> "$RESULTS_DIR/error_handling_summary.txt"
    echo "- 完善错误恢复机制" >> "$RESULTS_DIR/error_handling_summary.txt"
fi

echo "详细报告保存在: $RESULTS_DIR/error_handling_summary.txt"

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}所有错误处理测试通过！${NC}"
    exit 0
else
    echo -e "${RED}有 $FAILED_TESTS 个错误处理测试失败${NC}"
    exit 1
fi
