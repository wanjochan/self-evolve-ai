/**
 * test4.c - 复杂特性测试，包含更多C语言特性
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 复杂的结构体定义
typedef struct Node {
    int data;
    char *name;
    struct Node *next;
} Node;

// 枚举类型
typedef enum Color {
    RED,
    GREEN,
    BLUE,
    YELLOW = 10,
    PURPLE,
    WHITE
} Color;

// 使用函数指针
typedef int (*CompareFunc)(const void*, const void*);

// 宏定义（预处理）
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define SQUARE(x) ((x) * (x))
#define PI 3.14159265359

// 全局变量
int global_var = 42;
const double g_pi = PI;

// 函数前置声明
Node* create_node(int data, const char* name);
void free_node(Node* node);
int compare_nodes(const void* a, const void* b);

// 带默认参数的函数实现
Node* create_node(int data, const char* name) {
    Node* node = (Node*)malloc(sizeof(Node));
    if (node) {
        node->data = data;
        node->name = name ? strdup(name) : NULL;
        node->next = NULL;
    }
    return node;
}

// 递归函数
void free_node_list(Node* head) {
    if (head) {
        free_node_list(head->next);
        free_node(head);
    }
}

void free_node(Node* node) {
    if (node) {
        if (node->name) free(node->name);
        free(node);
    }
}

// 使用函数指针的函数
int compare_nodes(const void* a, const void* b) {
    const Node* node_a = *(const Node**)a;
    const Node* node_b = *(const Node**)b;
    return node_a->data - node_b->data;
}

// 复杂控制流
void process_colors(Color* colors, int count) {
    int red_count = 0, green_count = 0, other_count = 0;
    
    for (int i = 0; i < count; i++) {
        switch (colors[i]) {
            case RED:
                red_count++;
                break;
            case GREEN:
                green_count++;
                break;
            default:
                other_count++;
                break;
        }
    }
    
    printf("Colors: %d red, %d green, %d other\n", red_count, green_count, other_count);
}

// 使用变长数组（C99特性）
double calculate_average(int count, int numbers[count]) {
    double sum = 0.0;
    for (int i = 0; i < count; i++) {
        sum += numbers[i];
    }
    return sum / count;
}

// 使用goto语句
int find_in_matrix(int rows, int cols, int matrix[rows][cols], int target) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (matrix[i][j] == target) {
                goto found;
            }
        }
    }
    return -1; // 未找到
    
found:
    return 1; // 找到
}

// 位运算
unsigned int set_bit(unsigned int value, int bit_position) {
    return value | (1u << bit_position);
}

unsigned int clear_bit(unsigned int value, int bit_position) {
    return value & ~(1u << bit_position);
}

unsigned int toggle_bit(unsigned int value, int bit_position) {
    return value ^ (1u << bit_position);
}

// 主函数
int main() {
    // 创建链表
    Node* head = create_node(100, "Head");
    head->next = create_node(50, "Middle");
    head->next->next = create_node(200, "Tail");
    
    // 使用数组
    Node* nodes[3] = { head, head->next, head->next->next };
    
    // 排序（使用函数指针）
    qsort(nodes, 3, sizeof(Node*), compare_nodes);
    
    // 输出排序后的结果
    for (int i = 0; i < 3; i++) {
        printf("Node %d: %s (data: %d)\n", i+1, nodes[i]->name, nodes[i]->data);
    }
    
    // 测试枚举
    Color colors[5] = {RED, GREEN, BLUE, YELLOW, PURPLE};
    process_colors(colors, 5);
    
    // 测试可变长数组
    int scores[] = {85, 92, 78, 95, 88};
    double avg = calculate_average(5, scores);
    printf("Average score: %.2f\n", avg);
    
    // 测试位运算
    unsigned int flags = 0;
    flags = set_bit(flags, 3);    // 设置第3位
    flags = set_bit(flags, 5);    // 设置第5位
    flags = toggle_bit(flags, 3); // 切换第3位
    
    printf("Flags: %u\n", flags);
    
    // 清理资源
    free_node_list(head);
    
    return 0;
} 