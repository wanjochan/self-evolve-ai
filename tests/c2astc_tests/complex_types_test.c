/**
 * complex_types_test.c - 复杂类型测试用例
 * 用于测试c2astc模块的结构体、联合体和枚举解析功能
 */

#include <stdio.h>
#include <stdlib.h>

// 枚举定义
enum Color {
    RED,
    GREEN,
    BLUE,
    YELLOW = 10,
    PURPLE
};

// 结构体定义
struct Point {
    int x;
    int y;
};

// 嵌套结构体
struct Rectangle {
    struct Point top_left;
    struct Point bottom_right;
    enum Color color;
};

// 联合体定义
union Value {
    int i;
    float f;
    char c;
};

// 匿名结构体
struct {
    int id;
    char name[50];
} anonymous_struct;

// 结构体包含联合体
struct ComplexData {
    int type;
    union {
        int i;
        float f;
        char *s;
    } data;
};

// 使用结构体的函数
void print_point(struct Point p) {
    printf("Point(%d, %d)\n", p.x, p.y);
}

// 使用联合体的函数
void print_value(union Value v, int type) {
    switch (type) {
        case 0:
            printf("Integer: %d\n", v.i);
            break;
        case 1:
            printf("Float: %f\n", v.f);
            break;
        case 2:
            printf("Char: %c\n", v.c);
            break;
    }
}

// 使用枚举的函数
void print_color(enum Color c) {
    switch (c) {
        case RED:
            printf("Red\n");
            break;
        case GREEN:
            printf("Green\n");
            break;
        case BLUE:
            printf("Blue\n");
            break;
        case YELLOW:
            printf("Yellow\n");
            break;
        case PURPLE:
            printf("Purple\n");
            break;
    }
}

int main() {
    // 使用结构体
    struct Point p1 = {10, 20};
    struct Point p2 = {30, 40};
    
    // 使用嵌套结构体
    struct Rectangle rect = {
        .top_left = p1,
        .bottom_right = p2,
        .color = BLUE
    };
    
    // 使用联合体
    union Value v;
    v.i = 42;
    print_value(v, 0);
    
    v.f = 3.14f;
    print_value(v, 1);
    
    v.c = 'A';
    print_value(v, 2);
    
    // 使用枚举
    enum Color c = GREEN;
    print_color(c);
    
    // 使用匿名结构体
    anonymous_struct.id = 1;
    
    // 使用复杂数据结构
    struct ComplexData cd;
    cd.type = 0;
    cd.data.i = 100;
    
    return 0;
} 