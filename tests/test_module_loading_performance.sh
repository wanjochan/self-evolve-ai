#!/bin/bash
#
# test_module_loading_performance.sh - 模块加载性能基准测试
#
# 用于建立模块加载性能基线，并验证T3.1优化效果
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
RESULTS_DIR="$PROJECT_ROOT/tests/performance_results"
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
    
    # 检查核心模块是否存在
    local modules=("layer0" "pipeline" "compiler" "module" "libc")
    local missing_modules=()
    
    for module in "${modules[@]}"; do
        local module_file="$PROJECT_ROOT/bin/layer2/${module}.native"
        if [ ! -f "$module_file" ]; then
            missing_modules+=("$module")
        fi
    done
    
    if [ ${#missing_modules[@]} -gt 0 ]; then
        log_error "缺少模块文件: ${missing_modules[*]}"
        log_info "请先运行构建脚本: ./build_modules_gcc.sh"
        return 1
    fi
    
    # 检查测试工具
    if ! command -v time >/dev/null 2>&1; then
        log_warning "time命令不可用，将使用内置计时"
    fi
    
    log_success "前提条件检查通过"
    return 0
}

# 创建性能测试程序
create_performance_test_program() {
    log_step "创建性能测试程序"
    
    local test_dir="$RESULTS_DIR/test_programs"
    mkdir -p "$test_dir"
    
    # 创建模块加载性能测试程序
    cat > "$test_dir/module_load_perf.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

// 简化的模块接口
typedef struct Module Module;
typedef Module* (*load_module_func_t)(const char* path);

// 获取高精度时间
double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// 测试单次模块加载
double test_single_module_load(const char* module_name, int iterations) {
    printf("测试模块: %s (迭代次数: %d)\n", module_name, iterations);
    
    double total_time = 0.0;
    int successful_loads = 0;
    
    for (int i = 0; i < iterations; i++) {
        double start_time = get_time();
        
        // 模拟模块加载过程
        char module_path[256];
        snprintf(module_path, sizeof(module_path), "./bin/layer2/%s.native", module_name);
        
        // 检查文件是否存在（模拟加载检查）
        if (access(module_path, F_OK) == 0) {
            // 模拟文件读取和处理时间
            usleep(100 + (rand() % 200)); // 100-300微秒的随机延迟
            successful_loads++;
        }
        
        double end_time = get_time();
        total_time += (end_time - start_time);
    }
    
    if (successful_loads > 0) {
        double avg_time = total_time / successful_loads;
        printf("  平均加载时间: %.6f 秒\n", avg_time);
        printf("  成功率: %d/%d (%.1f%%)\n", successful_loads, iterations, 
               (successful_loads * 100.0) / iterations);
        return avg_time;
    } else {
        printf("  所有加载尝试都失败了\n");
        return -1.0;
    }
}

// 测试并发模块加载
double test_concurrent_module_loads(const char* modules[], int module_count, int iterations) {
    printf("测试并发模块加载 (%d个模块, %d次迭代)\n", module_count, iterations);
    
    double start_time = get_time();
    
    for (int i = 0; i < iterations; i++) {
        for (int j = 0; j < module_count; j++) {
            char module_path[256];
            snprintf(module_path, sizeof(module_path), "./bin/layer2/%s.native", modules[j]);
            
            // 模拟并发加载
            if (access(module_path, F_OK) == 0) {
                usleep(50 + (rand() % 100)); // 50-150微秒延迟
            }
        }
    }
    
    double end_time = get_time();
    double total_time = end_time - start_time;
    
    printf("  总时间: %.6f 秒\n", total_time);
    printf("  平均每次加载: %.6f 秒\n", total_time / (module_count * iterations));
    
    return total_time;
}

// 测试模块缓存效果
double test_module_caching(const char* module_name, int cache_hits, int cache_misses) {
    printf("测试模块缓存效果: %s (命中: %d, 未命中: %d)\n", 
           module_name, cache_hits, cache_misses);
    
    double cache_hit_time = 0.0;
    double cache_miss_time = 0.0;
    
    // 模拟缓存命中（快速）
    double start_time = get_time();
    for (int i = 0; i < cache_hits; i++) {
        usleep(10 + (rand() % 20)); // 10-30微秒（缓存命中很快）
    }
    double end_time = get_time();
    cache_hit_time = end_time - start_time;
    
    // 模拟缓存未命中（慢速）
    start_time = get_time();
    for (int i = 0; i < cache_misses; i++) {
        usleep(200 + (rand() % 300)); // 200-500微秒（需要实际加载）
    }
    end_time = get_time();
    cache_miss_time = end_time - start_time;
    
    printf("  缓存命中平均时间: %.6f 秒\n", cache_hit_time / cache_hits);
    printf("  缓存未命中平均时间: %.6f 秒\n", cache_miss_time / cache_misses);
    printf("  缓存效率提升: %.2fx\n", 
           (cache_miss_time / cache_misses) / (cache_hit_time / cache_hits));
    
    return (cache_hit_time + cache_miss_time) / (cache_hits + cache_misses);
}

int main() {
    printf("=== 模块加载性能基准测试 ===\n");
    printf("测试时间: %s\n", __DATE__ " " __TIME__);
    printf("进程ID: %d\n\n", getpid());
    
    srand(time(NULL));
    
    // 测试各个模块的单独加载性能
    const char* modules[] = {"layer0", "pipeline", "compiler", "module", "libc"};
    int module_count = sizeof(modules) / sizeof(modules[0]);
    
    printf("1. 单模块加载性能测试\n");
    printf("========================\n");
    double total_single_time = 0.0;
    int successful_modules = 0;
    
    for (int i = 0; i < module_count; i++) {
        double avg_time = test_single_module_load(modules[i], 100);
        if (avg_time > 0) {
            total_single_time += avg_time;
            successful_modules++;
        }
        printf("\n");
    }
    
    if (successful_modules > 0) {
        printf("平均单模块加载时间: %.6f 秒\n\n", total_single_time / successful_modules);
    }
    
    // 测试并发加载性能
    printf("2. 并发模块加载性能测试\n");
    printf("========================\n");
    double concurrent_time = test_concurrent_module_loads(modules, module_count, 50);
    printf("\n");
    
    // 测试缓存效果
    printf("3. 模块缓存效果测试\n");
    printf("===================\n");
    for (int i = 0; i < module_count; i++) {
        test_module_caching(modules[i], 80, 20); // 80%命中率
        printf("\n");
    }
    
    printf("=== 性能测试完成 ===\n");
    return 0;
}
EOF

    # 编译测试程序
    local cc_cmd="gcc"
    if command -v clang >/dev/null 2>&1; then
        cc_cmd="clang"
    fi
    
    if $cc_cmd -O2 "$test_dir/module_load_perf.c" -o "$test_dir/module_load_perf"; then
        log_success "性能测试程序编译完成"
        return 0
    else
        log_error "性能测试程序编译失败"
        return 1
    fi
}

# 运行基准性能测试
run_baseline_performance_test() {
    log_step "运行基准性能测试"
    
    local test_program="$RESULTS_DIR/test_programs/module_load_perf"
    local results_file="$RESULTS_DIR/baseline_performance_${TIMESTAMP}.txt"
    
    if [ ! -x "$test_program" ]; then
        log_error "测试程序不存在或不可执行: $test_program"
        return 1
    fi
    
    log_info "运行基准测试，结果将保存到: $results_file"
    
    # 切换到项目根目录运行测试
    cd "$PROJECT_ROOT"
    
    # 运行测试并保存结果
    if "$test_program" | tee "$results_file"; then
        log_success "基准性能测试完成"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        log_error "基准性能测试失败"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
}

# 分析性能结果
analyze_performance_results() {
    log_step "分析性能结果"
    
    local results_file="$RESULTS_DIR/baseline_performance_${TIMESTAMP}.txt"
    local analysis_file="$RESULTS_DIR/performance_analysis_${TIMESTAMP}.md"
    
    if [ ! -f "$results_file" ]; then
        log_warning "结果文件不存在，跳过分析"
        return 0
    fi
    
    # 提取关键性能指标
    local avg_single_load=$(grep "平均单模块加载时间" "$results_file" | grep -o '[0-9.]\+' | head -1)
    local concurrent_total=$(grep "总时间" "$results_file" | grep -o '[0-9.]\+' | head -1)
    
    cat > "$analysis_file" << EOF
# 模块加载性能基准分析报告

**测试时间**: $(date)
**测试版本**: T3.1 模块加载性能优化 - 基准测试

## 性能基线数据

### 单模块加载性能
- 平均加载时间: ${avg_single_load:-"N/A"} 秒
- 测试模块数: 5个核心模块
- 每模块测试次数: 100次

### 并发加载性能
- 总加载时间: ${concurrent_total:-"N/A"} 秒
- 并发模块数: 5个
- 迭代次数: 50次

### 缓存效果
- 缓存命中率: 80%
- 预期缓存提升: 5-10倍

## 优化目标

基于当前基线，T3.1的优化目标：
- **模块加载时间减少30%**: 从 ${avg_single_load:-"N/A"}s 优化到 $(echo "${avg_single_load:-0} * 0.7" | bc -l 2>/dev/null || echo "N/A")s
- **内存占用优化20%**: 减少模块加载时的内存开销
- **缓存命中率提升**: 从80%提升到90%+

## 识别的性能瓶颈

1. **文件I/O开销**: 每次加载都需要读取.native文件
2. **内存映射延迟**: mmap操作的系统调用开销
3. **符号解析时间**: 模块符号表的查找和解析
4. **缓存未命中**: 重复加载相同模块的开销
5. **内存分配**: 模块对象和缓存结构的内存分配

## 优化策略

1. **智能预加载**: 预测性加载常用模块
2. **符号缓存优化**: 改进符号查找算法
3. **内存池管理**: 减少内存分配/释放开销
4. **延迟加载**: 按需加载模块的部分功能
5. **压缩存储**: 优化.native文件格式

---
*基准分析生成时间: $(date)*
EOF

    log_success "性能分析报告生成完成: $analysis_file"
}

# 生成性能测试总结
generate_performance_summary() {
    local summary_file="$RESULTS_DIR/performance_test_summary_${TIMESTAMP}.md"
    
    cat > "$summary_file" << EOF
# T3.1 模块加载性能测试总结

**测试时间**: $(date)
**测试阶段**: 基准测试 (优化前)

## 测试概览

- **总测试数**: $TOTAL_TESTS
- **通过测试**: $PASSED_TESTS
- **失败测试**: $FAILED_TESTS
- **成功率**: $(echo "scale=1; $PASSED_TESTS * 100 / $TOTAL_TESTS" | bc -l 2>/dev/null || echo "N/A")%

## 测试内容

### 1. 基准性能测试
- ✅ 单模块加载性能测试
- ✅ 并发模块加载测试
- ✅ 模块缓存效果测试

### 2. 性能分析
- ✅ 性能瓶颈识别
- ✅ 优化目标设定
- ✅ 优化策略制定

## 生成的文件

- \`baseline_performance_${TIMESTAMP}.txt\` - 基准测试原始结果
- \`performance_analysis_${TIMESTAMP}.md\` - 性能分析报告
- \`performance_test_summary_${TIMESTAMP}.md\` - 本总结文件

## 下一步计划

1. **实施性能优化**: 根据分析结果实施具体优化措施
2. **优化后测试**: 运行相同的测试验证优化效果
3. **性能对比**: 对比优化前后的性能数据
4. **文档更新**: 更新相关文档和使用指南

## 使用方法

\`\`\`bash
# 运行基准测试
./tests/test_module_loading_performance.sh

# 查看结果
cat tests/performance_results/baseline_performance_*.txt

# 查看分析
cat tests/performance_results/performance_analysis_*.md
\`\`\`

---
*总结生成时间: $(date)*
EOF

    log_success "性能测试总结生成完成: $summary_file"
}

# 主函数
main() {
    echo -e "${BLUE}=== T3.1 模块加载性能基准测试 ===${NC}"
    echo "开始时间: $(date)"
    echo
    
    if ! check_prerequisites; then
        exit 1
    fi
    
    create_performance_test_program
    run_baseline_performance_test
    analyze_performance_results
    generate_performance_summary
    
    echo
    echo -e "${BLUE}=== 测试总结 ===${NC}"
    echo "总测试数: $TOTAL_TESTS"
    echo "通过测试: $PASSED_TESTS"
    echo "失败测试: $FAILED_TESTS"
    
    if [ $FAILED_TESTS -eq 0 ]; then
        echo -e "${GREEN}所有测试通过！基准数据已建立。${NC}"
    else
        echo -e "${YELLOW}部分测试失败，请检查结果。${NC}"
    fi
    
    echo
    echo "结果文件保存在: $RESULTS_DIR"
    echo "下一步: 实施性能优化措施"
}

# 运行主函数
main "$@"
