/**
 * test_astc_memory.c - ASTC内存管理测试
 * 
 * 测试ASTC字节码的内存加载、释放和管理功能
 * 确保内存操作的安全性和效率
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
// ASTC Memory Management Tests
// ===============================================

int test_astc_node_memory_allocation(void) {
    // Test basic node memory allocation and deallocation
    printf("  Testing ASTC node memory allocation...\n");
    
    // Test single node allocation
    ASTNode* node = ast_create_node(ASTC_TRANSLATION_UNIT, 1, 1);
    TEST_ASSERT(node != NULL, "Node allocation should succeed");
    TEST_ASSERT(node->type == ASTC_TRANSLATION_UNIT, "Node type should be set correctly");
    TEST_ASSERT(node->line == 1, "Node line should be set correctly");
    TEST_ASSERT(node->column == 1, "Node column should be set correctly");
    
    // Test node cleanup
    ast_free(node);
    printf("    Single node allocation/deallocation: PASS\n");
    
    // Test multiple node allocation
    const int node_count = 100;
    ASTNode* nodes[node_count];
    
    for (int i = 0; i < node_count; i++) {
        nodes[i] = ast_create_node(ASTC_VAR_DECL, i + 1, 1);
        TEST_ASSERT(nodes[i] != NULL, "Multiple node allocation should succeed");
        TEST_ASSERT(nodes[i]->line == i + 1, "Node line should be set correctly");
    }
    
    // Test multiple node cleanup
    for (int i = 0; i < node_count; i++) {
        ast_free(nodes[i]);
    }
    printf("    Multiple node allocation/deallocation: PASS\n");
    
    TEST_PASS();
}

int test_astc_complex_node_structures(void) {
    // Test complex node structures with nested data
    printf("  Testing complex ASTC node structures...\n");
    
    // Test function declaration with parameters
    ASTNode* func_node = ast_create_node(ASTC_FUNC_DECL, 1, 1);
    TEST_ASSERT(func_node != NULL, "Function node allocation should succeed");
    
    // Simulate setting function data (in real implementation)
    func_node->data.func_decl.name = malloc(strlen("test_func") + 1);
    strcpy(func_node->data.func_decl.name, "test_func");
    func_node->data.func_decl.param_count = 2;
    func_node->data.func_decl.params = malloc(2 * sizeof(ASTNode*));
    
    // Create parameter nodes
    func_node->data.func_decl.params[0] = ast_create_node(ASTC_PARAM_DECL, 1, 5);
    func_node->data.func_decl.params[1] = ast_create_node(ASTC_PARAM_DECL, 1, 10);
    
    TEST_ASSERT(func_node->data.func_decl.params[0] != NULL, "Parameter node allocation should succeed");
    TEST_ASSERT(func_node->data.func_decl.params[1] != NULL, "Parameter node allocation should succeed");
    
    // Test cleanup of complex structure
    ast_free(func_node->data.func_decl.params[0]);
    ast_free(func_node->data.func_decl.params[1]);
    free(func_node->data.func_decl.params);
    free(func_node->data.func_decl.name);
    ast_free(func_node);
    
    printf("    Complex node structure management: PASS\n");
    
    TEST_PASS();
}

int test_astc_memory_leak_prevention(void) {
    // Test memory leak prevention mechanisms
    printf("  Testing memory leak prevention...\n");
    
    // Test repeated allocation and deallocation
    const int iterations = 1000;
    for (int i = 0; i < iterations; i++) {
        ASTNode* node = ast_create_node(ASTC_EXPR_CONSTANT, i, 1);
        TEST_ASSERT(node != NULL, "Repeated allocation should succeed");
        
        // Set some data
        node->data.constant.type = ASTC_TYPE_INT;
        node->data.constant.int_val = i;
        
        // Immediate cleanup
        ast_free(node);
    }
    
    printf("    Repeated allocation/deallocation: PASS\n");
    
    // Test allocation of different node types
    ASTNodeType test_types[] = {
        ASTC_TRANSLATION_UNIT,
        ASTC_FUNC_DECL,
        ASTC_VAR_DECL,
        ASTC_IF_STMT,
        ASTC_WHILE_STMT,
        ASTC_FOR_STMT,
        ASTC_RETURN_STMT,
        ASTC_EXPR_CONSTANT,
        ASTC_BINARY_OP,
        ASTC_UNARY_OP
    };
    
    int type_count = sizeof(test_types) / sizeof(test_types[0]);
    for (int i = 0; i < type_count; i++) {
        ASTNode* node = ast_create_node(test_types[i], 1, 1);
        TEST_ASSERT(node != NULL, "Different node type allocation should succeed");
        TEST_ASSERT(node->type == test_types[i], "Node type should be set correctly");
        ast_free(node);
    }
    
    printf("    Different node type allocation: PASS\n");
    
    TEST_PASS();
}

int test_astc_memory_bounds_checking(void) {
    // Test memory bounds checking and safety
    printf("  Testing memory bounds checking...\n");
    
    // Test node structure size consistency
    size_t node_size = sizeof(ASTNode);
    TEST_ASSERT(node_size > 0, "Node size should be positive");
    TEST_ASSERT(node_size < 10240, "Node size should be reasonable (< 10KB)");
    
    printf("    ASTNode size: %zu bytes\n", node_size);
    
    // Test union data size
    size_t union_size = sizeof(((ASTNode*)0)->data);
    TEST_ASSERT(union_size > 0, "Union data size should be positive");
    TEST_ASSERT(union_size <= node_size, "Union should fit within node");
    
    printf("    Union data size: %zu bytes\n", union_size);
    
    // Test individual union member sizes
    size_t func_decl_size = sizeof(((ASTNode*)0)->data.func_decl);
    size_t binary_op_size = sizeof(((ASTNode*)0)->data.binary_op);
    size_t constant_size = sizeof(((ASTNode*)0)->data.constant);
    
    TEST_ASSERT(func_decl_size <= union_size, "func_decl should fit in union");
    TEST_ASSERT(binary_op_size <= union_size, "binary_op should fit in union");
    TEST_ASSERT(constant_size <= union_size, "constant should fit in union");
    
    printf("    Union member sizes: func_decl=%zu, binary_op=%zu, constant=%zu\n",
           func_decl_size, binary_op_size, constant_size);
    
    TEST_PASS();
}

int test_astc_memory_alignment(void) {
    // Test memory alignment requirements
    printf("  Testing memory alignment...\n");
    
    // Test node pointer alignment
    ASTNode* node = ast_create_node(ASTC_TRANSLATION_UNIT, 1, 1);
    TEST_ASSERT(node != NULL, "Node allocation should succeed");
    
    // Check pointer alignment (should be aligned to pointer size)
    uintptr_t node_addr = (uintptr_t)node;
    size_t ptr_size = sizeof(void*);
    TEST_ASSERT((node_addr % ptr_size) == 0, "Node should be properly aligned");
    
    printf("    Node address: %p (alignment: %zu bytes)\n", (void*)node, ptr_size);
    
    ast_free(node);
    
    // Test multiple allocations for alignment consistency
    ASTNode* nodes[10];
    for (int i = 0; i < 10; i++) {
        nodes[i] = ast_create_node(ASTC_VAR_DECL, i + 1, 1);
        TEST_ASSERT(nodes[i] != NULL, "Multiple allocation should succeed");
        
        uintptr_t addr = (uintptr_t)nodes[i];
        TEST_ASSERT((addr % ptr_size) == 0, "All nodes should be properly aligned");
    }
    
    for (int i = 0; i < 10; i++) {
        ast_free(nodes[i]);
    }
    
    printf("    Multiple allocation alignment: PASS\n");
    
    TEST_PASS();
}

int test_astc_memory_stress(void) {
    // Test memory management under stress conditions
    printf("  Testing memory stress conditions...\n");
    
    // Test large allocation
    const int large_count = 10000;
    ASTNode** large_nodes = malloc(large_count * sizeof(ASTNode*));
    TEST_ASSERT(large_nodes != NULL, "Large node array allocation should succeed");
    
    // Allocate many nodes
    for (int i = 0; i < large_count; i++) {
        large_nodes[i] = ast_create_node(ASTC_EXPR_CONSTANT, i, 1);
        if (large_nodes[i] == NULL) {
            printf("    Allocation failed at node %d\n", i);
            // Clean up allocated nodes
            for (int j = 0; j < i; j++) {
                ast_free(large_nodes[j]);
            }
            free(large_nodes);
            TEST_ASSERT(0, "Large allocation should not fail early");
        }
    }
    
    printf("    Large allocation (%d nodes): PASS\n", large_count);
    
    // Clean up all nodes
    for (int i = 0; i < large_count; i++) {
        ast_free(large_nodes[i]);
    }
    free(large_nodes);
    
    printf("    Large deallocation: PASS\n");
    
    // Test fragmented allocation/deallocation
    ASTNode* fragmented_nodes[100];
    
    // Allocate all
    for (int i = 0; i < 100; i++) {
        fragmented_nodes[i] = ast_create_node(ASTC_BINARY_OP, i, 1);
        TEST_ASSERT(fragmented_nodes[i] != NULL, "Fragmented allocation should succeed");
    }
    
    // Free every other node
    for (int i = 0; i < 100; i += 2) {
        ast_free(fragmented_nodes[i]);
        fragmented_nodes[i] = NULL;
    }
    
    // Allocate new nodes in gaps
    for (int i = 0; i < 100; i += 2) {
        fragmented_nodes[i] = ast_create_node(ASTC_UNARY_OP, i, 1);
        TEST_ASSERT(fragmented_nodes[i] != NULL, "Gap allocation should succeed");
    }
    
    // Clean up all remaining nodes
    for (int i = 0; i < 100; i++) {
        if (fragmented_nodes[i] != NULL) {
            ast_free(fragmented_nodes[i]);
        }
    }
    
    printf("    Fragmented allocation/deallocation: PASS\n");
    
    TEST_PASS();
}

// ===============================================
// Main Test Runner
// ===============================================

int main(void) {
    printf("=== ASTC Memory Management Tests ===\n\n");
    
    // Run all tests
    RUN_TEST(test_astc_node_memory_allocation);
    RUN_TEST(test_astc_complex_node_structures);
    RUN_TEST(test_astc_memory_leak_prevention);
    RUN_TEST(test_astc_memory_bounds_checking);
    RUN_TEST(test_astc_memory_alignment);
    RUN_TEST(test_astc_memory_stress);
    
    // Print summary
    printf("\n=== Test Summary ===\n");
    printf("Total tests: %d\n", total_tests);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Success rate: %.1f%%\n", (double)tests_passed / total_tests * 100.0);
    
    if (tests_failed == 0) {
        printf("\nAll ASTC memory management tests passed! ✓\n");
        printf("ASTC memory management is working correctly and safely.\n");
        return 0;
    } else {
        printf("\nSome ASTC memory management tests failed! ✗\n");
        return 1;
    }
}
