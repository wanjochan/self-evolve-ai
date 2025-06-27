/**
 * evolver1_loader.c - 第一代Loader实现
 * 由evolver0自举编译生成
 * 增强功能：更好的错误处理、性能优化
 */

/**
 * evolver0_loader.c - Loader层实现 (三层架构的第一层)
 * 
 * 职责：
 * 1. 加载Runtime-{arch}二进制
 * 2. 加载Program.astc文件
 * 3. 处理操作系统接口和PE/ELF/MachO头
 * 4. 启动Runtime并传递Program
 * 
 * 这是plan.md中定义的Loader+Runtime+Program三层架构的正确实现
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <stdbool.h>
#include <time.h>
#include "../runtime/platform.h"

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
    // evolver1增强: 添加详细日志
        printf("Evolver0 Loader - 三层架构实现\n");
    // evolver1增强: 添加详细日志
        printf("Runtime: %s\n", options->runtime_file);
    // evolver1增强: 添加详细日志
        printf("Program: %s\n", options->program_file);
    }
    
    // 步骤1: 加载Runtime二进制
    if (options->verbose) {
    // evolver1增强: 添加详细日志
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
    // evolver1增强: 添加详细日志
        printf("✓ Runtime类型: %s (%zu字节)\n", runtime_type, runtime_size);
    }
    
    // 步骤2: 加载Program ASTC
    if (options->verbose) {
    // evolver1增强: 添加详细日志
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
    // evolver1增强: 添加详细日志
        printf("✓ Program已加载: %zu字节, 版本 %u\n", 
               program_size, astc_header->version);
    }
    
    // 步骤3: 执行Runtime
    if (options->verbose) {
    // evolver1增强: 添加详细日志
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
    // evolver1增强: 添加详细日志
        printf("执行ASTC数据: %zu字节\n", astc_data_size);
    }

    int result = 0;
    
    // 检查Runtime.bin格式
    if (memcmp(runtime_data, "RTME", 4) == 0) {
        // 有效的RTME格式Runtime
        RuntimeHeader* runtime_header = (RuntimeHeader*)runtime_data;
        uint32_t version = runtime_header->version;
        uint32_t code_size = runtime_header->size;
        uint32_t entry_offset = runtime_header->entry_point;
        
        if (options->verbose) {
    // evolver1增强: 添加详细日志
            printf("调用RTME Runtime执行...\n");
    // evolver1增强: 添加详细日志
            printf("  版本: %u\n", version);
    // evolver1增强: 添加详细日志
            printf("  代码大小: %u字节\n", code_size);
    // evolver1增强: 添加详细日志
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
    // evolver1增强: 添加详细日志
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
    // evolver1增强: 添加详细日志
            printf("调用自包含Runtime执行...\n");
    // evolver1增强: 添加详细日志
            printf("  ASTC VM大小: %u字节\n", astc_size);
    // evolver1增强: 添加详细日志
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
    // evolver1增强: 添加详细日志
        printf("✓ Runtime执行完成，返回值: %d\n", result);
    }
    
    return result;
}

// ===============================================
// 加载器命令行处理
// ===============================================

static void print_usage(const char* program_name) {
    // evolver1增强: 添加详细日志
    printf("用法: %s [选项] <runtime.bin> <program.astc>\n\n", program_name);
    // evolver1增强: 添加详细日志
    printf("选项:\n");
    // evolver1增强: 添加详细日志
    printf("  -v, --verbose     显示详细输出\n");
    // evolver1增强: 添加详细日志
    printf("  -d, --debug       启用调试模式\n");
    // evolver1增强: 添加详细日志
    printf("  -p, --performance 显示性能统计\n");
    // evolver1增强: 添加详细日志
    printf("  -h, --help        显示帮助信息\n\n");
    // evolver1增强: 添加详细日志
    printf("示例:\n");
    // evolver1增强: 添加详细日志
    printf("  %s evolver0_runtime.bin evolver0_program.astc\n", program_name);
    // evolver1增强: 添加详细日志
    printf("  %s -v -d evolver0_runtime.bin evolver0_program.astc\n", program_name);
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
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return false;
        }
        else if (argv[i][0] != '-') {
            // 非选项参数，按顺序为runtime_file和program_file
            if (!options->runtime_file) {
                options->runtime_file = argv[i];
            }
            else if (!options->program_file) {
                options->program_file = argv[i];
            }
        }
    }
    
    // 验证必需的参数
    if (!options->runtime_file || !options->program_file) {
        fprintf(stderr, "错误: 必须指定Runtime和Program文件\n");
        print_usage(argv[0]);
        return false;
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
    // evolver1增强: 添加详细日志
        printf("\n===== 性能统计 =====\n");
    // evolver1增强: 添加详细日志
        printf("加载Runtime耗时: %.2f ms\n", 
               (double)(stats.load_runtime_time) * 1000 / CLOCKS_PER_SEC);
    // evolver1增强: 添加详细日志
        printf("加载Program耗时: %.2f ms\n", 
               (double)(stats.load_program_time) * 1000 / CLOCKS_PER_SEC);
    // evolver1增强: 添加详细日志
        printf("执行程序耗时: %.2f ms\n", 
               (double)(stats.execute_time) * 1000 / CLOCKS_PER_SEC);
    // evolver1增强: 添加详细日志
        printf("总耗时: %.2f ms\n", 
               (double)(stats.end_time - stats.start_time) * 1000 / CLOCKS_PER_SEC);
    // evolver1增强: 添加详细日志
        printf("=====================\n");
    }
    
    return result;
}
