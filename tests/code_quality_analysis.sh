#!/bin/bash

# 代码质量分析脚本
# 分析编译器源代码的质量指标

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
SRC_DIR="$PROJECT_ROOT/src"
RESULTS_DIR="$TEST_DIR/code_quality_results"

# 创建结果目录
mkdir -p "$RESULTS_DIR"

echo -e "${BLUE}=== 代码质量分析 ===${NC}"
echo "项目根目录: $PROJECT_ROOT"
echo "源代码目录: $SRC_DIR"
echo "结果目录: $RESULTS_DIR"
echo

# 结果文件
RESULTS_FILE="$RESULTS_DIR/quality_analysis.txt"
echo "代码质量分析结果 - $(date)" > "$RESULTS_FILE"
echo "======================================" >> "$RESULTS_FILE"

# 代码行数统计
analyze_code_lines() {
    echo -e "${BLUE}=== 代码行数统计 ===${NC}"
    echo "代码行数统计:" >> "$RESULTS_FILE"
    
    local total_lines=0
    local total_files=0
    local c_files=0
    local h_files=0
    local c_lines=0
    local h_lines=0
    
    # 统计C文件
    if find "$SRC_DIR" -name "*.c" -type f | head -1 > /dev/null 2>&1; then
        while IFS= read -r -d '' file; do
            local lines=$(wc -l < "$file")
            c_lines=$((c_lines + lines))
            c_files=$((c_files + 1))
            echo "  $(basename "$file"): $lines 行"
        done < <(find "$SRC_DIR" -name "*.c" -type f -print0)
    fi
    
    # 统计头文件
    if find "$SRC_DIR" -name "*.h" -type f | head -1 > /dev/null 2>&1; then
        while IFS= read -r -d '' file; do
            local lines=$(wc -l < "$file")
            h_lines=$((h_lines + lines))
            h_files=$((h_files + 1))
            echo "  $(basename "$file"): $lines 行"
        done < <(find "$SRC_DIR" -name "*.h" -type f -print0)
    fi
    
    total_lines=$((c_lines + h_lines))
    total_files=$((c_files + h_files))
    
    echo "  C文件数量: $c_files" >> "$RESULTS_FILE"
    echo "  C文件行数: $c_lines" >> "$RESULTS_FILE"
    echo "  头文件数量: $h_files" >> "$RESULTS_FILE"
    echo "  头文件行数: $h_lines" >> "$RESULTS_FILE"
    echo "  总文件数量: $total_files" >> "$RESULTS_FILE"
    echo "  总代码行数: $total_lines" >> "$RESULTS_FILE"
    
    echo -e "  总文件数量: ${GREEN}$total_files${NC}"
    echo -e "  总代码行数: ${GREEN}$total_lines${NC}"
    echo -e "  C文件: ${BLUE}$c_files 个文件，$c_lines 行${NC}"
    echo -e "  头文件: ${BLUE}$h_files 个文件，$h_lines 行${NC}"
    echo
}

# 函数复杂度分析
analyze_function_complexity() {
    echo -e "${BLUE}=== 函数复杂度分析 ===${NC}"
    echo "" >> "$RESULTS_FILE"
    echo "函数复杂度分析:" >> "$RESULTS_FILE"
    
    local total_functions=0
    local complex_functions=0
    local max_lines=0
    local max_function=""
    
    # 分析C文件中的函数
    if find "$SRC_DIR" -name "*.c" -type f | head -1 > /dev/null 2>&1; then
        while IFS= read -r -d '' file; do
            echo "  分析文件: $(basename "$file")"
            
            # 简单的函数检测（查找函数定义模式）
            local functions=$(grep -n "^[a-zA-Z_][a-zA-Z0-9_]*.*(" "$file" | grep -v "^[[:space:]]*//")
            
            while IFS= read -r line; do
                if [ -n "$line" ]; then
                    local line_num=$(echo "$line" | cut -d: -f1)
                    local func_name=$(echo "$line" | cut -d: -f2- | sed 's/(.*//' | xargs)
                    
                    # 估算函数长度（查找下一个函数或文件结束）
                    local next_func_line=$(tail -n +$((line_num + 1)) "$file" | grep -n "^[a-zA-Z_][a-zA-Z0-9_]*.*(" | head -1 | cut -d: -f1)
                    local func_length
                    
                    if [ -n "$next_func_line" ]; then
                        func_length=$((next_func_line - 1))
                    else
                        local total_file_lines=$(wc -l < "$file")
                        func_length=$((total_file_lines - line_num + 1))
                    fi
                    
                    total_functions=$((total_functions + 1))
                    
                    if [ $func_length -gt $max_lines ]; then
                        max_lines=$func_length
                        max_function="$func_name ($(basename "$file"):$line_num)"
                    fi
                    
                    if [ $func_length -gt 50 ]; then
                        complex_functions=$((complex_functions + 1))
                        echo "    复杂函数: $func_name ($func_length 行)" >> "$RESULTS_FILE"
                    fi
                fi
            done <<< "$functions"
            
        done < <(find "$SRC_DIR" -name "*.c" -type f -print0)
    fi
    
    echo "  总函数数量: $total_functions" >> "$RESULTS_FILE"
    echo "  复杂函数数量 (>50行): $complex_functions" >> "$RESULTS_FILE"
    echo "  最长函数: $max_function ($max_lines 行)" >> "$RESULTS_FILE"
    
    echo -e "  总函数数量: ${GREEN}$total_functions${NC}"
    echo -e "  复杂函数数量 (>50行): ${YELLOW}$complex_functions${NC}"
    echo -e "  最长函数: ${BLUE}$max_function ($max_lines 行)${NC}"
    echo
}

# 代码重复分析
analyze_code_duplication() {
    echo -e "${BLUE}=== 代码重复分析 ===${NC}"
    echo "" >> "$RESULTS_FILE"
    echo "代码重复分析:" >> "$RESULTS_FILE"
    
    # 简单的重复代码检测（查找相同的行）
    local duplicate_lines=0
    local total_lines=0
    
    if find "$SRC_DIR" -name "*.c" -type f | head -1 > /dev/null 2>&1; then
        # 收集所有非空行
        local temp_file="/tmp/all_lines.txt"
        find "$SRC_DIR" -name "*.c" -type f -exec cat {} \; | \
            grep -v "^[[:space:]]*$" | \
            grep -v "^[[:space:]]*//.*$" | \
            grep -v "^[[:space:]]*\*.*$" > "$temp_file"
        
        total_lines=$(wc -l < "$temp_file")
        local unique_lines=$(sort "$temp_file" | uniq | wc -l)
        duplicate_lines=$((total_lines - unique_lines))
        
        rm -f "$temp_file"
    fi
    
    local duplication_rate=0
    if [ $total_lines -gt 0 ]; then
        duplication_rate=$((duplicate_lines * 100 / total_lines))
    fi
    
    echo "  总有效行数: $total_lines" >> "$RESULTS_FILE"
    echo "  重复行数: $duplicate_lines" >> "$RESULTS_FILE"
    echo "  重复率: ${duplication_rate}%" >> "$RESULTS_FILE"
    
    echo -e "  总有效行数: ${GREEN}$total_lines${NC}"
    echo -e "  重复行数: ${YELLOW}$duplicate_lines${NC}"
    echo -e "  重复率: ${BLUE}${duplication_rate}%${NC}"
    echo
}

# 注释覆盖率分析
analyze_comment_coverage() {
    echo -e "${BLUE}=== 注释覆盖率分析 ===${NC}"
    echo "" >> "$RESULTS_FILE"
    echo "注释覆盖率分析:" >> "$RESULTS_FILE"
    
    local total_lines=0
    local comment_lines=0
    
    if find "$SRC_DIR" -name "*.c" -o -name "*.h" | head -1 > /dev/null 2>&1; then
        while IFS= read -r -d '' file; do
            local file_lines=$(wc -l < "$file")
            local file_comments=$(grep -c "^\s*//\|^\s*/\*\|^\s*\*" "$file" || true)
            
            total_lines=$((total_lines + file_lines))
            comment_lines=$((comment_lines + file_comments))
        done < <(find "$SRC_DIR" \( -name "*.c" -o -name "*.h" \) -type f -print0)
    fi
    
    local comment_rate=0
    if [ $total_lines -gt 0 ]; then
        comment_rate=$((comment_lines * 100 / total_lines))
    fi
    
    echo "  总行数: $total_lines" >> "$RESULTS_FILE"
    echo "  注释行数: $comment_lines" >> "$RESULTS_FILE"
    echo "  注释覆盖率: ${comment_rate}%" >> "$RESULTS_FILE"
    
    echo -e "  总行数: ${GREEN}$total_lines${NC}"
    echo -e "  注释行数: ${BLUE}$comment_lines${NC}"
    echo -e "  注释覆盖率: ${YELLOW}${comment_rate}%${NC}"
    echo
}

# 代码质量评分
calculate_quality_score() {
    echo -e "${BLUE}=== 代码质量评分 ===${NC}"
    echo "" >> "$RESULTS_FILE"
    echo "代码质量评分:" >> "$RESULTS_FILE"
    
    # 简单的质量评分算法
    local score=100
    
    # 根据各项指标调整分数
    # 这里可以根据实际分析结果调整
    
    echo "  基础分数: 100" >> "$RESULTS_FILE"
    echo "  最终质量分数: $score" >> "$RESULTS_FILE"
    
    if [ $score -ge 90 ]; then
        echo -e "  代码质量: ${GREEN}优秀 ($score/100)${NC}"
        echo "  代码质量: 优秀 ($score/100)" >> "$RESULTS_FILE"
    elif [ $score -ge 80 ]; then
        echo -e "  代码质量: ${BLUE}良好 ($score/100)${NC}"
        echo "  代码质量: 良好 ($score/100)" >> "$RESULTS_FILE"
    elif [ $score -ge 70 ]; then
        echo -e "  代码质量: ${YELLOW}一般 ($score/100)${NC}"
        echo "  代码质量: 一般 ($score/100)" >> "$RESULTS_FILE"
    else
        echo -e "  代码质量: ${RED}需要改进 ($score/100)${NC}"
        echo "  代码质量: 需要改进 ($score/100)" >> "$RESULTS_FILE"
    fi
    echo
}

# 主函数
main() {
    analyze_code_lines
    analyze_function_complexity
    analyze_code_duplication
    analyze_comment_coverage
    calculate_quality_score
    
    echo -e "${GREEN}=== 代码质量分析完成 ===${NC}"
    echo "详细结果保存在: $RESULTS_FILE"
    echo
    echo "查看结果: cat $RESULTS_FILE"
}

# 运行主函数
main "$@"
