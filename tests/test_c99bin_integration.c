/**
 * test_c99bin_integration.c - C99Bin Module Integration Test
 * 
 * 测试c99bin模块与现有模块系统的集成
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 包含模块系统头文件
#include "../src/core/module.h"

int main() {
    printf("=== C99Bin Module Integration Test ===\n");
    
    // 初始化模块系统
    printf("Initializing module system...\n");
    if (module_system_init() != 0) {
        printf("❌ Failed to initialize module system\n");
        return 1;
    }
    printf("✅ Module system initialized\n");
    
    // 测试智能路径解析
    printf("\nTesting smart path resolution...\n");
    char* resolved_path = resolve_native_file("./bin/c99bin");
    if (resolved_path) {
        printf("✅ Path resolved: ./bin/c99bin -> %s\n", resolved_path);
        free(resolved_path);
    } else {
        printf("❌ Failed to resolve path\n");
    }
    
    // 测试模块加载
    printf("\nTesting module loading...\n");
    Module* c99bin_module = load_module("./bin/c99bin");
    if (c99bin_module) {
        printf("✅ C99Bin module loaded successfully\n");
        printf("   Module name: %s\n", c99bin_module->name);
        printf("   Module state: %d\n", c99bin_module->state);
        printf("   Module path: %s\n", c99bin_module->path);
        
        // 测试符号解析
        printf("\nTesting symbol resolution...\n");
        if (c99bin_module->sym) {
            void* symbol = c99bin_module->sym(c99bin_module, "c99bin_get_error");
            if (symbol) {
                printf("✅ Symbol resolution works (found c99bin_get_error)\n");
                
                // 调用函数测试
                const char* (*get_error_func)(void) = (const char* (*)(void))symbol;
                const char* error_msg = get_error_func();
                printf("   Error message: %s\n", error_msg ? error_msg : "(null)");
            } else {
                printf("❌ Symbol resolution failed for c99bin_get_error\n");
            }
            
            // 测试其他符号
            void* compile_symbol = c99bin_module->sym(c99bin_module, "c99bin_compile_to_executable");
            if (compile_symbol) {
                printf("✅ Found c99bin_compile_to_executable function\n");
            } else {
                printf("❌ Failed to find c99bin_compile_to_executable\n");
            }
        } else {
            printf("❌ Module sym function is NULL\n");
        }
        
    } else {
        printf("❌ Failed to load C99Bin module\n");
    }
    
    // 清理模块系统
    printf("\nCleaning up module system...\n");
    module_system_cleanup();
    printf("✅ Module system cleanup completed\n");
    
    printf("\n=== Integration test completed ===\n");
    return 0;
}
