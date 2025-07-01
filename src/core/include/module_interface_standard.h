/**
 * module_interface_standard.h - Standardized Module Interface System
 * 
 * Header for standard module interfaces including function signatures,
 * data types, and error handling
 */

#ifndef MODULE_INTERFACE_STANDARD_H
#define MODULE_INTERFACE_STANDARD_H

#include "core_astc.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Standard module interface version
#define MODULE_INTERFACE_VERSION_MAJOR 1
#define MODULE_INTERFACE_VERSION_MINOR 0
#define MODULE_INTERFACE_VERSION_PATCH 0

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

// Core interface functions

/**
 * Initialize module interface standard system
 * @return ASTC_IFACE_SUCCESS on success, error code on failure
 */
int module_interface_standard_init(void);

/**
 * Cleanup module interface standard system
 */
void module_interface_standard_cleanup(void);

/**
 * Load standard interfaces
 * @return ASTC_IFACE_SUCCESS on success, error code on failure
 */
int load_standard_interfaces(void);

// Interface registration functions

/**
 * Register memory management interface
 * @return ASTC_IFACE_SUCCESS on success, error code on failure
 */
int register_memory_management_interface(void);

/**
 * Register I/O interface
 * @return ASTC_IFACE_SUCCESS on success, error code on failure
 */
int register_io_interface(void);

/**
 * Register string interface
 * @return ASTC_IFACE_SUCCESS on success, error code on failure
 */
int register_string_interface(void);

/**
 * Register math interface
 * @return ASTC_IFACE_SUCCESS on success, error code on failure
 */
int register_math_interface(void);

/**
 * Register system interface
 * @return ASTC_IFACE_SUCCESS on success, error code on failure
 */
int register_system_interface(void);

/**
 * Register custom interface
 * @param interface Interface definition to register
 * @return ASTC_IFACE_SUCCESS on success, error code on failure
 */
int register_custom_interface(const ASTCInterfaceDefinition* interface);

// Interface lookup functions

/**
 * Find interface by name
 * @param interface_name Name of interface to find
 * @return Pointer to interface definition, NULL if not found
 */
ASTCInterfaceDefinition* find_interface(const char* interface_name);

/**
 * Find function signature
 * @param function_name Name of function
 * @param module_name Name of module (can be NULL)
 * @return Pointer to function signature, NULL if not found
 */
ASTCInterfaceSignature* find_function_signature(const char* function_name, const char* module_name);

/**
 * Get interface by ID
 * @param interface_id Interface ID to find
 * @return Pointer to interface definition, NULL if not found
 */
ASTCInterfaceDefinition* get_interface_by_id(const char* interface_id);

// Validation functions

/**
 * Validate function call
 * @param function_name Name of function
 * @param module_name Name of module
 * @param arguments Function arguments
 * @param argument_count Number of arguments
 * @return ASTC_IFACE_SUCCESS if valid, error code otherwise
 */
int validate_function_call(const char* function_name, const char* module_name, 
                          const ASTCValue* arguments, int argument_count);

/**
 * Validate parameter type
 * @param value Parameter value
 * @param param Parameter specification
 * @return true if valid, false otherwise
 */
bool validate_parameter_type(const ASTCValue* value, const ASTCInterfaceParameter* param);

/**
 * Validate interface definition
 * @param interface Interface definition to validate
 * @return true if valid, false otherwise
 */
bool validate_interface_definition(const ASTCInterfaceDefinition* interface);

/**
 * Check interface compatibility
 * @param interface1 First interface
 * @param interface2 Second interface
 * @return true if compatible, false otherwise
 */
bool check_interface_compatibility(const ASTCInterfaceDefinition* interface1, 
                                  const ASTCInterfaceDefinition* interface2);

// Type conversion functions

/**
 * Convert ASTC type to interface type
 * @param astc_type ASTC type
 * @return Corresponding interface type
 */
ASTCInterfaceDataType astc_type_to_interface_type(ASTCValueType astc_type);

/**
 * Convert interface type to ASTC type
 * @param interface_type Interface type
 * @return Corresponding ASTC type
 */
ASTCValueType interface_type_to_astc_type(ASTCInterfaceDataType interface_type);

/**
 * Get type size
 * @param type Interface data type
 * @return Size in bytes, 0 if variable size
 */
size_t get_interface_type_size(ASTCInterfaceDataType type);

/**
 * Check type compatibility
 * @param type1 First type
 * @param type2 Second type
 * @return true if compatible, false otherwise
 */
bool check_type_compatibility(ASTCInterfaceDataType type1, ASTCInterfaceDataType type2);

// Information and statistics

/**
 * Get interface statistics
 * @param interface_calls Pointer to store interface call count
 * @param interface_errors Pointer to store interface error count
 * @param type_conversions Pointer to store type conversion count
 */
void get_interface_statistics(uint64_t* interface_calls, uint64_t* interface_errors, uint64_t* type_conversions);

/**
 * List all interfaces
 */
void list_all_interfaces(void);

/**
 * Get interface count
 * @return Number of registered interfaces
 */
int get_interface_count(void);

/**
 * Get function count for interface
 * @param interface_name Name of interface
 * @return Number of functions in interface, -1 if not found
 */
int get_function_count_for_interface(const char* interface_name);

// Utility functions

/**
 * Convert data type to string
 * @param type Interface data type
 * @return String representation
 */
const char* interface_data_type_to_string(ASTCInterfaceDataType type);

/**
 * Convert error code to string
 * @param error Error code
 * @return String representation
 */
const char* interface_error_to_string(ASTCInterfaceErrorCode error);

/**
 * Create parameter specification
 * @param name Parameter name
 * @param type Parameter type
 * @param is_input Is input parameter
 * @param is_output Is output parameter
 * @param is_optional Is optional parameter
 * @param description Parameter description
 * @param param Pointer to store created parameter
 * @return ASTC_IFACE_SUCCESS on success, error code on failure
 */
int create_parameter_spec(const char* name, ASTCInterfaceDataType type, 
                         bool is_input, bool is_output, bool is_optional,
                         const char* description, ASTCInterfaceParameter* param);

/**
 * Create function signature
 * @param function_name Function name
 * @param module_name Module name
 * @param return_type Return type
 * @param parameters Array of parameters
 * @param parameter_count Number of parameters
 * @param description Function description
 * @param signature Pointer to store created signature
 * @return ASTC_IFACE_SUCCESS on success, error code on failure
 */
int create_function_signature(const char* function_name, const char* module_name,
                             ASTCInterfaceDataType return_type,
                             const ASTCInterfaceParameter* parameters, int parameter_count,
                             const char* description, ASTCInterfaceSignature* signature);

/**
 * Dump interface definition to string
 * @param interface Interface to dump
 * @param buffer Buffer to store string
 * @param buffer_size Size of buffer
 * @return ASTC_IFACE_SUCCESS on success, error code on failure
 */
int dump_interface_to_string(const ASTCInterfaceDefinition* interface, char* buffer, size_t buffer_size);

/**
 * Load interface from string
 * @param interface_string String representation of interface
 * @param interface Pointer to store loaded interface
 * @return ASTC_IFACE_SUCCESS on success, error code on failure
 */
int load_interface_from_string(const char* interface_string, ASTCInterfaceDefinition* interface);

// Standard interface IDs
#define ASTC_STD_INTERFACE_MEMORY    "astc.std.memory"
#define ASTC_STD_INTERFACE_IO        "astc.std.io"
#define ASTC_STD_INTERFACE_STRING    "astc.std.string"
#define ASTC_STD_INTERFACE_MATH      "astc.std.math"
#define ASTC_STD_INTERFACE_SYSTEM    "astc.std.system"

// Interface flags
#define ASTC_IFACE_FLAG_THREAD_SAFE  0x00000001
#define ASTC_IFACE_FLAG_REENTRANT     0x00000002
#define ASTC_IFACE_FLAG_DEPRECATED    0x00000004
#define ASTC_IFACE_FLAG_EXPERIMENTAL  0x00000008

#ifdef __cplusplus
}
#endif

#endif // MODULE_INTERFACE_STANDARD_H
