#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "../src/core/module.h"
#include "../src/core/astc.h"

// 使用动态加载而不是外部声明
// extern Module module_pipeline;

int main() {
    printf("=== ASTC Core Functions Test ===\n");

    // 1. 初始化模块系统
    printf("1. Initializing module system...\n");
    if (module_system_init() != 0) {
        printf("ERROR: Failed to initialize module system\n");
        return 1;
    }
    printf("   ✓ Module system initialized successfully\n");

    // 2. 加载pipeline模块
    printf("\n2. Loading pipeline module...\n");
    Module* pipeline = load_module("/mnt/persist/workspace/bin/pipeline");
    if (!pipeline) {
        printf("ERROR: Failed to load pipeline module\n");
        return 1;
    }
    printf("   ✓ Pipeline module loaded successfully\n");
    
    // 3. 测试pipeline编译函数
    printf("\n3. Testing pipeline compile function...\n");
    void* compile_func = module_resolve(pipeline, "pipeline_compile");
    if (!compile_func) {
        printf("ERROR: Could not resolve pipeline_compile function\n");
        return 1;
    }
    printf("   ✓ pipeline_compile function resolved successfully\n");

    // 4. 测试pipeline错误获取函数
    printf("\n4. Testing pipeline error function...\n");
    void* error_func = module_resolve(pipeline, "pipeline_get_error");
    if (!error_func) {
        printf("ERROR: Could not resolve pipeline_get_error function\n");
        return 1;
    }
    printf("   ✓ pipeline_get_error function resolved successfully\n");

    // 5. 测试pipeline执行函数
    printf("\n5. Testing pipeline execute function...\n");
    void* execute_func = module_resolve(pipeline, "pipeline_execute");
    if (!execute_func) {
        printf("ERROR: Could not resolve pipeline_execute function\n");
        return 1;
    }
    printf("   ✓ pipeline_execute function resolved successfully\n");
    
    // 6. 测试AST节点创建
    printf("\n6. Testing AST node creation...\n");
    ASTNode* test_node = ast_create_node(ASTC_MODULE_DECL, 1, 1);
    if (!test_node) {
        printf("ERROR: Failed to create AST node\n");
        return 1;
    }
    printf("   ✓ AST node created successfully (type: %d)\n", test_node->type);
    
    // 7. 测试AST节点打印
    printf("\n7. Testing AST node printing...\n");
    printf("   AST node details: ");
    ast_print(test_node, 0);
    printf("\n   ✓ AST node printed successfully\n");
    
    // 8. 测试pipeline汇编获取函数
    printf("\n8. Testing pipeline assembly function...\n");
    void* assembly_func = module_resolve(pipeline, "pipeline_get_assembly");
    if (!assembly_func) {
        printf("ERROR: Could not resolve pipeline_get_assembly function\n");
        return 1;
    }
    printf("   ✓ pipeline_get_assembly function resolved successfully\n");
    
    // 9. 测试AST节点释放
    printf("\n9. Testing AST node cleanup...\n");
    if (test_node) {
        ast_free(test_node);
        printf("   ✓ AST node freed successfully\n");
    }
    
    // 10. 清理
    printf("\n10. Cleaning up...\n");
    
    // 清理模块系统
    module_system_cleanup();
    printf("   ✓ Cleanup completed\n");
    
    printf("\n=== Test Completed Successfully ===\n");
    return 0;
}
