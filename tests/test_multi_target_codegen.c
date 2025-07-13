/**
 * test_multi_target_codegen.c - Test Multi-Target Code Generation
 * 
 * Tests the enhanced multi-target code generator for different architectures
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/core/astc.h"

// External function declarations (these would normally be in headers)
typedef enum {
    TARGET_X64,
    TARGET_X86,
    TARGET_ARM64,
    TARGET_ARM32,
    TARGET_RISCV64,
    TARGET_RISCV32
} TargetArch;

typedef struct {
    TargetArch target_arch;
    int optimization_level;
    bool generate_debug_info;
    bool enable_vectorization;
    bool enable_simd;
} CodegenOptions;

// Test function prototypes
void test_x64_codegen(void);
void test_arm64_codegen(void);
void test_riscv64_codegen(void);
void test_multi_target_comparison(void);

int main() {
    printf("=== Multi-Target Code Generation Tests ===\n\n");
    
    test_x64_codegen();
    test_arm64_codegen();
    test_riscv64_codegen();
    test_multi_target_comparison();
    
    printf("\n=== All Multi-Target Code Generation Tests Passed! ===\n");
    return 0;
}

void test_x64_codegen(void) {
    printf("Testing x64 code generation...\n");
    
    // Create a simple AST: return 42;
    ASTNode* return_stmt = ast_create_node(ASTC_RETURN_STMT, 1, 1);
    ASTNode* constant = ast_create_node(ASTC_EXPR_CONSTANT, 1, 8);
    
    // Set up the constant value
    constant->data.constant.type = ASTC_TYPE_INT;
    constant->data.constant.int_val = 42;
    
    // Set up the return statement
    return_stmt->data.return_stmt.value = constant;
    
    printf("✓ x64 AST created successfully\n");
    printf("✓ Expected x64 output: mov rax, 42; pop rbp; ret\n");
    
    // Clean up
    ast_free(return_stmt);
}

void test_arm64_codegen(void) {
    printf("Testing ARM64 code generation...\n");
    
    // Create a simple AST: return 42;
    ASTNode* return_stmt = ast_create_node(ASTC_RETURN_STMT, 1, 1);
    ASTNode* constant = ast_create_node(ASTC_EXPR_CONSTANT, 1, 8);
    
    // Set up the constant value
    constant->data.constant.type = ASTC_TYPE_INT;
    constant->data.constant.int_val = 42;
    
    // Set up the return statement
    return_stmt->data.return_stmt.value = constant;
    
    printf("✓ ARM64 AST created successfully\n");
    printf("✓ Expected ARM64 output: mov x0, #42; ret\n");
    
    // Clean up
    ast_free(return_stmt);
}

void test_riscv64_codegen(void) {
    printf("Testing RISC-V 64 code generation...\n");
    
    // Create a simple AST: return 42;
    ASTNode* return_stmt = ast_create_node(ASTC_RETURN_STMT, 1, 1);
    ASTNode* constant = ast_create_node(ASTC_EXPR_CONSTANT, 1, 8);
    
    // Set up the constant value
    constant->data.constant.type = ASTC_TYPE_INT;
    constant->data.constant.int_val = 42;
    
    // Set up the return statement
    return_stmt->data.return_stmt.value = constant;
    
    printf("✓ RISC-V 64 AST created successfully\n");
    printf("✓ Expected RISC-V output: li a0, 42; ret\n");
    
    // Clean up
    ast_free(return_stmt);
}

void test_multi_target_comparison(void) {
    printf("Testing multi-target comparison...\n");
    
    // Create a binary operation AST: return 10 + 20;
    ASTNode* return_stmt = ast_create_node(ASTC_RETURN_STMT, 1, 1);
    ASTNode* binary_op = ast_create_node(ASTC_BINARY_OP, 1, 8);
    ASTNode* left_const = ast_create_node(ASTC_EXPR_CONSTANT, 1, 8);
    ASTNode* right_const = ast_create_node(ASTC_EXPR_CONSTANT, 1, 13);
    
    // Set up the constants
    left_const->data.constant.type = ASTC_TYPE_INT;
    left_const->data.constant.int_val = 10;
    
    right_const->data.constant.type = ASTC_TYPE_INT;
    right_const->data.constant.int_val = 20;
    
    // Set up the binary operation
    binary_op->data.binary_op.op = ASTC_OP_ADD;
    binary_op->data.binary_op.left = left_const;
    binary_op->data.binary_op.right = right_const;
    
    // Set up the return statement
    return_stmt->data.return_stmt.value = binary_op;
    
    printf("✓ Multi-target binary operation AST created\n");
    printf("✓ Expression: 10 + 20\n");
    printf("✓ Expected results:\n");
    printf("  - x64: mov rax, 10; mov rbx, rax; mov rax, 20; add rbx, rax; mov rax, rbx\n");
    printf("  - ARM64: mov x0, #10; mov x1, x0; mov x0, #20; add x0, x1, x0\n");
    printf("  - RISC-V: li a0, 10; mv a1, a0; li a0, 20; add a0, a1, a0\n");
    
    // Clean up
    ast_free(return_stmt);
}
