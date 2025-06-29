#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <time.h>

// 全面的C99编译器测试套件
// 测试各种C99语言特性和边缘情况

// 1. 基本数据类型测试
void test_basic_types() {
    char c = 'A';
    int i = 42;
    long l = 123456L;
    float f = 3.14f;
    double d = 2.718281828;
    
    printf("Basic types test:\n");
    printf("char: %c\n", c);
    printf("int: %d\n", i);
    printf("long: %ld\n", l);
    printf("float: %.2f\n", f);
    printf("double: %.6f\n", d);
}

// 2. 控制流测试
void test_control_flow() {
    printf("\nControl flow test:\n");
    
    // if-else测试
    int x = 10;
    if (x > 5) {
        printf("x is greater than 5\n");
    } else {
        printf("x is not greater than 5\n");
    }
    
    // for循环测试
    printf("For loop: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", i);
    }
    printf("\n");
    
    // while循环测试
    printf("While loop: ");
    int j = 0;
    while (j < 3) {
        printf("%d ", j);
        j++;
    }
    printf("\n");
    
    // switch测试
    int choice = 2;
    switch (choice) {
        case 1:
            printf("Choice is 1\n");
            break;
        case 2:
            printf("Choice is 2\n");
            break;
        default:
            printf("Unknown choice\n");
            break;
    }
}

// 3. 函数和递归测试
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

void test_functions() {
    printf("\nFunction test:\n");
    int result = factorial(5);
    printf("factorial(5) = %d\n", result);
}

// 4. 数组和指针测试
void test_arrays_pointers() {
    printf("\nArrays and pointers test:\n");
    
    int arr[5] = {1, 2, 3, 4, 5};
    int* ptr = arr;
    
    printf("Array elements: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
    
    printf("Via pointer: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", *(ptr + i));
    }
    printf("\n");
}

// 5. 字符串处理测试
void test_strings() {
    printf("\nString test:\n");
    
    char str1[] = "Hello";
    char str2[] = "World";
    char result[20];
    
    strcpy(result, str1);
    strcat(result, " ");
    strcat(result, str2);
    
    printf("String concatenation: %s\n", result);
    printf("String length: %d\n", strlen(result));
    
    if (strcmp(str1, "Hello") == 0) {
        printf("String comparison: PASS\n");
    }
}

// 6. 结构体测试
struct Point {
    int x;
    int y;
};

struct Person {
    char name[50];
    int age;
    struct Point location;
};

void test_structures() {
    printf("\nStructure test:\n");
    
    struct Person person;
    strcpy(person.name, "Alice");
    person.age = 25;
    person.location.x = 100;
    person.location.y = 200;
    
    printf("Person: %s, age %d, location (%d, %d)\n",
           person.name, person.age, person.location.x, person.location.y);
}

// 7. 动态内存分配测试
void test_dynamic_memory() {
    printf("\nDynamic memory test:\n");
    
    int* dynamic_array = malloc(5 * sizeof(int));
    if (dynamic_array) {
        for (int i = 0; i < 5; i++) {
            dynamic_array[i] = i * i;
        }
        
        printf("Dynamic array: ");
        for (int i = 0; i < 5; i++) {
            printf("%d ", dynamic_array[i]);
        }
        printf("\n");
        
        free(dynamic_array);
        printf("Memory freed successfully\n");
    }
}

// 8. 数学函数测试
void test_math_functions() {
    printf("\nMath functions test:\n");
    
    double angle = 3.14159 / 4;  // 45 degrees in radians
    printf("sin(π/4) = %.4f\n", sin(angle));
    printf("cos(π/4) = %.4f\n", cos(angle));
    printf("sqrt(16) = %.2f\n", sqrt(16.0));
    printf("pow(2, 3) = %.0f\n", pow(2.0, 3.0));
    printf("abs(-42) = %d\n", abs(-42));
}

// 9. 字符分类测试
void test_character_functions() {
    printf("\nCharacter functions test:\n");
    
    char test_chars[] = {'A', 'a', '5', ' ', '!'};
    int num_chars = sizeof(test_chars) / sizeof(test_chars[0]);
    
    for (int i = 0; i < num_chars; i++) {
        char c = test_chars[i];
        printf("'%c': alpha=%d, digit=%d, space=%d, upper=%d, lower=%d\n",
               c, isalpha(c), isdigit(c), isspace(c), isupper(c), islower(c));
    }
}

// 10. 时间函数测试
void test_time_functions() {
    printf("\nTime functions test:\n");
    
    time_t current_time = time(NULL);
    printf("Current time (timestamp): %ld\n", current_time);
    
    clock_t start_clock = clock();
    // 简单的计算来消耗一些时间
    volatile int sum = 0;
    for (int i = 0; i < 1000; i++) {
        sum += i;
    }
    clock_t end_clock = clock();
    
    printf("Clock ticks elapsed: %ld\n", end_clock - start_clock);
}

// 11. 错误处理测试
void test_error_handling() {
    printf("\nError handling test:\n");
    
    // 测试除零保护
    int a = 10, b = 0;
    if (b != 0) {
        printf("Division result: %d\n", a / b);
    } else {
        printf("Division by zero avoided\n");
    }
    
    // 测试空指针检查
    char* null_ptr = NULL;
    if (null_ptr != NULL) {
        printf("String: %s\n", null_ptr);
    } else {
        printf("Null pointer check passed\n");
    }
}

// 12. 复杂表达式测试
void test_complex_expressions() {
    printf("\nComplex expressions test:\n");
    
    int a = 5, b = 3, c = 2;
    int result1 = (a + b) * c - (a - b) / c;
    printf("(5+3)*2-(5-3)/2 = %d\n", result1);
    
    double x = 2.5, y = 1.5;
    double result2 = (x * y + sqrt(x)) / (y - 1.0);
    printf("(2.5*1.5+sqrt(2.5))/(1.5-1.0) = %.4f\n", result2);
}

// 主测试函数
int main() {
    printf("=== C2ASTC Comprehensive Test Suite ===\n");
    printf("Testing C99 language features and edge cases\n\n");
    
    test_basic_types();
    test_control_flow();
    test_functions();
    test_arrays_pointers();
    test_strings();
    test_structures();
    test_dynamic_memory();
    test_math_functions();
    test_character_functions();
    test_time_functions();
    test_error_handling();
    test_complex_expressions();
    
    printf("\n=== All tests completed ===\n");
    printf("If you can see this message, the C2ASTC compiler\n");
    printf("successfully handled a comprehensive C99 test suite!\n");
    
    return 0;
}
