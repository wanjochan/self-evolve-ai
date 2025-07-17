#!/bin/bash
#
# test_stress_module_system.sh - 模块系统压力测试
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# 测试统计
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

print_test_result() {
    local test_name="$1"
    local result="$2"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    if [ "$result" = "PASS" ]; then
        echo -e "${GREEN}✅ PASS${NC}: $test_name"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}❌ FAIL${NC}: $test_name"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
}

# 压力测试1: 大量模块加载
test_massive_module_loading() {
    echo -e "${BLUE}=== 压力测试1: 大量模块加载 ===${NC}"
    
    # 创建压力测试程序
    cat > "$PROJECT_ROOT/stress_load_test.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <time.h>
#include <sys/time.h>

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

int main() {
    const char* modules[] = {"layer0", "pipeline", "compiler", "libc", "module"};
    const int module_count = 5;
    const int iterations = 100;
    
    printf("开始大量模块加载测试 (%d次迭代)...\n", iterations);
    
    double start_time = get_time();
    int success_count = 0;
    
    for (int i = 0; i < iterations; i++) {
        for (int j = 0; j < module_count; j++) {
            char module_path[256];
            snprintf(module_path, sizeof(module_path), "./bin/layer2/%s.so", modules[j]);
            
            void* handle = dlopen(module_path, RTLD_LAZY);
            if (handle) {
                success_count++;
                dlclose(handle);
            }
        }
        
        if (i % 20 == 0) {
            printf("完成 %d/%d 次迭代\n", i, iterations);
        }
    }
    
    double end_time = get_time();
    double total_time = end_time - start_time;
    
    printf("测试完成:\n");
    printf("  总加载次数: %d\n", iterations * module_count);
    printf("  成功次数: %d\n", success_count);
    printf("  总时间: %.3f秒\n", total_time);
    printf("  平均每次加载: %.3f毫秒\n", (total_time * 1000) / (iterations * module_count));
    
    return (success_count == iterations * module_count) ? 0 : 1;
}
EOF

    # 编译并运行测试
    if gcc "$PROJECT_ROOT/stress_load_test.c" -o "$PROJECT_ROOT/stress_load_test" -ldl; then
        if "$PROJECT_ROOT/stress_load_test"; then
            print_test_result "大量模块加载测试" "PASS"
        else
            print_test_result "大量模块加载测试" "FAIL"
        fi
    else
        print_test_result "大量模块加载测试编译" "FAIL"
    fi
    
    rm -f "$PROJECT_ROOT/stress_load_test.c" "$PROJECT_ROOT/stress_load_test"
}

# 压力测试2: 并发模块访问
test_concurrent_module_access() {
    echo -e "${BLUE}=== 压力测试2: 并发模块访问 ===${NC}"
    
    # 创建并发测试程序
    cat > "$PROJECT_ROOT/concurrent_test.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 4
#define OPERATIONS_PER_THREAD 50

typedef struct {
    int thread_id;
    int success_count;
} ThreadData;

void* thread_worker(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    const char* modules[] = {"layer0", "pipeline", "compiler"};
    const int module_count = 3;
    
    for (int i = 0; i < OPERATIONS_PER_THREAD; i++) {
        for (int j = 0; j < module_count; j++) {
            char module_path[256];
            snprintf(module_path, sizeof(module_path), "./bin/layer2/%s.so", modules[j]);
            
            void* handle = dlopen(module_path, RTLD_LAZY);
            if (handle) {
                data->success_count++;
                usleep(1000);  // 1ms延迟
                dlclose(handle);
            }
        }
    }
    
    return NULL;
}

int main() {
    printf("开始并发模块访问测试 (%d线程, 每线程%d操作)...\n", NUM_THREADS, OPERATIONS_PER_THREAD);
    
    pthread_t threads[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];
    
    // 创建线程
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].success_count = 0;
        
        if (pthread_create(&threads[i], NULL, thread_worker, &thread_data[i]) != 0) {
            printf("线程创建失败\n");
            return 1;
        }
    }
    
    // 等待线程完成
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // 统计结果
    int total_success = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        printf("线程 %d: %d 次成功\n", i, thread_data[i].success_count);
        total_success += thread_data[i].success_count;
    }
    
    int expected_total = NUM_THREADS * OPERATIONS_PER_THREAD * 3;  // 3个模块
    printf("总成功次数: %d/%d\n", total_success, expected_total);
    
    return (total_success >= expected_total * 0.95) ? 0 : 1;  // 95%成功率
}
EOF

    # 编译并运行测试
    if gcc "$PROJECT_ROOT/concurrent_test.c" -o "$PROJECT_ROOT/concurrent_test" -ldl -lpthread; then
        if "$PROJECT_ROOT/concurrent_test"; then
            print_test_result "并发模块访问测试" "PASS"
        else
            print_test_result "并发模块访问测试" "FAIL"
        fi
    else
        print_test_result "并发模块访问测试编译" "FAIL"
    fi
    
    rm -f "$PROJECT_ROOT/concurrent_test.c" "$PROJECT_ROOT/concurrent_test"
}

# 压力测试3: 内存压力测试
test_memory_pressure() {
    echo -e "${BLUE}=== 压力测试3: 内存压力测试 ===${NC}"
    
    # 创建内存压力测试程序
    cat > "$PROJECT_ROOT/memory_pressure_test.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>

int main() {
    printf("开始内存压力测试...\n");
    
    const int max_handles = 1000;
    void* handles[max_handles];
    int loaded_count = 0;
    
    // 记录初始内存状态
    system("echo '初始内存状态:' && ps -o pid,vsz,rss -p $$ | tail -1");
    
    // 大量加载模块（不释放）
    for (int i = 0; i < max_handles; i++) {
        void* handle = dlopen("./bin/layer2/layer0.so", RTLD_LAZY);
        if (handle) {
            handles[loaded_count] = handle;
            loaded_count++;
        } else {
            break;
        }
        
        if (i % 100 == 0) {
            printf("已加载 %d 个模块句柄\n", loaded_count);
        }
    }
    
    printf("最大加载数量: %d\n", loaded_count);
    
    // 记录峰值内存状态
    system("echo '峰值内存状态:' && ps -o pid,vsz,rss -p $$ | tail -1");
    
    // 释放所有句柄
    for (int i = 0; i < loaded_count; i++) {
        dlclose(handles[i]);
    }
    
    // 记录释放后内存状态
    system("echo '释放后内存状态:' && ps -o pid,vsz,rss -p $$ | tail -1");
    
    printf("内存压力测试完成\n");
    return (loaded_count > 100) ? 0 : 1;  // 至少能加载100个句柄
}
EOF

    # 编译并运行测试
    if gcc "$PROJECT_ROOT/memory_pressure_test.c" -o "$PROJECT_ROOT/memory_pressure_test" -ldl; then
        if "$PROJECT_ROOT/memory_pressure_test"; then
            print_test_result "内存压力测试" "PASS"
        else
            print_test_result "内存压力测试" "FAIL"
        fi
    else
        print_test_result "内存压力测试编译" "FAIL"
    fi
    
    rm -f "$PROJECT_ROOT/memory_pressure_test.c" "$PROJECT_ROOT/memory_pressure_test"
}

# 压力测试4: 长时间运行测试
test_long_running() {
    echo -e "${BLUE}=== 压力测试4: 长时间运行测试 ===${NC}"
    
    # 创建长时间运行测试程序
    cat > "$PROJECT_ROOT/long_running_test.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <time.h>
#include <unistd.h>

int main() {
    printf("开始长时间运行测试 (30秒)...\n");
    
    time_t start_time = time(NULL);
    time_t end_time = start_time + 30;  // 30秒测试
    
    int cycle_count = 0;
    int error_count = 0;
    
    while (time(NULL) < end_time) {
        // 加载和卸载模块
        void* handle = dlopen("./bin/layer2/layer0.so", RTLD_LAZY);
        if (handle) {
            dlclose(handle);
        } else {
            error_count++;
        }
        
        cycle_count++;
        
        if (cycle_count % 1000 == 0) {
            printf("完成 %d 个周期, 错误: %d\n", cycle_count, error_count);
        }
        
        usleep(1000);  // 1ms延迟
    }
    
    printf("长时间运行测试完成:\n");
    printf("  总周期数: %d\n", cycle_count);
    printf("  错误次数: %d\n", error_count);
    printf("  错误率: %.2f%%\n", (double)error_count / cycle_count * 100);
    
    return (error_count < cycle_count * 0.01) ? 0 : 1;  // 错误率<1%
}
EOF

    # 编译并运行测试
    if gcc "$PROJECT_ROOT/long_running_test.c" -o "$PROJECT_ROOT/long_running_test" -ldl; then
        if timeout 35 "$PROJECT_ROOT/long_running_test"; then
            print_test_result "长时间运行测试" "PASS"
        else
            print_test_result "长时间运行测试" "FAIL"
        fi
    else
        print_test_result "长时间运行测试编译" "FAIL"
    fi
    
    rm -f "$PROJECT_ROOT/long_running_test.c" "$PROJECT_ROOT/long_running_test"
}

# 主函数
main() {
    echo -e "${BLUE}=== 模块系统压力测试套件 ===${NC}"
    echo "测试目标: 验证模块系统在高负载下的稳定性和性能"
    echo
    
    # 确保模块系统已构建
    if [ ! -d "$PROJECT_ROOT/bin/layer2" ]; then
        echo -e "${RED}错误: 模块系统未构建${NC}"
        echo "请先运行: ./build_modules_gcc.sh"
        exit 1
    fi
    
    cd "$PROJECT_ROOT"
    
    # 运行所有压力测试
    test_massive_module_loading
    echo
    test_concurrent_module_access
    echo
    test_memory_pressure
    echo
    test_long_running
    
    # 输出测试结果
    echo
    echo -e "${BLUE}=== 压力测试结果统计 ===${NC}"
    echo "总测试数: $TOTAL_TESTS"
    echo -e "通过: ${GREEN}$PASSED_TESTS${NC}"
    echo -e "失败: ${RED}$FAILED_TESTS${NC}"
    
    if [ $FAILED_TESTS -eq 0 ]; then
        echo -e "${GREEN}🎉 所有压力测试通过！模块系统稳定性优秀。${NC}"
        exit 0
    else
        echo -e "${RED}⚠️  有 $FAILED_TESTS 个压力测试失败。${NC}"
        exit 1
    fi
}

# 运行主函数
main "$@"
