/**
 * test_c99bin_frontend.c - Test C99Bin Frontend Integration
 * 
 * 测试c99bin模块与pipeline前端的集成
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 包含模块系统头文件
#include "../src/core/module.h"

int main() {
    printf("=== C99Bin Frontend Integration Test ===\n");
    
    // 初始化模块系统
    printf("Initializing module system...\n");
    if (module_system_init() != 0) {
        printf("❌ Failed to initialize module system\n");
        return 1;
    }
    printf("✅ Module system initialized\n");
    
    // 加载c99bin模块
    printf("\nLoading c99bin module...\n");
    Module* c99bin_module = load_module("./bin/c99bin");
    if (!c99bin_module) {
        printf("❌ Failed to load c99bin module\n");
        module_system_cleanup();
        return 1;
    }
    printf("✅ C99Bin module loaded successfully\n");
    
    // 测试模块初始化
    printf("\nTesting module initialization...\n");
    void* init_func = c99bin_module->sym(c99bin_module, "module_init");
    if (init_func) {
        int (*module_init_func)(void) = (int (*)(void))init_func;
        int result = module_init_func();
        if (result == 0) {
            printf("✅ C99Bin module initialized successfully\n");
        } else {
            printf("❌ C99Bin module initialization failed: %d\n", result);
        }
    } else {
        printf("❌ module_init function not found\n");
    }

    // 设置依赖模块
    printf("\nSetting up dependency modules...\n");
    Module* pipeline_module = load_module("./bin/pipeline");
    Module* compiler_module = load_module("./bin/compiler");
    Module* layer0_module = load_module("./bin/layer0");

    void* set_deps_func = c99bin_module->sym(c99bin_module, "c99bin_set_dependencies");
    if (set_deps_func) {
        int (*set_dependencies)(Module*, Module*, Module*) =
            (int (*)(Module*, Module*, Module*))set_deps_func;
        int result = set_dependencies(pipeline_module, compiler_module, layer0_module);
        printf("✅ Dependencies set, result: %d\n", result);
    } else {
        printf("❌ c99bin_set_dependencies function not found\n");
    }
    
    // 测试编译函数
    printf("\nTesting compile function...\n");
    void* compile_func = c99bin_module->sym(c99bin_module, "c99bin_compile_to_executable");
    if (compile_func) {
        printf("✅ Found c99bin_compile_to_executable function\n");
        
        // 创建一个简单的测试C文件
        const char* test_c_content = "#include <stdio.h>\nint main() { printf(\"Hello World\\n\"); return 0; }\n";
        FILE* test_file = fopen("/tmp/test_hello.c", "w");
        if (test_file) {
            fprintf(test_file, "%s", test_c_content);
            fclose(test_file);
            printf("✅ Created test C file: /tmp/test_hello.c\n");
            
            // 调用编译函数
            int (*compile_to_executable)(const char*, const char*) = 
                (int (*)(const char*, const char*))compile_func;
            
            printf("\nCalling c99bin_compile_to_executable...\n");
            int compile_result = compile_to_executable("/tmp/test_hello.c", "/tmp/test_hello");
            
            printf("Compile result: %d\n", compile_result);
            
            // 获取错误信息
            void* get_error_func = c99bin_module->sym(c99bin_module, "c99bin_get_error");
            if (get_error_func) {
                const char* (*get_error)(void) = (const char* (*)(void))get_error_func;
                const char* error_msg = get_error();
                printf("Error message: %s\n", error_msg ? error_msg : "(null)");
            }
            
        } else {
            printf("❌ Failed to create test C file\n");
        }
    } else {
        printf("❌ c99bin_compile_to_executable function not found\n");
    }
    
    // 测试模块清理
    printf("\nTesting module cleanup...\n");
    void* cleanup_func = c99bin_module->sym(c99bin_module, "module_cleanup");
    if (cleanup_func) {
        void (*module_cleanup_func)(void) = (void (*)(void))cleanup_func;
        module_cleanup_func();
        printf("✅ C99Bin module cleanup completed\n");
    } else {
        printf("❌ module_cleanup function not found\n");
    }
    
    // 清理模块系统
    printf("\nCleaning up module system...\n");
    module_system_cleanup();
    printf("✅ Module system cleanup completed\n");
    
    printf("\n=== Frontend integration test completed ===\n");
    return 0;
}
