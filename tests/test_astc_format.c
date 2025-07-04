/**
 * test_astc_format.c - ASTC字节码格式验证测试
 * 
 * 测试ASTC字节码的文件头、指令集、数据结构等基本格式
 * 确保ASTC核心定义的正确性和一致性
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

// Include the ASTC core definitions
#include "../src/core/astc.h"
#include "../src/core/utils.h"

// Test framework macros
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s - %s\n", __func__, message); \
            return 0; \
        } \
    } while(0)

#define TEST_PASS() \
    do { \
        printf("PASS: %s\n", __func__); \
        return 1; \
    } while(0)

#define RUN_TEST(test_func) \
    do { \
        printf("Running %s...\n", #test_func); \
        if (test_func()) { \
            tests_passed++; \
        } else { \
            tests_failed++; \
        } \
        total_tests++; \
    } while(0)

// Global test counters
static int total_tests = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// ===============================================
// ASTC Format Tests
// ===============================================

int test_astc_node_types(void) {
    // Test basic ASTC node type definitions
    printf("  Testing ASTC node type definitions...\n");
    
    // Test WebAssembly compatibility
    TEST_ASSERT(AST_MODULE == 0x00, "AST_MODULE should match WebAssembly spec");
    TEST_ASSERT(AST_FUNC_TYPE == 0x60, "AST_FUNC_TYPE should match WebAssembly spec");
    TEST_ASSERT(AST_IMPORT == 0x02, "AST_IMPORT should match WebAssembly spec");
    
    // Test control flow instructions
    TEST_ASSERT(AST_UNREACHABLE == 0x00, "AST_UNREACHABLE should match WebAssembly spec");
    TEST_ASSERT(AST_NOP == 0x01, "AST_NOP should match WebAssembly spec");
    TEST_ASSERT(AST_BLOCK == 0x02, "AST_BLOCK should match WebAssembly spec");
    TEST_ASSERT(AST_LOOP == 0x03, "AST_LOOP should match WebAssembly spec");
    TEST_ASSERT(AST_IF == 0x04, "AST_IF should match WebAssembly spec");
    TEST_ASSERT(AST_ELSE == 0x05, "AST_ELSE should match WebAssembly spec");
    TEST_ASSERT(AST_END == 0x0B, "AST_END should match WebAssembly spec");
    
    // Test arithmetic instructions
    TEST_ASSERT(AST_I32_ADD == 0x6A, "AST_I32_ADD should match WebAssembly spec");
    TEST_ASSERT(AST_I32_SUB == 0x6B, "AST_I32_SUB should match WebAssembly spec");
    TEST_ASSERT(AST_I32_MUL == 0x6C, "AST_I32_MUL should match WebAssembly spec");
    
    // Test constant instructions
    TEST_ASSERT(AST_I32_CONST == 0x41, "AST_I32_CONST should match WebAssembly spec");
    TEST_ASSERT(AST_I64_CONST == 0x42, "AST_I64_CONST should match WebAssembly spec");
    TEST_ASSERT(AST_F32_CONST == 0x43, "AST_F32_CONST should match WebAssembly spec");
    TEST_ASSERT(AST_F64_CONST == 0x44, "AST_F64_CONST should match WebAssembly spec");
    
    printf("    WebAssembly compatibility: PASS\n");
    
    // Test ASTC extensions
    TEST_ASSERT(ASTC_TRANSLATION_UNIT > AST_TABLE_FILL, "ASTC extensions should not conflict with WebAssembly");
    TEST_ASSERT(ASTC_FUNC_DECL > ASTC_TRANSLATION_UNIT, "ASTC function declarations should be properly ordered");
    TEST_ASSERT(ASTC_VAR_DECL > ASTC_FUNC_DECL, "ASTC variable declarations should be properly ordered");
    
    printf("    ASTC extensions: PASS\n");
    
    TEST_PASS();
}

int test_astc_node_structure(void) {
    // Test ASTNode structure integrity
    printf("  Testing ASTNode structure...\n");
    
    // Test basic structure size and alignment
    size_t node_size = sizeof(ASTNode);
    TEST_ASSERT(node_size > 0, "ASTNode should have positive size");
    TEST_ASSERT(node_size < 1024, "ASTNode should not be excessively large");
    
    printf("    ASTNode size: %zu bytes\n", node_size);
    
    // Test node creation
    ASTNode* node = ast_create_node(ASTC_TRANSLATION_UNIT, 1, 1);
    TEST_ASSERT(node != NULL, "ast_create_node should succeed");
    TEST_ASSERT(node->type == ASTC_TRANSLATION_UNIT, "Node type should be set correctly");
    TEST_ASSERT(node->line == 1, "Node line should be set correctly");
    TEST_ASSERT(node->column == 1, "Node column should be set correctly");
    
    // Test node cleanup
    ast_free(node);
    printf("    Node creation and cleanup: PASS\n");
    
    TEST_PASS();
}

int test_astc_type_system(void) {
    // Test ASTC type system definitions
    printf("  Testing ASTC type system...\n");
    
    // Test basic C types
    TEST_ASSERT(ASTC_TYPE_VOID != ASTC_TYPE_INVALID, "VOID type should be valid");
    TEST_ASSERT(ASTC_TYPE_INT != ASTC_TYPE_INVALID, "INT type should be valid");
    TEST_ASSERT(ASTC_TYPE_CHAR != ASTC_TYPE_INVALID, "CHAR type should be valid");
    TEST_ASSERT(ASTC_TYPE_FLOAT != ASTC_TYPE_INVALID, "FLOAT type should be valid");
    TEST_ASSERT(ASTC_TYPE_DOUBLE != ASTC_TYPE_INVALID, "DOUBLE type should be valid");
    
    // Test C99 types
    TEST_ASSERT(ASTC_TYPE_BOOL != ASTC_TYPE_INVALID, "BOOL type should be valid (C99)");
    TEST_ASSERT(ASTC_TYPE_LONG_LONG != ASTC_TYPE_INVALID, "LONG_LONG type should be valid (C99)");
    
    // Test pointer and array types
    TEST_ASSERT(ASTC_TYPE_POINTER != ASTC_TYPE_INVALID, "POINTER type should be valid");
    TEST_ASSERT(ASTC_TYPE_ARRAY != ASTC_TYPE_INVALID, "ARRAY type should be valid");
    TEST_ASSERT(ASTC_TYPE_FUNCTION != ASTC_TYPE_INVALID, "FUNCTION type should be valid");
    
    // Test composite types
    TEST_ASSERT(ASTC_TYPE_STRUCT != ASTC_TYPE_INVALID, "STRUCT type should be valid");
    TEST_ASSERT(ASTC_TYPE_UNION != ASTC_TYPE_INVALID, "UNION type should be valid");
    TEST_ASSERT(ASTC_TYPE_ENUM != ASTC_TYPE_INVALID, "ENUM type should be valid");
    
    printf("    Type system definitions: PASS\n");
    
    TEST_PASS();
}

int test_astc_operators(void) {
    // Test ASTC operator definitions
    printf("  Testing ASTC operators...\n");
    
    // Test arithmetic operators
    TEST_ASSERT(ASTC_OP_ADD != ASTC_OP_UNKNOWN, "ADD operator should be defined");
    TEST_ASSERT(ASTC_OP_SUB != ASTC_OP_UNKNOWN, "SUB operator should be defined");
    TEST_ASSERT(ASTC_OP_MUL != ASTC_OP_UNKNOWN, "MUL operator should be defined");
    TEST_ASSERT(ASTC_OP_DIV != ASTC_OP_UNKNOWN, "DIV operator should be defined");
    TEST_ASSERT(ASTC_OP_MOD != ASTC_OP_UNKNOWN, "MOD operator should be defined");
    
    // Test comparison operators
    TEST_ASSERT(ASTC_OP_EQ != ASTC_OP_UNKNOWN, "EQ operator should be defined");
    TEST_ASSERT(ASTC_OP_NE != ASTC_OP_UNKNOWN, "NE operator should be defined");
    TEST_ASSERT(ASTC_OP_LT != ASTC_OP_UNKNOWN, "LT operator should be defined");
    TEST_ASSERT(ASTC_OP_LE != ASTC_OP_UNKNOWN, "LE operator should be defined");
    TEST_ASSERT(ASTC_OP_GT != ASTC_OP_UNKNOWN, "GT operator should be defined");
    TEST_ASSERT(ASTC_OP_GE != ASTC_OP_UNKNOWN, "GE operator should be defined");
    
    // Test logical operators
    TEST_ASSERT(ASTC_OP_AND != ASTC_OP_UNKNOWN, "AND operator should be defined");
    TEST_ASSERT(ASTC_OP_OR != ASTC_OP_UNKNOWN, "OR operator should be defined");
    TEST_ASSERT(ASTC_OP_XOR != ASTC_OP_UNKNOWN, "XOR operator should be defined");
    TEST_ASSERT(ASTC_OP_NOT != ASTC_OP_UNKNOWN, "NOT operator should be defined");
    TEST_ASSERT(ASTC_OP_LOGICAL_AND != ASTC_OP_UNKNOWN, "LOGICAL_AND operator should be defined");
    TEST_ASSERT(ASTC_OP_LOGICAL_OR != ASTC_OP_UNKNOWN, "LOGICAL_OR operator should be defined");
    
    // Test unary operators
    TEST_ASSERT(ASTC_OP_NEG != ASTC_OP_UNKNOWN, "NEG operator should be defined");
    TEST_ASSERT(ASTC_OP_POS != ASTC_OP_UNKNOWN, "POS operator should be defined");
    TEST_ASSERT(ASTC_OP_DEREF != ASTC_OP_UNKNOWN, "DEREF operator should be defined");
    TEST_ASSERT(ASTC_OP_ADDR != ASTC_OP_UNKNOWN, "ADDR operator should be defined");
    
    // Test assignment operator
    TEST_ASSERT(ASTC_OP_ASSIGN != ASTC_OP_UNKNOWN, "ASSIGN operator should be defined");
    
    printf("    Operator definitions: PASS\n");
    
    TEST_PASS();
}

int test_astc_module_system(void) {
    // Test ASTC module system definitions
    printf("  Testing ASTC module system...\n");
    
    // Test module node types
    TEST_ASSERT(ASTC_MODULE_DECL != ASTC_TYPE_INVALID, "MODULE_DECL should be defined");
    TEST_ASSERT(ASTC_EXPORT_DECL != ASTC_TYPE_INVALID, "EXPORT_DECL should be defined");
    TEST_ASSERT(ASTC_IMPORT_DECL != ASTC_TYPE_INVALID, "IMPORT_DECL should be defined");
    TEST_ASSERT(ASTC_REQUIRES_DECL != ASTC_TYPE_INVALID, "REQUIRES_DECL should be defined");
    TEST_ASSERT(ASTC_MODULE_ATTRIBUTE != ASTC_TYPE_INVALID, "MODULE_ATTRIBUTE should be defined");
    TEST_ASSERT(ASTC_SYMBOL_REF != ASTC_TYPE_INVALID, "SYMBOL_REF should be defined");
    
    // Test module creation functions exist
    // Note: We're just testing that the function declarations exist
    // The actual implementation testing will be in separate tests
    
    printf("    Module system definitions: PASS\n");
    
    TEST_PASS();
}

int test_astc_format_consistency(void) {
    // Test overall ASTC format consistency
    printf("  Testing ASTC format consistency...\n");
    
    // Test that WebAssembly and ASTC extensions don't overlap
    int wasm_max = AST_TABLE_FILL;
    int astc_min = ASTC_TRANSLATION_UNIT;
    TEST_ASSERT(astc_min > wasm_max, "ASTC extensions should not overlap with WebAssembly opcodes");
    
    // Test that type definitions are consistent
    TEST_ASSERT(ASTC_TYPE_INVALID < ASTC_TYPE_VOID, "Type ordering should be consistent");
    TEST_ASSERT(ASTC_TYPE_VOID < ASTC_TYPE_INT, "Basic types should be ordered");
    
    // Test that operators are properly grouped
    TEST_ASSERT(ASTC_OP_ADD < ASTC_OP_SUB, "Arithmetic operators should be grouped");
    TEST_ASSERT(ASTC_OP_SUB < ASTC_OP_MUL, "Arithmetic operators should be sequential");
    TEST_ASSERT(ASTC_OP_EQ < ASTC_OP_NE, "Comparison operators should be grouped");
    
    printf("    Format consistency: PASS\n");
    
    TEST_PASS();
}

// ===============================================
// Main Test Runner
// ===============================================

int main(void) {
    printf("=== ASTC Format Validation Tests ===\n\n");
    
    // Run all tests
    RUN_TEST(test_astc_node_types);
    RUN_TEST(test_astc_node_structure);
    RUN_TEST(test_astc_type_system);
    RUN_TEST(test_astc_operators);
    RUN_TEST(test_astc_module_system);
    RUN_TEST(test_astc_format_consistency);
    
    // Print summary
    printf("\n=== Test Summary ===\n");
    printf("Total tests: %d\n", total_tests);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Success rate: %.1f%%\n", (double)tests_passed / total_tests * 100.0);
    
    if (tests_failed == 0) {
        printf("\nAll ASTC format tests passed! ✓\n");
        printf("ASTC bytecode format is correctly defined and consistent.\n");
        return 0;
    } else {
        printf("\nSome ASTC format tests failed! ✗\n");
        return 1;
    }
}
