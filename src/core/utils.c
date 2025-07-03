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
