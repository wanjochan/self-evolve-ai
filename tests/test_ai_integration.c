/**
 * test_ai_integration.c - AI Integration Test Program
 * 
 * Tests the AI evolution integration functionality
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/ai/ai_integration.h"
#include "../src/core/include/module_attributes.h"

MODULE("ai_test")
VERSION(1, 0, 0)
AUTHOR("Self-Evolve AI")
DESCRIPTION("AI Integration Test Module")
LICENSE("MIT")
void ai_test_init(void) {
    printf("AI Integration Test Module initialized\n");
}

// Test function that could benefit from AI optimization
EXPORT_FUNC
int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// Test function with performance bottleneck
EXPORT_FUNC
void bubble_sort(int arr[], int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

// Test function for runtime metrics collection
EXPORT_FUNC
double compute_intensive_task(int iterations) {
    double result = 0.0;
    for (int i = 0; i < iterations; i++) {
        for (int j = 0; j < 1000; j++) {
            result += (double)(i * j) / (i + j + 1);
        }
    }
    return result;
}

// Simulate runtime metrics
RuntimeMetrics create_test_metrics(void) {
    RuntimeMetrics metrics = {0};
    metrics.execution_count = 100;
    metrics.total_execution_time = 50000; // 50ms
    metrics.memory_usage_peak = 1024 * 1024; // 1MB
    metrics.cache_misses = 150;
    metrics.branch_mispredictions = 25;
    metrics.average_cpu_usage = 75.5;
    metrics.error_count = 0;
    return metrics;
}

// Test AI configuration
void test_ai_configuration(void) {
    printf("\n=== Testing AI Configuration ===\n");
    
    AIIntegrationConfig config = ai_get_default_config();
    config.enable_code_optimization = 1;
    config.enable_runtime_adaptation = 1;
    config.enable_learning = 1;
    config.enable_self_modification = 0; // Keep disabled for safety
    config.optimization_threshold = 0.05;
    config.max_evolution_cycles = 3;
    
    if (ai_configure_integration(&config) == 0) {
        printf("✓ AI configuration successful\n");
    } else {
        printf("✗ AI configuration failed\n");
    }
}

// Test AI health check
void test_ai_health_check(void) {
    printf("\n=== Testing AI Health Check ===\n");
    
    if (ai_integration_health_check() == 0) {
        printf("✓ AI integration is healthy\n");
    } else {
        printf("✗ AI integration has issues\n");
    }
}

// Test AI statistics
void test_ai_statistics(void) {
    printf("\n=== Testing AI Statistics ===\n");
    
    AIIntegrationStats stats;
    ai_get_integration_stats(&stats);
    
    printf("AI Integration Statistics:\n");
    printf("  Total compilations: %llu\n", (unsigned long long)stats.total_compilations);
    printf("  Successful optimizations: %llu\n", (unsigned long long)stats.successful_optimizations);
    printf("  Average improvement: %.2f%%\n", stats.average_improvement * 100);
    printf("  Learning database size: %llu\n", (unsigned long long)stats.learning_database_size);
    printf("  Evolution stats:\n");
    printf("    Total evolutions: %llu\n", (unsigned long long)stats.evolution_stats.total_evolutions);
    printf("    Successful evolutions: %llu\n", (unsigned long long)stats.evolution_stats.successful_evolutions);
    printf("  Analysis stats:\n");
    printf("    Total analyses: %llu\n", (unsigned long long)stats.analysis_stats.total_analyses);
    printf("    Hotspots found: %llu\n", (unsigned long long)stats.analysis_stats.hotspots_found);
}

// Test AI learning
void test_ai_learning(void) {
    printf("\n=== Testing AI Learning ===\n");
    
    // Simulate some bytecode
    uint8_t test_bytecode[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    size_t bytecode_size = sizeof(test_bytecode);
    
    RuntimeMetrics metrics = create_test_metrics();
    
    if (ai_learn_from_execution(test_bytecode, bytecode_size, &metrics, "test_program") == 0) {
        printf("✓ AI learning successful\n");
    } else {
        printf("✗ AI learning failed\n");
    }
}

// Test runtime adaptation
void test_runtime_adaptation(void) {
    printf("\n=== Testing Runtime Adaptation ===\n");
    
    // Simulate some bytecode with poor performance
    uint8_t test_bytecode[] = {0x10, 0x20, 0x30, 0x40, 0x50};
    size_t bytecode_size = sizeof(test_bytecode);
    
    RuntimeMetrics metrics = create_test_metrics();
    metrics.average_cpu_usage = 95.0; // High CPU usage indicates poor performance
    metrics.cache_misses = 500; // High cache misses
    
    uint8_t* adapted_bytecode = NULL;
    size_t adapted_size = 0;
    
    if (ai_runtime_adaptation(test_bytecode, bytecode_size, &metrics, 
                             &adapted_bytecode, &adapted_size) == 0) {
        printf("✓ Runtime adaptation successful\n");
        if (adapted_bytecode) {
            printf("  Adapted bytecode size: %zu bytes\n", adapted_size);
            free(adapted_bytecode);
        }
    } else {
        printf("✗ Runtime adaptation failed or not needed\n");
    }
}

// Test compilation hooks
void test_compilation_hooks(void) {
    printf("\n=== Testing Compilation Hooks ===\n");
    
    // Test C to ASTC hook
    int result = ai_hook_c2astc_compilation("test.c", "test.astc", NULL);
    printf("C to ASTC hook result: %d\n", result);
    
    // Test ASTC to native hook
    uint8_t test_bytecode[] = {0xAA, 0xBB, 0xCC, 0xDD};
    result = ai_hook_astc2native_conversion("test.astc", "test.native", 
                                           test_bytecode, sizeof(test_bytecode));
    printf("ASTC to native hook result: %d\n", result);
    
    // Test runtime hook
    RuntimeMetrics metrics = create_test_metrics();
    result = ai_hook_runtime_execution(test_bytecode, sizeof(test_bytecode), &metrics);
    printf("Runtime execution hook result: %d\n", result);
}

// Main test function
int main(int argc, char* argv[]) {
    printf("AI Integration Test Program\n");
    printf("===========================\n");
    
    // Initialize AI integration
    printf("Initializing AI integration...\n");
    if (ai_integration_init(NULL) != 0) {
        printf("Failed to initialize AI integration\n");
        printf("Note: This is expected as the full AI system is not implemented\n");
        printf("Continuing with mock tests...\n");
    }
    
    // Run tests
    test_ai_configuration();
    test_ai_health_check();
    test_ai_statistics();
    test_ai_learning();
    test_runtime_adaptation();
    test_compilation_hooks();
    
    // Test some actual functions
    printf("\n=== Testing Actual Functions ===\n");
    
    printf("Computing fibonacci(10): %d\n", fibonacci(10));
    
    int test_array[] = {64, 34, 25, 12, 22, 11, 90};
    int array_size = sizeof(test_array) / sizeof(test_array[0]);
    printf("Before sorting: ");
    for (int i = 0; i < array_size; i++) {
        printf("%d ", test_array[i]);
    }
    printf("\n");
    
    bubble_sort(test_array, array_size);
    printf("After sorting: ");
    for (int i = 0; i < array_size; i++) {
        printf("%d ", test_array[i]);
    }
    printf("\n");
    
    double result = compute_intensive_task(100);
    printf("Compute intensive task result: %.6f\n", result);
    
    // Cleanup
    printf("\nCleaning up AI integration...\n");
    ai_integration_cleanup();
    
    printf("\nAI Integration test completed!\n");
    printf("Note: Many functions return mock results as the full AI system\n");
    printf("is not implemented, but the integration framework is in place.\n");
    
    return 0;
}
