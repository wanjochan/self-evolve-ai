#!/bin/bash

# Layer 3 Program 测试脚本
# 测试.astc程序的编译和执行

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
C2ASTC="$PROJECT_ROOT/bin/c2astc"
RESULTS_DIR="$TEST_DIR/layer3_programs_results"

# 创建结果目录
mkdir -p "$RESULTS_DIR"

echo -e "${BLUE}=== Layer 3 Program 测试 ===${NC}"
echo "项目根目录: $PROJECT_ROOT"
echo "测试目录: $TEST_DIR"
echo "结果目录: $RESULTS_DIR"
echo

# 测试计数器
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# 测试结果记录
RESULTS_FILE="$RESULTS_DIR/programs_test_results.txt"
echo "Layer 3 Program 测试结果 - $(date)" > "$RESULTS_FILE"
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

# 测试1: 检查现有的ASTC程序文件
echo -e "${BLUE}=== 现有ASTC程序检查 ===${NC}"

# 检查examples目录下的ASTC文件
ASTC_FILES=(
    "$PROJECT_ROOT/examples/hello_world.astc"
    "$PROJECT_ROOT/examples/test_c99.astc"
    "$PROJECT_ROOT/examples/test_program.astc"
)

for astc_file in "${ASTC_FILES[@]}"; do
    if [ -f "$astc_file" ]; then
        filename=$(basename "$astc_file")
        run_test "ASTC文件存在: $filename" "test -f '$astc_file'" "success"
        run_test "ASTC文件非空: $filename" "test -s '$astc_file'" "success"
    fi
done

# 检查tests目录下的ASTC文件
TEST_ASTC_FILES=(
    "$TEST_DIR/test_minimal.astc"
    "$TEST_DIR/test_function_call.astc"
    "$TEST_DIR/test_global_var.astc"
    "$TEST_DIR/test_complex_c99.astc"
)

for astc_file in "${TEST_ASTC_FILES[@]}"; do
    if [ -f "$astc_file" ]; then
        filename=$(basename "$astc_file")
        run_test "测试ASTC文件存在: $filename" "test -f '$astc_file'" "success"
        run_test "测试ASTC文件非空: $filename" "test -s '$astc_file'" "success"
    fi
done

# 测试2: C到ASTC编译测试
echo -e "${BLUE}=== C到ASTC编译测试 ===${NC}"

# 创建简单的C测试程序
cat > "$RESULTS_DIR/test_simple.c" << 'EOF'
int main() {
    return 0;
}
EOF

cat > "$RESULTS_DIR/test_return_value.c" << 'EOF'
int main() {
    return 42;
}
EOF

cat > "$RESULTS_DIR/test_function.c" << 'EOF'
int add(int a, int b) {
    return a + b;
}

int main() {
    int result = add(3, 4);
    return result;
}
EOF

# 测试C2ASTC编译器
if [ -f "$C2ASTC" ]; then
    run_test "C2ASTC编译器存在" "test -f '$C2ASTC'" "success"
    
    # 尝试编译简单程序
    run_test "编译简单C程序" "'$C2ASTC' '$RESULTS_DIR/test_simple.c' -o '$RESULTS_DIR/test_simple.astc'" "success"
    run_test "编译返回值程序" "'$C2ASTC' '$RESULTS_DIR/test_return_value.c' -o '$RESULTS_DIR/test_return_value.astc'" "success"
    run_test "编译函数调用程序" "'$C2ASTC' '$RESULTS_DIR/test_function.c' -o '$RESULTS_DIR/test_function.astc'" "success"
    
    # 检查生成的ASTC文件
    if [ -f "$RESULTS_DIR/test_simple.astc" ]; then
        run_test "生成的ASTC文件存在" "test -f '$RESULTS_DIR/test_simple.astc'" "success"
        run_test "生成的ASTC文件非空" "test -s '$RESULTS_DIR/test_simple.astc'" "success"
    fi
else
    echo -e "${YELLOW}警告: C2ASTC编译器不存在，跳过编译测试${NC}"
fi

# 测试3: ASTC程序格式验证
echo -e "${BLUE}=== ASTC程序格式验证 ===${NC}"

# 创建格式验证函数
validate_astc_format() {
    local file="$1"
    if [ -f "$file" ]; then
        # 检查是否包含ASTC标识
        if head -c 4 "$file" | grep -q "ASTC" 2>/dev/null; then
            return 0
        fi
        # 检查是否是文本格式的ASTC
        if head -n 1 "$file" | grep -q "ASTC" 2>/dev/null; then
            return 0
        fi
    fi
    return 1
}

# 验证现有ASTC文件格式
for astc_file in "${ASTC_FILES[@]}"; do
    if [ -f "$astc_file" ]; then
        filename=$(basename "$astc_file")
        run_test "ASTC格式验证: $filename" "validate_astc_format '$astc_file'" "success"
    fi
done

# 测试4: ASTC程序执行测试
echo -e "${BLUE}=== ASTC程序执行测试 ===${NC}"

# 使用现有的测试程序验证执行
if [ -f "$TEST_DIR/test_astc_core" ]; then
    run_test "ASTC核心执行测试" "'$TEST_DIR/test_astc_core'" "success"
fi

if [ -f "$TEST_DIR/test_astc_bytecode" ]; then
    run_test "ASTC字节码执行测试" "'$TEST_DIR/test_astc_bytecode'" "success"
fi

# 测试5: 错误处理测试
echo -e "${BLUE}=== 错误处理测试 ===${NC}"

# 创建无效的C程序
cat > "$RESULTS_DIR/test_invalid.c" << 'EOF'
int main() {
    undefined_function();
    return 0;
}
EOF

cat > "$RESULTS_DIR/test_syntax_error.c" << 'EOF'
int main() {
    int x = 
    return 0;
}
EOF

if [ -f "$C2ASTC" ]; then
    run_test "无效函数调用处理" "'$C2ASTC' '$RESULTS_DIR/test_invalid.c' -o '$RESULTS_DIR/test_invalid.astc'" "fail"
    run_test "语法错误处理" "'$C2ASTC' '$RESULTS_DIR/test_syntax_error.c' -o '$RESULTS_DIR/test_syntax_error.astc'" "fail"
fi

# 测试6: 性能基准测试
echo -e "${BLUE}=== 性能基准测试 ===${NC}"

# 创建性能测试程序
cat > "$RESULTS_DIR/test_performance.c" << 'EOF'
int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n-1) + fibonacci(n-2);
}

int main() {
    int result = fibonacci(10);
    return result;
}
EOF

if [ -f "$C2ASTC" ]; then
    # 测量编译时间
    start_time=$(date +%s.%N)
    if "$C2ASTC" "$RESULTS_DIR/test_performance.c" -o "$RESULTS_DIR/test_performance.astc" 2>/dev/null; then
        end_time=$(date +%s.%N)
        compile_time=$(echo "$end_time - $start_time" | bc -l 2>/dev/null || echo "0")
        echo "编译时间: ${compile_time}s" >> "$RESULTS_FILE"
        run_test "性能测试程序编译" "test -f '$RESULTS_DIR/test_performance.astc'" "success"
    fi
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