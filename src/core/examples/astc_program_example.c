/**
 * astc_program_example.c - Example ASTC Program Using Modules
 * 
 * Demonstrates how ASTC programs can import and use system modules
 * like libc.rt and math.rt for common operations.
 */

#include "../include/astc_program_modules.h"
#include "../include/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simulate an ASTC program that uses multiple modules
int simulate_astc_program_with_modules(void) {
    LOG_RUNTIME_INFO("=== Simulating ASTC Program with Modules ===");
    
    // Initialize program module system
    if (astc_program_modules_init("example_program.astc", "/path/to/example_program.astc") != 0) {
        LOG_RUNTIME_ERROR("Failed to initialize program module system");
        return -1;
    }
    
    // Import system modules
    LOG_RUNTIME_INFO("Importing system modules...");
    
    if (ASTC_IMPORT_LIBC() != 0) {
        LOG_RUNTIME_ERROR("Failed to import libc.rt");
        return -1;
    }
    
    if (ASTC_IMPORT_MATH() != 0) {
        LOG_RUNTIME_ERROR("Failed to import math.rt");
        return -1;
    }
    
    // List imported modules
    astc_program_list_modules();
    
    // Simulate ASTC program execution
    ASTCValue args[4];
    ASTCValue result;
    
    // Example 1: String operations
    LOG_RUNTIME_INFO("--- String Operations Example ---");
    
    // Simulate: strlen("Hello, World!")
    args[0] = ASTC_VALUE_STR("Hello, World!");
    if (ASTC_CALL_LIBC(ASTC_FUNC_STRLEN, args, 1, &result) == 0) {
        LOG_RUNTIME_INFO("strlen(\"Hello, World!\") = %lld", result.data.i64);
    }
    
    // Simulate: strcmp("apple", "banana")
    args[0] = ASTC_VALUE_STR("apple");
    args[1] = ASTC_VALUE_STR("banana");
    if (ASTC_CALL_LIBC(ASTC_FUNC_STRCMP, args, 2, &result) == 0) {
        LOG_RUNTIME_INFO("strcmp(\"apple\", \"banana\") = %d", result.data.i32);
    }
    
    // Example 2: Math operations
    LOG_RUNTIME_INFO("--- Math Operations Example ---");
    
    // Simulate: sqrt(16.0)
    args[0] = ASTC_VALUE_F64(16.0);
    if (ASTC_CALL_MATH(ASTC_FUNC_SQRT, args, 1, &result) == 0) {
        LOG_RUNTIME_INFO("sqrt(16.0) = %f", result.data.f64);
    }
    
    // Simulate: pow(2.0, 8.0)
    args[0] = ASTC_VALUE_F64(2.0);
    args[1] = ASTC_VALUE_F64(8.0);
    if (ASTC_CALL_MATH(ASTC_FUNC_POW, args, 2, &result) == 0) {
        LOG_RUNTIME_INFO("pow(2.0, 8.0) = %f", result.data.f64);
    }
    
    // Simulate: sin(3.14159/2)
    args[0] = ASTC_VALUE_F64(3.14159 / 2.0);
    if (ASTC_CALL_MATH(ASTC_FUNC_SIN, args, 1, &result) == 0) {
        LOG_RUNTIME_INFO("sin(π/2) = %f", result.data.f64);
    }
    
    // Example 3: Memory operations
    LOG_RUNTIME_INFO("--- Memory Operations Example ---");
    
    // Simulate: malloc(1024)
    args[0] = ASTC_VALUE_I64(1024);
    if (ASTC_CALL_LIBC(ASTC_FUNC_MALLOC, args, 1, &result) == 0) {
        void* ptr = result.data.ptr;
        LOG_RUNTIME_INFO("malloc(1024) = %p", ptr);
        
        // Simulate: free(ptr)
        args[0] = ASTC_VALUE_PTR(ptr);
        if (ASTC_CALL_LIBC(ASTC_FUNC_FREE, args, 1, &result) == 0) {
            LOG_RUNTIME_INFO("free(%p) completed", ptr);
        }
    }
    
    LOG_RUNTIME_INFO("ASTC program simulation completed successfully");
    return 0;
}

// Simulate a more complex ASTC program
int simulate_complex_astc_program(void) {
    LOG_RUNTIME_INFO("=== Complex ASTC Program Simulation ===");
    
    // This simulates a program that calculates the area of various shapes
    ASTCValue args[4];
    ASTCValue result;
    
    LOG_RUNTIME_INFO("Calculating areas of geometric shapes...");
    
    // Circle area: π * r²
    double radius = 5.0;
    LOG_RUNTIME_INFO("Circle with radius %f:", radius);
    
    // Calculate r²
    args[0] = ASTC_VALUE_F64(radius);
    args[1] = ASTC_VALUE_F64(2.0);
    if (ASTC_CALL_MATH(ASTC_FUNC_POW, args, 2, &result) == 0) {
        double r_squared = result.data.f64;
        double pi = 3.14159265359;
        double circle_area = pi * r_squared;
        LOG_RUNTIME_INFO("  Area = π * %f² = %f", radius, circle_area);
    }
    
    // Right triangle area: 0.5 * base * height
    double base = 8.0, height = 6.0;
    double triangle_area = 0.5 * base * height;
    LOG_RUNTIME_INFO("Right triangle (base=%f, height=%f):", base, height);
    LOG_RUNTIME_INFO("  Area = 0.5 * %f * %f = %f", base, height, triangle_area);
    
    // Calculate hypotenuse using Pythagorean theorem: √(base² + height²)
    args[0] = ASTC_VALUE_F64(base);
    args[1] = ASTC_VALUE_F64(2.0);
    if (ASTC_CALL_MATH(ASTC_FUNC_POW, args, 2, &result) == 0) {
        double base_squared = result.data.f64;
        
        args[0] = ASTC_VALUE_F64(height);
        args[1] = ASTC_VALUE_F64(2.0);
        if (ASTC_CALL_MATH(ASTC_FUNC_POW, args, 2, &result) == 0) {
            double height_squared = result.data.f64;
            
            args[0] = ASTC_VALUE_F64(base_squared + height_squared);
            if (ASTC_CALL_MATH(ASTC_FUNC_SQRT, args, 1, &result) == 0) {
                double hypotenuse = result.data.f64;
                LOG_RUNTIME_INFO("  Hypotenuse = √(%f² + %f²) = %f", base, height, hypotenuse);
            }
        }
    }
    
    LOG_RUNTIME_INFO("Complex program simulation completed");
    return 0;
}

// Test module introspection
void test_module_introspection(void) {
    LOG_RUNTIME_INFO("=== Testing Module Introspection ===");
    
    // Get information about libc.rt module
    ASTCProgramModuleInfo info;
    if (astc_program_get_module_info("libc.rt", &info) == 0) {
        LOG_RUNTIME_INFO("Module: %s", info.module_name);
        LOG_RUNTIME_INFO("  Version: %s", info.version);
        LOG_RUNTIME_INFO("  Type: %s", info.is_system_module ? "System" : "User");
        LOG_RUNTIME_INFO("  Functions: %d", info.function_count);
        LOG_RUNTIME_INFO("  Loaded: %s", info.is_loaded ? "Yes" : "No");
    }
    
    // Test function lookup
    const ASTCFunctionInfo* func_info = astc_program_find_function("libc.rt", "strlen");
    if (func_info) {
        LOG_RUNTIME_INFO("Function: strlen");
        LOG_RUNTIME_INFO("  Parameters: %d", func_info->param_count);
        LOG_RUNTIME_INFO("  Return type: %d", func_info->return_type);
        LOG_RUNTIME_INFO("  Description: %s", func_info->description);
    }
    
    // Test function lookup for math module
    func_info = astc_program_find_function("math.rt", "sqrt");
    if (func_info) {
        LOG_RUNTIME_INFO("Function: sqrt");
        LOG_RUNTIME_INFO("  Parameters: %d", func_info->param_count);
        LOG_RUNTIME_INFO("  Return type: %d", func_info->return_type);
    }
}

// Test error handling
void test_error_handling(void) {
    LOG_RUNTIME_INFO("=== Testing Error Handling ===");
    
    ASTCValue args[4];
    ASTCValue result;
    
    // Test calling non-existent function
    LOG_RUNTIME_INFO("Testing call to non-existent function...");
    args[0] = ASTC_VALUE_I32(42);
    if (astc_program_call_function("libc.rt", "nonexistent", args, 1, &result) != 0) {
        LOG_RUNTIME_INFO("Expected error: function not found");
    }
    
    // Test calling function with wrong argument count
    LOG_RUNTIME_INFO("Testing call with wrong argument count...");
    if (astc_program_call_function("libc.rt", "strlen", args, 0, &result) != 0) {
        LOG_RUNTIME_INFO("Expected error: argument count mismatch");
    }
    
    // Test calling function from non-existent module
    LOG_RUNTIME_INFO("Testing call to non-existent module...");
    if (astc_program_call_function("nonexistent.rt", "somefunction", args, 1, &result) != 0) {
        LOG_RUNTIME_INFO("Expected error: module not found");
    }
}

// Main example function
int astc_program_example_main(void) {
    LOG_RUNTIME_INFO("=== ASTC Program Module System Example ===");
    
    // Initialize logger
    if (logger_init() != 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        return -1;
    }
    
    // Initialize ASTC-Native bridge (required for system modules)
    if (astc_native_bridge_init() != 0) {
        LOG_RUNTIME_ERROR("Failed to initialize ASTC-Native bridge");
        return -1;
    }
    
    // Register standard library interfaces
    if (astc_native_register_stdlib() != 0) {
        LOG_RUNTIME_WARN("Failed to register standard library interfaces");
    }
    
    // Run simulations
    if (simulate_astc_program_with_modules() != 0) {
        LOG_RUNTIME_ERROR("Basic program simulation failed");
        return -1;
    }
    
    if (simulate_complex_astc_program() != 0) {
        LOG_RUNTIME_ERROR("Complex program simulation failed");
        return -1;
    }
    
    // Run tests
    test_module_introspection();
    test_error_handling();
    
    // Cleanup
    astc_program_modules_cleanup();
    astc_native_bridge_cleanup();
    logger_cleanup();
    
    LOG_RUNTIME_INFO("ASTC program module system example completed successfully");
    return 0;
}

// Entry point for standalone testing
#ifdef ASTC_PROGRAM_EXAMPLE_STANDALONE
int main(void) {
    return astc_program_example_main();
}
#endif
