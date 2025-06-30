/**
 * tool_rt_builder.c - 标准化.rt文件构建工具
 * 
 * 用于创建符合标准的.rt运行时文件
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "runtime/rt_format_standard.h"

// 工具版本信息
#define TOOL_VERSION "1.0.0"
#define TOOL_NAME "rt_builder"

// 命令行选项
typedef struct {
    char* input_file;           // 输入文件 (机器码或ASTC)
    char* output_file;          // 输出.rt文件
    RTArchitecture arch;        // 目标架构
    RTOperatingSystem os;       // 目标操作系统
    RTABI abi;                 // ABI约定
    int optimization_level;     // 优化级别
    bool verbose;              // 详细输出
    bool include_debug;        // 包含调试信息
    bool compress;             // 压缩代码段
    uint32_t stack_size;       // 栈大小
    uint32_t heap_size;        // 堆大小
} BuildOptions;

// 显示帮助信息
void show_help(const char* program_name) {
    printf("RT File Builder v%s - 标准化.rt文件构建工具\n", TOOL_VERSION);
    printf("用法: %s [选项] <输入文件> <输出文件>\n\n", program_name);
    printf("选项:\n");
    printf("  -a, --arch <arch>     目标架构 (x86_64, arm64, x86_32, arm32)\n");
    printf("  -o, --os <os>         目标操作系统 (windows, linux, macos)\n");
    printf("  -b, --abi <abi>       ABI约定 (sysv, win64, aapcs, aapcs64)\n");
    printf("  -O, --optimize <n>    优化级别 (0-3)\n");
    printf("  -s, --stack <size>    最小栈大小 (字节)\n");
    printf("  -h, --heap <size>     最小堆大小 (字节)\n");
    printf("  -g, --debug           包含调试信息\n");
    printf("  -z, --compress        压缩代码段\n");
    printf("  -v, --verbose         详细输出\n");
    printf("  --help                显示此帮助信息\n\n");
    printf("示例:\n");
    printf("  %s -a x86_64 -o linux -O2 program.bin program.rt\n", program_name);
    printf("  %s --arch arm64 --os macos --debug app.o app.rt\n", program_name);
}

// 解析架构字符串
RTArchitecture parse_architecture(const char* arch_str) {
    if (strcmp(arch_str, "x86_64") == 0 || strcmp(arch_str, "amd64") == 0) {
        return RT_ARCH_X86_64;
    } else if (strcmp(arch_str, "x86_32") == 0 || strcmp(arch_str, "i386") == 0) {
        return RT_ARCH_X86_32;
    } else if (strcmp(arch_str, "arm64") == 0 || strcmp(arch_str, "aarch64") == 0) {
        return RT_ARCH_ARM64;
    } else if (strcmp(arch_str, "arm32") == 0 || strcmp(arch_str, "arm") == 0) {
        return RT_ARCH_ARM32;
    } else if (strcmp(arch_str, "riscv64") == 0) {
        return RT_ARCH_RISCV64;
    } else if (strcmp(arch_str, "riscv32") == 0) {
        return RT_ARCH_RISCV32;
    } else if (strcmp(arch_str, "wasm32") == 0) {
        return RT_ARCH_WASM32;
    } else if (strcmp(arch_str, "wasm64") == 0) {
        return RT_ARCH_WASM64;
    }
    return RT_ARCH_UNKNOWN;
}

// 解析操作系统字符串
RTOperatingSystem parse_os(const char* os_str) {
    if (strcmp(os_str, "windows") == 0 || strcmp(os_str, "win") == 0) {
        return RT_OS_WINDOWS;
    } else if (strcmp(os_str, "linux") == 0) {
        return RT_OS_LINUX;
    } else if (strcmp(os_str, "macos") == 0 || strcmp(os_str, "darwin") == 0) {
        return RT_OS_MACOS;
    } else if (strcmp(os_str, "freebsd") == 0) {
        return RT_OS_FREEBSD;
    } else if (strcmp(os_str, "android") == 0) {
        return RT_OS_ANDROID;
    } else if (strcmp(os_str, "ios") == 0) {
        return RT_OS_IOS;
    } else if (strcmp(os_str, "bare") == 0) {
        return RT_OS_BARE_METAL;
    }
    return RT_OS_UNKNOWN;
}

// 解析ABI字符串
RTABI parse_abi(const char* abi_str) {
    if (strcmp(abi_str, "sysv") == 0) {
        return RT_ABI_SYSV;
    } else if (strcmp(abi_str, "win64") == 0) {
        return RT_ABI_WIN64;
    } else if (strcmp(abi_str, "aapcs") == 0) {
        return RT_ABI_AAPCS;
    } else if (strcmp(abi_str, "aapcs64") == 0) {
        return RT_ABI_AAPCS64;
    } else if (strcmp(abi_str, "riscv") == 0) {
        return RT_ABI_RISCV;
    } else if (strcmp(abi_str, "wasm") == 0) {
        return RT_ABI_WASM;
    }
    return RT_ABI_UNKNOWN;
}

// 自动检测架构和操作系统
void auto_detect_platform(BuildOptions* opts) {
    if (opts->arch == RT_ARCH_UNKNOWN) {
        opts->arch = rt_detect_architecture();
    }
    if (opts->os == RT_OS_UNKNOWN) {
        opts->os = rt_detect_os();
    }
    if (opts->abi == RT_ABI_UNKNOWN) {
        opts->abi = rt_detect_abi();
    }
}

// 读取输入文件
int read_input_file(const char* filename, uint8_t** data, size_t* size) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        printf("错误: 无法打开输入文件 %s\n", filename);
        return -1;
    }
    
    // 获取文件大小
    fseek(fp, 0, SEEK_END);
    *size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    // 分配内存并读取
    *data = malloc(*size);
    if (!*data) {
        printf("错误: 内存分配失败\n");
        fclose(fp);
        return -1;
    }
    
    if (fread(*data, 1, *size, fp) != *size) {
        printf("错误: 读取文件失败\n");
        free(*data);
        fclose(fp);
        return -1;
    }
    
    fclose(fp);
    return 0;
}

// 构建.rt文件
int build_rt_file(const BuildOptions* opts) {
    if (opts->verbose) {
        printf("开始构建.rt文件...\n");
        printf("输入文件: %s\n", opts->input_file);
        printf("输出文件: %s\n", opts->output_file);
        printf("目标架构: %d\n", opts->arch);
        printf("目标操作系统: %d\n", opts->os);
        printf("ABI约定: %d\n", opts->abi);
    }
    
    // 读取输入文件
    uint8_t* input_data;
    size_t input_size;
    if (read_input_file(opts->input_file, &input_data, &input_size) != 0) {
        return -1;
    }
    
    // 创建文件头
    RTFileHeader* header = rt_create_header(opts->arch, opts->os, opts->abi);
    if (!header) {
        printf("错误: 创建文件头失败\n");
        free(input_data);
        return -1;
    }
    
    // 设置优化和特性标志
    header->optimization_level = opts->optimization_level;
    header->min_stack_size = opts->stack_size;
    header->min_heap_size = opts->heap_size;
    
    if (opts->include_debug) {
        header->flags |= RT_FLAG_DEBUG_INFO;
        header->feature_flags |= RT_FEATURE_PROFILING;
    }
    
    if (opts->compress) {
        header->flags |= RT_FLAG_COMPRESSED;
    }
    
    // 创建元数据
    RTMetadata metadata = {0};
    strncpy(metadata.compiler_name, TOOL_NAME, sizeof(metadata.compiler_name) - 1);
    strncpy(metadata.compiler_version, TOOL_VERSION, sizeof(metadata.compiler_version) - 1);
    
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(metadata.build_date, sizeof(metadata.build_date), "%Y-%m-%d", tm_info);
    
    snprintf(metadata.build_flags, sizeof(metadata.build_flags), "-O%d%s%s", 
             opts->optimization_level,
             opts->include_debug ? " -g" : "",
             opts->compress ? " -z" : "");
    
    metadata.required_runtime_version = header->runtime_version;
    metadata.compilation_time_ms = 0; // 将在实际编译时设置
    
    // 写入.rt文件
    int result = rt_write_file(opts->output_file, header, 
                              input_data, input_size,
                              NULL, 0, // 暂时没有数据段
                              &metadata);
    
    if (result == 0) {
        printf("成功创建.rt文件: %s (%zu字节代码)\n", opts->output_file, input_size);
        
        if (opts->verbose) {
            printf("文件头大小: %u字节\n", header->header_size);
            printf("代码段大小: %u字节\n", header->code_size);
            printf("元数据大小: %u字节\n", header->metadata_size);
            printf("优化级别: %u\n", header->optimization_level);
            printf("特性标志: 0x%08X\n", header->feature_flags);
        }
    } else {
        printf("错误: 创建.rt文件失败\n");
    }
    
    // 清理
    free(input_data);
    free(header);
    
    return result;
}

int main(int argc, char* argv[]) {
    BuildOptions opts = {
        .arch = RT_ARCH_UNKNOWN,
        .os = RT_OS_UNKNOWN,
        .abi = RT_ABI_UNKNOWN,
        .optimization_level = 1,
        .verbose = false,
        .include_debug = false,
        .compress = false,
        .stack_size = 64 * 1024,
        .heap_size = 1024 * 1024
    };
    
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            show_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            opts.verbose = true;
        } else if (strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--debug") == 0) {
            opts.include_debug = true;
        } else if (strcmp(argv[i], "-z") == 0 || strcmp(argv[i], "--compress") == 0) {
            opts.compress = true;
        } else if ((strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--arch") == 0) && i + 1 < argc) {
            opts.arch = parse_architecture(argv[++i]);
        } else if ((strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--os") == 0) && i + 1 < argc) {
            opts.os = parse_os(argv[++i]);
        } else if ((strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--abi") == 0) && i + 1 < argc) {
            opts.abi = parse_abi(argv[++i]);
        } else if ((strcmp(argv[i], "-O") == 0 || strcmp(argv[i], "--optimize") == 0) && i + 1 < argc) {
            opts.optimization_level = atoi(argv[++i]);
        } else if ((strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--stack") == 0) && i + 1 < argc) {
            opts.stack_size = atoi(argv[++i]);
        } else if ((strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--heap") == 0) && i + 1 < argc) {
            opts.heap_size = atoi(argv[++i]);
        } else if (!opts.input_file) {
            opts.input_file = argv[i];
        } else if (!opts.output_file) {
            opts.output_file = argv[i];
        }
    }
    
    // 检查必需参数
    if (!opts.input_file || !opts.output_file) {
        printf("错误: 需要指定输入文件和输出文件\n");
        show_help(argv[0]);
        return 1;
    }
    
    // 自动检测平台
    auto_detect_platform(&opts);
    
    // 构建.rt文件
    return build_rt_file(&opts);
}
