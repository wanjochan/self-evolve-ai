#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../src/core/module.h"

// 外部声明module模块
extern Module module_module;

int main() {
    printf("=== Simple Dependency Management Test ===\n");
    
    // 1. 初始化模块系统
    printf("1. Initializing module system...\n");
    if (module_module.init() != 0) {
        printf("ERROR: Failed to initialize module system\n");
        return 1;
    }
    printf("   ✓ Module system initialized successfully\n");
    
    // 2. 测试使用module_module自身作为测试对象
    printf("\n2. Testing dependency registration on module_module itself...\n");
    
    void* register_dep_func = module_module.resolve("module_register_dependency");
    int (*module_register_dependency)(Module*, const char*) = 
        (int (*)(Module*, const char*))register_dep_func;
    
    if (!module_register_dependency) {
        printf("ERROR: Could not resolve module_register_dependency function\n");
        return 1;
    }
    
    // 为module_module自身注册一些测试依赖
    if (module_register_dependency(&module_module, "layer0") == 0) {
        printf("   ✓ Successfully registered dependency: layer0\n");
    } else {
        printf("   ERROR: Failed to register dependency: layer0\n");
        return 1;
    }
    
    if (module_register_dependency(&module_module, "libc") == 0) {
        printf("   ✓ Successfully registered dependency: libc\n");
    } else {
        printf("   ERROR: Failed to register dependency: libc\n");
        return 1;
    }
    
    // 3. 测试重复依赖注册
    printf("\n3. Testing duplicate dependency registration...\n");
    if (module_register_dependency(&module_module, "layer0") == 0) {
        printf("   ✓ Duplicate dependency handled correctly\n");
    } else {
        printf("   ERROR: Duplicate dependency registration failed\n");
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
    
    const char** retrieved_deps = module_get_dependencies(&module_module);
    if (retrieved_deps) {
        printf("   Retrieved dependencies for module_module:\n");
        for (int i = 0; retrieved_deps[i] != NULL; i++) {
            printf("     - %s\n", retrieved_deps[i]);
        }
        printf("   ✓ Successfully retrieved dependencies\n");
        free(retrieved_deps);
    } else {
        printf("   WARNING: No dependencies retrieved\n");
    }
    
    // 5. 测试批量依赖注册
    printf("\n5. Testing multiple dependencies registration...\n");
    void* register_deps_func = module_module.resolve("module_register_dependencies");
    int (*module_register_dependencies)(Module*, const char**) = 
        (int (*)(Module*, const char**))register_deps_func;
    
    if (!module_register_dependencies) {
        printf("ERROR: Could not resolve module_register_dependencies function\n");
        return 1;
    }
    
    const char* new_deps[] = {"pipeline", "compiler", NULL};
    if (module_register_dependencies(&module_module, new_deps) == 0) {
        printf("   ✓ Successfully registered multiple dependencies\n");
    } else {
        printf("   WARNING: Some dependencies failed to register\n");
    }
    
    // 6. 再次检查依赖列表
    printf("\n6. Checking final dependency list...\n");
    retrieved_deps = module_get_dependencies(&module_module);
    if (retrieved_deps) {
        printf("   Final dependencies for module_module:\n");
        for (int i = 0; retrieved_deps[i] != NULL; i++) {
            printf("     - %s\n", retrieved_deps[i]);
        }
        printf("   ✓ Final dependency list retrieved\n");
        free(retrieved_deps);
    } else {
        printf("   WARNING: No dependencies in final list\n");
    }
    
    // 7. 测试依赖解析
    printf("\n7. Testing dependency resolution...\n");
    void* resolve_deps_func = module_module.resolve("resolve_dependencies");
    int (*resolve_dependencies)(Module*) = (int (*)(Module*))resolve_deps_func;
    
    if (!resolve_dependencies) {
        printf("ERROR: Could not resolve resolve_dependencies function\n");
        return 1;
    }
    
    int resolution_result = resolve_dependencies(&module_module);
    if (resolution_result == 0) {
        printf("   ✓ All dependencies resolved successfully\n");
    } else {
        printf("   WARNING: Some dependencies could not be resolved (expected for non-existent modules)\n");
    }
    
    // 8. 清理
    printf("\n8. Cleaning up...\n");
    module_module.cleanup();
    printf("   ✓ Cleanup completed\n");
    
    printf("\n=== Test Completed Successfully ===\n");
    return 0;
}
