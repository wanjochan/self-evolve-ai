/**
 * utils_module.c - Utilities Module
 * 
 * Provides utility functions as a module.
 * Depends on the memory module.
 */

#include "../module.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>

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

// Module name
static const char* MODULE_NAME = "utils";

// Dependency on memory module
MODULE_DEPENDS_ON(memory);

// Function type definitions for memory module functions
typedef void* (*memory_alloc_t)(size_t size, int pool);
typedef void (*memory_free_t)(void* ptr);

// Cached memory functions
static memory_alloc_t mem_alloc;
static memory_free_t mem_free;

// ===============================================
// Memory Pool Types (from memory.h)
// ===============================================

typedef enum {
    MEMORY_POOL_GENERAL,    // General purpose allocations
    MEMORY_POOL_BYTECODE,   // ASTC bytecode storage
    MEMORY_POOL_JIT,        // JIT compiled code
    MEMORY_POOL_MODULES,    // Native modules
    MEMORY_POOL_TEMP,       // Temporary allocations
    MEMORY_POOL_C99_AST,    // C99 AST nodes
    MEMORY_POOL_C99_SYMBOLS,// C99 symbol table
    MEMORY_POOL_C99_STRINGS,// C99 string literals
    MEMORY_POOL_COUNT
} MemoryPoolType;

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
// Runtime Platform Detection
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
// Architecture Detection Functions
// ===============================================

/**
 * Detect the current system architecture
 */
static DetectedArchitecture detect_architecture(void) {
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

/**
 * Get architecture name as string
 */
static const char* get_architecture_name(DetectedArchitecture arch) {
    switch (arch) {
        case ARCH_X86_64: return "x64";
        case ARCH_X86_32: return "x86";
        case ARCH_ARM64: return "arm64";
        case ARCH_ARM32: return "arm32";
        default: return "unknown";
    }
}

/**
 * Get string representation of architecture
 */
static const char* get_architecture_string(DetectedArchitecture arch) {
    switch (arch) {
        case ARCH_X86_64: return "x64_64";
        case ARCH_X86_32: return "x86_32";
        case ARCH_ARM64:  return "arm64";
        case ARCH_ARM32:  return "arm32";
        default:          return "unknown";
    }
}

/**
 * Get bit width of architecture
 */
static int get_architecture_bits(DetectedArchitecture arch) {
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
// String Utility Functions
// ===============================================

/**
 * Safe string copy function
 */
static void safe_strncpy(char* dest, const char* src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) {
        return;
    }

    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
}

/**
 * Safe string duplication (handles NULL input)
 */
static char* safe_strdup(const char* str) {
    if (!str) return NULL;
    
    size_t len = strlen(str);
    char* copy = mem_alloc(len + 1, MEMORY_POOL_GENERAL);
    if (!copy) return NULL;
    
    strcpy(copy, str);
    return copy;
}

/**
 * Safe snprintf with error checking
 */
static int safe_snprintf(char* buffer, size_t size, const char* format, ...) {
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

// ===============================================
// Logging Functions
// ===============================================

/**
 * Print error message to stderr
 */
static void print_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "ERROR: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

/**
 * Print informational message
 */
static void print_info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("INFO: ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

/**
 * Print warning message
 */
static void print_warning(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "WARNING: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

// ===============================================
// Memory Management Functions
// ===============================================

/**
 * Allocate executable memory (cross-platform)
 */
static void* allocate_executable_memory(size_t size) {
    void* ptr = NULL;

#ifdef _WIN32
    // Windows implementation
    ptr = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#else
    // Unix-like implementation
    ptr = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC,
               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        ptr = NULL;
    }
#endif

    return ptr;
}

/**
 * Free executable memory (cross-platform)
 */
static void free_executable_memory(void* ptr, size_t size) {
    if (!ptr) return;

#ifdef _WIN32
    // Windows implementation
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    // Unix-like implementation
    munmap(ptr, size);
#endif
}

// ===============================================
// File Utility Functions
// ===============================================

/**
 * Check if a file exists
 */
static int file_exists(const char* path) {
    FILE* file = fopen(path, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

/**
 * Get the size of a file
 */
static long get_file_size(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) return -1;
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fclose(file);
    
    return size;
}

/**
 * Read entire file into a buffer
 */
static int read_file_to_buffer(const char* path, void** buffer, size_t* size) {
    FILE* file = fopen(path, "rb");
    if (!file) return -1;
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size < 0) {
        fclose(file);
        return -1;
    }
    
    *buffer = mem_alloc(file_size, MEMORY_POOL_GENERAL);
    if (!*buffer) {
        fclose(file);
        return -1;
    }
    
    size_t read_size = fread(*buffer, 1, file_size, file);
    fclose(file);
    
    if (read_size != (size_t)file_size) {
        mem_free(*buffer);
        *buffer = NULL;
        return -1;
    }
    
    *size = file_size;
    return 0;
}

// ===============================================
// Time Utility Functions
// ===============================================

/**
 * Get current time in microseconds
 */
static uint64_t get_current_time_us(void) {
    struct timespec ts;
    
#ifdef _WIN32
    // Windows implementation using QueryPerformanceCounter
    static LARGE_INTEGER frequency;
    static int initialized = 0;
    LARGE_INTEGER counter;
    
    if (!initialized) {
        QueryPerformanceFrequency(&frequency);
        initialized = 1;
    }
    
    QueryPerformanceCounter(&counter);
    return (uint64_t)(counter.QuadPart * 1000000 / frequency.QuadPart);
#else
    // POSIX implementation
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
#endif
}

/**
 * Sleep for specified milliseconds
 */
static void sleep_ms(unsigned int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

// ===============================================
// Symbol Table
// ===============================================

// Symbol table
static struct {
    const char* name;
    void* symbol;
} utils_symbols[] = {
    // Architecture detection
    {"detect_architecture", detect_architecture},
    {"get_architecture_name", get_architecture_name},
    {"get_architecture_string", get_architecture_string},
    {"get_architecture_bits", get_architecture_bits},
    
    // String utilities
    {"safe_strncpy", safe_strncpy},
    {"safe_strdup", safe_strdup},
    {"safe_snprintf", safe_snprintf},
    
    // Logging functions
    {"print_error", print_error},
    {"print_info", print_info},
    {"print_warning", print_warning},
    
    // Memory management
    {"allocate_executable_memory", allocate_executable_memory},
    {"free_executable_memory", free_executable_memory},
    
    // File utilities
    {"file_exists", file_exists},
    {"get_file_size", get_file_size},
    {"read_file_to_buffer", read_file_to_buffer},
    
    // Time utilities
    {"get_current_time_us", get_current_time_us},
    {"sleep_ms", sleep_ms},
    
    {NULL, NULL}  // Sentinel
};

// ===============================================
// Module Interface
// ===============================================

// Module load function
static int utils_load(void) {
    // Resolve required memory functions
    Module* memory = module_get("memory");
    if (!memory) {
        return -1;
    }
    
    mem_alloc = module_resolve(memory, "alloc_pool");
    mem_free = module_resolve(memory, "free");
    
    if (!mem_alloc || !mem_free) {
        return -1;
    }
    
    return 0;
}

// Module unload function
static void utils_unload(void) {
    // Nothing to clean up
}

// Symbol resolution function
static void* utils_resolve(const char* symbol) {
    if (!symbol) {
        return NULL;
    }
    
    for (int i = 0; utils_symbols[i].name; i++) {
        if (strcmp(utils_symbols[i].name, symbol) == 0) {
            return utils_symbols[i].symbol;
        }
    }
    
    return NULL;
}

// Module definition
static Module module_utils = {
    .name = MODULE_NAME,
    .handle = NULL,
    .state = MODULE_UNLOADED,
    .error = NULL,
    .load = utils_load,
    .unload = utils_unload,
    .resolve = utils_resolve,
    .on_init = NULL,
    .on_exit = NULL,
    .on_error = NULL
};

// Register module
REGISTER_MODULE(utils); 