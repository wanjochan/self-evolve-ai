/**
 * array_member_access_test.c - 测试数组访问和结构体/联合体成员访问的解析和表示
 */

#include <stdio.h>

// 结构体定义
struct Point {
    int x;
    int y;
};

// 嵌套结构体
struct Rectangle {
    struct Point top_left;
    struct Point bottom_right;
};

// 联合体定义
union Value {
    int i;
    float f;
    char c;
};

// 数组变量
int numbers[10];
char string[20];
struct Point points[5];

// 二维数组
int matrix[3][3];

// 结构体数组
struct Point path[100];

// 测试数组访问
void test_array_access() {
    // 简单数组访问
    numbers[0] = 42;
    string[5] = 'A';
    
    // 多维数组访问
    matrix[1][2] = 10;
    
    // 结构体数组访问
    points[2].x = 10;
    points[2].y = 20;
    
    // 表达式作为索引
    int i = 5;
    numbers[i + 1] = 100;
    
    // 多级访问
    path[i].x = matrix[0][1];
}

// 测试结构体成员访问
void test_struct_access() {
    struct Point p;
    struct Rectangle rect;
    
    // 简单成员访问
    p.x = 10;
    p.y = 20;
    
    // 嵌套成员访问
    rect.top_left.x = 0;
    rect.top_left.y = 0;
    rect.bottom_right.x = 100;
    rect.bottom_right.y = 100;
    
    // 结构体赋值
    rect.top_left = p;
}

// 测试联合体成员访问
void test_union_access() {
    union Value val;
    
    // 不同成员的访问
    val.i = 42;
    val.f = 3.14;
    val.c = 'X';
}

// 测试指针和成员访问结合
void test_pointer_member_access() {
    struct Point *p_ptr = &points[0];
    struct Rectangle *rect_ptr = NULL;
    
    // 通过指针访问成员
    p_ptr->x = 5;
    p_ptr->y = 10;
    
    // 多级指针访问
    if (rect_ptr != NULL) {
        rect_ptr->top_left.x = 0;
    }
    
    // 数组、指针和成员访问组合
    (p_ptr + 1)->x = 15;
}

// 测试复杂表达式中的成员访问
void test_complex_access() {
    struct Rectangle rects[5];
    struct Rectangle *rect_ptr = &rects[0];
    
    // 复杂组合访问
    rects[1].top_left.x = rect_ptr->bottom_right.y;
    
    // 函数返回结构体的成员访问
    // get_rectangle().top_left.x = 10;
    
    // 条件表达式中的成员访问
    int idx = 0;
    (idx > 0 ? rects[0] : rects[1]).top_left.x = 20;
}

int main() {
    test_array_access();
    test_struct_access();
    test_union_access();
    test_pointer_member_access();
    test_complex_access();
    
    printf("Array and member access tests completed.\n");
    return 0;
} 