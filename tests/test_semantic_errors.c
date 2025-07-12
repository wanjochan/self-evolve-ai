// 语义错误测试文件
// 用于测试语义分析器的错误检测能力

#include <stdio.h>

// 错误1: 使用关键字作为变量名
// int int = 5;  // 应该报错

// 错误2: void类型变量
// void invalid_var;  // 应该报错

// 错误3: 重复声明
int duplicate_var;
// int duplicate_var;  // 应该报错

// 错误4: const变量未初始化
// const int uninitialized_const;  // 应该报错

// 错误5: 数组元素为void类型
// void invalid_array[10];  // 应该报错

// 错误6: 数组元素为函数类型
// int func()[10];  // 应该报错

int main() {
    int a = 10;
    int b = 0;
    float f = 3.14;
    int* ptr;
    
    // 错误7: 赋值给非左值
    // 10 = a;  // 应该报错
    
    // 错误8: 除零错误
    // int result = a / 0;  // 应该报错
    
    // 错误9: 模零错误
    // int remainder = a % 0;  // 应该报错
    
    // 错误10: 对非指针类型解引用
    // int value = *a;  // 应该报错
    
    // 错误11: 对非标量类型取地址后解引用
    // int value = &a + f;  // 类型不匹配
    
    // 错误12: 数组访问非整数索引
    int arr[10];
    // int value = arr[f];  // 应该报错
    
    // 错误13: 对非数组/指针类型进行数组访问
    // int value = a[0];  // 应该报错
    
    // 错误14: 对非结构体/联合体进行成员访问
    // int value = a.member;  // 应该报错
    
    // 错误15: 递增递减非标量类型
    // arr++;  // 应该报错
    
    // 错误16: 递增递减非左值
    // (a + b)++;  // 应该报错
    
    // 错误17: 逻辑运算符操作数非标量类型
    // int result = arr && a;  // 应该报错
    
    // 错误18: 位运算符操作数非整数类型
    // int result = a & f;  // 应该报错
    
    // 错误19: 模运算操作数非整数类型
    // float result = f % a;  // 应该报错
    
    // 错误20: if条件非标量类型
    // if (arr) { }  // 应该报错
    
    // 错误21: while条件非标量类型
    // while (arr) { }  // 应该报错
    
    // 错误22: for条件非标量类型
    // for (int i = 0; arr; i++) { }  // 应该报错
    
    // 错误23: switch表达式非整数类型
    // switch (f) { }  // 应该报错
    
    // 错误24: break语句在循环/switch外
    // break;  // 应该报错
    
    // 错误25: continue语句在循环外
    // continue;  // 应该报错
    
    // 错误26: return语句类型不匹配
    // return f;  // main函数返回int，这里返回float
    
    // 错误27: 函数调用参数数量不匹配
    // printf();  // printf需要至少一个参数
    
    // 错误28: 函数调用参数类型不匹配
    // printf(123);  // printf第一个参数应该是字符串
    
    // 错误29: 调用未声明的函数
    // undefined_function();  // 应该报错
    
    // 错误30: 使用未声明的变量
    // int value = undefined_var;  // 应该报错
    
    return 0;
}

// 错误31: 函数返回类型不匹配
int wrong_return_type() {
    float f = 3.14;
    // return f;  // 应该警告或报错
    return 0;
}

// 错误32: void函数有返回值
void void_function_with_return() {
    // return 42;  // 应该报错
}

// 错误33: 非void函数缺少返回值
int missing_return_function() {
    int a = 10;
    // 缺少return语句，应该警告
}

// 错误34: 函数参数重复声明
// int duplicate_param_function(int a, int a) {  // 应该报错
//     return a;
// }

// 错误35: 结构体成员重复声明
// struct InvalidStruct {
//     int member;
//     int member;  // 应该报错
// };
