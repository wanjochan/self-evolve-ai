/**
 * test_c99bin_module.c - C99Bin Module Test
 * 
 * 测试c99bin模块的基础功能
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

int main() {
    printf("=== C99Bin Module Test ===\n");
    
    // 加载c99bin模块
    void* handle = dlopen("./bin/c99bin_x64_64.native", RTLD_LAZY);
    if (!handle) {
        printf("❌ Failed to load c99bin module: %s\n", dlerror());
        return 1;
    }
    printf("✅ C99Bin module loaded successfully\n");
    
    // 测试module_init函数
    int (*module_init)(void) = dlsym(handle, "module_init");
    if (!module_init) {
        printf("❌ Failed to find module_init: %s\n", dlerror());
        dlclose(handle);
        return 1;
    }
    
    int init_result = module_init();
    if (init_result == 0) {
        printf("✅ Module initialization successful\n");
    } else {
        printf("❌ Module initialization failed: %d\n", init_result);
    }
    
    // 测试test_export_function
    int (*test_export_function)(void) = dlsym(handle, "test_export_function");
    if (!test_export_function) {
        printf("❌ Failed to find test_export_function: %s\n", dlerror());
    } else {
        int test_result = test_export_function();
        if (test_result == 99) {
            printf("✅ Test export function works (returned %d)\n", test_result);
        } else {
            printf("❌ Test export function returned unexpected value: %d\n", test_result);
        }
    }
    
    // 测试符号解析函数
    void* (*c99bin_module_resolve)(const char*) = dlsym(handle, "c99bin_module_resolve");
    if (!c99bin_module_resolve) {
        printf("❌ Failed to find c99bin_module_resolve: %s\n", dlerror());
    } else {
        // 测试解析已知符号
        void* symbol = c99bin_module_resolve("c99bin_get_error");
        if (symbol) {
            printf("✅ Symbol resolution works (found c99bin_get_error)\n");
        } else {
            printf("❌ Symbol resolution failed for c99bin_get_error\n");
        }
    }
    
    // 测试module_cleanup函数
    void (*module_cleanup)(void) = dlsym(handle, "module_cleanup");
    if (!module_cleanup) {
        printf("❌ Failed to find module_cleanup: %s\n", dlerror());
    } else {
        module_cleanup();
        printf("✅ Module cleanup completed\n");
    }
    
    // 关闭模块
    dlclose(handle);
    printf("✅ C99Bin module test completed successfully\n");
    
    return 0;
}
