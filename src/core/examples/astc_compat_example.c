/**
 * astc_compat_example.c - ASTC Cross-Platform Compatibility Example
 * 
 * Demonstrates how ASTC programs achieve cross-platform compatibility
 * and "write once, run anywhere" capability.
 */

#include "../include/astc_platform_compat.h"
#include "../include/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test platform detection
void test_platform_detection(void) {
    LOG_RUNTIME_INFO("=== Testing Platform Detection ===");
    
    const ASTCPlatformInfo* info = astc_get_platform_info();
    
    LOG_RUNTIME_INFO("Current Platform Information:");
    LOG_RUNTIME_INFO("  Platform: %s (%d)", info->platform_name, info->platform);
    LOG_RUNTIME_INFO("  Architecture: %s (%d)", info->arch_name, info->architecture);
    LOG_RUNTIME_INFO("  Pointer Size: %d bytes", info->pointer_size);
    LOG_RUNTIME_INFO("  64-bit: %s", info->is_64bit ? "Yes" : "No");
    LOG_RUNTIME_INFO("  Endianness: %s", info->endianness == ASTC_ENDIAN_LITTLE ? "Little" : "Big");
    
    // Test utility macros
    LOG_RUNTIME_INFO("Platform Detection Macros:");
    LOG_RUNTIME_INFO("  ASTC_IS_WINDOWS(): %s", ASTC_IS_WINDOWS() ? "true" : "false");
    LOG_RUNTIME_INFO("  ASTC_IS_LINUX(): %s", ASTC_IS_LINUX() ? "true" : "false");
    LOG_RUNTIME_INFO("  ASTC_IS_MACOS(): %s", ASTC_IS_MACOS() ? "true" : "false");
    LOG_RUNTIME_INFO("  ASTC_IS_64BIT(): %s", ASTC_IS_64BIT() ? "true" : "false");
    LOG_RUNTIME_INFO("  ASTC_IS_X64(): %s", ASTC_IS_X64() ? "true" : "false");
    LOG_RUNTIME_INFO("  ASTC_IS_ARM64(): %s", ASTC_IS_ARM64() ? "true" : "false");
    LOG_RUNTIME_INFO("  ASTC_POINTER_SIZE(): %d", ASTC_POINTER_SIZE());
}

// Test program compatibility checking
void test_program_compatibility(void) {
    LOG_RUNTIME_INFO("=== Testing Program Compatibility ===");
    
    // Create a test program header
    ASTCProgramHeader header = {0};
    header.magic = 0x43545341; // "ASTC"
    header.version = 1;
    
    // Test 1: Universal compatibility
    LOG_RUNTIME_INFO("Test 1: Universal compatibility");
    header.supported_platforms[0] = ASTC_PLATFORM_TYPE_ANY;
    header.supported_platform_count = 1;
    header.supported_architectures[0] = ASTC_ARCH_TYPE_ANY;
    header.supported_arch_count = 1;
    header.min_pointer_size = 4;
    
    bool compatible = astc_is_program_compatible(&header);
    LOG_RUNTIME_INFO("  Universal program compatible: %s", compatible ? "Yes" : "No");
    
    // Test 2: Platform-specific compatibility
    LOG_RUNTIME_INFO("Test 2: Platform-specific compatibility");
    header.supported_platforms[0] = ASTC_PLATFORM_TYPE_WINDOWS;
    header.supported_platforms[1] = ASTC_PLATFORM_TYPE_LINUX;
    header.supported_platforms[2] = ASTC_PLATFORM_TYPE_MACOS;
    header.supported_platform_count = 3;
    header.supported_architectures[0] = ASTC_ARCH_TYPE_X64;
    header.supported_architectures[1] = ASTC_ARCH_TYPE_ARM64;
    header.supported_arch_count = 2;
    
    compatible = astc_is_program_compatible(&header);
    LOG_RUNTIME_INFO("  Multi-platform program compatible: %s", compatible ? "Yes" : "No");
    
    // Test 3: Incompatible program
    LOG_RUNTIME_INFO("Test 3: Incompatible program");
    header.supported_platforms[0] = ASTC_PLATFORM_TYPE_FREEBSD;
    header.supported_platform_count = 1;
    header.supported_architectures[0] = ASTC_ARCH_TYPE_RISCV64;
    header.supported_arch_count = 1;
    header.min_pointer_size = 16; // Unrealistic requirement
    
    compatible = astc_is_program_compatible(&header);
    LOG_RUNTIME_INFO("  Incompatible program compatible: %s", compatible ? "Yes" : "No");
}

// Test path normalization
void test_path_normalization(void) {
    LOG_RUNTIME_INFO("=== Testing Path Normalization ===");
    
    const char* test_paths[] = {
        "modules/libc.rt",
        "modules\\math.rt",
        "/usr/local/lib/astc/string.rt",
        "C:\\Program Files\\ASTC\\modules\\io.rt",
        "./relative/path/module.rt",
        "../parent/module.rt"
    };
    
    int num_paths = sizeof(test_paths) / sizeof(test_paths[0]);
    
    for (int i = 0; i < num_paths; i++) {
        char normalized[512];
        if (astc_normalize_path(test_paths[i], normalized, sizeof(normalized)) == 0) {
            LOG_RUNTIME_INFO("  '%s' -> '%s'", test_paths[i], normalized);
        } else {
            LOG_RUNTIME_ERROR("  Failed to normalize: '%s'", test_paths[i]);
        }
    }
}

// Test module path resolution
void test_module_path_resolution(void) {
    LOG_RUNTIME_INFO("=== Testing Module Path Resolution ===");
    
    const char* test_modules[] = {
        "libc.rt",
        "math.rt",
        "io.rt",
        "user_module",
        "graphics_engine"
    };
    
    int num_modules = sizeof(test_modules) / sizeof(test_modules[0]);
    
    for (int i = 0; i < num_modules; i++) {
        char resolved_path[512];
        if (astc_resolve_module_path(test_modules[i], resolved_path, sizeof(resolved_path)) == 0) {
            LOG_RUNTIME_INFO("  '%s' -> '%s'", test_modules[i], resolved_path);
        } else {
            LOG_RUNTIME_WARN("  Could not resolve: '%s'", test_modules[i]);
        }
    }
}

// Test endianness conversion
void test_endianness_conversion(void) {
    LOG_RUNTIME_INFO("=== Testing Endianness Conversion ===");
    
    // Test 32-bit integer conversion
    uint32_t test_int32 = 0x12345678;
    uint32_t original_int32 = test_int32;
    
    LOG_RUNTIME_INFO("Original 32-bit value: 0x%08X", test_int32);
    
    if (astc_convert_endianness(&test_int32, sizeof(test_int32), 
                               ASTC_ENDIAN_LITTLE, ASTC_ENDIAN_BIG) == 0) {
        LOG_RUNTIME_INFO("After endian conversion: 0x%08X", test_int32);
        
        // Convert back
        astc_convert_endianness(&test_int32, sizeof(test_int32), 
                               ASTC_ENDIAN_BIG, ASTC_ENDIAN_LITTLE);
        LOG_RUNTIME_INFO("After converting back: 0x%08X", test_int32);
        
        if (test_int32 == original_int32) {
            LOG_RUNTIME_INFO("Endianness conversion test: PASSED");
        } else {
            LOG_RUNTIME_ERROR("Endianness conversion test: FAILED");
        }
    }
    
    // Test 64-bit integer conversion
    uint64_t test_int64 = 0x123456789ABCDEF0ULL;
    uint64_t original_int64 = test_int64;
    
    LOG_RUNTIME_INFO("Original 64-bit value: 0x%016llX", test_int64);
    
    if (astc_convert_endianness(&test_int64, sizeof(test_int64), 
                               ASTC_ENDIAN_LITTLE, ASTC_ENDIAN_BIG) == 0) {
        LOG_RUNTIME_INFO("After endian conversion: 0x%016llX", test_int64);
        
        // Convert back
        astc_convert_endianness(&test_int64, sizeof(test_int64), 
                               ASTC_ENDIAN_BIG, ASTC_ENDIAN_LITTLE);
        
        if (test_int64 == original_int64) {
            LOG_RUNTIME_INFO("64-bit endianness conversion test: PASSED");
        } else {
            LOG_RUNTIME_ERROR("64-bit endianness conversion test: FAILED");
        }
    }
}

// Test type size validation
void test_type_size_validation(void) {
    LOG_RUNTIME_INFO("=== Testing Type Size Validation ===");
    
    const ASTCPlatformInfo* info = astc_get_platform_info();
    
    // Test compatible type info
    ASTCTypeInfo compatible_types = {
        .char_size = 1,
        .short_size = 2,
        .int_size = 4,
        .long_size = info->is_64bit ? 8 : 4,
        .long_long_size = 8,
        .float_size = 4,
        .double_size = 8,
        .pointer_size = info->pointer_size,
        .size_t_size = info->pointer_size
    };
    
    bool valid = astc_validate_type_sizes(&compatible_types);
    LOG_RUNTIME_INFO("Compatible type sizes valid: %s", valid ? "Yes" : "No");
    
    // Test incompatible type info
    ASTCTypeInfo incompatible_types = compatible_types;
    incompatible_types.pointer_size = info->pointer_size == 8 ? 4 : 8; // Wrong pointer size
    
    valid = astc_validate_type_sizes(&incompatible_types);
    LOG_RUNTIME_INFO("Incompatible type sizes valid: %s", valid ? "Yes" : "No");
}

// Test module search paths
void test_module_search_paths(void) {
    LOG_RUNTIME_INFO("=== Testing Module Search Paths ===");
    
    char paths[16][256];
    int path_count = astc_get_module_search_paths(paths, 16);
    
    if (path_count > 0) {
        LOG_RUNTIME_INFO("Module search paths (%d):", path_count);
        for (int i = 0; i < path_count; i++) {
            LOG_RUNTIME_INFO("  %d: %s", i + 1, paths[i]);
        }
    } else {
        LOG_RUNTIME_WARN("No module search paths found");
    }
}

// Test compatibility configuration
void test_compatibility_config(void) {
    LOG_RUNTIME_INFO("=== Testing Compatibility Configuration ===");
    
    const ASTCCompatibilityConfig* current_config = astc_get_compatibility_config();
    
    LOG_RUNTIME_INFO("Current configuration:");
    LOG_RUNTIME_INFO("  Type size validation: %s", current_config->enable_type_size_validation ? "Enabled" : "Disabled");
    LOG_RUNTIME_INFO("  Endian conversion: %s", current_config->enable_endian_conversion ? "Enabled" : "Disabled");
    LOG_RUNTIME_INFO("  Path normalization: %s", current_config->enable_path_normalization ? "Enabled" : "Disabled");
    LOG_RUNTIME_INFO("  Module path resolution: %s", current_config->enable_module_path_resolution ? "Enabled" : "Disabled");
    LOG_RUNTIME_INFO("  Strict ABI compatibility: %s", current_config->strict_abi_compatibility ? "Enabled" : "Disabled");
    
    // Test changing configuration
    ASTCCompatibilityConfig new_config = *current_config;
    new_config.strict_abi_compatibility = true;
    new_config.allow_unsafe_casts = false;
    
    if (astc_set_compatibility_config(&new_config) == 0) {
        LOG_RUNTIME_INFO("Configuration updated successfully");
        
        // Restore original configuration
        astc_set_compatibility_config(current_config);
        LOG_RUNTIME_INFO("Configuration restored");
    }
}

// Main example function
int astc_compat_example_main(void) {
    LOG_RUNTIME_INFO("=== ASTC Cross-Platform Compatibility Example ===");
    
    // Initialize logger
    if (logger_init() != 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        return -1;
    }
    
    // Initialize platform compatibility system
    if (astc_platform_compat_init() != 0) {
        LOG_RUNTIME_ERROR("Failed to initialize platform compatibility system");
        return -1;
    }
    
    // Run tests
    test_platform_detection();
    test_program_compatibility();
    test_path_normalization();
    test_module_path_resolution();
    test_endianness_conversion();
    test_type_size_validation();
    test_module_search_paths();
    test_compatibility_config();
    
    // Cleanup
    astc_platform_compat_cleanup();
    logger_cleanup();
    
    LOG_RUNTIME_INFO("ASTC cross-platform compatibility example completed successfully");
    return 0;
}

// Entry point for standalone testing
#ifdef ASTC_COMPAT_EXAMPLE_STANDALONE
int main(void) {
    return astc_compat_example_main();
}
#endif
