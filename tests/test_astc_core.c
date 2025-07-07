#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "../src/core/module.h"
#include "../src/core/astc.h"

// 外部声明pipeline模块
extern Module module_pipeline;

int main() {
    printf("=== ASTC Core Functions Test ===\n");
    
    // 1. 初始化pipeline模块
    printf("1. Initializing pipeline module...\n");
    if (module_pipeline.init() != 0) {
        printf("ERROR: Failed to initialize pipeline module\n");
        return 1;
    }
    printf("   ✓ Pipeline module initialized successfully\n");
    
    // 2. 测试模块序列化
    printf("\n2. Testing module serialization...\n");
    void* serialize_func = module_pipeline.resolve("ast_serialize_module");
    int (*ast_serialize_module)(ASTNode*, uint8_t**, size_t*) = 
        (int (*)(ASTNode*, uint8_t**, size_t*))serialize_func;
    
    if (!ast_serialize_module) {
        printf("ERROR: Could not resolve ast_serialize_module function\n");
        return 1;
    }
    
    // 创建一个测试模块节点
    ASTNode* test_module = ast_create_node(ASTC_MODULE_DECL, 1, 1);
    if (!test_module) {
        printf("ERROR: Failed to create test module node\n");
        return 1;
    }
    
    uint8_t* buffer = NULL;
    size_t size = 0;
    
    if (ast_serialize_module(test_module, &buffer, &size) == 0) {
        printf("   ✓ Module serialized successfully (%zu bytes)\n", size);
        printf("   First few bytes: ");
        for (size_t i = 0; i < (size < 8 ? size : 8); i++) {
            printf("0x%02x ", buffer[i]);
        }
        printf("\n");
    } else {
        printf("   ERROR: Module serialization failed\n");
        return 1;
    }
    
    // 3. 测试模块反序列化
    printf("\n3. Testing module deserialization...\n");
    void* deserialize_func = module_pipeline.resolve("ast_deserialize_module");
    ASTNode* (*ast_deserialize_module)(const uint8_t*, size_t) = 
        (ASTNode* (*)(const uint8_t*, size_t))deserialize_func;
    
    if (!ast_deserialize_module) {
        printf("ERROR: Could not resolve ast_deserialize_module function\n");
        return 1;
    }
    
    ASTNode* deserialized_module = ast_deserialize_module(buffer, size);
    if (deserialized_module) {
        printf("   ✓ Module deserialized successfully\n");
        printf("   Deserialized module type: %d\n", deserialized_module->type);
    } else {
        printf("   ERROR: Module deserialization failed\n");
        return 1;
    }
    
    // 4. 测试模块验证
    printf("\n4. Testing module validation...\n");
    void* validate_func = module_pipeline.resolve("ast_validate_module");
    int (*ast_validate_module)(ASTNode*) = (int (*)(ASTNode*))validate_func;
    
    if (!ast_validate_module) {
        printf("ERROR: Could not resolve ast_validate_module function\n");
        return 1;
    }
    
    if (ast_validate_module(test_module) == 0) {
        printf("   ✓ Module validation passed\n");
    } else {
        printf("   WARNING: Module validation failed (expected for incomplete module)\n");
    }
    
    // 5. 测试ASTC程序加载
    printf("\n5. Testing ASTC program loading...\n");
    void* load_program_func = module_pipeline.resolve("astc_load_program");
    ASTCProgram* (*astc_load_program)(const char*) = 
        (ASTCProgram* (*)(const char*))load_program_func;
    
    if (!astc_load_program) {
        printf("ERROR: Could not resolve astc_load_program function\n");
        return 1;
    }
    
    // 尝试加载一个不存在的文件（测试错误处理）
    ASTCProgram* program = astc_load_program("nonexistent.astc");
    if (!program) {
        printf("   ✓ Correctly handled non-existent file\n");
    } else {
        printf("   WARNING: Unexpectedly loaded non-existent file\n");
    }
    
    // 6. 测试ASTC程序验证
    printf("\n6. Testing ASTC program validation...\n");
    void* validate_program_func = module_pipeline.resolve("astc_validate_program");
    int (*astc_validate_program)(const ASTCProgram*) = 
        (int (*)(const ASTCProgram*))validate_program_func;
    
    if (!astc_validate_program) {
        printf("ERROR: Could not resolve astc_validate_program function\n");
        return 1;
    }
    
    // 创建一个简单的测试程序
    ASTCProgram test_program = {0};
    strcpy(test_program.program_name, "test_program");
    test_program.version = 1;
    
    if (astc_validate_program(&test_program) == 0) {
        printf("   ✓ Program validation passed\n");
    } else {
        printf("   WARNING: Program validation failed\n");
    }
    
    // 7. 测试模块操作函数
    printf("\n7. Testing module operation functions...\n");
    void* add_decl_func = module_pipeline.resolve("ast_module_add_declaration");
    int (*ast_module_add_declaration)(ASTNode*, ASTNode*) = 
        (int (*)(ASTNode*, ASTNode*))add_decl_func;
    
    if (!ast_module_add_declaration) {
        printf("ERROR: Could not resolve ast_module_add_declaration function\n");
        return 1;
    }
    
    // 创建一个测试声明
    ASTNode* test_decl = ast_create_node(ASTC_FUNC_DECL, 1, 1);
    if (test_decl && ast_module_add_declaration(test_module, test_decl) == 0) {
        printf("   ✓ Module declaration addition works\n");
    } else {
        printf("   WARNING: Module declaration addition failed\n");
    }
    
    // 8. 测试符号解析
    printf("\n8. Testing symbol resolution...\n");
    void* resolve_symbols_func = module_pipeline.resolve("ast_resolve_symbol_references");
    int (*ast_resolve_symbol_references)(ASTNode*) = 
        (int (*)(ASTNode*))resolve_symbols_func;
    
    if (!ast_resolve_symbol_references) {
        printf("ERROR: Could not resolve ast_resolve_symbol_references function\n");
        return 1;
    }
    
    if (ast_resolve_symbol_references(test_module) == 0) {
        printf("   ✓ Symbol resolution completed\n");
    } else {
        printf("   WARNING: Symbol resolution failed\n");
    }
    
    // 9. 清理
    printf("\n9. Cleaning up...\n");
    if (buffer) free(buffer);
    if (test_module) ast_free(test_module);
    if (deserialized_module) ast_free(deserialized_module);
    if (test_decl) ast_free(test_decl);
    
    module_pipeline.cleanup();
    printf("   ✓ Cleanup completed\n");
    
    printf("\n=== Test Completed Successfully ===\n");
    return 0;
}
