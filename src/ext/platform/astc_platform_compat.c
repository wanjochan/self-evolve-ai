/**
 * astc_platform_compat.c - ASTC Cross-Platform Compatibility Layer
 * 
 * Implements cross-platform compatibility for ASTC bytecode programs,
 * ensuring "write once, run anywhere" capability across different
 * operating systems and architectures.
 */

#include "include/astc_platform_compat.h"
#include "include/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Platform detection
#ifdef _WIN32
    #define ASTC_PLATFORM_WINDOWS
    #ifdef _WIN64
        #define ASTC_ARCH_X64
    #else
        #define ASTC_ARCH_X86
    #endif
#elif defined(__linux__)
    #define ASTC_PLATFORM_LINUX
    #ifdef __x86_64__
        #define ASTC_ARCH_X64
    #elif defined(__aarch64__)
        #define ASTC_ARCH_ARM64
    #elif defined(__arm__)
        #define ASTC_ARCH_ARM32
    #else
        #define ASTC_ARCH_X86
    #endif
#elif defined(__APPLE__)
    #define ASTC_PLATFORM_MACOS
    #ifdef __x86_64__
        #define ASTC_ARCH_X64
    #elif defined(__aarch64__)
        #define ASTC_ARCH_ARM64
    #endif
#else
    #define ASTC_PLATFORM_UNKNOWN
    #define ASTC_ARCH_UNKNOWN
#endif

// Global platform compatibility state
static struct {
    ASTCPlatformInfo platform_info;
    ASTCCompatibilityConfig config;
    bool initialized;
} g_compat_state = {0};

// Initialize platform compatibility system
int astc_platform_compat_init(void) {
    if (g_compat_state.initialized) {
        return 0;
    }

    memset(&g_compat_state, 0, sizeof(g_compat_state));

    // Detect current platform
    ASTCPlatformInfo* info = &g_compat_state.platform_info;

    // Set platform type
#ifdef ASTC_PLATFORM_WINDOWS
    info->platform = ASTC_PLATFORM_TYPE_WINDOWS;
    strncpy(info->platform_name, "Windows", sizeof(info->platform_name) - 1);
#elif defined(ASTC_PLATFORM_LINUX)
    info->platform = ASTC_PLATFORM_TYPE_LINUX;
    strncpy(info->platform_name, "Linux", sizeof(info->platform_name) - 1);
#elif defined(ASTC_PLATFORM_MACOS)
    info->platform = ASTC_PLATFORM_TYPE_MACOS;
    strncpy(info->platform_name, "macOS", sizeof(info->platform_name) - 1);
#else
    info->platform = ASTC_PLATFORM_TYPE_UNKNOWN;
    strncpy(info->platform_name, "Unknown", sizeof(info->platform_name) - 1);
#endif

    // Set architecture
#ifdef ASTC_ARCH_X64
    info->architecture = ASTC_ARCH_TYPE_X64;
    strncpy(info->arch_name, "x86_64", sizeof(info->arch_name) - 1);
    info->pointer_size = 8;
    info->is_64bit = true;
#elif defined(ASTC_ARCH_ARM64)
    info->architecture = ASTC_ARCH_TYPE_ARM64;
    strncpy(info->arch_name, "ARM64", sizeof(info->arch_name) - 1);
    info->pointer_size = 8;
    info->is_64bit = true;
#elif defined(ASTC_ARCH_ARM32)
    info->architecture = ASTC_ARCH_TYPE_ARM32;
    strncpy(info->arch_name, "ARM32", sizeof(info->arch_name) - 1);
    info->pointer_size = 4;
    info->is_64bit = false;
#else
    info->architecture = ASTC_ARCH_TYPE_X86;
    strncpy(info->arch_name, "x86", sizeof(info->arch_name) - 1);
    info->pointer_size = 4;
    info->is_64bit = false;
#endif

    // Set endianness (most modern platforms are little-endian)
    info->endianness = ASTC_ENDIAN_LITTLE;

    // Set default compatibility configuration
    ASTCCompatibilityConfig* config = &g_compat_state.config;
    config->enable_type_size_validation = true;
    config->enable_endian_conversion = true;
    config->enable_path_normalization = true;
    config->enable_module_path_resolution = true;
    config->strict_abi_compatibility = false;

    g_compat_state.initialized = true;

    LOG_RUNTIME_INFO("Platform compatibility initialized: %s %s (%d-bit)",
                    info->platform_name, info->arch_name, info->is_64bit ? 64 : 32);
    return 0;
}

// Cleanup platform compatibility system
void astc_platform_compat_cleanup(void) {
    if (!g_compat_state.initialized) {
        return;
    }

    memset(&g_compat_state, 0, sizeof(g_compat_state));
    LOG_RUNTIME_INFO("Platform compatibility system cleaned up");
}

// Get current platform information
const ASTCPlatformInfo* astc_get_platform_info(void) {
    if (!g_compat_state.initialized) {
        astc_platform_compat_init();
    }
    return &g_compat_state.platform_info;
}

// Check if ASTC program is compatible with current platform
bool astc_is_program_compatible(const ASTCProgramHeader* program_header) {
    if (!program_header) {
        return false;
    }

    const ASTCPlatformInfo* info = astc_get_platform_info();

    // Check if program supports current platform
    bool platform_supported = false;
    for (int i = 0; i < program_header->supported_platform_count; i++) {
        if (program_header->supported_platforms[i] == info->platform ||
            program_header->supported_platforms[i] == ASTC_PLATFORM_TYPE_ANY) {
            platform_supported = true;
            break;
        }
    }

    if (!platform_supported) {
        LOG_RUNTIME_WARN("Program does not support current platform: %s", info->platform_name);
        return false;
    }

    // Check if program supports current architecture
    bool arch_supported = false;
    for (int i = 0; i < program_header->supported_arch_count; i++) {
        if (program_header->supported_architectures[i] == info->architecture ||
            program_header->supported_architectures[i] == ASTC_ARCH_TYPE_ANY) {
            arch_supported = true;
            break;
        }
    }

    if (!arch_supported) {
        LOG_RUNTIME_WARN("Program does not support current architecture: %s", info->arch_name);
        return false;
    }

    // Check minimum requirements
    if (program_header->min_pointer_size > info->pointer_size) {
        LOG_RUNTIME_WARN("Program requires %d-byte pointers, current platform has %d-byte pointers",
                        program_header->min_pointer_size, info->pointer_size);
        return false;
    }

    LOG_RUNTIME_DEBUG("Program is compatible with current platform");
    return true;
}

// Normalize file path for current platform
int astc_normalize_path(const char* input_path, char* output_path, size_t output_size) {
    if (!input_path || !output_path || output_size == 0) {
        return -1;
    }

    if (!g_compat_state.config.enable_path_normalization) {
        strncpy(output_path, input_path, output_size - 1);
        output_path[output_size - 1] = '\0';
        return 0;
    }

    size_t input_len = strlen(input_path);
    if (input_len >= output_size) {
        return -1; // Path too long
    }

    // Copy and normalize path separators
    for (size_t i = 0; i < input_len && i < output_size - 1; i++) {
        char c = input_path[i];
        
#ifdef ASTC_PLATFORM_WINDOWS
        // Convert forward slashes to backslashes on Windows
        if (c == '/') {
            output_path[i] = '\\';
        } else {
            output_path[i] = c;
        }
#else
        // Convert backslashes to forward slashes on Unix-like systems
        if (c == '\\') {
            output_path[i] = '/';
        } else {
            output_path[i] = c;
        }
#endif
    }

    output_path[input_len] = '\0';
    return 0;
}

// Resolve module path for current platform
int astc_resolve_module_path(const char* module_name, char* resolved_path, size_t path_size) {
    if (!module_name || !resolved_path || path_size == 0) {
        return -1;
    }

    const ASTCPlatformInfo* info = astc_get_platform_info();

    // Build platform-specific module filename
    char module_filename[256];
    if (strstr(module_name, ".rt") != NULL) {
        // System runtime module
        snprintf(module_filename, sizeof(module_filename), "%s_%s_%d.native",
                module_name, info->arch_name, info->pointer_size * 8);
    } else {
        // User module
        snprintf(module_filename, sizeof(module_filename), "%s_%s_%d.native",
                module_name, info->arch_name, info->pointer_size * 8);
    }

    // Search in standard module directories
    const char* search_paths[] = {
        "./modules/",
        "./lib/",
        "/usr/local/lib/astc/",
        "/usr/lib/astc/",
        NULL
    };

    for (int i = 0; search_paths[i] != NULL; i++) {
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s%s", search_paths[i], module_filename);
        
        // Normalize path
        if (astc_normalize_path(full_path, resolved_path, path_size) == 0) {
            // Check if file exists (simplified check)
            FILE* file = fopen(resolved_path, "rb");
            if (file) {
                fclose(file);
                LOG_RUNTIME_DEBUG("Resolved module %s to %s", module_name, resolved_path);
                return 0;
            }
        }
    }

    LOG_RUNTIME_WARN("Could not resolve module path for: %s", module_name);
    return -1;
}

// Convert data between different endianness
int astc_convert_endianness(void* data, size_t size, ASTCEndianness from, ASTCEndianness to) {
    if (!data || size == 0 || from == to) {
        return 0;
    }

    if (!g_compat_state.config.enable_endian_conversion) {
        return 0; // Conversion disabled
    }

    // Simple byte swapping for common sizes
    uint8_t* bytes = (uint8_t*)data;
    
    switch (size) {
        case 2: {
            uint8_t temp = bytes[0];
            bytes[0] = bytes[1];
            bytes[1] = temp;
            break;
        }
        case 4: {
            uint8_t temp;
            temp = bytes[0]; bytes[0] = bytes[3]; bytes[3] = temp;
            temp = bytes[1]; bytes[1] = bytes[2]; bytes[2] = temp;
            break;
        }
        case 8: {
            uint8_t temp;
            temp = bytes[0]; bytes[0] = bytes[7]; bytes[7] = temp;
            temp = bytes[1]; bytes[1] = bytes[6]; bytes[6] = temp;
            temp = bytes[2]; bytes[2] = bytes[5]; bytes[5] = temp;
            temp = bytes[3]; bytes[3] = bytes[4]; bytes[4] = temp;
            break;
        }
        default:
            // For other sizes, swap byte by byte
            for (size_t i = 0; i < size / 2; i++) {
                uint8_t temp = bytes[i];
                bytes[i] = bytes[size - 1 - i];
                bytes[size - 1 - i] = temp;
            }
            break;
    }

    return 0;
}

// Validate type sizes for compatibility
bool astc_validate_type_sizes(const ASTCTypeInfo* type_info) {
    if (!type_info) {
        return false;
    }

    if (!g_compat_state.config.enable_type_size_validation) {
        return true; // Validation disabled
    }

    const ASTCPlatformInfo* info = astc_get_platform_info();

    // Check pointer size compatibility
    if (type_info->pointer_size != info->pointer_size) {
        LOG_RUNTIME_WARN("Type size mismatch: expected pointer size %d, got %d",
                        info->pointer_size, type_info->pointer_size);
        return false;
    }

    // Validate basic type sizes
    if (type_info->int_size != 4 || type_info->long_size < 4 || type_info->long_size > 8) {
        LOG_RUNTIME_WARN("Invalid basic type sizes in type info");
        return false;
    }

    return true;
}

// Get platform-specific module search paths
int astc_get_module_search_paths(char paths[][256], int max_paths) {
    if (!paths || max_paths <= 0) {
        return -1;
    }

    int path_count = 0;

    // Add current directory
    if (path_count < max_paths) {
        strncpy(paths[path_count], "./modules/", 255);
        paths[path_count][255] = '\0';
        path_count++;
    }

    // Add platform-specific system paths
#ifdef ASTC_PLATFORM_WINDOWS
    if (path_count < max_paths) {
        strncpy(paths[path_count], "C:\\Program Files\\ASTC\\modules\\", 255);
        paths[path_count][255] = '\0';
        path_count++;
    }
    if (path_count < max_paths) {
        strncpy(paths[path_count], "C:\\ASTC\\lib\\", 255);
        paths[path_count][255] = '\0';
        path_count++;
    }
#else
    if (path_count < max_paths) {
        strncpy(paths[path_count], "/usr/local/lib/astc/", 255);
        paths[path_count][255] = '\0';
        path_count++;
    }
    if (path_count < max_paths) {
        strncpy(paths[path_count], "/usr/lib/astc/", 255);
        paths[path_count][255] = '\0';
        path_count++;
    }
    if (path_count < max_paths) {
        strncpy(paths[path_count], "/opt/astc/lib/", 255);
        paths[path_count][255] = '\0';
        path_count++;
    }
#endif

    return path_count;
}

// Set compatibility configuration
int astc_set_compatibility_config(const ASTCCompatibilityConfig* config) {
    if (!config) {
        return -1;
    }

    g_compat_state.config = *config;
    LOG_RUNTIME_DEBUG("Compatibility configuration updated");
    return 0;
}

// Get current compatibility configuration
const ASTCCompatibilityConfig* astc_get_compatibility_config(void) {
    return &g_compat_state.config;
}
