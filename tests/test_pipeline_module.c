#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "../src/core/module.h"

// 外部声明pipeline模块
extern Module module_pipeline;

int main() {
    printf("=== Pipeline Module Test ===\n");
    
    // 1. 初始化模块
    printf("1. Initializing pipeline module...\n");
    if (module_pipeline.init() != 0) {
        printf("ERROR: Failed to initialize pipeline module\n");
        return 1;
    }
    printf("   ✓ Pipeline module initialized successfully\n");
    
    // 2. 测试编译简单的C代码
    printf("\n2. Testing C code compilation...\n");
    const char* test_code = 
        "int main() {\n"
        "    int x = 42;\n"
        "    return x;\n"
        "}\n";
    
    printf("   Test code:\n%s\n", test_code);
    
    // 获取编译函数
    void* compile_func = module_pipeline.resolve("pipeline_compile");
    bool (*pipeline_compile)(const char*, void*) = (bool (*)(const char*, void*))compile_func;
    
    if (!pipeline_compile) {
        printf("ERROR: Could not resolve pipeline_compile function\n");
        return 1;
    }
    
    // 编译代码
    if (!pipeline_compile(test_code, NULL)) {
        void* get_error_func = module_pipeline.resolve("pipeline_get_error");
        const char* (*get_error)(void) = (const char* (*)(void))get_error_func;
        if (get_error) {
            printf("ERROR: Compilation failed: %s\n", get_error());
        } else {
            printf("ERROR: Compilation failed (no error details)\n");
        }
        return 1;
    }
    printf("   ✓ Code compiled successfully\n");
    
    // 3. 测试获取生成的汇编代码
    printf("\n3. Testing assembly code generation...\n");
    void* get_assembly_func = module_pipeline.resolve("pipeline_get_assembly");
    const char* (*get_assembly)(void) = (const char* (*)(void))get_assembly_func;
    
    if (get_assembly) {
        const char* assembly = get_assembly();
        if (assembly) {
            printf("   Generated assembly:\n%s\n", assembly);
            printf("   ✓ Assembly code generated successfully\n");
        } else {
            printf("   WARNING: No assembly code generated\n");
        }
    } else {
        printf("   ERROR: Could not resolve pipeline_get_assembly function\n");
    }
    
    // 4. 测试获取字节码
    printf("\n4. Testing bytecode generation...\n");
    void* get_bytecode_func = module_pipeline.resolve("pipeline_get_bytecode");
    const uint8_t* (*get_bytecode)(size_t*) = (const uint8_t* (*)(size_t*))get_bytecode_func;
    
    if (get_bytecode) {
        size_t bytecode_size;
        const uint8_t* bytecode = get_bytecode(&bytecode_size);
        if (bytecode && bytecode_size > 0) {
            printf("   Bytecode size: %zu bytes\n", bytecode_size);
            printf("   First few bytes: ");
            for (size_t i = 0; i < (bytecode_size < 10 ? bytecode_size : 10); i++) {
                printf("0x%02x ", bytecode[i]);
            }
            printf("\n   ✓ Bytecode generated successfully\n");
        } else {
            printf("   WARNING: No bytecode generated\n");
        }
    } else {
        printf("   ERROR: Could not resolve pipeline_get_bytecode function\n");
    }
    
    // 5. 测试执行
    printf("\n5. Testing bytecode execution...\n");
    void* execute_func = module_pipeline.resolve("pipeline_execute");
    bool (*pipeline_execute)(void) = (bool (*)(void))execute_func;
    
    if (pipeline_execute) {
        if (pipeline_execute()) {
            printf("   ✓ Bytecode executed successfully\n");
        } else {
            void* get_error_func = module_pipeline.resolve("pipeline_get_error");
            const char* (*get_error)(void) = (const char* (*)(void))get_error_func;
            if (get_error) {
                printf("   ERROR: Execution failed: %s\n", get_error());
            } else {
                printf("   ERROR: Execution failed (no error details)\n");
            }
        }
    } else {
        printf("   ERROR: Could not resolve pipeline_execute function\n");
    }
    
    // 6. 清理
    printf("\n6. Cleaning up...\n");
    module_pipeline.cleanup();
    printf("   ✓ Cleanup completed\n");
    
    printf("\n=== Test Completed ===\n");
    return 0;
}
