#!/bin/bash

# C99编译器性能测试脚本
# 测试编译速度、内存使用和生成代码质量

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
TINYCC_COMPILER="tcc"
RESULTS_DIR="$TEST_DIR/performance_results"

# 创建结果目录
mkdir -p "$RESULTS_DIR"

echo -e "${BLUE}=== C99编译器性能测试 ===${NC}"
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

# 检查TinyCC是否可用
if ! command -v "$TINYCC_COMPILER" &> /dev/null; then
    echo -e "${YELLOW}警告: TinyCC不可用，跳过性能对比测试${NC}"
    TINYCC_AVAILABLE=false
else
    TINYCC_AVAILABLE=true
    echo -e "${GREEN}TinyCC可用，将进行性能对比${NC}"
fi

echo

# 测试用例文件
TEST_FILES=(
    "$TEST_DIR/test_minimal.c"
    "$TEST_DIR/test_semantic_comprehensive.c"
    "$TEST_DIR/test_simple_if.c"
)

# 创建额外的测试文件
create_test_files() {
    echo -e "${BLUE}创建性能测试文件...${NC}"
    
    # 创建小型测试文件
    cat > "$TEST_DIR/perf_small.c" << 'EOF'
#include <stdio.h>

int main() {
    int sum = 0;
    for (int i = 0; i < 100; i++) {
        sum += i;
    }
    printf("Sum: %d\n", sum);
    return 0;
}
EOF

    # 创建中型测试文件
    cat > "$TEST_DIR/perf_medium.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>

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
    printf("Rectangle: (%d,%d) to (%d,%d), Area: %d\n",
           rect.top_left.x, rect.top_left.y,
           rect.bottom_right.x, rect.bottom_right.y,
           calculate_area(rect));
}

int main() {
    struct Rectangle rectangles[10];
    
    for (int i = 0; i < 10; i++) {
        rectangles[i].top_left.x = i;
        rectangles[i].top_left.y = i;
        rectangles[i].bottom_right.x = i + 10;
        rectangles[i].bottom_right.y = i + 10;
    }
    
    int total_area = 0;
    for (int i = 0; i < 10; i++) {
        print_rectangle(rectangles[i]);
        total_area += calculate_area(rectangles[i]);
    }
    
    printf("Total area: %d\n", total_area);
    return 0;
}
EOF

    # 创建大型测试文件
    cat > "$TEST_DIR/perf_large.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ITEMS 1000
#define MAX_NAME_LEN 64

typedef struct {
    int id;
    char name[MAX_NAME_LEN];
    float price;
    int quantity;
} Item;

typedef struct {
    Item items[MAX_ITEMS];
    int count;
} Inventory;

void init_inventory(Inventory* inv) {
    inv->count = 0;
    memset(inv->items, 0, sizeof(inv->items));
}

int add_item(Inventory* inv, int id, const char* name, float price, int quantity) {
    if (inv->count >= MAX_ITEMS) {
        return -1;
    }
    
    Item* item = &inv->items[inv->count];
    item->id = id;
    strncpy(item->name, name, MAX_NAME_LEN - 1);
    item->name[MAX_NAME_LEN - 1] = '\0';
    item->price = price;
    item->quantity = quantity;
    
    inv->count++;
    return 0;
}

Item* find_item(Inventory* inv, int id) {
    for (int i = 0; i < inv->count; i++) {
        if (inv->items[i].id == id) {
            return &inv->items[i];
        }
    }
    return NULL;
}

float calculate_total_value(Inventory* inv) {
    float total = 0.0;
    for (int i = 0; i < inv->count; i++) {
        total += inv->items[i].price * inv->items[i].quantity;
    }
    return total;
}

void print_inventory(Inventory* inv) {
    printf("Inventory (%d items):\n", inv->count);
    for (int i = 0; i < inv->count; i++) {
        Item* item = &inv->items[i];
        printf("  %d: %s - $%.2f x %d = $%.2f\n",
               item->id, item->name, item->price, item->quantity,
               item->price * item->quantity);
    }
    printf("Total value: $%.2f\n", calculate_total_value(inv));
}

int main() {
    Inventory inventory;
    init_inventory(&inventory);
    
    // 添加一些测试数据
    for (int i = 0; i < 50; i++) {
        char name[MAX_NAME_LEN];
        snprintf(name, sizeof(name), "Item_%d", i);
        add_item(&inventory, i, name, 10.0 + i * 0.5, i + 1);
    }
    
    print_inventory(&inventory);
    
    // 查找测试
    for (int i = 0; i < 10; i++) {
        Item* item = find_item(&inventory, i * 5);
        if (item) {
            printf("Found item %d: %s\n", item->id, item->name);
        }
    }
    
    return 0;
}
EOF

    TEST_FILES+=(
        "$TEST_DIR/perf_small.c"
        "$TEST_DIR/perf_medium.c"
        "$TEST_DIR/perf_large.c"
    )
    
    echo -e "${GREEN}性能测试文件创建完成${NC}"
}

# 测试编译速度
test_compilation_speed() {
    echo -e "${BLUE}=== 编译速度测试 ===${NC}"
    
    local results_file="$RESULTS_DIR/compilation_speed.txt"
    echo "编译速度测试结果 - $(date)" > "$results_file"
    echo "======================================" >> "$results_file"
    
    for test_file in "${TEST_FILES[@]}"; do
        if [ ! -f "$test_file" ]; then
            echo -e "${YELLOW}跳过不存在的文件: $test_file${NC}"
            continue
        fi
        
        local filename=$(basename "$test_file")
        local output_file="$RESULTS_DIR/${filename%.c}.astc"
        
        echo -e "${BLUE}测试文件: $filename${NC}"
        echo "文件: $filename" >> "$results_file"
        
        # 测试C99编译器
        echo -n "  C99编译器: "
        local start_time=$(date +%s.%N)
        
        if timeout 30s "$C99_COMPILER" "$test_file" -o "$output_file" 2>/dev/null; then
            local end_time=$(date +%s.%N)
            local duration=$(echo "$end_time - $start_time" | bc -l)
            echo -e "${GREEN}${duration}s${NC}"
            echo "  C99编译器: ${duration}s" >> "$results_file"
        else
            echo -e "${RED}失败或超时${NC}"
            echo "  C99编译器: 失败或超时" >> "$results_file"
        fi
        
        # 测试TinyCC（如果可用）
        if [ "$TINYCC_AVAILABLE" = true ]; then
            echo -n "  TinyCC: "
            local tcc_output="/tmp/${filename%.c}.o"
            local start_time=$(date +%s.%N)
            
            if timeout 30s "$TINYCC_COMPILER" -c "$test_file" -o "$tcc_output" 2>/dev/null; then
                local end_time=$(date +%s.%N)
                local duration=$(echo "$end_time - $start_time" | bc -l)
                echo -e "${GREEN}${duration}s${NC}"
                echo "  TinyCC: ${duration}s" >> "$results_file"
                rm -f "$tcc_output"
            else
                echo -e "${RED}失败或超时${NC}"
                echo "  TinyCC: 失败或超时" >> "$results_file"
            fi
        fi
        
        echo "" >> "$results_file"
        echo
    done
    
    echo -e "${GREEN}编译速度测试完成，结果保存到: $results_file${NC}"
}

# 测试内存使用
test_memory_usage() {
    echo -e "${BLUE}=== 内存使用测试 ===${NC}"
    
    local results_file="$RESULTS_DIR/memory_usage.txt"
    echo "内存使用测试结果 - $(date)" > "$results_file"
    echo "======================================" >> "$results_file"
    
    for test_file in "${TEST_FILES[@]}"; do
        if [ ! -f "$test_file" ]; then
            continue
        fi
        
        local filename=$(basename "$test_file")
        local output_file="$RESULTS_DIR/${filename%.c}.astc"
        
        echo -e "${BLUE}测试文件: $filename${NC}"
        echo "文件: $filename" >> "$results_file"
        
        # 使用time命令测试内存使用
        if command -v /usr/bin/time &> /dev/null; then
            echo -n "  内存使用: "
            local time_output=$(/usr/bin/time -f "%M KB" "$C99_COMPILER" "$test_file" -o "$output_file" 2>&1 | tail -1)
            if [[ $time_output =~ ([0-9]+)\ KB ]]; then
                local memory_kb=${BASH_REMATCH[1]}
                echo -e "${GREEN}${memory_kb} KB${NC}"
                echo "  内存使用: ${memory_kb} KB" >> "$results_file"
            else
                echo -e "${YELLOW}无法测量${NC}"
                echo "  内存使用: 无法测量" >> "$results_file"
            fi
        else
            echo -e "${YELLOW}time命令不可用，跳过内存测试${NC}"
            echo "  内存使用: time命令不可用" >> "$results_file"
        fi
        
        echo "" >> "$results_file"
    done
    
    echo -e "${GREEN}内存使用测试完成，结果保存到: $results_file${NC}"
}

# 主函数
main() {
    create_test_files
    test_compilation_speed
    test_memory_usage
    
    echo
    echo -e "${GREEN}=== 性能测试完成 ===${NC}"
    echo "所有结果保存在: $RESULTS_DIR"
    echo
    echo "查看结果:"
    echo "  编译速度: cat $RESULTS_DIR/compilation_speed.txt"
    echo "  内存使用: cat $RESULTS_DIR/memory_usage.txt"
}

# 运行主函数
main "$@"
