// 综合语义分析测试文件
// 测试C99语义分析器的各种功能

#include <stdio.h>

// 全局变量声明测试
int global_var = 42;
const int const_var = 100;
float pi = 3.14159;
char greeting[] = "Hello, World!";

// 结构体声明测试
struct Point {
    int x;
    int y;
};

struct Rectangle {
    struct Point top_left;
    struct Point bottom_right;
};

// 函数声明测试
int add(int a, int b);
float calculate_area(float width, float height);
void print_point(struct Point p);

// 主函数 - 测试各种语义检查
int main() {
    // 变量声明和初始化测试
    int local_var = 10;
    float result;
    struct Point origin = {0, 0};
    struct Point *ptr_point;
    
    // 数组声明测试
    int numbers[10];
    char buffer[256];
    
    // 表达式类型检查测试
    result = local_var + pi;                    // 算术运算
    local_var += 5;                             // 复合赋值
    local_var++;                                // 后缀递增
    ++local_var;                                // 前缀递增
    
    // 指针运算测试
    ptr_point = &origin;                        // 取地址
    int x_coord = ptr_point->x;                 // 指针成员访问
    int y_coord = (*ptr_point).y;               // 解引用成员访问
    
    // 数组访问测试
    numbers[0] = local_var;
    numbers[1] = numbers[0] + 1;
    
    // 控制流语句测试
    if (local_var > 0) {
        printf("Positive number\n");
    } else {
        printf("Non-positive number\n");
    }
    
    // 循环测试
    for (int i = 0; i < 10; i++) {
        numbers[i] = i * i;
    }
    
    int j = 0;
    while (j < 5) {
        buffer[j] = 'A' + j;
        j++;
    }
    
    // switch语句测试
    switch (local_var % 3) {
        case 0:
            printf("Divisible by 3\n");
            break;
        case 1:
            printf("Remainder 1\n");
            break;
        case 2:
            printf("Remainder 2\n");
            break;
        default:
            printf("Unexpected\n");
            break;
    }
    
    // 函数调用测试
    int sum = add(local_var, global_var);
    float area = calculate_area(10.5, 20.3);
    print_point(origin);
    
    // 三元运算符测试
    int max_val = (local_var > global_var) ? local_var : global_var;
    
    // 逗号运算符测试
    int a, b, c;
    c = (a = 1, b = 2, a + b);
    
    // 类型转换测试
    float float_val = (float)local_var;
    int int_val = (int)pi;
    
    // sizeof运算符测试
    size_t size_of_int = sizeof(int);
    size_t size_of_struct = sizeof(struct Point);
    
    return 0;
}

// 函数定义测试
int add(int a, int b) {
    return a + b;
}

float calculate_area(float width, float height) {
    if (width <= 0 || height <= 0) {
        return 0.0;
    }
    return width * height;
}

void print_point(struct Point p) {
    printf("Point: (%d, %d)\n", p.x, p.y);
}

// 递归函数测试
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

// 指针函数测试
int* find_max(int* arr, int size) {
    if (!arr || size <= 0) {
        return NULL;
    }
    
    int* max_ptr = arr;
    for (int i = 1; i < size; i++) {
        if (arr[i] > *max_ptr) {
            max_ptr = &arr[i];
        }
    }
    
    return max_ptr;
}
