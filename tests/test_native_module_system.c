/**
 * test_native_module_system.c - Tests for Native Module Calling System
 * 
 * Comprehensive test suite for the new native module calling system including:
 * - Module loading and unloading
 * - Function execution
 * - Argument passing and result handling
 * - Error handling
 * - Value type conversions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Include the utils header
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
// Native Module System Tests
// ===============================================

int test_native_module_system_init_cleanup(void) {
    // Test initialization
    int result = native_module_system_init();
    TEST_ASSERT(result == 0, "Native module system initialization should succeed");
    
    // Test double initialization (should be safe)
    result = native_module_system_init();
    TEST_ASSERT(result == 0, "Double initialization should be safe");
    
    // Test module count
    int count = native_module_get_count();
    TEST_ASSERT(count == 0, "Initial module count should be 0");
    
    // Test cleanup
    native_module_system_cleanup();
    
    // Test cleanup when not initialized (should be safe)
    native_module_system_cleanup();
    
    TEST_PASS();
}

int test_native_value_creation(void) {
    // Test int32 value
    NativeValue val_i32 = native_value_int32(42);
    TEST_ASSERT(val_i32.type == NATIVE_TYPE_INT32, "Int32 value should have correct type");
    TEST_ASSERT(val_i32.value.i32 == 42, "Int32 value should have correct value");
    TEST_ASSERT(val_i32.size == sizeof(int32_t), "Int32 value should have correct size");
    
    // Test int64 value
    NativeValue val_i64 = native_value_int64(123456789LL);
    TEST_ASSERT(val_i64.type == NATIVE_TYPE_INT64, "Int64 value should have correct type");
    TEST_ASSERT(val_i64.value.i64 == 123456789LL, "Int64 value should have correct value");
    
    // Test float value
    NativeValue val_f32 = native_value_float(3.14f);
    TEST_ASSERT(val_f32.type == NATIVE_TYPE_FLOAT, "Float value should have correct type");
    TEST_ASSERT(val_f32.value.f32 > 3.13f && val_f32.value.f32 < 3.15f, "Float value should be approximately correct");
    
    // Test double value
    NativeValue val_f64 = native_value_double(2.718281828);
    TEST_ASSERT(val_f64.type == NATIVE_TYPE_DOUBLE, "Double value should have correct type");
    TEST_ASSERT(val_f64.value.f64 > 2.71 && val_f64.value.f64 < 2.72, "Double value should be approximately correct");
    
    // Test string value
    NativeValue val_str = native_value_string("Hello, World!");
    TEST_ASSERT(val_str.type == NATIVE_TYPE_STRING, "String value should have correct type");
    TEST_ASSERT(strcmp(val_str.value.string, "Hello, World!") == 0, "String value should have correct content");
    TEST_ASSERT(val_str.size == 14, "String value should have correct size");
    
    // Cleanup string
    free(val_str.value.string);
    
    // Test pointer value
    int test_data = 999;
    NativeValue val_ptr = native_value_pointer(&test_data, sizeof(int));
    TEST_ASSERT(val_ptr.type == NATIVE_TYPE_POINTER, "Pointer value should have correct type");
    TEST_ASSERT(val_ptr.value.pointer == &test_data, "Pointer value should have correct address");
    TEST_ASSERT(val_ptr.size == sizeof(int), "Pointer value should have correct size");
    
    // Test bool value
    NativeValue val_bool = native_value_bool(1);
    TEST_ASSERT(val_bool.type == NATIVE_TYPE_BOOL, "Bool value should have correct type");
    TEST_ASSERT(val_bool.value.boolean == 1, "Bool value should have correct value");
    
    TEST_PASS();
}

int test_native_value_conversion(void) {
    // Test int32 conversions
    NativeValue val = native_value_int32(42);
    TEST_ASSERT(native_value_as_int32(&val) == 42, "Int32 to int32 conversion");
    TEST_ASSERT(native_value_as_int64(&val) == 42LL, "Int32 to int64 conversion");
    TEST_ASSERT(native_value_as_float(&val) == 42.0f, "Int32 to float conversion");
    TEST_ASSERT(native_value_as_double(&val) == 42.0, "Int32 to double conversion");
    TEST_ASSERT(native_value_as_bool(&val) == 1, "Int32 to bool conversion (non-zero)");
    
    // Test zero value
    val = native_value_int32(0);
    TEST_ASSERT(native_value_as_bool(&val) == 0, "Int32 to bool conversion (zero)");
    
    // Test float conversions
    val = native_value_float(3.14f);
    TEST_ASSERT(native_value_as_int32(&val) == 3, "Float to int32 conversion");
    TEST_ASSERT(native_value_as_bool(&val) == 1, "Float to bool conversion (non-zero)");
    
    // Test string conversions
    val = native_value_string("test");
    TEST_ASSERT(strcmp(native_value_as_string(&val), "test") == 0, "String to string conversion");
    TEST_ASSERT(native_value_as_bool(&val) == 1, "String to bool conversion (non-empty)");
    free(val.value.string);
    
    // Test NULL string
    val = native_value_string(NULL);
    TEST_ASSERT(native_value_as_string(&val) == NULL, "NULL string conversion");
    TEST_ASSERT(native_value_as_bool(&val) == 0, "NULL string to bool conversion");
    
    TEST_PASS();
}

int test_module_open_nonexistent(void) {
    native_module_system_init();
    
    // Try to open a non-existent module
    NativeModuleHandle* handle = module_open_native("nonexistent_module.native", NULL, MODULE_FLAG_NONE);
    TEST_ASSERT(handle == NULL, "Opening non-existent module should fail");
    
    native_module_system_cleanup();
    
    TEST_PASS();
}

int test_module_error_handling(void) {
    // Test NULL handle error handling
    int result = module_unload_native(NULL);
    TEST_ASSERT(result == -1, "Unloading NULL handle should fail");
    
    result = native_exec_native(NULL, "test", NULL, 0, NULL);
    TEST_ASSERT(result == -1, "Executing on NULL handle should fail");
    
    result = module_get_function_info(NULL, "test", NULL);
    TEST_ASSERT(result == -1, "Getting function info from NULL handle should fail");
    
    int count = module_list_functions(NULL, NULL, 0);
    TEST_ASSERT(count == -1, "Listing functions from NULL handle should fail");
    
    const char* error = module_get_last_error(NULL);
    TEST_ASSERT(error != NULL, "Getting error from NULL handle should return error message");
    
    TEST_PASS();
}

int test_module_system_info(void) {
    native_module_system_init();
    
    // Test initial state
    int count = native_module_get_count();
    TEST_ASSERT(count == 0, "Initial module count should be 0");
    
    // Test print info (should not crash)
    native_module_print_info();
    
    native_module_system_cleanup();
    
    TEST_PASS();
}

int test_value_edge_cases(void) {
    // Test NULL value conversions
    TEST_ASSERT(native_value_as_int32(NULL) == 0, "NULL value to int32 should return 0");
    TEST_ASSERT(native_value_as_int64(NULL) == 0, "NULL value to int64 should return 0");
    TEST_ASSERT(native_value_as_float(NULL) == 0.0f, "NULL value to float should return 0.0");
    TEST_ASSERT(native_value_as_double(NULL) == 0.0, "NULL value to double should return 0.0");
    TEST_ASSERT(native_value_as_bool(NULL) == 0, "NULL value to bool should return 0");
    TEST_ASSERT(native_value_as_string(NULL) == NULL, "NULL value to string should return NULL");
    TEST_ASSERT(native_value_as_pointer(NULL) == NULL, "NULL value to pointer should return NULL");
    
    // Test empty string
    NativeValue empty_str = native_value_string("");
    TEST_ASSERT(native_value_as_bool(&empty_str) == 0, "Empty string should convert to false");
    free(empty_str.value.string);
    
    TEST_PASS();
}

// ===============================================
// Main Test Runner
// ===============================================

int main(void) {
    printf("=== Native Module Calling System Tests ===\n\n");
    
    // Run all tests
    RUN_TEST(test_native_module_system_init_cleanup);
    RUN_TEST(test_native_value_creation);
    RUN_TEST(test_native_value_conversion);
    RUN_TEST(test_module_open_nonexistent);
    RUN_TEST(test_module_error_handling);
    RUN_TEST(test_module_system_info);
    RUN_TEST(test_value_edge_cases);
    
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
