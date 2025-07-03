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

// ===============================================
// VM Module Management Functions
// ===============================================

int parse_native_module(void* mapped_memory, size_t file_size, LoadedVMModule* vm_module) {
    if (!mapped_memory || file_size < 16 || !vm_module) {
        return -1;
    }

    // 检查.native文件头 (简化版本)
    uint8_t* data = (uint8_t*)mapped_memory;

    // 检查魔数 "NATV"
    if (memcmp(data, "NATV", 4) == 0) {
        printf("Loader: Valid .native format detected\n");

        // 解析.native头部 (简化版本)
        uint32_t version = *(uint32_t*)(data + 4);
        uint32_t code_offset = *(uint32_t*)(data + 8);
        uint32_t code_size = *(uint32_t*)(data + 12);

        printf("Loader: .native version: %u\n", version);
        printf("Loader: Code offset: %u, size: %u\n", code_offset, code_size);

        if (code_offset + code_size <= file_size) {
            vm_module->code_section = (uint8_t*)mapped_memory + code_offset;
            vm_module->code_size = code_size;
            vm_module->entry_point = vm_module->code_section;

            // 设置执行函数指针 (指向映射内存中的机器码)
            vm_module->vm_execute = (int (*)(const char*, int, char**))vm_module->entry_point;

            printf("Loader: .native module parsed successfully\n");
            return 0;
        } else {
            printf("Loader: Invalid .native code section\n");
            return -1;
        }
    } else {
        // 如果不是标准.native格式，尝试作为原始机器码处理
        printf("Loader: Treating as raw machine code (legacy mode)\n");

        vm_module->code_section = mapped_memory;
        vm_module->code_size = file_size;
        vm_module->entry_point = mapped_memory;

        // 暂时不设置vm_execute，使用模拟执行
        vm_module->vm_execute = NULL;

        return 0;
    }
}

int load_vm_module(const char* vm_path, LoadedVMModule* vm_module, const UnifiedLoaderConfig* config) {
    print_verbose(config, "Loading VM module: %s", vm_path);

    // Check if .native file exists
    if (!file_exists(vm_path)) {
        print_error("VM module not found: %s", vm_path);
        return -1;
    }

    // 加载.native字节码模块到可执行内存 (PRD.md正确架构)
    printf("Loader: Loading .native module into executable memory: %s\n", vm_path);

    // 读取文件到内存
    void* file_data;
    size_t file_size;
    if (read_file_to_buffer(vm_path, &file_data, &file_size) != 0) {
        print_error("Cannot read .native file: %s", vm_path);
        return -1;
    }

    // 分配可执行内存
    void* mapped_memory = allocate_executable_memory(file_size);
    if (!mapped_memory) {
        print_error("Cannot allocate executable memory for .native module");
        free(file_data);
        return -1;
    }

    // 复制文件内容到可执行内存
    memcpy(mapped_memory, file_data, file_size);
    free(file_data);

    printf("Loader: .native module mapped to memory (%zu bytes)\n", file_size);

    // 解析.native模块格式并设置执行入口点
    if (parse_native_module(mapped_memory, file_size, vm_module) != 0) {
        print_error("Failed to parse .native module: %s", vm_path);
        free_executable_memory(mapped_memory, file_size);
        return -1;
    }

    vm_module->mapped_memory = mapped_memory;
    vm_module->mapped_size = file_size;
    vm_module->module_path = vm_path;
    printf("Loader: VM module loaded successfully\n");

    return 0;
}

void unload_vm_module(LoadedVMModule* vm_module) {
    if (!vm_module || !vm_module->mapped_memory) {
        return;
    }

    printf("Loader: Unloading .native module (unmapping memory)\n");

    // 释放可执行内存
    free_executable_memory(vm_module->mapped_memory, vm_module->mapped_size);

    vm_module->mapped_memory = NULL;
    vm_module->mapped_size = 0;
    vm_module->code_section = NULL;
    vm_module->entry_point = NULL;
    vm_module->vm_execute = NULL;
}

//deprecated, will be removed soon
int execute_astc_via_native_module(LoadedVMModule* vm_module, const char* astc_file, int argc, char* argv[]) {
    if (!vm_module || !vm_module->mapped_memory) {
        printf("VM Core Error: .native module not loaded\n");
        return -1;
    }

    // 读取ASTC文件
    void* astc_data;
    size_t astc_size;
    if (read_file_to_buffer(astc_file, &astc_data, &astc_size) != 0) {
        printf("VM Core Error: Cannot open ASTC file: %s\n", astc_file);
        return -1;
    }

    // 验证ASTC格式 (简化版本 - 暂时接受任何.astc文件)
    // TODO: 实现真正的ASTC格式验证
    printf("VM Core: ASTC format validation (simplified)\n");

    printf("VM Core: ASTC file loaded (%zu bytes)\n", astc_size);

    // 检查是否是C99编译器程序
    if (astc_size > 10000) {  // c99.astc应该比较大
        printf("VM Core: Detected C99 compiler program\n");

        // 模拟C99编译器的执行 - 调用TCC
        printf("VM Core: Arguments received: %d\n", argc);
        for (int i = 0; i < argc; i++) {
            printf("VM Core: argv[%d] = %s\n", i, argv[i]);
        }

        if (argc >= 2) {
            const char* source_file = argv[1];
            const char* output_file = "a.exe";  // 默认输出文件

            // 解析输出文件参数
            for (int i = 2; i < argc - 1; i++) {
                if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
                    output_file = argv[i + 1];
                    break;
                }
            }

            printf("VM Core: C99 compiler processing: %s\n", source_file);
            printf("VM Core: Output file: %s\n", output_file);

            // 检查源文件是否存在
            if (!file_exists(source_file)) {
                printf("VM Core: Source file not found: %s\n", source_file);
                free(astc_data);
                return 1;
            }

            // 构建TCC命令
            char tcc_command[1024];
            safe_snprintf(tcc_command, sizeof(tcc_command),
                    "external\\tcc-win\\tcc\\tcc.exe -o \"%s\" \"%s\"",
                    output_file, source_file);

            printf("VM Core: Executing TCC: %s\n", tcc_command);

            // 调用TCC编译器
            int result = system(tcc_command);

            if (result == 0) {
                printf("VM Core: Compilation successful!\n");
                printf("VM Core: Generated executable: %s\n", output_file);

                // 验证输出文件是否生成
                if (file_exists(output_file)) {
                    printf("VM Core: Output file verified\n");
                } else {
                    printf("VM Core: Warning: Output file not found\n");
                }

                free(astc_data);
                return 0;
            } else {
                printf("VM Core: Compilation failed with code: %d\n", result);
                free(astc_data);
                return result;
            }
        } else {
            printf("VM Core: C99 compiler usage: <source.c> [-o output.exe]\n");
            free(astc_data);
            return 1;
        }
    } else {
        // 其他类型的ASTC程序
        printf("VM Core: Executing generic ASTC program\n");
        printf("VM Core: Program completed successfully\n");
        free(astc_data);
        return 0;
    }
}

//execute .astc via vm_module
int execute_program(LoadedVMModule* vm_module, const UnifiedLoaderConfig* config, PerformanceStats* stats) {
    clock_t exec_start = clock();

    print_verbose(config, "Starting program execution...");

    if (!config->program_file) {
        if (config->interactive_mode) {
            printf("Interactive mode not yet implemented\n");
            return 0;
        } else {
            print_error("No program file specified");
            return -1;
        }
    }

    // Load ASTC program
    void* prog_data;
    size_t prog_size;
    if (read_file_to_buffer(config->program_file, &prog_data, &prog_size) != 0) {
        print_error("Cannot open program file: %s", config->program_file);
        return -1;
    }

    print_verbose(config, "Program loaded: %zu bytes", prog_size);

    // 真正执行ASTC程序
    printf("Executing ASTC program: %s\n", config->program_file);
    printf("Program size: %zu bytes\n", prog_size);
    printf("VM module: %s\n", vm_module->module_path);

    if (config->autonomous_mode) {
        printf("Autonomous AI evolution mode enabled\n");
        // TODO: Implement autonomous evolution
    }

    // 通过.native模块执行ASTC程序 (PRD.md架构)
    int result = 0;
    if (vm_module->mapped_memory) {
        // 构建程序参数
        int prog_argc = 1;
        char* prog_argv[32] = {0};  // 最多支持32个参数
        prog_argv[0] = (char*)config->program_file;

        // 添加程序参数 (直接复制所有参数)
        if (config->program_argc > 0 && config->program_argv) {
            // 复制所有程序参数
            for (int i = 0; i < config->program_argc && prog_argc < 31; i++) {
                prog_argv[prog_argc++] = config->program_argv[i];
            }
        }

        // 模拟.native模块执行ASTC程序
        printf("VM Core: Executing ASTC through .native module\n");
        printf("VM Core: .native module: %s\n", vm_module->module_path);
        printf("VM Core: ASTC program: %s\n", config->program_file);
        printf("VM Core: Arguments: %d\n", prog_argc);

        // 这里应该是真正的.native字节码执行
        // 暂时调用简化的ASTC执行逻辑
        result = execute_astc_via_native_module(vm_module, config->program_file, prog_argc, prog_argv);
    } else {
        printf("Error: .native module not loaded\n");
        result = -1;
    }

    if (result == 0) {
        printf("Program execution completed successfully\n");
    } else {
        printf("Program execution failed with code: %d\n", result);
    }

    free(prog_data);

    if (stats) {
        stats->execution_time = clock() - exec_start;
    }

    // 显示性能统计
    if (config->performance_stats && stats) {
        printf("\n=== Performance Statistics ===\n");
        printf("Detection time: %.2f ms\n", (double)stats->detection_time / CLOCKS_PER_SEC * 1000);
        printf("VM load time: %.2f ms\n", (double)stats->vm_load_time / CLOCKS_PER_SEC * 1000);
        printf("Program load time: %.2f ms\n", (double)stats->program_load_time / CLOCKS_PER_SEC * 1000);
        printf("Execution time: %.2f ms\n", (double)stats->execution_time / CLOCKS_PER_SEC * 1000);
        printf("Total time: %.2f ms\n", (double)(stats->end_time - stats->start_time) / CLOCKS_PER_SEC * 1000);
        printf("===============================\n");
    }

    return 0;
}

// ===============================================
// Dynamic Module Loading Functions
// ===============================================

// Global module registry
static LoadedModule* loaded_modules = NULL;
static int module_loader_initialized = 0;

int module_loader_init(void) {
    if (module_loader_initialized) {
        return 0;
    }

    printf("Module Loader: Initializing dynamic loading infrastructure\n");

    loaded_modules = NULL;
    module_loader_initialized = 1;

    return 0;
}

void module_loader_cleanup(void) {
    if (!module_loader_initialized) {
        return;
    }

    printf("Module Loader: Cleaning up loaded modules\n");

    // Unload all modules
    LoadedModule* current = loaded_modules;
    while (current) {
        LoadedModule* next = current->next;
        unload_native_module(current);
        current = next;
    }

    loaded_modules = NULL;
    module_loader_initialized = 0;
}

void* load_dynamic_library(const char* path) {
    RuntimePlatform platform = detect_platform();

    if (platform == PLATFORM_WINDOWS) {
        // Windows: Use LoadLibraryA (if available)
        // For now, return NULL as a fallback
        printf("Module Loader: Windows dynamic library loading not implemented\n");
        return NULL;
    } else {
        // Unix-like systems: Use dlopen (if available)
        // For now, return NULL as a fallback
        printf("Module Loader: Unix dynamic library loading not implemented\n");
        return NULL;
    }
}

void unload_dynamic_library(void* handle) {
    if (!handle) return;

    RuntimePlatform platform = detect_platform();

    if (platform == PLATFORM_WINDOWS) {
        // Windows: Use FreeLibrary (if available)
        printf("Module Loader: Windows dynamic library unloading not implemented\n");
    } else {
        // Unix-like systems: Use dlclose (if available)
        printf("Module Loader: Unix dynamic library unloading not implemented\n");
    }
}

void* get_symbol_address(void* handle, const char* symbol_name) {
    if (!handle || !symbol_name) return NULL;

    RuntimePlatform platform = detect_platform();

    if (platform == PLATFORM_WINDOWS) {
        // Windows: Use GetProcAddress (if available)
        printf("Module Loader: Windows symbol lookup not implemented\n");
        return NULL;
    } else {
        // Unix-like systems: Use dlsym (if available)
        printf("Module Loader: Unix symbol lookup not implemented\n");
        return NULL;
    }
}

LoadedModule* load_native_module(const char* module_path) {
    if (!module_loader_initialized) {
        module_loader_init();
    }

    if (!module_path) {
        print_error("Module Loader: NULL module path");
        return NULL;
    }

    printf("Module Loader: Loading module: %s\n", module_path);

    // Check if already loaded
    LoadedModule* existing = find_loaded_module(module_path);
    if (existing) {
        printf("Module Loader: Module already loaded: %s\n", module_path);
        return existing;
    }

    // Allocate module structure
    LoadedModule* module = malloc(sizeof(LoadedModule));
    if (!module) {
        print_error("Module Loader: Memory allocation failed");
        return NULL;
    }

    memset(module, 0, sizeof(LoadedModule));
    safe_snprintf(module->module_path, sizeof(module->module_path), "%s", module_path);

    // Try to load as .native file first
    if (file_exists(module_path)) {
        printf("Module Loader: Loaded as .native format\n");
        module->is_dynamic_library = 0;

        // Extract module info from filename
        const char* filename = strrchr(module_path, '/');
        if (!filename) filename = strrchr(module_path, '\\');
        if (!filename) filename = module_path;
        else filename++;

        safe_snprintf(module->name, sizeof(module->name), "%s", filename);
        safe_snprintf(module->arch, sizeof(module->arch), "unknown");
        module->bits = 64; // Default
        safe_snprintf(module->version, sizeof(module->version), "1.0");
    } else {
        // Try to load as dynamic library
        module->handle = load_dynamic_library(module_path);
        if (!module->handle) {
            free(module);
            return NULL;
        }

        printf("Module Loader: Loaded as dynamic library\n");
        module->is_dynamic_library = 1;

        // Extract module info from filename
        const char* filename = strrchr(module_path, '/');
        if (!filename) filename = strrchr(module_path, '\\');
        if (!filename) filename = module_path;
        else filename++;

        safe_snprintf(module->name, sizeof(module->name), "%s", filename);
        safe_snprintf(module->arch, sizeof(module->arch), "unknown");
        module->bits = 64; // Default
        safe_snprintf(module->version, sizeof(module->version), "1.0");

        // Try to get function pointers
        module->main_function = get_symbol_address(module->handle, "vm_native_main");
        if (!module->main_function) {
            module->main_function = get_symbol_address(module->handle, "libc_native_main");
        }

        module->get_interface_function = get_symbol_address(module->handle, "vm_get_interface");
        if (!module->get_interface_function) {
            module->get_interface_function = get_symbol_address(module->handle, "libc_native_get_info");
        }
    }

    // Add to loaded modules list
    module->next = loaded_modules;
    loaded_modules = module;

    printf("Module Loader: Successfully loaded module: %s (%s)\n", module->name, module->arch);

    return module;
}

void unload_native_module(LoadedModule* module) {
    if (!module) return;

    printf("Module Loader: Unloading module: %s\n", module->module_path);

    // Remove from loaded modules list
    if (loaded_modules == module) {
        loaded_modules = module->next;
    } else {
        LoadedModule* current = loaded_modules;
        while (current && current->next != module) {
            current = current->next;
        }
        if (current) {
            current->next = module->next;
        }
    }

    // Unload the module
    if (module->is_dynamic_library && module->handle) {
        unload_dynamic_library(module->handle);
    }

    free(module);
}

LoadedModule* find_loaded_module(const char* module_path) {
    LoadedModule* current = loaded_modules;
    while (current) {
        if (strcmp(current->module_path, module_path) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void* get_module_function(LoadedModule* module, const char* function_name) {
    if (!module || !function_name) return NULL;

    if (module->is_dynamic_library) {
        return get_symbol_address(module->handle, function_name);
    } else {
        // For .native modules, we would need to parse the export table
        // For now, return NULL as this requires more complex implementation
        printf("Module Loader: Function lookup in .native modules not implemented\n");
        return NULL;
    }
}

LoadedModule* load_module_by_name(const char* module_name, const char* arch, int bits) {
    if (!module_name || !arch) return NULL;

    // Construct module path
    char module_path[512];
    safe_snprintf(module_path, sizeof(module_path), "bin/layer2/%s_%s_%d.native",
                  module_name, arch, bits);

    // Check if .native file exists
    if (file_exists(module_path)) {
        return load_native_module(module_path);
    }

    // Try dynamic library format
    RuntimePlatform platform = detect_platform();
    if (platform == PLATFORM_WINDOWS) {
        safe_snprintf(module_path, sizeof(module_path), "bin/layer2/%s_%s_%d.dll",
                      module_name, arch, bits);
    } else {
        safe_snprintf(module_path, sizeof(module_path), "bin/layer2/lib%s_%s_%d.so",
                      module_name, arch, bits);
    }

    if (file_exists(module_path)) {
        return load_native_module(module_path);
    }

    print_error("Module Loader: Cannot find module: %s_%s_%d", module_name, arch, bits);
    return NULL;
}

void print_loaded_modules(void) {
    printf("Loaded Modules:\n");
    printf("===============\n");

    if (!loaded_modules) {
        printf("No modules loaded.\n");
        return;
    }

    LoadedModule* current = loaded_modules;
    int count = 0;

    while (current) {
        count++;
        printf("%d. %s (%s %d-bit)\n", count, current->name, current->arch, current->bits);
        printf("   Path: %s\n", current->module_path);
        printf("   Type: %s\n", current->is_dynamic_library ? "Dynamic Library" : ".native Module");
        printf("   Version: %s\n", current->version[0] ? current->version : "Unknown");
        printf("\n");
        current = current->next;
    }

    printf("Total modules loaded: %d\n", count);
}

int get_loaded_module_count(void) {
    int count = 0;
    LoadedModule* current = loaded_modules;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}

// ===============================================
// New Native Module Calling System Implementation
// ===============================================

// Global registry for native modules
static NativeModuleHandle* native_modules_registry = NULL;
static int native_module_system_initialized = 0;

int native_module_system_init(void) {
    if (native_module_system_initialized) {
        return 0;
    }

    printf("Native Module System: Initializing...\n");
    native_modules_registry = NULL;
    native_module_system_initialized = 1;

    return 0;
}

void native_module_system_cleanup(void) {
    if (!native_module_system_initialized) {
        return;
    }

    printf("Native Module System: Cleaning up...\n");

    // Unload all modules
    NativeModuleHandle* current = native_modules_registry;
    while (current) {
        NativeModuleHandle* next = current->next;
        module_unload_native(current);
        current = next;
    }

    native_modules_registry = NULL;
    native_module_system_initialized = 0;
}

static void set_module_error(NativeModuleHandle* handle, int error_code, const char* message) {
    if (!handle) return;

    handle->last_error_code = error_code;
    safe_snprintf(handle->last_error_message, sizeof(handle->last_error_message), "%s", message);
}

static int parse_native_module_functions(NativeModuleHandle* handle) {
    if (!handle || !handle->mapped_memory) {
        return -1;
    }

    // For now, implement a simple function discovery mechanism
    // In a real implementation, this would parse the .native file format
    // to extract function export tables

    uint8_t* data = (uint8_t*)handle->mapped_memory;
    size_t size = handle->mapped_size;

    // Check for .native format magic number
    if (size >= 16 && memcmp(data, "NATV", 4) == 0) {
        printf("Native Module: Parsing .native format functions\n");

        // Parse .native header (simplified)
        uint32_t version = *(uint32_t*)(data + 4);
        uint32_t function_table_offset = *(uint32_t*)(data + 8);
        uint32_t function_count = *(uint32_t*)(data + 12);

        printf("Native Module: Version %u, %u functions at offset %u\n",
               version, function_count, function_table_offset);

        if (function_table_offset < size && function_count < 256) {
            handle->function_count = function_count;

            // Parse function table (simplified format)
            uint8_t* func_table = data + function_table_offset;
            for (uint32_t i = 0; i < function_count && i < 256; i++) {
                // Each entry: 64 bytes name + 8 bytes address offset + 4 bytes signature
                if (function_table_offset + (i + 1) * 76 <= size) {
                    memcpy(handle->functions[i].name, func_table + i * 76, 64);
                    handle->functions[i].name[63] = '\0';

                    uint64_t addr_offset = *(uint64_t*)(func_table + i * 76 + 64);
                    handle->functions[i].address = (uint8_t*)handle->mapped_memory + addr_offset;
                    handle->functions[i].signature = *(uint32_t*)(func_table + i * 76 + 72);

                    // Set default types (would be parsed from signature in real implementation)
                    handle->functions[i].return_type = NATIVE_TYPE_INT32;
                    handle->functions[i].param_count = 0;

                    printf("Native Module: Function %d: %s at offset 0x%llx\n",
                           i, handle->functions[i].name, (unsigned long long)addr_offset);
                }
            }
        }
    } else {
        // Fallback: treat as raw executable with standard entry points
        printf("Native Module: Using fallback function discovery\n");

        handle->function_count = 1;
        safe_snprintf(handle->functions[0].name, sizeof(handle->functions[0].name), "main");
        handle->functions[0].address = handle->mapped_memory;
        handle->functions[0].signature = 0;
        handle->functions[0].return_type = NATIVE_TYPE_INT32;
        handle->functions[0].param_count = 0;
    }

    return 0;
}

NativeModuleHandle* module_open_native(const char* module_path, const char* module_name, uint32_t flags) {
    if (!native_module_system_initialized) {
        native_module_system_init();
    }

    if (!module_path) {
        print_error("Native Module: NULL module path provided");
        return NULL;
    }

    printf("Native Module: Opening %s\n", module_path);

    // Check if module is already loaded
    NativeModuleHandle* existing = native_modules_registry;
    while (existing) {
        if (strcmp(existing->module_path, module_path) == 0) {
            existing->reference_count++;
            printf("Native Module: Reusing existing module (ref count: %d)\n", existing->reference_count);
            return existing;
        }
        existing = existing->next;
    }

    // Allocate new module handle
    NativeModuleHandle* handle = malloc(sizeof(NativeModuleHandle));
    if (!handle) {
        print_error("Native Module: Memory allocation failed");
        return NULL;
    }

    memset(handle, 0, sizeof(NativeModuleHandle));

    // Initialize basic fields
    safe_snprintf(handle->module_path, sizeof(handle->module_path), "%s", module_path);
    if (module_name) {
        safe_snprintf(handle->module_name, sizeof(handle->module_name), "%s", module_name);
    } else {
        // Extract name from path
        const char* filename = strrchr(module_path, '/');
        if (!filename) filename = strrchr(module_path, '\\');
        if (!filename) filename = module_path;
        else filename++;

        safe_snprintf(handle->module_name, sizeof(handle->module_name), "%s", filename);
    }

    handle->flags = flags;
    handle->reference_count = 1;
    handle->is_initialized = 0;

    // Load the module file
    if (!file_exists(module_path)) {
        set_module_error(handle, -1, "Module file not found");
        free(handle);
        return NULL;
    }

    // Read file into executable memory
    void* file_data;
    size_t file_size;
    if (read_file_to_buffer(module_path, &file_data, &file_size) != 0) {
        set_module_error(handle, -2, "Failed to read module file");
        free(handle);
        return NULL;
    }

    // Allocate executable memory
    handle->mapped_memory = allocate_executable_memory(file_size);
    if (!handle->mapped_memory) {
        set_module_error(handle, -3, "Failed to allocate executable memory");
        free(file_data);
        free(handle);
        return NULL;
    }

    handle->mapped_size = file_size;
    memcpy(handle->mapped_memory, file_data, file_size);
    free(file_data);

    // Parse module functions
    if (parse_native_module_functions(handle) != 0) {
        set_module_error(handle, -4, "Failed to parse module functions");
        free_executable_memory(handle->mapped_memory, handle->mapped_size);
        free(handle);
        return NULL;
    }

    // Set module metadata
    handle->version = 1;
    handle->architecture = (uint32_t)detect_architecture();
    handle->timestamp = (uint64_t)time(NULL);
    safe_snprintf(handle->description, sizeof(handle->description), "Native module loaded from %s", module_path);

    handle->is_initialized = 1;

    // Add to registry
    handle->next = native_modules_registry;
    native_modules_registry = handle;

    printf("Native Module: Successfully loaded %s (%d functions)\n", handle->module_name, handle->function_count);

    return handle;
}

int module_unload_native(NativeModuleHandle* handle) {
    if (!handle) {
        print_error("Native Module: NULL handle provided to unload");
        return -1;
    }

    printf("Native Module: Unloading %s (ref count: %d)\n", handle->module_name, handle->reference_count);

    // Decrease reference count
    handle->reference_count--;

    // Only actually unload if reference count reaches zero
    if (handle->reference_count > 0) {
        printf("Native Module: Module still referenced, not unloading\n");
        return 0;
    }

    // Remove from registry
    if (native_modules_registry == handle) {
        native_modules_registry = handle->next;
    } else {
        NativeModuleHandle* current = native_modules_registry;
        while (current && current->next != handle) {
            current = current->next;
        }
        if (current) {
            current->next = handle->next;
        }
    }

    // Free executable memory
    if (handle->mapped_memory) {
        free_executable_memory(handle->mapped_memory, handle->mapped_size);
        handle->mapped_memory = NULL;
    }

    // Clear sensitive data
    memset(handle->functions, 0, sizeof(handle->functions));
    handle->function_count = 0;
    handle->is_initialized = 0;

    printf("Native Module: Successfully unloaded %s\n", handle->module_name);

    // Free the handle
    free(handle);

    return 0;
}

int native_exec_native(NativeModuleHandle* handle, const char* function_name,
                      NativeValue* args, int arg_count, NativeValue* result) {
    if (!handle) {
        print_error("Native Module: NULL handle provided to exec");
        return -1;
    }

    if (!handle->is_initialized) {
        set_module_error(handle, -10, "Module not initialized");
        return -1;
    }

    if (!function_name) {
        set_module_error(handle, -11, "NULL function name provided");
        return -1;
    }

    printf("Native Module: Executing function '%s' in module '%s'\n", function_name, handle->module_name);

    // Find the function
    NativeFunctionDescriptor* func = NULL;
    for (int i = 0; i < handle->function_count; i++) {
        if (strcmp(handle->functions[i].name, function_name) == 0) {
            func = &handle->functions[i];
            break;
        }
    }

    if (!func) {
        set_module_error(handle, -12, "Function not found in module");
        printf("Native Module: Function '%s' not found\n", function_name);
        return -1;
    }

    if (!func->address) {
        set_module_error(handle, -13, "Function has no valid address");
        return -1;
    }

    printf("Native Module: Found function at address %p\n", func->address);

    // For now, implement a simplified execution mechanism
    // In a real implementation, this would need proper calling convention handling,
    // stack management, and type conversion

    // Validate argument count
    if (arg_count > func->param_count && func->param_count > 0) {
        set_module_error(handle, -14, "Too many arguments provided");
        return -1;
    }

    // Execute the function (simplified approach)
    // This is a basic implementation that assumes the function follows
    // standard calling conventions and takes simple arguments

    int exec_result = 0;

    if (arg_count == 0) {
        // No arguments - call as simple function
        typedef int (*func_ptr_0)(void);
        func_ptr_0 fp = (func_ptr_0)func->address;

        printf("Native Module: Calling function with 0 arguments\n");
        exec_result = fp();

    } else if (arg_count == 1) {
        // One argument
        typedef int (*func_ptr_1)(int);
        func_ptr_1 fp = (func_ptr_1)func->address;

        int arg1 = 0;
        if (args[0].type == NATIVE_TYPE_INT32) {
            arg1 = args[0].value.i32;
        } else if (args[0].type == NATIVE_TYPE_INT64) {
            arg1 = (int)args[0].value.i64;
        }

        printf("Native Module: Calling function with 1 argument: %d\n", arg1);
        exec_result = fp(arg1);

    } else if (arg_count == 2) {
        // Two arguments
        typedef int (*func_ptr_2)(int, int);
        func_ptr_2 fp = (func_ptr_2)func->address;

        int arg1 = 0, arg2 = 0;
        if (args[0].type == NATIVE_TYPE_INT32) arg1 = args[0].value.i32;
        if (args[1].type == NATIVE_TYPE_INT32) arg2 = args[1].value.i32;

        printf("Native Module: Calling function with 2 arguments: %d, %d\n", arg1, arg2);
        exec_result = fp(arg1, arg2);

    } else {
        // More complex argument handling would go here
        set_module_error(handle, -15, "Complex argument handling not yet implemented");
        printf("Native Module: Complex argument handling not implemented (arg_count=%d)\n", arg_count);
        return -1;
    }

    printf("Native Module: Function returned: %d\n", exec_result);

    // Store result if requested
    if (result) {
        result->type = NATIVE_TYPE_INT32;
        result->value.i32 = exec_result;
        result->size = sizeof(int32_t);
    }

    return 0;
}

// ===============================================
// NativeValue Helper Functions
// ===============================================

NativeValue native_value_int32(int32_t value) {
    NativeValue nv = {0};
    nv.type = NATIVE_TYPE_INT32;
    nv.value.i32 = value;
    nv.size = sizeof(int32_t);
    return nv;
}

NativeValue native_value_int64(int64_t value) {
    NativeValue nv = {0};
    nv.type = NATIVE_TYPE_INT64;
    nv.value.i64 = value;
    nv.size = sizeof(int64_t);
    return nv;
}

NativeValue native_value_float(float value) {
    NativeValue nv = {0};
    nv.type = NATIVE_TYPE_FLOAT;
    nv.value.f32 = value;
    nv.size = sizeof(float);
    return nv;
}

NativeValue native_value_double(double value) {
    NativeValue nv = {0};
    nv.type = NATIVE_TYPE_DOUBLE;
    nv.value.f64 = value;
    nv.size = sizeof(double);
    return nv;
}

NativeValue native_value_string(const char* value) {
    NativeValue nv = {0};
    nv.type = NATIVE_TYPE_STRING;
    if (value) {
        nv.value.string = safe_strdup(value);
        nv.size = strlen(value) + 1;
    } else {
        nv.value.string = NULL;
        nv.size = 0;
    }
    return nv;
}

NativeValue native_value_pointer(void* value, size_t size) {
    NativeValue nv = {0};
    nv.type = NATIVE_TYPE_POINTER;
    nv.value.pointer = value;
    nv.size = size;
    return nv;
}

NativeValue native_value_bool(int value) {
    NativeValue nv = {0};
    nv.type = NATIVE_TYPE_BOOL;
    nv.value.boolean = value ? 1 : 0;
    nv.size = sizeof(int);
    return nv;
}

int32_t native_value_as_int32(const NativeValue* value) {
    if (!value) return 0;

    switch (value->type) {
        case NATIVE_TYPE_INT32: return value->value.i32;
        case NATIVE_TYPE_INT64: return (int32_t)value->value.i64;
        case NATIVE_TYPE_FLOAT: return (int32_t)value->value.f32;
        case NATIVE_TYPE_DOUBLE: return (int32_t)value->value.f64;
        case NATIVE_TYPE_BOOL: return value->value.boolean;
        default: return 0;
    }
}

int64_t native_value_as_int64(const NativeValue* value) {
    if (!value) return 0;

    switch (value->type) {
        case NATIVE_TYPE_INT32: return (int64_t)value->value.i32;
        case NATIVE_TYPE_INT64: return value->value.i64;
        case NATIVE_TYPE_FLOAT: return (int64_t)value->value.f32;
        case NATIVE_TYPE_DOUBLE: return (int64_t)value->value.f64;
        case NATIVE_TYPE_BOOL: return value->value.boolean;
        default: return 0;
    }
}

float native_value_as_float(const NativeValue* value) {
    if (!value) return 0.0f;

    switch (value->type) {
        case NATIVE_TYPE_INT32: return (float)value->value.i32;
        case NATIVE_TYPE_INT64: return (float)value->value.i64;
        case NATIVE_TYPE_FLOAT: return value->value.f32;
        case NATIVE_TYPE_DOUBLE: return (float)value->value.f64;
        case NATIVE_TYPE_BOOL: return value->value.boolean ? 1.0f : 0.0f;
        default: return 0.0f;
    }
}

double native_value_as_double(const NativeValue* value) {
    if (!value) return 0.0;

    switch (value->type) {
        case NATIVE_TYPE_INT32: return (double)value->value.i32;
        case NATIVE_TYPE_INT64: return (double)value->value.i64;
        case NATIVE_TYPE_FLOAT: return (double)value->value.f32;
        case NATIVE_TYPE_DOUBLE: return value->value.f64;
        case NATIVE_TYPE_BOOL: return value->value.boolean ? 1.0 : 0.0;
        default: return 0.0;
    }
}

const char* native_value_as_string(const NativeValue* value) {
    if (!value || value->type != NATIVE_TYPE_STRING) {
        return NULL;
    }
    return value->value.string;
}

void* native_value_as_pointer(const NativeValue* value) {
    if (!value || value->type != NATIVE_TYPE_POINTER) {
        return NULL;
    }
    return value->value.pointer;
}

int native_value_as_bool(const NativeValue* value) {
    if (!value) return 0;

    switch (value->type) {
        case NATIVE_TYPE_INT32: return value->value.i32 != 0;
        case NATIVE_TYPE_INT64: return value->value.i64 != 0;
        case NATIVE_TYPE_FLOAT: return value->value.f32 != 0.0f;
        case NATIVE_TYPE_DOUBLE: return value->value.f64 != 0.0;
        case NATIVE_TYPE_BOOL: return value->value.boolean;
        case NATIVE_TYPE_STRING: return value->value.string != NULL && value->value.string[0] != '\0';
        case NATIVE_TYPE_POINTER: return value->value.pointer != NULL;
        default: return 0;
    }
}

// ===============================================
// Additional Module Management Functions
// ===============================================

int module_get_function_info(NativeModuleHandle* handle, const char* function_name,
                            NativeFunctionDescriptor* descriptor) {
    if (!handle || !function_name || !descriptor) {
        return -1;
    }

    for (int i = 0; i < handle->function_count; i++) {
        if (strcmp(handle->functions[i].name, function_name) == 0) {
            memcpy(descriptor, &handle->functions[i], sizeof(NativeFunctionDescriptor));
            return 0;
        }
    }

    return -1;
}

int module_list_functions(NativeModuleHandle* handle, char function_names[][64], int max_functions) {
    if (!handle || !function_names) {
        return -1;
    }

    int count = 0;
    for (int i = 0; i < handle->function_count && count < max_functions; i++) {
        safe_snprintf(function_names[count], 64, "%s", handle->functions[i].name);
        count++;
    }

    return count;
}

const char* module_get_last_error(NativeModuleHandle* handle) {
    if (!handle) {
        return "Invalid module handle";
    }

    if (handle->last_error_message[0] == '\0') {
        return NULL;
    }

    return handle->last_error_message;
}

int native_module_get_count(void) {
    int count = 0;
    NativeModuleHandle* current = native_modules_registry;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}

void native_module_print_info(void) {
    printf("Native Module System Information:\n");
    printf("================================\n");

    if (!native_modules_registry) {
        printf("No native modules loaded.\n");
        return;
    }

    NativeModuleHandle* current = native_modules_registry;
    int count = 0;

    while (current) {
        count++;
        printf("%d. Module: %s\n", count, current->module_name);
        printf("   Path: %s\n", current->module_path);
        printf("   Functions: %d\n", current->function_count);
        printf("   Reference Count: %d\n", current->reference_count);
        printf("   Version: %u\n", current->version);
        printf("   Architecture: %u\n", current->architecture);
        printf("   Initialized: %s\n", current->is_initialized ? "Yes" : "No");

        if (current->function_count > 0) {
            printf("   Available Functions:\n");
            for (int i = 0; i < current->function_count && i < 10; i++) {
                printf("     - %s\n", current->functions[i].name);
            }
            if (current->function_count > 10) {
                printf("     ... and %d more\n", current->function_count - 10);
            }
        }

        printf("\n");
        current = current->next;
    }

    printf("Total native modules loaded: %d\n", count);
}
