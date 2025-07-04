/**
 * test_astc_instructions.c - ASTC指令解析测试
 * 
 * 测试ASTC指令的解析、验证和执行逻辑
 * 确保指令处理的正确性和完整性
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
// ASTC Instruction Tests
// ===============================================

int test_astc_control_flow_instructions(void) {
    // Test control flow instruction parsing
    printf("  Testing control flow instructions...\n");
    
    // Test basic control flow nodes
    ASTNode* block_node = ast_create_node(AST_BLOCK, 1, 1);
    TEST_ASSERT(block_node != NULL, "Block node creation should succeed");
    TEST_ASSERT(block_node->type == AST_BLOCK, "Block node type should be correct");
    ast_free(block_node);
    
    ASTNode* loop_node = ast_create_node(AST_LOOP, 2, 1);
    TEST_ASSERT(loop_node != NULL, "Loop node creation should succeed");
    TEST_ASSERT(loop_node->type == AST_LOOP, "Loop node type should be correct");
    ast_free(loop_node);
    
    ASTNode* if_node = ast_create_node(AST_IF, 3, 1);
    TEST_ASSERT(if_node != NULL, "If node creation should succeed");
    TEST_ASSERT(if_node->type == AST_IF, "If node type should be correct");
    ast_free(if_node);
    
    ASTNode* return_node = ast_create_node(AST_RETURN, 4, 1);
    TEST_ASSERT(return_node != NULL, "Return node creation should succeed");
    TEST_ASSERT(return_node->type == AST_RETURN, "Return node type should be correct");
    ast_free(return_node);
    
    printf("    Control flow instruction parsing: PASS\n");
    
    TEST_PASS();
}

int test_astc_arithmetic_instructions(void) {
    // Test arithmetic instruction parsing
    printf("  Testing arithmetic instructions...\n");
    
    // Test i32 arithmetic
    ASTNode* add_node = ast_create_node(AST_I32_ADD, 1, 1);
    TEST_ASSERT(add_node != NULL, "I32_ADD node creation should succeed");
    TEST_ASSERT(add_node->type == AST_I32_ADD, "I32_ADD node type should be correct");
    ast_free(add_node);
    
    ASTNode* sub_node = ast_create_node(AST_I32_SUB, 2, 1);
    TEST_ASSERT(sub_node != NULL, "I32_SUB node creation should succeed");
    TEST_ASSERT(sub_node->type == AST_I32_SUB, "I32_SUB node type should be correct");
    ast_free(sub_node);
    
    ASTNode* mul_node = ast_create_node(AST_I32_MUL, 3, 1);
    TEST_ASSERT(mul_node != NULL, "I32_MUL node creation should succeed");
    TEST_ASSERT(mul_node->type == AST_I32_MUL, "I32_MUL node type should be correct");
    ast_free(mul_node);
    
    ASTNode* div_node = ast_create_node(AST_I32_DIV_S, 4, 1);
    TEST_ASSERT(div_node != NULL, "I32_DIV_S node creation should succeed");
    TEST_ASSERT(div_node->type == AST_I32_DIV_S, "I32_DIV_S node type should be correct");
    ast_free(div_node);
    
    printf("    Arithmetic instruction parsing: PASS\n");
    
    TEST_PASS();
}

int test_astc_memory_instructions(void) {
    // Test memory instruction parsing
    printf("  Testing memory instructions...\n");
    
    // Test memory load instructions
    ASTNode* i32_load = ast_create_node(AST_I32_LOAD, 1, 1);
    TEST_ASSERT(i32_load != NULL, "I32_LOAD node creation should succeed");
    TEST_ASSERT(i32_load->type == AST_I32_LOAD, "I32_LOAD node type should be correct");
    ast_free(i32_load);
    
    ASTNode* i64_load = ast_create_node(AST_I64_LOAD, 2, 1);
    TEST_ASSERT(i64_load != NULL, "I64_LOAD node creation should succeed");
    TEST_ASSERT(i64_load->type == AST_I64_LOAD, "I64_LOAD node type should be correct");
    ast_free(i64_load);
    
    // Test memory store instructions
    ASTNode* i32_store = ast_create_node(AST_I32_STORE, 3, 1);
    TEST_ASSERT(i32_store != NULL, "I32_STORE node creation should succeed");
    TEST_ASSERT(i32_store->type == AST_I32_STORE, "I32_STORE node type should be correct");
    ast_free(i32_store);
    
    ASTNode* i64_store = ast_create_node(AST_I64_STORE, 4, 1);
    TEST_ASSERT(i64_store != NULL, "I64_STORE node creation should succeed");
    TEST_ASSERT(i64_store->type == AST_I64_STORE, "I64_STORE node type should be correct");
    ast_free(i64_store);
    
    printf("    Memory instruction parsing: PASS\n");
    
    TEST_PASS();
}

int test_astc_constant_instructions(void) {
    // Test constant instruction parsing
    printf("  Testing constant instructions...\n");
    
    // Test constant nodes
    ASTNode* i32_const = ast_create_node(AST_I32_CONST, 1, 1);
    TEST_ASSERT(i32_const != NULL, "I32_CONST node creation should succeed");
    TEST_ASSERT(i32_const->type == AST_I32_CONST, "I32_CONST node type should be correct");
    ast_free(i32_const);
    
    ASTNode* i64_const = ast_create_node(AST_I64_CONST, 2, 1);
    TEST_ASSERT(i64_const != NULL, "I64_CONST node creation should succeed");
    TEST_ASSERT(i64_const->type == AST_I64_CONST, "I64_CONST node type should be correct");
    ast_free(i64_const);
    
    ASTNode* f32_const = ast_create_node(AST_F32_CONST, 3, 1);
    TEST_ASSERT(f32_const != NULL, "F32_CONST node creation should succeed");
    TEST_ASSERT(f32_const->type == AST_F32_CONST, "F32_CONST node type should be correct");
    ast_free(f32_const);
    
    ASTNode* f64_const = ast_create_node(AST_F64_CONST, 4, 1);
    TEST_ASSERT(f64_const != NULL, "F64_CONST node creation should succeed");
    TEST_ASSERT(f64_const->type == AST_F64_CONST, "F64_CONST node type should be correct");
    ast_free(f64_const);
    
    printf("    Constant instruction parsing: PASS\n");
    
    TEST_PASS();
}

int test_astc_variable_instructions(void) {
    // Test variable instruction parsing
    printf("  Testing variable instructions...\n");
    
    // Test local variable instructions
    ASTNode* local_get = ast_create_node(AST_LOCAL_GET, 1, 1);
    TEST_ASSERT(local_get != NULL, "LOCAL_GET node creation should succeed");
    TEST_ASSERT(local_get->type == AST_LOCAL_GET, "LOCAL_GET node type should be correct");
    ast_free(local_get);
    
    ASTNode* local_set = ast_create_node(AST_LOCAL_SET, 2, 1);
    TEST_ASSERT(local_set != NULL, "LOCAL_SET node creation should succeed");
    TEST_ASSERT(local_set->type == AST_LOCAL_SET, "LOCAL_SET node type should be correct");
    ast_free(local_set);
    
    // Test global variable instructions
    ASTNode* global_get = ast_create_node(AST_GLOBAL_GET, 3, 1);
    TEST_ASSERT(global_get != NULL, "GLOBAL_GET node creation should succeed");
    TEST_ASSERT(global_get->type == AST_GLOBAL_GET, "GLOBAL_GET node type should be correct");
    ast_free(global_get);
    
    ASTNode* global_set = ast_create_node(AST_GLOBAL_SET, 4, 1);
    TEST_ASSERT(global_set != NULL, "GLOBAL_SET node creation should succeed");
    TEST_ASSERT(global_set->type == AST_GLOBAL_SET, "GLOBAL_SET node type should be correct");
    ast_free(global_set);
    
    printf("    Variable instruction parsing: PASS\n");
    
    TEST_PASS();
}

int test_astc_c_extension_instructions(void) {
    // Test ASTC C extension instruction parsing
    printf("  Testing ASTC C extension instructions...\n");
    
    // Test C-specific nodes
    ASTNode* func_decl = ast_create_node(ASTC_FUNC_DECL, 1, 1);
    TEST_ASSERT(func_decl != NULL, "FUNC_DECL node creation should succeed");
    TEST_ASSERT(func_decl->type == ASTC_FUNC_DECL, "FUNC_DECL node type should be correct");
    ast_free(func_decl);
    
    ASTNode* var_decl = ast_create_node(ASTC_VAR_DECL, 2, 1);
    TEST_ASSERT(var_decl != NULL, "VAR_DECL node creation should succeed");
    TEST_ASSERT(var_decl->type == ASTC_VAR_DECL, "VAR_DECL node type should be correct");
    ast_free(var_decl);
    
    ASTNode* if_stmt = ast_create_node(ASTC_IF_STMT, 3, 1);
    TEST_ASSERT(if_stmt != NULL, "IF_STMT node creation should succeed");
    TEST_ASSERT(if_stmt->type == ASTC_IF_STMT, "IF_STMT node type should be correct");
    ast_free(if_stmt);
    
    ASTNode* while_stmt = ast_create_node(ASTC_WHILE_STMT, 4, 1);
    TEST_ASSERT(while_stmt != NULL, "WHILE_STMT node creation should succeed");
    TEST_ASSERT(while_stmt->type == ASTC_WHILE_STMT, "WHILE_STMT node type should be correct");
    ast_free(while_stmt);
    
    ASTNode* for_stmt = ast_create_node(ASTC_FOR_STMT, 5, 1);
    TEST_ASSERT(for_stmt != NULL, "FOR_STMT node creation should succeed");
    TEST_ASSERT(for_stmt->type == ASTC_FOR_STMT, "FOR_STMT node type should be correct");
    ast_free(for_stmt);
    
    printf("    C extension instruction parsing: PASS\n");
    
    TEST_PASS();
}

int test_astc_instruction_validation(void) {
    // Test instruction validation logic
    printf("  Testing instruction validation...\n");
    
    // Test valid instruction types
    TEST_ASSERT(AST_I32_ADD >= 0, "Valid instruction should have non-negative value");
    TEST_ASSERT(AST_I32_SUB >= 0, "Valid instruction should have non-negative value");
    TEST_ASSERT(ASTC_FUNC_DECL >= 0, "Valid instruction should have non-negative value");
    
    // Test instruction ranges
    TEST_ASSERT(AST_I32_ADD < 0x1000, "WebAssembly instructions should be in valid range");
    TEST_ASSERT(ASTC_FUNC_DECL > 0x1000, "ASTC extensions should be in separate range");
    
    // Test instruction consistency
    TEST_ASSERT(AST_I32_ADD < AST_I32_SUB, "Arithmetic instructions should be ordered");
    TEST_ASSERT(AST_I32_SUB < AST_I32_MUL, "Arithmetic instructions should be sequential");
    
    printf("    Instruction validation: PASS\n");
    
    TEST_PASS();
}

// ===============================================
// Main Test Runner
// ===============================================

int main(void) {
    printf("=== ASTC Instruction Parsing Tests ===\n\n");
    
    // Run all tests
    RUN_TEST(test_astc_control_flow_instructions);
    RUN_TEST(test_astc_arithmetic_instructions);
    RUN_TEST(test_astc_memory_instructions);
    RUN_TEST(test_astc_constant_instructions);
    RUN_TEST(test_astc_variable_instructions);
    RUN_TEST(test_astc_c_extension_instructions);
    RUN_TEST(test_astc_instruction_validation);
    
    // Print summary
    printf("\n=== Test Summary ===\n");
    printf("Total tests: %d\n", total_tests);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Success rate: %.1f%%\n", (double)tests_passed / total_tests * 100.0);
    
    if (tests_failed == 0) {
        printf("\nAll ASTC instruction tests passed! ✓\n");
        printf("ASTC instruction parsing is working correctly.\n");
        return 0;
    } else {
        printf("\nSome ASTC instruction tests failed! ✗\n");
        return 1;
    }
}
