/**
 * astc_program_modules.h - Program-Level Module System for ASTC
 * 
 * Header for program-level module import and usage system
 */

#ifndef ASTC_PROGRAM_MODULES_H
#define ASTC_PROGRAM_MODULES_H

#include "astc_native_bridge.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Maximum number of functions per module
#define ASTC_MAX_MODULE_FUNCTIONS 64

// Module types
typedef enum {
    ASTC_MODULE_SYSTEM = 1,     // System module (e.g., libc.rt, math.rt)
    ASTC_MODULE_USER = 2,       // User-defined module
    ASTC_MODULE_NATIVE = 3      // Native .native module
} ASTCModuleType;

// Function information in a module
typedef struct {
    char name[64];
    ASTCDataType param_types[ASTC_MAX_CALL_PARAMS];
    int param_count;
    ASTCDataType return_type;
    char description[256];
} ASTCFunctionInfo;

// Module interface definition
typedef struct {
    ASTCFunctionInfo functions[ASTC_MAX_MODULE_FUNCTIONS];
    int function_count;
    char description[256];
} ASTCModuleInterface;

// Program module information
typedef struct {
    char module_name[128];
    char version[32];
    char module_path[256];
    ASTCModuleType module_type;
    bool is_system_module;
    int function_count;
    bool is_loaded;
} ASTCProgramModuleInfo;

// Program module system functions

/**
 * Initialize program module system
 * @param program_name Name of the ASTC program
 * @param program_path Path to the ASTC program file
 * @return 0 on success, -1 on error
 */
int astc_program_modules_init(const char* program_name, const char* program_path);

/**
 * Cleanup program module system
 */
void astc_program_modules_cleanup(void);

/**
 * Import a module into the program
 * @param module_name Name of the module to import
 * @param module_path Path to the module file (NULL for system modules)
 * @param version_requirement Version requirement string (optional)
 * @return 0 on success, -1 on error
 */
int astc_program_import_module(const char* module_name, const char* module_path, const char* version_requirement);

/**
 * Unload a module from the program
 * @param module_name Name of the module to unload
 * @return 0 on success, -1 on error
 */
int astc_program_unload_module(const char* module_name);

/**
 * Find a function in imported modules
 * @param module_name Name of the module
 * @param function_name Name of the function
 * @return Function information on success, NULL if not found
 */
const ASTCFunctionInfo* astc_program_find_function(const char* module_name, const char* function_name);

/**
 * Call a function from an imported module
 * @param module_name Name of the module
 * @param function_name Name of the function
 * @param args Array of arguments
 * @param arg_count Number of arguments
 * @param result Pointer to store the result
 * @return 0 on success, -1 on error
 */
int astc_program_call_function(const char* module_name, const char* function_name, 
                              const ASTCValue* args, int arg_count, ASTCValue* result);

/**
 * List all imported modules (for debugging)
 */
void astc_program_list_modules(void);

/**
 * Get information about a specific module
 * @param module_name Name of the module
 * @param info Pointer to structure to fill with module information
 * @return 0 on success, -1 on error
 */
int astc_program_get_module_info(const char* module_name, ASTCProgramModuleInfo* info);

// Convenience macros for common module operations

/**
 * Import libc.rt system module
 */
#define ASTC_IMPORT_LIBC() astc_program_import_module("libc.rt", NULL, NULL)

/**
 * Import math.rt system module
 */
#define ASTC_IMPORT_MATH() astc_program_import_module("math.rt", NULL, NULL)

/**
 * Import io.rt system module
 */
#define ASTC_IMPORT_IO() astc_program_import_module("io.rt", NULL, NULL)

/**
 * Call libc function
 */
#define ASTC_CALL_LIBC(func, args, count, result) \
    astc_program_call_function("libc.rt", func, args, count, result)

/**
 * Call math function
 */
#define ASTC_CALL_MATH(func, args, count, result) \
    astc_program_call_function("math.rt", func, args, count, result)

// Standard system modules
#define ASTC_MODULE_LIBC    "libc.rt"
#define ASTC_MODULE_MATH    "math.rt"
#define ASTC_MODULE_IO      "io.rt"
#define ASTC_MODULE_STRING  "string.rt"
#define ASTC_MODULE_MEMORY  "memory.rt"

// Common function names
#define ASTC_FUNC_PRINTF    "printf"
#define ASTC_FUNC_MALLOC    "malloc"
#define ASTC_FUNC_FREE      "free"
#define ASTC_FUNC_STRLEN    "strlen"
#define ASTC_FUNC_STRCPY    "strcpy"
#define ASTC_FUNC_STRCMP    "strcmp"
#define ASTC_FUNC_MEMCPY    "memcpy"
#define ASTC_FUNC_MEMSET    "memset"
#define ASTC_FUNC_SIN       "sin"
#define ASTC_FUNC_COS       "cos"
#define ASTC_FUNC_SQRT      "sqrt"
#define ASTC_FUNC_POW       "pow"
#define ASTC_FUNC_LOG       "log"
#define ASTC_FUNC_EXP       "exp"

// Error codes specific to program modules
#define ASTC_PROGRAM_SUCCESS           0
#define ASTC_PROGRAM_ERROR_INVALID     -1
#define ASTC_PROGRAM_ERROR_NOT_FOUND   -2
#define ASTC_PROGRAM_ERROR_ALREADY_LOADED -3
#define ASTC_PROGRAM_ERROR_VERSION_MISMATCH -4
#define ASTC_PROGRAM_ERROR_NO_MEMORY   -5

// Module validation and dependency checking

/**
 * Check if a module is available
 * @param module_name Name of the module to check
 * @return true if available, false otherwise
 */
bool astc_program_is_module_available(const char* module_name);

/**
 * Get module dependencies
 * @param module_name Name of the module
 * @param dependencies Array to store dependency names
 * @param max_dependencies Maximum number of dependencies to return
 * @return Number of dependencies found, -1 on error
 */
int astc_program_get_module_dependencies(const char* module_name, 
                                        char dependencies[][128], int max_dependencies);

/**
 * Validate all imported modules
 * @return 0 if all modules are valid, -1 if validation failed
 */
int astc_program_validate_modules(void);

/**
 * Auto-import commonly used system modules
 * @return 0 on success, -1 on error
 */
int astc_program_auto_import_system_modules(void);

// Module search and discovery

/**
 * Search for available modules in standard locations
 * @param search_pattern Pattern to search for (e.g., "*.rt")
 * @param results Array to store found module names
 * @param max_results Maximum number of results to return
 * @return Number of modules found, -1 on error
 */
int astc_program_search_modules(const char* search_pattern, 
                               char results[][128], int max_results);

/**
 * Get module search paths
 * @param paths Array to store search paths
 * @param max_paths Maximum number of paths to return
 * @return Number of paths found, -1 on error
 */
int astc_program_get_module_search_paths(char paths[][256], int max_paths);

/**
 * Add module search path
 * @param path Path to add to module search
 * @return 0 on success, -1 on error
 */
int astc_program_add_module_search_path(const char* path);

#ifdef __cplusplus
}
#endif

#endif // ASTC_PROGRAM_MODULES_H
