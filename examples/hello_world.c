/**
 * hello_world.c - Enhanced Hello World Program
 *
 * Enhanced C program demonstrating C99 features and three-layer architecture.
 * Tests various C99 language constructs and runtime capabilities.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test function declarations
void test_c99_features(void);
void test_variables_and_loops(void);
void test_functions_and_recursion(void);
int factorial(int n);

int main(void) {
    printf("=== Enhanced Hello World Program ===\n");
    printf("Running in Self-Evolve AI Three-Layer Architecture\n\n");

    printf("Architecture Overview:\n");
    printf("  Layer 1: simple_loader (Entry Point)\n");
    printf("  Layer 2: pipeline_*.native (VM Runtime)\n");
    printf("  Layer 3: hello_world.astc (This Program)\n\n");

    // Test C99 features
    printf("Testing C99 Language Features:\n");
    test_c99_features();

    printf("\nTesting Variables and Control Flow:\n");
    test_variables_and_loops();

    printf("\nTesting Functions and Recursion:\n");
    test_functions_and_recursion();

    printf("\n=== Program Completed Successfully ===\n");
    return 0;
}

void test_c99_features(void) {
    // C99 variable declarations in for loops
    printf("  - C99 for-loop variable declarations: ");
    for (int i = 0; i < 3; i++) {
        printf("%d ", i);
    }
    printf("\n");

    // Mixed declarations and code
    int x = 10;
    printf("  - Mixed declarations: x = %d\n", x);
    int y = 20;
    printf("  - Variable y = %d\n", y);

    // Compound literals (if supported)
    int arr[] = {1, 2, 3, 4, 5};
    printf("  - Array initialization: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

void test_variables_and_loops(void) {
    // Test different variable types
    char c = 'A';
    int i = 42;
    float f = 3.14f;
    double d = 2.718281828;

    printf("  - char: %c\n", c);
    printf("  - int: %d\n", i);
    printf("  - float: %.2f\n", f);
    printf("  - double: %.6f\n", d);

    // Test while loop
    printf("  - while loop countdown: ");
    int count = 5;
    while (count > 0) {
        printf("%d ", count);
        count--;
    }
    printf("\n");

    // Test do-while loop
    printf("  - do-while loop: ");
    int j = 1;
    do {
        printf("%d ", j);
        j++;
    } while (j <= 3);
    printf("\n");
}

void test_functions_and_recursion(void) {
    printf("  - Testing factorial function:\n");
    for (int i = 0; i <= 5; i++) {
        printf("    factorial(%d) = %d\n", i, factorial(i));
    }
}

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}
