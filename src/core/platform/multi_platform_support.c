/**
 * multi_platform_support.c - Multi-Platform Support System
 * 
 * Comprehensive multi-platform support for Windows/Linux/macOS with
 * platform-specific system calls, file formats, and runtime adaptation.
 */

#include "../include/astc_platform_compat.h"
#include "../include/logger.h"
#include "../include/native_format.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <direct.h>
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STR "\\"
#define LINE_ENDING "\r\n"
#elif defined(__APPLE__)
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <mach-o/dyld.h>
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"
#define LINE_ENDING "\n"
#else // Linux and other Unix-like
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dlfcn.h>
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"
#define LINE_ENDING "\n"
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

// Platform-specific system call mappings
typedef struct {
    const char* astc_name;
    const char* platform_name;
    void* function_pointer;
    const char* description;
} SystemCallMapping;

// Multi-platform support state
static struct {
    PlatformConfig configs[8];
    int config_count;
    ASTCPlatformType current_platform;
    bool initialized;
    
    // Runtime platform detection
    bool runtime_detection_enabled;
    ASTCPlatformType detected_platform;
    
    // Cross-platform compilation
    bool cross_platform_enabled;
    ASTCPlatformType target_platform;
    
    // System call mappings
    SystemCallMapping* syscall_mappings;
    int syscall_mapping_count;
    
    // Statistics
    uint64_t platform_specific_calls;
    uint64_t cross_platform_translations;
    uint64_t runtime_adaptations;
} g_multi_platform = {0};

// Initialize multi-platform support
int multi_platform_support_init(void) {
    if (g_multi_platform.initialized) {
        return 0;
    }
    
    memset(&g_multi_platform, 0, sizeof(g_multi_platform));
    
    // Initialize platform configurations
    if (init_platform_configs() != 0) {
        LOG_PLATFORM_ERROR("Failed to initialize platform configurations");
        return -1;
    }
    
    // Detect current platform
    if (detect_runtime_platform() != 0) {
        LOG_PLATFORM_ERROR("Failed to detect runtime platform");
        return -1;
    }
    
    // Initialize system call mappings
    if (init_syscall_mappings() != 0) {
        LOG_PLATFORM_ERROR("Failed to initialize system call mappings");
        return -1;
    }
    
    g_multi_platform.runtime_detection_enabled = true;
    g_multi_platform.cross_platform_enabled = true;
    g_multi_platform.initialized = true;
    
    LOG_PLATFORM_INFO("Multi-platform support initialized");
    LOG_PLATFORM_INFO("Current platform: %s", get_platform_name(g_multi_platform.current_platform));
    LOG_PLATFORM_INFO("Detected platform: %s", get_platform_name(g_multi_platform.detected_platform));
    
    return 0;
}

// Cleanup multi-platform support
void multi_platform_support_cleanup(void) {
    if (!g_multi_platform.initialized) {
        return;
    }
    
    LOG_PLATFORM_INFO("Multi-platform statistics:");
    LOG_PLATFORM_INFO("  Platform-specific calls: %llu", g_multi_platform.platform_specific_calls);
    LOG_PLATFORM_INFO("  Cross-platform translations: %llu", g_multi_platform.cross_platform_translations);
    LOG_PLATFORM_INFO("  Runtime adaptations: %llu", g_multi_platform.runtime_adaptations);
    
    if (g_multi_platform.syscall_mappings) {
        free(g_multi_platform.syscall_mappings);
        g_multi_platform.syscall_mappings = NULL;
    }
    
    g_multi_platform.initialized = false;
}

// Initialize platform configurations
int init_platform_configs(void) {
    // Windows configuration
    PlatformConfig* windows = &g_multi_platform.configs[g_multi_platform.config_count++];
    windows->platform_type = ASTC_PLATFORM_TYPE_WINDOWS;
    windows->platform_name = "Windows";
    windows->platform_id = "win32";
    windows->os_family = "Windows NT";
    windows->path_separator = '\\';
    windows->path_separator_str = "\\";
    windows->line_ending = "\r\n";
    windows->case_sensitive_fs = false;
    windows->max_path_length = 260;
    windows->max_filename_length = 255;
    windows->lib_prefix = "";
    windows->lib_extension = ".dll";
    windows->supports_dlopen = true;
    windows->has_posix_api = false;
    windows->has_win32_api = true;
    windows->has_mach_api = false;
    windows->supports_threads = true;
    windows->supports_processes = true;
    windows->supports_shared_memory = true;
    windows->supports_memory_mapping = true;
    windows->supports_signals = false;
    windows->default_page_size = 4096;
    windows->cache_line_size = 64;
    windows->numa_aware = true;
    
    // Linux configuration
    PlatformConfig* linux_config = &g_multi_platform.configs[g_multi_platform.config_count++];
    linux_config->platform_type = ASTC_PLATFORM_TYPE_LINUX;
    linux_config->platform_name = "Linux";
    linux_config->platform_id = "linux";
    linux_config->os_family = "Unix";
    linux_config->path_separator = '/';
    linux_config->path_separator_str = "/";
    linux_config->line_ending = "\n";
    linux_config->case_sensitive_fs = true;
    linux_config->max_path_length = 4096;
    linux_config->max_filename_length = 255;
    linux_config->lib_prefix = "lib";
    linux_config->lib_extension = ".so";
    linux_config->supports_dlopen = true;
    linux_config->has_posix_api = true;
    linux_config->has_win32_api = false;
    linux_config->has_mach_api = false;
    linux_config->supports_threads = true;
    linux_config->supports_processes = true;
    linux_config->supports_shared_memory = true;
    linux_config->supports_memory_mapping = true;
    linux_config->supports_signals = true;
    linux_config->default_page_size = 4096;
    linux_config->cache_line_size = 64;
    linux_config->numa_aware = true;
    
    // macOS configuration
    PlatformConfig* macos = &g_multi_platform.configs[g_multi_platform.config_count++];
    macos->platform_type = ASTC_PLATFORM_TYPE_MACOS;
    macos->platform_name = "macOS";
    macos->platform_id = "darwin";
    macos->os_family = "Unix";
    macos->path_separator = '/';
    macos->path_separator_str = "/";
    macos->line_ending = "\n";
    macos->case_sensitive_fs = false; // HFS+ default, but can be case-sensitive
    macos->max_path_length = 1024;
    macos->max_filename_length = 255;
    macos->lib_prefix = "lib";
    macos->lib_extension = ".dylib";
    macos->supports_dlopen = true;
    macos->has_posix_api = true;
    macos->has_win32_api = false;
    macos->has_mach_api = true;
    macos->supports_threads = true;
    macos->supports_processes = true;
    macos->supports_shared_memory = true;
    macos->supports_memory_mapping = true;
    macos->supports_signals = true;
    macos->default_page_size = 4096;
    macos->cache_line_size = 64;
    macos->numa_aware = false;
    
    LOG_PLATFORM_DEBUG("Initialized %d platform configurations", g_multi_platform.config_count);
    return 0;
}

// Detect runtime platform
int detect_runtime_platform(void) {
#ifdef _WIN32
    g_multi_platform.detected_platform = ASTC_PLATFORM_TYPE_WINDOWS;
#elif defined(__APPLE__)
    g_multi_platform.detected_platform = ASTC_PLATFORM_TYPE_MACOS;
#elif defined(__linux__)
    g_multi_platform.detected_platform = ASTC_PLATFORM_TYPE_LINUX;
#else
    g_multi_platform.detected_platform = ASTC_PLATFORM_TYPE_UNKNOWN;
#endif
    
    g_multi_platform.current_platform = g_multi_platform.detected_platform;
    
    const PlatformConfig* config = get_platform_config(g_multi_platform.current_platform);
    if (config) {
        LOG_PLATFORM_DEBUG("Detected platform: %s (%s)", config->platform_name, config->platform_id);
    } else {
        LOG_PLATFORM_WARN("Unknown platform detected");
    }
    
    return 0;
}

// Initialize system call mappings
int init_syscall_mappings(void) {
    // Allocate initial mapping table
    g_multi_platform.syscall_mappings = malloc(256 * sizeof(SystemCallMapping));
    if (!g_multi_platform.syscall_mappings) {
        LOG_PLATFORM_ERROR("Failed to allocate system call mapping table");
        return -1;
    }
    
    g_multi_platform.syscall_mapping_count = 0;
    
    // Add common system call mappings
    add_syscall_mapping("file.open", "fopen", NULL, "Open file");
    add_syscall_mapping("file.close", "fclose", NULL, "Close file");
    add_syscall_mapping("file.read", "fread", NULL, "Read from file");
    add_syscall_mapping("file.write", "fwrite", NULL, "Write to file");
    add_syscall_mapping("memory.alloc", "malloc", NULL, "Allocate memory");
    add_syscall_mapping("memory.free", "free", NULL, "Free memory");
    add_syscall_mapping("process.exit", "exit", NULL, "Exit process");
    
    // Platform-specific mappings
    const PlatformConfig* config = get_current_platform_config();
    if (config) {
        if (config->has_win32_api) {
            add_win32_syscall_mappings();
        }
        if (config->has_posix_api) {
            add_posix_syscall_mappings();
        }
        if (config->has_mach_api) {
            add_mach_syscall_mappings();
        }
    }
    
    LOG_PLATFORM_DEBUG("Initialized %d system call mappings", g_multi_platform.syscall_mapping_count);
    return 0;
}

// Add system call mapping
int add_syscall_mapping(const char* astc_name, const char* platform_name, void* function_pointer, const char* description) {
    if (g_multi_platform.syscall_mapping_count >= 256) {
        LOG_PLATFORM_ERROR("System call mapping table full");
        return -1;
    }
    
    SystemCallMapping* mapping = &g_multi_platform.syscall_mappings[g_multi_platform.syscall_mapping_count++];
    strncpy((char*)mapping->astc_name, astc_name, 63);
    strncpy((char*)mapping->platform_name, platform_name, 63);
    mapping->function_pointer = function_pointer;
    strncpy((char*)mapping->description, description, 127);
    
    return 0;
}

// Add Windows-specific system call mappings
void add_win32_syscall_mappings(void) {
#ifdef _WIN32
    add_syscall_mapping("thread.create", "CreateThread", (void*)CreateThread, "Create thread");
    add_syscall_mapping("process.create", "CreateProcess", (void*)CreateProcessA, "Create process");
    add_syscall_mapping("file.map", "CreateFileMapping", (void*)CreateFileMappingA, "Create file mapping");
    add_syscall_mapping("library.load", "LoadLibrary", (void*)LoadLibraryA, "Load dynamic library");
    add_syscall_mapping("library.symbol", "GetProcAddress", (void*)GetProcAddress, "Get symbol address");
#endif
}

// Add POSIX system call mappings
void add_posix_syscall_mappings(void) {
#ifndef _WIN32
    add_syscall_mapping("thread.create", "pthread_create", NULL, "Create POSIX thread");
    add_syscall_mapping("process.fork", "fork", NULL, "Fork process");
    add_syscall_mapping("file.map", "mmap", NULL, "Memory map file");
    add_syscall_mapping("library.load", "dlopen", (void*)dlopen, "Load dynamic library");
    add_syscall_mapping("library.symbol", "dlsym", (void*)dlsym, "Get symbol address");
#endif
}

// Add Mach system call mappings (macOS)
void add_mach_syscall_mappings(void) {
#ifdef __APPLE__
    add_syscall_mapping("mach.port", "mach_port_allocate", NULL, "Allocate Mach port");
    add_syscall_mapping("mach.task", "task_for_pid", NULL, "Get task for PID");
#endif
}

// Get platform configuration
const PlatformConfig* get_platform_config(ASTCPlatformType platform) {
    for (int i = 0; i < g_multi_platform.config_count; i++) {
        if (g_multi_platform.configs[i].platform_type == platform) {
            return &g_multi_platform.configs[i];
        }
    }
    return NULL;
}

// Get current platform configuration
const PlatformConfig* get_current_platform_config(void) {
    return get_platform_config(g_multi_platform.current_platform);
}

// Set target platform for cross-compilation
int set_target_platform(ASTCPlatformType target_platform) {
    const PlatformConfig* config = get_platform_config(target_platform);
    if (!config) {
        LOG_PLATFORM_ERROR("Unsupported target platform: %d", target_platform);
        return -1;
    }
    
    g_multi_platform.target_platform = target_platform;
    g_multi_platform.cross_platform_enabled = (target_platform != g_multi_platform.current_platform);
    
    LOG_PLATFORM_INFO("Target platform set to: %s", config->platform_name);
    if (g_multi_platform.cross_platform_enabled) {
        LOG_PLATFORM_INFO("Cross-platform compilation enabled: %s -> %s",
                         get_platform_name(g_multi_platform.current_platform),
                         get_platform_name(target_platform));
    }
    
    return 0;
}

// Normalize path for platform
int normalize_path_for_platform(const char* input_path, ASTCPlatformType platform, char* output_path, size_t output_size) {
    if (!input_path || !output_path) {
        return -1;
    }
    
    const PlatformConfig* config = get_platform_config(platform);
    if (!config) {
        return -1;
    }
    
    size_t input_len = strlen(input_path);
    if (input_len >= output_size) {
        return -1;
    }
    
    // Copy and convert path separators
    for (size_t i = 0; i < input_len; i++) {
        char c = input_path[i];
        if (c == '/' || c == '\\') {
            output_path[i] = config->path_separator;
        } else {
            output_path[i] = c;
        }
    }
    output_path[input_len] = '\0';
    
    // Handle case sensitivity
    if (!config->case_sensitive_fs) {
        // Convert to lowercase for case-insensitive filesystems
        for (size_t i = 0; i < input_len; i++) {
            if (output_path[i] >= 'A' && output_path[i] <= 'Z') {
                output_path[i] += 32; // Convert to lowercase
            }
        }
    }
    
    return 0;
}

// Get library filename for platform
int get_library_filename_for_platform(const char* base_name, ASTCPlatformType platform, char* filename, size_t filename_size) {
    if (!base_name || !filename) {
        return -1;
    }
    
    const PlatformConfig* config = get_platform_config(platform);
    if (!config) {
        return -1;
    }
    
    int result = snprintf(filename, filename_size, "%s%s%s", 
                         config->lib_prefix, base_name, config->lib_extension);
    
    if (result < 0 || (size_t)result >= filename_size) {
        return -1;
    }
    
    return 0;
}

// Execute platform-specific system call
int execute_platform_syscall(const char* astc_name, void* args, void* result) {
    if (!astc_name) {
        return -1;
    }
    
    g_multi_platform.platform_specific_calls++;
    
    // Find system call mapping
    SystemCallMapping* mapping = NULL;
    for (int i = 0; i < g_multi_platform.syscall_mapping_count; i++) {
        if (strcmp(g_multi_platform.syscall_mappings[i].astc_name, astc_name) == 0) {
            mapping = &g_multi_platform.syscall_mappings[i];
            break;
        }
    }
    
    if (!mapping) {
        LOG_PLATFORM_WARN("No mapping found for system call: %s", astc_name);
        return -1;
    }
    
    LOG_PLATFORM_DEBUG("Executing platform syscall: %s -> %s", astc_name, mapping->platform_name);
    
    // Execute platform-specific call
    // This is a simplified implementation - real implementation would handle
    // argument marshalling and calling conventions properly
    
    if (mapping->function_pointer) {
        // Call the function pointer (simplified)
        // In reality, this would need proper argument handling
        LOG_PLATFORM_DEBUG("Calling function pointer for %s", mapping->platform_name);
    } else {
        LOG_PLATFORM_DEBUG("No function pointer available for %s", mapping->platform_name);
    }
    
    return 0;
}

// Check platform compatibility
bool is_platform_compatible(ASTCPlatformType platform1, ASTCPlatformType platform2) {
    // Same platform is always compatible
    if (platform1 == platform2) {
        return true;
    }
    
    const PlatformConfig* config1 = get_platform_config(platform1);
    const PlatformConfig* config2 = get_platform_config(platform2);
    
    if (!config1 || !config2) {
        return false;
    }
    
    // Check OS family compatibility
    if (strcmp(config1->os_family, config2->os_family) == 0) {
        return true;
    }
    
    // Unix-like platforms have some compatibility
    if ((strcmp(config1->os_family, "Unix") == 0) && (strcmp(config2->os_family, "Unix") == 0)) {
        return true;
    }
    
    return false;
}

// Get platform name
const char* get_platform_name(ASTCPlatformType platform) {
    const PlatformConfig* config = get_platform_config(platform);
    return config ? config->platform_name : "Unknown";
}

// Get platform capabilities
void get_platform_capabilities(ASTCPlatformType platform, bool* has_threads, bool* has_processes, bool* has_signals) {
    const PlatformConfig* config = get_platform_config(platform);
    if (config) {
        if (has_threads) *has_threads = config->supports_threads;
        if (has_processes) *has_processes = config->supports_processes;
        if (has_signals) *has_signals = config->supports_signals;
    } else {
        if (has_threads) *has_threads = false;
        if (has_processes) *has_processes = false;
        if (has_signals) *has_signals = false;
    }
}

// Get current working directory (platform-specific)
int get_current_directory_platform(char* buffer, size_t buffer_size) {
    if (!buffer) {
        return -1;
    }
    
#ifdef _WIN32
    DWORD result = GetCurrentDirectoryA((DWORD)buffer_size, buffer);
    return (result > 0 && result < buffer_size) ? 0 : -1;
#else
    char* result = getcwd(buffer, buffer_size);
    return result ? 0 : -1;
#endif
}

// Create directory (platform-specific)
int create_directory_platform(const char* path) {
    if (!path) {
        return -1;
    }
    
#ifdef _WIN32
    return CreateDirectoryA(path, NULL) ? 0 : -1;
#else
    return mkdir(path, 0755);
#endif
}

// Check if file exists (platform-specific)
bool file_exists_platform(const char* path) {
    if (!path) {
        return false;
    }
    
#ifdef _WIN32
    DWORD attributes = GetFileAttributesA(path);
    return (attributes != INVALID_FILE_ATTRIBUTES);
#else
    return access(path, F_OK) == 0;
#endif
}

// List system call mappings
void list_syscall_mappings(void) {
    LOG_PLATFORM_INFO("System call mappings (%d):", g_multi_platform.syscall_mapping_count);
    for (int i = 0; i < g_multi_platform.syscall_mapping_count; i++) {
        SystemCallMapping* mapping = &g_multi_platform.syscall_mappings[i];
        LOG_PLATFORM_INFO("  %s -> %s: %s", mapping->astc_name, mapping->platform_name, mapping->description);
    }
}

// Get multi-platform statistics
void get_multi_platform_stats(uint64_t* platform_calls, uint64_t* translations, uint64_t* adaptations) {
    if (platform_calls) *platform_calls = g_multi_platform.platform_specific_calls;
    if (translations) *translations = g_multi_platform.cross_platform_translations;
    if (adaptations) *adaptations = g_multi_platform.runtime_adaptations;
}
