#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "../src/core/module.h"

// 外部声明pipeline模块
extern Module module_pipeline;

// 测试用例结构
typedef struct {
    const char* name;
    const char* code;
    bool should_compile;
    const char* description;
} TestCase;

// 扩展的测试用例
TestCase test_cases[] = {
    {
        "simple_return",
        "int main() { return 0; }",
        true,
        "Simple main function with return 0"
    },
    {
        "variable_declaration",
        "int main() { int x = 42; return x; }",
        true,
        "Variable declaration and return"
    },
    {
        "arithmetic_operations",
        "int main() { int a = 10; int b = 20; return a + b; }",
        true,
        "Basic arithmetic operations"
    },
    {
        "function_call",
        "int add(int x, int y) { return x + y; } int main() { return add(5, 3); }",
        true,
        "Function definition and call"
    },
    {
        "if_statement",
        "int main() { int x = 10; if (x > 5) return 1; return 0; }",
        true,
        "Conditional if statement"
    },
    {
        "while_loop",
        "int main() { int i = 0; while (i < 3) i++; return i; }",
        true,
        "Simple while loop"
    },
    {
        "for_loop",
        "int main() { int sum = 0; for (int i = 0; i < 5; i++) sum += i; return sum; }",
        true,
        "For loop with accumulator"
    },
    {
        "array_access",
        "int main() { int arr[3] = {1, 2, 3}; return arr[1]; }",
        true,
        "Array declaration and access"
    },
    {
        "pointer_basic",
        "int main() { int x = 42; int* p = &x; return *p; }",
        true,
        "Basic pointer operations"
    },
    {
        "syntax_error",
        "int main() { int x = ; return x; }",
        false,
        "Syntax error - missing value"
    },
    {
        "missing_semicolon",
        "int main() { int x = 42 return x; }",
        false,
        "Syntax error - missing semicolon"
    },
    {
        "undefined_variable",
        "int main() { return undefined_var; }",
        false,
        "Semantic error - undefined variable"
    }
};

int run_compilation_tests() {
    printf("=== Extended Pipeline Module Compilation Tests ===\n");
    
    int total_tests = sizeof(test_cases) / sizeof(test_cases[0]);
    int passed = 0;
    int failed = 0;
    
    // 获取编译函数
    void* compile_func = module_pipeline.resolve("pipeline_compile");
    bool (*pipeline_compile)(const char*, void*) = (bool (*)(const char*, void*))compile_func;
    
    if (!pipeline_compile) {
        printf("ERROR: Could not resolve pipeline_compile function\n");
        return 1;
    }
    
    // 获取错误信息函数
    void* get_error_func = module_pipeline.resolve("pipeline_get_error");
    const char* (*get_error)(void) = (const char* (*)(void))get_error_func;
    
    for (int i = 0; i < total_tests; i++) {
        TestCase* test = &test_cases[i];
        printf("\nTest %d: %s\n", i + 1, test->name);
        printf("Description: %s\n", test->description);
        printf("Code:\n%s\n", test->code);
        
        bool result = pipeline_compile(test->code, NULL);
        
        if (result == test->should_compile) {
            printf("✓ PASS - Expected %s, got %s\n", 
                   test->should_compile ? "success" : "failure",
                   result ? "success" : "failure");
            passed++;
        } else {
            printf("✗ FAIL - Expected %s, got %s\n", 
                   test->should_compile ? "success" : "failure",
                   result ? "success" : "failure");
            
            if (!result && get_error) {
                const char* error = get_error();
                if (error) {
                    printf("  Error: %s\n", error);
                }
            }
            failed++;
        }
    }
    
    printf("\n=== Compilation Test Summary ===\n");
    printf("Total tests: %d\n", total_tests);
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", failed);
    printf("Success rate: %.1f%%\n", (float)passed / total_tests * 100);
    
    return failed;
}

int test_pipeline_features() {
    printf("\n=== Pipeline Feature Tests ===\n");
    
    // 测试获取汇编代码
    printf("1. Testing assembly code generation...\n");
    void* get_assembly_func = module_pipeline.resolve("pipeline_get_assembly");
    const char* (*get_assembly)(void) = (const char* (*)(void))get_assembly_func;
    
    if (get_assembly) {
        printf("   ✓ Assembly generation function available\n");
    } else {
        printf("   ✗ Assembly generation function not available\n");
        return 1;
    }
    
    // 测试获取字节码
    printf("2. Testing bytecode generation...\n");
    void* get_bytecode_func = module_pipeline.resolve("pipeline_get_bytecode");
    const uint8_t* (*get_bytecode)(size_t*) = (const uint8_t* (*)(size_t*))get_bytecode_func;
    
    if (get_bytecode) {
        printf("   ✓ Bytecode generation function available\n");
    } else {
        printf("   ✗ Bytecode generation function not available\n");
        return 1;
    }
    
    // 测试执行功能
    printf("3. Testing execution capability...\n");
    void* execute_func = module_pipeline.resolve("pipeline_execute");
    bool (*pipeline_execute)(void) = (bool (*)(void))execute_func;
    
    if (pipeline_execute) {
        printf("   ✓ Execution function available\n");
    } else {
        printf("   ✗ Execution function not available\n");
        return 1;
    }
    
    // 测试AOT编译功能
    printf("4. Testing AOT compilation...\n");
    void* aot_func = module_pipeline.resolve("pipeline_astc2native");
    bool (*pipeline_astc2native)(const uint8_t*, size_t, const char*) = 
        (bool (*)(const uint8_t*, size_t, const char*))aot_func;
    
    if (pipeline_astc2native) {
        printf("   ✓ AOT compilation function available\n");
    } else {
        printf("   ✗ AOT compilation function not available\n");
        return 1;
    }
    
    printf("   ✓ All pipeline features available\n");
    return 0;
}

int test_error_handling() {
    printf("\n=== Error Handling Tests ===\n");
    
    // 获取编译函数
    void* compile_func = module_pipeline.resolve("pipeline_compile");
    bool (*pipeline_compile)(const char*, void*) = (bool (*)(const char*, void*))compile_func;
    
    // 获取错误信息函数
    void* get_error_func = module_pipeline.resolve("pipeline_get_error");
    const char* (*get_error)(void) = (const char* (*)(void))get_error_func;
    
    if (!pipeline_compile || !get_error) {
        printf("   ✗ Required functions not available\n");
        return 1;
    }
    
    // 测试NULL输入
    printf("1. Testing NULL input handling...\n");
    bool result = pipeline_compile(NULL, NULL);
    if (!result) {
        printf("   ✓ NULL input properly rejected\n");
    } else {
        printf("   ✗ NULL input not properly handled\n");
        return 1;
    }
    
    // 测试空字符串
    printf("2. Testing empty string handling...\n");
    result = pipeline_compile("", NULL);
    if (!result) {
        printf("   ✓ Empty string properly rejected\n");
    } else {
        printf("   ✗ Empty string not properly handled\n");
        return 1;
    }
    
    // 测试语法错误
    printf("3. Testing syntax error handling...\n");
    result = pipeline_compile("invalid syntax here", NULL);
    if (!result) {
        const char* error = get_error();
        if (error && strlen(error) > 0) {
            printf("   ✓ Syntax error properly reported: %s\n", error);
        } else {
            printf("   ✗ Syntax error not properly reported\n");
            return 1;
        }
    } else {
        printf("   ✗ Syntax error not detected\n");
        return 1;
    }
    
    printf("   ✓ Error handling working correctly\n");
    return 0;
}

int main() {
    printf("=== Extended Pipeline Module Test ===\n");
    
    // 1. 初始化模块
    printf("1. Initializing pipeline module...\n");
    if (module_pipeline.init() != 0) {
        printf("ERROR: Failed to initialize pipeline module\n");
        return 1;
    }
    printf("   ✓ Pipeline module initialized successfully\n");
    
    int total_failures = 0;
    
    // 2. 运行编译测试
    total_failures += run_compilation_tests();
    
    // 3. 测试管道功能
    total_failures += test_pipeline_features();
    
    // 4. 测试错误处理
    total_failures += test_error_handling();
    
    // 5. 清理
    printf("\n=== Cleanup ===\n");
    module_pipeline.cleanup();
    printf("   ✓ Cleanup completed\n");
    
    printf("\n=== Extended Test Summary ===\n");
    if (total_failures == 0) {
        printf("✓ All extended tests passed!\n");
    } else {
        printf("✗ %d test(s) failed\n", total_failures);
    }
    
    return total_failures;
}
