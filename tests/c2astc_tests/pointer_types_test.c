/**
 * pointer_types_test.c - 指针类型测试用例
 * 用于测试c2astc模块的指针类型解析功能
 */

#include <stdio.h>
#include <stdlib.h>

// 基本指针类型
int* global_ptr;
char** string_array;
void* generic_ptr;
const int* const_int_ptr;
int* const const_ptr;
const int* const const_int_const_ptr;

// 多级指针
int*** triple_ptr;
void**** quad_ptr;

// 函数指针
int (*func_ptr)(int, int);
void (*callback)(void*);
int* (*get_array)(int size);
void (*signal_handlers[10])(int);

// 结构体指针
struct Point {
    int x;
    int y;
};

struct Point* point_ptr;
struct Point** point_ptr_array;

// 联合体指针
union Value {
    int i;
    float f;
    char* s;
};

union Value* value_ptr;

// 指针数组
int* int_ptr_array[10];
char** string_ptr_array[5];

// 指针作为函数参数
void process_data(int* data, int size) {
    for (int i = 0; i < size; i++) {
        printf("%d ", data[i]);
    }
    printf("\n");
}

// 返回指针的函数
int* create_array(int size) {
    int* arr = (int*)malloc(size * sizeof(int));
    for (int i = 0; i < size; i++) {
        arr[i] = i * 10;
    }
    return arr;
}

// 使用指针的函数
void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

// 指针算术运算
void pointer_arithmetic(int* arr, int size) {
    int* p = arr;
    for (int i = 0; i < size; i++) {
        printf("%d ", *p);
        p++;
    }
    printf("\n");
}

int main() {
    // 局部指针变量
    int local_var = 42;
    int* ptr = &local_var;
    
    // 指针数组
    int numbers[5] = {10, 20, 30, 40, 50};
    int* num_ptr = numbers;
    
    // 字符串指针
    char* message = "Hello, World!";
    
    // 多级指针使用
    int value = 100;
    int* p1 = &value;
    int** p2 = &p1;
    int*** p3 = &p2;
    
    // 函数指针使用
    func_ptr = &swap;  // 注意：这里有类型不匹配，仅作示例
    
    // 结构体指针
    struct Point point = {10, 20};
    point_ptr = &point;
    
    // 指针运算
    pointer_arithmetic(numbers, 5);
    
    // 创建和释放动态内存
    int* dynamic_array = create_array(10);
    free(dynamic_array);
    
    return 0;
} 