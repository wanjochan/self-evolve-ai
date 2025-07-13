#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../src/core/module.h"

// 外部声明module模块
extern Module module_module;

// 模拟一个简单的测试模块
static int test_module_init(void) {
    printf("Test Module: Initialized\n");
    return 0;
}

static void test_module_cleanup(void) {
    printf("Test Module: Cleaned up\n");
}

static void* test_module_resolve(const char* symbol) {
    if (strcmp(symbol, "test_function") == 0) {
        return (void*)0x12345678; // 模拟函数地址
    }
    return NULL;
}

Module test_module = {
    .name = "test_module",
    .state = MODULE_UNLOADED,
    .error = NULL,
    .init = test_module_init,
    .cleanup = test_module_cleanup,
    .resolve = test_module_resolve
};

int main() {
    printf("=== Module Dependencies Test ===\n");
    
    // 1. 初始化模块系统
    printf("1. Initializing module system...\n");
    if (module_module.init() != 0) {
        printf("ERROR: Failed to initialize module system\n");
        return 1;
    }
    printf("   ✓ Module system initialized successfully\n");

    // 1.5. 手动将测试模块添加到缓存中
    printf("\n1.5. Adding test module to cache...\n");
    void* add_to_cache_func = module_module.resolve("module_add_to_cache");
    int (*module_add_to_cache)(Module*) = (int (*)(Module*))add_to_cache_func;
    
    if (!module_add_to_cache) {
        printf("ERROR: Could not resolve module_add_to_cache function\n");
        return 1;
    }
    
    if (module_add_to_cache(&test_module) != 0) {
        printf("ERROR: Failed to add test module to cache\n");
        return 1;
    }
    printf("   ✓ Test module added to cache successfully\n");

    // 2. 测试注册单个依赖
    printf("\n2. Testing single dependency registration...\n");
    void* register_dep_func = module_module.resolve("module_register_dependency");
    int (*module_register_dependency)(Module*, const char*) = 
        (int (*)(Module*, const char*))register_dep_func;
    
    if (!module_register_dependency) {
        printf("ERROR: Could not resolve module_register_dependency function\n");
        return 1;
    }
    
    // 为测试模块注册依赖
    if (module_register_dependency(&test_module, "libc") == 0) {
        printf("   ✓ Successfully registered dependency: libc\n");
    } else {
        printf("   ERROR: Failed to register dependency: libc\n");
        return 1;
    }
    
    if (module_register_dependency(&test_module, "layer0") == 0) {
        printf("   ✓ Successfully registered dependency: layer0\n");
    } else {
        printf("   ERROR: Failed to register dependency: layer0\n");
        return 1;
    }
    
    // 3. 测试注册多个依赖
    printf("\n3. Testing multiple dependencies registration...\n");
    void* register_deps_func = module_module.resolve("module_register_dependencies");
    int (*module_register_dependencies)(Module*, const char**) = 
        (int (*)(Module*, const char**))register_deps_func;
    
    if (!module_register_dependencies) {
        printf("ERROR: Could not resolve module_register_dependencies function\n");
        return 1;
    }
    
    const char* deps[] = {"pipeline", "compiler", NULL};
    if (module_register_dependencies(&test_module, deps) == 0) {
        printf("   ✓ Successfully registered multiple dependencies\n");
    } else {
        printf("   WARNING: Some dependencies failed to register\n");
    }
    
    // 4. 测试获取依赖列表
    printf("\n4. Testing dependency retrieval...\n");
    void* get_deps_func = module_module.resolve("module_get_dependencies");
    const char** (*module_get_dependencies)(const Module*) = 
        (const char** (*)(const Module*))get_deps_func;
    
    if (!module_get_dependencies) {
        printf("ERROR: Could not resolve module_get_dependencies function\n");
        return 1;
    }
    
    const char** retrieved_deps = module_get_dependencies(&test_module);
    if (retrieved_deps) {
        printf("   Retrieved dependencies:\n");
        for (int i = 0; retrieved_deps[i] != NULL; i++) {
            printf("     - %s\n", retrieved_deps[i]);
        }
        printf("   ✓ Successfully retrieved dependencies\n");
        free(retrieved_deps);
    } else {
        printf("   WARNING: No dependencies retrieved\n");
    }
    
    // 5. 测试依赖解析
    printf("\n5. Testing dependency resolution...\n");
    void* resolve_deps_func = module_module.resolve("resolve_dependencies");
    int (*resolve_dependencies)(Module*) = (int (*)(Module*))resolve_deps_func;
    
    if (!resolve_dependencies) {
        printf("ERROR: Could not resolve resolve_dependencies function\n");
        return 1;
    }
    
    int resolution_result = resolve_dependencies(&test_module);
    if (resolution_result == 0) {
        printf("   ✓ All dependencies resolved successfully\n");
    } else {
        printf("   WARNING: Some dependencies could not be resolved (expected)\n");
    }
    
    // 6. 测试重复依赖注册
    printf("\n6. Testing duplicate dependency registration...\n");
    if (module_register_dependency(&test_module, "libc") == 0) {
        printf("   ✓ Duplicate dependency handled correctly\n");
    } else {
        printf("   ERROR: Duplicate dependency registration failed\n");
    }
    
    // 7. 清理
    printf("\n7. Cleaning up...\n");
    module_module.cleanup();
    printf("   ✓ Cleanup completed\n");
    
    printf("\n=== Test Completed ===\n");
    return 0;
}
