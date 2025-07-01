/**
 * module_communication.c - Inter-module Communication System
 * 
 * Implements high-performance communication between .native modules
 * including function calls, data exchange, and event handling.
 */

#include "include/module_communication.h"
#include "include/logger.h"
#include "loader/module_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// Maximum number of registered interfaces
#define MAX_INTERFACES 256

// Maximum number of pending calls
#define MAX_PENDING_CALLS 1024

// Interface registry
typedef struct {
    char name[128];
    char module_name[128];
    void* function_ptr;
    ModuleCallSignature signature;
    bool is_active;
} RegisteredInterface;

// Call context for async operations
typedef struct {
    uint32_t call_id;
    char interface_name[128];
    ModuleCallContext context;
    bool is_pending;
    uint64_t timestamp;
} PendingCall;

// Global communication state
static struct {
    RegisteredInterface interfaces[MAX_INTERFACES];
    int interface_count;
    PendingCall pending_calls[MAX_PENDING_CALLS];
    int pending_count;
    uint32_t next_call_id;
    bool initialized;
} g_comm_state = {0};

// Initialize module communication system
int module_comm_init(void) {
    if (g_comm_state.initialized) {
        return 0;
    }

    memset(&g_comm_state, 0, sizeof(g_comm_state));
    g_comm_state.next_call_id = 1;
    g_comm_state.initialized = true;

    LOG_MODULE_INFO("Module communication system initialized");
    return 0;
}

// Cleanup module communication system
void module_comm_cleanup(void) {
    if (!g_comm_state.initialized) {
        return;
    }

    // Clear all interfaces and pending calls
    memset(&g_comm_state, 0, sizeof(g_comm_state));
    LOG_MODULE_INFO("Module communication system cleaned up");
}

// Register an interface for inter-module calls
int module_comm_register_interface(const char* interface_name, const char* module_name,
                                  void* function_ptr, const ModuleCallSignature* signature) {
    if (!interface_name || !module_name || !function_ptr || !signature) {
        SET_ERROR(ERROR_INVALID_ARGUMENT, "Invalid arguments to register_interface");
        return -1;
    }

    if (g_comm_state.interface_count >= MAX_INTERFACES) {
        SET_ERROR(ERROR_MEMORY_ALLOCATION, "Maximum interfaces reached");
        return -1;
    }

    // Check if interface already exists
    for (int i = 0; i < g_comm_state.interface_count; i++) {
        if (strcmp(g_comm_state.interfaces[i].name, interface_name) == 0) {
            LOG_MODULE_WARN("Interface %s already registered, updating", interface_name);
            g_comm_state.interfaces[i].function_ptr = function_ptr;
            g_comm_state.interfaces[i].signature = *signature;
            g_comm_state.interfaces[i].is_active = true;
            return 0;
        }
    }

    // Register new interface
    RegisteredInterface* iface = &g_comm_state.interfaces[g_comm_state.interface_count];
    strncpy(iface->name, interface_name, sizeof(iface->name) - 1);
    strncpy(iface->module_name, module_name, sizeof(iface->module_name) - 1);
    iface->function_ptr = function_ptr;
    iface->signature = *signature;
    iface->is_active = true;

    g_comm_state.interface_count++;

    LOG_MODULE_INFO("Registered interface: %s from module %s", interface_name, module_name);
    return 0;
}

// Unregister an interface
int module_comm_unregister_interface(const char* interface_name) {
    if (!interface_name) {
        return -1;
    }

    for (int i = 0; i < g_comm_state.interface_count; i++) {
        if (strcmp(g_comm_state.interfaces[i].name, interface_name) == 0) {
            g_comm_state.interfaces[i].is_active = false;
            LOG_MODULE_INFO("Unregistered interface: %s", interface_name);
            return 0;
        }
    }

    return -1; // Not found
}

// Find a registered interface
static RegisteredInterface* find_interface(const char* interface_name) {
    for (int i = 0; i < g_comm_state.interface_count; i++) {
        if (g_comm_state.interfaces[i].is_active &&
            strcmp(g_comm_state.interfaces[i].name, interface_name) == 0) {
            return &g_comm_state.interfaces[i];
        }
    }
    return NULL;
}

// Make a synchronous call to another module
int module_comm_call_sync(const char* interface_name, ModuleCallContext* context) {
    if (!interface_name || !context) {
        SET_ERROR(ERROR_INVALID_ARGUMENT, "Invalid arguments to call_sync");
        return -1;
    }

    RegisteredInterface* iface = find_interface(interface_name);
    if (!iface) {
        SET_ERROR(ERROR_SYMBOL_NOT_FOUND, "Interface not found: %s", interface_name);
        return -1;
    }

    LOG_MODULE_DEBUG("Making sync call to interface: %s", interface_name);

    // Validate argument count
    if (context->arg_count != iface->signature.arg_count) {
        SET_ERROR(ERROR_INVALID_ARGUMENT, "Argument count mismatch for %s: expected %d, got %d",
                 interface_name, iface->signature.arg_count, context->arg_count);
        return -1;
    }

    // Make the call based on signature
    switch (iface->signature.arg_count) {
        case 0: {
            typedef int (*func0_t)(void);
            func0_t func = (func0_t)iface->function_ptr;
            context->return_value.int_val = func();
            break;
        }
        case 1: {
            typedef int (*func1_t)(ModuleCallArg);
            func1_t func = (func1_t)iface->function_ptr;
            context->return_value.int_val = func(context->args[0]);
            break;
        }
        case 2: {
            typedef int (*func2_t)(ModuleCallArg, ModuleCallArg);
            func2_t func = (func2_t)iface->function_ptr;
            context->return_value.int_val = func(context->args[0], context->args[1]);
            break;
        }
        case 3: {
            typedef int (*func3_t)(ModuleCallArg, ModuleCallArg, ModuleCallArg);
            func3_t func = (func3_t)iface->function_ptr;
            context->return_value.int_val = func(context->args[0], context->args[1], context->args[2]);
            break;
        }
        default:
            SET_ERROR(ERROR_INVALID_ARGUMENT, "Unsupported argument count: %d", iface->signature.arg_count);
            return -1;
    }

    context->status = MODULE_CALL_SUCCESS;
    LOG_MODULE_DEBUG("Sync call to %s completed successfully", interface_name);
    return 0;
}

// Make an asynchronous call to another module
uint32_t module_comm_call_async(const char* interface_name, ModuleCallContext* context) {
    if (!interface_name || !context) {
        SET_ERROR(ERROR_INVALID_ARGUMENT, "Invalid arguments to call_async");
        return 0;
    }

    RegisteredInterface* iface = find_interface(interface_name);
    if (!iface) {
        SET_ERROR(ERROR_SYMBOL_NOT_FOUND, "Interface not found: %s", interface_name);
        return 0;
    }

    if (g_comm_state.pending_count >= MAX_PENDING_CALLS) {
        SET_ERROR(ERROR_MEMORY_ALLOCATION, "Maximum pending calls reached");
        return 0;
    }

    // Create pending call
    uint32_t call_id = g_comm_state.next_call_id++;
    PendingCall* pending = &g_comm_state.pending_calls[g_comm_state.pending_count];
    pending->call_id = call_id;
    strncpy(pending->interface_name, interface_name, sizeof(pending->interface_name) - 1);
    pending->context = *context;
    pending->is_pending = true;
    pending->timestamp = (uint64_t)time(NULL);

    g_comm_state.pending_count++;

    LOG_MODULE_DEBUG("Created async call %u to interface: %s", call_id, interface_name);

    // For now, execute immediately (in a real implementation, this would be queued)
    int result = module_comm_call_sync(interface_name, &pending->context);
    if (result == 0) {
        pending->context.status = MODULE_CALL_SUCCESS;
    } else {
        pending->context.status = MODULE_CALL_ERROR;
    }

    return call_id;
}

// Check status of an asynchronous call
ModuleCallStatus module_comm_check_async(uint32_t call_id, ModuleCallContext* result) {
    for (int i = 0; i < g_comm_state.pending_count; i++) {
        if (g_comm_state.pending_calls[i].call_id == call_id) {
            if (result) {
                *result = g_comm_state.pending_calls[i].context;
            }
            return g_comm_state.pending_calls[i].context.status;
        }
    }
    return MODULE_CALL_NOT_FOUND;
}

// List all registered interfaces
void module_comm_list_interfaces(void) {
    LOG_MODULE_INFO("Registered interfaces (%d):", g_comm_state.interface_count);
    for (int i = 0; i < g_comm_state.interface_count; i++) {
        if (g_comm_state.interfaces[i].is_active) {
            RegisteredInterface* iface = &g_comm_state.interfaces[i];
            LOG_MODULE_INFO("  %s (module: %s, args: %d, return: %d)",
                           iface->name, iface->module_name,
                           iface->signature.arg_count, iface->signature.return_type);
        }
    }
}

// Get interface information
int module_comm_get_interface_info(const char* interface_name, ModuleInterfaceInfo* info) {
    if (!interface_name || !info) {
        return -1;
    }

    RegisteredInterface* iface = find_interface(interface_name);
    if (!iface) {
        return -1;
    }

    strncpy(info->name, iface->name, sizeof(info->name) - 1);
    strncpy(info->module_name, iface->module_name, sizeof(info->module_name) - 1);
    info->signature = iface->signature;
    info->is_active = iface->is_active;

    return 0;
}
