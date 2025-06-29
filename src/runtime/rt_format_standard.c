/**
 * rt_format_standard.c - 标准化.rt文件格式实现
 */

#include "rt_format_standard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ===============================================
// 架构和平台检测
// ===============================================

RTArchitecture rt_detect_architecture(void) {
    #if defined(_M_X64) || defined(__x86_64__) || defined(__x86_64) || defined(__amd64__) || defined(__amd64)
        return RT_ARCH_X86_64;
    #elif defined(_M_IX86) || defined(__i386__) || defined(__i386) || defined(i386)
        return RT_ARCH_X86_32;
    #elif defined(_M_ARM64) || defined(__aarch64__)
        return RT_ARCH_ARM64;
    #elif defined(_M_ARM) || defined(__arm__) || defined(__arm)
        return RT_ARCH_ARM32;
    #elif defined(__riscv) && (__riscv_xlen == 64)
        return RT_ARCH_RISCV64;
    #elif defined(__riscv) && (__riscv_xlen == 32)
        return RT_ARCH_RISCV32;
    #else
        return RT_ARCH_UNKNOWN;
    #endif
}

RTOperatingSystem rt_detect_os(void) {
    #if defined(_WIN32) || defined(_WIN64)
        return RT_OS_WINDOWS;
    #elif defined(__linux__)
        return RT_OS_LINUX;
    #elif defined(__APPLE__) && defined(__MACH__)
        return RT_OS_MACOS;
    #elif defined(__FreeBSD__)
        return RT_OS_FREEBSD;
    #else
        return RT_OS_UNKNOWN;
    #endif
}

RTABI rt_detect_abi(void) {
    #if defined(_WIN64)
        return RT_ABI_WIN64;
    #elif defined(__x86_64__) && (defined(__linux__) || defined(__FreeBSD__))
        return RT_ABI_SYSV;
    #elif defined(__arm__) || defined(__aarch64__)
        return RT_ABI_AAPCS;
    #else
        return RT_ABI_UNKNOWN;
    #endif
}

// ===============================================
// 字符串转换函数
// ===============================================

const char* rt_get_architecture_name(RTArchitecture arch) {
    switch (arch) {
        case RT_ARCH_X86_32: return "x86_32";
        case RT_ARCH_X86_64: return "x86_64";
        case RT_ARCH_ARM32:  return "arm32";
        case RT_ARCH_ARM64:  return "arm64";
        case RT_ARCH_RISCV32: return "riscv32";
        case RT_ARCH_RISCV64: return "riscv64";
        default: return "unknown";
    }
}

const char* rt_get_os_name(RTOperatingSystem os) {
    switch (os) {
        case RT_OS_WINDOWS: return "windows";
        case RT_OS_LINUX:   return "linux";
        case RT_OS_MACOS:   return "macos";
        case RT_OS_FREEBSD: return "freebsd";
        default: return "unknown";
    }
}

const char* rt_get_abi_name(RTABI abi) {
    switch (abi) {
        case RT_ABI_SYSV:   return "sysv";
        case RT_ABI_WIN64:  return "win64";
        case RT_ABI_AAPCS:  return "aapcs";
        default: return "unknown";
    }
}

// ===============================================
// 文件头操作
// ===============================================

RTFileHeader* rt_create_header(RTArchitecture arch, RTOperatingSystem os, RTABI abi) {
    RTFileHeader* header = calloc(1, sizeof(RTFileHeader));
    if (!header) return NULL;
    
    // 设置魔数和版本
    memcpy(header->magic, RT_MAGIC, 4);
    header->version_major = RT_VERSION_MAJOR;
    header->version_minor = RT_VERSION_MINOR;
    header->version_patch = RT_VERSION_PATCH;
    
    // 设置架构信息
    header->architecture = arch;
    header->os = os;
    header->abi = abi;
    
    // 设置头部大小
    header->header_size = sizeof(RTFileHeader);
    
    // 设置时间戳
    header->timestamp = (uint32_t)time(NULL);
    
    // 设置默认标志
    header->flags = RT_FLAG_EXECUTABLE;
    
    return header;
}

bool rt_validate_header(const RTFileHeader* header) {
    if (!header) return false;
    
    // 检查魔数
    if (memcmp(header->magic, RT_MAGIC, 4) != 0) {
        return false;
    }
    
    // 检查版本
    if (header->version_major > RT_VERSION_MAJOR) {
        return false; // 不支持更高的主版本
    }
    
    // 检查头部大小
    if (header->header_size < sizeof(RTFileHeader)) {
        return false;
    }
    
    // 检查架构
    if (header->architecture == RT_ARCH_UNKNOWN) {
        return false;
    }
    
    return true;
}

bool rt_check_compatibility(const RTFileHeader* header, 
                           RTArchitecture current_arch, 
                           RTOperatingSystem current_os) {
    if (!rt_validate_header(header)) {
        return false;
    }
    
    // 检查架构兼容性
    if (header->architecture != current_arch) {
        // 某些架构可能兼容
        if (current_arch == RT_ARCH_X86_64 && header->architecture == RT_ARCH_X86_32) {
            // x64可以运行x86程序
        } else {
            return false;
        }
    }
    
    // 检查操作系统兼容性
    if (header->os != current_os && header->os != RT_OS_UNKNOWN) {
        return false;
    }
    
    return true;
}

// ===============================================
// 校验和计算
// ===============================================

uint32_t rt_calculate_checksum(const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t checksum = 0;
    
    for (size_t i = 0; i < size; i++) {
        checksum = (checksum << 1) ^ bytes[i];
        if (checksum & 0x80000000) {
            checksum ^= 0x04C11DB7; // CRC-32 polynomial
        }
    }
    
    return checksum;
}

// ===============================================
// 文件I/O操作
// ===============================================

int rt_write_file(const char* filename, 
                  const RTFileHeader* header,
                  const void* code, size_t code_size,
                  const void* data, size_t data_size,
                  const RTMetadata* metadata) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        return -1;
    }
    
    // 创建可修改的头部副本
    RTFileHeader header_copy = *header;
    
    // 设置偏移和大小
    header_copy.code_size = (uint32_t)code_size;
    header_copy.data_size = (uint32_t)data_size;
    header_copy.code_offset = sizeof(RTFileHeader);
    header_copy.data_offset = header_copy.code_offset + code_size;
    
    if (metadata) {
        header_copy.metadata_size = sizeof(RTMetadata);
        header_copy.metadata_offset = header_copy.data_offset + data_size;
    }
    
    // 计算校验和（排除校验和字段本身）
    header_copy.checksum = 0;
    header_copy.checksum = rt_calculate_checksum(&header_copy, sizeof(RTFileHeader));
    if (code && code_size > 0) {
        header_copy.checksum ^= rt_calculate_checksum(code, code_size);
    }
    if (data && data_size > 0) {
        header_copy.checksum ^= rt_calculate_checksum(data, data_size);
    }
    
    // 写入文件头
    if (fwrite(&header_copy, sizeof(RTFileHeader), 1, fp) != 1) {
        fclose(fp);
        return -1;
    }
    
    // 写入代码段
    if (code && code_size > 0) {
        if (fwrite(code, 1, code_size, fp) != code_size) {
            fclose(fp);
            return -1;
        }
    }
    
    // 写入数据段
    if (data && data_size > 0) {
        if (fwrite(data, 1, data_size, fp) != data_size) {
            fclose(fp);
            return -1;
        }
    }
    
    // 写入元数据
    if (metadata) {
        if (fwrite(metadata, sizeof(RTMetadata), 1, fp) != 1) {
            fclose(fp);
            return -1;
        }
    }
    
    fclose(fp);
    return 0;
}

int rt_read_file(const char* filename,
                 RTFileHeader** header,
                 void** code, size_t* code_size,
                 void** data, size_t* data_size,
                 RTMetadata** metadata) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        return -1;
    }
    
    // 读取文件头
    *header = malloc(sizeof(RTFileHeader));
    if (!*header) {
        fclose(fp);
        return -1;
    }
    
    if (fread(*header, sizeof(RTFileHeader), 1, fp) != 1) {
        free(*header);
        *header = NULL;
        fclose(fp);
        return -1;
    }
    
    // 验证文件头
    if (!rt_validate_header(*header)) {
        free(*header);
        *header = NULL;
        fclose(fp);
        return -1;
    }
    
    // 读取代码段
    *code_size = (*header)->code_size;
    if (*code_size > 0) {
        *code = malloc(*code_size);
        if (!*code) {
            free(*header);
            *header = NULL;
            fclose(fp);
            return -1;
        }
        
        fseek(fp, (*header)->code_offset, SEEK_SET);
        if (fread(*code, 1, *code_size, fp) != *code_size) {
            free(*code);
            free(*header);
            *code = NULL;
            *header = NULL;
            fclose(fp);
            return -1;
        }
    } else {
        *code = NULL;
    }
    
    // 读取数据段
    *data_size = (*header)->data_size;
    if (*data_size > 0) {
        *data = malloc(*data_size);
        if (!*data) {
            free(*code);
            free(*header);
            *code = NULL;
            *header = NULL;
            fclose(fp);
            return -1;
        }
        
        fseek(fp, (*header)->data_offset, SEEK_SET);
        if (fread(*data, 1, *data_size, fp) != *data_size) {
            free(*data);
            free(*code);
            free(*header);
            *data = NULL;
            *code = NULL;
            *header = NULL;
            fclose(fp);
            return -1;
        }
    } else {
        *data = NULL;
    }
    
    // 读取元数据
    if ((*header)->metadata_size > 0) {
        *metadata = malloc(sizeof(RTMetadata));
        if (!*metadata) {
            free(*data);
            free(*code);
            free(*header);
            *data = NULL;
            *code = NULL;
            *header = NULL;
            fclose(fp);
            return -1;
        }
        
        fseek(fp, (*header)->metadata_offset, SEEK_SET);
        if (fread(*metadata, sizeof(RTMetadata), 1, fp) != 1) {
            free(*metadata);
            free(*data);
            free(*code);
            free(*header);
            *metadata = NULL;
            *data = NULL;
            *code = NULL;
            *header = NULL;
            fclose(fp);
            return -1;
        }
    } else {
        *metadata = NULL;
    }
    
    fclose(fp);
    return 0;
}

bool rt_verify_integrity(const char* filename) {
    RTFileHeader* header;
    void* code;
    void* data;
    RTMetadata* metadata;
    size_t code_size, data_size;
    
    if (rt_read_file(filename, &header, &code, &code_size, &data, &data_size, &metadata) != 0) {
        return false;
    }
    
    // 重新计算校验和
    uint32_t saved_checksum = header->checksum;
    header->checksum = 0;
    
    uint32_t calculated_checksum = rt_calculate_checksum(header, sizeof(RTFileHeader));
    if (code && code_size > 0) {
        calculated_checksum ^= rt_calculate_checksum(code, code_size);
    }
    if (data && data_size > 0) {
        calculated_checksum ^= rt_calculate_checksum(data, data_size);
    }
    
    // 清理内存
    free(header);
    free(code);
    free(data);
    free(metadata);
    
    return saved_checksum == calculated_checksum;
}
