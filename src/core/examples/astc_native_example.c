/**
 * astc_native_example.c - Example of ASTC-Native Bridge Usage
 * 
 * Demonstrates how ASTC programs can call native module functions
 * through the standardized bridge interface.
 */

#include "../include/astc_native_bridge.h"
#include "../include/logger.h"
#include "../include/module_communication.h"
#include "../loader/module_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Example native functions that could be in a .native module
int example_add(int a, int b) {
    int result = a + b;
    LOG_MODULE_INFO("Native add: %d + %d = %d", a, b, result);
    return result;
}

double example_sqrt(double x) {
    double result = sqrt(x);
    LOG_MODULE_INFO("Native sqrt: %f = %f", x, result);
    return result;
}

const char* example_hello(const char* name) {
    static char buffer[256];
    snprintf(buffer, sizeof(buffer), "Hello, %s!", name);
    LOG_MODULE_INFO("Native hello: %s", buffer);
    return buffer;
}

// Register example native functions
int register_example_native_functions(void) {
    ASTCCallSignature sig;
    
    // Register add function
    ASTC_SIG_INIT(sig, "Add two integers");
    ASTC_SIG_ADD_PARAM(sig, ASTC_TYPE_I32);
    ASTC_SIG_ADD_PARAM(sig, ASTC_TYPE_I32);
    ASTC_SIG_SET_RETURN(sig, ASTC_TYPE_I32);
    
    // For this example, we'll simulate the native function registration
    // In a real implementation, these would be resolved from .native modules
    if (astc_native_register_interface("math.add", "example_module", "add", &sig) != 0) {
        LOG_MODULE_ERROR("Failed to register math.add interface");
        return -1;
    }
    
    // Register sqrt function
    ASTC_SIG_INIT(sig, "Calculate square root");
    ASTC_SIG_ADD_PARAM(sig, ASTC_TYPE_F64);
    ASTC_SIG_SET_RETURN(sig, ASTC_TYPE_F64);
    
    if (astc_native_register_interface("math.sqrt", "example_module", "sqrt", &sig) != 0) {
        LOG_MODULE_ERROR("Failed to register math.sqrt interface");
        return -1;
    }
    
    // Register hello function
    ASTC_SIG_INIT(sig, "Generate greeting message");
    ASTC_SIG_ADD_PARAM(sig, ASTC_TYPE_STRING);
    ASTC_SIG_SET_RETURN(sig, ASTC_TYPE_STRING);
    
    if (astc_native_register_interface("string.hello", "example_module", "hello", &sig) != 0) {
        LOG_MODULE_ERROR("Failed to register string.hello interface");
        return -1;
    }
    
    LOG_MODULE_INFO("Example native functions registered successfully");
    return 0;
}

// Simulate ASTC VM calling native functions
void simulate_astc_calls(void) {
    LOG_MODULE_INFO("=== Simulating ASTC Native Calls ===");
    
    ASTCValue args[ASTC_MAX_CALL_PARAMS];
    ASTCValue result;
    
    // Test math.add call
    LOG_MODULE_INFO("Testing math.add(15, 27)");
    args[0] = ASTC_VALUE_I32(15);
    args[1] = ASTC_VALUE_I32(27);
    
    if (astc_native_call("math.add", args, 2, &result) == 0) {
        LOG_MODULE_INFO("math.add result: %d", result.data.i32);
    } else {
        LOG_MODULE_ERROR("math.add call failed");
    }
    
    // Test math.sqrt call
    LOG_MODULE_INFO("Testing math.sqrt(16.0)");
    args[0] = ASTC_VALUE_F64(16.0);
    
    if (astc_native_call("math.sqrt", args, 1, &result) == 0) {
        LOG_MODULE_INFO("math.sqrt result: %f", result.data.f64);
    } else {
        LOG_MODULE_ERROR("math.sqrt call failed");
    }
    
    // Test string.hello call
    LOG_MODULE_INFO("Testing string.hello(\"World\")");
    args[0] = ASTC_VALUE_STR("World");
    
    if (astc_native_call("string.hello", args, 1, &result) == 0) {
        LOG_MODULE_INFO("string.hello result: %s", result.data.str);
    } else {
        LOG_MODULE_ERROR("string.hello call failed");
    }
}

// Test error handling
void test_error_handling(void) {
    LOG_MODULE_INFO("=== Testing Error Handling ===");
    
    ASTCValue args[ASTC_MAX_CALL_PARAMS];
    ASTCValue result;
    
    // Test calling non-existent interface
    LOG_MODULE_INFO("Testing call to non-existent interface");
    args[0] = ASTC_VALUE_I32(42);
    
    if (astc_native_call("nonexistent.function", args, 1, &result) != 0) {
        LOG_MODULE_INFO("Expected error: interface not found");
    }
    
    // Test wrong argument count
    LOG_MODULE_INFO("Testing wrong argument count");
    args[0] = ASTC_VALUE_I32(10);
    
    if (astc_native_call("math.add", args, 1, &result) != 0) {
        LOG_MODULE_INFO("Expected error: argument count mismatch");
    }
    
    // Test type mismatch (this would be caught by a real VM)
    LOG_MODULE_INFO("Testing type mismatch");
    args[0] = ASTC_VALUE_STR("not a number");
    args[1] = ASTC_VALUE_I32(5);
    
    if (astc_native_call("math.add", args, 2, &result) != 0) {
        LOG_MODULE_INFO("Expected error: type mismatch");
    }
}

// Test interface introspection
void test_interface_introspection(void) {
    LOG_MODULE_INFO("=== Testing Interface Introspection ===");
    
    // List all interfaces
    astc_native_list_interfaces();
    
    // Get specific interface info
    ASTCNativeInterfaceInfo info;
    if (astc_native_get_interface_info("math.add", &info) == 0) {
        LOG_MODULE_INFO("Interface info for math.add:");
        LOG_MODULE_INFO("  Module: %s", info.module_name);
        LOG_MODULE_INFO("  Symbol: %s", info.native_symbol);
        LOG_MODULE_INFO("  Params: %d", info.signature.param_count);
        LOG_MODULE_INFO("  Return type: %d", info.signature.return_type);
        LOG_MODULE_INFO("  Description: %s", info.signature.description);
    }
}

// Simulate a complete ASTC program execution
void simulate_astc_program(void) {
    LOG_MODULE_INFO("=== Simulating Complete ASTC Program ===");
    
    // This simulates what an ASTC program might do:
    // 1. Call native malloc to allocate memory
    // 2. Call native functions to process data
    // 3. Call native printf to output results
    // 4. Call native free to clean up
    
    ASTCValue args[ASTC_MAX_CALL_PARAMS];
    ASTCValue result;
    
    LOG_MODULE_INFO("ASTC Program: Calculating area of circle");
    
    // Simulate: double radius = 5.0;
    double radius = 5.0;
    LOG_MODULE_INFO("ASTC: radius = %f", radius);
    
    // Simulate: double area = 3.14159 * radius * radius;
    // In real ASTC, this might call native math functions
    args[0] = ASTC_VALUE_F64(radius * radius);
    
    if (astc_native_call("math.sqrt", args, 1, &result) == 0) {
        // This is just for demonstration - normally we'd multiply by pi
        double area = 3.14159 * result.data.f64 * result.data.f64;
        LOG_MODULE_INFO("ASTC: calculated area = %f", area);
        
        // Simulate: printf("Area of circle: %f\n", area);
        // In real implementation, this would format the string and call printf
        LOG_MODULE_INFO("ASTC: would call printf with result");
    }
    
    LOG_MODULE_INFO("ASTC Program: execution completed");
}

// Main example function
int astc_native_example_main(void) {
    LOG_MODULE_INFO("=== ASTC-Native Bridge Example ===");
    
    // Initialize systems
    if (logger_init() != 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        return -1;
    }
    
    if (module_comm_init() != 0) {
        LOG_MODULE_ERROR("Failed to initialize module communication");
        return -1;
    }
    
    if (astc_native_bridge_init() != 0) {
        LOG_MODULE_ERROR("Failed to initialize ASTC-Native bridge");
        return -1;
    }
    
    // Register example native functions
    if (register_example_native_functions() != 0) {
        LOG_MODULE_ERROR("Failed to register example native functions");
        return -1;
    }
    
    // Register standard library (would normally be done automatically)
    if (astc_native_register_stdlib() != 0) {
        LOG_MODULE_WARN("Failed to register standard library interfaces");
    }
    
    // Run tests
    simulate_astc_calls();
    test_error_handling();
    test_interface_introspection();
    simulate_astc_program();
    
    // Cleanup
    astc_native_bridge_cleanup();
    module_comm_cleanup();
    logger_cleanup();
    
    LOG_MODULE_INFO("ASTC-Native bridge example completed successfully");
    return 0;
}

// Entry point for standalone testing
#ifdef ASTC_NATIVE_EXAMPLE_STANDALONE
int main(void) {
    return astc_native_example_main();
}
#endif
