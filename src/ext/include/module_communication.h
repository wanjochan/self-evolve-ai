/**
 * module_communication.h - Inter-module Communication System
 * 
 * Header for high-performance communication between .native modules
 */

#ifndef MODULE_COMMUNICATION_H
#define MODULE_COMMUNICATION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Maximum number of arguments in a module call
#define MODULE_MAX_ARGS 8

// Argument types for module calls
typedef enum {
    MODULE_ARG_INT32 = 1,
    MODULE_ARG_INT64 = 2,
    MODULE_ARG_UINT32 = 3,
    MODULE_ARG_UINT64 = 4,
    MODULE_ARG_FLOAT = 5,
    MODULE_ARG_DOUBLE = 6,
    MODULE_ARG_POINTER = 7,
    MODULE_ARG_STRING = 8
} ModuleArgType;

// Return types for module calls
typedef enum {
    MODULE_RETURN_VOID = 0,
    MODULE_RETURN_INT32 = 1,
    MODULE_RETURN_INT64 = 2,
    MODULE_RETURN_UINT32 = 3,
    MODULE_RETURN_UINT64 = 4,
    MODULE_RETURN_FLOAT = 5,
    MODULE_RETURN_DOUBLE = 6,
    MODULE_RETURN_POINTER = 7
} ModuleReturnType;

// Call status
typedef enum {
    MODULE_CALL_PENDING = 0,
    MODULE_CALL_SUCCESS = 1,
    MODULE_CALL_ERROR = 2,
    MODULE_CALL_TIMEOUT = 3,
    MODULE_CALL_NOT_FOUND = 4
} ModuleCallStatus;

// Argument value union
typedef union {
    int32_t int32_val;
    int64_t int64_val;
    uint32_t uint32_val;
    uint64_t uint64_val;
    float float_val;
    double double_val;
    void* ptr_val;
    const char* str_val;
} ModuleCallArg;

// Return value union
typedef union {
    int32_t int_val;
    int64_t long_val;
    uint32_t uint_val;
    uint64_t ulong_val;
    float float_val;
    double double_val;
    void* ptr_val;
} ModuleCallReturn;

// Function signature for module interfaces
typedef struct {
    ModuleArgType arg_types[MODULE_MAX_ARGS];
    int arg_count;
    ModuleReturnType return_type;
    char description[256];
} ModuleCallSignature;

// Call context for module communication
typedef struct {
    ModuleCallArg args[MODULE_MAX_ARGS];
    int arg_count;
    ModuleCallReturn return_value;
    ModuleCallStatus status;
    uint64_t timestamp;
    char error_message[256];
} ModuleCallContext;

// Interface information
typedef struct {
    char name[128];
    char module_name[128];
    ModuleCallSignature signature;
    bool is_active;
} ModuleInterfaceInfo;

// Module communication functions

/**
 * Initialize the module communication system
 * @return 0 on success, -1 on error
 */
int module_comm_init(void);

/**
 * Cleanup the module communication system
 */
void module_comm_cleanup(void);

/**
 * Register an interface for inter-module calls
 * @param interface_name Name of the interface
 * @param module_name Name of the module providing the interface
 * @param function_ptr Pointer to the function implementing the interface
 * @param signature Function signature description
 * @return 0 on success, -1 on error
 */
int module_comm_register_interface(const char* interface_name, const char* module_name,
                                  void* function_ptr, const ModuleCallSignature* signature);

/**
 * Unregister an interface
 * @param interface_name Name of the interface to unregister
 * @return 0 on success, -1 on error
 */
int module_comm_unregister_interface(const char* interface_name);

/**
 * Make a synchronous call to another module
 * @param interface_name Name of the interface to call
 * @param context Call context with arguments and return value
 * @return 0 on success, -1 on error
 */
int module_comm_call_sync(const char* interface_name, ModuleCallContext* context);

/**
 * Make an asynchronous call to another module
 * @param interface_name Name of the interface to call
 * @param context Call context with arguments
 * @return Call ID for tracking, 0 on error
 */
uint32_t module_comm_call_async(const char* interface_name, ModuleCallContext* context);

/**
 * Check status of an asynchronous call
 * @param call_id Call ID returned by module_comm_call_async
 * @param result Optional pointer to receive result
 * @return Call status
 */
ModuleCallStatus module_comm_check_async(uint32_t call_id, ModuleCallContext* result);

/**
 * List all registered interfaces (for debugging)
 */
void module_comm_list_interfaces(void);

/**
 * Get information about a specific interface
 * @param interface_name Name of the interface
 * @param info Pointer to structure to fill with interface information
 * @return 0 on success, -1 on error
 */
int module_comm_get_interface_info(const char* interface_name, ModuleInterfaceInfo* info);

// Convenience macros for creating call contexts

#define MODULE_CALL_INIT(ctx) \
    do { \
        memset(&(ctx), 0, sizeof(ctx)); \
        (ctx).timestamp = (uint64_t)time(NULL); \
    } while(0)

#define MODULE_CALL_ADD_ARG_INT32(ctx, val) \
    do { \
        if ((ctx).arg_count < MODULE_MAX_ARGS) { \
            (ctx).args[(ctx).arg_count].int32_val = (val); \
            (ctx).arg_count++; \
        } \
    } while(0)

#define MODULE_CALL_ADD_ARG_INT64(ctx, val) \
    do { \
        if ((ctx).arg_count < MODULE_MAX_ARGS) { \
            (ctx).args[(ctx).arg_count].int64_val = (val); \
            (ctx).arg_count++; \
        } \
    } while(0)

#define MODULE_CALL_ADD_ARG_PTR(ctx, val) \
    do { \
        if ((ctx).arg_count < MODULE_MAX_ARGS) { \
            (ctx).args[(ctx).arg_count].ptr_val = (val); \
            (ctx).arg_count++; \
        } \
    } while(0)

#define MODULE_CALL_ADD_ARG_STR(ctx, val) \
    do { \
        if ((ctx).arg_count < MODULE_MAX_ARGS) { \
            (ctx).args[(ctx).arg_count].str_val = (val); \
            (ctx).arg_count++; \
        } \
    } while(0)

// Signature creation macros

#define MODULE_SIG_INIT(sig, desc) \
    do { \
        memset(&(sig), 0, sizeof(sig)); \
        strncpy((sig).description, (desc), sizeof((sig).description) - 1); \
    } while(0)

#define MODULE_SIG_ADD_ARG(sig, type) \
    do { \
        if ((sig).arg_count < MODULE_MAX_ARGS) { \
            (sig).arg_types[(sig).arg_count] = (type); \
            (sig).arg_count++; \
        } \
    } while(0)

#define MODULE_SIG_SET_RETURN(sig, type) \
    do { \
        (sig).return_type = (type); \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif // MODULE_COMMUNICATION_H
