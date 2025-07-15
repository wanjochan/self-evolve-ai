/**
 * simple_loader.c - Layer 1 Loader Implementation
 * 
 * Implements the PRD.md Layer 1 specification:
 * - Detects hardware architecture and OS
 * - Loads appropriate VM module (vm_{arch}_{bits}.native)
 * - Forwards arguments and environment to the program
 * 
 * Usage: simple_loader program.astc [args...]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

// Architecture detection
#if defined(__x86_64__) || defined(__amd64__)
    #define ARCH_NAME "x64"
    #define ARCH_BITS 64
#elif defined(__aarch64__) || defined(__arm64__)
    #define ARCH_NAME "arm64"
    #define ARCH_BITS 64
#elif defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__)
    #define ARCH_NAME "x86"
    #define ARCH_BITS 32
#elif defined(__arm__)
    #define ARCH_NAME "arm"
    #define ARCH_BITS 32
#else
    #error "Unsupported architecture"
#endif

// Native module format
typedef struct {
    char magic[4];          // "NATV"
    uint32_t version;       // 版本号
    uint32_t arch;          // 架构类型
    uint32_t module_type;   // 模块类型
    uint32_t flags;         // 标志
    uint32_t header_size;   // 头部大小
    uint32_t code_size;     // 代码大小
    uint32_t data_size;     // 数据大小
    uint32_t export_count;  // 导出函数数量
    uint32_t export_offset; // 导出表偏移
    uint32_t reserved[6];   // 保留字段
} NativeHeader;

typedef struct {
    char name[64];          // 函数名
    uint32_t offset;        // 函数偏移
    uint32_t size;          // 函数大小（可选）
    uint32_t flags;         // 标志
    uint32_t reserved;      // 保留
} ExportEntry;

typedef struct {
    void* base_addr;        // 基地址
    size_t size;           // 大小
    NativeHeader* header;   // 头部
    ExportEntry* exports;   // 导出表
    void* code_section;     // 代码段
} LoadedModule;

// Function pointer types
typedef int (*vm_execute_astc_t)(const char* astc_file, int argc, char* argv[]);
typedef int (*execute_astc_t)(const char* astc_file, int argc, char* argv[]);
typedef int (*native_main_t)(int argc, char* argv[]);

// ASTC文件头
typedef struct {
    char magic[4];          // "ASTC"
    uint32_t version;       // 版本号
    uint32_t flags;         // 标志
    uint32_t entry_point;   // 入口点
    uint32_t source_size;   // 源码大小
} ASTCHeader;

/**
 * 执行Pipeline模块中的ASTC程序
 */
static int execute_astc_via_pipeline(LoadedModule* pipeline_module, const char* astc_file, int argc, char* argv[]) {
    if (!pipeline_module || !astc_file) {
        printf("Loader: 错误: 无效的参数\n");
        return -1;
    }

    printf("Loader: 通过Pipeline模块执行ASTC程序: %s\n", astc_file);

    // 尝试查找vm_execute_astc函数
    typedef int (*vm_execute_astc_func_t)(const char*, int, char**);
    vm_execute_astc_func_t vm_execute_astc = NULL;

    // 查找Pipeline模块的执行函数
    if (pipeline_module->base_addr && pipeline_module->exports) {
        // 首先尝试vm_execute_astc函数（正确的签名）
        for (uint32_t i = 0; i < pipeline_module->header->export_count; i++) {
            if (strcmp(pipeline_module->exports[i].name, "vm_execute_astc") == 0) {
                vm_execute_astc = (vm_execute_astc_func_t)((char*)pipeline_module->base_addr + pipeline_module->exports[i].offset);
                printf("Loader: 找到vm_execute_astc函数，偏移: %u\n", pipeline_module->exports[i].offset);
                break;
            }
        }

        // 如果没找到vm_execute_astc，尝试execute_astc
        if (!vm_execute_astc) {
            for (uint32_t i = 0; i < pipeline_module->header->export_count; i++) {
                if (strcmp(pipeline_module->exports[i].name, "execute_astc") == 0) {
                    vm_execute_astc = (vm_execute_astc_func_t)((char*)pipeline_module->base_addr + pipeline_module->exports[i].offset);
                    printf("Loader: 找到execute_astc函数，偏移: %u\n", pipeline_module->exports[i].offset);
                    break;
                }
            }
        }

        // 如果还没找到，尝试native_main
        if (!vm_execute_astc) {
            for (uint32_t i = 0; i < pipeline_module->header->export_count; i++) {
                if (strcmp(pipeline_module->exports[i].name, "native_main") == 0) {
                    vm_execute_astc = (vm_execute_astc_func_t)((char*)pipeline_module->base_addr + pipeline_module->exports[i].offset);
                    printf("Loader: 找到native_main函数，偏移: %u\n", pipeline_module->exports[i].offset);
                    break;
                }
            }
        }

        if (vm_execute_astc) {
            printf("Loader: 找到Pipeline模块的执行函数\n");
            printf("Loader: 三层架构执行:\n");
            printf("  Layer 1: simple_loader (当前程序)\n");
            printf("  Layer 2: Pipeline模块\n");
            printf("  Layer 3: %s (ASTC程序)\n", astc_file);

            printf("Loader: 即将调用Pipeline模块函数...\n");
            fflush(stdout);  // 确保输出被刷新

            // 调用Pipeline模块的执行函数
            int result = vm_execute_astc(astc_file, argc, argv);

            printf("Loader: Pipeline模块执行完成，返回值: %d\n", result);
            return result;
        }
    }

    printf("Loader: 错误: 无法找到Pipeline模块的执行函数\n");
    return -1;
}

/**
 * 加载native模块
 */
static LoadedModule* load_native_module(const char* module_path) {
    printf("Loader: 尝试加载模块 %s\n", module_path);
    fflush(stdout);

    // 打开文件
    int fd = open(module_path, O_RDONLY);
    if (fd == -1) {
        printf("Loader: 警告: 无法打开模块文件 %s: %s\n", module_path, strerror(errno));
        return NULL;
    }

    printf("Loader: 文件打开成功，fd=%d\n", fd);
    fflush(stdout);
    
    // 获取文件大小
    struct stat st;
    if (fstat(fd, &st) == -1) {
        printf("Loader: 警告: 获取文件大小失败: %s\n", strerror(errno));
        close(fd);
        return NULL;
    }
    
    size_t file_size = st.st_size;
    printf("Loader: 模块文件大小: %zu 字节\n", file_size);
    
    // 映射文件到内存
    void* mapped = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    close(fd);
    
    if (mapped == MAP_FAILED) {
        printf("Loader: 警告: 内存映射失败: %s\n", strerror(errno));
        return NULL;
    }
    
    // 设置执行权限
    if (mprotect(mapped, file_size, PROT_READ | PROT_EXEC) != 0) {
        printf("Loader: 警告: 设置执行权限失败: %s\n", strerror(errno));
        // 继续执行，某些系统可能不允许设置执行权限
    }
    
    printf("Loader: 模块映射到地址: %p\n", mapped);
    
    // 验证头部
    NativeHeader* header = (NativeHeader*)mapped;
    if (memcmp(header->magic, "NATV", 4) != 0) {
        printf("Loader: 警告: 无效的模块格式 (magic: %.4s)\n", header->magic);
        munmap(mapped, file_size);
        return NULL;
    }
    
    printf("Loader: 模块信息:\n");
    printf("  版本: %u\n", header->version);
    printf("  架构: %u\n", header->arch);
    printf("  模块类型: %u\n", header->module_type);
    printf("  代码大小: %u 字节\n", header->code_size);
    printf("  导出数量: %u\n", header->export_count);
    printf("  导出表偏移: %u\n", header->export_offset);
    
    // 验证文件大小
    if (header->export_offset + header->export_count * sizeof(ExportEntry) > file_size) {
        printf("Loader: 警告: 模块文件损坏\n");
        munmap(mapped, file_size);
        return NULL;
    }
    
    // 创建模块结构
    LoadedModule* module = malloc(sizeof(LoadedModule));
    if (!module) {
        printf("Loader: 警告: 内存分配失败\n");
        munmap(mapped, file_size);
        return NULL;
    }
    
    module->base_addr = mapped;
    module->size = file_size;
    module->header = header;
    module->exports = (ExportEntry*)((char*)mapped + header->export_offset);
    module->code_section = (char*)mapped + header->header_size;
    
    printf("Loader: 模块加载成功\n");
    printf("  基地址: %p\n", module->base_addr);
    printf("  代码段: %p\n", module->code_section);
    printf("  导出表: %p\n", module->exports);
    
    // 打印导出函数
    printf("Loader: 导出函数:\n");
    for (uint32_t i = 0; i < header->export_count; i++) {
        printf("  [%u] %s (偏移: %u)\n", i, module->exports[i].name, module->exports[i].offset);
    }
    
    return module;
}

/**
 * 卸载native模块
 */
static void unload_native_module(LoadedModule* module) {
    if (module) {
        if (module->base_addr && module->base_addr != MAP_FAILED) {
            munmap(module->base_addr, module->size);
        }
        free(module);
    }
}

/**
 * 解析导出函数
 */
static void* resolve_export(LoadedModule* module, const char* function_name) {
    if (!module || !function_name) {
        return NULL;
    }
    
    printf("Loader: 查找导出函数 '%s'\n", function_name);
    
    for (uint32_t i = 0; i < module->header->export_count; i++) {
        if (strcmp(module->exports[i].name, function_name) == 0) {
            // 计算函数地址
            void* func_addr = (char*)module->code_section + module->exports[i].offset;
            printf("Loader: 找到函数 '%s' 地址: %p (偏移: %u)\n", 
                   function_name, func_addr, module->exports[i].offset);
            return func_addr;
        }
    }
    
    printf("Loader: 警告: 未找到导出函数 '%s'\n", function_name);
    return NULL;
}

/**
 * 主函数
 */
int main(int argc, char* argv[]) {
    printf("Simple Loader v2.1 - 三层架构加载器\n");
    printf("架构: %s_%d\n", ARCH_NAME, ARCH_BITS);
    
    if (argc < 2) {
        printf("用法: %s <program.astc> [args...]\n", argv[0]);
        return 1;
    }
    
    const char* astc_file = argv[1];
    printf("Loader: 目标程序: %s\n", astc_file);
    
    // 构建Pipeline模块路径（包含VM执行功能）
    char pipeline_module_path[256];
    snprintf(pipeline_module_path, sizeof(pipeline_module_path), "bin/pipeline_%s_%d.native", ARCH_NAME, ARCH_BITS);

    printf("Loader: Pipeline模块路径: %s\n", pipeline_module_path);

    // 尝试加载Pipeline模块
    printf("Loader: 正在加载Pipeline模块...\n");
    fflush(stdout);

    LoadedModule* pipeline_module = load_native_module(pipeline_module_path);

    if (!pipeline_module) {
        printf("Loader: 错误: 无法加载Pipeline模块 %s\n", pipeline_module_path);
        printf("Loader: 这可能是因为Pipeline模块存在问题或文件不存在\n");
        printf("Loader: 请检查Pipeline模块是否正确构建\n");
        return -1;
    }

    printf("Loader: Pipeline模块加载成功\n");
    fflush(stdout);

    // 通过Pipeline模块执行ASTC程序
    printf("Loader: 准备调用Pipeline模块执行函数...\n");
    int result = execute_astc_via_pipeline(pipeline_module, astc_file, argc - 1, argv + 1);
    printf("Loader: Pipeline模块调用返回，结果: %d\n", result);

    // 清理资源
    if (pipeline_module) {
        if (pipeline_module->base_addr) {
            munmap(pipeline_module->base_addr, pipeline_module->size);
        }
        if (pipeline_module->header) {
            free(pipeline_module->header);
        }
        if (pipeline_module->exports) {
            free(pipeline_module->exports);
        }
        free(pipeline_module);
    }

    return result;
}
