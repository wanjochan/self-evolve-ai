/**
 * function_pointer_test.c - 测试函数指针类型的解析和表示
 */

#include <stdio.h>

// 基本函数指针类型定义
typedef int (*FuncPtr)(int, int);
typedef void (*CallbackFunc)(void *data);

// 函数指针变量声明
int (*operation)(int, int);
void (*callback)(void *);

// 返回函数指针的函数
FuncPtr get_operation(char op);

// 接受函数指针参数的函数
void apply_operation(int a, int b, int (*op)(int, int));

// 示例函数，用于赋值给函数指针
int add(int a, int b) {
    return a + b;
}

int subtract(int a, int b) {
    return a - b;
}

int multiply(int a, int b) {
    return a * b;
}

int divide(int a, int b) {
    if (b != 0) {
        return a / b;
    }
    return 0;
}

// 函数指针数组
int (*operations[4])(int, int) = {add, subtract, multiply, divide};

// 返回函数指针的函数实现
FuncPtr get_operation(char op) {
    switch (op) {
        case '+': return add;
        case '-': return subtract;
        case '*': return multiply;
        case '/': return divide;
        default: return NULL;
    }
}

// 接受函数指针参数的函数实现
void apply_operation(int a, int b, int (*op)(int, int)) {
    printf("结果: %d\n", op(a, b));
}

// 回调函数示例
void print_data(void *data) {
    printf("%s\n", (char*)data);
}

// 函数指针作为结构体成员
struct Handler {
    void (*handle)(void*);
    void *data;
};

int main() {
    // 使用函数指针
    operation = add;
    printf("1 + 2 = %d\n", operation(1, 2));
    
    operation = subtract;
    printf("5 - 3 = %d\n", operation(5, 3));
    
    // 使用函数指针数组
    printf("10 * 2 = %d\n", operations[2](10, 2));
    
    // 使用返回函数指针的函数
    FuncPtr op = get_operation('+');
    printf("3 + 4 = %d\n", op(3, 4));
    
    // 使用接受函数指针参数的函数
    apply_operation(8, 2, divide);
    
    // 使用回调函数
    callback = print_data;
    callback("Hello, Function Pointer!");
    
    // 使用函数指针作为结构体成员
    struct Handler h;
    h.handle = print_data;
    h.data = "Hello from struct!";
    h.handle(h.data);
    
    return 0;
} 