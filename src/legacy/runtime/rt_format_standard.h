/**
 * rt_format_standard.h - 标准化的.rt文件格式定义
 * 
 * 定义统一的Runtime文件格式，支持版本兼容性、架构检测和优化加载
 */

#ifndef RT_FORMAT_STANDARD_H
#define RT_FORMAT_STANDARD_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// 常量定义
// ===============================================

#define RT_MAGIC "RTME"                    // Runtime文件魔数
#define RT_VERSION_MAJOR 1                 // 主版本号
#define RT_VERSION_MINOR 0                 // 次版本号
#define RT_VERSION_PATCH 0                 // 补丁版本号

#define RT_MAX_METADATA_SIZE 1024          // 最大元数据大小
#define RT_MAX_DEPENDENCIES 16             // 最大依赖数量

// ===============================================
// 架构和平台定义
// ===============================================

// 支持的架构类型
typedef enum {
    RT_ARCH_UNKNOWN = 0,
    RT_ARCH_X86_32 = 1,
    RT_ARCH_X86_64 = 2,
    RT_ARCH_ARM32 = 3,
    RT_ARCH_ARM64 = 4,
    RT_ARCH_RISCV32 = 5,
    RT_ARCH_RISCV64 = 6,
    RT_ARCH_WASM32 = 7,
    RT_ARCH_WASM64 = 8
} RTArchitecture;

// 支持的操作系统
typedef enum {
    RT_OS_UNKNOWN = 0,
    RT_OS_WINDOWS = 1,
    RT_OS_LINUX = 2,
    RT_OS_MACOS = 3,
    RT_OS_FREEBSD = 4,
    RT_OS_OPENBSD = 5,
    RT_OS_NETBSD = 6,
    RT_OS_ANDROID = 7,
    RT_OS_IOS = 8,
    RT_OS_BARE_METAL = 9
} RTOperatingSystem;

// 支持的ABI
typedef enum {
    RT_ABI_UNKNOWN = 0,
    RT_ABI_SYSV = 1,        // System V ABI (Linux, Unix)
    RT_ABI_WIN64 = 2,       // Windows x64 ABI
    RT_ABI_AAPCS = 3,       // ARM AAPCS
    RT_ABI_AAPCS64 = 4,     // ARM64 AAPCS64
    RT_ABI_RISCV = 5,       // RISC-V ABI
    RT_ABI_WASM = 6         // WebAssembly ABI
} RTABI;

typedef enum {
    RT_ARCH_X86_32 = 0x01,               // x86 32位
    RT_ARCH_X86_64 = 0x02,               // x86 64位
    RT_ARCH_ARM32 = 0x03,                // ARM 32位
    RT_ARCH_ARM64 = 0x04,                // ARM 64位
    RT_ARCH_RISCV32 = 0x05,              // RISC-V 32位
    RT_ARCH_RISCV64 = 0x06,              // RISC-V 64位
    RT_ARCH_UNKNOWN = 0xFF               // 未知架构
} RTArchitecture;

typedef enum {
    RT_OS_WINDOWS = 0x01,                // Windows
    RT_OS_LINUX = 0x02,                  // Linux
    RT_OS_MACOS = 0x03,                  // macOS
    RT_OS_FREEBSD = 0x04,                // FreeBSD
    RT_OS_UNKNOWN = 0xFF                 // 未知操作系统
} RTOperatingSystem;

typedef enum {
    RT_ABI_SYSV = 0x01,                  // System V ABI
    RT_ABI_WIN64 = 0x02,                 // Windows x64 ABI
    RT_ABI_AAPCS = 0x03,                 // ARM AAPCS
    RT_ABI_UNKNOWN = 0xFF                // 未知ABI
} RTABI;

// ===============================================
// 文件头结构 (标准化版本 v1.0)
// ===============================================

typedef struct {
    // 基本标识 (16 bytes)
    char magic[4];                       // "RTME" - Runtime Module Executable
    uint8_t version_major;               // 主版本号 (当前: 1)
    uint8_t version_minor;               // 次版本号 (当前: 0)
    uint8_t version_patch;               // 补丁版本号 (当前: 0)
    uint8_t flags;                       // 标志位 (见RT_FLAG_*)

    // 平台信息 (8 bytes)
    RTArchitecture architecture;         // 目标架构 (4 bytes)
    RTOperatingSystem os;                // 目标操作系统 (1 byte)
    RTABI abi;                          // ABI约定 (1 byte)
    uint8_t endianness;                  // 字节序 (0=little, 1=big)
    uint8_t reserved1;                   // 保留字段

    // 段信息 (32 bytes)
    uint32_t header_size;                // 头部大小 (固定为128字节)
    uint32_t code_size;                  // 代码段大小
    uint32_t data_size;                  // 数据段大小
    uint32_t rodata_size;                // 只读数据段大小
    uint32_t bss_size;                   // BSS段大小
    uint32_t metadata_size;              // 元数据大小
    uint32_t symbol_table_size;          // 符号表大小
    uint32_t relocation_table_size;      // 重定位表大小

    // 偏移信息 (32 bytes)
    uint32_t entry_point;                // 入口点偏移 (相对于代码段)
    uint32_t code_offset;                // 代码段偏移
    uint32_t data_offset;                // 数据段偏移
    uint32_t rodata_offset;              // 只读数据段偏移
    uint32_t metadata_offset;            // 元数据偏移
    uint32_t symbol_table_offset;        // 符号表偏移
    uint32_t relocation_table_offset;    // 重定位表偏移
    uint32_t debug_info_offset;          // 调试信息偏移

    // 校验和版本信息 (16 bytes)
    uint32_t checksum;                   // 文件校验和 (CRC32)
    uint32_t timestamp;                  // 创建时间戳 (Unix时间)
    uint32_t compiler_version;           // 编译器版本
    uint32_t runtime_version;            // 需要的运行时版本

    // 性能和兼容性信息 (16 bytes)
    uint32_t min_stack_size;             // 最小栈大小 (字节)
    uint32_t min_heap_size;              // 最小堆大小 (字节)
    uint32_t optimization_level;         // 优化级别 (0-3)
    uint32_t feature_flags;              // 特性标志位

    // 保留字段 (8 bytes) - 总计128字节
    uint64_t reserved2;                  // 保留字段
} RTFileHeader;

// ===============================================
// 元数据结构 (扩展版本)
// ===============================================

typedef struct {
    // 依赖信息 (16 bytes)
    uint32_t libc_version;               // 需要的libc版本
    uint16_t dependency_count;           // 依赖模块数量
    uint16_t import_count;               // 导入函数数量
    uint16_t export_count;               // 导出函数数量
    uint16_t reserved1;                  // 保留字段
    uint32_t dependency_table_offset;    // 依赖表偏移

    // 编译信息 (64 bytes)
    char compiler_name[32];              // 编译器名称 ("c2astc", "gcc", "clang")
    char compiler_version[16];           // 编译器版本 ("1.0.0")
    char build_date[16];                 // 构建日期 ("2024-12-30")

    // 构建配置 (32 bytes)
    char build_flags[32];                // 构建标志 ("-O2 -g")
    uint32_t source_file_count;          // 源文件数量
    uint32_t source_line_count;          // 源代码行数
    uint32_t astc_instruction_count;     // ASTC指令数量
    uint32_t machine_instruction_count;  // 机器指令数量
    uint32_t optimization_passes;        // 优化遍数
    uint32_t compilation_time_ms;        // 编译时间(毫秒)
    uint32_t reserved2;                  // 保留字段

    // 运行时需求 (16 bytes)
    uint32_t required_runtime_version;   // 需要的运行时版本
    uint32_t required_libc_functions;    // 需要的libc函数数量
    uint32_t thread_safety_level;       // 线程安全级别
    uint32_t memory_model;               // 内存模型
} RTMetadata;

// ===============================================
// 符号表结构
// ===============================================

typedef enum {
    RT_SYMBOL_FUNCTION = 0,              // 函数符号
    RT_SYMBOL_VARIABLE = 1,              // 变量符号
    RT_SYMBOL_CONSTANT = 2,              // 常量符号
    RT_SYMBOL_TYPE = 3,                  // 类型符号
    RT_SYMBOL_LABEL = 4,                 // 标签符号
    RT_SYMBOL_SECTION = 5                // 段符号
} RTSymbolType;

typedef enum {
    RT_SYMBOL_LOCAL = 0,                 // 本地符号
    RT_SYMBOL_GLOBAL = 1,                // 全局符号
    RT_SYMBOL_WEAK = 2,                  // 弱符号
    RT_SYMBOL_EXTERNAL = 3               // 外部符号
} RTSymbolBinding;

typedef struct {
    uint32_t name_offset;                // 符号名在字符串表中的偏移
    uint32_t value;                      // 符号值 (地址/偏移)
    uint32_t size;                       // 符号大小
    RTSymbolType type;                   // 符号类型
    RTSymbolBinding binding;             // 符号绑定
    uint16_t section_index;              // 所属段索引
    uint16_t flags;                      // 符号标志
} RTSymbol;

// ===============================================
// 重定位表结构
// ===============================================

typedef enum {
    RT_RELOC_ABSOLUTE = 0,               // 绝对地址重定位
    RT_RELOC_RELATIVE = 1,               // 相对地址重定位
    RT_RELOC_GOT = 2,                    // GOT表重定位
    RT_RELOC_PLT = 3,                    // PLT表重定位
    RT_RELOC_SECTION = 4                 // 段相对重定位
} RTRelocationType;

typedef struct {
    uint32_t offset;                     // 重定位位置偏移
    uint32_t symbol_index;               // 符号表索引
    RTRelocationType type;               // 重定位类型
    int32_t addend;                      // 重定位加数
} RTRelocation;

typedef struct {
    char name[32];                       // 依赖名称
    uint8_t version_major;               // 主版本号
    uint8_t version_minor;               // 次版本号
    uint8_t version_patch;               // 补丁版本号
    uint8_t flags;                       // 标志位
} RTDependency;

// ===============================================
// 标志位定义 (扩展版本)
// ===============================================

// 基本标志位 (flags字段)
#define RT_FLAG_COMPRESSED    0x01       // 代码段已压缩
#define RT_FLAG_ENCRYPTED     0x02       // 代码段已加密
#define RT_FLAG_DEBUG_INFO    0x04       // 包含调试信息
#define RT_FLAG_OPTIMIZED     0x08       // 已优化
#define RT_FLAG_RELOCATABLE   0x10       // 可重定位
#define RT_FLAG_SHARED        0x20       // 共享库
#define RT_FLAG_EXECUTABLE    0x40       // 可执行文件
#define RT_FLAG_POSITION_INDEPENDENT 0x80 // 位置无关代码

// 特性标志位 (feature_flags字段)
#define RT_FEATURE_LIBC_FORWARDING   0x00000001  // libc函数转发
#define RT_FEATURE_DYNAMIC_LINKING   0x00000002  // 动态链接
#define RT_FEATURE_THREAD_SAFE       0x00000004  // 线程安全
#define RT_FEATURE_EXCEPTION_HANDLING 0x00000008 // 异常处理
#define RT_FEATURE_GARBAGE_COLLECTION 0x00000010 // 垃圾回收
#define RT_FEATURE_JIT_COMPILATION   0x00000020  // JIT编译
#define RT_FEATURE_PROFILING         0x00000040  // 性能分析
#define RT_FEATURE_SECURITY_CHECKS   0x00000080  // 安全检查
#define RT_FEATURE_MEMORY_PROTECTION 0x00000100  // 内存保护
#define RT_FEATURE_STACK_PROTECTION  0x00000200  // 栈保护
#define RT_FEATURE_CONTROL_FLOW_INTEGRITY 0x00000400 // 控制流完整性
#define RT_FEATURE_ADDRESS_SANITIZER 0x00000800  // 地址消毒器
#define RT_FEATURE_UNDEFINED_BEHAVIOR_SANITIZER 0x00001000 // 未定义行为消毒器
#define RT_FEATURE_MEMORY_SANITIZER  0x00002000  // 内存消毒器
#define RT_FEATURE_THREAD_SANITIZER  0x00004000  // 线程消毒器
#define RT_FEATURE_FUZZING_SUPPORT   0x00008000  // 模糊测试支持

// ===============================================
// 函数声明
// ===============================================

/**
 * 创建标准化的RT文件头
 */
RTFileHeader* rt_create_header(RTArchitecture arch, RTOperatingSystem os, RTABI abi);

/**
 * 验证RT文件头的有效性
 */
bool rt_validate_header(const RTFileHeader* header);

/**
 * 检查RT文件版本兼容性
 */
bool rt_check_compatibility(const RTFileHeader* header, 
                           RTArchitecture current_arch, 
                           RTOperatingSystem current_os);

/**
 * 计算RT文件校验和
 */
uint32_t rt_calculate_checksum(const void* data, size_t size);

/**
 * 获取架构名称字符串
 */
const char* rt_get_architecture_name(RTArchitecture arch);

/**
 * 获取操作系统名称字符串
 */
const char* rt_get_os_name(RTOperatingSystem os);

/**
 * 获取ABI名称字符串
 */
const char* rt_get_abi_name(RTABI abi);

/**
 * 检测当前运行时环境
 */
RTArchitecture rt_detect_architecture(void);
RTOperatingSystem rt_detect_os(void);
RTABI rt_detect_abi(void);

/**
 * 写入标准化RT文件
 */
int rt_write_file(const char* filename, 
                  const RTFileHeader* header,
                  const void* code, size_t code_size,
                  const void* data, size_t data_size,
                  const RTMetadata* metadata);

/**
 * 读取标准化RT文件
 */
int rt_read_file(const char* filename,
                 RTFileHeader** header,
                 void** code, size_t* code_size,
                 void** data, size_t* data_size,
                 RTMetadata** metadata);

/**
 * 优化RT文件大小
 */
int rt_optimize_file_size(const char* input_file, const char* output_file);

/**
 * 验证RT文件完整性
 */
bool rt_verify_integrity(const char* filename);

#ifdef __cplusplus
}
#endif

#endif // RT_FORMAT_STANDARD_H
