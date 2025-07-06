/**
 * build_native_module_standalone.c - Standalone Native Module Builder
 * 
 * A standalone tool to create .native module files from object files.
 * This version doesn't depend on the module system.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/stat.h>

// ===============================================
// Native Module Types (copied from native_module.c)
// ===============================================

// Magic number for .native files: "NATV"
#define NATIVE_MAGIC 0x5654414E

// Current format version
#define NATIVE_VERSION_V1 1

// Maximum number of exports per module
#define NATIVE_MAX_EXPORTS 1024

// Maximum length of export names
#define NATIVE_MAX_NAME_LENGTH 256

// Architecture types
typedef enum {
    NATIVE_ARCH_X86_64 = 1,
    NATIVE_ARCH_ARM64 = 2,
    NATIVE_ARCH_X86_32 = 3
} NativeArchitecture;

// Module types
typedef enum {
    NATIVE_TYPE_VM = 1,        // VM core module
    NATIVE_TYPE_LIBC = 2,      // libc forwarding module
    NATIVE_TYPE_USER = 3       // User-defined module
} NativeModuleType;

// Export types
typedef enum {
    NATIVE_EXPORT_FUNCTION = 1,
    NATIVE_EXPORT_VARIABLE = 2,
    NATIVE_EXPORT_CONSTANT = 3,
    NATIVE_EXPORT_TYPE = 4,
    NATIVE_EXPORT_INTERFACE = 5
} NativeExportType;

// .native file header (128 bytes, aligned)
typedef struct {
    uint32_t magic;              // Magic number: "NATV" (0x5654414E)
    uint32_t version;            // Format version (1)
    uint32_t architecture;       // NativeArchitecture
    uint32_t module_type;        // NativeModuleType
    uint64_t code_size;          // Size of code section
    uint64_t data_size;          // Size of data section
    uint64_t code_offset;        // Offset to code section
    uint64_t data_offset;        // Offset to data section
    uint64_t export_table_offset; // Offset to export table
    uint32_t export_count;       // Number of exports
    uint32_t entry_point_offset;  // Offset to entry point in code section
    uint64_t metadata_offset;    // Offset to metadata
    uint64_t checksum;           // CRC64 checksum of code and data
    uint32_t flags;              // Module flags
    uint32_t relocation_count;   // Number of relocations
    uint64_t relocation_offset;  // Offset to relocation table
    uint8_t reserved[32];        // Reserved for future use
} NativeHeader;

// Export entry
typedef struct {
    char name[NATIVE_MAX_NAME_LENGTH];
    uint32_t type;               // NativeExportType
    uint32_t flags;              // Export flags
    uint64_t offset;             // Offset in code/data section
    uint64_t size;               // Size in bytes
} NativeExport;

// Simple CRC64 calculation
static uint64_t calculate_simple_crc64(const uint8_t* data, size_t length) {
    uint64_t crc = 0xFFFFFFFFFFFFFFFFULL;
    const uint64_t poly = 0xC96C5795D7870F42ULL;
    
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ poly;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return crc ^ 0xFFFFFFFFFFFFFFFFULL;
}

// Parse architecture string
static NativeArchitecture parse_architecture(const char* arch_str) {
    if (strcmp(arch_str, "x86_64") == 0) {
        return NATIVE_ARCH_X86_64;
    } else if (strcmp(arch_str, "arm64") == 0) {
        return NATIVE_ARCH_ARM64;
    } else if (strcmp(arch_str, "x86_32") == 0) {
        return NATIVE_ARCH_X86_32;
    }
    return 0; // Invalid
}

// Parse module type string
static NativeModuleType parse_module_type(const char* type_str) {
    if (strcmp(type_str, "vm") == 0) {
        return NATIVE_TYPE_VM;
    } else if (strcmp(type_str, "libc") == 0) {
        return NATIVE_TYPE_LIBC;
    } else if (strcmp(type_str, "user") == 0) {
        return NATIVE_TYPE_USER;
    }
    return 0; // Invalid
}

// Read file into buffer
static uint8_t* read_file(const char* filename, size_t* size) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "无法打开文件: %s\n", filename);
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate buffer
    uint8_t* buffer = malloc(*size);
    if (!buffer) {
        fprintf(stderr, "内存分配失败\n");
        fclose(file);
        return NULL;
    }
    
    // Read file
    if (fread(buffer, 1, *size, file) != *size) {
        fprintf(stderr, "读取文件失败: %s\n", filename);
        free(buffer);
        fclose(file);
        return NULL;
    }
    
    fclose(file);
    return buffer;
}

// Write native module to file
static int write_native_module(const char* filename, const uint8_t* code, size_t code_size,
                              NativeArchitecture arch, NativeModuleType type) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "无法创建输出文件: %s\n", filename);
        return -1;
    }
    
    // Create header
    NativeHeader header = {0};
    header.magic = NATIVE_MAGIC;
    header.version = NATIVE_VERSION_V1;
    header.architecture = arch;
    header.module_type = type;
    header.code_size = code_size;
    header.data_size = 0;
    header.code_offset = sizeof(NativeHeader);
    header.data_offset = 0;
    header.export_table_offset = sizeof(NativeHeader) + code_size;
    header.export_count = 0;
    header.entry_point_offset = 0;
    header.metadata_offset = 0;
    header.checksum = calculate_simple_crc64(code, code_size);
    header.flags = 0;
    header.relocation_count = 0;
    header.relocation_offset = 0;
    
    // Write header
    if (fwrite(&header, sizeof(NativeHeader), 1, file) != 1) {
        fprintf(stderr, "写入头部失败\n");
        fclose(file);
        return -1;
    }
    
    // Write code section
    if (fwrite(code, code_size, 1, file) != 1) {
        fprintf(stderr, "写入代码段失败\n");
        fclose(file);
        return -1;
    }
    
    // Write empty export table
    uint32_t export_count = 0;
    if (fwrite(&export_count, sizeof(uint32_t), 1, file) != 1) {
        fprintf(stderr, "写入导出表失败\n");
        fclose(file);
        return -1;
    }
    
    fclose(file);
    return 0;
}

// Print usage
static void print_usage(const char* program_name) {
    printf("用法: %s <输入.o文件> <输出.native文件> --arch=<架构> --type=<类型>\n", program_name);
    printf("\n");
    printf("参数:\n");
    printf("  <输入.o文件>     输入的目标文件\n");
    printf("  <输出.native文件> 输出的native模块文件\n");
    printf("  --arch=<架构>    目标架构 (x86_64, arm64, x86_32)\n");
    printf("  --type=<类型>    模块类型 (vm, libc, user)\n");
    printf("\n");
    printf("示例:\n");
    printf("  %s test.o test.native --arch=x86_64 --type=user\n", program_name);
}

// Main function
int main(int argc, char* argv[]) {
    if (argc != 5) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char* input_file = argv[1];
    const char* output_file = argv[2];
    const char* arch_arg = argv[3];
    const char* type_arg = argv[4];
    
    // Parse architecture argument
    if (strncmp(arch_arg, "--arch=", 7) != 0) {
        fprintf(stderr, "错误: 架构参数格式错误\n");
        print_usage(argv[0]);
        return 1;
    }
    
    NativeArchitecture arch = parse_architecture(arch_arg + 7);
    if (arch == 0) {
        fprintf(stderr, "错误: 不支持的架构: %s\n", arch_arg + 7);
        return 1;
    }
    
    // Parse type argument
    if (strncmp(type_arg, "--type=", 7) != 0) {
        fprintf(stderr, "错误: 类型参数格式错误\n");
        print_usage(argv[0]);
        return 1;
    }
    
    NativeModuleType type = parse_module_type(type_arg + 7);
    if (type == 0) {
        fprintf(stderr, "错误: 不支持的模块类型: %s\n", type_arg + 7);
        return 1;
    }
    
    // Read input file
    size_t code_size;
    uint8_t* code = read_file(input_file, &code_size);
    if (!code) {
        return 1;
    }
    
    printf("读取输入文件: %s (%zu 字节)\n", input_file, code_size);
    
    // Write native module
    if (write_native_module(output_file, code, code_size, arch, type) != 0) {
        free(code);
        return 1;
    }
    
    printf("成功创建native模块: %s\n", output_file);
    
    free(code);
    return 0;
}
