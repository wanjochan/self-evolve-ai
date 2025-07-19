/**
 * crossplatform_loader.c - Cross-Platform ASTC Simple Loader
 *
 * Unified ASTC bytecode loader for Windows, macOS, and Linux
 * Automatically detects platform and loads appropriate native modules
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Platform detection
#ifdef _WIN32
    #ifndef PLATFORM_WINDOWS
        #define PLATFORM_WINDOWS
    #endif
    #include <windows.h>
    #include <io.h>
    #define PATH_SEP "\\"
    #define LIB_EXT ".dll"
    #define EXE_EXT ".exe"
#elif defined(__APPLE__)
    #ifndef PLATFORM_MACOS
        #define PLATFORM_MACOS
    #endif
    #include <dlfcn.h>
    #include <unistd.h>
    #include <sys/utsname.h>
    #define PATH_SEP "/"
    #define LIB_EXT ".dylib"
    #define EXE_EXT ""
#else
    #ifndef PLATFORM_LINUX
        #define PLATFORM_LINUX
    #endif
    #include <dlfcn.h>
    #include <unistd.h>
    #include <sys/utsname.h>
    #define PATH_SEP "/"
    #define LIB_EXT ".so"
    #define EXE_EXT ""
#endif

// POSIX functions
#if defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
    #define _GNU_SOURCE
    #include <string.h>
#endif

// Architecture detection
#if defined(_M_X64) || defined(__x86_64__)
    #define ARCH_X64
    #define ARCH_NAME "x64"
    #define ARCH_BITS "64"
#elif defined(_M_IX86) || defined(__i386__)
    #define ARCH_X86
    #define ARCH_NAME "x86"
    #define ARCH_BITS "32"
#elif defined(_M_ARM64) || defined(__aarch64__)
    #define ARCH_ARM64
    #define ARCH_NAME "arm64"
    #define ARCH_BITS "64"
#else
    #define ARCH_UNKNOWN
    #define ARCH_NAME "unknown"
    #define ARCH_BITS "64"
#endif

// Platform info structure
typedef struct {
    const char* platform_name;
    const char* arch_name;
    const char* arch_bits;
    const char* lib_ext;
    const char* exe_ext;
    int is_windows;
    int is_macos;
    int is_linux;
} PlatformInfo;

// Native module handle
typedef struct {
    void* handle;
    const char* path;
    int loaded;
} NativeModule;

// ASTC file header
#pragma pack(push, 1)
typedef struct {
    uint32_t magic;           // 'ASTC'
    uint32_t version;         // Version number
    uint32_t code_size;       // Size of code section
    uint32_t data_size;       // Size of data section
    uint32_t entry_point;     // Entry point offset
    uint32_t flags;           // Flags
    char reserved[8];         // Reserved bytes
} ASTC_Header;
#pragma pack(pop)

#define ASTC_MAGIC 0x43545341  // 'ASTC'
#define ASTC_VERSION_1 1

// Global platform info
static PlatformInfo g_platform = {0};
static NativeModule g_pipeline_module = {0};
static int g_initialized = 0;

// Platform detection and initialization
int detect_platform(void) {
    printf("🔍 Detecting platform and architecture...\n");
    
#ifdef PLATFORM_WINDOWS
    g_platform.platform_name = "windows";
    g_platform.is_windows = 1;
    printf("   🖥️  Platform: Windows\n");
#elif defined(PLATFORM_MACOS)
    g_platform.platform_name = "macos";
    g_platform.is_macos = 1;
    printf("   🍎 Platform: macOS\n");
#else
    g_platform.platform_name = "linux";
    g_platform.is_linux = 1;
    printf("   🐧 Platform: Linux\n");
#endif

    g_platform.arch_name = ARCH_NAME;
    g_platform.arch_bits = ARCH_BITS;
    g_platform.lib_ext = LIB_EXT;
    g_platform.exe_ext = EXE_EXT;
    
    printf("   🎯 Architecture: %s_%s\n", g_platform.arch_name, g_platform.arch_bits);
    
    return 0;
}

// Get detailed system information
void get_system_info(void) {
    printf("📋 System Information:\n");
    
#ifdef PLATFORM_WINDOWS
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    
    const char* arch_str = "Unknown";
    switch (si.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64:
            arch_str = "x64 (AMD64)";
            break;
        case PROCESSOR_ARCHITECTURE_INTEL:
            arch_str = "x86 (Intel)";
            break;
        case PROCESSOR_ARCHITECTURE_ARM64:
            arch_str = "ARM64";
            break;
    }
    
    printf("   CPU Architecture: %s\n", arch_str);
    printf("   Number of processors: %lu\n", si.dwNumberOfProcessors);
    printf("   Page size: %lu bytes\n", si.dwPageSize);
    
#else
    struct utsname uts;
    if (uname(&uts) == 0) {
        printf("   System: %s %s\n", uts.sysname, uts.release);
        printf("   Machine: %s\n", uts.machine);
        printf("   Node: %s\n", uts.nodename);
    }
#endif

    printf("   Detected config: %s_%s_%s\n", 
           g_platform.platform_name, g_platform.arch_name, g_platform.arch_bits);
}

// Construct native module path
char* construct_module_path(const char* module_name) {
    static char path[512];
    
    // Format: {module}_{platform}_{arch}_{bits}.native
    snprintf(path, sizeof(path), "%s_%s_%s_%s.native",
             module_name, 
             g_platform.platform_name,
             g_platform.arch_name, 
             g_platform.arch_bits);
    
    printf("🔗 Module path: %s\n", path);
    return path;
}

// Load native module
int load_native_module(const char* module_name, NativeModule* module) {
    char* module_path = construct_module_path(module_name);
    
    printf("📦 Loading native module: %s\n", module_path);
    
    // Check if file exists
    FILE* check = fopen(module_path, "rb");
    if (!check) {
        printf("❌ Module file not found: %s\n", module_path);
        return -1;
    }
    fclose(check);
    
#ifdef PLATFORM_WINDOWS
    module->handle = LoadLibraryA(module_path);
    if (!module->handle) {
        DWORD error = GetLastError();
        printf("❌ Failed to load module: %s (error: %lu)\n", module_path, error);
        return -1;
    }
#else
    module->handle = dlopen(module_path, RTLD_LAZY);
    if (!module->handle) {
        printf("❌ Failed to load module: %s (%s)\n", module_path, dlerror());
        return -1;
    }
#endif
    
    module->path = malloc(strlen(module_path) + 1);
    if (module->path) {
        strcpy((char*)module->path, module_path);
    }
    module->loaded = 1;
    
    printf("✅ Module loaded successfully: %s\n", module_path);
    return 0;
}

// Get symbol from native module
void* get_module_symbol(NativeModule* module, const char* symbol_name) {
    if (!module->loaded) {
        printf("❌ Module not loaded\n");
        return NULL;
    }
    
#ifdef PLATFORM_WINDOWS
    void* symbol = GetProcAddress((HMODULE)module->handle, symbol_name);
    if (!symbol) {
        printf("❌ Symbol not found: %s\n", symbol_name);
        return NULL;
    }
#else
    void* symbol = dlsym(module->handle, symbol_name);
    if (!symbol) {
        printf("❌ Symbol not found: %s (%s)\n", symbol_name, dlerror());
        return NULL;
    }
#endif
    
    printf("🔗 Symbol found: %s\n", symbol_name);
    return symbol;
}

// Unload native module
void unload_native_module(NativeModule* module) {
    if (!module->loaded) {
        return;
    }
    
    printf("🧹 Unloading module: %s\n", module->path);
    
#ifdef PLATFORM_WINDOWS
    FreeLibrary((HMODULE)module->handle);
#else
    dlclose(module->handle);
#endif
    
    if (module->path) {
        free((void*)module->path);
    }
    
    module->handle = NULL;
    module->path = NULL;
    module->loaded = 0;
}

// Validate ASTC file
int validate_astc_file(const char* filename, ASTC_Header* header) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("❌ Cannot open ASTC file: %s\n", filename);
        return -1;
    }
    
    size_t read = fread(header, sizeof(ASTC_Header), 1, file);
    fclose(file);
    
    if (read != 1) {
        printf("❌ Failed to read ASTC header\n");
        return -1;
    }
    
    if (header->magic != ASTC_MAGIC) {
        printf("❌ Invalid ASTC magic: 0x%08x (expected 0x%08x)\n", 
               header->magic, ASTC_MAGIC);
        return -1;
    }
    
    if (header->version != ASTC_VERSION_1) {
        printf("❌ Unsupported ASTC version: %u\n", header->version);
        return -1;
    }
    
    printf("✅ ASTC file validation passed\n");
    printf("   📊 Code size: %u bytes\n", header->code_size);
    printf("   📊 Data size: %u bytes\n", header->data_size);
    printf("   🎯 Entry point: 0x%x\n", header->entry_point);
    
    return 0;
}

// Execute ASTC program
int execute_astc_program(const char* filename, int argc, char* argv[]) {
    printf("🚀 Executing ASTC program: %s\n", filename);
    
    // Validate ASTC file
    ASTC_Header header;
    if (validate_astc_file(filename, &header) != 0) {
        return -1;
    }
    
    // Load pipeline module
    if (load_native_module("pipeline", &g_pipeline_module) != 0) {
        printf("❌ Failed to load pipeline module\n");
        return -1;
    }
    
    // Get execution function from pipeline module
    typedef int (*execute_func_t)(const char*, int, char**);
    execute_func_t execute_func = (execute_func_t)get_module_symbol(&g_pipeline_module, "pipeline_execute");
    
    if (!execute_func) {
        printf("❌ Pipeline execution function not found\n");
        unload_native_module(&g_pipeline_module);
        return -1;
    }
    
    printf("🎯 Calling pipeline execution...\n");
    int result = execute_func(filename, argc, argv);
    
    if (result == 0) {
        printf("✅ ASTC program executed successfully\n");
    } else {
        printf("❌ ASTC program execution failed (exit code: %d)\n", result);
    }
    
    // Cleanup
    unload_native_module(&g_pipeline_module);
    
    return result;
}

// Show platform compatibility information
void show_platform_compatibility(void) {
    printf("\n🌍 CROSS-PLATFORM COMPATIBILITY\n");
    printf("================================\n");
    
    printf("Current Platform: %s_%s_%s\n", 
           g_platform.platform_name, g_platform.arch_name, g_platform.arch_bits);
    
    printf("\nSupported Platforms:\n");
    printf("  🖥️  Windows x64    - windows_x64_64.native\n");
    printf("  🖥️  Windows x86    - windows_x86_32.native\n");
    printf("  🍎 macOS ARM64    - macos_arm64_64.native\n");
    printf("  🍎 macOS x64      - macos_x64_64.native\n");
    printf("  🐧 Linux x64      - linux_x64_64.native\n");
    printf("  🐧 Linux x86      - linux_x86_32.native\n");
    
    printf("\nModule Search Path: ./{module}_{platform}_{arch}_{bits}.native\n");
    printf("Required Modules: pipeline, layer0, compiler, libc\n");
}

// Show help information
void show_help(const char* program_name) {
    printf("📖 Cross-Platform ASTC Simple Loader\n");
    printf("=====================================\n");
    printf("Usage: %s [options] <astc_file> [program_args...]\n\n", program_name);
    
    printf("Options:\n");
    printf("  -h, --help     Show this help message\n");
    printf("  -v, --version  Show version information\n");
    printf("  -i, --info     Show platform information\n");
    printf("  -c, --compat   Show platform compatibility\n");
    
    printf("\nExamples:\n");
    printf("  %s program.astc\n", program_name);
    printf("  %s program.astc arg1 arg2\n", program_name);
    printf("  %s --info\n", program_name);
    
    show_platform_compatibility();
}

// Show version information
void show_version(void) {
    printf("Cross-Platform ASTC Simple Loader v1.0.0\n");
    printf("Part of C99Bin Cross-Platform Toolchain\n");
    printf("Built for: %s_%s_%s\n", 
           g_platform.platform_name, g_platform.arch_name, g_platform.arch_bits);
    printf("Supports: Windows, macOS, Linux (x86, x64, ARM64)\n");
}

// Initialize cross-platform loader
int initialize_loader(void) {
    if (g_initialized) {
        return 0;
    }
    
    printf("🔧 Initializing Cross-Platform ASTC Loader...\n");
    
    if (detect_platform() != 0) {
        printf("❌ Platform detection failed\n");
        return -1;
    }
    
    get_system_info();
    
    g_initialized = 1;
    printf("✅ Cross-Platform Loader initialized\n\n");
    
    return 0;
}

// Cleanup loader
void cleanup_loader(void) {
    if (!g_initialized) {
        return;
    }
    
    printf("\n🧹 Cleaning up Cross-Platform Loader...\n");
    
    // Unload any loaded modules
    unload_native_module(&g_pipeline_module);
    
    g_initialized = 0;
    printf("✅ Cleanup complete\n");
}

// Main entry point
int main(int argc, char* argv[]) {
    // Initialize loader
    if (initialize_loader() != 0) {
        return 1;
    }
    
    // Parse command line arguments
    if (argc < 2) {
        show_help(argv[0]);
        cleanup_loader();
        return 1;
    }
    
    // Handle options
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        show_help(argv[0]);
        cleanup_loader();
        return 0;
    }
    
    if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
        show_version();
        cleanup_loader();
        return 0;
    }
    
    if (strcmp(argv[1], "-i") == 0 || strcmp(argv[1], "--info") == 0) {
        get_system_info();
        cleanup_loader();
        return 0;
    }
    
    if (strcmp(argv[1], "-c") == 0 || strcmp(argv[1], "--compat") == 0) {
        show_platform_compatibility();
        cleanup_loader();
        return 0;
    }
    
    // Execute ASTC program
    const char* astc_file = argv[1];
    int result = execute_astc_program(astc_file, argc - 1, &argv[1]);
    
    cleanup_loader();
    return result;
}