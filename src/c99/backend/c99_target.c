/**
 * c99_target.c - C99 Cross-Platform Target Support Implementation
 */

#include "c99_target.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Forward declarations
static void target_init_type_info(TargetInfo* info);

// ===============================================
// Target Information Tables
// ===============================================

static const char* arch_names[] = {
    "unknown", "i386", "x86_64", "arm", "aarch64", 
    "riscv32", "riscv64", "mips", "mips64", "wasm32", "wasm64"
};

static const char* os_names[] = {
    "unknown", "windows", "linux", "macos", "freebsd", 
    "android", "ios", "wasm", "bare-metal"
};

static const char* abi_names[] = {
    "unknown", "sysv", "win64", "aapcs", "aapcs64", "riscv", "wasm"
};

// ===============================================
// Target Context Management
// ===============================================

TargetContext* target_create(void) {
    TargetContext* target = malloc(sizeof(TargetContext));
    if (!target) return NULL;
    
    memset(target, 0, sizeof(TargetContext));
    
    // Initialize with host target
    target->host_target = target_get_host_info();
    target->current_target = target->host_target;
    target->is_cross_compiling = false;
    
    return target;
}

void target_destroy(TargetContext* target) {
    if (!target) return;
    
    if (target->current_target && target->current_target != target->host_target) {
        free(target->current_target);
    }
    
    if (target->host_target) {
        free(target->host_target);
    }
    
    if (target->sysroot) {
        free(target->sysroot);
    }
    
    if (target->toolchain_prefix) {
        free(target->toolchain_prefix);
    }
    
    free(target);
}

TargetInfo* target_get_host_info(void) {
    TargetInfo* info = malloc(sizeof(TargetInfo));
    if (!info) return NULL;
    
    memset(info, 0, sizeof(TargetInfo));
    
    // Detect host architecture
#if defined(_M_X64) || defined(__x86_64__)
    info->arch = TARGET_ARCH_X86_64;
    info->pointer_size = 8;
    info->word_size = 8;
#elif defined(_M_IX86) || defined(__i386__)
    info->arch = TARGET_ARCH_X86_32;
    info->pointer_size = 4;
    info->word_size = 4;
#elif defined(_M_ARM64) || defined(__aarch64__)
    info->arch = TARGET_ARCH_ARM64;
    info->pointer_size = 8;
    info->word_size = 8;
#elif defined(_M_ARM) || defined(__arm__)
    info->arch = TARGET_ARCH_ARM32;
    info->pointer_size = 4;
    info->word_size = 4;
#else
    info->arch = TARGET_ARCH_UNKNOWN;
    info->pointer_size = sizeof(void*);
    info->word_size = sizeof(void*);
#endif

    // Detect host OS
#if defined(_WIN32)
    info->os = TARGET_OS_WINDOWS;
    info->abi = (info->arch == TARGET_ARCH_X86_64) ? TARGET_ABI_WIN64 : TARGET_ABI_UNKNOWN;
#elif defined(__linux__)
    info->os = TARGET_OS_LINUX;
    info->abi = TARGET_ABI_SYSV;
#elif defined(__APPLE__)
    info->os = TARGET_OS_MACOS;
    info->abi = TARGET_ABI_SYSV;
#elif defined(__FreeBSD__)
    info->os = TARGET_OS_FREEBSD;
    info->abi = TARGET_ABI_SYSV;
#else
    info->os = TARGET_OS_UNKNOWN;
    info->abi = TARGET_ABI_UNKNOWN;
#endif

    // Initialize type sizes and alignments
    target_init_type_info(info);
    
    // Set target description
    info->target_description = malloc(256);
    if (info->target_description) {
        snprintf(info->target_description, 256, "%s-%s", 
                target_get_arch_name(info->arch), 
                target_get_os_name(info->os));
    }
    
    printf("Target: Detected host as %s\n", info->target_description);
    
    return info;
}

static void target_init_type_info(TargetInfo* info) {
    if (!info) return;
    
    // Set standard type sizes
    info->sizeof_char = 1;
    info->sizeof_short = 2;
    info->sizeof_int = 4;
    info->sizeof_float = 4;
    info->sizeof_double = 8;
    info->sizeof_pointer = info->pointer_size;
    
    // Architecture-specific sizes
    switch (info->arch) {
        case TARGET_ARCH_X86_32:
        case TARGET_ARCH_ARM32:
        case TARGET_ARCH_WASM32:
            info->sizeof_long = 4;
            info->sizeof_long_long = 8;
            info->sizeof_long_double = 12;
            info->max_alignment = 8;
            break;
            
        case TARGET_ARCH_X86_64:
        case TARGET_ARCH_ARM64:
        case TARGET_ARCH_WASM64:
            info->sizeof_long = 8;
            info->sizeof_long_long = 8;
            info->sizeof_long_double = 16;
            info->max_alignment = 16;
            break;
            
        default:
            info->sizeof_long = 8;
            info->sizeof_long_long = 8;
            info->sizeof_long_double = 16;
            info->max_alignment = 8;
            break;
    }
    
    // OS-specific adjustments
    if (info->os == TARGET_OS_WINDOWS && info->arch == TARGET_ARCH_X86_64) {
        info->sizeof_long = 4; // Windows LLP64 model
    }
    
    // Set alignment requirements
    info->align_char = 1;
    info->align_short = 2;
    info->align_int = 4;
    info->align_long = info->sizeof_long;
    info->align_long_long = info->sizeof_long_long;
    info->align_float = 4;
    info->align_double = 8;
    info->align_long_double = info->sizeof_long_double;
    info->align_pointer = info->sizeof_pointer;
    
    // Set calling convention properties
    switch (info->arch) {
        case TARGET_ARCH_X86_64:
            info->max_register_args = (info->os == TARGET_OS_WINDOWS) ? 4 : 6;
            info->caller_saves_registers = true;
            info->has_red_zone = (info->os != TARGET_OS_WINDOWS);
            break;
            
        case TARGET_ARCH_ARM64:
            info->max_register_args = 8;
            info->caller_saves_registers = false;
            info->has_red_zone = false;
            break;
            
        default:
            info->max_register_args = 0;
            info->caller_saves_registers = true;
            info->has_red_zone = false;
            break;
    }
    
    // Set hardware capabilities
    info->has_fpu = true;
    info->has_vector_unit = (info->arch == TARGET_ARCH_X86_64 || info->arch == TARGET_ARCH_ARM64);
}

// ===============================================
// Target Information Functions
// ===============================================

const char* target_get_arch_name(TargetArchitecture arch) {
    if (arch >= 0 && arch < TARGET_ARCH_COUNT) {
        return arch_names[arch];
    }
    return "unknown";
}

const char* target_get_os_name(TargetOperatingSystem os) {
    if (os >= 0 && os < TARGET_OS_COUNT) {
        return os_names[os];
    }
    return "unknown";
}

const char* target_get_abi_name(TargetABI abi) {
    if (abi >= 0 && abi < TARGET_ABI_COUNT) {
        return abi_names[abi];
    }
    return "unknown";
}

TargetArchitecture target_parse_arch(const char* arch_str) {
    if (!arch_str) return TARGET_ARCH_UNKNOWN;
    
    if (strcmp(arch_str, "i386") == 0 || strcmp(arch_str, "i686") == 0) {
        return TARGET_ARCH_X86_32;
    } else if (strcmp(arch_str, "x86_64") == 0 || strcmp(arch_str, "amd64") == 0) {
        return TARGET_ARCH_X86_64;
    } else if (strcmp(arch_str, "arm") == 0 || strcmp(arch_str, "armv7") == 0) {
        return TARGET_ARCH_ARM32;
    } else if (strcmp(arch_str, "aarch64") == 0 || strcmp(arch_str, "arm64") == 0) {
        return TARGET_ARCH_ARM64;
    } else if (strcmp(arch_str, "wasm32") == 0) {
        return TARGET_ARCH_WASM32;
    } else if (strcmp(arch_str, "wasm64") == 0) {
        return TARGET_ARCH_WASM64;
    }
    
    return TARGET_ARCH_UNKNOWN;
}

TargetOperatingSystem target_parse_os(const char* os_str) {
    if (!os_str) return TARGET_OS_UNKNOWN;
    
    if (strstr(os_str, "windows") || strstr(os_str, "win32") || strstr(os_str, "mingw")) {
        return TARGET_OS_WINDOWS;
    } else if (strstr(os_str, "linux")) {
        return TARGET_OS_LINUX;
    } else if (strstr(os_str, "darwin") || strstr(os_str, "macos")) {
        return TARGET_OS_MACOS;
    } else if (strstr(os_str, "freebsd")) {
        return TARGET_OS_FREEBSD;
    } else if (strstr(os_str, "android")) {
        return TARGET_OS_ANDROID;
    } else if (strstr(os_str, "wasm")) {
        return TARGET_OS_WASM;
    }
    
    return TARGET_OS_UNKNOWN;
}

bool target_is_supported(TargetArchitecture arch, TargetOperatingSystem os) {
    switch (arch) {
        case TARGET_ARCH_X86_64:
            return (os == TARGET_OS_WINDOWS || os == TARGET_OS_LINUX || os == TARGET_OS_MACOS);
        case TARGET_ARCH_X86_32:
            return (os == TARGET_OS_WINDOWS || os == TARGET_OS_LINUX);
        case TARGET_ARCH_ARM64:
            return (os == TARGET_OS_LINUX || os == TARGET_OS_MACOS || os == TARGET_OS_ANDROID);
        case TARGET_ARCH_ARM32:
            return (os == TARGET_OS_LINUX || os == TARGET_OS_ANDROID);
        case TARGET_ARCH_WASM32:
        case TARGET_ARCH_WASM64:
            return (os == TARGET_OS_WASM);
        default:
            return false;
    }
}

// ===============================================
// Utility Functions
// ===============================================

void target_print_info(TargetInfo* target) {
    if (!target) return;
    
    printf("Target Information:\n");
    printf("  Architecture: %s\n", target_get_arch_name(target->arch));
    printf("  Operating System: %s\n", target_get_os_name(target->os));
    printf("  ABI: %s\n", target_get_abi_name(target->abi));
    printf("  Pointer size: %d bytes\n", target->pointer_size);
    printf("  Word size: %d bytes\n", target->word_size);
    printf("  Max alignment: %d bytes\n", target->max_alignment);
    printf("  Has FPU: %s\n", target->has_fpu ? "yes" : "no");
    printf("  Has vector unit: %s\n", target->has_vector_unit ? "yes" : "no");
}

const char* target_get_error(TargetContext* target) {
    return target ? target->error_message : "Invalid target context";
}
