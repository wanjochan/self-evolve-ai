/**
 * array_types_test.c - 测试数组类型的解析和表示
 */

#include <stdio.h>

// 测试基本数组声明
int array1[10];
float array2[5];

// 测试多维数组
int matrix[3][3];
char cube[2][2][2];

// 测试数组初始化
int initialized_array[3] = {1, 2, 3};
char string_array[] = "Hello";

// 测试函数参数中的数组
void process_array(int arr[], int size);
void process_2d_array(int arr[][10], int rows);

// 测试函数实现
void process_array(int arr[], int size) {
    int i;
    for (i = 0; i < size; i++) {
        arr[i] *= 2;
    }
}

// 测试函数中的局部数组变量
void test_local_arrays() {
    int local_array[5];
    char local_string[20];
    
    int i;
    for (i = 0; i < 5; i++) {
        local_array[i] = i;
    }
}

// 测试数组作为返回值（C99不允许直接返回数组，但可以返回指向数组的指针）
int* get_array() {
    static int result[10];
    int i;
    for (i = 0; i < 10; i++) {
        result[i] = i * i;
    }
    return result;
}

int main() {
    // 测试数组访问
    int i, j;
    for (i = 0; i < 3; i++) {
        initialized_array[i] = i * 10;
    }
    
    // 测试多维数组访问
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            matrix[i][j] = i + j;
        }
    }
    
    // 测试数组作为函数参数
    process_array(initialized_array, 3);
    
    // 测试局部数组
    test_local_arrays();
    
    // 测试获取数组
    int* result = get_array();
    for (i = 0; i < 10; i++) {
        printf("%d ", result[i]);
    }
    printf("\n");
    
    return 0;
} 