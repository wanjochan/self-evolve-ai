/**
 * development_toolchain.h - Comprehensive Development Toolchain
 * 
 * Header for complete development toolchain including debugger, profiler,
 * module manager, and other development tools
 */

#ifndef DEVELOPMENT_TOOLCHAIN_H
#define DEVELOPMENT_TOOLCHAIN_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Toolchain component types
typedef enum {
    TOOL_DEBUGGER = 1,
    TOOL_PROFILER = 2,
    TOOL_MODULE_MANAGER = 3,
    TOOL_CODE_ANALYZER = 4,
    TOOL_PERFORMANCE_MONITOR = 5,
    TOOL_DEPENDENCY_RESOLVER = 6,
    TOOL_BUILD_SYSTEM = 7,
    TOOL_TEST_RUNNER = 8
} ToolchainComponentType;

// Toolchain component structure
typedef struct {
    ToolchainComponentType type;
    char name[128];
    char description[256];
    char version[32];
    bool is_active;
    bool is_initialized;
    
    // Function pointers for tool operations
    int (*init_func)(void);
    void (*cleanup_func)(void);
    int (*run_func)(int argc, char* argv[]);
    void (*help_func)(void);
    
    // Statistics
    uint64_t usage_count;
    uint64_t success_count;
    uint64_t error_count;
    time_t last_used;
} ToolchainComponent;

// Toolchain configuration
typedef struct {
    bool verbose_mode;
    bool debug_mode;
    char workspace_path[256];
    char output_directory[256];
    char temp_directory[256];
    int max_parallel_jobs;
} ToolchainConfig;

// Toolchain statistics
typedef struct {
    uint64_t total_tool_invocations;
    uint64_t successful_operations;
    uint64_t failed_operations;
    int active_tools;
    int initialized_tools;
    time_t last_operation;
} ToolchainStats;

// Core toolchain functions

/**
 * Initialize development toolchain
 * @return 0 on success, -1 on error
 */
int development_toolchain_init(void);

/**
 * Cleanup development toolchain
 */
void development_toolchain_cleanup(void);

/**
 * Configure toolchain
 * @param config Toolchain configuration
 * @return 0 on success, -1 on error
 */
int configure_toolchain(const ToolchainConfig* config);

/**
 * Get toolchain configuration
 * @param config Pointer to store configuration
 */
void get_toolchain_config(ToolchainConfig* config);

// Tool registration and management

/**
 * Register all toolchain components
 * @return 0 on success, -1 on error
 */
int register_toolchain_components(void);

/**
 * Register individual tool component
 * @param type Tool type
 * @param name Tool name
 * @param description Tool description
 * @param version Tool version
 * @param init_func Initialization function
 * @param cleanup_func Cleanup function
 * @param run_func Run function
 * @param help_func Help function
 * @return 0 on success, -1 on error
 */
int register_tool_component(ToolchainComponentType type, const char* name, const char* description, const char* version,
                           int (*init_func)(void), void (*cleanup_func)(void), 
                           int (*run_func)(int, char**), void (*help_func)(void));

/**
 * Unregister tool component
 * @param tool_name Name of tool to unregister
 * @return 0 on success, -1 on error
 */
int unregister_tool_component(const char* tool_name);

/**
 * Find tool by name
 * @param tool_name Name of tool to find
 * @return Pointer to tool component, NULL if not found
 */
ToolchainComponent* find_tool_by_name(const char* tool_name);

/**
 * Find tool by type
 * @param type Tool type
 * @return Pointer to tool component, NULL if not found
 */
ToolchainComponent* find_tool_by_type(ToolchainComponentType type);

// Tool execution

/**
 * Run specific tool
 * @param tool_name Name of tool to run
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, -1 on error
 */
int run_tool(const char* tool_name, int argc, char* argv[]);

/**
 * Run tool by type
 * @param type Tool type
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, -1 on error
 */
int run_tool_by_type(ToolchainComponentType type, int argc, char* argv[]);

/**
 * Run multiple tools in sequence
 * @param tool_names Array of tool names
 * @param tool_count Number of tools
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, -1 on error
 */
int run_tools_sequence(const char* tool_names[], int tool_count, int argc, char* argv[]);

// Information and help

/**
 * List all available tools
 */
void list_available_tools(void);

/**
 * Show tool help
 * @param tool_name Name of tool (NULL for general help)
 */
void show_tool_help(const char* tool_name);

/**
 * Get tool count
 * @return Number of registered tools
 */
int get_tool_count(void);

/**
 * Get tool information
 * @param tool_name Name of tool
 * @param component Pointer to store tool information
 * @return 0 on success, -1 on error
 */
int get_tool_info(const char* tool_name, ToolchainComponent* component);

// Statistics and monitoring

/**
 * Get toolchain statistics
 * @param stats Pointer to store statistics
 */
void get_toolchain_stats(ToolchainStats* stats);

/**
 * Reset toolchain statistics
 */
void reset_toolchain_stats(void);

/**
 * Get tool usage statistics
 * @param tool_name Name of tool
 * @param usage_count Pointer to store usage count
 * @param success_count Pointer to store success count
 * @param error_count Pointer to store error count
 * @return 0 on success, -1 on error
 */
int get_tool_usage_stats(const char* tool_name, uint64_t* usage_count, uint64_t* success_count, uint64_t* error_count);

// Tool-specific function declarations

// Debugger functions
int debugger_init(void);
void debugger_cleanup(void);
int debugger_run(int argc, char* argv[]);
void debugger_help(void);

// Profiler functions
int profiler_init(void);
void profiler_cleanup(void);
int profiler_run(int argc, char* argv[]);
void profiler_help(void);

// Module manager functions
int module_manager_init(void);
void module_manager_cleanup(void);
int module_manager_run(int argc, char* argv[]);
void module_manager_help(void);

// Code analyzer functions
int code_analyzer_init(void);
void code_analyzer_cleanup(void);
int code_analyzer_run(int argc, char* argv[]);
void code_analyzer_help(void);

// Performance monitor functions
int perfmon_init(void);
void perfmon_cleanup(void);
int perfmon_run(int argc, char* argv[]);
void perfmon_help(void);

// Dependency resolver functions
int depres_init(void);
void depres_cleanup(void);
int depres_run(int argc, char* argv[]);
void depres_help(void);

// Build system functions
int build_system_init(void);
void build_system_cleanup(void);
int build_system_run(int argc, char* argv[]);
void build_system_help(void);

// Test runner functions
int test_runner_init(void);
void test_runner_cleanup(void);
int test_runner_run(int argc, char* argv[]);
void test_runner_help(void);

// Utility functions

/**
 * Parse tool arguments
 * @param argc Argument count
 * @param argv Argument vector
 * @param tool_name Pointer to store tool name
 * @param tool_argc Pointer to store tool argument count
 * @param tool_argv Pointer to store tool argument vector
 * @return 0 on success, -1 on error
 */
int parse_tool_arguments(int argc, char* argv[], char** tool_name, int* tool_argc, char*** tool_argv);

/**
 * Validate tool name
 * @param tool_name Tool name to validate
 * @return true if valid, false otherwise
 */
bool validate_tool_name(const char* tool_name);

/**
 * Get tool type string
 * @param type Tool type
 * @return String representation of tool type
 */
const char* get_tool_type_string(ToolchainComponentType type);

/**
 * Convert string to tool type
 * @param type_string String representation
 * @return Tool type, or -1 if invalid
 */
ToolchainComponentType string_to_tool_type(const char* type_string);

/**
 * Check if tool is available
 * @param tool_name Name of tool to check
 * @return true if available, false otherwise
 */
bool is_tool_available(const char* tool_name);

/**
 * Get tool status
 * @param tool_name Name of tool
 * @return Status string
 */
const char* get_tool_status(const char* tool_name);

// Error codes
#define TOOLCHAIN_SUCCESS           0
#define TOOLCHAIN_ERROR_INVALID     -1
#define TOOLCHAIN_ERROR_NOT_FOUND   -2
#define TOOLCHAIN_ERROR_NOT_INIT    -3
#define TOOLCHAIN_ERROR_EXECUTION   -4
#define TOOLCHAIN_ERROR_CONFIG      -5

// Tool names (constants)
#define TOOL_NAME_DEBUGGER          "astc-debugger"
#define TOOL_NAME_PROFILER          "astc-profiler"
#define TOOL_NAME_MODULE_MANAGER    "astc-modmgr"
#define TOOL_NAME_CODE_ANALYZER     "astc-analyzer"
#define TOOL_NAME_PERFORMANCE_MONITOR "astc-perfmon"
#define TOOL_NAME_DEPENDENCY_RESOLVER "astc-depres"
#define TOOL_NAME_BUILD_SYSTEM      "astc-build"
#define TOOL_NAME_TEST_RUNNER       "astc-test"

#ifdef __cplusplus
}
#endif

#endif // DEVELOPMENT_TOOLCHAIN_H
