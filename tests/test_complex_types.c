// Test complex types in C99 compiler
#include <stdio.h>

// 结构体定义
struct Point {
    int x;
    int y;
};

// 联合体定义
union Data {
    int i;
    float f;
    char str[20];
};

// 函数声明
struct Point create_point(int x, int y);
void print_point(struct Point p);

int main() {
    // 基本变量
    int a = 10;
    float b = 3.14f;
    
    // 指针变量
    int* ptr = &a;
    printf("Value of a: %d, via pointer: %d\n", a, *ptr);
    
    // 数组
    int arr[5] = {1, 2, 3, 4, 5};
    printf("Array elements: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
    
    // 结构体
    struct Point p1;
    p1.x = 10;
    p1.y = 20;

    printf("Point p1: (%d, %d)\n", p1.x, p1.y);
    
    // 联合体
    union Data data;
    data.i = 42;
    printf("Union as int: %d\n", data.i);
    
    data.f = 3.14f;
    printf("Union as float: %f\n", data.f);
    
    // 指针算术
    int* arr_ptr = arr;
    printf("Pointer arithmetic: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", *(arr_ptr + i));
    }
    printf("\n");
    
    // 复合赋值运算符
    a += 5;
    printf("After a += 5: %d\n", a);
    
    ptr++;
    printf("After ptr++, value: %d\n", *ptr);
    
    return 0;
}

struct Point create_point(int x, int y) {
    struct Point p;
    p.x = x;
    p.y = y;
    return p;
}

void print_point(struct Point p) {
    printf("Point: (%d, %d)\n", p.x, p.y);
}
