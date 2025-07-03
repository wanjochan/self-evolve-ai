/**
 * astc_platform_compat.h - ASTC Cross-Platform Compatibility Layer
 * 
 * Header for cross-platform compatibility system for ASTC bytecode
 */

#ifndef ASTC_PLATFORM_COMPAT_H
#define ASTC_PLATFORM_COMPAT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Maximum number of supported platforms/architectures per program
#define ASTC_MAX_SUPPORTED_PLATFORMS 8
#define ASTC_MAX_SUPPORTED_ARCHITECTURES 8

// Platform types
typedef enum {
    ASTC_PLATFORM_TYPE_UNKNOWN = 0,
    ASTC_PLATFORM_TYPE_WINDOWS = 1,
    ASTC_PLATFORM_TYPE_LINUX = 2,
    ASTC_PLATFORM_TYPE_MACOS = 3,
    ASTC_PLATFORM_TYPE_FREEBSD = 4,
    ASTC_PLATFORM_TYPE_ANY = 255
} ASTCPlatformType;

// Architecture types
typedef enum {
    ASTC_ARCH_TYPE_UNKNOWN = 0,
    ASTC_ARCH_TYPE_X86 = 1,
    ASTC_ARCH_TYPE_X64 = 2,
    ASTC_ARCH_TYPE_ARM32 = 3,
    ASTC_ARCH_TYPE_ARM64 = 4,
    ASTC_ARCH_TYPE_RISCV32 = 5,
    ASTC_ARCH_TYPE_RISCV64 = 6,
    ASTC_ARCH_TYPE_ANY = 255
} ASTCArchitectureType;

// Endianness types
typedef enum {
    ASTC_ENDIAN_UNKNOWN = 0,
    ASTC_ENDIAN_LITTLE = 1,
    ASTC_ENDIAN_BIG = 2
} ASTCEndianness;

// Platform information
typedef struct {
    ASTCPlatformType platform;
    ASTCArchitectureType architecture;
    ASTCEndianness endianness;
    char platform_name[32];
    char arch_name[32];
    int pointer_size;           // Size of pointers in bytes
    bool is_64bit;             // True if 64-bit platform
    int page_size;             // Memory page size
    int cache_line_size;       // CPU cache line size
} ASTCPlatformInfo;

// Type size information for compatibility checking
typedef struct {
    int char_size;             // sizeof(char)
    int short_size;            // sizeof(short)
    int int_size;              // sizeof(int)
    int long_size;             // sizeof(long)
    int long_long_size;        // sizeof(long long)
    int float_size;            // sizeof(float)
    int double_size;           // sizeof(double)
    int pointer_size;          // sizeof(void*)
    int size_t_size;           // sizeof(size_t)
} ASTCTypeInfo;

// Program header with platform compatibility information
typedef struct {
    uint32_t magic;            // Magic number
    uint32_t version;          // ASTC format version
    ASTCPlatformType supported_platforms[ASTC_MAX_SUPPORTED_PLATFORMS];
    int supported_platform_count;
    ASTCArchitectureType supported_architectures[ASTC_MAX_SUPPORTED_ARCHITECTURES];
    int supported_arch_count;
    int min_pointer_size;      // Minimum required pointer size
    ASTCEndianness target_endianness;
    ASTCTypeInfo type_info;    // Type size requirements
    uint32_t flags;            // Compatibility flags
} ASTCProgramHeader;

// Compatibility configuration
typedef struct {
    bool enable_type_size_validation;    // Validate type sizes
    bool enable_endian_conversion;       // Enable endianness conversion
    bool enable_path_normalization;      // Normalize file paths
    bool enable_module_path_resolution;  // Resolve module paths
    bool strict_abi_compatibility;       // Strict ABI compatibility checking
    bool allow_unsafe_casts;             // Allow potentially unsafe type casts
} ASTCCompatibilityConfig;

// Platform compatibility functions

/**
 * Initialize platform compatibility system
 * @return 0 on success, -1 on error
 */
int astc_platform_compat_init(void);

/**
 * Cleanup platform compatibility system
 */
void astc_platform_compat_cleanup(void);

/**
 * Get current platform information
 * @return Pointer to platform information structure
 */
const ASTCPlatformInfo* astc_get_platform_info(void);

/**
 * Check if ASTC program is compatible with current platform
 * @param program_header Program header with compatibility information
 * @return true if compatible, false otherwise
 */
bool astc_is_program_compatible(const ASTCProgramHeader* program_header);

/**
 * Normalize file path for current platform
 * @param input_path Input path to normalize
 * @param output_path Buffer for normalized path
 * @param output_size Size of output buffer
 * @return 0 on success, -1 on error
 */
int astc_normalize_path(const char* input_path, char* output_path, size_t output_size);

/**
 * Resolve module path for current platform
 * @param module_name Name of the module
 * @param resolved_path Buffer for resolved path
 * @param path_size Size of path buffer
 * @return 0 on success, -1 on error
 */
int astc_resolve_module_path(const char* module_name, char* resolved_path, size_t path_size);

/**
 * Convert data between different endianness
 * @param data Pointer to data to convert
 * @param size Size of data in bytes
 * @param from Source endianness
 * @param to Target endianness
 * @return 0 on success, -1 on error
 */
int astc_convert_endianness(void* data, size_t size, ASTCEndianness from, ASTCEndianness to);

/**
 * Validate type sizes for compatibility
 * @param type_info Type information to validate
 * @return true if compatible, false otherwise
 */
bool astc_validate_type_sizes(const ASTCTypeInfo* type_info);

/**
 * Get platform-specific module search paths
 * @param paths Array to store search paths
 * @param max_paths Maximum number of paths to return
 * @return Number of paths found, -1 on error
 */
int astc_get_module_search_paths(char paths[][256], int max_paths);

/**
 * Set compatibility configuration
 * @param config Compatibility configuration
 * @return 0 on success, -1 on error
 */
int astc_set_compatibility_config(const ASTCCompatibilityConfig* config);

/**
 * Get current compatibility configuration
 * @return Pointer to current configuration
 */
const ASTCCompatibilityConfig* astc_get_compatibility_config(void);

// Utility macros for platform detection

#define ASTC_IS_WINDOWS() (astc_get_platform_info()->platform == ASTC_PLATFORM_TYPE_WINDOWS)
#define ASTC_IS_LINUX() (astc_get_platform_info()->platform == ASTC_PLATFORM_TYPE_LINUX)
#define ASTC_IS_MACOS() (astc_get_platform_info()->platform == ASTC_PLATFORM_TYPE_MACOS)
#define ASTC_IS_64BIT() (astc_get_platform_info()->is_64bit)
#define ASTC_IS_X64() (astc_get_platform_info()->architecture == ASTC_ARCH_TYPE_X64)
#define ASTC_IS_ARM64() (astc_get_platform_info()->architecture == ASTC_ARCH_TYPE_ARM64)
#define ASTC_POINTER_SIZE() (astc_get_platform_info()->pointer_size)

// Convenience functions for common operations

/**
 * Create a platform-compatible program header
 * @param header Header structure to fill
 * @param target_platforms Array of target platforms
 * @param platform_count Number of target platforms
 * @param target_architectures Array of target architectures
 * @param arch_count Number of target architectures
 * @return 0 on success, -1 on error
 */
int astc_create_program_header(ASTCProgramHeader* header,
                              const ASTCPlatformType* target_platforms, int platform_count,
                              const ASTCArchitectureType* target_architectures, int arch_count);

/**
 * Check if two platforms are binary compatible
 * @param platform1 First platform
 * @param platform2 Second platform
 * @return true if binary compatible, false otherwise
 */
bool astc_are_platforms_binary_compatible(const ASTCPlatformInfo* platform1,
                                         const ASTCPlatformInfo* platform2);

/**
 * Get platform-specific file extension for modules
 * @param platform Target platform
 * @return File extension string (e.g., ".dll", ".so", ".dylib")
 */
const char* astc_get_module_extension(ASTCPlatformType platform);

/**
 * Convert platform type to string
 * @param platform Platform type
 * @return Platform name string
 */
const char* astc_platform_type_to_string(ASTCPlatformType platform);

/**
 * Convert architecture type to string
 * @param architecture Architecture type
 * @return Architecture name string
 */
const char* astc_architecture_type_to_string(ASTCArchitectureType architecture);

/**
 * Parse platform type from string
 * @param platform_string Platform name string
 * @return Platform type, or ASTC_PLATFORM_TYPE_UNKNOWN if not recognized
 */
ASTCPlatformType astc_parse_platform_type(const char* platform_string);

/**
 * Parse architecture type from string
 * @param arch_string Architecture name string
 * @return Architecture type, or ASTC_ARCH_TYPE_UNKNOWN if not recognized
 */
ASTCArchitectureType astc_parse_architecture_type(const char* arch_string);

// Error codes specific to platform compatibility
#define ASTC_COMPAT_SUCCESS           0
#define ASTC_COMPAT_ERROR_INVALID     -1
#define ASTC_COMPAT_ERROR_UNSUPPORTED -2
#define ASTC_COMPAT_ERROR_TYPE_MISMATCH -3
#define ASTC_COMPAT_ERROR_ENDIAN_MISMATCH -4
#define ASTC_COMPAT_ERROR_ABI_MISMATCH -5

#ifdef __cplusplus
}
#endif

#endif // ASTC_PLATFORM_COMPAT_H
