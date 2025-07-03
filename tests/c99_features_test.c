#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Test struct
typedef struct {
    int id;
    char name[32];
    float value;
} TestStruct;

// Test function with pointers
int process_array(int *arr, size_t len) {
    int sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += arr[i];
    }
    return sum;
}

// Test function with struct
void init_struct(TestStruct *ts, int id, const char *name, float value) {
    ts->id = id;
    strncpy(ts->name, name, sizeof(ts->name) - 1);
    ts->name[sizeof(ts->name) - 1] = '\0';
    ts->value = value;
}

// Test recursive function
int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n-1) + fibonacci(n-2);
}

int main() {
    printf("=== C99 Features Test ===\n");
    
    // Test 1: Arrays and pointers
    int numbers[] = {1, 2, 3, 4, 5};
    int sum = process_array(numbers, sizeof(numbers)/sizeof(numbers[0]));
    printf("Array sum: %d\n", sum);
    
    // Test 2: Structs
    TestStruct ts;
    init_struct(&ts, 42, "Test", 3.14f);
    printf("Struct: id=%d, name=%s, value=%.2f\n", ts.id, ts.name, ts.value);
    
    // Test 3: Dynamic memory
    int *dynamic_arr = malloc(5 * sizeof(int));
    if (dynamic_arr) {
        for (int i = 0; i < 5; i++) {
            dynamic_arr[i] = i * i;
        }
        printf("Dynamic array: ");
        for (int i = 0; i < 5; i++) {
            printf("%d ", dynamic_arr[i]);
        }
        printf("\n");
        free(dynamic_arr);
    }
    
    // Test 4: Recursion
    printf("Fibonacci(8) = %d\n", fibonacci(8));
    
    // Test 5: C99 features
    // Variable-length arrays (if supported)
    int n = 3;
    int vla[n];
    for (int i = 0; i < n; i++) {
        vla[i] = i + 10;
    }
    printf("VLA: ");
    for (int i = 0; i < n; i++) {
        printf("%d ", vla[i]);
    }
    printf("\n");
    
    printf("=== All C99 tests completed successfully! ===\n");
    return 0;
}
