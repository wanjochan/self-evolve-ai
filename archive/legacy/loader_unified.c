/**
 * loader_unified.c - Unified Cross-Platform Loader
 * 
 * This implements the unified loader according to PRD.md requirements:
 * - Cross-platform unified startup (TODO: Cosmopolitan-style design)
 * - Hardware environment detection
 * - Load corresponding vm_{arch}_{bits}.native
 * - Unified entry point for simplified deployment
 * 
 * Architecture: loader.exe -> vm_{arch}_{bits}.native -> program.astc
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#define PATH_SEPARATOR "\\"
#else
#include <dlfcn.h>
#include <unistd.h>
#define PATH_SEPARATOR "/"
#endif

// ===============================================
// Platform Detection
// ===============================================

typedef enum {
    ARCH_UNKNOWN = 0,
    ARCH_X64,
    ARCH_ARM64,
    ARCH_X86
} Architecture;

typedef struct {
    Architecture arch;
    int bits;
    const char* os_name;
    const char* arch_name;
} PlatformInfo;

// Detect current platform
PlatformInfo detect_platform(void) {
    PlatformInfo info = {0};
    
    // Detect architecture
#if defined(_M_X64) || defined(__x86_64__)
    info.arch = ARCH_X64;
    info.bits = 64;
    info.arch_name = "x64";
#elif defined(_M_ARM64) || defined(__aarch64__)
    info.arch = ARCH_ARM64;
    info.bits = 64;
    info.arch_name = "arm64";
#elif defined(_M_IX86) || defined(__i386__)
    info.arch = ARCH_X86;
    info.bits = 32;
    info.arch_name = "x86";
#else
    info.arch = ARCH_UNKNOWN;
    info.bits = 0;
    info.arch_name = "unknown";
#endif
    
    // Detect OS
#ifdef _WIN32
    info.os_name = "windows";
#elif defined(__linux__)
    info.os_name = "linux";
#elif defined(__APPLE__)
    info.os_name = "macos";
#else
    info.os_name = "unknown";
#endif
    
    return info;
}

// ===============================================
// VM Module Loading
// ===============================================

typedef struct {
    void* handle;
    int (*vm_main)(int argc, char* argv[]);
    const void* (*get_interface)(void);
} VMModule;

// Load VM module for current platform
VMModule* load_vm_module(const PlatformInfo* platform) {
    char vm_path[256];
    
    // Construct VM module path: vm_{arch}_{bits}.native
    snprintf(vm_path, sizeof(vm_path), "bin%svm_%s_%d.native", 
             PATH_SEPARATOR, platform->arch_name, platform->bits);
    
    printf("Loader: Loading VM module: %s\n", vm_path);
    
    VMModule* vm = malloc(sizeof(VMModule));
    if (!vm) {
        fprintf(stderr, "Loader Error: Memory allocation failed\n");
        return NULL;
    }
    
    memset(vm, 0, sizeof(VMModule));
    
#ifdef _WIN32
    // Windows: Load DLL
    vm->handle = LoadLibraryA(vm_path);
    if (!vm->handle) {
        fprintf(stderr, "Loader Error: Failed to load VM module: %s (Error: %lu)\n", 
                vm_path, GetLastError());
        free(vm);
        return NULL;
    }
    
    // Get function pointers
    vm->vm_main = (int(*)(int, char**))GetProcAddress(vm->handle, "vm_native_main");
    vm->get_interface = (const void*(*)(void))GetProcAddress(vm->handle, "vm_get_interface");
#else
    // Unix: Load shared library
    vm->handle = dlopen(vm_path, RTLD_LAZY);
    if (!vm->handle) {
        fprintf(stderr, "Loader Error: Failed to load VM module: %s (%s)\n", 
                vm_path, dlerror());
        free(vm);
        return NULL;
    }
    
    // Get function pointers
    vm->vm_main = (int(*)(int, char**))dlsym(vm->handle, "vm_native_main");
    vm->get_interface = (const void*(*)(void))dlsym(vm->handle, "vm_get_interface");
#endif
    
    if (!vm->vm_main) {
        fprintf(stderr, "Loader Error: VM module missing vm_native_main function\n");
        unload_vm_module(vm);
        return NULL;
    }
    
    printf("Loader: VM module loaded successfully\n");
    return vm;
}

// Unload VM module
void unload_vm_module(VMModule* vm) {
    if (!vm) return;
    
    if (vm->handle) {
#ifdef _WIN32
        FreeLibrary(vm->handle);
#else
        dlclose(vm->handle);
#endif
    }
    
    free(vm);
}

// ===============================================
// Main Loader Logic
// ===============================================

int main(int argc, char* argv[]) {
    printf("========================================\n");
    printf("Self-Evolve AI Unified Loader\n");
    printf("========================================\n");
    
    // Check arguments
    if (argc < 2) {
        fprintf(stderr, "Usage: loader.exe <program.astc> [args...]\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "The loader will automatically:\n");
        fprintf(stderr, "1. Detect hardware platform\n");
        fprintf(stderr, "2. Load appropriate vm_{arch}_{bits}.native\n");
        fprintf(stderr, "3. Execute the ASTC program\n");
        return -1;
    }
    
    // Detect platform
    PlatformInfo platform = detect_platform();
    printf("Platform Detection:\n");
    printf("  OS: %s\n", platform.os_name);
    printf("  Architecture: %s (%d-bit)\n", platform.arch_name, platform.bits);
    
    if (platform.arch == ARCH_UNKNOWN) {
        fprintf(stderr, "Loader Error: Unsupported platform architecture\n");
        return -1;
    }
    
    // Load VM module
    VMModule* vm = load_vm_module(&platform);
    if (!vm) {
        fprintf(stderr, "Loader Error: Failed to load VM module for platform\n");
        return -1;
    }
    
    // Execute ASTC program through VM
    printf("Loader: Executing ASTC program: %s\n", argv[1]);
    printf("========================================\n");
    
    int result = vm->vm_main(argc, argv);
    
    printf("========================================\n");
    printf("Loader: Program execution completed with result: %d\n", result);
    
    // Cleanup
    unload_vm_module(vm);
    
    return result;
}

// ===============================================
// Alternative Entry Points
// ===============================================

// Entry point for testing platform detection
int test_platform_detection(void) {
    printf("Platform Detection Test:\n");
    
    PlatformInfo platform = detect_platform();
    printf("  OS: %s\n", platform.os_name);
    printf("  Architecture: %s\n", platform.arch_name);
    printf("  Bits: %d\n", platform.bits);
    printf("  Expected VM module: vm_%s_%d.native\n", 
           platform.arch_name, platform.bits);
    
    return 0;
}

// Entry point for listing available VM modules
int list_vm_modules(void) {
    printf("Available VM Modules:\n");
    
    // TODO: Scan bin directory for vm_*.native files
    printf("  vm_x64_64.native (x64 64-bit)\n");
    printf("  vm_arm64_64.native (ARM64 64-bit) [TODO]\n");
    printf("  vm_x86_32.native (x86 32-bit) [TODO]\n");
    
    return 0;
}
