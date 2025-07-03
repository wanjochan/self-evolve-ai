/**
 * module_interface_standard.c - Standardized Module Interface System
 * 
 * Defines and implements standard module interfaces including function signatures,
 * data types, error handling, and inter-module communication protocols.
 */

#include "../include/native_format.h"
#include "../include/logger.h"
#include "../include/module_communication.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// Standard module interface version
#define MODULE_INTERFACE_VERSION_MAJOR 1
#define MODULE_INTERFACE_VERSION_MINOR 0
#define MODULE_INTERFACE_VERSION_PATCH 0

// Maximum number of interface definitions
#define MAX_INTERFACE_DEFINITIONS 512

// Standard data type identifiers
typedef enum {
    ASTC_IFACE_TYPE_VOID = 0,
    ASTC_IFACE_TYPE_BOOL = 1,
    ASTC_IFACE_TYPE_I8 = 2,
    ASTC_IFACE_TYPE_U8 = 3,
    ASTC_IFACE_TYPE_I16 = 4,
    ASTC_IFACE_TYPE_U16 = 5,
    ASTC_IFACE_TYPE_I32 = 6,
    ASTC_IFACE_TYPE_U32 = 7,
    ASTC_IFACE_TYPE_I64 = 8,
    ASTC_IFACE_TYPE_U64 = 9,
    ASTC_IFACE_TYPE_F32 = 10,
    ASTC_IFACE_TYPE_F64 = 11,
    ASTC_IFACE_TYPE_PTR = 12,
    ASTC_IFACE_TYPE_STRING = 13,
    ASTC_IFACE_TYPE_BUFFER = 14,
    ASTC_IFACE_TYPE_STRUCT = 15,
    ASTC_IFACE_TYPE_ARRAY = 16,
    ASTC_IFACE_TYPE_FUNCTION = 17,
    ASTC_IFACE_TYPE_HANDLE = 18
} ASTCInterfaceDataType;

// Standard error codes
typedef enum {
    ASTC_IFACE_SUCCESS = 0,
    ASTC_IFACE_ERROR_INVALID_PARAM = -1,
    ASTC_IFACE_ERROR_NULL_POINTER = -2,
    ASTC_IFACE_ERROR_BUFFER_TOO_SMALL = -3,
    ASTC_IFACE_ERROR_OUT_OF_MEMORY = -4,
    ASTC_IFACE_ERROR_NOT_IMPLEMENTED = -5,
    ASTC_IFACE_ERROR_ACCESS_DENIED = -6,
    ASTC_IFACE_ERROR_TIMEOUT = -7,
    ASTC_IFACE_ERROR_BUSY = -8,
    ASTC_IFACE_ERROR_NOT_FOUND = -9,
    ASTC_IFACE_ERROR_ALREADY_EXISTS = -10,
    ASTC_IFACE_ERROR_INCOMPATIBLE = -11,
    ASTC_IFACE_ERROR_INTERNAL = -12
} ASTCInterfaceErrorCode;

// Parameter specification
typedef struct {
    char name[64];
    ASTCInterfaceDataType type;
    bool is_input;
    bool is_output;
    bool is_optional;
    size_t size;
    char description[128];
} ASTCInterfaceParameter;

// Function signature specification
typedef struct {
    char function_name[128];
    char module_name[128];
    ASTCInterfaceDataType return_type;
    ASTCInterfaceParameter parameters[16];
    int parameter_count;
    char description[256];
    uint32_t interface_version;
    uint32_t flags;
} ASTCInterfaceSignature;

// Interface definition
typedef struct {
    char interface_name[128];
    char interface_id[64];
    ASTCInterfaceSignature signatures[32];
    int signature_count;
    uint32_t version_major;
    uint32_t version_minor;
    uint32_t version_patch;
    char description[256];
    bool is_standard;
} ASTCInterfaceDefinition;

// Interface registry
static struct {
    ASTCInterfaceDefinition interfaces[MAX_INTERFACE_DEFINITIONS];
    int interface_count;
    bool initialized;
    
    // Standard interfaces
    bool standard_interfaces_loaded;
    
    // Statistics
    uint64_t interface_calls;
    uint64_t interface_errors;
    uint64_t type_conversions;
} g_interface_registry = {0};

// Initialize module interface standard system
int module_interface_standard_init(void) {
    if (g_interface_registry.initialized) {
        return ASTC_IFACE_SUCCESS;
    }
    
    memset(&g_interface_registry, 0, sizeof(g_interface_registry));
    
    // Load standard interfaces
    if (load_standard_interfaces() != ASTC_IFACE_SUCCESS) {
        LOG_MODULE_ERROR("Failed to load standard interfaces");
        return ASTC_IFACE_ERROR_INTERNAL;
    }
    
    g_interface_registry.initialized = true;
    g_interface_registry.standard_interfaces_loaded = true;
    
    LOG_MODULE_INFO("Module interface standard system initialized");
    LOG_MODULE_INFO("Interface version: %d.%d.%d", 
                   MODULE_INTERFACE_VERSION_MAJOR,
                   MODULE_INTERFACE_VERSION_MINOR,
                   MODULE_INTERFACE_VERSION_PATCH);
    
    return ASTC_IFACE_SUCCESS;
}

// Cleanup module interface standard system
void module_interface_standard_cleanup(void) {
    if (!g_interface_registry.initialized) {
        return;
    }
    
    LOG_MODULE_INFO("Interface statistics:");
    LOG_MODULE_INFO("  Interface calls: %llu", g_interface_registry.interface_calls);
    LOG_MODULE_INFO("  Interface errors: %llu", g_interface_registry.interface_errors);
    LOG_MODULE_INFO("  Type conversions: %llu", g_interface_registry.type_conversions);
    
    g_interface_registry.initialized = false;
}

// Load standard interfaces
int load_standard_interfaces(void) {
    // Standard Memory Management Interface
    if (register_memory_management_interface() != ASTC_IFACE_SUCCESS) {
        return ASTC_IFACE_ERROR_INTERNAL;
    }
    
    // Standard I/O Interface
    if (register_io_interface() != ASTC_IFACE_SUCCESS) {
        return ASTC_IFACE_ERROR_INTERNAL;
    }
    
    // Standard String Interface
    if (register_string_interface() != ASTC_IFACE_SUCCESS) {
        return ASTC_IFACE_ERROR_INTERNAL;
    }
    
    // Standard Math Interface
    if (register_math_interface() != ASTC_IFACE_SUCCESS) {
        return ASTC_IFACE_ERROR_INTERNAL;
    }
    
    // Standard System Interface
    if (register_system_interface() != ASTC_IFACE_SUCCESS) {
        return ASTC_IFACE_ERROR_INTERNAL;
    }
    
    LOG_MODULE_INFO("Standard interfaces loaded successfully");
    return ASTC_IFACE_SUCCESS;
}

// Register memory management interface
int register_memory_management_interface(void) {
    ASTCInterfaceDefinition* iface = &g_interface_registry.interfaces[g_interface_registry.interface_count];
    memset(iface, 0, sizeof(ASTCInterfaceDefinition));
    
    strcpy(iface->interface_name, "MemoryManagement");
    strcpy(iface->interface_id, "astc.std.memory");
    strcpy(iface->description, "Standard memory management interface");
    iface->version_major = 1;
    iface->version_minor = 0;
    iface->version_patch = 0;
    iface->is_standard = true;
    
    // malloc function
    ASTCInterfaceSignature* sig = &iface->signatures[iface->signature_count++];
    strcpy(sig->function_name, "malloc");
    strcpy(sig->module_name, "libc");
    strcpy(sig->description, "Allocate memory");
    sig->return_type = ASTC_IFACE_TYPE_PTR;
    sig->parameter_count = 1;
    
    strcpy(sig->parameters[0].name, "size");
    sig->parameters[0].type = ASTC_IFACE_TYPE_U64;
    sig->parameters[0].is_input = true;
    sig->parameters[0].is_output = false;
    sig->parameters[0].is_optional = false;
    strcpy(sig->parameters[0].description, "Size in bytes to allocate");
    
    // free function
    sig = &iface->signatures[iface->signature_count++];
    strcpy(sig->function_name, "free");
    strcpy(sig->module_name, "libc");
    strcpy(sig->description, "Free allocated memory");
    sig->return_type = ASTC_IFACE_TYPE_VOID;
    sig->parameter_count = 1;
    
    strcpy(sig->parameters[0].name, "ptr");
    sig->parameters[0].type = ASTC_IFACE_TYPE_PTR;
    sig->parameters[0].is_input = true;
    sig->parameters[0].is_output = false;
    sig->parameters[0].is_optional = false;
    strcpy(sig->parameters[0].description, "Pointer to memory to free");
    
    // realloc function
    sig = &iface->signatures[iface->signature_count++];
    strcpy(sig->function_name, "realloc");
    strcpy(sig->module_name, "libc");
    strcpy(sig->description, "Reallocate memory");
    sig->return_type = ASTC_IFACE_TYPE_PTR;
    sig->parameter_count = 2;
    
    strcpy(sig->parameters[0].name, "ptr");
    sig->parameters[0].type = ASTC_IFACE_TYPE_PTR;
    sig->parameters[0].is_input = true;
    sig->parameters[0].is_output = false;
    sig->parameters[0].is_optional = true;
    strcpy(sig->parameters[0].description, "Pointer to existing memory");
    
    strcpy(sig->parameters[1].name, "size");
    sig->parameters[1].type = ASTC_IFACE_TYPE_U64;
    sig->parameters[1].is_input = true;
    sig->parameters[1].is_output = false;
    sig->parameters[1].is_optional = false;
    strcpy(sig->parameters[1].description, "New size in bytes");
    
    g_interface_registry.interface_count++;
    return ASTC_IFACE_SUCCESS;
}

// Register I/O interface
int register_io_interface(void) {
    ASTCInterfaceDefinition* iface = &g_interface_registry.interfaces[g_interface_registry.interface_count];
    memset(iface, 0, sizeof(ASTCInterfaceDefinition));
    
    strcpy(iface->interface_name, "InputOutput");
    strcpy(iface->interface_id, "astc.std.io");
    strcpy(iface->description, "Standard input/output interface");
    iface->version_major = 1;
    iface->version_minor = 0;
    iface->version_patch = 0;
    iface->is_standard = true;
    
    // printf function
    ASTCInterfaceSignature* sig = &iface->signatures[iface->signature_count++];
    strcpy(sig->function_name, "printf");
    strcpy(sig->module_name, "libc");
    strcpy(sig->description, "Print formatted string");
    sig->return_type = ASTC_IFACE_TYPE_I32;
    sig->parameter_count = 1; // Simplified - variadic not fully supported
    
    strcpy(sig->parameters[0].name, "format");
    sig->parameters[0].type = ASTC_IFACE_TYPE_STRING;
    sig->parameters[0].is_input = true;
    sig->parameters[0].is_output = false;
    sig->parameters[0].is_optional = false;
    strcpy(sig->parameters[0].description, "Format string");
    
    // fopen function
    sig = &iface->signatures[iface->signature_count++];
    strcpy(sig->function_name, "fopen");
    strcpy(sig->module_name, "libc");
    strcpy(sig->description, "Open file");
    sig->return_type = ASTC_IFACE_TYPE_HANDLE;
    sig->parameter_count = 2;
    
    strcpy(sig->parameters[0].name, "filename");
    sig->parameters[0].type = ASTC_IFACE_TYPE_STRING;
    sig->parameters[0].is_input = true;
    sig->parameters[0].is_output = false;
    sig->parameters[0].is_optional = false;
    strcpy(sig->parameters[0].description, "File name to open");
    
    strcpy(sig->parameters[1].name, "mode");
    sig->parameters[1].type = ASTC_IFACE_TYPE_STRING;
    sig->parameters[1].is_input = true;
    sig->parameters[1].is_output = false;
    sig->parameters[1].is_optional = false;
    strcpy(sig->parameters[1].description, "File open mode");
    
    g_interface_registry.interface_count++;
    return ASTC_IFACE_SUCCESS;
}

// Register string interface
int register_string_interface(void) {
    ASTCInterfaceDefinition* iface = &g_interface_registry.interfaces[g_interface_registry.interface_count];
    memset(iface, 0, sizeof(ASTCInterfaceDefinition));
    
    strcpy(iface->interface_name, "StringOperations");
    strcpy(iface->interface_id, "astc.std.string");
    strcpy(iface->description, "Standard string operations interface");
    iface->version_major = 1;
    iface->version_minor = 0;
    iface->version_patch = 0;
    iface->is_standard = true;
    
    // strlen function
    ASTCInterfaceSignature* sig = &iface->signatures[iface->signature_count++];
    strcpy(sig->function_name, "strlen");
    strcpy(sig->module_name, "libc");
    strcpy(sig->description, "Get string length");
    sig->return_type = ASTC_IFACE_TYPE_U64;
    sig->parameter_count = 1;
    
    strcpy(sig->parameters[0].name, "str");
    sig->parameters[0].type = ASTC_IFACE_TYPE_STRING;
    sig->parameters[0].is_input = true;
    sig->parameters[0].is_output = false;
    sig->parameters[0].is_optional = false;
    strcpy(sig->parameters[0].description, "String to measure");
    
    // strcpy function
    sig = &iface->signatures[iface->signature_count++];
    strcpy(sig->function_name, "strcpy");
    strcpy(sig->module_name, "libc");
    strcpy(sig->description, "Copy string");
    sig->return_type = ASTC_IFACE_TYPE_STRING;
    sig->parameter_count = 2;
    
    strcpy(sig->parameters[0].name, "dest");
    sig->parameters[0].type = ASTC_IFACE_TYPE_STRING;
    sig->parameters[0].is_input = false;
    sig->parameters[0].is_output = true;
    sig->parameters[0].is_optional = false;
    strcpy(sig->parameters[0].description, "Destination buffer");
    
    strcpy(sig->parameters[1].name, "src");
    sig->parameters[1].type = ASTC_IFACE_TYPE_STRING;
    sig->parameters[1].is_input = true;
    sig->parameters[1].is_output = false;
    sig->parameters[1].is_optional = false;
    strcpy(sig->parameters[1].description, "Source string");
    
    g_interface_registry.interface_count++;
    return ASTC_IFACE_SUCCESS;
}

// Register math interface
int register_math_interface(void) {
    ASTCInterfaceDefinition* iface = &g_interface_registry.interfaces[g_interface_registry.interface_count];
    memset(iface, 0, sizeof(ASTCInterfaceDefinition));
    
    strcpy(iface->interface_name, "Mathematics");
    strcpy(iface->interface_id, "astc.std.math");
    strcpy(iface->description, "Standard mathematics interface");
    iface->version_major = 1;
    iface->version_minor = 0;
    iface->version_patch = 0;
    iface->is_standard = true;
    
    // sqrt function
    ASTCInterfaceSignature* sig = &iface->signatures[iface->signature_count++];
    strcpy(sig->function_name, "sqrt");
    strcpy(sig->module_name, "math");
    strcpy(sig->description, "Square root");
    sig->return_type = ASTC_IFACE_TYPE_F64;
    sig->parameter_count = 1;
    
    strcpy(sig->parameters[0].name, "x");
    sig->parameters[0].type = ASTC_IFACE_TYPE_F64;
    sig->parameters[0].is_input = true;
    sig->parameters[0].is_output = false;
    sig->parameters[0].is_optional = false;
    strcpy(sig->parameters[0].description, "Input value");
    
    // pow function
    sig = &iface->signatures[iface->signature_count++];
    strcpy(sig->function_name, "pow");
    strcpy(sig->module_name, "math");
    strcpy(sig->description, "Power function");
    sig->return_type = ASTC_IFACE_TYPE_F64;
    sig->parameter_count = 2;
    
    strcpy(sig->parameters[0].name, "base");
    sig->parameters[0].type = ASTC_IFACE_TYPE_F64;
    sig->parameters[0].is_input = true;
    sig->parameters[0].is_output = false;
    sig->parameters[0].is_optional = false;
    strcpy(sig->parameters[0].description, "Base value");
    
    strcpy(sig->parameters[1].name, "exponent");
    sig->parameters[1].type = ASTC_IFACE_TYPE_F64;
    sig->parameters[1].is_input = true;
    sig->parameters[1].is_output = false;
    sig->parameters[1].is_optional = false;
    strcpy(sig->parameters[1].description, "Exponent value");
    
    g_interface_registry.interface_count++;
    return ASTC_IFACE_SUCCESS;
}

// Register system interface
int register_system_interface(void) {
    ASTCInterfaceDefinition* iface = &g_interface_registry.interfaces[g_interface_registry.interface_count];
    memset(iface, 0, sizeof(ASTCInterfaceDefinition));
    
    strcpy(iface->interface_name, "SystemOperations");
    strcpy(iface->interface_id, "astc.std.system");
    strcpy(iface->description, "Standard system operations interface");
    iface->version_major = 1;
    iface->version_minor = 0;
    iface->version_patch = 0;
    iface->is_standard = true;
    
    // exit function
    ASTCInterfaceSignature* sig = &iface->signatures[iface->signature_count++];
    strcpy(sig->function_name, "exit");
    strcpy(sig->module_name, "libc");
    strcpy(sig->description, "Exit program");
    sig->return_type = ASTC_IFACE_TYPE_VOID;
    sig->parameter_count = 1;
    
    strcpy(sig->parameters[0].name, "status");
    sig->parameters[0].type = ASTC_IFACE_TYPE_I32;
    sig->parameters[0].is_input = true;
    sig->parameters[0].is_output = false;
    sig->parameters[0].is_optional = false;
    strcpy(sig->parameters[0].description, "Exit status code");
    
    g_interface_registry.interface_count++;
    return ASTC_IFACE_SUCCESS;
}

// Find interface by name
ASTCInterfaceDefinition* find_interface(const char* interface_name) {
    if (!interface_name) {
        return NULL;
    }
    
    for (int i = 0; i < g_interface_registry.interface_count; i++) {
        if (strcmp(g_interface_registry.interfaces[i].interface_name, interface_name) == 0) {
            return &g_interface_registry.interfaces[i];
        }
    }
    
    return NULL;
}

// Find function signature
ASTCInterfaceSignature* find_function_signature(const char* function_name, const char* module_name) {
    if (!function_name) {
        return NULL;
    }
    
    for (int i = 0; i < g_interface_registry.interface_count; i++) {
        ASTCInterfaceDefinition* iface = &g_interface_registry.interfaces[i];
        for (int j = 0; j < iface->signature_count; j++) {
            ASTCInterfaceSignature* sig = &iface->signatures[j];
            if (strcmp(sig->function_name, function_name) == 0) {
                if (!module_name || strcmp(sig->module_name, module_name) == 0) {
                    return sig;
                }
            }
        }
    }
    
    return NULL;
}

// Validate function call
int validate_function_call(const char* function_name, const char* module_name, 
                          const ASTCValue* arguments, int argument_count) {
    g_interface_registry.interface_calls++;
    
    ASTCInterfaceSignature* sig = find_function_signature(function_name, module_name);
    if (!sig) {
        LOG_MODULE_WARN("No interface signature found for function: %s.%s", 
                       module_name ? module_name : "unknown", function_name);
        return ASTC_IFACE_ERROR_NOT_FOUND;
    }
    
    // Check parameter count
    if (argument_count != sig->parameter_count) {
        LOG_MODULE_ERROR("Parameter count mismatch for %s: expected %d, got %d",
                        function_name, sig->parameter_count, argument_count);
        g_interface_registry.interface_errors++;
        return ASTC_IFACE_ERROR_INVALID_PARAM;
    }
    
    // Validate parameter types
    for (int i = 0; i < argument_count; i++) {
        if (!validate_parameter_type(&arguments[i], &sig->parameters[i])) {
            LOG_MODULE_ERROR("Parameter type mismatch for %s parameter %d", function_name, i);
            g_interface_registry.interface_errors++;
            return ASTC_IFACE_ERROR_INVALID_PARAM;
        }
    }
    
    return ASTC_IFACE_SUCCESS;
}

// Validate parameter type
bool validate_parameter_type(const ASTCValue* value, const ASTCInterfaceParameter* param) {
    if (!value || !param) {
        return false;
    }
    
    // Type compatibility check
    switch (param->type) {
        case ASTC_IFACE_TYPE_BOOL:
            return value->type == ASTC_TYPE_BOOL;
        case ASTC_IFACE_TYPE_I32:
        case ASTC_IFACE_TYPE_U32:
            return value->type == ASTC_TYPE_I32;
        case ASTC_IFACE_TYPE_I64:
        case ASTC_IFACE_TYPE_U64:
            return value->type == ASTC_TYPE_I64;
        case ASTC_IFACE_TYPE_F32:
            return value->type == ASTC_TYPE_F32;
        case ASTC_IFACE_TYPE_F64:
            return value->type == ASTC_TYPE_F64;
        case ASTC_IFACE_TYPE_STRING:
            return value->type == ASTC_TYPE_STRING;
        case ASTC_IFACE_TYPE_PTR:
        case ASTC_IFACE_TYPE_HANDLE:
            return value->type == ASTC_TYPE_PTR;
        default:
            return false;
    }
}

// Get interface statistics
void get_interface_statistics(uint64_t* interface_calls, uint64_t* interface_errors, uint64_t* type_conversions) {
    if (interface_calls) *interface_calls = g_interface_registry.interface_calls;
    if (interface_errors) *interface_errors = g_interface_registry.interface_errors;
    if (type_conversions) *type_conversions = g_interface_registry.type_conversions;
}

// List all interfaces
void list_all_interfaces(void) {
    LOG_MODULE_INFO("Registered interfaces (%d):", g_interface_registry.interface_count);
    for (int i = 0; i < g_interface_registry.interface_count; i++) {
        ASTCInterfaceDefinition* iface = &g_interface_registry.interfaces[i];
        LOG_MODULE_INFO("  %s (%s) v%d.%d.%d - %d functions",
                       iface->interface_name, iface->interface_id,
                       iface->version_major, iface->version_minor, iface->version_patch,
                       iface->signature_count);
    }
}

// Convert data type to string
const char* interface_data_type_to_string(ASTCInterfaceDataType type) {
    switch (type) {
        case ASTC_IFACE_TYPE_VOID: return "void";
        case ASTC_IFACE_TYPE_BOOL: return "bool";
        case ASTC_IFACE_TYPE_I8: return "i8";
        case ASTC_IFACE_TYPE_U8: return "u8";
        case ASTC_IFACE_TYPE_I16: return "i16";
        case ASTC_IFACE_TYPE_U16: return "u16";
        case ASTC_IFACE_TYPE_I32: return "i32";
        case ASTC_IFACE_TYPE_U32: return "u32";
        case ASTC_IFACE_TYPE_I64: return "i64";
        case ASTC_IFACE_TYPE_U64: return "u64";
        case ASTC_IFACE_TYPE_F32: return "f32";
        case ASTC_IFACE_TYPE_F64: return "f64";
        case ASTC_IFACE_TYPE_PTR: return "ptr";
        case ASTC_IFACE_TYPE_STRING: return "string";
        case ASTC_IFACE_TYPE_BUFFER: return "buffer";
        case ASTC_IFACE_TYPE_STRUCT: return "struct";
        case ASTC_IFACE_TYPE_ARRAY: return "array";
        case ASTC_IFACE_TYPE_FUNCTION: return "function";
        case ASTC_IFACE_TYPE_HANDLE: return "handle";
        default: return "unknown";
    }
}

// Convert error code to string
const char* interface_error_to_string(ASTCInterfaceErrorCode error) {
    switch (error) {
        case ASTC_IFACE_SUCCESS: return "Success";
        case ASTC_IFACE_ERROR_INVALID_PARAM: return "Invalid parameter";
        case ASTC_IFACE_ERROR_NULL_POINTER: return "Null pointer";
        case ASTC_IFACE_ERROR_BUFFER_TOO_SMALL: return "Buffer too small";
        case ASTC_IFACE_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case ASTC_IFACE_ERROR_NOT_IMPLEMENTED: return "Not implemented";
        case ASTC_IFACE_ERROR_ACCESS_DENIED: return "Access denied";
        case ASTC_IFACE_ERROR_TIMEOUT: return "Timeout";
        case ASTC_IFACE_ERROR_BUSY: return "Busy";
        case ASTC_IFACE_ERROR_NOT_FOUND: return "Not found";
        case ASTC_IFACE_ERROR_ALREADY_EXISTS: return "Already exists";
        case ASTC_IFACE_ERROR_INCOMPATIBLE: return "Incompatible";
        case ASTC_IFACE_ERROR_INTERNAL: return "Internal error";
        default: return "Unknown error";
    }
}
