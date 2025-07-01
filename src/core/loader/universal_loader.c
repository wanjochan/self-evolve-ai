/**
 * universal_loader.c - 跨平台统一加载器
 *
 * 支持Windows、Linux、macOS的统一加载器
 * 加载.native模块并执行
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include "../include/native_format.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#endif

// ===============================================
// 加载器结构定义
// ===============================================

typedef struct {
    NativeModule* module;
    void* code_memory;
    size_t code_size;
    void* data_memory;
    size_t data_size;
} LoadedModule;

// ===============================================
// 内存管理
// ===============================================

static void* allocate_executable_memory(size_t size) {
#ifdef _WIN32
    return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#else
    void* mem = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return (mem == MAP_FAILED) ? NULL : mem;
#endif
}

static void free_executable_memory(void* ptr, size_t size) {
#ifdef _WIN32
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    munmap(ptr, size);
#endif
}

// 简化的平台检测（在实际实现中会使用系统调用）
Platform detect_platform(void) {
    // 在实际实现中，这里会检查系统特征
    // 例如：检查 /proc/version, GetVersionEx(), uname() 等
    
    #ifdef _WIN32
        return PLATFORM_WINDOWS;
    #elif defined(__linux__)
        return PLATFORM_LINUX;
    #elif defined(__APPLE__)
        return PLATFORM_MACOS;
    #else
        return PLATFORM_UNKNOWN;
    #endif
}

// ===============================================
// 模块加载功能
// ===============================================

LoadedModule* load_native_module(const char* filename) {
    // 加载.native文件
    NativeModule* module = native_module_load_file(filename);
    if (!module) {
        printf("Error: Failed to load native module: %s\n", filename);
        return NULL;
    }

    // 验证模块
    if (native_module_validate(module) != NATIVE_SUCCESS) {
        printf("Error: Invalid native module: %s\n", filename);
        native_module_free(module);
        return NULL;
    }

    // 创建加载的模块结构
    LoadedModule* loaded = malloc(sizeof(LoadedModule));
    if (!loaded) {
        printf("Error: Memory allocation failed\n");
        native_module_free(module);
        return NULL;
    }

    loaded->module = module;
    loaded->code_size = module->header.code_size;
    loaded->data_size = module->header.data_size;

    // 分配可执行内存并复制代码
    if (loaded->code_size > 0) {
        loaded->code_memory = allocate_executable_memory(loaded->code_size);
        if (!loaded->code_memory) {
            printf("Error: Failed to allocate executable memory\n");
            free(loaded);
            native_module_free(module);
            return NULL;
        }

        memcpy(loaded->code_memory, module->code_section, loaded->code_size);
    } else {
        loaded->code_memory = NULL;
    }

    // 分配数据内存并复制数据
    if (loaded->data_size > 0) {
        loaded->data_memory = malloc(loaded->data_size);
        if (!loaded->data_memory) {
            printf("Error: Failed to allocate data memory\n");
            if (loaded->code_memory) {
                free_executable_memory(loaded->code_memory, loaded->code_size);
            }
            free(loaded);
            native_module_free(module);
            return NULL;
        }

        memcpy(loaded->data_memory, module->data_section, loaded->data_size);
    } else {
        loaded->data_memory = NULL;
    }

    return loaded;
}

void unload_native_module(LoadedModule* loaded) {
    if (!loaded) return;

    if (loaded->code_memory) {
        free_executable_memory(loaded->code_memory, loaded->code_size);
    }

    if (loaded->data_memory) {
        free(loaded->data_memory);
    }

    if (loaded->module) {
        native_module_free(loaded->module);
    }

    free(loaded);
}

// 获取导出函数地址
void* get_export_function(LoadedModule* loaded, const char* name) {
    if (!loaded || !name) return NULL;

    const NativeExport* export = native_module_find_export(loaded->module, name);
    if (!export || export->type != NATIVE_EXPORT_FUNCTION) {
        return NULL;
    }

    if (loaded->code_memory) {
        return (void*)((uintptr_t)loaded->code_memory + export->offset);
    }

    return NULL;
}

// ===============================================
// 主加载器入口
// ===============================================

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Universal Native Module Loader v1.0\n");
        printf("Usage: %s <module.native> [args...]\n", argv[0]);
        printf("\nExamples:\n");
        printf("  %s vm_x64_64.native program.astc\n", argv[0]);
        printf("  %s libc_x64_64.native\n", argv[0]);
        return 1;
    }

    const char* module_file = argv[1];

    printf("Loading native module: %s\n", module_file);

    // 加载模块
    LoadedModule* loaded = load_native_module(module_file);
    if (!loaded) {
        return 1;
    }

    printf("Module loaded successfully:\n");
    printf("  Architecture: %s\n",
           loaded->module->header.architecture == NATIVE_ARCH_X86_64 ? "x86_64" :
           loaded->module->header.architecture == NATIVE_ARCH_ARM64 ? "arm64" : "unknown");
    printf("  Module type: %s\n",
           loaded->module->header.module_type == NATIVE_TYPE_VM ? "VM" :
           loaded->module->header.module_type == NATIVE_TYPE_LIBC ? "libc" : "User");
    printf("  Code size: %zu bytes\n", loaded->code_size);
    printf("  Data size: %zu bytes\n", loaded->data_size);
    printf("  Exports: %u\n", loaded->module->header.export_count);

    // 查找并执行main函数
    typedef int (*main_func_t)(int, char**);
    main_func_t main_func = (main_func_t)get_export_function(loaded, "main");

    int result = 0;
    if (main_func) {
        printf("Executing main function...\n");
        result = main_func(argc - 1, argv + 1);
        printf("Module execution completed with result: %d\n", result);
    } else {
        printf("Warning: No main function found in module\n");
    }

    // 清理
    unload_native_module(loaded);

    return result;
}
