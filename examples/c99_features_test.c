/**
 * c99_features_test.c - Comprehensive C99 Features Test
 * 
 * This program tests various C99 language features to validate
 * the compiler and runtime system capabilities.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Test structures and unions
struct Point {
    int x, y;
};

union Data {
    int i;
    float f;
    char str[20];
};

// Function prototypes
void test_basic_types(void);
void test_arrays_and_strings(void);
void test_structures_and_unions(void);
void test_pointers_and_memory(void);
void test_control_flow(void);
void test_preprocessor(void);
int compare_ints(const void *a, const void *b);

// Global variables for testing
static int global_counter = 0;
const char* program_name = "C99 Features Test";

int main(void) {
    printf("=== %s ===\n", program_name);
    printf("Comprehensive C99 language features validation\n\n");
    
    test_basic_types();
    test_arrays_and_strings();
    test_structures_and_unions();
    test_pointers_and_memory();
    test_control_flow();
    test_preprocessor();
    
    printf("\nGlobal counter final value: %d\n", global_counter);
    printf("=== All tests completed ===\n");
    return 0;
}

void test_basic_types(void) {
    printf("1. Testing Basic Data Types:\n");
    
    // Integer types
    char c = 127;
    short s = 32767;
    int i = 2147483647;
    long l = 2147483647L;
    
    // Unsigned types
    unsigned char uc = 255;
    unsigned short us = 65535;
    unsigned int ui = 4294967295U;
    unsigned long ul = 4294967295UL;
    
    // Floating point types
    float f = 3.14159f;
    double d = 2.718281828459045;
    
    // Boolean type (C99)
    bool flag = true;
    
    printf("   char: %d, short: %d, int: %d, long: %ld\n", c, s, i, l);
    printf("   unsigned char: %u, unsigned short: %u\n", uc, us);
    printf("   unsigned int: %u, unsigned long: %lu\n", ui, ul);
    printf("   float: %.5f, double: %.15f\n", f, d);
    printf("   bool: %s\n", flag ? "true" : "false");
    
    global_counter++;
}

void test_arrays_and_strings(void) {
    printf("\n2. Testing Arrays and Strings:\n");
    
    // Array initialization
    int numbers[] = {1, 2, 3, 4, 5};
    int matrix[2][3] = {{1, 2, 3}, {4, 5, 6}};
    
    // String operations
    char str1[] = "Hello";
    char str2[] = "World";
    char result[50];
    
    strcpy(result, str1);
    strcat(result, " ");
    strcat(result, str2);
    
    printf("   Array: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    
    printf("   Matrix:\n");
    for (int i = 0; i < 2; i++) {
        printf("     ");
        for (int j = 0; j < 3; j++) {
            printf("%d ", matrix[i][j]);
        }
        printf("\n");
    }
    
    printf("   String concatenation: %s\n", result);
    printf("   String length: %zu\n", strlen(result));
    
    global_counter++;
}

void test_structures_and_unions(void) {
    printf("\n3. Testing Structures and Unions:\n");
    
    // Structure initialization
    struct Point p1 = {10, 20};
    struct Point p2;
    p2.x = 30;
    p2.y = 40;
    
    // Union usage
    union Data data;
    data.i = 42;
    printf("   Union as int: %d\n", data.i);
    
    data.f = 3.14f;
    printf("   Union as float: %.2f\n", data.f);
    
    strcpy(data.str, "Hello");
    printf("   Union as string: %s\n", data.str);
    
    printf("   Point 1: (%d, %d)\n", p1.x, p1.y);
    printf("   Point 2: (%d, %d)\n", p2.x, p2.y);
    
    global_counter++;
}

void test_pointers_and_memory(void) {
    printf("\n4. Testing Pointers and Memory:\n");
    
    int value = 100;
    int *ptr = &value;
    int **double_ptr = &ptr;
    
    printf("   Value: %d\n", value);
    printf("   Pointer to value: %d\n", *ptr);
    printf("   Double pointer to value: %d\n", **double_ptr);
    
    // Dynamic memory allocation
    int *dynamic_array = malloc(5 * sizeof(int));
    if (dynamic_array) {
        for (int i = 0; i < 5; i++) {
            dynamic_array[i] = i * i;
        }
        
        printf("   Dynamic array: ");
        for (int i = 0; i < 5; i++) {
            printf("%d ", dynamic_array[i]);
        }
        printf("\n");
        
        free(dynamic_array);
    }
    
    global_counter++;
}

void test_control_flow(void) {
    printf("\n5. Testing Control Flow:\n");
    
    // Switch statement
    printf("   Switch statement test:\n");
    for (int i = 1; i <= 3; i++) {
        printf("     Case %d: ", i);
        switch (i) {
            case 1:
                printf("First\n");
                break;
            case 2:
                printf("Second\n");
                break;
            case 3:
                printf("Third\n");
                break;
            default:
                printf("Unknown\n");
                break;
        }
    }
    
    // Nested loops with break and continue
    printf("   Nested loops with break/continue:\n");
    for (int i = 1; i <= 3; i++) {
        printf("     Outer loop %d: ", i);
        for (int j = 1; j <= 5; j++) {
            if (j == 3) continue;
            if (j == 5) break;
            printf("%d ", j);
        }
        printf("\n");
    }
    
    global_counter++;
}

void test_preprocessor(void) {
    printf("\n6. Testing Preprocessor Features:\n");
    
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define SQUARE(x) ((x) * (x))
    
    int a = 10, b = 20;
    printf("   MAX(%d, %d) = %d\n", a, b, MAX(a, b));
    printf("   MIN(%d, %d) = %d\n", a, b, MIN(a, b));
    printf("   SQUARE(%d) = %d\n", a, SQUARE(a));
    
#ifdef __STDC_VERSION__
    printf("   C Standard Version: %ld\n", __STDC_VERSION__);
#endif
    
    printf("   File: %s\n", __FILE__);
    printf("   Line: %d\n", __LINE__);
    
    global_counter++;
}

// Comparison function for qsort
int compare_ints(const void *a, const void *b) {
    int ia = *(const int*)a;
    int ib = *(const int*)b;
    return (ia > ib) - (ia < ib);
}
