/**
 * astc_native_bridge.c - ASTC to Native Module Bridge
 * 
 * Implements the bridge between ASTC bytecode programs and .native modules,
 * providing standardized calling conventions and data marshaling.
 */

#include "include/astc_native_bridge.h"
#include "include/module_communication.h"
#include "include/logger.h"
#include "loader/module_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Maximum number of registered native interfaces
#define MAX_NATIVE_INTERFACES 512

// Bridge state
typedef struct {
    char interface_name[128];
    char module_name[128];
    char native_symbol[128];
    ASTCCallSignature signature;
    void* native_function;
    bool is_active;
} NativeInterface;

static struct {
    NativeInterface interfaces[MAX_NATIVE_INTERFACES];
    int interface_count;
    bool initialized;
} g_bridge_state = {0};

// Initialize the ASTC-Native bridge
int astc_native_bridge_init(void) {
    if (g_bridge_state.initialized) {
        return 0;
    }

    memset(&g_bridge_state, 0, sizeof(g_bridge_state));
    g_bridge_state.initialized = true;

    LOG_RUNTIME_INFO("ASTC-Native bridge initialized");
    return 0;
}

// Cleanup the bridge
void astc_native_bridge_cleanup(void) {
    if (!g_bridge_state.initialized) {
        return;
    }

    memset(&g_bridge_state, 0, sizeof(g_bridge_state));
    LOG_RUNTIME_INFO("ASTC-Native bridge cleaned up");
}

// Register a native interface for ASTC access
int astc_native_register_interface(const char* interface_name, const char* module_name,
                                  const char* native_symbol, const ASTCCallSignature* signature) {
    if (!interface_name || !module_name || !native_symbol || !signature) {
        SET_ERROR(ERROR_INVALID_ARGUMENT, "Invalid arguments to register_interface");
        return -1;
    }

    if (g_bridge_state.interface_count >= MAX_NATIVE_INTERFACES) {
        SET_ERROR(ERROR_MEMORY_ALLOCATION, "Maximum native interfaces reached");
        return -1;
    }

    // Resolve the native function
    void* function_ptr = module_loader_resolve_symbol(module_name, native_symbol);
    if (!function_ptr) {
        SET_ERROR(ERROR_SYMBOL_NOT_FOUND, "Native symbol not found: %s.%s", module_name, native_symbol);
        return -1;
    }

    // Register the interface
    NativeInterface* iface = &g_bridge_state.interfaces[g_bridge_state.interface_count];
    strncpy(iface->interface_name, interface_name, sizeof(iface->interface_name) - 1);
    strncpy(iface->module_name, module_name, sizeof(iface->module_name) - 1);
    strncpy(iface->native_symbol, native_symbol, sizeof(iface->native_symbol) - 1);
    iface->signature = *signature;
    iface->native_function = function_ptr;
    iface->is_active = true;

    g_bridge_state.interface_count++;

    LOG_RUNTIME_INFO("Registered native interface: %s -> %s.%s", 
                    interface_name, module_name, native_symbol);
    return 0;
}

// Convert ASTC value to native argument
static int astc_value_to_native_arg(const ASTCValue* astc_val, ModuleCallArg* native_arg, ASTCDataType expected_type) {
    if (!astc_val || !native_arg) {
        return -1;
    }

    switch (expected_type) {
        case ASTC_TYPE_I32:
            if (astc_val->type != ASTC_TYPE_I32) {
                SET_ERROR(ERROR_INVALID_ARGUMENT, "Type mismatch: expected i32");
                return -1;
            }
            native_arg->int32_val = astc_val->data.i32;
            break;

        case ASTC_TYPE_I64:
            if (astc_val->type != ASTC_TYPE_I64) {
                SET_ERROR(ERROR_INVALID_ARGUMENT, "Type mismatch: expected i64");
                return -1;
            }
            native_arg->int64_val = astc_val->data.i64;
            break;

        case ASTC_TYPE_F32:
            if (astc_val->type != ASTC_TYPE_F32) {
                SET_ERROR(ERROR_INVALID_ARGUMENT, "Type mismatch: expected f32");
                return -1;
            }
            native_arg->float_val = astc_val->data.f32;
            break;

        case ASTC_TYPE_F64:
            if (astc_val->type != ASTC_TYPE_F64) {
                SET_ERROR(ERROR_INVALID_ARGUMENT, "Type mismatch: expected f64");
                return -1;
            }
            native_arg->double_val = astc_val->data.f64;
            break;

        case ASTC_TYPE_PTR:
            if (astc_val->type != ASTC_TYPE_PTR) {
                SET_ERROR(ERROR_INVALID_ARGUMENT, "Type mismatch: expected pointer");
                return -1;
            }
            native_arg->ptr_val = astc_val->data.ptr;
            break;

        case ASTC_TYPE_STRING:
            if (astc_val->type != ASTC_TYPE_STRING) {
                SET_ERROR(ERROR_INVALID_ARGUMENT, "Type mismatch: expected string");
                return -1;
            }
            native_arg->str_val = astc_val->data.str;
            break;

        default:
            SET_ERROR(ERROR_INVALID_ARGUMENT, "Unsupported ASTC type: %d", expected_type);
            return -1;
    }

    return 0;
}

// Convert native return value to ASTC value
static int native_return_to_astc_value(const ModuleCallReturn* native_ret, ASTCValue* astc_val, ASTCDataType expected_type) {
    if (!native_ret || !astc_val) {
        return -1;
    }

    astc_val->type = expected_type;

    switch (expected_type) {
        case ASTC_TYPE_I32:
            astc_val->data.i32 = native_ret->int_val;
            break;

        case ASTC_TYPE_I64:
            astc_val->data.i64 = native_ret->long_val;
            break;

        case ASTC_TYPE_F32:
            astc_val->data.f32 = native_ret->float_val;
            break;

        case ASTC_TYPE_F64:
            astc_val->data.f64 = native_ret->double_val;
            break;

        case ASTC_TYPE_PTR:
            astc_val->data.ptr = native_ret->ptr_val;
            break;

        case ASTC_TYPE_VOID:
            // No return value
            break;

        default:
            SET_ERROR(ERROR_INVALID_ARGUMENT, "Unsupported return type: %d", expected_type);
            return -1;
    }

    return 0;
}

// Find a registered native interface
static NativeInterface* find_native_interface(const char* interface_name) {
    for (int i = 0; i < g_bridge_state.interface_count; i++) {
        if (g_bridge_state.interfaces[i].is_active &&
            strcmp(g_bridge_state.interfaces[i].interface_name, interface_name) == 0) {
            return &g_bridge_state.interfaces[i];
        }
    }
    return NULL;
}

// Make a call from ASTC to native module
int astc_native_call(const char* interface_name, const ASTCValue* args, int arg_count, ASTCValue* result) {
    if (!interface_name || !args || !result) {
        SET_ERROR(ERROR_INVALID_ARGUMENT, "Invalid arguments to astc_native_call");
        return -1;
    }

    NativeInterface* iface = find_native_interface(interface_name);
    if (!iface) {
        SET_ERROR(ERROR_SYMBOL_NOT_FOUND, "Native interface not found: %s", interface_name);
        return -1;
    }

    // Validate argument count
    if (arg_count != iface->signature.param_count) {
        SET_ERROR(ERROR_INVALID_ARGUMENT, "Argument count mismatch for %s: expected %d, got %d",
                 interface_name, iface->signature.param_count, arg_count);
        return -1;
    }

    LOG_RUNTIME_DEBUG("Making ASTC->Native call: %s", interface_name);

    // Prepare module call context
    ModuleCallContext ctx;
    MODULE_CALL_INIT(ctx);

    // Convert ASTC arguments to native arguments
    for (int i = 0; i < arg_count; i++) {
        if (astc_value_to_native_arg(&args[i], &ctx.args[i], iface->signature.param_types[i]) != 0) {
            LOG_RUNTIME_ERROR("Failed to convert argument %d for %s", i, interface_name);
            return -1;
        }
        ctx.arg_count++;
    }

    // Make the call using module communication system
    if (module_comm_call_sync(interface_name, &ctx) != 0) {
        LOG_RUNTIME_ERROR("Native call failed: %s", interface_name);
        return -1;
    }

    // Convert return value
    if (native_return_to_astc_value(&ctx.return_value, result, iface->signature.return_type) != 0) {
        LOG_RUNTIME_ERROR("Failed to convert return value for %s", interface_name);
        return -1;
    }

    LOG_RUNTIME_DEBUG("ASTC->Native call completed: %s", interface_name);
    return 0;
}

// Register standard library interfaces
int astc_native_register_stdlib(void) {
    LOG_RUNTIME_INFO("Registering standard library interfaces");

    // Register libc functions
    ASTCCallSignature sig;

    // printf function
    sig.param_count = 1;
    sig.param_types[0] = ASTC_TYPE_STRING;
    sig.return_type = ASTC_TYPE_I32;
    strncpy(sig.description, "Print formatted string", sizeof(sig.description) - 1);

    if (astc_native_register_interface("libc.printf", "libc_x64_64.native", "printf", &sig) != 0) {
        LOG_RUNTIME_WARN("Failed to register libc.printf");
    }

    // malloc function
    sig.param_count = 1;
    sig.param_types[0] = ASTC_TYPE_I64;
    sig.return_type = ASTC_TYPE_PTR;
    strncpy(sig.description, "Allocate memory", sizeof(sig.description) - 1);

    if (astc_native_register_interface("libc.malloc", "libc_x64_64.native", "malloc", &sig) != 0) {
        LOG_RUNTIME_WARN("Failed to register libc.malloc");
    }

    // free function
    sig.param_count = 1;
    sig.param_types[0] = ASTC_TYPE_PTR;
    sig.return_type = ASTC_TYPE_VOID;
    strncpy(sig.description, "Free memory", sizeof(sig.description) - 1);

    if (astc_native_register_interface("libc.free", "libc_x64_64.native", "free", &sig) != 0) {
        LOG_RUNTIME_WARN("Failed to register libc.free");
    }

    // strlen function
    sig.param_count = 1;
    sig.param_types[0] = ASTC_TYPE_STRING;
    sig.return_type = ASTC_TYPE_I64;
    strncpy(sig.description, "Get string length", sizeof(sig.description) - 1);

    if (astc_native_register_interface("libc.strlen", "libc_x64_64.native", "strlen", &sig) != 0) {
        LOG_RUNTIME_WARN("Failed to register libc.strlen");
    }

    LOG_RUNTIME_INFO("Standard library interfaces registered");
    return 0;
}

// List all registered native interfaces
void astc_native_list_interfaces(void) {
    LOG_RUNTIME_INFO("Registered native interfaces (%d):", g_bridge_state.interface_count);
    for (int i = 0; i < g_bridge_state.interface_count; i++) {
        if (g_bridge_state.interfaces[i].is_active) {
            NativeInterface* iface = &g_bridge_state.interfaces[i];
            LOG_RUNTIME_INFO("  %s -> %s.%s (params: %d, return: %d)",
                           iface->interface_name, iface->module_name, iface->native_symbol,
                           iface->signature.param_count, iface->signature.return_type);
        }
    }
}

// Get interface information
int astc_native_get_interface_info(const char* interface_name, ASTCNativeInterfaceInfo* info) {
    if (!interface_name || !info) {
        return -1;
    }

    NativeInterface* iface = find_native_interface(interface_name);
    if (!iface) {
        return -1;
    }

    strncpy(info->interface_name, iface->interface_name, sizeof(info->interface_name) - 1);
    strncpy(info->module_name, iface->module_name, sizeof(info->module_name) - 1);
    strncpy(info->native_symbol, iface->native_symbol, sizeof(info->native_symbol) - 1);
    info->signature = iface->signature;
    info->is_active = iface->is_active;

    return 0;
}
