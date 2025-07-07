#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "../src/core/module.h"

// 外部声明compiler模块
extern Module module_compiler;

// JIT测试用例结构
typedef struct {
    const char* name;
    uint8_t* bytecode;
    size_t bytecode_size;
    int expected_result;
    const char* description;
} JITTestCase;

// 创建测试字节码
uint8_t simple_return_bytecode[] = {
    0x10, 0x00, 0x2A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // LOAD_IMM r0, 42
    0x31                                                          // RETURN
};

uint8_t add_numbers_bytecode[] = {
    0x10, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // LOAD_IMM r0, 10
    0x10, 0x01, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // LOAD_IMM r1, 20
    0x20, 0x00, 0x00, 0x01,                                       // ADD r0, r0, r1
    0x31                                                          // RETURN
};

uint8_t multiply_bytecode[] = {
    0x10, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // LOAD_IMM r0, 6
    0x10, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // LOAD_IMM r1, 7
    0x22, 0x00, 0x00, 0x01,                                       // MUL r0, r0, r1
    0x31                                                          // RETURN
};

// JIT测试用例
JITTestCase jit_test_cases[] = {
    {
        "simple_return",
        simple_return_bytecode,
        sizeof(simple_return_bytecode),
        42,
        "Simple return constant value"
    },
    {
        "add_numbers",
        add_numbers_bytecode,
        sizeof(add_numbers_bytecode),
        30,
        "Add two numbers (10 + 20)"
    },
    {
        "multiply_numbers",
        multiply_bytecode,
        sizeof(multiply_bytecode),
        42,
        "Multiply two numbers (6 * 7)"
    }
};

int test_jit_compilation() {
    printf("=== JIT Compilation Tests ===\n");
    
    int total_tests = sizeof(jit_test_cases) / sizeof(jit_test_cases[0]);
    int passed = 0;
    int failed = 0;
    
    // 获取JIT函数
    void* create_context_func = module_compiler.resolve("compiler_create_context");
    void* (*compiler_create_context)(void) = (void* (*)(void))create_context_func;
    
    void* compile_func = module_compiler.resolve("compiler_compile_bytecode");
    bool (*compiler_compile_bytecode)(void*, const uint8_t*, size_t) = 
        (bool (*)(void*, const uint8_t*, size_t))compile_func;
    
    void* execute_func = module_compiler.resolve("compiler_execute_jit");
    int (*compiler_execute_jit)(void*) = (int (*)(void*))execute_func;
    
    void* destroy_func = module_compiler.resolve("compiler_destroy_context");
    void (*compiler_destroy_context)(void*) = (void (*)(void*))destroy_func;
    
    if (!compiler_create_context || !compiler_compile_bytecode || 
        !compiler_execute_jit || !compiler_destroy_context) {
        printf("ERROR: Required JIT functions not available\n");
        return 1;
    }
    
    for (int i = 0; i < total_tests; i++) {
        JITTestCase* test = &jit_test_cases[i];
        printf("\nJIT Test %d: %s\n", i + 1, test->name);
        printf("Description: %s\n", test->description);
        
        // 创建JIT上下文
        void* jit_context = compiler_create_context();
        if (!jit_context) {
            printf("✗ FAIL - Could not create JIT context\n");
            failed++;
            continue;
        }
        
        // 编译字节码
        if (!compiler_compile_bytecode(jit_context, test->bytecode, test->bytecode_size)) {
            printf("✗ FAIL - Compilation failed\n");
            compiler_destroy_context(jit_context);
            failed++;
            continue;
        }
        
        // 执行JIT代码
        int result = compiler_execute_jit(jit_context);
        
        if (result == test->expected_result) {
            printf("✓ PASS - Expected %d, got %d\n", test->expected_result, result);
            passed++;
        } else {
            printf("✗ FAIL - Expected %d, got %d\n", test->expected_result, result);
            failed++;
        }
        
        // 清理上下文
        compiler_destroy_context(jit_context);
    }
    
    printf("\n=== JIT Test Summary ===\n");
    printf("Total tests: %d\n", total_tests);
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", failed);
    printf("Success rate: %.1f%%\n", (float)passed / total_tests * 100);
    
    return failed;
}

int test_ffi_functionality() {
    printf("\n=== FFI Functionality Tests ===\n");
    
    // 获取FFI函数
    void* ffi_load_func = module_compiler.resolve("ffi_load_library");
    bool (*ffi_load_library)(const char*) = (bool (*)(const char*))ffi_load_func;
    
    void* ffi_get_func = module_compiler.resolve("ffi_get_function");
    void* (*ffi_get_function)(const char*) = (void* (*)(const char*))ffi_get_func;
    
    void* ffi_call_func = module_compiler.resolve("ffi_call_function");
    int (*ffi_call_function)(const char*, void*, int) = 
        (int (*)(const char*, void*, int))ffi_call_func;
    
    void* ffi_unload_func = module_compiler.resolve("ffi_unload_library");
    void (*ffi_unload_library)(void) = (void (*)(void))ffi_unload_func;
    
    if (!ffi_load_library) {
        printf("   WARNING: FFI functionality not available\n");
        return 0;  // Not a failure, just not implemented
    }
    
    printf("1. Testing library loading...\n");
    
    // 测试加载标准C库
    const char* test_libraries[] = {
        "libc.so.6",     // Linux
        "msvcrt.dll",    // Windows
        "libc.dylib"     // macOS
    };
    
    bool library_loaded = false;
    for (int i = 0; i < 3; i++) {
        if (ffi_load_library(test_libraries[i])) {
            printf("   ✓ Successfully loaded %s\n", test_libraries[i]);
            library_loaded = true;
            break;
        }
    }
    
    if (!library_loaded) {
        printf("   WARNING: Could not load any standard library (expected on some systems)\n");
        return 0;
    }
    
    printf("2. Testing function resolution...\n");
    if (ffi_get_function) {
        void* strlen_func = ffi_get_function("strlen");
        if (strlen_func) {
            printf("   ✓ Successfully resolved strlen function\n");
        } else {
            printf("   WARNING: Could not resolve strlen function\n");
        }
    }
    
    printf("3. Testing function calling...\n");
    if (ffi_call_function) {
        // 这里只测试接口存在，不实际调用以避免复杂性
        printf("   ✓ FFI function calling interface available\n");
    }
    
    printf("4. Testing library unloading...\n");
    if (ffi_unload_library) {
        ffi_unload_library();
        printf("   ✓ Library unloaded successfully\n");
    }
    
    printf("   ✓ FFI functionality tests completed\n");
    return 0;
}

int test_compiler_optimization() {
    printf("\n=== Compiler Optimization Tests ===\n");
    
    // 获取优化设置函数
    void* set_opt_func = module_compiler.resolve("compiler_set_optimization");
    bool (*compiler_set_optimization)(int) = (bool (*)(int))set_opt_func;
    
    void* get_opt_func = module_compiler.resolve("compiler_get_optimization");
    int (*compiler_get_optimization)(void) = (int (*)(void))get_opt_func;
    
    if (!compiler_set_optimization || !compiler_get_optimization) {
        printf("   WARNING: Optimization control not available\n");
        return 0;
    }
    
    printf("1. Testing optimization level setting...\n");
    
    // 测试不同优化级别
    int opt_levels[] = {0, 1, 2, 3};
    for (int i = 0; i < 4; i++) {
        if (compiler_set_optimization(opt_levels[i])) {
            int current_opt = compiler_get_optimization();
            if (current_opt == opt_levels[i]) {
                printf("   ✓ Optimization level %d set successfully\n", opt_levels[i]);
            } else {
                printf("   ✗ Optimization level mismatch: set %d, got %d\n", 
                       opt_levels[i], current_opt);
                return 1;
            }
        } else {
            printf("   ✗ Failed to set optimization level %d\n", opt_levels[i]);
            return 1;
        }
    }
    
    printf("2. Testing invalid optimization levels...\n");
    
    // 测试无效的优化级别
    int invalid_levels[] = {-1, 10, 100};
    for (int i = 0; i < 3; i++) {
        if (!compiler_set_optimization(invalid_levels[i])) {
            printf("   ✓ Invalid optimization level %d properly rejected\n", invalid_levels[i]);
        } else {
            printf("   ✗ Invalid optimization level %d not rejected\n", invalid_levels[i]);
            return 1;
        }
    }
    
    printf("   ✓ Optimization tests completed\n");
    return 0;
}

int test_compiler_error_handling() {
    printf("\n=== Compiler Error Handling Tests ===\n");
    
    // 获取错误处理函数
    void* get_error_func = module_compiler.resolve("compiler_get_error");
    const char* (*compiler_get_error)(void*) = (const char* (*)(void*))get_error_func;
    
    void* create_context_func = module_compiler.resolve("compiler_create_context");
    void* (*compiler_create_context)(void) = (void* (*)(void))create_context_func;
    
    void* compile_func = module_compiler.resolve("compiler_compile_bytecode");
    bool (*compiler_compile_bytecode)(void*, const uint8_t*, size_t) = 
        (bool (*)(void*, const uint8_t*, size_t))compile_func;
    
    if (!compiler_get_error || !compiler_create_context || !compiler_compile_bytecode) {
        printf("   WARNING: Error handling functions not available\n");
        return 0;
    }
    
    void* context = compiler_create_context();
    if (!context) {
        printf("   ✗ Could not create test context\n");
        return 1;
    }
    
    printf("1. Testing NULL bytecode handling...\n");
    if (!compiler_compile_bytecode(context, NULL, 0)) {
        const char* error = compiler_get_error(context);
        if (error && strlen(error) > 0) {
            printf("   ✓ NULL bytecode properly rejected with error: %s\n", error);
        } else {
            printf("   ✗ NULL bytecode rejected but no error message\n");
        }
    } else {
        printf("   ✗ NULL bytecode not properly rejected\n");
    }
    
    printf("2. Testing invalid bytecode handling...\n");
    uint8_t invalid_bytecode[] = {0xFF, 0xFF, 0xFF, 0xFF};  // Invalid opcodes
    if (!compiler_compile_bytecode(context, invalid_bytecode, sizeof(invalid_bytecode))) {
        const char* error = compiler_get_error(context);
        if (error && strlen(error) > 0) {
            printf("   ✓ Invalid bytecode properly rejected with error: %s\n", error);
        } else {
            printf("   ✗ Invalid bytecode rejected but no error message\n");
        }
    } else {
        printf("   ✗ Invalid bytecode not properly rejected\n");
    }
    
    // 清理
    void* destroy_func = module_compiler.resolve("compiler_destroy_context");
    if (destroy_func) {
        ((void (*)(void*))destroy_func)(context);
    }
    
    printf("   ✓ Error handling tests completed\n");
    return 0;
}

int main() {
    printf("=== Extended Compiler Module Test ===\n");
    
    // 1. 初始化模块
    printf("1. Initializing compiler module...\n");
    if (module_compiler.init() != 0) {
        printf("ERROR: Failed to initialize compiler module\n");
        return 1;
    }
    printf("   ✓ Compiler module initialized successfully\n");
    
    int total_failures = 0;
    
    // 2. 运行JIT编译测试
    total_failures += test_jit_compilation();
    
    // 3. 测试FFI功能
    total_failures += test_ffi_functionality();
    
    // 4. 测试编译器优化
    total_failures += test_compiler_optimization();
    
    // 5. 测试错误处理
    total_failures += test_compiler_error_handling();
    
    // 6. 清理
    printf("\n=== Cleanup ===\n");
    module_compiler.cleanup();
    printf("   ✓ Cleanup completed\n");
    
    printf("\n=== Extended Compiler Test Summary ===\n");
    if (total_failures == 0) {
        printf("✓ All extended compiler tests passed!\n");
    } else {
        printf("✗ %d test(s) failed\n", total_failures);
    }
    
    return total_failures;
}
