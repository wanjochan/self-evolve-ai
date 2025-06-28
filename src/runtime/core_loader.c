/**
 * loader.c - 跨平台Loader实现 (PRD.md三层架构第一层)
 *
 * 职责：
 * 1. 自动检测硬件架构和操作系统
 * 2. 自动选择对应的runtime{arch}{bits}.rt文件
 * 3. 加载Program.astc文件
 * 4. 提供统一的入口点，简化部署和使用
 * 5. 启动Runtime并传递Program
 *
 * 符合PRD.md要求：单一加载器，跨架构支持
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "platform.h"

// ===============================================
// 文件格式定义
// ===============================================

#define ASTC_MAGIC "ASTC"
#define RUNTIME_MAGIC "RTME"

typedef struct {
    char magic[4];          // "ASTC"
    uint32_t version;       // 版本号
    uint32_t size;          // 数据大小
    uint32_t entry_point;   // 入口点
} ASTCHeader;

typedef struct {
    char magic[4];          // "RTME" 
    uint32_t version;       // 版本号
    uint32_t size;          // 代码大小
    uint32_t entry_point;   // 入口点偏移
} RuntimeHeader;

// ===============================================
// 架构检测和Runtime选择
// ===============================================

// 运行时检测操作系统
const char* detect_operating_system(void) {
    // 通过文件系统特征检测操作系统
    FILE* f;

    // 检测Windows - 尝试访问Windows特有的文件
    f = fopen("C:\\Windows\\System32\\kernel32.dll", "rb");
    if (f) {
        fclose(f);
        return "windows";
    }

    // 检测Linux - 尝试访问Linux特有的文件
    f = fopen("/proc/version", "r");
    if (f) {
        fclose(f);
        return "linux";
    }

    // 检测macOS - 尝试访问macOS特有的文件
    f = fopen("/System/Library/CoreServices/SystemVersion.plist", "r");
    if (f) {
        fclose(f);
        return "macos";
    }

    return "unknown";
}

// 运行时检测CPU架构
const char* detect_cpu_architecture(void) {
    const char* os = detect_operating_system();

    if (strcmp(os, "windows") == 0) {
        // Windows: 尝试调用GetNativeSystemInfo (如果可用)
        // 这里需要动态加载kernel32.dll来避免编译时依赖
        // 暂时用简单方法
        return sizeof(void*) == 8 ? "x64" : "x86";
    } else if (strcmp(os, "linux") == 0) {
        // Linux: 读取 /proc/cpuinfo
        FILE* f = fopen("/proc/cpuinfo", "r");
        if (f) {
            char line[256];
            while (fgets(line, sizeof(line), f)) {
                if (strstr(line, "aarch64") || strstr(line, "arm64")) {
                    fclose(f);
                    return "arm64";
                }
                if (strstr(line, "armv7") || strstr(line, "armv6")) {
                    fclose(f);
                    return "arm";
                }
            }
            fclose(f);
        }
        return sizeof(void*) == 8 ? "x64" : "x86";
    } else {
        // 其他系统，使用指针大小推测
        return sizeof(void*) == 8 ? "x64" : "x86";
    }
}

typedef struct {
    const char* arch;           // 架构名称 (x64, arm64, etc.)
    const char* os;             // 操作系统 (windows, linux, macos)
    int bits;                   // 位数 (32, 64)
    const char* runtime_file;   // 自动选择的runtime文件
} PlatformInfo;

// 运行时检测当前平台架构
PlatformInfo detect_platform(void) {
    PlatformInfo info = {0};

    // 运行时检测架构 - 通过指针大小和CPU特性
    info.bits = sizeof(void*) * 8;  // 32位或64位

    // 运行时检测CPU架构
    info.arch = detect_cpu_architecture();

    // 运行时检测操作系统
    info.os = detect_operating_system();

    return info;
}

// 构建runtime文件名
void build_runtime_filename(PlatformInfo* info, char* filename, size_t size) {
    snprintf(filename, size, "bin/runtime%s_%d.rt", info->arch, info->bits);
    info->runtime_file = filename;
}

// 根据程序文件名构建对应的runtime文件名
void build_runtime_filename_for_program(PlatformInfo* info, const char* program_file, char* filename, size_t size) {
    // 从程序文件名推断runtime名称
    // 例如: evolver0_program.astc -> evolver0_runtime_x64_64.rt
    //      c99_program.astc -> c99_runtime_x64_64.rt

    const char* basename = strrchr(program_file, '/');
    if (!basename) basename = strrchr(program_file, '\\');
    if (!basename) basename = program_file;
    else basename++; // 跳过路径分隔符

    // 查找程序名称前缀
    char runtime_prefix[128] = {0};
    const char* program_suffix = "_program.astc";
    const char* program_pos = strstr(basename, program_suffix);

    if (program_pos) {
        // 提取前缀 (例如: "evolver0" 或 "c99")
        size_t prefix_len = program_pos - basename;
        if (prefix_len < sizeof(runtime_prefix)) {
            strncpy(runtime_prefix, basename, prefix_len);
            runtime_prefix[prefix_len] = '\0';
        }
    } else {
        // 如果没有找到标准后缀，使用默认前缀
        strcpy(runtime_prefix, "evolver0");
    }

    // 构建runtime文件名: {prefix}_runtime_{arch}_{bits}.rt
    snprintf(filename, size, "bin/%s_runtime_%s_%d.rt", runtime_prefix, info->arch, info->bits);
    info->runtime_file = filename;
}

// ===============================================
// 加载器选项
// ===============================================

typedef struct {
    const char* runtime_file;   // Runtime二进制文件
    const char* program_file;   // Program ASTC文件
    bool verbose;               // 详细输出
    bool debug;                 // 调试模式
    bool performance;           // 性能监控
} LoaderOptions;

// 性能统计
typedef struct {
    clock_t start_time;
    clock_t load_runtime_time;
    clock_t load_program_time;
    clock_t execute_time;
    clock_t end_time;
} PerformanceStats;

// ===============================================
// 文件加载函数
// ===============================================

static void* load_file(const char* filename, size_t* size) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "错误: 无法打开文件: %s\n", filename);
        return NULL;
    }
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // 分配内存并读取
    void* data = malloc(*size);
    if (!data) {
        fprintf(stderr, "错误: 内存分配失败\n");
        fclose(file);
        return NULL;
    }
    
    if (fread(data, 1, *size, file) != *size) {
        fprintf(stderr, "错误: 无法读取文件: %s\n", filename);
        free(data);
        fclose(file);
        return NULL;
    }
    
    fclose(file);
    return data;
}

// 检查文件是否存在
static bool file_exists(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return false;
    }
    fclose(file);
    return true;
}

// ===============================================
// Runtime加载和执行
// ===============================================

static int load_and_execute_runtime(const LoaderOptions* options, PerformanceStats* stats) {
    if (options->verbose) {
        printf("Evolver0 Loader - 三层架构实现\n");
        printf("Runtime: %s\n", options->runtime_file);
        printf("Program: %s\n", options->program_file);
    }
    
    // 步骤1: 加载Runtime二进制
    if (options->verbose) {
        printf("步骤1: 加载Runtime二进制...\n");
    }
    
    clock_t step_start = clock();
    
    size_t runtime_size;
    void* runtime_data = load_file(options->runtime_file, &runtime_size);
    if (!runtime_data) {
        fprintf(stderr, "错误: 无法加载Runtime文件\n");
        return 1;
    }
    
    if (stats) {
        stats->load_runtime_time = clock() - step_start;
    }
    
    // 验证Runtime格式 - 支持多种格式
    if (runtime_size < 16) {
        fprintf(stderr, "错误: Runtime文件太小\n");
        free(runtime_data);
        return 1;
    }

    // 检查并输出Runtime类型
    const char* runtime_type = "未知";
    bool valid_runtime = false;
    
    // 检查PE可执行文件格式
    if (memcmp(runtime_data, "MZ", 2) == 0) {
        runtime_type = "PE可执行文件";
        valid_runtime = true;
    }
    // 检查自包含runtime格式
    else if (memcmp(runtime_data, "EVOLVER0_RUNTIME", 16) == 0) {
        runtime_type = "自包含ASTC虚拟机";
        valid_runtime = true;
    }
    // 检查RTME格式
    else if (runtime_size >= sizeof(RuntimeHeader) && memcmp(runtime_data, RUNTIME_MAGIC, 4) == 0) {
        runtime_type = "RTME格式";
        valid_runtime = true;
    }
    
    if (!valid_runtime) {
        fprintf(stderr, "错误: 无效的Runtime文件格式\n");
        free(runtime_data);
        return 1;
    }
    
    if (options->verbose) {
        printf("✓ Runtime类型: %s (%zu字节)\n", runtime_type, runtime_size);
    }
    
    // 步骤2: 加载Program ASTC
    if (options->verbose) {
        printf("步骤2: 加载Program ASTC...\n");
    }
    
    step_start = clock();
    
    size_t program_size;
    void* program_data = load_file(options->program_file, &program_size);
    if (!program_data) {
        fprintf(stderr, "错误: 无法加载Program文件\n");
        free(runtime_data);
        return 1;
    }
    
    if (stats) {
        stats->load_program_time = clock() - step_start;
    }
    
    // 验证ASTC格式
    if (program_size < sizeof(ASTCHeader)) {
        fprintf(stderr, "错误: 无效的ASTC文件格式\n");
        free(runtime_data);
        free(program_data);
        return 1;
    }
    
    ASTCHeader* astc_header = (ASTCHeader*)program_data;
    if (memcmp(astc_header->magic, ASTC_MAGIC, 4) != 0) {
        fprintf(stderr, "错误: 无效的ASTC魔数\n");
        free(runtime_data);
        free(program_data);
        return 1;
    }
    
    if (options->verbose) {
        printf("✓ Program已加载: %zu字节, 版本 %u\n", 
               program_size, astc_header->version);
    }
    
    // 步骤3: 执行Runtime
    if (options->verbose) {
        printf("步骤3: 执行Runtime和Program...\n");
    }
    
    step_start = clock();

    // 执行前的Runtime检查
    if (memcmp(runtime_data, RUNTIME_MAGIC, 4) != 0) {
        fprintf(stderr, "警告: Runtime不是标准RTME格式，尝试自动适配...\n");
    }

    // 简化实现：直接调用我们的runtime库来执行ASTC
    // 跳过Runtime头部，获取ASTC数据
    unsigned char* astc_data = (unsigned char*)program_data + sizeof(ASTCHeader);
    size_t astc_data_size = astc_header->size;

    if (options->verbose) {
        printf("执行ASTC数据: %zu字节\n", astc_data_size);
    }

    int result = 0;
    
    // 检查Runtime.rt格式
    if (memcmp(runtime_data, "RTME", 4) == 0) {
        // 有效的RTME格式Runtime
        RuntimeHeader* runtime_header = (RuntimeHeader*)runtime_data;
        uint32_t version = runtime_header->version;
        uint32_t code_size = runtime_header->size;
        uint32_t entry_offset = runtime_header->entry_point;
        
        if (options->verbose) {
            printf("调用RTME Runtime执行...\n");
            printf("  版本: %u\n", version);
            printf("  代码大小: %u字节\n", code_size);
            printf("  入口点偏移: %u\n", entry_offset);
        }
        
        // 提取Runtime机器码
        void* runtime_code = (uint8_t*)runtime_data + entry_offset;
        
        // 使用平台抽象层分配可执行内存
        void* exec_mem = platform_alloc_executable(code_size);
        if (!exec_mem) {
            fprintf(stderr, "错误: 无法分配可执行内存\n");
            free(runtime_data);
            free(program_data);
            return 1;
        }
        
        // 复制机器码到可执行内存
        memcpy(exec_mem, runtime_code, code_size);
        
        // 创建函数指针
        typedef int (*RuntimeFunc)(void* program_data, size_t program_size);
        RuntimeFunc runtime_func = (RuntimeFunc)exec_mem;
        
        if (options->debug) {
            printf("开始执行Runtime...\n");
        }
        
        // 调用Runtime执行ASTC程序
        result = runtime_func(program_data, program_size);
        
        // 清理可执行内存
        platform_free_executable(exec_mem, code_size);
    }
    else if (memcmp(runtime_data, "EVOLVER0_RUNTIME", 16) == 0) {
        // 自包含的Runtime格式
        uint32_t* header_data = (uint32_t*)((char*)runtime_data + 16);
        uint32_t astc_size = header_data[0];
        uint32_t entry_offset = header_data[1];
        
        if (options->verbose) {
            printf("调用自包含Runtime执行...\n");
            printf("  ASTC VM大小: %u字节\n", astc_size);
            printf("  入口点偏移: %u\n", entry_offset);
        }
        
        // 提取Runtime VM代码
        void* vm_code = (uint8_t*)runtime_data + entry_offset;
        
        // 创建函数指针
        typedef int (*RuntimeVMFunc)(void* program_data, size_t program_size);
        RuntimeVMFunc runtime_vm_func = (RuntimeVMFunc)vm_code;
        
        // 调用Runtime VM执行ASTC程序
        result = runtime_vm_func(program_data, program_size);
    }
    else if (memcmp(runtime_data, "MZ", 2) == 0) {
        // Windows PE可执行文件格式
        fprintf(stderr, "错误: 直接执行PE文件尚未实现，请使用RTME格式\n");
        free(runtime_data);
        free(program_data);
        return 1;
    }
    else {
        fprintf(stderr, "错误: 无法执行未知格式的Runtime\n");
        free(runtime_data);
        free(program_data);
        return 1;
    }
    
    if (stats) {
        stats->execute_time = clock() - step_start;
    }
    
    // 释放内存
    free(runtime_data);
    free(program_data);
    
    if (options->verbose) {
        printf("✓ Runtime执行完成，返回值: %d\n", result);
    }
    
    return result;
}

// ===============================================
// 加载器命令行处理
// ===============================================

static void print_usage(const char* program_name) {
    printf("用法: %s [选项] <program.astc>\n\n", program_name);
    printf("PRD.md三层架构统一加载器 - 自动检测平台并选择Runtime\n\n");
    printf("选项:\n");
    printf("  -v, --verbose     显示详细输出\n");
    printf("  -d, --debug       启用调试模式\n");
    printf("  -p, --performance 显示性能统计\n");
    printf("  -r, --runtime     手动指定runtime文件 (覆盖自动检测)\n");
    printf("  -h, --help        显示帮助信息\n\n");
    printf("示例:\n");
    printf("  %s evolver0_program.astc                    # 自动检测平台\n", program_name);
    printf("  %s -v evolver0_program.astc                 # 详细输出\n", program_name);
    printf("  %s -r custom.rt evolver0_program.astc       # 手动指定runtime\n", program_name);
}

static bool parse_arguments(int argc, char* argv[], LoaderOptions* options) {
    // 初始化默认选项
    options->runtime_file = NULL;
    options->program_file = NULL;
    options->verbose = false;
    options->debug = false;
    options->performance = false;

    // 遍历命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            options->verbose = true;
        }
        else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
            options->debug = true;
        }
        else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--performance") == 0) {
            options->performance = true;
        }
        else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--runtime") == 0) {
            if (i + 1 < argc) {
                options->runtime_file = argv[++i];
            } else {
                fprintf(stderr, "错误: -r 选项需要指定runtime文件\n");
                return false;
            }
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return false;
        }
        else if (argv[i][0] != '-') {
            // 非选项参数，只有program_file
            if (!options->program_file) {
                options->program_file = argv[i];
            } else {
                fprintf(stderr, "错误: 多余的参数: %s\n", argv[i]);
                return false;
            }
        }
    }

    // 验证必需的参数
    if (!options->program_file) {
        fprintf(stderr, "错误: 必须指定Program文件\n");
        print_usage(argv[0]);
        return false;
    }

    // 如果没有手动指定runtime，则自动检测
    static char auto_runtime_filename[256];
    if (!options->runtime_file) {
        PlatformInfo platform = detect_platform();
        build_runtime_filename_for_program(&platform, options->program_file, auto_runtime_filename, sizeof(auto_runtime_filename));
        options->runtime_file = auto_runtime_filename;

        if (options->verbose) {
            printf("自动检测平台: %s %s %d位\n", platform.os, platform.arch, platform.bits);
            printf("自动选择Runtime: %s\n", options->runtime_file);
        }
    }

    // 验证文件存在
    if (!file_exists(options->runtime_file)) {
        fprintf(stderr, "错误: Runtime文件不存在: %s\n", options->runtime_file);
        return false;
    }

    if (!file_exists(options->program_file)) {
        fprintf(stderr, "错误: Program文件不存在: %s\n", options->program_file);
        return false;
    }

    return true;
}

int main(int argc, char* argv[]) {
    LoaderOptions options;
    PerformanceStats stats = {0};

    // 记录启动时间
    stats.start_time = clock();

    // 解析命令行参数
    if (!parse_arguments(argc, argv, &options)) {
        return 1;
    }

    // 如果启用了性能统计，则收集统计数据
    PerformanceStats* stats_ptr = options.performance ? &stats : NULL;

    // 执行Loader主要逻辑
    int result = load_and_execute_runtime(&options, stats_ptr);

    // 记录结束时间
    if (stats_ptr) {
        stats.end_time = clock();

        // 输出性能统计
        printf("\n===== 性能统计 =====\n");
        printf("加载Runtime耗时: %.2f ms\n",
               (double)(stats.load_runtime_time) * 1000 / CLOCKS_PER_SEC);
        printf("加载Program耗时: %.2f ms\n",
               (double)(stats.load_program_time) * 1000 / CLOCKS_PER_SEC);
        printf("执行程序耗时: %.2f ms\n",
               (double)(stats.execute_time) * 1000 / CLOCKS_PER_SEC);
        printf("总耗时: %.2f ms\n",
               (double)(stats.end_time - stats.start_time) * 1000 / CLOCKS_PER_SEC);
        printf("=====================\n");
    }

    return result;
}
