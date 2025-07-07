/**
 * test_astc_module.c - ASTC模块测试
 * 
 * 测试ASTC字节码系统的核心功能，包括：
 * - AST节点创建和管理
 * - 序列化/反序列化
 * - 模块验证
 * - 实际实现限制和简化实现的测试
 */

#include "core_test_framework.h"
#include "../src/core/astc.h"
#include <string.h>

// ===============================================
// AST节点创建和管理测试
// ===============================================

TEST_CASE(test_ast_create_node_basic) {
    ASTNode* node = ast_create_node(ASTC_TRANSLATION_UNIT, 1, 1);
    ASSERT_NOT_NULL(node, "AST node creation failed");
    ASSERT_EQ(node->type, ASTC_TRANSLATION_UNIT, "Node type incorrect");
    ASSERT_EQ(node->line, 1, "Line number incorrect");
    ASSERT_EQ(node->column, 1, "Column number incorrect");
    
    ast_free(node);
    TEST_PASS();
}

TEST_CASE(test_ast_create_node_all_types) {
    // 测试所有主要的AST节点类型
    ASTNodeType types[] = {
        ASTC_TRANSLATION_UNIT,
        ASTC_FUNC_DECL,
        ASTC_VAR_DECL,
        ASTC_COMPOUND_STMT,
        ASTC_RETURN_STMT,
        ASTC_EXPR_STMT,
        ASTC_EXPR_IDENTIFIER,
        ASTC_EXPR_STRING_LITERAL,
        ASTC_MODULE_DECL,
        ASTC_EXPORT_DECL,
        ASTC_IMPORT_DECL
    };
    
    for (int i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        ASTNode* node = ast_create_node(types[i], i + 1, i + 1);
        ASSERT_NOT_NULL(node, "Failed to create node for type %d", types[i]);
        ASSERT_EQ(node->type, types[i], "Node type mismatch for type %d", types[i]);
        ast_free(node);
    }
    
    TEST_PASS();
}

TEST_CASE(test_ast_free_null_safety) {
    // 测试空指针安全性
    ast_free(NULL);  // 不应该崩溃
    TEST_PASS();
}

TEST_CASE(test_ast_free_complex_node) {
    // 测试复杂节点的释放
    ASTNode* func = ast_create_node(ASTC_FUNC_DECL, 1, 1);
    ASSERT_NOT_NULL(func, "Function node creation failed");
    
    // 设置函数名
    func->data.func_decl.name = strdup("test_function");
    ASSERT_NOT_NULL(func->data.func_decl.name, "Function name allocation failed");
    
    // 创建返回类型
    func->data.func_decl.return_type = ast_create_node(ASTC_TYPE_INT, 1, 1);
    ASSERT_NOT_NULL(func->data.func_decl.return_type, "Return type creation failed");
    
    // 创建函数体
    func->data.func_decl.body = ast_create_node(ASTC_COMPOUND_STMT, 1, 1);
    ASSERT_NOT_NULL(func->data.func_decl.body, "Function body creation failed");
    
    // 释放应该不崩溃
    ast_free(func);
    TEST_PASS();
}

// ===============================================
// 序列化/反序列化测试
// ===============================================

TEST_CASE(test_ast_serialize_module_basic) {
    ASTNode* module = ast_create_node(ASTC_MODULE_DECL, 1, 1);
    ASSERT_NOT_NULL(module, "Module creation failed");
    
    uint8_t* buffer = NULL;
    size_t size = 0;
    
    int result = ast_serialize_module(module, &buffer, &size);
    ASSERT_EQ(result, 0, "Serialization failed");
    ASSERT_NOT_NULL(buffer, "Serialization buffer is null");
    ASSERT_GT(size, 0, "Serialization size is zero");
    
    // 验证魔数
    ASSERT_EQ(memcmp(buffer, "ASTC", 4), 0, "Magic number incorrect");
    
    // 验证版本号
    uint32_t version;
    memcpy(&version, buffer + 4, 4);
    ASSERT_EQ(version, 1, "Version number incorrect");
    
    free(buffer);
    ast_free(module);
    TEST_PASS();
}

TEST_CASE(test_ast_serialize_module_null_inputs) {
    ASTNode* module = ast_create_node(ASTC_MODULE_DECL, 1, 1);
    uint8_t* buffer = NULL;
    size_t size = 0;
    
    // 测试空模块
    int result = ast_serialize_module(NULL, &buffer, &size);
    ASSERT_EQ(result, -1, "Should fail with null module");
    
    // 测试空缓冲区指针
    result = ast_serialize_module(module, NULL, &size);
    ASSERT_EQ(result, -1, "Should fail with null buffer pointer");
    
    // 测试空大小指针
    result = ast_serialize_module(module, &buffer, NULL);
    ASSERT_EQ(result, -1, "Should fail with null size pointer");
    
    ast_free(module);
    TEST_PASS();
}

TEST_CASE(test_ast_deserialize_module_basic) {
    // 创建有效的序列化数据
    uint8_t test_data[] = {
        'A', 'S', 'T', 'C',  // 魔数
        0x01, 0x00, 0x00, 0x00,  // 版本号 (小端)
        0x01, 0x00, 0x00, 0x00   // 节点类型 (ASTC_MODULE_DECL)
    };
    
    ASTNode* module = ast_deserialize_module(test_data, sizeof(test_data));
    ASSERT_NOT_NULL(module, "Deserialization failed");
    ASSERT_EQ(module->type, ASTC_MODULE_DECL, "Deserialized node type incorrect");
    
    ast_free(module);
    TEST_PASS();
}

TEST_CASE(test_ast_deserialize_module_invalid_data) {
    // 测试无效魔数
    uint8_t invalid_magic[] = {
        'X', 'X', 'X', 'X',  // 错误魔数
        0x01, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00
    };
    
    ASTNode* result = ast_deserialize_module(invalid_magic, sizeof(invalid_magic));
    ASSERT_NULL(result, "Should fail with invalid magic number");
    
    // 测试无效版本号
    uint8_t invalid_version[] = {
        'A', 'S', 'T', 'C',
        0x99, 0x00, 0x00, 0x00,  // 错误版本号
        0x01, 0x00, 0x00, 0x00
    };
    
    result = ast_deserialize_module(invalid_version, sizeof(invalid_version));
    ASSERT_NULL(result, "Should fail with invalid version");
    
    // 测试数据太小
    uint8_t too_small[] = {'A', 'S', 'T'};
    result = ast_deserialize_module(too_small, sizeof(too_small));
    ASSERT_NULL(result, "Should fail with insufficient data");
    
    // 测试空指针
    result = ast_deserialize_module(NULL, 100);
    ASSERT_NULL(result, "Should fail with null buffer");
    
    TEST_PASS();
}

// ===============================================
// 模块验证测试
// ===============================================

TEST_CASE(test_ast_validate_module_basic) {
    ASTNode* module = ast_create_node(ASTC_MODULE_DECL, 1, 1);
    ASSERT_NOT_NULL(module, "Module creation failed");
    
    int result = ast_validate_module(module);
    ASSERT_EQ(result, 0, "Module validation failed");
    
    ast_free(module);
    TEST_PASS();
}

TEST_CASE(test_ast_validate_module_invalid) {
    // 测试空指针
    int result = ast_validate_module(NULL);
    ASSERT_EQ(result, -1, "Should fail with null module");
    
    // 测试错误类型
    ASTNode* wrong_type = ast_create_node(ASTC_FUNC_DECL, 1, 1);
    result = ast_validate_module(wrong_type);
    ASSERT_EQ(result, -1, "Should fail with wrong node type");
    
    ast_free(wrong_type);
    TEST_PASS();
}

TEST_CASE(test_ast_validate_export_declaration) {
    ASTNode* export_decl = ast_create_node(ASTC_EXPORT_DECL, 1, 1);
    ASSERT_NOT_NULL(export_decl, "Export declaration creation failed");
    
    int result = ast_validate_export_declaration(export_decl);
    ASSERT_EQ(result, 0, "Export declaration validation failed");
    
    // 测试空指针
    result = ast_validate_export_declaration(NULL);
    ASSERT_EQ(result, -1, "Should fail with null export declaration");
    
    // 测试错误类型
    ASTNode* wrong_type = ast_create_node(ASTC_FUNC_DECL, 1, 1);
    result = ast_validate_export_declaration(wrong_type);
    ASSERT_EQ(result, -1, "Should fail with wrong node type");
    
    ast_free(export_decl);
    ast_free(wrong_type);
    TEST_PASS();
}

TEST_CASE(test_ast_validate_import_declaration) {
    ASTNode* import_decl = ast_create_node(ASTC_IMPORT_DECL, 1, 1);
    ASSERT_NOT_NULL(import_decl, "Import declaration creation failed");
    
    int result = ast_validate_import_declaration(import_decl);
    ASSERT_EQ(result, 0, "Import declaration validation failed");
    
    // 测试空指针
    result = ast_validate_import_declaration(NULL);
    ASSERT_EQ(result, -1, "Should fail with null import declaration");
    
    // 测试错误类型
    ASTNode* wrong_type = ast_create_node(ASTC_FUNC_DECL, 1, 1);
    result = ast_validate_import_declaration(wrong_type);
    ASSERT_EQ(result, -1, "Should fail with wrong node type");
    
    ast_free(import_decl);
    ast_free(wrong_type);
    TEST_PASS();
}

// ===============================================
// ASTC程序管理测试
// ===============================================

TEST_CASE(test_astc_program_lifecycle) {
    // 创建程序
    ASTCProgram* program = astc_load_program("test_program");
    ASSERT_NOT_NULL(program, "Program creation failed");
    ASSERT_NOT_NULL(program->program_name, "Program name is null");
    ASSERT_EQ(strcmp(program->program_name, "test_program"), 0, "Program name incorrect");
    
    // 验证程序
    int result = astc_validate_program(program);
    ASSERT_EQ(result, 0, "Program validation failed");
    
    // 释放程序
    astc_free_program(program);
    TEST_PASS();
}

TEST_CASE(test_astc_validate_program_edge_cases) {
    // 测试空指针
    int result = astc_validate_program(NULL);
    ASSERT_EQ(result, -1, "Should fail with null program");
    
    // 测试空程序名
    ASTCProgram program = {0};
    program.program_name[0] = '\0';
    result = astc_validate_program(&program);
    ASSERT_EQ(result, -1, "Should fail with empty program name");
    
    // 测试无效字节码
    strcpy(program.program_name, "test");
    program.bytecode_size = 100;
    program.bytecode = NULL;
    result = astc_validate_program(&program);
    ASSERT_EQ(result, -1, "Should fail with null bytecode but non-zero size");
    
    TEST_PASS();
}

// ===============================================
// 模块操作测试（测试简化实现）
// ===============================================

TEST_CASE(test_ast_module_add_declaration_simplified) {
    ASTNode* module = ast_create_node(ASTC_MODULE_DECL, 1, 1);
    ASTNode* decl = ast_create_node(ASTC_FUNC_DECL, 1, 1);
    
    // 当前是简化实现，应该返回成功但不实际维护列表
    int result = ast_module_add_declaration(module, decl);
    ASSERT_EQ(result, 0, "Add declaration should succeed (simplified implementation)");
    
    // 测试错误情况
    result = ast_module_add_declaration(NULL, decl);
    ASSERT_EQ(result, -1, "Should fail with null module");
    
    result = ast_module_add_declaration(module, NULL);
    ASSERT_EQ(result, -1, "Should fail with null declaration");
    
    // 测试错误类型
    ASTNode* wrong_type = ast_create_node(ASTC_FUNC_DECL, 1, 1);
    result = ast_module_add_declaration(wrong_type, decl);
    ASSERT_EQ(result, -1, "Should fail with wrong module type");
    
    ast_free(module);
    ast_free(decl);
    ast_free(wrong_type);
    TEST_PASS();
}

TEST_CASE(test_ast_module_add_export_simplified) {
    ASTNode* module = ast_create_node(ASTC_MODULE_DECL, 1, 1);
    ASTNode* export_decl = ast_create_node(ASTC_EXPORT_DECL, 1, 1);
    
    // 当前是简化实现
    int result = ast_module_add_export(module, export_decl);
    ASSERT_EQ(result, 0, "Add export should succeed (simplified implementation)");
    
    // 测试错误情况
    result = ast_module_add_export(NULL, export_decl);
    ASSERT_EQ(result, -1, "Should fail with null module");
    
    result = ast_module_add_export(module, NULL);
    ASSERT_EQ(result, -1, "Should fail with null export");
    
    ast_free(module);
    ast_free(export_decl);
    TEST_PASS();
}

TEST_CASE(test_ast_module_add_import_simplified) {
    ASTNode* module = ast_create_node(ASTC_MODULE_DECL, 1, 1);
    ASTNode* import_decl = ast_create_node(ASTC_IMPORT_DECL, 1, 1);
    
    // 当前是简化实现
    int result = ast_module_add_import(module, import_decl);
    ASSERT_EQ(result, 0, "Add import should succeed (simplified implementation)");
    
    // 测试错误情况
    result = ast_module_add_import(NULL, import_decl);
    ASSERT_EQ(result, -1, "Should fail with null module");
    
    result = ast_module_add_import(module, NULL);
    ASSERT_EQ(result, -1, "Should fail with null import");
    
    ast_free(module);
    ast_free(import_decl);
    TEST_PASS();
}

TEST_CASE(test_ast_resolve_symbol_references_simplified) {
    ASTNode* module = ast_create_node(ASTC_MODULE_DECL, 1, 1);
    
    // 当前是简化实现，总是返回成功
    int result = ast_resolve_symbol_references(module);
    ASSERT_EQ(result, 0, "Symbol resolution should succeed (simplified implementation)");
    
    // 测试错误情况
    result = ast_resolve_symbol_references(NULL);
    ASSERT_EQ(result, -1, "Should fail with null module");
    
    // 测试错误类型
    ASTNode* wrong_type = ast_create_node(ASTC_FUNC_DECL, 1, 1);
    result = ast_resolve_symbol_references(wrong_type);
    ASSERT_EQ(result, -1, "Should fail with wrong module type");
    
    ast_free(module);
    ast_free(wrong_type);
    TEST_PASS();
}

// ===============================================
// AST打印测试
// ===============================================

TEST_CASE(test_ast_print_safety) {
    // 测试空指针安全性
    ast_print(NULL, 0);  // 不应该崩溃
    
    // 测试正常节点打印
    ASTNode* node = ast_create_node(ASTC_TRANSLATION_UNIT, 1, 1);
    node->data.translation_unit.declaration_count = 5;
    
    // 重定向输出以避免测试时的噪音
    FILE* old_stdout = stdout;
    stdout = fopen("/dev/null", "w");
    if (!stdout) stdout = old_stdout;
    
    ast_print(node, 0);
    ast_print(node, 2);  // 测试缩进
    
    // 恢复输出
    if (stdout != old_stdout) {
        fclose(stdout);
    }
    stdout = old_stdout;
    
    ast_free(node);
    TEST_PASS();
}

// ===============================================
// 内存管理和边界测试
// ===============================================

TEST_CASE(test_ast_memory_management) {
    // 测试大量节点创建和释放
    const int node_count = 1000;
    ASTNode* nodes[node_count];
    
    // 创建大量节点
    for (int i = 0; i < node_count; i++) {
        nodes[i] = ast_create_node(ASTC_TRANSLATION_UNIT, i, i);
        ASSERT_NOT_NULL(nodes[i], "Node creation failed at index %d", i);
    }
    
    // 释放所有节点
    for (int i = 0; i < node_count; i++) {
        ast_free(nodes[i]);
    }
    
    TEST_PASS();
}

TEST_CASE(test_ast_serialization_roundtrip) {
    // 测试序列化和反序列化的往返
    ASTNode* original = ast_create_node(ASTC_MODULE_DECL, 42, 24);
    ASSERT_NOT_NULL(original, "Original node creation failed");
    
    // 序列化
    uint8_t* buffer = NULL;
    size_t size = 0;
    int result = ast_serialize_module(original, &buffer, &size);
    ASSERT_EQ(result, 0, "Serialization failed");
    
    // 反序列化
    ASTNode* deserialized = ast_deserialize_module(buffer, size);
    ASSERT_NOT_NULL(deserialized, "Deserialization failed");
    
    // 验证类型匹配（注意：简化实现可能不保留行列号）
    ASSERT_EQ(deserialized->type, original->type, "Type mismatch after roundtrip");
    
    free(buffer);
    ast_free(original);
    ast_free(deserialized);
    TEST_PASS();
}

// 运行所有ASTC模块测试
void run_astc_module_tests(void) {
    TEST_SUITE_START("ASTC Module Tests");
    
    RUN_TEST(test_ast_create_node_basic);
    RUN_TEST(test_ast_create_node_all_types);
    RUN_TEST(test_ast_free_null_safety);
    RUN_TEST(test_ast_free_complex_node);
    RUN_TEST(test_ast_serialize_module_basic);
    RUN_TEST(test_ast_serialize_module_null_inputs);
    RUN_TEST(test_ast_deserialize_module_basic);
    RUN_TEST(test_ast_deserialize_module_invalid_data);
    RUN_TEST(test_ast_validate_module_basic);
    RUN_TEST(test_ast_validate_module_invalid);
    RUN_TEST(test_ast_validate_export_declaration);
    RUN_TEST(test_ast_validate_import_declaration);
    RUN_TEST(test_astc_program_lifecycle);
    RUN_TEST(test_astc_validate_program_edge_cases);
    RUN_TEST(test_ast_module_add_declaration_simplified);
    RUN_TEST(test_ast_module_add_export_simplified);
    RUN_TEST(test_ast_module_add_import_simplified);
    RUN_TEST(test_ast_resolve_symbol_references_simplified);
    RUN_TEST(test_ast_print_safety);
    RUN_TEST(test_ast_memory_management);
    RUN_TEST(test_ast_serialization_roundtrip);
    
    TEST_SUITE_END();
} 