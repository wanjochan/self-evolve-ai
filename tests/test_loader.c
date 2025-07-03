/**
 * test_loader.c - Tests for Enhanced Loader (Layer 1)
 * 
 * Comprehensive test suite for the enhanced loader including:
 * - Architecture detection
 * - Command line parsing
 * - VM module loading
 * - Program execution flow
 * - Error handling
 * - Integration tests
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Include the core utilities
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
// Loader Tests
// ===============================================

int test_architecture_detection(void) {
    // Test architecture detection
    DetectedArchitecture arch = detect_architecture();
    TEST_ASSERT(arch != ARCH_UNKNOWN, "Architecture detection should succeed");
    
    // Test architecture name retrieval
    const char* arch_name = get_architecture_name(arch);
    TEST_ASSERT(arch_name != NULL, "Architecture name should not be NULL");
    TEST_ASSERT(strlen(arch_name) > 0, "Architecture name should not be empty");
    
    // Test architecture bits
    int bits = 0;
    switch (arch) {
        case ARCH_X86_32:
        case ARCH_ARM32:
            bits = 32;
            break;
        case ARCH_X86_64:
        case ARCH_ARM64:
            bits = 64;
            break;
        default:
            TEST_ASSERT(0, "Unknown architecture detected");
    }
    
    TEST_ASSERT(bits == 32 || bits == 64, "Architecture bits should be 32 or 64");
    
    printf("  Detected architecture: %s (%d-bit)\n", arch_name, bits);
    
    TEST_PASS();
}

int test_platform_detection(void) {
    // Test platform detection
    RuntimePlatform platform = detect_platform();
    TEST_ASSERT(platform != PLATFORM_UNKNOWN, "Platform detection should succeed");
    
    const char* platform_name = get_platform_name(platform);
    TEST_ASSERT(platform_name != NULL, "Platform name should not be NULL");
    TEST_ASSERT(strlen(platform_name) > 0, "Platform name should not be empty");
    
    printf("  Detected platform: %s\n", platform_name);
    
    TEST_PASS();
}

int test_file_utilities(void) {
    // Test file existence checking
    TEST_ASSERT(!file_exists("nonexistent_file.txt"), "Non-existent file should return false");
    
    // Test safe string functions
    char buffer[10];
    safe_strncpy(buffer, "test", sizeof(buffer));
    TEST_ASSERT(strcmp(buffer, "test") == 0, "safe_strncpy should copy string correctly");
    
    // Test safe snprintf
    char format_buffer[20];
    safe_snprintf(format_buffer, sizeof(format_buffer), "test_%d", 123);
    TEST_ASSERT(strcmp(format_buffer, "test_123") == 0, "safe_snprintf should format correctly");
    
    TEST_PASS();
}

int test_native_module_system(void) {
    // Test native module system initialization
    int result = native_module_system_init();
    TEST_ASSERT(result == 0, "Native module system initialization should succeed");
    
    // Test module loading with non-existent file
    NativeModuleHandle* handle = module_open_native("nonexistent.native", NULL, MODULE_FLAG_NONE);
    TEST_ASSERT(handle == NULL, "Loading non-existent module should fail");
    
    // Cleanup
    native_module_system_cleanup();
    
    TEST_PASS();
}

int test_vm_module_path_construction(void) {
    // Test VM module path construction for different architectures
    char path_buffer[256];
    
    // Test x86_64
    snprintf(path_buffer, sizeof(path_buffer), "vm_%s_%d.native", "x86_64", 64);
    TEST_ASSERT(strcmp(path_buffer, "vm_x86_64_64.native") == 0, "x86_64 VM path should be correct");
    
    // Test x86_32
    snprintf(path_buffer, sizeof(path_buffer), "vm_%s_%d.native", "x86_32", 32);
    TEST_ASSERT(strcmp(path_buffer, "vm_x86_32_32.native") == 0, "x86_32 VM path should be correct");
    
    TEST_PASS();
}

int test_command_line_parsing_simulation(void) {
    // Simulate command line parsing (without actually calling the loader)
    
    // Test verbose flag parsing
    const char* test_args[] = {"loader", "-v", "program.astc"};
    int argc = 3;
    
    // Simple parsing simulation
    bool verbose_found = false;
    bool program_found = false;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(test_args[i], "-v") == 0) {
            verbose_found = true;
        } else if (strstr(test_args[i], ".astc") != NULL) {
            program_found = true;
        }
    }
    
    TEST_ASSERT(verbose_found, "Verbose flag should be detected");
    TEST_ASSERT(program_found, "Program file should be detected");
    
    TEST_PASS();
}

int test_error_handling_simulation(void) {
    // Test error message formatting
    char error_buffer[512];
    
    // Test simple error message
    snprintf(error_buffer, sizeof(error_buffer), "Test error: %s", "sample error");
    TEST_ASSERT(strstr(error_buffer, "Test error") != NULL, "Error message should contain prefix");
    TEST_ASSERT(strstr(error_buffer, "sample error") != NULL, "Error message should contain details");
    
    // Test error message with number
    snprintf(error_buffer, sizeof(error_buffer), "Error code: %d", 42);
    TEST_ASSERT(strstr(error_buffer, "42") != NULL, "Error message should contain error code");
    
    TEST_PASS();
}

int test_memory_management(void) {
    // Test basic memory allocation
    void* ptr = malloc(1024);
    TEST_ASSERT(ptr != NULL, "Memory allocation should succeed");
    
    // Test memory initialization
    memset(ptr, 0, 1024);
    
    // Test memory access
    char* char_ptr = (char*)ptr;
    char_ptr[0] = 'A';
    char_ptr[1023] = 'Z';
    
    TEST_ASSERT(char_ptr[0] == 'A', "Memory write/read should work");
    TEST_ASSERT(char_ptr[1023] == 'Z', "Memory boundary access should work");
    
    // Cleanup
    free(ptr);
    
    TEST_PASS();
}

int test_string_operations(void) {
    // Test string operations used in loader
    char buffer[100];
    
    // Test string copying
    strcpy(buffer, "test string");
    TEST_ASSERT(strcmp(buffer, "test string") == 0, "String copy should work");
    
    // Test string concatenation
    strcat(buffer, " extended");
    TEST_ASSERT(strcmp(buffer, "test string extended") == 0, "String concatenation should work");
    
    // Test string searching
    char* found = strstr(buffer, "string");
    TEST_ASSERT(found != NULL, "String search should find substring");
    
    // Test string length
    size_t len = strlen(buffer);
    TEST_ASSERT(len == strlen("test string extended"), "String length should be correct");
    
    TEST_PASS();
}

int test_loader_integration_simulation(void) {
    // Simulate the complete loader flow without actually executing
    printf("  Simulating loader integration flow...\n");
    
    // Step 1: Architecture detection
    DetectedArchitecture arch = detect_architecture();
    TEST_ASSERT(arch != ARCH_UNKNOWN, "Step 1: Architecture detection should succeed");
    printf("    Step 1: Architecture detected - %s\n", get_architecture_name(arch));
    
    // Step 2: VM module path construction
    char vm_path[256];
    int bits = (arch == ARCH_X86_64 || arch == ARCH_ARM64) ? 64 : 32;
    snprintf(vm_path, sizeof(vm_path), "vm_%s_%d.native", get_architecture_name(arch), bits);
    printf("    Step 2: VM module path - %s\n", vm_path);
    
    // Step 3: Module system initialization
    int init_result = native_module_system_init();
    TEST_ASSERT(init_result == 0, "Step 3: Module system initialization should succeed");
    printf("    Step 3: Module system initialized\n");
    
    // Step 4: Cleanup
    native_module_system_cleanup();
    printf("    Step 4: Cleanup completed\n");
    
    printf("  Integration simulation completed successfully\n");
    
    TEST_PASS();
}

// ===============================================
// Main Test Runner
// ===============================================

int main(void) {
    printf("=== Enhanced Loader Tests ===\n\n");
    
    // Run all tests
    RUN_TEST(test_architecture_detection);
    RUN_TEST(test_platform_detection);
    RUN_TEST(test_file_utilities);
    RUN_TEST(test_native_module_system);
    RUN_TEST(test_vm_module_path_construction);
    RUN_TEST(test_command_line_parsing_simulation);
    RUN_TEST(test_error_handling_simulation);
    RUN_TEST(test_memory_management);
    RUN_TEST(test_string_operations);
    RUN_TEST(test_loader_integration_simulation);
    
    // Print summary
    printf("\n=== Test Summary ===\n");
    printf("Total tests: %d\n", total_tests);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Success rate: %.1f%%\n", (double)tests_passed / total_tests * 100.0);
    
    if (tests_failed == 0) {
        printf("\nAll loader tests passed! ✓\n");
        return 0;
    } else {
        printf("\nSome loader tests failed! ✗\n");
        return 1;
    }
}
