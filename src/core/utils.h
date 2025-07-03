/*
 * utils.h - Header file for utility functions
 * Common utilities used across the self-evolve AI system
 */

#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

// ===============================================
// Architecture Detection Types
// ===============================================

typedef enum {
    ARCH_UNKNOWN = 0,
    ARCH_X86_32,
    ARCH_X86_64,
    ARCH_ARM32,
    ARCH_ARM64
} DetectedArchitecture;

// ===============================================
// Configuration Types
// ===============================================

typedef struct {
    // Basic options
    int verbose_mode;
    int debug_mode;
    int performance_stats;
    int interactive_mode;
    int autonomous_mode;
    int security_level;

    // File paths
    const char* program_file;
    const char* vm_module_override;
    const char* config_file;

    // Program arguments
    int program_argc;
    char** program_argv;
} UnifiedLoaderConfig;

/**
 * Performance statistics structure
 */
typedef struct {
    clock_t start_time;
    clock_t detection_time;
    clock_t vm_load_time;
    clock_t program_load_time;
    clock_t execution_time;
    clock_t end_time;
} PerformanceStats;

/**
 * Loaded VM Module structure for managing native modules
 */
typedef struct {
    void* mapped_memory;           // mmap映射的内存地址
    size_t mapped_size;            // 映射的内存大小
    const char* module_path;       // Path to module
    DetectedArchitecture arch;     // Architecture

    // .native模块的入口点 (从映射内存中解析)
    void* entry_point;             // 模块入口点
    void* code_section;            // 代码段地址
    size_t code_size;              // 代码段大小

    // 执行函数指针 (指向映射内存中的机器码)
    int (*vm_execute)(const char* astc_file, int argc, char* argv[]);
} LoadedVMModule;

/**
 * Dynamic Module structure for managing loaded modules
 */
typedef struct LoadedModule {
    char module_path[512];
    void* handle;                    // HMODULE (Windows) or void* (Unix)
    void* native_module;             // Parsed .native module (simplified)
    int is_dynamic_library;          // True if loaded as DLL/SO, false if .native

    // Function pointers (cached)
    void* main_function;
    void* get_interface_function;

    // Module info
    char name[128];
    char version[32];
    char arch[16];
    int bits;

    struct LoadedModule* next;       // Linked list
} LoadedModule;

// ===============================================
// New Native Module Calling System Types
// ===============================================

/**
 * Native value types for argument and result passing
 */
typedef enum {
    NATIVE_TYPE_VOID = 0,
    NATIVE_TYPE_INT32,
    NATIVE_TYPE_INT64,
    NATIVE_TYPE_FLOAT,
    NATIVE_TYPE_DOUBLE,
    NATIVE_TYPE_STRING,
    NATIVE_TYPE_POINTER,
    NATIVE_TYPE_BOOL
} NativeValueType;

/**
 * Native value structure for arguments and results
 */
typedef struct {
    NativeValueType type;
    union {
        int32_t i32;
        int64_t i64;
        float f32;
        double f64;
        char* string;
        void* pointer;
        int boolean;
    } value;
    size_t size;  // For strings and pointers
} NativeValue;

/**
 * Function descriptor within a native module
 */
typedef struct {
    char name[64];
    void* address;
    uint32_t signature;
    NativeValueType return_type;
    NativeValueType param_types[16];
    int param_count;
} NativeFunctionDescriptor;

/**
 * Module loading flags
 */
typedef enum {
    MODULE_FLAG_NONE = 0,
    MODULE_FLAG_LAZY_LOAD = 1,
    MODULE_FLAG_VERIFY_SIGNATURE = 2,
    MODULE_FLAG_ENABLE_DEBUG = 4,
    MODULE_FLAG_CACHE_FUNCTIONS = 8
} ModuleLoadFlags;

/**
 * Native module handle for the new calling system
 */
typedef struct NativeModuleHandle {
    // Basic module information
    char module_path[512];
    char module_name[128];
    void* mapped_memory;
    size_t mapped_size;
    uint32_t flags;

    // Function table
    NativeFunctionDescriptor functions[256];
    int function_count;

    // Module metadata
    uint32_t version;
    uint32_t architecture;
    uint64_t timestamp;
    char description[256];

    // Runtime state
    int reference_count;
    int is_initialized;
    int last_error_code;
    char last_error_message[512];

    // Linked list for module registry
    struct NativeModuleHandle* next;
} NativeModuleHandle;

// ===============================================
// Architecture Detection Functions
// ===============================================

/**
 * Detect the current system architecture
 * @return DetectedArchitecture enum value
 */
DetectedArchitecture detect_architecture(void);

/**
 * Get string representation of architecture
 * @param arch Architecture enum value
 * @return String representation (e.g., "x64_64", "arm64")
 */
const char* get_architecture_string(DetectedArchitecture arch);

/**
 * Get bit width of architecture
 * @param arch Architecture enum value
 * @return Bit width (32 or 64), or 0 for unknown
 */
int get_architecture_bits(DetectedArchitecture arch);

// ===============================================
// Path Construction Functions
// ===============================================

/**
 * Construct VM module path based on architecture and configuration
 * @param buffer Output buffer for the path
 * @param buffer_size Size of the output buffer
 * @param config Loader configuration
 * @return 0 on success, -1 on error
 */
int construct_vm_module_path(char* buffer, size_t buffer_size, const UnifiedLoaderConfig* config);

// ===============================================
// Logging and Error Handling Functions
// ===============================================

/**
 * Print error message to stderr
 * @param format Printf-style format string
 * @param ... Variable arguments
 */
void print_error(const char* format, ...);

/**
 * Print verbose message if verbose mode is enabled
 * @param config Loader configuration (checked for verbose_mode)
 * @param format Printf-style format string
 * @param ... Variable arguments
 */
void print_verbose(const UnifiedLoaderConfig* config, const char* format, ...);

/**
 * Print informational message
 * @param format Printf-style format string
 * @param ... Variable arguments
 */
void print_info(const char* format, ...);

/**
 * Print warning message
 * @param format Printf-style format string
 * @param ... Variable arguments
 */
void print_warning(const char* format, ...);

/**
 * Print debug message if debug mode is enabled
 * @param config Loader configuration (checked for debug_mode)
 * @param format Printf-style format string
 * @param ... Variable arguments
 */
void print_debug(const UnifiedLoaderConfig* config, const char* format, ...);

// ===============================================
// Memory Management Functions
// ===============================================

/**
 * Allocate executable memory (cross-platform)
 * @param size Size of memory to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
void* allocate_executable_memory(size_t size);

/**
 * Free executable memory (cross-platform)
 * @param ptr Pointer to memory to free
 * @param size Size of memory (required on some platforms)
 */
void free_executable_memory(void* ptr, size_t size);

// ===============================================
// File Utility Functions
// ===============================================

/**
 * Check if a file exists
 * @param path File path to check
 * @return 1 if file exists, 0 otherwise
 */
int file_exists(const char* path);

/**
 * Get the size of a file
 * @param path File path
 * @return File size in bytes, or -1 on error
 */
long get_file_size(const char* path);

/**
 * Read entire file into a buffer
 * @param path File path to read
 * @param buffer Output pointer to allocated buffer (caller must free)
 * @param size Output size of the file
 * @return 0 on success, -1 on error
 */
int read_file_to_buffer(const char* path, void** buffer, size_t* size);

// ===============================================
// String Utility Functions
// ===============================================

/**
 * Safe string duplication (handles NULL input)
 * @param str String to duplicate
 * @return Duplicated string (caller must free), or NULL
 */
char* safe_strdup(const char* str);

/**
 * Safe snprintf with error checking
 * @param buffer Output buffer
 * @param size Buffer size
 * @param format Printf-style format string
 * @param ... Variable arguments
 * @return Number of characters written, or -1 on error
 */
int safe_snprintf(char* buffer, size_t size, const char* format, ...);

// ===============================================
// VM Module Management Functions
// ===============================================

/**
 * Parse native module format and set up execution entry points
 * @param mapped_memory Pointer to mapped native module memory
 * @param file_size Size of the native module file
 * @param vm_module VM module structure to populate
 * @return 0 on success, -1 on error
 */
int parse_native_module(void* mapped_memory, size_t file_size, LoadedVMModule* vm_module);

/**
 * Load VM module from file path
 * @param vm_path Path to the .native VM module file
 * @param vm_module VM module structure to populate
 * @param config Loader configuration
 * @return 0 on success, -1 on error
 */
int load_vm_module(const char* vm_path, LoadedVMModule* vm_module, const UnifiedLoaderConfig* config);

/**
 * Unload VM module and free resources
 * @param vm_module VM module structure to unload
 */
void unload_vm_module(LoadedVMModule* vm_module);

/**
 * Execute ASTC program via native module
 * @param vm_module Loaded VM module
 * @param astc_file Path to ASTC program file
 * @param argc Number of arguments
 * @param argv Argument array
 * @return Program exit code
 */
int execute_astc_via_native_module(LoadedVMModule* vm_module, const char* astc_file, int argc, char* argv[]);

/**
 * Execute program through the VM module
 * @param vm_module Loaded VM module
 * @param config Loader configuration
 * @param stats Performance statistics (optional)
 * @return Program exit code
 */
int execute_program(LoadedVMModule* vm_module, const UnifiedLoaderConfig* config, PerformanceStats* stats);

// ===============================================
// Dynamic Module Loading Functions
// ===============================================

/**
 * Initialize the module loader system
 * @return 0 on success, -1 on error
 */
int module_loader_init(void);

/**
 * Cleanup the module loader system and unload all modules
 */
void module_loader_cleanup(void);

/**
 * Load a dynamic library (DLL/SO)
 * @param path Path to the library file
 * @return Handle to the loaded library, or NULL on error
 */
void* load_dynamic_library(const char* path);

/**
 * Unload a dynamic library
 * @param handle Handle to the library
 */
void unload_dynamic_library(void* handle);

/**
 * Get symbol address from a loaded library
 * @param handle Handle to the library
 * @param symbol_name Name of the symbol to find
 * @return Pointer to the symbol, or NULL if not found
 */
void* get_symbol_address(void* handle, const char* symbol_name);

/**
 * Load a native module (.native file or dynamic library)
 * @param module_path Path to the module file
 * @return Pointer to LoadedModule structure, or NULL on error
 */
LoadedModule* load_native_module(const char* module_path);

/**
 * Unload a native module and free resources
 * @param module Pointer to the LoadedModule structure
 */
void unload_native_module(LoadedModule* module);

/**
 * Find a loaded module by path
 * @param module_path Path to search for
 * @return Pointer to LoadedModule if found, NULL otherwise
 */
LoadedModule* find_loaded_module(const char* module_path);

/**
 * Get a function pointer from a loaded module
 * @param module Pointer to the LoadedModule
 * @param function_name Name of the function to find
 * @return Function pointer, or NULL if not found
 */
void* get_module_function(LoadedModule* module, const char* function_name);

/**
 * Load a module by name, architecture, and bit width
 * @param module_name Name of the module (e.g., "vm", "libc")
 * @param arch Architecture string (e.g., "x64", "arm64")
 * @param bits Bit width (32 or 64)
 * @return Pointer to LoadedModule structure, or NULL on error
 */
LoadedModule* load_module_by_name(const char* module_name, const char* arch, int bits);

/**
 * Print information about all loaded modules
 */
void print_loaded_modules(void);

/**
 * Get the count of loaded modules
 * @return Number of loaded modules
 */
int get_loaded_module_count(void);

// ===============================================
// New Native Module Calling System Functions
// ===============================================

/**
 * Open and load a native module (.native file)
 * @param module_path Path to the .native module file
 * @param module_name Optional name for the module (can be NULL for auto-detect)
 * @param flags Loading flags (MODULE_FLAG_* constants)
 * @return Handle to the loaded module, or NULL on error
 */
NativeModuleHandle* module_open_native(const char* module_path, const char* module_name, uint32_t flags);

/**
 * Unload a native module and free all resources
 * @param handle Handle to the module to unload
 * @return 0 on success, -1 on error
 */
int module_unload_native(NativeModuleHandle* handle);

/**
 * Execute a function within a native module
 * @param handle Handle to the loaded module
 * @param function_name Name of the function to execute
 * @param args Array of arguments to pass (can be NULL if arg_count is 0)
 * @param arg_count Number of arguments
 * @param result Pointer to store the result (can be NULL if no return value expected)
 * @return 0 on success, -1 on error
 */
int native_exec_native(NativeModuleHandle* handle, const char* function_name,
                      NativeValue* args, int arg_count, NativeValue* result);

/**
 * Get function information from a loaded module
 * @param handle Handle to the loaded module
 * @param function_name Name of the function to query
 * @param descriptor Pointer to store function descriptor
 * @return 0 on success, -1 on error
 */
int module_get_function_info(NativeModuleHandle* handle, const char* function_name,
                            NativeFunctionDescriptor* descriptor);

/**
 * List all functions in a loaded module
 * @param handle Handle to the loaded module
 * @param function_names Array to store function names
 * @param max_functions Maximum number of functions to return
 * @return Number of functions found, -1 on error
 */
int module_list_functions(NativeModuleHandle* handle, char function_names[][64], int max_functions);

/**
 * Get the last error message for a module
 * @param handle Handle to the module
 * @return Error message string, or NULL if no error
 */
const char* module_get_last_error(NativeModuleHandle* handle);

/**
 * Create a NativeValue from basic types
 */
NativeValue native_value_int32(int32_t value);
NativeValue native_value_int64(int64_t value);
NativeValue native_value_float(float value);
NativeValue native_value_double(double value);
NativeValue native_value_string(const char* value);
NativeValue native_value_pointer(void* value, size_t size);
NativeValue native_value_bool(int value);

/**
 * Extract values from NativeValue
 */
int32_t native_value_as_int32(const NativeValue* value);
int64_t native_value_as_int64(const NativeValue* value);
float native_value_as_float(const NativeValue* value);
double native_value_as_double(const NativeValue* value);
const char* native_value_as_string(const NativeValue* value);
void* native_value_as_pointer(const NativeValue* value);
int native_value_as_bool(const NativeValue* value);

/**
 * Initialize the native module calling system
 * @return 0 on success, -1 on error
 */
int native_module_system_init(void);

/**
 * Cleanup the native module calling system
 */
void native_module_system_cleanup(void);

/**
 * Get count of loaded native modules
 * @return Number of loaded modules
 */
int native_module_get_count(void);

/**
 * Print information about all loaded native modules
 */
void native_module_print_info(void);

#endif // UTILS_H
