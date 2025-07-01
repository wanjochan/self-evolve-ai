/**
 * module_comm_example.c - Example of Inter-module Communication
 * 
 * Demonstrates how .native modules can communicate with each other
 * using the module communication system.
 */

#include "../include/module_communication.h"
#include "../include/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Example math module functions
int math_add(ModuleCallArg a, ModuleCallArg b) {
    int result = a.int32_val + b.int32_val;
    LOG_MODULE_INFO("Math module: %d + %d = %d", a.int32_val, b.int32_val, result);
    return result;
}

int math_multiply(ModuleCallArg a, ModuleCallArg b) {
    int result = a.int32_val * b.int32_val;
    LOG_MODULE_INFO("Math module: %d * %d = %d", a.int32_val, b.int32_val, result);
    return result;
}

// Example string module functions
int string_length(ModuleCallArg str_arg) {
    const char* str = str_arg.str_val;
    if (!str) return -1;
    
    int len = strlen(str);
    LOG_MODULE_INFO("String module: length of '%s' = %d", str, len);
    return len;
}

int string_compare(ModuleCallArg str1_arg, ModuleCallArg str2_arg) {
    const char* str1 = str1_arg.str_val;
    const char* str2 = str2_arg.str_val;
    
    if (!str1 || !str2) return -1;
    
    int result = strcmp(str1, str2);
    LOG_MODULE_INFO("String module: compare '%s' vs '%s' = %d", str1, str2, result);
    return result;
}

// Register math module interfaces
int register_math_module(void) {
    ModuleCallSignature sig;
    
    // Register math_add interface
    MODULE_SIG_INIT(sig, "Add two integers");
    MODULE_SIG_ADD_ARG(sig, MODULE_ARG_INT32);
    MODULE_SIG_ADD_ARG(sig, MODULE_ARG_INT32);
    MODULE_SIG_SET_RETURN(sig, MODULE_RETURN_INT32);
    
    if (module_comm_register_interface("math.add", "math_module", 
                                      (void*)math_add, &sig) != 0) {
        LOG_MODULE_ERROR("Failed to register math.add interface");
        return -1;
    }
    
    // Register math_multiply interface
    MODULE_SIG_INIT(sig, "Multiply two integers");
    MODULE_SIG_ADD_ARG(sig, MODULE_ARG_INT32);
    MODULE_SIG_ADD_ARG(sig, MODULE_ARG_INT32);
    MODULE_SIG_SET_RETURN(sig, MODULE_RETURN_INT32);
    
    if (module_comm_register_interface("math.multiply", "math_module", 
                                      (void*)math_multiply, &sig) != 0) {
        LOG_MODULE_ERROR("Failed to register math.multiply interface");
        return -1;
    }
    
    LOG_MODULE_INFO("Math module interfaces registered successfully");
    return 0;
}

// Register string module interfaces
int register_string_module(void) {
    ModuleCallSignature sig;
    
    // Register string_length interface
    MODULE_SIG_INIT(sig, "Get string length");
    MODULE_SIG_ADD_ARG(sig, MODULE_ARG_STRING);
    MODULE_SIG_SET_RETURN(sig, MODULE_RETURN_INT32);
    
    if (module_comm_register_interface("string.length", "string_module", 
                                      (void*)string_length, &sig) != 0) {
        LOG_MODULE_ERROR("Failed to register string.length interface");
        return -1;
    }
    
    // Register string_compare interface
    MODULE_SIG_INIT(sig, "Compare two strings");
    MODULE_SIG_ADD_ARG(sig, MODULE_ARG_STRING);
    MODULE_SIG_ADD_ARG(sig, MODULE_ARG_STRING);
    MODULE_SIG_SET_RETURN(sig, MODULE_RETURN_INT32);
    
    if (module_comm_register_interface("string.compare", "string_module", 
                                      (void*)string_compare, &sig) != 0) {
        LOG_MODULE_ERROR("Failed to register string.compare interface");
        return -1;
    }
    
    LOG_MODULE_INFO("String module interfaces registered successfully");
    return 0;
}

// Test synchronous calls
void test_sync_calls(void) {
    LOG_MODULE_INFO("=== Testing Synchronous Calls ===");
    
    ModuleCallContext ctx;
    
    // Test math.add
    MODULE_CALL_INIT(ctx);
    MODULE_CALL_ADD_ARG_INT32(ctx, 15);
    MODULE_CALL_ADD_ARG_INT32(ctx, 27);
    
    if (module_comm_call_sync("math.add", &ctx) == 0) {
        LOG_MODULE_INFO("math.add result: %d", ctx.return_value.int_val);
    } else {
        LOG_MODULE_ERROR("math.add call failed");
    }
    
    // Test math.multiply
    MODULE_CALL_INIT(ctx);
    MODULE_CALL_ADD_ARG_INT32(ctx, 6);
    MODULE_CALL_ADD_ARG_INT32(ctx, 7);
    
    if (module_comm_call_sync("math.multiply", &ctx) == 0) {
        LOG_MODULE_INFO("math.multiply result: %d", ctx.return_value.int_val);
    } else {
        LOG_MODULE_ERROR("math.multiply call failed");
    }
    
    // Test string.length
    MODULE_CALL_INIT(ctx);
    MODULE_CALL_ADD_ARG_STR(ctx, "Hello, World!");
    
    if (module_comm_call_sync("string.length", &ctx) == 0) {
        LOG_MODULE_INFO("string.length result: %d", ctx.return_value.int_val);
    } else {
        LOG_MODULE_ERROR("string.length call failed");
    }
    
    // Test string.compare
    MODULE_CALL_INIT(ctx);
    MODULE_CALL_ADD_ARG_STR(ctx, "apple");
    MODULE_CALL_ADD_ARG_STR(ctx, "banana");
    
    if (module_comm_call_sync("string.compare", &ctx) == 0) {
        LOG_MODULE_INFO("string.compare result: %d", ctx.return_value.int_val);
    } else {
        LOG_MODULE_ERROR("string.compare call failed");
    }
}

// Test asynchronous calls
void test_async_calls(void) {
    LOG_MODULE_INFO("=== Testing Asynchronous Calls ===");
    
    ModuleCallContext ctx;
    
    // Start async math.add call
    MODULE_CALL_INIT(ctx);
    MODULE_CALL_ADD_ARG_INT32(ctx, 100);
    MODULE_CALL_ADD_ARG_INT32(ctx, 200);
    
    uint32_t call_id = module_comm_call_async("math.add", &ctx);
    if (call_id != 0) {
        LOG_MODULE_INFO("Started async math.add call with ID: %u", call_id);
        
        // Check result
        ModuleCallContext result;
        ModuleCallStatus status = module_comm_check_async(call_id, &result);
        if (status == MODULE_CALL_SUCCESS) {
            LOG_MODULE_INFO("Async math.add result: %d", result.return_value.int_val);
        } else {
            LOG_MODULE_ERROR("Async math.add call status: %d", status);
        }
    } else {
        LOG_MODULE_ERROR("Failed to start async math.add call");
    }
}

// Main example function
int module_comm_example_main(void) {
    LOG_MODULE_INFO("=== Module Communication Example ===");
    
    // Initialize systems
    if (logger_init() != 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        return -1;
    }
    
    if (module_comm_init() != 0) {
        LOG_MODULE_ERROR("Failed to initialize module communication");
        return -1;
    }
    
    // Register modules
    if (register_math_module() != 0) {
        LOG_MODULE_ERROR("Failed to register math module");
        return -1;
    }
    
    if (register_string_module() != 0) {
        LOG_MODULE_ERROR("Failed to register string module");
        return -1;
    }
    
    // List registered interfaces
    module_comm_list_interfaces();
    
    // Test synchronous calls
    test_sync_calls();
    
    // Test asynchronous calls
    test_async_calls();
    
    // Cleanup
    module_comm_cleanup();
    logger_cleanup();
    
    LOG_MODULE_INFO("Module communication example completed successfully");
    return 0;
}

// Entry point for standalone testing
#ifdef MODULE_COMM_EXAMPLE_STANDALONE
int main(void) {
    return module_comm_example_main();
}
#endif
