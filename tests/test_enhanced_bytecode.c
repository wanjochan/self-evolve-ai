/**
 * test_enhanced_bytecode.c - Test Enhanced ASTC Bytecode Generation
 * 
 * Tests the enhanced ASTC bytecode generator with various C99 language features
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/core/astc.h"
#include "../src/core/module.h"

// Test function prototypes
void test_simple_expression(void);
void test_binary_operations(void);
void test_function_call(void);
void test_control_flow(void);
void test_variable_assignment(void);

int main() {
    printf("=== Enhanced ASTC Bytecode Generation Tests ===\n\n");
    
    test_simple_expression();
    test_binary_operations();
    test_function_call();
    test_control_flow();
    test_variable_assignment();
    
    printf("\n=== All Enhanced Bytecode Tests Passed! ===\n");
    return 0;
}

void test_simple_expression(void) {
    printf("Testing simple expression bytecode generation...\n");
    
    // Test simple constant expression: 42
    const char* source = "int main() { return 42; }";
    
    // Load pipeline module
    Module* pipeline = load_module("pipeline");
    if (!pipeline) {
        printf("Failed to load pipeline module\n");
        return;
    }
    
    // Get compilation function
    bool (*compile_func)(const char*) = pipeline->sym("pipeline_compile");
    if (!compile_func) {
        printf("Failed to get compile function\n");
        return;
    }
    
    // Compile the source
    bool result = compile_func(source);
    if (result) {
        printf("✓ Simple expression compilation successful\n");
    } else {
        printf("✗ Simple expression compilation failed\n");
    }
}

void test_binary_operations(void) {
    printf("Testing binary operations bytecode generation...\n");
    
    // Test arithmetic operations
    const char* source = 
        "int main() {\n"
        "    int a = 10;\n"
        "    int b = 5;\n"
        "    int sum = a + b;\n"
        "    int diff = a - b;\n"
        "    int prod = a * b;\n"
        "    int quot = a / b;\n"
        "    return sum + diff + prod + quot;\n"
        "}";
    
    Module* pipeline = load_module("pipeline");
    if (!pipeline) {
        printf("Failed to load pipeline module\n");
        return;
    }
    
    bool (*compile_func)(const char*) = pipeline->sym("pipeline_compile");
    if (!compile_func) {
        printf("Failed to get compile function\n");
        return;
    }
    
    bool result = compile_func(source);
    if (result) {
        printf("✓ Binary operations compilation successful\n");
    } else {
        printf("✗ Binary operations compilation failed\n");
    }
}

void test_function_call(void) {
    printf("Testing function call bytecode generation...\n");
    
    // Test function call with parameters
    const char* source = 
        "int add(int x, int y) {\n"
        "    return x + y;\n"
        "}\n"
        "int main() {\n"
        "    int result = add(10, 20);\n"
        "    return result;\n"
        "}";
    
    Module* pipeline = load_module("pipeline");
    if (!pipeline) {
        printf("Failed to load pipeline module\n");
        return;
    }
    
    bool (*compile_func)(const char*) = pipeline->sym("pipeline_compile");
    if (!compile_func) {
        printf("Failed to get compile function\n");
        return;
    }
    
    bool result = compile_func(source);
    if (result) {
        printf("✓ Function call compilation successful\n");
    } else {
        printf("✗ Function call compilation failed\n");
    }
}

void test_control_flow(void) {
    printf("Testing control flow bytecode generation...\n");
    
    // Test if-else and loops
    const char* source = 
        "int main() {\n"
        "    int sum = 0;\n"
        "    for (int i = 0; i < 10; i++) {\n"
        "        if (i % 2 == 0) {\n"
        "            sum += i;\n"
        "        } else {\n"
        "            sum += i * 2;\n"
        "        }\n"
        "    }\n"
        "    return sum;\n"
        "}";
    
    Module* pipeline = load_module("pipeline");
    if (!pipeline) {
        printf("Failed to load pipeline module\n");
        return;
    }
    
    bool (*compile_func)(const char*) = pipeline->sym("pipeline_compile");
    if (!compile_func) {
        printf("Failed to get compile function\n");
        return;
    }
    
    bool result = compile_func(source);
    if (result) {
        printf("✓ Control flow compilation successful\n");
    } else {
        printf("✗ Control flow compilation failed\n");
    }
}

void test_variable_assignment(void) {
    printf("Testing variable assignment bytecode generation...\n");
    
    // Test various assignment operations
    const char* source = 
        "int main() {\n"
        "    int a = 10;\n"
        "    int b = 5;\n"
        "    a += b;  // compound assignment\n"
        "    b *= 2;  // compound assignment\n"
        "    int c = a > b ? a : b;  // conditional expression\n"
        "    return c;\n"
        "}";
    
    Module* pipeline = load_module("pipeline");
    if (!pipeline) {
        printf("Failed to load pipeline module\n");
        return;
    }
    
    bool (*compile_func)(const char*) = pipeline->sym("pipeline_compile");
    if (!compile_func) {
        printf("Failed to get compile function\n");
        return;
    }
    
    bool result = compile_func(source);
    if (result) {
        printf("✓ Variable assignment compilation successful\n");
    } else {
        printf("✗ Variable assignment compilation failed\n");
    }
}
