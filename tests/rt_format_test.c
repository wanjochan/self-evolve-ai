/**
 * rt_format_test.c - 测试标准化.rt文件格式
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/runtime/rt_format_standard.h"

int main() {
    printf("=== RT Format Standardization Test ===\n");
    
    // 测试架构检测
    RTArchitecture arch = rt_detect_architecture();
    RTOperatingSystem os = rt_detect_os();
    RTABI abi = rt_detect_abi();
    
    printf("Detected environment:\n");
    printf("  Architecture: %s\n", rt_get_architecture_name(arch));
    printf("  OS: %s\n", rt_get_os_name(os));
    printf("  ABI: %s\n", rt_get_abi_name(abi));
    
    // 创建测试文件头
    RTFileHeader* header = rt_create_header(arch, os, abi);
    if (!header) {
        printf("❌ Failed to create RT header\n");
        return 1;
    }
    
    printf("✅ RT header created successfully\n");
    printf("  Magic: %.4s\n", header->magic);
    printf("  Version: %d.%d.%d\n", header->version_major, header->version_minor, header->version_patch);
    printf("  Timestamp: %u\n", header->timestamp);
    
    // 验证文件头
    if (rt_validate_header(header)) {
        printf("✅ RT header validation passed\n");
    } else {
        printf("❌ RT header validation failed\n");
        free(header);
        return 1;
    }
    
    // 测试兼容性检查
    if (rt_check_compatibility(header, arch, os)) {
        printf("✅ RT compatibility check passed\n");
    } else {
        printf("❌ RT compatibility check failed\n");
        free(header);
        return 1;
    }
    
    // 创建测试代码和数据
    const char* test_code = "Hello, RT Format!";
    const char* test_data = "Test data section";
    
    RTMetadata metadata = {0};
    metadata.libc_version = 1;
    metadata.min_stack_size = 8192;
    metadata.min_heap_size = 4096;
    metadata.optimization_level = 2;
    strcpy(metadata.compiler_name, "self-evolve-ai");
    strcpy(metadata.compiler_version, "1.0.0");
    strcpy(metadata.build_date, "2025-06-29");
    
    // 写入测试文件
    const char* test_filename = "tests/test_runtime.rt";
    if (rt_write_file(test_filename, header, 
                      test_code, strlen(test_code),
                      test_data, strlen(test_data),
                      &metadata) == 0) {
        printf("✅ RT file written successfully: %s\n", test_filename);
    } else {
        printf("❌ Failed to write RT file\n");
        free(header);
        return 1;
    }
    
    // 验证文件完整性
    if (rt_verify_integrity(test_filename)) {
        printf("✅ RT file integrity verification passed\n");
    } else {
        printf("❌ RT file integrity verification failed\n");
        free(header);
        return 1;
    }
    
    // 读取并验证文件
    RTFileHeader* read_header;
    void* read_code;
    void* read_data;
    RTMetadata* read_metadata;
    size_t read_code_size, read_data_size;
    
    if (rt_read_file(test_filename, &read_header, 
                     &read_code, &read_code_size,
                     &read_data, &read_data_size,
                     &read_metadata) == 0) {
        printf("✅ RT file read successfully\n");
        printf("  Code size: %zu bytes\n", read_code_size);
        printf("  Data size: %zu bytes\n", read_data_size);
        printf("  Code content: %.*s\n", (int)read_code_size, (char*)read_code);
        printf("  Data content: %.*s\n", (int)read_data_size, (char*)read_data);
        
        if (read_metadata) {
            printf("  Metadata:\n");
            printf("    Compiler: %s %s\n", read_metadata->compiler_name, read_metadata->compiler_version);
            printf("    Build date: %s\n", read_metadata->build_date);
            printf("    Min stack: %u bytes\n", read_metadata->min_stack_size);
            printf("    Min heap: %u bytes\n", read_metadata->min_heap_size);
        }
        
        // 清理读取的数据
        free(read_header);
        free(read_code);
        free(read_data);
        free(read_metadata);
    } else {
        printf("❌ Failed to read RT file\n");
        free(header);
        return 1;
    }
    
    free(header);
    
    printf("=== All RT Format Tests Passed! ===\n");
    return 0;
}
