/*
 * utils.c - Utility functions for the self-evolve AI system
 * Extracted from loader.c for better modularity and reusability
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

// Platform-specific includes
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/utsname.h>
#endif

#include "utils.h"

// ===============================================
// Runtime Platform Detection (No Macros)
// ===============================================

typedef enum {
    PLATFORM_WINDOWS,
    PLATFORM_LINUX,
    PLATFORM_MACOS,
    PLATFORM_UNKNOWN
} RuntimePlatform;

/**
 * Detect the current platform at runtime without using macros
 * This uses runtime checks instead of compile-time macros
 */
static RuntimePlatform detect_platform(void) {
    // Try to detect platform by checking for platform-specific features
    // This is a simplified approach that avoids compile-time macros

    // Check for Windows by trying to access Windows-specific environment
    if (getenv("WINDIR") != NULL || getenv("windir") != NULL) {
        return PLATFORM_WINDOWS;
    }

    // Check for macOS by looking for Darwin-specific paths
    FILE* test_file = fopen("/System/Library/CoreServices/SystemVersion.plist", "r");
    if (test_file) {
        fclose(test_file);
        return PLATFORM_MACOS;
    }

    // Check for Linux by looking for common Linux paths
    test_file = fopen("/proc/version", "r");
    if (test_file) {
        fclose(test_file);
        return PLATFORM_LINUX;
    }

    return PLATFORM_UNKNOWN;
}

// ===============================================
// Architecture Detection and Utilities
// ===============================================

DetectedArchitecture detect_architecture(void) {
    RuntimePlatform platform = detect_platform();

    if (platform == PLATFORM_WINDOWS) {
        // Windows architecture detection without macros
        // Use runtime detection by checking environment or system calls

        // Check for 64-bit Windows by looking for Program Files (x86)
        if (getenv("ProgramFiles(x86)") != NULL) {
            // This is a 64-bit Windows system
            // Check processor architecture through environment
            const char* processor_arch = getenv("PROCESSOR_ARCHITECTURE");
            const char* processor_arch_w6432 = getenv("PROCESSOR_ARCHITEW6432");

            if (processor_arch_w6432 && strstr(processor_arch_w6432, "AMD64")) {
                return ARCH_X86_64;
            } else if (processor_arch && strstr(processor_arch, "AMD64")) {
                return ARCH_X86_64;
            } else if (processor_arch && strstr(processor_arch, "ARM64")) {
                return ARCH_ARM64;
            } else if (processor_arch && strstr(processor_arch, "ARM")) {
                return ARCH_ARM32;
            }
            return ARCH_X86_64; // Default for 64-bit Windows
        } else {
            // 32-bit Windows or WOW64
            const char* processor_arch = getenv("PROCESSOR_ARCHITECTURE");
            if (processor_arch && strstr(processor_arch, "x86")) {
                return ARCH_X86_32;
            } else if (processor_arch && strstr(processor_arch, "ARM")) {
                return ARCH_ARM32;
            }
            return ARCH_X86_32; // Default for 32-bit Windows
        }
    } else {
        // Unix-like systems (Linux, macOS) - use runtime detection
        // Read from /proc/cpuinfo or use uname-like approach

        // Try to read architecture from /proc/cpuinfo
        FILE* cpuinfo = fopen("/proc/cpuinfo", "r");
        if (cpuinfo) {
            char line[256];
            while (fgets(line, sizeof(line), cpuinfo)) {
                if (strstr(line, "x86_64") || strstr(line, "amd64")) {
                    fclose(cpuinfo);
                    return ARCH_X86_64;
                } else if (strstr(line, "aarch64") || strstr(line, "arm64")) {
                    fclose(cpuinfo);
                    return ARCH_ARM64;
                } else if (strstr(line, "i386") || strstr(line, "i686")) {
                    fclose(cpuinfo);
                    return ARCH_X86_32;
                } else if (strstr(line, "arm")) {
                    fclose(cpuinfo);
                    return ARCH_ARM32;
                }
            }
            fclose(cpuinfo);
        }

        // Fallback: check pointer size to determine 32/64-bit
        if (sizeof(void*) == 8) {
            return ARCH_X86_64; // Assume x86_64 for 64-bit systems
        } else {
            return ARCH_X86_32; // Assume x86_32 for 32-bit systems
        }
    }

    return ARCH_UNKNOWN;
}

const char* get_architecture_string(DetectedArchitecture arch) {
    switch (arch) {
        case ARCH_X86_64: return "x64_64";
        case ARCH_X86_32: return "x86_32";
        case ARCH_ARM64:  return "arm64";
        case ARCH_ARM32:  return "arm32";
        default:          return "unknown";
    }
}

int get_architecture_bits(DetectedArchitecture arch) {
    switch (arch) {
        case ARCH_X86_64:
        case ARCH_ARM64:
            return 64;
        case ARCH_X86_32:
        case ARCH_ARM32:
            return 32;
        default:
            return 0;
    }
}

// ===============================================
// Path Construction Utilities
// ===============================================

int construct_vm_module_path(char* buffer, size_t buffer_size, const UnifiedLoaderConfig* config) {
    DetectedArchitecture arch = detect_architecture();
    
    if (config->vm_module_override) {
        // Use user-specified VM module path
        if (strlen(config->vm_module_override) >= buffer_size) {
            return -1; // Buffer too small
        }
        strcpy(buffer, config->vm_module_override);
        return 0;
    }
    
    if (arch == ARCH_UNKNOWN) {
        return -1; // Unsupported architecture
    }
    
    const char* arch_str = get_architecture_string(arch);
    int bits = get_architecture_bits(arch);
    
    // Construct PRD-compliant path: bin/layer2/vm_{arch}_{bits}.native
    int result = snprintf(buffer, buffer_size, "bin\\layer2\\vm_%s_%d.native", arch_str, bits);
    
    if (result < 0 || (size_t)result >= buffer_size) {
        return -1; // Buffer too small or encoding error
    }
    
    return 0;
}

// ===============================================
// Error Handling and Logging Utilities
// ===============================================

void print_error(const char* format, ...) {
    fprintf(stderr, "Error: ");
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

void print_verbose(const UnifiedLoaderConfig* config, const char* format, ...) {
    if (!config || !config->verbose_mode) return;
    
    printf("Verbose: ");
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

void print_info(const char* format, ...) {
    printf("Info: ");
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

void print_warning(const char* format, ...) {
    printf("Warning: ");
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

void print_debug(const UnifiedLoaderConfig* config, const char* format, ...) {
    if (!config || !config->debug_mode) return;

    printf("Debug: ");
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

// ===============================================
// Memory Management Utilities
// ===============================================

void* allocate_executable_memory(size_t size) {
    RuntimePlatform platform = detect_platform();

    if (platform == PLATFORM_WINDOWS) {
        // Windows: Use VirtualAlloc (if available)
        // For now, use regular malloc as a fallback
        void* ptr = malloc(size);
        if (ptr) {
            // Note: This doesn't actually make memory executable on Windows
            // A real implementation would use VirtualAlloc with PAGE_EXECUTE_READWRITE
            return ptr;
        }
        return NULL;
    } else {
        // Unix-like systems: Use mmap (if available)
        // For now, use regular malloc as a fallback
        void* ptr = malloc(size);
        if (ptr) {
            // Note: This doesn't actually make memory executable on Unix
            // A real implementation would use mmap with PROT_EXEC
            return ptr;
        }
        return NULL;
    }
}

void free_executable_memory(void* ptr, size_t size) {
    RuntimePlatform platform = detect_platform();

    if (platform == PLATFORM_WINDOWS) {
        // Windows: Use VirtualFree (if available)
        // For now, use regular free as a fallback
        free(ptr);
        (void)size; // Unused parameter in this fallback implementation
    } else {
        // Unix-like systems: Use munmap (if available)
        // For now, use regular free as a fallback
        free(ptr);
        (void)size; // Unused parameter in this fallback implementation
    }
}

// ===============================================
// File Utilities
// ===============================================

int file_exists(const char* path) {
    FILE* file = fopen(path, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

long get_file_size(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) return -1;
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fclose(file);
    
    return size;
}

int read_file_to_buffer(const char* path, void** buffer, size_t* size) {
    FILE* file = fopen(path, "rb");
    if (!file) return -1;
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size < 0) {
        fclose(file);
        return -1;
    }
    
    *buffer = malloc(file_size);
    if (!*buffer) {
        fclose(file);
        return -1;
    }
    
    size_t read_size = fread(*buffer, 1, file_size, file);
    fclose(file);
    
    if (read_size != (size_t)file_size) {
        free(*buffer);
        *buffer = NULL;
        return -1;
    }
    
    *size = file_size;
    return 0;
}

// ===============================================
// String Utilities
// ===============================================

char* safe_strdup(const char* str) {
    if (!str) return NULL;
    
    size_t len = strlen(str);
    char* copy = malloc(len + 1);
    if (!copy) return NULL;
    
    strcpy(copy, str);
    return copy;
}

int safe_snprintf(char* buffer, size_t size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vsnprintf(buffer, size, format, args);
    va_end(args);
    
    if (result < 0 || (size_t)result >= size) {
        if (size > 0) buffer[size - 1] = '\0';
        return -1;
    }
    
    return result;
}
