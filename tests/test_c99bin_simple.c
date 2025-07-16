/**
 * test_c99bin_simple.c - Simple C99Bin Test
 * 
 * 简单测试c99bin模块的基础功能
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 包含模块系统头文件
#include "../src/core/module.h"

int main() {
    printf("=== C99Bin Simple Test ===\n");
    
    // 初始化模块系统
    if (module_system_init() != 0) {
        printf("❌ Failed to initialize module system\n");
        return 1;
    }
    printf("✅ Module system initialized\n");
    
    // 加载c99bin模块
    Module* c99bin_module = load_module("./bin/c99bin");
    if (!c99bin_module) {
        printf("❌ Failed to load c99bin module\n");
        module_system_cleanup();
        return 1;
    }
    printf("✅ C99Bin module loaded\n");
    
    // 测试c99bin_set_dependencies函数
    void* set_deps_func = c99bin_module->sym(c99bin_module, "c99bin_set_dependencies");
    if (set_deps_func) {
        printf("✅ Found c99bin_set_dependencies function\n");
        
        // 调用函数设置NULL依赖 (测试)
        int (*set_dependencies)(Module*, Module*, Module*) = 
            (int (*)(Module*, Module*, Module*))set_deps_func;
        int result = set_dependencies(NULL, NULL, NULL);
        printf("✅ Called c99bin_set_dependencies with NULL deps, result: %d\n", result);
    } else {
        printf("❌ c99bin_set_dependencies function not found\n");
    }
    
    // 测试c99bin_compile_to_executable函数
    void* compile_func = c99bin_module->sym(c99bin_module, "c99bin_compile_to_executable");
    if (compile_func) {
        printf("✅ Found c99bin_compile_to_executable function\n");
        
        // 创建测试文件
        const char* test_content = "#include <stdio.h>\nint main() { return 0; }\n";
        FILE* f = fopen("/tmp/test.c", "w");
        if (f) {
            fprintf(f, "%s", test_content);
            fclose(f);
            printf("✅ Created test file /tmp/test.c\n");
            
            // 调用编译函数
            int (*compile_to_executable)(const char*, const char*) = 
                (int (*)(const char*, const char*))compile_func;
            
            printf("Calling c99bin_compile_to_executable...\n");
            int compile_result = compile_to_executable("/tmp/test.c", "/tmp/test_out");
            printf("✅ Compile function returned: %d\n", compile_result);
            
            // 获取错误信息
            void* get_error_func = c99bin_module->sym(c99bin_module, "c99bin_get_error");
            if (get_error_func) {
                const char* (*get_error)(void) = (const char* (*)(void))get_error_func;
                const char* error_msg = get_error();
                printf("Error message: %s\n", error_msg ? error_msg : "(null)");
            }
        } else {
            printf("❌ Failed to create test file\n");
        }
    } else {
        printf("❌ c99bin_compile_to_executable function not found\n");
    }
    
    // 清理
    module_system_cleanup();
    printf("✅ Test completed\n");
    
    return 0;
}
