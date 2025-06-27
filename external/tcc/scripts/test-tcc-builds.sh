#!/bin/bash

# TCC构建结果测试脚本
# 验证生成的TCC可执行文件功能是否正常

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TCC_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$TCC_ROOT/build"
TEST_DIR="$TCC_ROOT/test_programs"

echo "=== TCC构建结果测试 ==="

# 创建测试程序目录
mkdir -p "$TEST_DIR"

# 创建简单的C测试程序
create_test_programs() {
    echo "创建测试程序..."
    
    # hello.c - 基本测试
    cat > "$TEST_DIR/hello.c" << 'EOF'
#include <stdio.h>

int main() {
    printf("Hello from TCC!\n");
    return 0;
}
EOF

    # math.c - 数学库测试
    cat > "$TEST_DIR/math.c" << 'EOF'
#include <stdio.h>
#include <math.h>

int main() {
    double x = 2.0;
    printf("sqrt(%.1f) = %.6f\n", x, sqrt(x));
    printf("TCC math test passed!\n");
    return 0;
}
EOF

    # struct.c - 结构体测试
    cat > "$TEST_DIR/struct.c" << 'EOF'
#include <stdio.h>

struct Point {
    int x, y;
};

int main() {
    struct Point p = {10, 20};
    printf("Point: (%d, %d)\n", p.x, p.y);
    printf("TCC struct test passed!\n");
    return 0;
}
EOF
}

# 测试单个TCC可执行文件
test_tcc_executable() {
    local tcc_path=$1
    local tcc_name=$(basename "$tcc_path")
    
    echo ""
    echo "=== 测试 $tcc_name ==="
    
    if [ ! -x "$tcc_path" ]; then
        echo "✗ $tcc_name: 文件不存在或不可执行"
        return 1
    fi
    
    # 检查TCC版本
    if "$tcc_path" -v 2>/dev/null; then
        echo "✓ $tcc_name: 版本信息正常"
    else
        echo "✗ $tcc_name: 无法获取版本信息"
        return 1
    fi
    
    # 测试编译能力
    local test_output="$TEST_DIR/${tcc_name}_hello"
    
    if "$tcc_path" -o "$test_output" "$TEST_DIR/hello.c" 2>/dev/null; then
        echo "✓ $tcc_name: 编译测试程序成功"
        
        # 如果是本地架构，尝试运行
        if [[ "$tcc_name" == *"x86_64-linux"* ]] && [ "$(uname -m)" = "x86_64" ]; then
            if "$test_output" 2>/dev/null; then
                echo "✓ $tcc_name: 运行测试程序成功"
            else
                echo "⚠ $tcc_name: 编译成功但运行失败"
            fi
        else
            echo "⚠ $tcc_name: 交叉编译目标，跳过运行测试"
        fi
        
        # 清理测试输出
        rm -f "$test_output"
    else
        echo "✗ $tcc_name: 编译测试程序失败"
        return 1
    fi
    
    return 0
}

# 生成测试报告
generate_test_report() {
    local report_file="$BUILD_DIR/test_report.txt"
    
    echo ""
    echo "=== 生成测试报告 ==="
    
    echo "TCC构建结果测试报告" > "$report_file"
    echo "测试时间: $(date)" >> "$report_file"
    echo "" >> "$report_file"
    
    local total_count=0
    local success_count=0
    
    echo "测试结果摘要:" | tee -a "$report_file"
    
    find "$BUILD_DIR" -name "tcc-*" -type f | while read -r tcc_file; do
        if [ -x "$tcc_file" ]; then
            ((total_count++))
            local tcc_name=$(basename "$tcc_file")
            
            if test_tcc_executable "$tcc_file" >/dev/null 2>&1; then
                echo "  ✓ $tcc_name" | tee -a "$report_file"
                ((success_count++))
            else
                echo "  ✗ $tcc_name" | tee -a "$report_file"
            fi
        fi
    done
    
    echo "" | tee -a "$report_file"
    echo "总计: $success_count/$total_count 通过测试" | tee -a "$report_file"
    echo "" | tee -a "$report_file"
    echo "详细日志请查看完整输出" | tee -a "$report_file"
    
    echo "测试报告保存至: $report_file"
}

# 主函数
main() {
    if [ ! -d "$BUILD_DIR" ]; then
        echo "错误: 构建目录不存在，请先运行构建脚本"
        exit 1
    fi
    
    # 创建测试程序
    create_test_programs
    
    # 查找所有TCC可执行文件
    echo "查找TCC可执行文件..."
    local tcc_files=($(find "$BUILD_DIR" -name "tcc-*" -type f))
    
    if [ ${#tcc_files[@]} -eq 0 ]; then
        echo "错误: 未找到TCC可执行文件"
        exit 1
    fi
    
    echo "找到 ${#tcc_files[@]} 个TCC可执行文件"
    
    # 测试每个TCC可执行文件
    local success_count=0
    
    for tcc_file in "${tcc_files[@]}"; do
        if test_tcc_executable "$tcc_file"; then
            ((success_count++))
        fi
    done
    
    echo ""
    echo "=== 测试总结 ==="
    echo "成功: $success_count/${#tcc_files[@]}"
    
    # 生成测试报告
    generate_test_report
    
    if [ $success_count -eq ${#tcc_files[@]} ]; then
        echo "🎉 所有TCC可执行文件测试通过！"
        return 0
    else
        echo "⚠️ 部分TCC可执行文件测试失败"
        return 1
    fi
}

# 如果直接运行此脚本
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi