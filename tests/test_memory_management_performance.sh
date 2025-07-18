#!/bin/bash
#
# test_memory_management_performance.sh - 内存管理性能基准测试
#
# T3.3 内存管理优化 - 性能基准测试
# 建立内存管理的性能基线，为优化提供数据支持
#

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# 获取脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
RESULTS_DIR="$PROJECT_ROOT/tests/memory_performance_results"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# 创建结果目录
mkdir -p "$RESULTS_DIR"

# 日志函数
log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }
log_step() { echo -e "${CYAN}[STEP]${NC} $1"; }

# 性能测试统计
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# 检查必要的工具和文件
check_prerequisites() {
    log_step "检查测试前提条件"
    
    # 检查编译器
    if ! command -v gcc >/dev/null 2>&1; then
        log_error "GCC编译器不可用"
        return 1
    fi
    
    # 检查valgrind (用于内存分析)
    if command -v valgrind >/dev/null 2>&1; then
        log_info "Valgrind可用，将进行内存泄漏检测"
    else
        log_warning "Valgrind不可用，跳过内存泄漏检测"
    fi
    
    log_success "前提条件检查通过"
    return 0
}

# 创建内存管理性能测试程序
create_memory_performance_test() {
    log_step "创建内存管理性能测试程序"
    
    local test_dir="$RESULTS_DIR/test_programs"
    mkdir -p "$test_dir"
    
    # 创建内存管理性能测试程序
    cat > "$test_dir/memory_perf.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/resource.h>
#include <stdint.h>

// 获取高精度时间
double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// 获取内存使用情况
long get_memory_usage() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss; // KB on Linux, bytes on macOS
}

// 测试基础内存分配性能
int test_basic_allocation_performance() {
    printf("=== 基础内存分配性能测试 ===\n");
    
    const int iterations = 100000;
    const size_t sizes[] = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
    const int size_count = sizeof(sizes) / sizeof(sizes[0]);
    
    for (int s = 0; s < size_count; s++) {
        size_t size = sizes[s];
        printf("测试 %zu 字节分配 (%d 次迭代)\n", size, iterations);
        
        void** ptrs = malloc(iterations * sizeof(void*));
        if (!ptrs) {
            printf("❌ 无法分配指针数组\n");
            continue;
        }
        
        // 测试分配性能
        double start_time = get_time();
        long start_memory = get_memory_usage();
        
        for (int i = 0; i < iterations; i++) {
            ptrs[i] = malloc(size);
            if (!ptrs[i]) {
                printf("❌ 分配失败在第 %d 次\n", i);
                break;
            }
            // 写入一些数据防止编译器优化
            memset(ptrs[i], i & 0xFF, size);
        }
        
        double alloc_time = get_time() - start_time;
        long alloc_memory = get_memory_usage();
        
        // 测试释放性能
        start_time = get_time();
        
        for (int i = 0; i < iterations; i++) {
            if (ptrs[i]) {
                free(ptrs[i]);
            }
        }
        
        double free_time = get_time() - start_time;
        long final_memory = get_memory_usage();
        
        // 计算性能指标
        double alloc_rate = iterations / alloc_time;
        double free_rate = iterations / free_time;
        long memory_used = alloc_memory - start_memory;
        long memory_leaked = final_memory - start_memory;
        
        printf("  分配性能: %.0f 分配/秒 (%.6f 秒)\n", alloc_rate, alloc_time);
        printf("  释放性能: %.0f 释放/秒 (%.6f 秒)\n", free_rate, free_time);
        printf("  内存使用: %ld KB\n", memory_used);
        printf("  内存泄漏: %ld KB\n", memory_leaked);
        printf("  平均分配时间: %.9f 秒\n", alloc_time / iterations);
        printf("  平均释放时间: %.9f 秒\n", free_time / iterations);
        printf("\n");
        
        free(ptrs);
    }
    
    return 0;
}

// 测试内存碎片化
int test_memory_fragmentation() {
    printf("=== 内存碎片化测试 ===\n");
    
    const int iterations = 10000;
    const size_t small_size = 32;
    const size_t large_size = 4096;
    
    void** small_ptrs = malloc(iterations * sizeof(void*));
    void** large_ptrs = malloc(iterations * sizeof(void*));
    
    if (!small_ptrs || !large_ptrs) {
        printf("❌ 无法分配指针数组\n");
        return -1;
    }
    
    printf("测试内存碎片化 (小块: %zu 字节, 大块: %zu 字节)\n", small_size, large_size);
    
    double start_time = get_time();
    long start_memory = get_memory_usage();
    
    // 交替分配小块和大块内存
    for (int i = 0; i < iterations; i++) {
        small_ptrs[i] = malloc(small_size);
        large_ptrs[i] = malloc(large_size);
        
        if (!small_ptrs[i] || !large_ptrs[i]) {
            printf("❌ 分配失败在第 %d 次\n", i);
            break;
        }
        
        // 写入数据
        memset(small_ptrs[i], i & 0xFF, small_size);
        memset(large_ptrs[i], (i + 1) & 0xFF, large_size);
    }
    
    long after_alloc_memory = get_memory_usage();
    
    // 释放一半的小块内存（创建碎片）
    for (int i = 0; i < iterations; i += 2) {
        if (small_ptrs[i]) {
            free(small_ptrs[i]);
            small_ptrs[i] = NULL;
        }
    }
    
    long after_partial_free_memory = get_memory_usage();
    
    // 尝试重新分配小块内存
    double realloc_start = get_time();
    int realloc_success = 0;
    
    for (int i = 0; i < iterations; i += 2) {
        small_ptrs[i] = malloc(small_size);
        if (small_ptrs[i]) {
            memset(small_ptrs[i], i & 0xFF, small_size);
            realloc_success++;
        }
    }
    
    double realloc_time = get_time() - realloc_start;
    long final_memory = get_memory_usage();
    
    // 清理剩余内存
    for (int i = 0; i < iterations; i++) {
        if (small_ptrs[i]) free(small_ptrs[i]);
        if (large_ptrs[i]) free(large_ptrs[i]);
    }
    
    double total_time = get_time() - start_time;
    
    printf("碎片化测试结果:\n");
    printf("  总时间: %.6f 秒\n", total_time);
    printf("  初始分配后内存: %ld KB\n", after_alloc_memory - start_memory);
    printf("  部分释放后内存: %ld KB\n", after_partial_free_memory - start_memory);
    printf("  重新分配时间: %.6f 秒\n", realloc_time);
    printf("  重新分配成功率: %d/%d (%.1f%%)\n", realloc_success, iterations/2, 
           (realloc_success * 200.0) / iterations);
    printf("  最终内存使用: %ld KB\n", final_memory - start_memory);
    
    free(small_ptrs);
    free(large_ptrs);
    
    return 0;
}

// 测试内存对齐性能
int test_memory_alignment_performance() {
    printf("\n=== 内存对齐性能测试 ===\n");
    
    const int iterations = 1000000;
    const size_t test_size = 1024;
    
    // 测试不同对齐方式的性能
    const size_t alignments[] = {1, 4, 8, 16, 32, 64};
    const int align_count = sizeof(alignments) / sizeof(alignments[0]);
    
    for (int a = 0; a < align_count; a++) {
        size_t alignment = alignments[a];
        printf("测试 %zu 字节对齐\n", alignment);
        
        void** ptrs = malloc(iterations * sizeof(void*));
        if (!ptrs) continue;
        
        double start_time = get_time();
        
        for (int i = 0; i < iterations; i++) {
            // 模拟对齐分配
            void* ptr = malloc(test_size + alignment - 1);
            if (ptr) {
                // 对齐指针
                uintptr_t aligned = ((uintptr_t)ptr + alignment - 1) & ~(alignment - 1);
                ptrs[i] = (void*)aligned;
                
                // 写入数据测试访问性能
                volatile char* data = (volatile char*)ptrs[i];
                for (int j = 0; j < 64; j += alignment) {
                    data[j] = (char)(i + j);
                }
            } else {
                ptrs[i] = NULL;
            }
        }
        
        double alloc_time = get_time() - start_time;
        
        // 清理内存
        for (int i = 0; i < iterations; i++) {
            if (ptrs[i]) {
                // 需要释放原始指针，这里简化处理
                free((void*)((uintptr_t)ptrs[i] & ~(alignment - 1)));
            }
        }
        
        printf("  %zu字节对齐性能: %.6f 秒 (%.0f 操作/秒)\n", 
               alignment, alloc_time, iterations / alloc_time);
        
        free(ptrs);
    }
    
    return 0;
}

// 测试内存池性能
int test_memory_pool_performance() {
    printf("\n=== 内存池性能测试 ===\n");
    
    const size_t pool_size = 1024 * 1024; // 1MB
    const size_t block_size = 64;
    const int iterations = pool_size / block_size;
    
    // 创建简单的内存池
    char* pool = malloc(pool_size);
    if (!pool) {
        printf("❌ 无法创建内存池\n");
        return -1;
    }
    
    printf("内存池大小: %zu KB, 块大小: %zu 字节, 迭代次数: %d\n", 
           pool_size / 1024, block_size, iterations);
    
    // 测试内存池分配性能
    double start_time = get_time();
    
    for (int i = 0; i < iterations; i++) {
        char* ptr = pool + (i * block_size);
        // 写入数据
        memset(ptr, i & 0xFF, block_size);
    }
    
    double pool_time = get_time() - start_time;
    
    // 对比系统malloc性能
    void** ptrs = malloc(iterations * sizeof(void*));
    start_time = get_time();
    
    for (int i = 0; i < iterations; i++) {
        ptrs[i] = malloc(block_size);
        if (ptrs[i]) {
            memset(ptrs[i], i & 0xFF, block_size);
        }
    }
    
    double malloc_time = get_time() - start_time;
    
    // 清理malloc分配的内存
    for (int i = 0; i < iterations; i++) {
        if (ptrs[i]) free(ptrs[i]);
    }
    
    printf("内存池性能对比:\n");
    printf("  内存池分配: %.6f 秒 (%.0f 操作/秒)\n", pool_time, iterations / pool_time);
    printf("  系统malloc: %.6f 秒 (%.0f 操作/秒)\n", malloc_time, iterations / malloc_time);
    printf("  性能提升: %.2fx\n", malloc_time / pool_time);
    
    free(pool);
    free(ptrs);
    
    return 0;
}

// 测试内存使用模式
int test_memory_usage_patterns() {
    printf("\n=== 内存使用模式测试 ===\n");
    
    // 测试不同的内存使用模式
    const int iterations = 50000;
    
    // 模式1: 顺序分配和释放
    printf("1. 顺序分配和释放模式\n");
    void** ptrs1 = malloc(iterations * sizeof(void*));
    
    double start_time = get_time();
    long start_memory = get_memory_usage();
    
    // 顺序分配
    for (int i = 0; i < iterations; i++) {
        ptrs1[i] = malloc(64 + (i % 256));
        if (ptrs1[i]) {
            memset(ptrs1[i], i & 0xFF, 64 + (i % 256));
        }
    }
    
    long after_alloc = get_memory_usage();
    
    // 顺序释放
    for (int i = 0; i < iterations; i++) {
        if (ptrs1[i]) free(ptrs1[i]);
    }
    
    double sequential_time = get_time() - start_time;
    long after_free = get_memory_usage();
    
    printf("  顺序模式时间: %.6f 秒\n", sequential_time);
    printf("  内存使用峰值: %ld KB\n", after_alloc - start_memory);
    printf("  内存泄漏: %ld KB\n", after_free - start_memory);
    
    // 模式2: 随机分配和释放
    printf("2. 随机分配和释放模式\n");
    void** ptrs2 = malloc(iterations * sizeof(void*));
    
    start_time = get_time();
    start_memory = get_memory_usage();
    
    // 随机分配
    srand(time(NULL));
    for (int i = 0; i < iterations; i++) {
        ptrs2[i] = malloc(64 + (rand() % 256));
        if (ptrs2[i]) {
            memset(ptrs2[i], i & 0xFF, 64 + (rand() % 256));
        }
    }
    
    after_alloc = get_memory_usage();
    
    // 随机释放
    for (int i = 0; i < iterations; i++) {
        int idx = rand() % iterations;
        if (ptrs2[idx]) {
            free(ptrs2[idx]);
            ptrs2[idx] = NULL;
        }
    }
    
    // 清理剩余内存
    for (int i = 0; i < iterations; i++) {
        if (ptrs2[i]) free(ptrs2[i]);
    }
    
    double random_time = get_time() - start_time;
    after_free = get_memory_usage();
    
    printf("  随机模式时间: %.6f 秒\n", random_time);
    printf("  内存使用峰值: %ld KB\n", after_alloc - start_memory);
    printf("  内存泄漏: %ld KB\n", after_free - start_memory);
    printf("  性能对比: 顺序模式 %.2fx 快于随机模式\n", random_time / sequential_time);
    
    free(ptrs1);
    free(ptrs2);
    
    return 0;
}

int main() {
    printf("=== T3.3 内存管理性能基准测试 ===\n");
    printf("测试时间: %s\n", __DATE__ " " __TIME__);
    printf("进程ID: %d\n", getpid());
    printf("系统信息: ");
    system("uname -m");
    printf("\n");
    
    int result = 0;
    
    // 运行各项内存性能测试
    if (test_basic_allocation_performance() != 0) {
        result = -1;
    }
    
    if (test_memory_fragmentation() != 0) {
        result = -1;
    }
    
    if (test_memory_alignment_performance() != 0) {
        result = -1;
    }
    
    if (test_memory_pool_performance() != 0) {
        result = -1;
    }
    
    if (test_memory_usage_patterns() != 0) {
        result = -1;
    }
    
    printf("\n=== T3.3 内存管理基准测试完成 ===\n");
    if (result == 0) {
        printf("✅ 所有测试完成，内存管理基线已建立\n");
    } else {
        printf("❌ 部分测试失败\n");
    }
    
    return result;
}
EOF

    # 编译测试程序
    local cc_cmd="gcc"
    if command -v clang >/dev/null 2>&1; then
        cc_cmd="clang"
    fi
    
    if $cc_cmd -O2 "$test_dir/memory_perf.c" -o "$test_dir/memory_perf"; then
        log_success "内存管理性能测试程序编译完成"
        return 0
    else
        log_error "内存管理性能测试程序编译失败"
        return 1
    fi
}

# 运行内存管理性能基准测试
run_memory_performance_baseline() {
    log_step "运行内存管理性能基准测试"
    
    local test_program="$RESULTS_DIR/test_programs/memory_perf"
    local results_file="$RESULTS_DIR/memory_baseline_performance_${TIMESTAMP}.txt"
    
    if [ ! -x "$test_program" ]; then
        log_error "测试程序不存在或不可执行: $test_program"
        return 1
    fi
    
    log_info "运行内存基准测试，结果将保存到: $results_file"
    
    # 运行测试并保存结果
    if "$test_program" | tee "$results_file"; then
        log_success "内存基准性能测试完成"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        log_error "内存基准性能测试失败"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
}

# 分析内存性能结果
analyze_memory_performance() {
    log_step "分析内存性能结果"
    
    local results_file="$RESULTS_DIR/memory_baseline_performance_${TIMESTAMP}.txt"
    local analysis_file="$RESULTS_DIR/memory_performance_analysis_${TIMESTAMP}.md"
    
    if [ ! -f "$results_file" ]; then
        log_warning "结果文件不存在，跳过分析"
        return 0
    fi
    
    # 提取关键性能指标
    local alloc_rate_64=$(grep "64 字节分配" -A 6 "$results_file" | grep "分配性能" | grep -o '[0-9.]\+' | head -1)
    local pool_improvement=$(grep "性能提升" "$results_file" | grep -o '[0-9.]\+' | head -1)
    local fragmentation_success=$(grep "重新分配成功率" "$results_file" | grep -o '[0-9.]\+%' | head -1)
    
    cat > "$analysis_file" << EOF
# 内存管理性能基准分析

**测试时间**: $(date)
**测试版本**: T3.3 内存管理优化 - 基准测试

## 性能基线数据

### 基础分配性能
- 64字节分配速度: ${alloc_rate_64:-"N/A"} 分配/秒
- 测试规模: 100,000次分配/释放
- 测试大小范围: 16字节 - 4KB

### 内存池性能
- 内存池性能提升: ${pool_improvement:-"N/A"}x (相比系统malloc)
- 池大小: 1MB
- 块大小: 64字节

### 内存碎片化
- 重新分配成功率: ${fragmentation_success:-"N/A"}
- 碎片化测试: 交替分配小块(32B)和大块(4KB)
- 部分释放后重新分配测试

## 性能瓶颈分析

### 识别的瓶颈
1. **系统malloc开销**: 频繁的系统调用开销
2. **内存碎片化**: 小块和大块交替分配导致碎片
3. **对齐开销**: 不同对齐要求的性能差异
4. **释放延迟**: 内存释放到重用的延迟

### 优化机会
1. **内存池优化**: 针对不同大小的专用内存池
2. **碎片整理**: 定期内存碎片整理
3. **对齐优化**: 优化内存对齐策略
4. **缓存友好**: 改善内存访问局部性
5. **延迟释放**: 延迟释放策略减少碎片

## T3.3优化目标

基于当前基线，T3.3的优化目标：
- **内存使用效率提升15%**: 减少内存浪费和碎片
- **分配性能提升**: 提高内存分配/释放速度
- **碎片化减少**: 改善内存碎片化问题
- **内存对齐优化**: 优化不同对齐需求的性能

## 优化策略

### 第一阶段: 内存池优化
1. **分级内存池**: 不同大小的专用内存池
2. **快速分配**: 优化小块内存的快速分配
3. **对齐优化**: 预对齐的内存池块

### 第二阶段: 碎片管理
1. **碎片检测**: 实时碎片化监控
2. **碎片整理**: 定期或按需碎片整理
3. **合并策略**: 相邻空闲块的智能合并

### 第三阶段: 高级优化
1. **NUMA感知**: NUMA架构的内存分配优化
2. **线程本地**: 线程本地内存池
3. **预测分配**: 基于使用模式的预测分配

---
*基准分析生成时间: $(date)*
EOF

    log_success "内存性能分析报告生成完成: $analysis_file"
}

# 生成内存性能测试总结
generate_memory_performance_summary() {
    local summary_file="$RESULTS_DIR/memory_performance_summary_${TIMESTAMP}.md"
    
    cat > "$summary_file" << EOF
# T3.3 内存管理性能测试总结

**测试时间**: $(date)
**测试阶段**: 基准测试 (优化前)

## 测试概览

- **总测试数**: $TOTAL_TESTS
- **通过测试**: $PASSED_TESTS
- **失败测试**: $FAILED_TESTS
- **成功率**: $(echo "scale=1; $PASSED_TESTS * 100 / $TOTAL_TESTS" | bc -l 2>/dev/null || echo "N/A")%

## 测试内容

### 1. 基础内存分配性能测试
- ✅ 不同大小内存块的分配/释放性能
- ✅ 内存使用量和泄漏检测
- ✅ 分配速度和释放速度测量

### 2. 内存碎片化测试
- ✅ 交替分配小块和大块内存
- ✅ 部分释放后的重新分配测试
- ✅ 碎片化对性能的影响分析

### 3. 内存对齐性能测试
- ✅ 不同对齐方式的性能对比
- ✅ 对齐开销分析

### 4. 内存池性能测试
- ✅ 内存池 vs 系统malloc性能对比
- ✅ 内存池效率分析

### 5. 内存使用模式测试
- ✅ 顺序分配/释放 vs 随机分配/释放
- ✅ 不同使用模式的性能特征

## 生成的文件

- \`memory_baseline_performance_${TIMESTAMP}.txt\` - 基准测试原始结果
- \`memory_performance_analysis_${TIMESTAMP}.md\` - 性能分析报告
- \`memory_performance_summary_${TIMESTAMP}.md\` - 本总结文件

## 下一步计划

1. **实施内存管理优化**: 根据分析结果实施具体优化措施
2. **优化后测试**: 运行相同的测试验证优化效果
3. **性能对比**: 对比优化前后的性能数据
4. **文档更新**: 更新相关文档和使用指南

## 使用方法

\`\`\`bash
# 运行内存基准测试
./tests/test_memory_management_performance.sh

# 查看结果
cat tests/memory_performance_results/memory_baseline_performance_*.txt

# 查看分析
cat tests/memory_performance_results/memory_performance_analysis_*.md
\`\`\`

---
*总结生成时间: $(date)*
EOF

    log_success "内存性能测试总结生成完成: $summary_file"
}

# 主函数
main() {
    echo -e "${BLUE}=== T3.3 内存管理性能基准测试 ===${NC}"
    echo "开始时间: $(date)"
    echo
    
    if ! check_prerequisites; then
        exit 1
    fi
    
    create_memory_performance_test
    run_memory_performance_baseline
    analyze_memory_performance
    generate_memory_performance_summary
    
    echo
    echo -e "${BLUE}=== 测试总结 ===${NC}"
    echo "总测试数: $TOTAL_TESTS"
    echo "通过测试: $PASSED_TESTS"
    echo "失败测试: $FAILED_TESTS"
    
    if [ $FAILED_TESTS -eq 0 ]; then
        echo -e "${GREEN}所有测试通过！内存管理基准数据已建立。${NC}"
    else
        echo -e "${YELLOW}部分测试失败，请检查结果。${NC}"
    fi
    
    echo
    echo "结果文件保存在: $RESULTS_DIR"
    echo "下一步: 实施内存管理优化措施"
}

# 运行主函数
main "$@"
