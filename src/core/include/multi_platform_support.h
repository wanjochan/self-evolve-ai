/**
 * multi_platform_support.h - Multi-Platform Support System
 * 
 * Header for comprehensive multi-platform support for Windows/Linux/macOS
 */

#ifndef MULTI_PLATFORM_SUPPORT_H
#define MULTI_PLATFORM_SUPPORT_H

#include "astc_platform_compat.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Platform-specific configuration
typedef struct {
    ASTCPlatformType platform_type;
    const char* platform_name;
    const char* platform_id;
    const char* os_family;
    
    // File system characteristics
    char path_separator;
    const char* path_separator_str;
    const char* line_ending;
    bool case_sensitive_fs;
    int max_path_length;
    int max_filename_length;
    
    // Dynamic library support
    const char* lib_prefix;
    const char* lib_extension;
    bool supports_dlopen;
    
    // System call interface
    bool has_posix_api;
    bool has_win32_api;
    bool has_mach_api;
    
    // Platform capabilities
    bool supports_threads;
    bool supports_processes;
    bool supports_shared_memory;
    bool supports_memory_mapping;
    bool supports_signals;
    
    // Performance characteristics
    int default_page_size;
    int cache_line_size;
    bool numa_aware;
} PlatformConfig;

// Platform capabilities structure
typedef struct {
    bool has_threads;
    bool has_processes;
    bool has_signals;
    bool has_shared_memory;
    bool has_memory_mapping;
    bool supports_dlopen;
    bool case_sensitive_fs;
    int max_path_length;
    int default_page_size;
} PlatformCapabilities;

// Multi-platform statistics
typedef struct {
    uint64_t platform_specific_calls;
    uint64_t cross_platform_translations;
    uint64_t runtime_adaptations;
    int supported_platforms;
    ASTCPlatformType current_platform;
    ASTCPlatformType target_platform;
} MultiPlatformStats;

// Core multi-platform functions

/**
 * Initialize multi-platform support
 * @return 0 on success, -1 on error
 */
int multi_platform_support_init(void);

/**
 * Cleanup multi-platform support
 */
void multi_platform_support_cleanup(void);

/**
 * Initialize platform configurations
 * @return 0 on success, -1 on error
 */
int init_platform_configs(void);

/**
 * Detect runtime platform
 * @return 0 on success, -1 on error
 */
int detect_runtime_platform(void);

/**
 * Initialize system call mappings
 * @return 0 on success, -1 on error
 */
int init_syscall_mappings(void);

// Platform configuration functions

/**
 * Get platform configuration
 * @param platform Platform type
 * @return Pointer to configuration, NULL if not found
 */
const PlatformConfig* get_platform_config(ASTCPlatformType platform);

/**
 * Get current platform configuration
 * @return Pointer to current platform configuration
 */
const PlatformConfig* get_current_platform_config(void);

/**
 * Set target platform for cross-compilation
 * @param target_platform Target platform
 * @return 0 on success, -1 on error
 */
int set_target_platform(ASTCPlatformType target_platform);

/**
 * Get target platform
 * @return Current target platform
 */
ASTCPlatformType get_target_platform(void);

/**
 * Check if cross-platform compilation is enabled
 * @return true if enabled, false otherwise
 */
bool is_cross_platform_enabled(void);

// System call mapping functions

/**
 * Add system call mapping
 * @param astc_name ASTC system call name
 * @param platform_name Platform-specific function name
 * @param function_pointer Function pointer (can be NULL)
 * @param description Human-readable description
 * @return 0 on success, -1 on error
 */
int add_syscall_mapping(const char* astc_name, const char* platform_name, void* function_pointer, const char* description);

/**
 * Add Windows-specific system call mappings
 */
void add_win32_syscall_mappings(void);

/**
 * Add POSIX system call mappings
 */
void add_posix_syscall_mappings(void);

/**
 * Add Mach system call mappings (macOS)
 */
void add_mach_syscall_mappings(void);

/**
 * Execute platform-specific system call
 * @param astc_name ASTC system call name
 * @param args Arguments (platform-specific format)
 * @param result Result buffer (platform-specific format)
 * @return 0 on success, -1 on error
 */
int execute_platform_syscall(const char* astc_name, void* args, void* result);

/**
 * List system call mappings
 */
void list_syscall_mappings(void);

// Path and file system functions

/**
 * Normalize path for platform
 * @param input_path Input path
 * @param platform Target platform
 * @param output_path Buffer for normalized path
 * @param output_size Size of output buffer
 * @return 0 on success, -1 on error
 */
int normalize_path_for_platform(const char* input_path, ASTCPlatformType platform, char* output_path, size_t output_size);

/**
 * Get library filename for platform
 * @param base_name Base library name
 * @param platform Target platform
 * @param filename Buffer for filename
 * @param filename_size Size of filename buffer
 * @return 0 on success, -1 on error
 */
int get_library_filename_for_platform(const char* base_name, ASTCPlatformType platform, char* filename, size_t filename_size);

/**
 * Get current working directory (platform-specific)
 * @param buffer Buffer to store directory path
 * @param buffer_size Size of buffer
 * @return 0 on success, -1 on error
 */
int get_current_directory_platform(char* buffer, size_t buffer_size);

/**
 * Create directory (platform-specific)
 * @param path Directory path to create
 * @return 0 on success, -1 on error
 */
int create_directory_platform(const char* path);

/**
 * Check if file exists (platform-specific)
 * @param path File path to check
 * @return true if exists, false otherwise
 */
bool file_exists_platform(const char* path);

/**
 * Get file size (platform-specific)
 * @param path File path
 * @param size Pointer to store file size
 * @return 0 on success, -1 on error
 */
int get_file_size_platform(const char* path, uint64_t* size);

/**
 * Delete file (platform-specific)
 * @param path File path to delete
 * @return 0 on success, -1 on error
 */
int delete_file_platform(const char* path);

// Compatibility and capability functions

/**
 * Check platform compatibility
 * @param platform1 First platform
 * @param platform2 Second platform
 * @return true if compatible, false otherwise
 */
bool is_platform_compatible(ASTCPlatformType platform1, ASTCPlatformType platform2);

/**
 * Get platform name
 * @param platform Platform type
 * @return Platform name string
 */
const char* get_platform_name(ASTCPlatformType platform);

/**
 * Get platform capabilities
 * @param platform Platform type
 * @param has_threads Pointer to store thread capability
 * @param has_processes Pointer to store process capability
 * @param has_signals Pointer to store signal capability
 */
void get_platform_capabilities(ASTCPlatformType platform, bool* has_threads, bool* has_processes, bool* has_signals);

/**
 * Get detailed platform capabilities
 * @param platform Platform type
 * @param capabilities Pointer to store capabilities
 * @return 0 on success, -1 on error
 */
int get_detailed_platform_capabilities(ASTCPlatformType platform, PlatformCapabilities* capabilities);

/**
 * Check if platform supports feature
 * @param platform Platform type
 * @param feature Feature name
 * @return true if supported, false otherwise
 */
bool platform_supports_feature(ASTCPlatformType platform, const char* feature);

// Information and statistics

/**
 * Get multi-platform statistics
 * @param platform_calls Pointer to store platform call count
 * @param translations Pointer to store translation count
 * @param adaptations Pointer to store adaptation count
 */
void get_multi_platform_stats(uint64_t* platform_calls, uint64_t* translations, uint64_t* adaptations);

/**
 * Get detailed multi-platform statistics
 * @param stats Pointer to store detailed statistics
 */
void get_detailed_multi_platform_stats(MultiPlatformStats* stats);

/**
 * List supported platforms
 * @param platforms Array to store platform types
 * @param max_platforms Maximum number of platforms to return
 * @return Number of supported platforms
 */
int list_supported_platforms(ASTCPlatformType* platforms, int max_platforms);

/**
 * Get platform count
 * @return Number of supported platforms
 */
int get_supported_platform_count(void);

// Utility functions

/**
 * Convert platform string to type
 * @param platform_string Platform string
 * @return Platform type, ASTC_PLATFORM_TYPE_UNKNOWN if not found
 */
ASTCPlatformType string_to_platform_type(const char* platform_string);

/**
 * Convert platform type to string
 * @param platform Platform type
 * @return Platform string
 */
const char* platform_type_to_string(ASTCPlatformType platform);

/**
 * Get platform-specific path separator
 * @param platform Platform type
 * @return Path separator character
 */
char get_path_separator_for_platform(ASTCPlatformType platform);

/**
 * Get platform-specific line ending
 * @param platform Platform type
 * @return Line ending string
 */
const char* get_line_ending_for_platform(ASTCPlatformType platform);

/**
 * Get platform-specific library extension
 * @param platform Platform type
 * @return Library extension string
 */
const char* get_library_extension_for_platform(ASTCPlatformType platform);

// Error codes
#define MULTI_PLATFORM_SUCCESS           0
#define MULTI_PLATFORM_ERROR_INVALID     -1
#define MULTI_PLATFORM_ERROR_UNSUPPORTED -2
#define MULTI_PLATFORM_ERROR_NOT_FOUND   -3
#define MULTI_PLATFORM_ERROR_ACCESS      -4

// Feature names for platform_supports_feature()
#define PLATFORM_FEATURE_THREADS         "threads"
#define PLATFORM_FEATURE_PROCESSES       "processes"
#define PLATFORM_FEATURE_SIGNALS         "signals"
#define PLATFORM_FEATURE_SHARED_MEMORY   "shared_memory"
#define PLATFORM_FEATURE_MEMORY_MAPPING  "memory_mapping"
#define PLATFORM_FEATURE_DLOPEN          "dlopen"
#define PLATFORM_FEATURE_POSIX_API       "posix_api"
#define PLATFORM_FEATURE_WIN32_API       "win32_api"
#define PLATFORM_FEATURE_MACH_API        "mach_api"

#ifdef __cplusplus
}
#endif

#endif // MULTI_PLATFORM_SUPPORT_H
