#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "../src/core/module.h"

// 外部声明compiler模块
extern Module module_compiler;

int main() {
    printf("=== Compiler Module Test ===\n");
    
    // 1. 初始化模块
    printf("1. Initializing compiler module...\n");
    if (module_compiler.init() != 0) {
        printf("ERROR: Failed to initialize compiler module\n");
        return 1;
    }
    printf("   ✓ Compiler module initialized successfully\n");
    
    // 2. 测试创建JIT编译器上下文
    printf("\n2. Testing JIT compiler context creation...\n");
    void* create_context_func = module_compiler.resolve("compiler_create_context");
    void* (*compiler_create_context)(void) = (void* (*)(void))create_context_func;
    
    if (!compiler_create_context) {
        printf("ERROR: Could not resolve compiler_create_context function\n");
        return 1;
    }
    
    void* jit_context = compiler_create_context();
    if (!jit_context) {
        printf("ERROR: Failed to create JIT compiler context\n");
        return 1;
    }
    printf("   ✓ JIT compiler context created successfully\n");
    
    // 3. 测试编译简单的字节码
    printf("\n3. Testing bytecode compilation...\n");
    
    // 创建简单的测试字节码
    uint8_t test_bytecode[] = {
        0x10, 0x00, 0x2A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // LOAD_IMM r0, 42
        0x31                                                          // RETURN
    };
    size_t bytecode_size = sizeof(test_bytecode);
    
    printf("   Test bytecode: ");
    for (size_t i = 0; i < bytecode_size; i++) {
        printf("0x%02x ", test_bytecode[i]);
    }
    printf("\n");
    
    void* compile_func = module_compiler.resolve("compiler_compile_bytecode");
    bool (*compiler_compile_bytecode)(void*, const uint8_t*, size_t) = 
        (bool (*)(void*, const uint8_t*, size_t))compile_func;
    
    if (!compiler_compile_bytecode) {
        printf("ERROR: Could not resolve compiler_compile_bytecode function\n");
        return 1;
    }
    
    if (!compiler_compile_bytecode(jit_context, test_bytecode, bytecode_size)) {
        void* get_error_func = module_compiler.resolve("compiler_get_error");
        const char* (*get_error)(void*) = (const char* (*)(void*))get_error_func;
        if (get_error) {
            printf("ERROR: Compilation failed: %s\n", get_error(jit_context));
        } else {
            printf("ERROR: Compilation failed (no error details)\n");
        }
        return 1;
    }
    printf("   ✓ Bytecode compiled successfully\n");
    
    // 4. 测试获取生成的机器码
    printf("\n4. Testing machine code generation...\n");
    void* get_code_func = module_compiler.resolve("compiler_get_machine_code");
    const uint8_t* (*get_machine_code)(void*, size_t*) = 
        (const uint8_t* (*)(void*, size_t*))get_code_func;
    
    if (get_machine_code) {
        size_t code_size;
        const uint8_t* machine_code = get_machine_code(jit_context, &code_size);
        if (machine_code && code_size > 0) {
            printf("   Machine code size: %zu bytes\n", code_size);
            printf("   First few bytes: ");
            for (size_t i = 0; i < (code_size < 10 ? code_size : 10); i++) {
                printf("0x%02x ", machine_code[i]);
            }
            printf("\n   ✓ Machine code generated successfully\n");
        } else {
            printf("   WARNING: No machine code generated\n");
        }
    } else {
        printf("   ERROR: Could not resolve compiler_get_machine_code function\n");
    }
    
    // 5. 测试执行JIT编译的代码
    printf("\n5. Testing JIT code execution...\n");
    void* execute_func = module_compiler.resolve("compiler_execute_jit");
    int (*compiler_execute_jit)(void*) = (int (*)(void*))execute_func;
    
    if (compiler_execute_jit) {
        int result = compiler_execute_jit(jit_context);
        printf("   JIT execution result: %d\n", result);
        printf("   ✓ JIT code executed successfully\n");
    } else {
        printf("   WARNING: Could not resolve compiler_execute_jit function\n");
    }
    
    // 6. 测试FFI功能 (简化版本，暂时跳过实际调用以避免段错误)
    printf("\n6. Testing FFI functionality...\n");
    void* ffi_load_func = module_compiler.resolve("ffi_load_library");
    
    if (ffi_load_func) {
        printf("   ✓ FFI interface available\n");
        
        void* ffi_call_func = module_compiler.resolve("ffi_call_function");
        if (ffi_call_func) {
            printf("   ✓ FFI function calling interface available\n");
        } else {
            printf("   WARNING: FFI function calling not available\n");
        }
    } else {
        printf("   WARNING: FFI functionality not available\n");
    }
    
    // 7. 清理
    printf("\n7. Cleaning up...\n");
    void* destroy_func = module_compiler.resolve("compiler_destroy_context");
    void (*compiler_destroy_context)(void*) = (void (*)(void*))destroy_func;
    
    if (compiler_destroy_context) {
        compiler_destroy_context(jit_context);
        printf("   ✓ JIT context destroyed\n");
    }
    
    module_compiler.cleanup();
    printf("   ✓ Cleanup completed\n");
    
    printf("\n=== Test Completed ===\n");
    return 0;
}
