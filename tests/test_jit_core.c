/**
 * test_jit_core.c - Tests for Core JIT Compiler
 * 
 * Comprehensive test suite for the core JIT compiler including:
 * - JIT compiler initialization and cleanup
 * - ASTC bytecode compilation
 * - Architecture-specific code generation
 * - JIT cache functionality
 * - Cross-architecture compatibility
 * - Performance benchmarks
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

// Include the JIT core module
#include "../src/core/jit.h"
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
// JIT Core Tests
// ===============================================

int test_jit_initialization(void) {
    // Test JIT compiler initialization
    JITCompiler* jit = jit_init(ARCH_X86_64, JIT_OPT_BASIC, JIT_FLAG_NONE);
    TEST_ASSERT(jit != NULL, "JIT compiler initialization should succeed");
    
    // Test compiler properties
    TEST_ASSERT(jit->target_arch == ARCH_X86_64, "Target architecture should be set correctly");
    TEST_ASSERT(jit->opt_level == JIT_OPT_BASIC, "Optimization level should be set correctly");
    TEST_ASSERT(jit->flags == JIT_FLAG_NONE, "Flags should be set correctly");
    TEST_ASSERT(jit->code_buffer != NULL, "Code buffer should be allocated");
    TEST_ASSERT(jit->code_capacity > 0, "Code capacity should be positive");
    
    // Test cleanup
    jit_cleanup(jit);
    
    // Test initialization with auto-detection
    jit = jit_init(ARCH_UNKNOWN, JIT_OPT_NONE, JIT_FLAG_DEBUG_INFO);
    TEST_ASSERT(jit != NULL, "JIT compiler with auto-detection should succeed");
    TEST_ASSERT(jit->target_arch != ARCH_UNKNOWN, "Architecture should be auto-detected");
    
    jit_cleanup(jit);
    
    TEST_PASS();
}

int test_jit_architecture_support(void) {
    // Test architecture support checking
    TEST_ASSERT(jit_is_architecture_supported(ARCH_X86_64), "x86_64 should be supported");
    TEST_ASSERT(jit_is_architecture_supported(ARCH_X86_32), "x86_32 should be supported");
    TEST_ASSERT(!jit_is_architecture_supported(ARCH_ARM64), "ARM64 should not be supported yet");
    TEST_ASSERT(!jit_is_architecture_supported(ARCH_ARM32), "ARM32 should not be supported yet");
    TEST_ASSERT(!jit_is_architecture_supported(ARCH_UNKNOWN), "Unknown arch should not be supported");
    
    TEST_PASS();
}

int test_jit_bytecode_compilation(void) {
    JITCompiler* jit = jit_init(ARCH_X86_64, JIT_OPT_BASIC, JIT_FLAG_NONE);
    TEST_ASSERT(jit != NULL, "JIT compiler initialization should succeed");
    
    // Create simple ASTC bytecode: LOAD_IMM32 r0, 42; HALT
    uint8_t bytecode[] = {
        0x10, 0x00, 0x2A, 0x00, 0x00, 0x00,  // LOAD_IMM32 r0, 42
        0x01                                   // HALT
    };
    size_t bytecode_size = sizeof(bytecode);
    
    // Test compilation
    JITResult result = jit_compile_bytecode(jit, bytecode, bytecode_size, 0);
    TEST_ASSERT(result == JIT_SUCCESS, "Bytecode compilation should succeed");
    TEST_ASSERT(jit_get_code_size(jit) > 0, "Generated code size should be positive");
    
    void* entry_point = jit_get_entry_point(jit);
    TEST_ASSERT(entry_point != NULL, "Entry point should be available");
    
    // Test compilation with invalid input
    result = jit_compile_bytecode(jit, NULL, 0, 0);
    TEST_ASSERT(result == JIT_ERROR_INVALID_INPUT, "Compilation with NULL bytecode should fail");
    
    result = jit_compile_bytecode(NULL, bytecode, bytecode_size, 0);
    TEST_ASSERT(result == JIT_ERROR_INVALID_INPUT, "Compilation with NULL compiler should fail");
    
    jit_cleanup(jit);
    TEST_PASS();
}

int test_jit_cache_functionality(void) {
    // Initialize JIT cache
    int result = jit_cache_init(1024 * 1024); // 1MB cache
    TEST_ASSERT(result == 0, "JIT cache initialization should succeed");
    
    // Test cache lookup (should miss initially)
    void* entry_point;
    size_t code_size;
    bool found = jit_cache_lookup(0x12345678, &entry_point, &code_size);
    TEST_ASSERT(!found, "Cache lookup should miss for non-existent entry");
    
    // Test cache storage
    uint8_t dummy_code[] = {0x90, 0x90, 0x90}; // NOP instructions
    result = jit_cache_store(0x12345678, dummy_code, sizeof(dummy_code));
    TEST_ASSERT(result == 0, "Cache storage should succeed");
    
    // Test cache lookup (should hit now)
    found = jit_cache_lookup(0x12345678, &entry_point, &code_size);
    TEST_ASSERT(found, "Cache lookup should hit for stored entry");
    TEST_ASSERT(code_size == sizeof(dummy_code), "Cached code size should match");
    
    // Test cache clear
    jit_cache_clear();
    found = jit_cache_lookup(0x12345678, &entry_point, &code_size);
    TEST_ASSERT(!found, "Cache lookup should miss after clear");
    
    // Cleanup cache
    jit_cache_cleanup();
    
    TEST_PASS();
}

int test_jit_hash_function(void) {
    uint8_t data1[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t data2[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t data3[] = {0x04, 0x03, 0x02, 0x01};
    
    uint64_t hash1 = jit_hash_bytecode(data1, sizeof(data1));
    uint64_t hash2 = jit_hash_bytecode(data2, sizeof(data2));
    uint64_t hash3 = jit_hash_bytecode(data3, sizeof(data3));
    
    TEST_ASSERT(hash1 == hash2, "Identical data should produce identical hashes");
    TEST_ASSERT(hash1 != hash3, "Different data should produce different hashes");
    
    // Test with empty data
    uint64_t hash_empty = jit_hash_bytecode(NULL, 0);
    TEST_ASSERT(hash_empty != 0, "Empty data should still produce a hash");
    
    TEST_PASS();
}

int test_jit_statistics(void) {
    JITCompiler* jit = jit_init(ARCH_X86_64, JIT_OPT_BASIC, JIT_FLAG_NONE);
    TEST_ASSERT(jit != NULL, "JIT compiler initialization should succeed");
    
    // Get initial statistics
    JITStats stats;
    jit_get_stats(jit, &stats);
    size_t initial_compilations = stats.total_compilations;
    
    // Compile some bytecode
    uint8_t bytecode[] = {0x10, 0x00, 0x2A, 0x00, 0x00, 0x00, 0x01}; // LOAD_IMM32 + HALT
    jit_compile_bytecode(jit, bytecode, sizeof(bytecode), 0);
    
    // Check updated statistics
    jit_get_stats(jit, &stats);
    TEST_ASSERT(stats.total_compilations > initial_compilations, "Compilation count should increase");
    TEST_ASSERT(stats.total_code_size > 0, "Total code size should be positive");
    
    jit_cleanup(jit);
    TEST_PASS();
}

int test_jit_error_handling(void) {
    // Test various error conditions
    
    // NULL compiler operations
    JITResult result = jit_compile_bytecode(NULL, NULL, 0, 0);
    TEST_ASSERT(result == JIT_ERROR_INVALID_INPUT, "NULL compiler should return error");
    
    void* entry = jit_get_entry_point(NULL);
    TEST_ASSERT(entry == NULL, "NULL compiler should return NULL entry point");
    
    size_t size = jit_get_code_size(NULL);
    TEST_ASSERT(size == 0, "NULL compiler should return zero size");
    
    // Test unsupported architecture
    JITCompiler* jit = jit_init(ARCH_ARM64, JIT_OPT_BASIC, JIT_FLAG_NONE);
    TEST_ASSERT(jit == NULL, "Unsupported architecture should fail initialization");
    
    TEST_PASS();
}

int test_jit_performance_benchmark(void) {
    printf("  Running JIT performance benchmark...\n");
    
    JITCompiler* jit = jit_init(ARCH_X86_64, JIT_OPT_AGGRESSIVE, JIT_FLAG_CACHE_RESULT);
    TEST_ASSERT(jit != NULL, "JIT compiler initialization should succeed");
    
    // Create a more complex bytecode program
    uint8_t bytecode[] = {
        0x10, 0x00, 0x0A, 0x00, 0x00, 0x00,  // LOAD_IMM32 r0, 10
        0x10, 0x01, 0x14, 0x00, 0x00, 0x00,  // LOAD_IMM32 r1, 20
        0x20, 0x02, 0x00, 0x01,              // ADD r2, r0, r1
        0x10, 0x03, 0x05, 0x00, 0x00, 0x00,  // LOAD_IMM32 r3, 5
        0x20, 0x04, 0x02, 0x03,              // ADD r4, r2, r3
        0x01                                 // HALT
    };
    
    // Benchmark compilation time
    clock_t start = clock();
    
    const int iterations = 100;
    for (int i = 0; i < iterations; i++) {
        JITResult result = jit_compile_bytecode(jit, bytecode, sizeof(bytecode), 0);
        TEST_ASSERT(result == JIT_SUCCESS, "Compilation should succeed in benchmark");
    }
    
    clock_t end = clock();
    double total_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    double avg_time = total_time / iterations;
    
    printf("    Compiled %d programs in %.3f seconds\n", iterations, total_time);
    printf("    Average compilation time: %.6f seconds\n", avg_time);
    printf("    Compilation rate: %.1f programs/second\n", iterations / total_time);
    
    // Get final statistics
    JITStats stats;
    jit_get_stats(jit, &stats);
    printf("    Cache hits: %zu, Cache misses: %zu\n", stats.cache_hits, stats.cache_misses);
    
    jit_cleanup(jit);
    TEST_PASS();
}

int test_jit_version_info(void) {
    const char* version = jit_get_version();
    TEST_ASSERT(version != NULL, "Version string should not be NULL");
    TEST_ASSERT(strlen(version) > 0, "Version string should not be empty");
    
    printf("  JIT Compiler Version: %s\n", version);
    
    // Test info printing (visual test)
    printf("  JIT Compiler Information:\n");
    jit_print_info();
    
    TEST_PASS();
}

// ===============================================
// Main Test Runner
// ===============================================

int main(void) {
    printf("=== JIT Core Tests ===\n\n");
    
    // Initialize JIT cache for tests
    jit_cache_init(1024 * 1024);
    
    // Run all tests
    RUN_TEST(test_jit_initialization);
    RUN_TEST(test_jit_architecture_support);
    RUN_TEST(test_jit_bytecode_compilation);
    RUN_TEST(test_jit_cache_functionality);
    RUN_TEST(test_jit_hash_function);
    RUN_TEST(test_jit_statistics);
    RUN_TEST(test_jit_error_handling);
    RUN_TEST(test_jit_performance_benchmark);
    RUN_TEST(test_jit_version_info);
    
    // Cleanup
    jit_cache_cleanup();
    
    // Print summary
    printf("\n=== Test Summary ===\n");
    printf("Total tests: %d\n", total_tests);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Success rate: %.1f%%\n", (double)tests_passed / total_tests * 100.0);
    
    if (tests_failed == 0) {
        printf("\nAll JIT tests passed! ✓\n");
        return 0;
    } else {
        printf("\nSome JIT tests failed! ✗\n");
        return 1;
    }
}
