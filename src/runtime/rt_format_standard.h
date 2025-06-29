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
// 文件头结构
// ===============================================

typedef struct {
    char magic[4];                       // "RTME"
    uint8_t version_major;               // 主版本号
    uint8_t version_minor;               // 次版本号
    uint8_t version_patch;               // 补丁版本号
    uint8_t flags;                       // 标志位
    
    RTArchitecture architecture;         // 目标架构
    RTOperatingSystem os;                // 目标操作系统
    RTABI abi;                          // ABI约定
    uint8_t reserved1;                   // 保留字段
    
    uint32_t header_size;                // 头部大小
    uint32_t code_size;                  // 代码段大小
    uint32_t data_size;                  // 数据段大小
    uint32_t metadata_size;              // 元数据大小
    
    uint32_t entry_point;                // 入口点偏移
    uint32_t code_offset;                // 代码段偏移
    uint32_t data_offset;                // 数据段偏移
    uint32_t metadata_offset;            // 元数据偏移
    
    uint32_t checksum;                   // 文件校验和
    uint32_t timestamp;                  // 创建时间戳
    uint64_t reserved2;                  // 保留字段
} RTFileHeader;

// ===============================================
// 元数据结构
// ===============================================

typedef struct {
    uint32_t libc_version;               // 需要的libc版本
    uint32_t min_stack_size;             // 最小栈大小
    uint32_t min_heap_size;              // 最小堆大小
    uint32_t optimization_level;         // 优化级别
    
    uint16_t dependency_count;           // 依赖数量
    uint16_t symbol_count;               // 符号数量
    uint32_t symbol_table_offset;        // 符号表偏移
    
    char compiler_name[32];              // 编译器名称
    char compiler_version[16];           // 编译器版本
    char build_date[16];                 // 构建日期
    char build_flags[64];                // 构建标志
} RTMetadata;

typedef struct {
    char name[32];                       // 依赖名称
    uint8_t version_major;               // 主版本号
    uint8_t version_minor;               // 次版本号
    uint8_t version_patch;               // 补丁版本号
    uint8_t flags;                       // 标志位
} RTDependency;

// ===============================================
// 标志位定义
// ===============================================

#define RT_FLAG_COMPRESSED    0x01       // 代码段已压缩
#define RT_FLAG_ENCRYPTED     0x02       // 代码段已加密
#define RT_FLAG_DEBUG_INFO    0x04       // 包含调试信息
#define RT_FLAG_OPTIMIZED     0x08       // 已优化
#define RT_FLAG_RELOCATABLE   0x10       // 可重定位
#define RT_FLAG_SHARED        0x20       // 共享库
#define RT_FLAG_EXECUTABLE    0x40       // 可执行文件
#define RT_FLAG_RESERVED      0x80       // 保留

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
