/**
 * astc_native_bridge.h - ASTC to Native Module Bridge
 * 
 * Header for the bridge between ASTC bytecode programs and .native modules
 */

#ifndef ASTC_NATIVE_BRIDGE_H
#define ASTC_NATIVE_BRIDGE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Maximum number of parameters in a call
#define ASTC_MAX_CALL_PARAMS 16

// ASTC data types for bridge operations
typedef enum {
    ASTC_TYPE_VOID = 0,
    ASTC_TYPE_I32 = 1,
    ASTC_TYPE_I64 = 2,
    ASTC_TYPE_F32 = 3,
    ASTC_TYPE_F64 = 4,
    ASTC_TYPE_PTR = 5,
    ASTC_TYPE_STRING = 6
} ASTCDataType;

// ASTC value container
typedef struct {
    ASTCDataType type;
    union {
        int32_t i32;
        int64_t i64;
        float f32;
        double f64;
        void* ptr;
        const char* str;
    } data;
} ASTCValue;

// Call signature for ASTC-Native interface
typedef struct {
    ASTCDataType param_types[ASTC_MAX_CALL_PARAMS];
    int param_count;
    ASTCDataType return_type;
    char description[256];
} ASTCCallSignature;

// Interface information
typedef struct {
    char interface_name[128];
    char module_name[128];
    char native_symbol[128];
    ASTCCallSignature signature;
    bool is_active;
} ASTCNativeInterfaceInfo;

// Bridge functions

/**
 * Initialize the ASTC-Native bridge
 * @return 0 on success, -1 on error
 */
int astc_native_bridge_init(void);

/**
 * Cleanup the ASTC-Native bridge
 */
void astc_native_bridge_cleanup(void);

/**
 * Register a native interface for ASTC access
 * @param interface_name Name of the interface (e.g., "libc.printf")
 * @param module_name Name of the native module
 * @param native_symbol Symbol name in the native module
 * @param signature Function signature
 * @return 0 on success, -1 on error
 */
int astc_native_register_interface(const char* interface_name, const char* module_name,
                                  const char* native_symbol, const ASTCCallSignature* signature);

/**
 * Make a call from ASTC to native module
 * @param interface_name Name of the interface to call
 * @param args Array of arguments
 * @param arg_count Number of arguments
 * @param result Pointer to store the result
 * @return 0 on success, -1 on error
 */
int astc_native_call(const char* interface_name, const ASTCValue* args, int arg_count, ASTCValue* result);

/**
 * Register standard library interfaces
 * @return 0 on success, -1 on error
 */
int astc_native_register_stdlib(void);

/**
 * List all registered native interfaces (for debugging)
 */
void astc_native_list_interfaces(void);

/**
 * Get information about a specific interface
 * @param interface_name Name of the interface
 * @param info Pointer to structure to fill with interface information
 * @return 0 on success, -1 on error
 */
int astc_native_get_interface_info(const char* interface_name, ASTCNativeInterfaceInfo* info);

// Convenience macros for creating ASTC values

#define ASTC_VALUE_I32(val) ((ASTCValue){.type = ASTC_TYPE_I32, .data.i32 = (val)})
#define ASTC_VALUE_I64(val) ((ASTCValue){.type = ASTC_TYPE_I64, .data.i64 = (val)})
#define ASTC_VALUE_F32(val) ((ASTCValue){.type = ASTC_TYPE_F32, .data.f32 = (val)})
#define ASTC_VALUE_F64(val) ((ASTCValue){.type = ASTC_TYPE_F64, .data.f64 = (val)})
#define ASTC_VALUE_PTR(val) ((ASTCValue){.type = ASTC_TYPE_PTR, .data.ptr = (val)})
#define ASTC_VALUE_STR(val) ((ASTCValue){.type = ASTC_TYPE_STRING, .data.str = (val)})
#define ASTC_VALUE_VOID() ((ASTCValue){.type = ASTC_TYPE_VOID})

// Convenience macros for creating call signatures

#define ASTC_SIG_INIT(sig, desc) \
    do { \
        memset(&(sig), 0, sizeof(sig)); \
        strncpy((sig).description, (desc), sizeof((sig).description) - 1); \
    } while(0)

#define ASTC_SIG_ADD_PARAM(sig, type) \
    do { \
        if ((sig).param_count < ASTC_MAX_CALL_PARAMS) { \
            (sig).param_types[(sig).param_count] = (type); \
            (sig).param_count++; \
        } \
    } while(0)

#define ASTC_SIG_SET_RETURN(sig, type) \
    do { \
        (sig).return_type = (type); \
    } while(0)

// Error codes specific to ASTC-Native bridge
#define ASTC_BRIDGE_SUCCESS           0
#define ASTC_BRIDGE_ERROR_INVALID     -1
#define ASTC_BRIDGE_ERROR_NOT_FOUND   -2
#define ASTC_BRIDGE_ERROR_TYPE_MISMATCH -3
#define ASTC_BRIDGE_ERROR_CALL_FAILED -4
#define ASTC_BRIDGE_ERROR_NO_MEMORY   -5

// Standard library interface names (for convenience)
#define ASTC_STDLIB_PRINTF    "libc.printf"
#define ASTC_STDLIB_MALLOC    "libc.malloc"
#define ASTC_STDLIB_FREE      "libc.free"
#define ASTC_STDLIB_STRLEN    "libc.strlen"
#define ASTC_STDLIB_STRCPY    "libc.strcpy"
#define ASTC_STDLIB_STRCMP    "libc.strcmp"
#define ASTC_STDLIB_MEMCPY    "libc.memcpy"
#define ASTC_STDLIB_MEMSET    "libc.memset"

// VM integration functions (to be implemented by VM)

/**
 * Called by VM when ASTC code makes a native call
 * This function should be implemented by the ASTC VM
 * @param interface_name Name of the interface to call
 * @param stack_args Arguments from ASTC stack
 * @param arg_count Number of arguments
 * @param result Pointer to store result on ASTC stack
 * @return 0 on success, -1 on error
 */
int astc_vm_native_call_handler(const char* interface_name, 
                               const ASTCValue* stack_args, int arg_count, 
                               ASTCValue* result);

/**
 * Register VM callback for native calls
 * @param callback Callback function to handle native calls from VM
 * @return 0 on success, -1 on error
 */
int astc_native_register_vm_callback(int (*callback)(const char*, const ASTCValue*, int, ASTCValue*));

#ifdef __cplusplus
}
#endif

#endif // ASTC_NATIVE_BRIDGE_H
