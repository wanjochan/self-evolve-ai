/**
 * c99_target.h - C99 Cross-Platform Target Support
 * 
 * Cross-platform compilation support for C99 compiler including
 * target architecture detection, ABI handling, and platform-specific optimizations.
 */

#ifndef C99_TARGET_H
#define C99_TARGET_H

#include "../../core/astc.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// Target Architecture Types
// ===============================================

typedef enum {
    TARGET_ARCH_UNKNOWN = 0,
    TARGET_ARCH_X86_32,         // x86 32-bit
    TARGET_ARCH_X86_64,         // x86 64-bit (AMD64)
    TARGET_ARCH_ARM32,          // ARM 32-bit
    TARGET_ARCH_ARM64,          // ARM 64-bit (AArch64)
    TARGET_ARCH_RISCV32,        // RISC-V 32-bit
    TARGET_ARCH_RISCV64,        // RISC-V 64-bit
    TARGET_ARCH_MIPS32,         // MIPS 32-bit
    TARGET_ARCH_MIPS64,         // MIPS 64-bit
    TARGET_ARCH_WASM32,         // WebAssembly 32-bit
    TARGET_ARCH_WASM64,         // WebAssembly 64-bit
    TARGET_ARCH_COUNT
} TargetArchitecture;

// ===============================================
// Target Operating System
// ===============================================

typedef enum {
    TARGET_OS_UNKNOWN = 0,
    TARGET_OS_WINDOWS,          // Microsoft Windows
    TARGET_OS_LINUX,            // Linux
    TARGET_OS_MACOS,            // macOS
    TARGET_OS_FREEBSD,          // FreeBSD
    TARGET_OS_ANDROID,          // Android
    TARGET_OS_IOS,              // iOS
    TARGET_OS_WASM,             // WebAssembly
    TARGET_OS_BARE_METAL,       // Bare metal / embedded
    TARGET_OS_COUNT
} TargetOperatingSystem;

// ===============================================
// Target ABI (Application Binary Interface)
// ===============================================

typedef enum {
    TARGET_ABI_UNKNOWN = 0,
    TARGET_ABI_SYSV,            // System V ABI (Linux, Unix)
    TARGET_ABI_WIN64,           // Windows x64 ABI
    TARGET_ABI_AAPCS,           // ARM AAPCS
    TARGET_ABI_AAPCS64,         // ARM AAPCS64
    TARGET_ABI_RISCV,           // RISC-V ABI
    TARGET_ABI_WASM,            // WebAssembly ABI
    TARGET_ABI_COUNT
} TargetABI;

// ===============================================
// Target Information Structure
// ===============================================

typedef struct {
    TargetArchitecture arch;            // Target architecture
    TargetOperatingSystem os;           // Target operating system
    TargetABI abi;                      // Target ABI
    
    // Architecture properties
    int pointer_size;                   // Pointer size in bytes
    int word_size;                      // Word size in bytes
    int max_alignment;                  // Maximum alignment requirement
    bool has_fpu;                       // Has floating-point unit
    bool has_vector_unit;               // Has vector/SIMD unit
    
    // Type sizes (in bytes)
    int sizeof_char;                    // sizeof(char)
    int sizeof_short;                   // sizeof(short)
    int sizeof_int;                     // sizeof(int)
    int sizeof_long;                    // sizeof(long)
    int sizeof_long_long;               // sizeof(long long)
    int sizeof_float;                   // sizeof(float)
    int sizeof_double;                  // sizeof(double)
    int sizeof_long_double;             // sizeof(long double)
    int sizeof_pointer;                 // sizeof(void*)
    
    // Alignment requirements (in bytes)
    int align_char;                     // Alignment of char
    int align_short;                    // Alignment of short
    int align_int;                      // Alignment of int
    int align_long;                     // Alignment of long
    int align_long_long;                // Alignment of long long
    int align_float;                    // Alignment of float
    int align_double;                   // Alignment of double
    int align_long_double;              // Alignment of long double
    int align_pointer;                  // Alignment of pointer
    
    // Calling convention
    int max_register_args;              // Max arguments passed in registers
    bool caller_saves_registers;        // Caller saves registers
    bool has_red_zone;                  // Has red zone (x86-64)
    
    // Target-specific features
    char* cpu_features;                 // CPU feature string
    char* target_triple;                // Target triple string
    char* target_description;           // Human-readable description
} TargetInfo;

// ===============================================
// Target Context
// ===============================================

typedef struct {
    TargetInfo* current_target;         // Current compilation target
    TargetInfo* host_target;            // Host system target
    
    // Cross-compilation settings
    bool is_cross_compiling;            // Is cross-compiling
    char* sysroot;                      // System root for cross-compilation
    char* toolchain_prefix;             // Toolchain prefix
    
    // Target-specific options
    bool optimize_for_size;             // Optimize for size vs speed
    bool enable_pic;                    // Enable position-independent code
    bool enable_pie;                    // Enable position-independent executable
    int stack_alignment;                // Required stack alignment
    
    // Error handling
    char error_message[512];
    bool has_error;
} TargetContext;

// ===============================================
// Target Management Functions
// ===============================================

/**
 * Create target context
 */
TargetContext* target_create(void);

/**
 * Destroy target context
 */
void target_destroy(TargetContext* target);

/**
 * Set target from triple string (e.g., "x86_64-pc-linux-gnu")
 */
bool target_set_from_triple(TargetContext* target, const char* triple);

/**
 * Set target from architecture and OS
 */
bool target_set_from_arch_os(TargetContext* target, TargetArchitecture arch, TargetOperatingSystem os);

/**
 * Get host target information
 */
TargetInfo* target_get_host_info(void);

/**
 * Get current target information
 */
TargetInfo* target_get_current_info(TargetContext* target);

/**
 * Check if target is supported
 */
bool target_is_supported(TargetArchitecture arch, TargetOperatingSystem os);

// ===============================================
// Target Information Functions
// ===============================================

/**
 * Get architecture name
 */
const char* target_get_arch_name(TargetArchitecture arch);

/**
 * Get OS name
 */
const char* target_get_os_name(TargetOperatingSystem os);

/**
 * Get ABI name
 */
const char* target_get_abi_name(TargetABI abi);

/**
 * Parse architecture from string
 */
TargetArchitecture target_parse_arch(const char* arch_str);

/**
 * Parse OS from string
 */
TargetOperatingSystem target_parse_os(const char* os_str);

// ===============================================
// Type Layout Functions
// ===============================================

/**
 * Get type size for target
 */
int target_get_type_size(TargetInfo* target, struct ASTNode* type);

/**
 * Get type alignment for target
 */
int target_get_type_alignment(TargetInfo* target, struct ASTNode* type);

/**
 * Get pointer size for target
 */
int target_get_pointer_size(TargetInfo* target);

// ===============================================
// Utility Functions
// ===============================================

/**
 * Print target information
 */
void target_print_info(TargetInfo* target);

/**
 * Get supported targets list
 */
const char** target_get_supported_list(size_t* count);

/**
 * Check target compatibility
 */
bool target_is_compatible(TargetInfo* target1, TargetInfo* target2);

/**
 * Get target error message
 */
const char* target_get_error(TargetContext* target);

// ===============================================
// Predefined Target Configurations
// ===============================================

/**
 * Get x86-64 Linux target
 */
TargetInfo* target_get_x86_64_linux(void);

/**
 * Get x86-64 Windows target
 */
TargetInfo* target_get_x86_64_windows(void);

/**
 * Get ARM64 Linux target
 */
TargetInfo* target_get_arm64_linux(void);

/**
 * Get WebAssembly target
 */
TargetInfo* target_get_wasm32(void);

#ifdef __cplusplus
}
#endif

#endif // C99_TARGET_H
