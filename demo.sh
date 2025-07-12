#!/bin/bash

# C99编译器演示脚本
# 展示编译器的主要功能和特性

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# 脚本配置
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEMO_DIR="$SCRIPT_DIR/demo_files"
C99_COMPILER="$SCRIPT_DIR/bin/c99_compiler"
C99_WRAPPER="$SCRIPT_DIR/c99.sh"

# 创建演示目录
mkdir -p "$DEMO_DIR"

# 打印标题
print_title() {
    echo
    echo -e "${CYAN}================================${NC}"
    echo -e "${CYAN}$1${NC}"
    echo -e "${CYAN}================================${NC}"
    echo
}

# 打印步骤
print_step() {
    echo -e "${BLUE}[步骤 $1]${NC} $2"
}

# 打印成功信息
print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

# 打印警告信息
print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

# 打印错误信息
print_error() {
    echo -e "${RED}❌ $1${NC}"
}

# 等待用户输入
wait_for_user() {
    echo -e "${PURPLE}按 Enter 键继续...${NC}"
    read -r
}

# 检查先决条件
check_prerequisites() {
    print_title "检查先决条件"
    
    print_step "1" "检查C99编译器是否存在"
    if [ -f "$C99_COMPILER" ]; then
        print_success "C99编译器已找到: $C99_COMPILER"
    else
        print_error "C99编译器不存在，请先运行 bash build_c99.sh"
        exit 1
    fi
    
    print_step "2" "检查包装脚本是否存在"
    if [ -f "$C99_WRAPPER" ]; then
        print_success "包装脚本已找到: $C99_WRAPPER"
    else
        print_error "包装脚本不存在: $C99_WRAPPER"
        exit 1
    fi
    
    print_step "3" "检查脚本权限"
    if [ -x "$C99_WRAPPER" ]; then
        print_success "包装脚本具有执行权限"
    else
        print_warning "设置包装脚本执行权限"
        chmod +x "$C99_WRAPPER"
    fi
    
    wait_for_user
}

# 演示基本编译功能
demo_basic_compilation() {
    print_title "演示1: 基本编译功能"
    
    print_step "1" "创建简单的Hello World程序"
    cat > "$DEMO_DIR/hello.c" << 'EOF'
#include <stdio.h>

int main() {
    printf("Hello, C99 Compiler!\n");
    return 0;
}
EOF
    
    echo "创建的文件内容:"
    echo -e "${YELLOW}"
    cat "$DEMO_DIR/hello.c"
    echo -e "${NC}"
    
    print_step "2" "使用C99编译器编译"
    echo "命令: $C99_WRAPPER --c99-verbose $DEMO_DIR/hello.c -o $DEMO_DIR/hello"
    if "$C99_WRAPPER" --c99-verbose "$DEMO_DIR/hello.c" -o "$DEMO_DIR/hello" 2>&1; then
        print_success "编译成功！"
    else
        print_warning "C99编译失败，可能回退到TinyCC"
    fi
    
    wait_for_user
}

# 演示语义检查功能
demo_semantic_checking() {
    print_title "演示2: 语义检查功能"
    
    print_step "1" "创建包含语义错误的程序"
    cat > "$DEMO_DIR/semantic_error.c" << 'EOF'
#include <stdio.h>

int main() {
    int a = 5;
    int b = undefined_variable;  // 未声明的变量
    float result = a / 0;        // 除零错误
    return 0;
}
EOF
    
    echo "创建的文件内容:"
    echo -e "${YELLOW}"
    cat "$DEMO_DIR/semantic_error.c"
    echo -e "${NC}"
    
    print_step "2" "编译并观察错误检测"
    echo "命令: $C99_WRAPPER --c99-verbose $DEMO_DIR/semantic_error.c -o $DEMO_DIR/semantic_error"
    if "$C99_WRAPPER" --c99-verbose "$DEMO_DIR/semantic_error.c" -o "$DEMO_DIR/semantic_error" 2>&1; then
        print_warning "编译成功（可能使用了回退编译器）"
    else
        print_success "成功检测到语义错误！"
    fi
    
    wait_for_user
}

# 演示复杂程序编译
demo_complex_program() {
    print_title "演示3: 复杂程序编译"
    
    print_step "1" "创建复杂的C程序"
    cat > "$DEMO_DIR/complex.c" << 'EOF'
#include <stdio.h>

struct Point {
    int x, y;
};

struct Rectangle {
    struct Point top_left;
    struct Point bottom_right;
};

int calculate_area(struct Rectangle rect) {
    int width = rect.bottom_right.x - rect.top_left.x;
    int height = rect.bottom_right.y - rect.top_left.y;
    return width * height;
}

void print_rectangle(struct Rectangle rect) {
    printf("Rectangle: (%d,%d) to (%d,%d)\n",
           rect.top_left.x, rect.top_left.y,
           rect.bottom_right.x, rect.bottom_right.y);
    printf("Area: %d\n", calculate_area(rect));
}

int main() {
    struct Rectangle rect = {{0, 0}, {10, 5}};
    
    print_rectangle(rect);
    
    // 控制流测试
    for (int i = 0; i < 3; i++) {
        printf("Iteration %d\n", i);
    }
    
    // 条件语句测试
    int area = calculate_area(rect);
    if (area > 40) {
        printf("Large rectangle\n");
    } else {
        printf("Small rectangle\n");
    }
    
    return 0;
}
EOF
    
    echo "创建的文件内容:"
    echo -e "${YELLOW}"
    head -20 "$DEMO_DIR/complex.c"
    echo "... (文件较长，只显示前20行)"
    echo -e "${NC}"
    
    print_step "2" "编译复杂程序"
    echo "命令: $C99_WRAPPER --c99-verbose $DEMO_DIR/complex.c -o $DEMO_DIR/complex"
    if "$C99_WRAPPER" --c99-verbose "$DEMO_DIR/complex.c" -o "$DEMO_DIR/complex" 2>&1; then
        print_success "复杂程序编译成功！"
    else
        print_warning "编译失败或使用了回退编译器"
    fi
    
    wait_for_user
}

# 演示性能测试
demo_performance_test() {
    print_title "演示4: 性能测试"
    
    print_step "1" "运行性能测试模式"
    echo "命令: $C99_WRAPPER --c99-performance-test $DEMO_DIR/hello.c -o $DEMO_DIR/hello_perf"
    "$C99_WRAPPER" --c99-performance-test "$DEMO_DIR/hello.c" -o "$DEMO_DIR/hello_perf" 2>&1 || true
    
    print_step "2" "启用统计收集"
    echo "命令: $C99_WRAPPER --c99-statistics --c99-log $DEMO_DIR/complex.c -o $DEMO_DIR/complex_stats"
    "$C99_WRAPPER" --c99-statistics --c99-log "$DEMO_DIR/complex.c" -o "$DEMO_DIR/complex_stats" 2>&1 || true
    
    print_step "3" "查看统计信息"
    echo "命令: $C99_WRAPPER --c99-show-stats"
    "$C99_WRAPPER" --c99-show-stats 2>&1 || print_warning "统计文件可能不存在"
    
    wait_for_user
}

# 演示测试套件
demo_test_suite() {
    print_title "演示5: 测试套件"
    
    print_step "1" "运行C99兼容性测试"
    if [ -x "$SCRIPT_DIR/tests/c99_compliance_test.sh" ]; then
        echo "命令: $SCRIPT_DIR/tests/c99_compliance_test.sh"
        "$SCRIPT_DIR/tests/c99_compliance_test.sh" 2>&1 || print_warning "兼容性测试可能失败"
    else
        print_warning "兼容性测试脚本不存在或不可执行"
    fi
    
    print_step "2" "运行代码质量分析"
    if [ -x "$SCRIPT_DIR/tests/code_quality_analysis.sh" ]; then
        echo "命令: $SCRIPT_DIR/tests/code_quality_analysis.sh"
        "$SCRIPT_DIR/tests/code_quality_analysis.sh" 2>&1 || print_warning "代码质量分析可能失败"
    else
        print_warning "代码质量分析脚本不存在或不可执行"
    fi
    
    wait_for_user
}

# 演示部署功能
demo_deployment() {
    print_title "演示6: 部署功能"
    
    print_step "1" "模拟部署过程"
    if [ -x "$SCRIPT_DIR/scripts/deploy_c99_replacement.sh" ]; then
        echo "命令: $SCRIPT_DIR/scripts/deploy_c99_replacement.sh --dry-run"
        "$SCRIPT_DIR/scripts/deploy_c99_replacement.sh" --dry-run 2>&1 || print_warning "部署脚本可能失败"
    else
        print_warning "部署脚本不存在或不可执行"
    fi
    
    wait_for_user
}

# 清理演示文件
cleanup_demo() {
    print_title "清理演示文件"
    
    print_step "1" "删除演示文件"
    if [ -d "$DEMO_DIR" ]; then
        rm -rf "$DEMO_DIR"
        print_success "演示文件已清理"
    else
        print_warning "演示目录不存在"
    fi
}

# 显示总结
show_summary() {
    print_title "演示总结"
    
    echo -e "${GREEN}🎉 C99编译器演示完成！${NC}"
    echo
    echo "演示内容包括:"
    echo "✅ 基本编译功能"
    echo "✅ 语义错误检测"
    echo "✅ 复杂程序编译"
    echo "✅ 性能测试功能"
    echo "✅ 测试套件运行"
    echo "✅ 部署功能展示"
    echo
    echo "更多信息请查看:"
    echo "📖 C99_COMPILER_README.md - 使用指南"
    echo "📊 docs/project_completion_report.md - 项目报告"
    echo "📋 docs/project_deliverables.md - 交付物清单"
    echo
    echo -e "${CYAN}感谢使用C99编译器！${NC}"
}

# 主函数
main() {
    echo -e "${PURPLE}"
    echo "  ____  ___   ___     ____                      _ _           "
    echo " / ___|/ _ \ / _ \   / ___|___  _ __ ___  _ __ (_) | ___ _ __ "
    echo "| |   | (_) | (_) | | |   / _ \| '_ \` _ \| '_ \| | |/ _ \ '__|"
    echo "| |___ \__, |\__, | | |__| (_) | | | | | | |_) | | |  __/ |   "
    echo " \____|  /_/   /_/   \____\___/|_| |_| |_| .__/|_|_|\___|_|   "
    echo "                                        |_|                  "
    echo -e "${NC}"
    echo -e "${CYAN}欢迎使用C99编译器演示程序！${NC}"
    echo
    
    check_prerequisites
    demo_basic_compilation
    demo_semantic_checking
    demo_complex_program
    demo_performance_test
    demo_test_suite
    demo_deployment
    cleanup_demo
    show_summary
}

# 运行主函数
main "$@"
