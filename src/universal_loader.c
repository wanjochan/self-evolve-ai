/**
 * universal_loader.c - 跨平台统一加载器
 * 
 * 支持Windows、Linux、macOS的统一加载器
 * 自动检测平台并选择合适的runtime模块
 */

#include <stddef.h>
#include <stdint.h>

// ===============================================
// 平台检测
// ===============================================

typedef enum {
    PLATFORM_WINDOWS,
    PLATFORM_LINUX,
    PLATFORM_MACOS,
    PLATFORM_UNKNOWN
} Platform;

typedef enum {
    ARCH_X86_64,
    ARCH_X86_32,
    ARCH_ARM64,
    ARCH_ARM32,
    ARCH_UNKNOWN
} Architecture;

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

Architecture detect_architecture(void) {
    // 在实际实现中，这里会检查CPU架构
    // 例如：检查 CPUID, /proc/cpuinfo, sysctl 等
    
    #ifdef _WIN64
        return ARCH_X86_64;
    #elif defined(_WIN32)
        return ARCH_X86_32;
    #elif defined(__x86_64__)
        return ARCH_X86_64;
    #elif defined(__i386__)
        return ARCH_X86_32;
    #elif defined(__aarch64__)
        return ARCH_ARM64;
    #elif defined(__arm__)
        return ARCH_ARM32;
    #else
        return ARCH_UNKNOWN;
    #endif
}

// ===============================================
// 路径和文件名处理
// ===============================================

const char* get_platform_name(Platform platform) {
    switch (platform) {
        case PLATFORM_WINDOWS: return "windows";
        case PLATFORM_LINUX: return "linux";
        case PLATFORM_MACOS: return "macos";
        default: return "unknown";
    }
}

const char* get_arch_name(Architecture arch) {
    switch (arch) {
        case ARCH_X86_64: return "x64_64";
        case ARCH_X86_32: return "x86_32";
        case ARCH_ARM64: return "arm64";
        case ARCH_ARM32: return "arm32";
        default: return "unknown";
    }
}

// 构建runtime模块文件名
void build_runtime_filename(char* buffer, size_t buffer_size, Architecture arch) {
    const char* arch_name = get_arch_name(arch);
    
    // 简化的字符串拼接
    char* ptr = buffer;
    const char* prefix = "vm_";
    const char* suffix = ".native";
    
    // 复制 "vm_"
    while (*prefix != '\0' && ptr < buffer + buffer_size - 1) {
        *ptr++ = *prefix++;
    }
    
    // 复制架构名
    const char* arch_ptr = arch_name;
    while (*arch_ptr != '\0' && ptr < buffer + buffer_size - 1) {
        *ptr++ = *arch_ptr++;
    }
    
    // 复制 ".native"
    while (*suffix != '\0' && ptr < buffer + buffer_size - 1) {
        *ptr++ = *suffix++;
    }
    
    *ptr = '\0';
}

// 构建libc模块文件名
void build_libc_filename(char* buffer, size_t buffer_size, Architecture arch) {
    const char* arch_name = get_arch_name(arch);
    
    char* ptr = buffer;
    const char* prefix = "libc_";
    const char* suffix = ".native";
    
    // 复制 "libc_"
    while (*prefix != '\0' && ptr < buffer + buffer_size - 1) {
        *ptr++ = *prefix++;
    }
    
    // 复制架构名
    const char* arch_ptr = arch_name;
    while (*arch_ptr != '\0' && ptr < buffer + buffer_size - 1) {
        *ptr++ = *arch_ptr++;
    }
    
    // 复制 ".native"
    while (*suffix != '\0' && ptr < buffer + buffer_size - 1) {
        *ptr++ = *suffix++;
    }
    
    *ptr = '\0';
}

// ===============================================
// 模块加载
// ===============================================

typedef struct {
    void* handle;
    size_t size;
    const char* filename;
} LoadedModule;

// 简化的模块加载（在实际实现中会使用平台特定的API）
int load_native_module(LoadedModule* module, const char* filename) {
    // 在实际实现中，这里会：
    // Windows: 使用 LoadLibrary/GetProcAddress
    // Linux: 使用 dlopen/dlsym
    // macOS: 使用 dlopen/dlsym
    
    module->filename = filename;
    module->handle = (void*)0x12345678; // 模拟句柄
    module->size = 1024; // 模拟大小
    
    return 1; // 成功
}

void unload_native_module(LoadedModule* module) {
    // 在实际实现中，这里会：
    // Windows: 使用 FreeLibrary
    // Linux/macOS: 使用 dlclose
    
    module->handle = NULL;
    module->size = 0;
    module->filename = NULL;
}

// ===============================================
// 跨平台系统调用封装
// ===============================================

// 跨平台的程序执行
int execute_program(const char* program_path, const char* args) {
    // 在实际实现中，这里会使用平台特定的API：
    // Windows: CreateProcess
    // Linux/macOS: execve/fork
    
    // 模拟执行
    return 0; // 成功
}

// 跨平台的文件存在检查
int file_exists(const char* filename) {
    // 在实际实现中，这里会使用平台特定的API：
    // Windows: GetFileAttributes
    // Linux/macOS: access/stat
    
    // 模拟检查（假设文件存在）
    return 1;
}

// ===============================================
// 主加载器逻辑
// ===============================================

int universal_loader_main(int argc, char* argv[]) {
    if (argc < 2) {
        // 在实际实现中会使用printf
        return 1;
    }
    
    // 检测平台和架构
    Platform platform = detect_platform();
    Architecture arch = detect_architecture();
    
    if (platform == PLATFORM_UNKNOWN || arch == ARCH_UNKNOWN) {
        return 1;
    }
    
    // 构建模块文件名
    char runtime_filename[64];
    char libc_filename[64];
    
    build_runtime_filename(runtime_filename, sizeof(runtime_filename), arch);
    build_libc_filename(libc_filename, sizeof(libc_filename), arch);
    
    // 检查模块文件是否存在
    if (!file_exists(runtime_filename)) {
        return 1;
    }
    
    if (!file_exists(libc_filename)) {
        return 1;
    }
    
    // 加载模块
    LoadedModule runtime_module;
    LoadedModule libc_module;
    
    if (!load_native_module(&runtime_module, runtime_filename)) {
        return 1;
    }
    
    if (!load_native_module(&libc_module, libc_filename)) {
        unload_native_module(&runtime_module);
        return 1;
    }
    
    // 执行程序
    const char* program_path = argv[1];
    int result = execute_program(program_path, argc > 2 ? argv[2] : NULL);
    
    // 清理
    unload_native_module(&libc_module);
    unload_native_module(&runtime_module);
    
    return result;
}

// ===============================================
// 平台特定的入口点
// ===============================================

#ifdef _WIN32
// Windows入口点
int WinMain(void* hInstance, void* hPrevInstance, char* lpCmdLine, int nCmdShow) {
    // 在实际实现中会解析命令行
    char* argv[] = {"loader.exe", "program.astc"};
    return universal_loader_main(2, argv);
}
#endif

// 标准C入口点
int main(int argc, char* argv[]) {
    return universal_loader_main(argc, argv);
}

// ===============================================
// 版本信息
// ===============================================

const char* get_loader_version(void) {
    return "Universal Loader v1.0";
}

const char* get_supported_platforms(void) {
    return "Windows, Linux, macOS";
}

const char* get_supported_architectures(void) {
    return "x86_64, x86_32, ARM64, ARM32";
}

// ===============================================
// 测试函数
// ===============================================

int test_platform_detection(void) {
    Platform platform = detect_platform();
    Architecture arch = detect_architecture();
    
    if (platform == PLATFORM_UNKNOWN || arch == ARCH_UNKNOWN) {
        return 0; // 测试失败
    }
    
    // 测试文件名构建
    char runtime_filename[64];
    char libc_filename[64];
    
    build_runtime_filename(runtime_filename, sizeof(runtime_filename), arch);
    build_libc_filename(libc_filename, sizeof(libc_filename), arch);
    
    // 检查文件名格式
    if (runtime_filename[0] == '\0' || libc_filename[0] == '\0') {
        return 0; // 测试失败
    }
    
    return 1; // 测试通过
}
