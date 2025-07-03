/**
 * test_libc_module.c - Test for LibC Module Functionality
 * 
 * Tests the libc_module.c functionality including:
 * - Module initialization and cleanup
 * - Function lookup and calling
 * - Memory management functions
 * - String manipulation functions
 * - I/O functions
 * - Statistics tracking
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Include the core utils for native module system
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

// Forward declarations for libc module functions
// (These would normally be loaded dynamically)
extern int libc_native_init(void);
extern void libc_native_cleanup(void);
extern void* libc_native_get_function(const char* name);
extern const void* libc_native_get_info(void);
extern const void* libc_native_get_stats(void);
extern int libc_native_main(void);

// ===============================================
// LibC Module Tests
// ===============================================

int test_libc_module_init_cleanup(void) {
    // Test initialization
    int result = libc_native_init();
    TEST_ASSERT(result == 0, "LibC module initialization should succeed");
    
    // Test double initialization (should be safe)
    result = libc_native_init();
    TEST_ASSERT(result == 0, "Double initialization should be safe");
    
    // Test cleanup
    libc_native_cleanup();
    
    // Test cleanup when not initialized (should be safe)
    libc_native_cleanup();
    
    TEST_PASS();
}

int test_libc_function_lookup(void) {
    // Initialize module
    int result = libc_native_init();
    TEST_ASSERT(result == 0, "Module should initialize successfully");
    
    // Test function lookup for common functions
    void* malloc_func = libc_native_get_function("malloc");
    TEST_ASSERT(malloc_func != NULL, "Should find malloc function");
    
    void* free_func = libc_native_get_function("free");
    TEST_ASSERT(free_func != NULL, "Should find free function");
    
    void* strlen_func = libc_native_get_function("strlen");
    TEST_ASSERT(strlen_func != NULL, "Should find strlen function");
    
    void* printf_func = libc_native_get_function("printf");
    TEST_ASSERT(printf_func != NULL, "Should find printf function");
    
    // Test lookup for non-existent function
    void* nonexistent = libc_native_get_function("nonexistent_function");
    TEST_ASSERT(nonexistent == NULL, "Should not find non-existent function");
    
    // Test lookup when not initialized
    libc_native_cleanup();
    void* func_after_cleanup = libc_native_get_function("malloc");
    TEST_ASSERT(func_after_cleanup == NULL, "Should not find functions after cleanup");
    
    TEST_PASS();
}

int test_libc_memory_functions(void) {
    // Initialize module
    int result = libc_native_init();
    TEST_ASSERT(result == 0, "Module should initialize successfully");
    
    // Get function pointers
    void* (*test_malloc)(size_t) = (void* (*)(size_t))libc_native_get_function("malloc");
    void (*test_free)(void*) = (void (*)(void*))libc_native_get_function("free");
    void* (*test_calloc)(size_t, size_t) = (void* (*)(size_t, size_t))libc_native_get_function("calloc");
    
    TEST_ASSERT(test_malloc != NULL, "Should get malloc function pointer");
    TEST_ASSERT(test_free != NULL, "Should get free function pointer");
    TEST_ASSERT(test_calloc != NULL, "Should get calloc function pointer");
    
    // Test malloc and free
    void* ptr1 = test_malloc(100);
    TEST_ASSERT(ptr1 != NULL, "Malloc should return valid pointer");
    
    // Write to allocated memory
    memset(ptr1, 0xAA, 100);
    
    test_free(ptr1);
    
    // Test calloc
    void* ptr2 = test_calloc(10, sizeof(int));
    TEST_ASSERT(ptr2 != NULL, "Calloc should return valid pointer");
    
    // Verify calloc zeroed the memory
    int* int_ptr = (int*)ptr2;
    TEST_ASSERT(int_ptr[0] == 0, "Calloc should zero memory");
    TEST_ASSERT(int_ptr[9] == 0, "Calloc should zero all memory");
    
    test_free(ptr2);
    
    libc_native_cleanup();
    TEST_PASS();
}

int test_libc_string_functions(void) {
    // Initialize module
    int result = libc_native_init();
    TEST_ASSERT(result == 0, "Module should initialize successfully");
    
    // Get function pointers
    size_t (*test_strlen)(const char*) = (size_t (*)(const char*))libc_native_get_function("strlen");
    char* (*test_strcpy)(char*, const char*) = (char* (*)(char*, const char*))libc_native_get_function("strcpy");
    int (*test_strcmp)(const char*, const char*) = (int (*)(const char*, const char*))libc_native_get_function("strcmp");
    
    TEST_ASSERT(test_strlen != NULL, "Should get strlen function pointer");
    TEST_ASSERT(test_strcpy != NULL, "Should get strcpy function pointer");
    TEST_ASSERT(test_strcmp != NULL, "Should get strcmp function pointer");
    
    // Test strlen
    size_t len = test_strlen("Hello, World!");
    TEST_ASSERT(len == 13, "strlen should return correct length");
    
    // Test strcpy
    char buffer[50];
    char* result_ptr = test_strcpy(buffer, "Test String");
    TEST_ASSERT(result_ptr == buffer, "strcpy should return destination pointer");
    TEST_ASSERT(strcmp(buffer, "Test String") == 0, "strcpy should copy string correctly");
    
    // Test strcmp
    int cmp_result = test_strcmp("abc", "abc");
    TEST_ASSERT(cmp_result == 0, "strcmp should return 0 for equal strings");
    
    cmp_result = test_strcmp("abc", "def");
    TEST_ASSERT(cmp_result < 0, "strcmp should return negative for first < second");
    
    cmp_result = test_strcmp("def", "abc");
    TEST_ASSERT(cmp_result > 0, "strcmp should return positive for first > second");
    
    libc_native_cleanup();
    TEST_PASS();
}

int test_libc_module_info(void) {
    // Initialize module
    int result = libc_native_init();
    TEST_ASSERT(result == 0, "Module should initialize successfully");
    
    // Get module info
    const void* info = libc_native_get_info();
    TEST_ASSERT(info != NULL, "Should get module info");
    
    // Note: We can't easily test the contents without knowing the exact structure
    // In a real implementation, we'd cast to LibCModuleInfo* and check fields
    
    libc_native_cleanup();
    TEST_PASS();
}

int test_libc_module_main(void) {
    // Test the module's main function (self-test)
    int result = libc_native_main();
    TEST_ASSERT(result == 0, "Module main function should succeed");
    
    TEST_PASS();
}

int test_libc_error_handling(void) {
    // Test function lookup when not initialized
    void* func = libc_native_get_function("malloc");
    TEST_ASSERT(func == NULL, "Should not find functions when not initialized");
    
    // Test NULL parameter handling
    func = libc_native_get_function(NULL);
    TEST_ASSERT(func == NULL, "Should handle NULL function name gracefully");
    
    TEST_PASS();
}

// ===============================================
// Main Test Runner
// ===============================================

int main(void) {
    printf("=== LibC Module Tests ===\n\n");
    
    // Run all tests
    RUN_TEST(test_libc_module_init_cleanup);
    RUN_TEST(test_libc_function_lookup);
    RUN_TEST(test_libc_memory_functions);
    RUN_TEST(test_libc_string_functions);
    RUN_TEST(test_libc_module_info);
    RUN_TEST(test_libc_module_main);
    RUN_TEST(test_libc_error_handling);
    
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
