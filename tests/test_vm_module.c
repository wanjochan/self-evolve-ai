/**
 * test_vm_module.c - Tests for VM Module Functionality
 * 
 * Comprehensive test suite for the VM module including:
 * - ASTC program loading and validation
 * - VM context creation and management
 * - Memory management
 * - JIT compilation
 * - Module system integration
 * - Bytecode interpretation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Include the VM module
#include "../src/core/utils.h"

// Forward declarations for VM module functions
extern ASTCProgram* vm_load_astc_program(const char* astc_file);
extern int vm_unload_astc_program(ASTCProgram* program);
extern int vm_validate_astc_program(ASTCProgram* program);
extern VMContext* vm_create_context(ASTCProgram* program);
extern void vm_destroy_context(VMContext* context);
extern int vm_execute_program(VMContext* context, int argc, char* argv[]);
extern VMMemoryManager* vm_create_memory_manager(size_t heap_size, size_t stack_size);
extern void vm_destroy_memory_manager(VMMemoryManager* memory);
extern void* vm_malloc(VMContext* context, size_t size);
extern void vm_free(VMContext* context, void* ptr);
extern int vm_gc_collect(VMContext* context);
extern int vm_jit_compile_program(ASTCProgram* program);

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
// VM Module Tests
// ===============================================

int test_vm_load_nonexistent_program(void) {
    // Test loading non-existent ASTC program
    ASTCProgram* program = vm_load_astc_program("nonexistent.astc");
    TEST_ASSERT(program == NULL, "Loading non-existent program should fail");
    
    TEST_PASS();
}

int test_vm_memory_manager(void) {
    // Test memory manager creation
    VMMemoryManager* memory = vm_create_memory_manager(1024, 512);
    TEST_ASSERT(memory != NULL, "Memory manager creation should succeed");
    
    // Test memory manager destruction
    vm_destroy_memory_manager(memory);
    
    // Test with zero sizes
    memory = vm_create_memory_manager(0, 0);
    TEST_ASSERT(memory == NULL, "Memory manager with zero size should fail");
    
    TEST_PASS();
}

int test_vm_context_creation(void) {
    // Create a mock ASTC program
    ASTCProgram mock_program = {0};
    mock_program.program_name = "test_program";
    mock_program.entry_point = 0;
    mock_program.bytecode_size = 100;
    mock_program.bytecode = malloc(100);
    memset(mock_program.bytecode, 0, 100);
    
    // Test context creation
    VMContext* context = vm_create_context(&mock_program);
    TEST_ASSERT(context != NULL, "VM context creation should succeed");
    TEST_ASSERT(context->program == &mock_program, "Context should reference the program");
    TEST_ASSERT(context->memory != NULL, "Context should have memory manager");
    
    // Test context destruction
    vm_destroy_context(context);
    
    // Test with NULL program
    context = vm_create_context(NULL);
    TEST_ASSERT(context == NULL, "Context creation with NULL program should fail");
    
    free(mock_program.bytecode);
    TEST_PASS();
}

int test_vm_memory_allocation(void) {
    // Create memory manager and context
    ASTCProgram mock_program = {0};
    mock_program.program_name = "test_program";
    mock_program.entry_point = 0;
    mock_program.bytecode_size = 100;
    mock_program.bytecode = malloc(100);
    
    VMContext* context = vm_create_context(&mock_program);
    TEST_ASSERT(context != NULL, "Context creation should succeed");
    
    // Test memory allocation
    void* ptr1 = vm_malloc(context, 64);
    TEST_ASSERT(ptr1 != NULL, "Memory allocation should succeed");
    
    void* ptr2 = vm_malloc(context, 128);
    TEST_ASSERT(ptr2 != NULL, "Second memory allocation should succeed");
    TEST_ASSERT(ptr1 != ptr2, "Allocated pointers should be different");
    
    // Test memory freeing
    vm_free(context, ptr1);
    vm_free(context, ptr2);
    
    // Test garbage collection
    int gc_result = vm_gc_collect(context);
    TEST_ASSERT(gc_result == 0, "Garbage collection should succeed");
    
    // Test allocation with zero size
    void* ptr_zero = vm_malloc(context, 0);
    TEST_ASSERT(ptr_zero == NULL, "Zero-size allocation should fail");
    
    vm_destroy_context(context);
    free(mock_program.bytecode);
    TEST_PASS();
}

int test_vm_program_validation(void) {
    // Test validation with NULL program
    int result = vm_validate_astc_program(NULL);
    TEST_ASSERT(result == -1, "Validation of NULL program should fail");
    
    // Create mock program with invalid data
    ASTCProgram invalid_program = {0};
    invalid_program.program_name = "invalid_program";
    invalid_program.bytecode = NULL;
    invalid_program.bytecode_size = 0;
    
    result = vm_validate_astc_program(&invalid_program);
    TEST_ASSERT(result == -1, "Validation of program without bytecode should fail");
    
    // Create mock program with valid data
    ASTCProgram valid_program = {0};
    valid_program.program_name = "valid_program";
    valid_program.bytecode_size = 100;
    valid_program.bytecode = malloc(100);
    valid_program.entry_point = 0;
    memset(valid_program.bytecode, 0, 100);
    
    result = vm_validate_astc_program(&valid_program);
    TEST_ASSERT(result == 0, "Validation of valid program should succeed");
    
    // Test with invalid entry point
    valid_program.entry_point = 200; // Beyond bytecode size
    result = vm_validate_astc_program(&valid_program);
    TEST_ASSERT(result == -1, "Validation with invalid entry point should fail");
    
    free(valid_program.bytecode);
    TEST_PASS();
}

int test_vm_jit_compilation(void) {
    // Create mock program
    ASTCProgram mock_program = {0};
    mock_program.program_name = "jit_test_program";
    mock_program.bytecode_size = 20;
    mock_program.bytecode = malloc(20);
    mock_program.entry_point = 0;
    
    // Create simple bytecode: LOAD_IMM32 r0, 42; HALT
    mock_program.bytecode[0] = 0x10; // LOAD_IMM32
    mock_program.bytecode[1] = 0x00; // reg 0
    *(uint32_t*)(mock_program.bytecode + 2) = 42; // immediate value
    mock_program.bytecode[6] = 0x01; // HALT
    
    // Test JIT compilation
    int result = vm_jit_compile_program(&mock_program);
    TEST_ASSERT(result == 0, "JIT compilation should succeed");
    
    // Test JIT compilation with NULL program
    result = vm_jit_compile_program(NULL);
    TEST_ASSERT(result == -1, "JIT compilation of NULL program should fail");
    
    free(mock_program.bytecode);
    TEST_PASS();
}

int test_vm_module_system(void) {
    // Create context
    ASTCProgram mock_program = {0};
    mock_program.program_name = "module_test";
    mock_program.bytecode_size = 10;
    mock_program.bytecode = malloc(10);
    
    VMContext* context = vm_create_context(&mock_program);
    TEST_ASSERT(context != NULL, "Context creation should succeed");
    
    // Test module system initialization
    int result = vm_module_system_init(context);
    TEST_ASSERT(result == 0, "Module system initialization should succeed");
    
    // Test module listing
    char module_names[10][64];
    int module_count = vm_list_loaded_modules(context, module_names, 10);
    TEST_ASSERT(module_count >= 0, "Module listing should succeed");
    
    // Test module cleanup
    vm_module_system_cleanup(context);
    
    vm_destroy_context(context);
    free(mock_program.bytecode);
    TEST_PASS();
}

int test_vm_error_handling(void) {
    // Test various error conditions
    
    // NULL context operations
    void* ptr = vm_malloc(NULL, 100);
    TEST_ASSERT(ptr == NULL, "malloc with NULL context should fail");
    
    vm_free(NULL, ptr);
    
    int result = vm_gc_collect(NULL);
    TEST_ASSERT(result == -1, "GC with NULL context should fail");
    
    result = vm_load_native_module(NULL, "test.native");
    TEST_ASSERT(result == -1, "Module loading with NULL context should fail");
    
    TEST_PASS();
}

// ===============================================
// Main Test Runner
// ===============================================

int main(void) {
    printf("=== VM Module Tests ===\n\n");
    
    // Run all tests
    RUN_TEST(test_vm_load_nonexistent_program);
    RUN_TEST(test_vm_memory_manager);
    RUN_TEST(test_vm_context_creation);
    RUN_TEST(test_vm_memory_allocation);
    RUN_TEST(test_vm_program_validation);
    RUN_TEST(test_vm_jit_compilation);
    RUN_TEST(test_vm_module_system);
    RUN_TEST(test_vm_error_handling);
    
    // Print summary
    printf("\n=== Test Summary ===\n");
    printf("Total tests: %d\n", total_tests);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Success rate: %.1f%%\n", (double)tests_passed / total_tests * 100.0);
    
    if (tests_failed == 0) {
        printf("\nAll tests passed! ✓\n");
        return 0;
    } else {
        printf("\nSome tests failed! ✗\n");
        return 1;
    }
}
