/**
 * performance_test.c - Performance Benchmarking Program
 * 
 * This program tests the performance of various operations
 * in the ASTC runtime environment.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Test configuration
#define ITERATIONS_SMALL  1000
#define ITERATIONS_MEDIUM 10000
#define ITERATIONS_LARGE  100000
#define ARRAY_SIZE        1000

// Function prototypes
void test_arithmetic_performance(void);
void test_loop_performance(void);
void test_function_call_performance(void);
void test_memory_performance(void);
void test_string_performance(void);
void test_recursive_performance(void);

// Utility functions
double get_time_diff(clock_t start, clock_t end);
void print_performance_result(const char* test_name, int iterations, double time_ms);

// Test functions
int simple_calculation(int a, int b);
int fibonacci_recursive(int n);
int fibonacci_iterative(int n);

int main(void) {
    printf("=== ASTC Runtime Performance Test ===\n");
    printf("Testing performance in three-layer architecture\n\n");
    
    test_arithmetic_performance();
    test_loop_performance();
    test_function_call_performance();
    test_memory_performance();
    test_string_performance();
    test_recursive_performance();
    
    printf("\n=== Performance Testing Completed ===\n");
    return 0;
}

void test_arithmetic_performance(void) {
    printf("1. Arithmetic Operations Performance:\n");
    
    clock_t start, end;
    volatile int result = 0;  // volatile to prevent optimization
    
    // Integer arithmetic
    start = clock();
    for (int i = 0; i < ITERATIONS_LARGE; i++) {
        result += i * 2 + 1;
        result -= i / 2;
        result *= (i % 10 + 1);
        result /= (i % 5 + 1);
    }
    end = clock();
    print_performance_result("Integer arithmetic", ITERATIONS_LARGE, get_time_diff(start, end));
    
    // Floating point arithmetic
    volatile double dresult = 0.0;
    start = clock();
    for (int i = 0; i < ITERATIONS_MEDIUM; i++) {
        dresult += i * 3.14159;
        dresult -= i / 2.71828;
        dresult *= (i % 10 + 1) * 1.414;
        dresult /= (i % 5 + 1) * 1.732;
    }
    end = clock();
    print_performance_result("Floating point arithmetic", ITERATIONS_MEDIUM, get_time_diff(start, end));
}

void test_loop_performance(void) {
    printf("\n2. Loop Performance:\n");
    
    clock_t start, end;
    volatile int sum = 0;
    
    // For loop
    start = clock();
    for (int i = 0; i < ITERATIONS_LARGE; i++) {
        sum += i;
    }
    end = clock();
    print_performance_result("For loop", ITERATIONS_LARGE, get_time_diff(start, end));
    
    // While loop
    sum = 0;
    start = clock();
    int i = 0;
    while (i < ITERATIONS_LARGE) {
        sum += i;
        i++;
    }
    end = clock();
    print_performance_result("While loop", ITERATIONS_LARGE, get_time_diff(start, end));
    
    // Nested loops
    sum = 0;
    start = clock();
    for (int i = 0; i < ITERATIONS_SMALL; i++) {
        for (int j = 0; j < 100; j++) {
            sum += i + j;
        }
    }
    end = clock();
    print_performance_result("Nested loops", ITERATIONS_SMALL * 100, get_time_diff(start, end));
}

void test_function_call_performance(void) {
    printf("\n3. Function Call Performance:\n");
    
    clock_t start, end;
    volatile int result = 0;
    
    // Simple function calls
    start = clock();
    for (int i = 0; i < ITERATIONS_MEDIUM; i++) {
        result += simple_calculation(i, i + 1);
    }
    end = clock();
    print_performance_result("Simple function calls", ITERATIONS_MEDIUM, get_time_diff(start, end));
    
    // Function calls with parameters
    start = clock();
    for (int i = 0; i < ITERATIONS_SMALL; i++) {
        result += simple_calculation(i * 2, i * 3);
        result += simple_calculation(i + 10, i - 5);
    }
    end = clock();
    print_performance_result("Multiple function calls", ITERATIONS_SMALL * 2, get_time_diff(start, end));
}

void test_memory_performance(void) {
    printf("\n4. Memory Operations Performance:\n");
    
    clock_t start, end;
    
    // Array access
    int *array = malloc(ARRAY_SIZE * sizeof(int));
    if (!array) {
        printf("   Memory allocation failed!\n");
        return;
    }
    
    // Sequential write
    start = clock();
    for (int iter = 0; iter < ITERATIONS_MEDIUM; iter++) {
        for (int i = 0; i < ARRAY_SIZE; i++) {
            array[i] = i * iter;
        }
    }
    end = clock();
    print_performance_result("Sequential array write", ITERATIONS_MEDIUM * ARRAY_SIZE, get_time_diff(start, end));
    
    // Sequential read
    volatile int sum = 0;
    start = clock();
    for (int iter = 0; iter < ITERATIONS_MEDIUM; iter++) {
        for (int i = 0; i < ARRAY_SIZE; i++) {
            sum += array[i];
        }
    }
    end = clock();
    print_performance_result("Sequential array read", ITERATIONS_MEDIUM * ARRAY_SIZE, get_time_diff(start, end));
    
    // Memory allocation/deallocation
    start = clock();
    for (int i = 0; i < ITERATIONS_SMALL; i++) {
        int *temp = malloc(100 * sizeof(int));
        if (temp) {
            temp[0] = i;  // Use the memory
            free(temp);
        }
    }
    end = clock();
    print_performance_result("Memory alloc/free", ITERATIONS_SMALL, get_time_diff(start, end));
    
    free(array);
}

void test_string_performance(void) {
    printf("\n5. String Operations Performance:\n");
    
    clock_t start, end;
    char buffer[1000];
    char source[] = "Hello, World! This is a test string for performance measurement.";
    
    // String copy
    start = clock();
    for (int i = 0; i < ITERATIONS_MEDIUM; i++) {
        strcpy(buffer, source);
    }
    end = clock();
    print_performance_result("String copy", ITERATIONS_MEDIUM, get_time_diff(start, end));
    
    // String concatenation
    start = clock();
    for (int i = 0; i < ITERATIONS_SMALL; i++) {
        strcpy(buffer, "Start: ");
        strcat(buffer, source);
        strcat(buffer, " :End");
    }
    end = clock();
    print_performance_result("String concatenation", ITERATIONS_SMALL, get_time_diff(start, end));
    
    // String length calculation
    volatile size_t len = 0;
    start = clock();
    for (int i = 0; i < ITERATIONS_LARGE; i++) {
        len += strlen(source);
    }
    end = clock();
    print_performance_result("String length", ITERATIONS_LARGE, get_time_diff(start, end));
}

void test_recursive_performance(void) {
    printf("\n6. Recursion Performance:\n");
    
    clock_t start, end;
    volatile int result = 0;
    
    // Recursive fibonacci (small numbers to avoid stack overflow)
    start = clock();
    for (int i = 0; i < 100; i++) {
        result += fibonacci_recursive(20);
    }
    end = clock();
    print_performance_result("Recursive fibonacci(20)", 100, get_time_diff(start, end));
    
    // Iterative fibonacci for comparison
    start = clock();
    for (int i = 0; i < ITERATIONS_SMALL; i++) {
        result += fibonacci_iterative(30);
    }
    end = clock();
    print_performance_result("Iterative fibonacci(30)", ITERATIONS_SMALL, get_time_diff(start, end));
}

// Utility function implementations
double get_time_diff(clock_t start, clock_t end) {
    return ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;  // Convert to milliseconds
}

void print_performance_result(const char* test_name, int iterations, double time_ms) {
    double ops_per_sec = iterations / (time_ms / 1000.0);
    printf("   %-30s: %8.2f ms (%8.0f ops/sec)\n", test_name, time_ms, ops_per_sec);
}

int simple_calculation(int a, int b) {
    return (a + b) * (a - b) + (a % 10);
}

int fibonacci_recursive(int n) {
    if (n <= 1) return n;
    return fibonacci_recursive(n - 1) + fibonacci_recursive(n - 2);
}

int fibonacci_iterative(int n) {
    if (n <= 1) return n;
    
    int a = 0, b = 1, temp;
    for (int i = 2; i <= n; i++) {
        temp = a + b;
        a = b;
        b = temp;
    }
    return b;
}
