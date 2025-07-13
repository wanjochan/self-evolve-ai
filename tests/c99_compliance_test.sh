#!/bin/bash

# C99标准兼容性测试脚本
# 测试编译器对C99标准的支持程度

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
C99_COMPILER="$PROJECT_ROOT/bin/c99_compiler"
RESULTS_DIR="$TEST_DIR/c99_compliance_results"

# 创建结果目录
mkdir -p "$RESULTS_DIR"

echo -e "${BLUE}=== C99标准兼容性测试 ===${NC}"
echo "项目根目录: $PROJECT_ROOT"
echo "测试目录: $TEST_DIR"
echo "结果目录: $RESULTS_DIR"
echo

# 检查编译器是否存在
if [ ! -f "$C99_COMPILER" ]; then
    echo -e "${RED}错误: C99编译器不存在: $C99_COMPILER${NC}"
    echo "请先运行 bash build_c99.sh 构建编译器"
    exit 1
fi

# 测试计数器
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# 测试结果记录
RESULTS_FILE="$RESULTS_DIR/compliance_results.txt"
echo "C99标准兼容性测试结果 - $(date)" > "$RESULTS_FILE"
echo "======================================" >> "$RESULTS_FILE"

# 运行单个测试
run_test() {
    local test_name="$1"
    local test_file="$2"
    local expected_result="$3"  # "pass" 或 "fail"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    echo -n "测试 $TOTAL_TESTS: $test_name ... "
    
    local output_file="$RESULTS_DIR/test_$TOTAL_TESTS.astc"
    local compile_result=0
    
    # 运行编译器
    if timeout 45s "$C99_COMPILER" "$test_file" -o "$output_file" 2>/dev/null; then
        compile_result=0  # 编译成功
    else
        compile_result=1  # 编译失败
    fi
    
    # 检查结果是否符合预期
    local test_passed=false
    if [ "$expected_result" = "pass" ] && [ $compile_result -eq 0 ]; then
        test_passed=true
    elif [ "$expected_result" = "fail" ] && [ $compile_result -ne 0 ]; then
        test_passed=true
    fi
    
    if [ "$test_passed" = true ]; then
        echo -e "${GREEN}PASS${NC}"
        echo "测试 $TOTAL_TESTS: $test_name - PASS" >> "$RESULTS_FILE"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}FAIL${NC}"
        echo "测试 $TOTAL_TESTS: $test_name - FAIL (期望: $expected_result, 实际: $([ $compile_result -eq 0 ] && echo "pass" || echo "fail"))" >> "$RESULTS_FILE"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    
    # 清理输出文件
    rm -f "$output_file"
}

# 创建测试文件并运行测试
create_and_run_tests() {
    echo -e "${BLUE}创建并运行C99兼容性测试...${NC}"
    
    # 测试1: 基本数据类型
    cat > "$RESULTS_DIR/test_basic_types.c" << 'EOF'
int main() {
    char c = 'A';
    short s = 100;
    int i = 1000;
    long l = 100000L;
    long long ll = 1000000LL;
    float f = 3.14f;
    double d = 3.14159;
    long double ld = 3.14159L;
    _Bool b = 1;
    return 0;
}
EOF
    run_test "基本数据类型" "$RESULTS_DIR/test_basic_types.c" "pass"
    
    # 测试2: 复合数据类型
    cat > "$RESULTS_DIR/test_compound_types.c" << 'EOF'
struct Point {
    int x, y;
};

union Data {
    int i;
    float f;
    char c[4];
};

enum Color {
    RED, GREEN, BLUE
};

int main() {
    struct Point p = {10, 20};
    union Data d;
    enum Color c = RED;
    int arr[10];
    int *ptr = &arr[0];
    return 0;
}
EOF
    run_test "复合数据类型" "$RESULTS_DIR/test_compound_types.c" "pass"
    
    # 测试3: 函数声明和定义
    cat > "$RESULTS_DIR/test_functions.c" << 'EOF'
int add(int a, int b);
void print_hello(void);
int variadic_func(int count, ...);

int add(int a, int b) {
    return a + b;
}

void print_hello(void) {
    // printf("Hello\n");
}

int main() {
    int result = add(5, 3);
    print_hello();
    return 0;
}
EOF
    run_test "函数声明和定义" "$RESULTS_DIR/test_functions.c" "pass"
    
    # 测试4: 控制流语句
    cat > "$RESULTS_DIR/test_control_flow.c" << 'EOF'
int main() {
    int i, sum = 0;
    
    // if-else
    if (sum == 0) {
        sum = 1;
    } else {
        sum = 2;
    }
    
    // for循环
    for (i = 0; i < 10; i++) {
        sum += i;
    }
    
    // while循环
    while (sum > 100) {
        sum /= 2;
    }
    
    // switch语句
    switch (sum % 3) {
        case 0:
            sum += 1;
            break;
        case 1:
            sum += 2;
            break;
        default:
            sum += 3;
            break;
    }
    
    return 0;
}
EOF
    run_test "控制流语句" "$RESULTS_DIR/test_control_flow.c" "pass"
    
    # 测试5: 指针和数组
    cat > "$RESULTS_DIR/test_pointers_arrays.c" << 'EOF'
int main() {
    int arr[5] = {1, 2, 3, 4, 5};
    int *ptr = arr;
    int **ptr_ptr = &ptr;
    
    // 指针运算
    ptr++;
    ptr--;
    
    // 数组访问
    int value = arr[2];
    value = *(ptr + 2);
    
    // 多维数组
    int matrix[3][3];
    matrix[1][1] = 42;
    
    return 0;
}
EOF
    run_test "指针和数组" "$RESULTS_DIR/test_pointers_arrays.c" "pass"
    
    # 测试6: 类型转换
    cat > "$RESULTS_DIR/test_type_casting.c" << 'EOF'
int main() {
    int i = 42;
    float f = 3.14f;
    char c = 'A';
    void *ptr = &i;
    
    // 隐式转换
    f = i;
    i = f;
    
    // 显式转换
    i = (int)f;
    f = (float)i;
    c = (char)i;
    
    // 指针转换
    int *int_ptr = (int*)ptr;
    
    return 0;
}
EOF
    run_test "类型转换" "$RESULTS_DIR/test_type_casting.c" "pass"
    
    # 测试7: 预处理器（简化测试）
    cat > "$RESULTS_DIR/test_preprocessor.c" << 'EOF'
#define MAX_SIZE 100
#define SQUARE(x) ((x) * (x))

int main() {
    int size = MAX_SIZE;
    int area = SQUARE(5);
    return 0;
}
EOF
    run_test "预处理器" "$RESULTS_DIR/test_preprocessor.c" "pass"
    
    # 测试8: 错误检测 - 语法错误
    cat > "$RESULTS_DIR/test_syntax_error.c" << 'EOF'
int main() {
    int a = 5
    return 0;  // 缺少分号
}
EOF
    run_test "语法错误检测" "$RESULTS_DIR/test_syntax_error.c" "fail"
    
    # 测试9: 错误检测 - 类型错误
    cat > "$RESULTS_DIR/test_type_error.c" << 'EOF'
int main() {
    int arr[5];
    int value = arr + "string";  // 类型不匹配
    return 0;
}
EOF
    run_test "类型错误检测" "$RESULTS_DIR/test_type_error.c" "fail"
    
    # 测试10: 错误检测 - 未声明变量
    cat > "$RESULTS_DIR/test_undeclared_var.c" << 'EOF'
int main() {
    int a = 5;
    int b = undefined_variable;  // 未声明的变量
    return 0;
}
EOF
    run_test "未声明变量检测" "$RESULTS_DIR/test_undeclared_var.c" "fail"
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
        echo -e "${GREEN}C99兼容性测试结果良好！${NC}"
    elif [ $pass_rate -ge 60 ]; then
        echo -e "${YELLOW}C99兼容性测试结果一般，需要改进${NC}"
    else
        echo -e "${RED}C99兼容性测试结果较差，需要大量改进${NC}"
    fi
}

# 主函数
main() {
    create_and_run_tests
    generate_report
    
    echo
    echo -e "${GREEN}=== C99兼容性测试完成 ===${NC}"
    echo "详细结果保存在: $RESULTS_FILE"
    echo
    echo "查看结果: cat $RESULTS_FILE"
}

# 运行主函数
main "$@"
