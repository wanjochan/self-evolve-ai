#!/bin/bash

# Performance Benchmark Test Suite for prd_0_2
# 性能基准测试套件

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RESULTS_DIR="$TEST_DIR/performance_benchmark_results"

echo -e "${BLUE}=== 性能基准测试套件 ===${NC}"
echo "项目根目录: $PROJECT_ROOT"
echo "测试目录: $TEST_DIR"
echo "结果目录: $RESULTS_DIR"
echo "测试时间: $(date)"
echo

# 创建结果目录
mkdir -p "$RESULTS_DIR"

# 切换到项目根目录
cd "$PROJECT_ROOT"

# 性能测试函数
benchmark_test() {
    local test_name="$1"
    local test_command="$2"
    local iterations="$3"
    
    echo -e "${YELLOW}=== $test_name ===${NC}"
    
    local total_time=0
    local min_time=999999
    local max_time=0
    local success_count=0
    
    for i in $(seq 1 $iterations); do
        local start_time=$(date +%s.%N)
        
        if eval "$test_command" > "$RESULTS_DIR/${test_name}_${i}.log" 2>&1; then
            local end_time=$(date +%s.%N)
            local duration=$(echo "$end_time - $start_time" | bc -l)
            
            total_time=$(echo "$total_time + $duration" | bc -l)
            success_count=$((success_count + 1))
            
            # 更新最小和最大时间
            if (( $(echo "$duration < $min_time" | bc -l) )); then
                min_time=$duration
            fi
            if (( $(echo "$duration > $max_time" | bc -l) )); then
                max_time=$duration
            fi
            
            echo "  迭代 $i: ${duration}s"
        else
            echo "  迭代 $i: 失败"
        fi
    done
    
    if [ $success_count -gt 0 ]; then
        local avg_time=$(echo "scale=6; $total_time / $success_count" | bc -l)
        echo -e "  ${GREEN}成功率: $success_count/$iterations${NC}"
        echo -e "  ${GREEN}平均时间: ${avg_time}s${NC}"
        echo -e "  ${GREEN}最小时间: ${min_time}s${NC}"
        echo -e "  ${GREEN}最大时间: ${max_time}s${NC}"
        
        # 保存基准数据
        cat > "$RESULTS_DIR/${test_name}_benchmark.txt" << EOF
测试名称: $test_name
迭代次数: $iterations
成功次数: $success_count
成功率: $(echo "scale=2; $success_count * 100 / $iterations" | bc -l)%
平均时间: ${avg_time}s
最小时间: ${min_time}s
最大时间: ${max_time}s
总时间: ${total_time}s
测试时间: $(date)
EOF
    else
        echo -e "  ${RED}所有迭代都失败${NC}"
    fi
    echo
}

# 检查bc命令是否可用
if ! command -v bc > /dev/null 2>&1; then
    echo -e "${YELLOW}警告: bc命令未找到，使用简化的时间计算${NC}"
    # 简化版本的benchmark函数
    benchmark_test() {
        local test_name="$1"
        local test_command="$2"
        local iterations="$3"
        
        echo -e "${YELLOW}=== $test_name ===${NC}"
        
        local success_count=0
        local start_total=$(date +%s)
        
        for i in $(seq 1 $iterations); do
            if eval "$test_command" > "$RESULTS_DIR/${test_name}_${i}.log" 2>&1; then
                success_count=$((success_count + 1))
                echo "  迭代 $i: 成功"
            else
                echo "  迭代 $i: 失败"
            fi
        done
        
        local end_total=$(date +%s)
        local total_time=$((end_total - start_total))
        
        echo -e "  ${GREEN}成功率: $success_count/$iterations${NC}"
        echo -e "  ${GREEN}总时间: ${total_time}s${NC}"
        
        if [ $success_count -gt 0 ]; then
            local avg_time=$((total_time / success_count))
            echo -e "  ${GREEN}平均时间: ${avg_time}s${NC}"
        fi
        echo
    }
fi

echo -e "${BLUE}=== 模块加载性能测试 ===${NC}"

# 基准测试1: 简单模块加载
benchmark_test "simple_loader_basic" \
    "./bin/simple_loader ./tests/test_minimal.astc" \
    10

# 基准测试2: 复杂程序加载
if [ -f "./tests/test_complex_c99.astc" ]; then
    benchmark_test "simple_loader_complex" \
        "./bin/simple_loader ./tests/test_complex_c99.astc" \
        5
fi

echo -e "${BLUE}=== 编译性能测试 ===${NC}"

# 基准测试3: C2ASTC编译性能
if [ -f "./bin/c2astc" ]; then
    benchmark_test "c2astc_compilation" \
        "./bin/c2astc ./tests/test_simple.c $RESULTS_DIR/test_simple_compiled.astc" \
        5
fi

echo -e "${BLUE}=== 内存使用性能测试 ===${NC}"

# 基准测试4: 内存使用测试
if command -v /usr/bin/time > /dev/null 2>&1; then
    echo -e "${YELLOW}=== 内存使用分析 ===${NC}"
    /usr/bin/time -v ./bin/simple_loader ./tests/test_minimal.astc 2> "$RESULTS_DIR/memory_usage.txt" || true
    
    if [ -f "$RESULTS_DIR/memory_usage.txt" ]; then
        echo "内存使用情况:"
        grep -E "(Maximum resident set size|Page reclaims|Page faults)" "$RESULTS_DIR/memory_usage.txt" || true
    fi
    echo
fi

echo -e "${BLUE}=== 并发性能测试 ===${NC}"

# 基准测试5: 并发加载性能
echo -e "${YELLOW}=== 并发加载测试 ===${NC}"
concurrent_start=$(date +%s)
for i in {1..5}; do
    ./bin/simple_loader ./tests/test_minimal.astc > "$RESULTS_DIR/concurrent_${i}.log" 2>&1 &
done
wait
concurrent_end=$(date +%s)
concurrent_time=$((concurrent_end - concurrent_start))
echo -e "  ${GREEN}并发加载时间: ${concurrent_time}s${NC}"

echo -e "${BLUE}=== 系统资源监控 ===${NC}"

# 系统信息收集
cat > "$RESULTS_DIR/system_info.txt" << EOF
系统信息:
操作系统: $(uname -a)
CPU信息: $(grep "model name" /proc/cpuinfo | head -1 | cut -d: -f2 | xargs || echo "未知")
内存信息: $(free -h | grep "Mem:" || echo "未知")
磁盘信息: $(df -h . | tail -1 || echo "未知")
测试时间: $(date)
EOF

echo "系统信息已保存到: $RESULTS_DIR/system_info.txt"

echo -e "${BLUE}=== 性能基准汇总 ===${NC}"

# 生成性能报告
cat > "$RESULTS_DIR/performance_summary.txt" << EOF
性能基准测试汇总报告
===================

测试时间: $(date)
测试环境: $(uname -a)

基准测试结果:
EOF

# 汇总所有基准测试结果
for benchmark_file in "$RESULTS_DIR"/*_benchmark.txt; do
    if [ -f "$benchmark_file" ]; then
        echo "" >> "$RESULTS_DIR/performance_summary.txt"
        cat "$benchmark_file" >> "$RESULTS_DIR/performance_summary.txt"
        echo "---" >> "$RESULTS_DIR/performance_summary.txt"
    fi
done

echo -e "${GREEN}性能基准测试完成！${NC}"
echo "详细报告保存在: $RESULTS_DIR/performance_summary.txt"

# 显示简要汇总
if [ -f "$RESULTS_DIR/performance_summary.txt" ]; then
    echo
    echo -e "${BLUE}=== 性能汇总 ===${NC}"
    grep -E "(测试名称|平均时间|成功率)" "$RESULTS_DIR/performance_summary.txt" | head -20
fi
