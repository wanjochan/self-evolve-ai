/**
 * test_platform_detection.c - 测试平台检测功能
 * 
 * 验证改进后的loader能否正确检测平台架构
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 从loader中复制的检测函数
const char* detect_operating_system(void) {
    FILE* f;
    
    f = fopen("C:\\Windows\\System32\\kernel32.dll", "rb");
    if (f) {
        fclose(f);
        return "windows";
    }
    
    f = fopen("/proc/version", "r");
    if (f) {
        fclose(f);
        return "linux";
    }
    
    f = fopen("/System/Library/CoreServices/SystemVersion.plist", "r");
    if (f) {
        fclose(f);
        return "macos";
    }
    
    return "unknown";
}

const char* detect_cpu_architecture(void) {
    const char* os = detect_operating_system();
    
    if (strcmp(os, "linux") == 0) {
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
    }
    
    return sizeof(void*) == 8 ? "x64" : "x86";
}

int main(void) {
    printf("=== Platform Detection Test ===\n");
    
    const char* os = detect_operating_system();
    const char* arch = detect_cpu_architecture();
    int bits = sizeof(void*) * 8;
    
    printf("Detected OS: %s\n", os);
    printf("Detected Architecture: %s\n", arch);
    printf("Pointer size: %d bits\n", bits);
    
    // 构建runtime文件名
    char runtime_filename[256];
    snprintf(runtime_filename, sizeof(runtime_filename), "runtime%s_%d.rt", arch, bits);
    
    printf("Expected runtime file: %s\n", runtime_filename);
    
    // 检查runtime文件是否存在
    FILE* f = fopen(runtime_filename, "rb");
    if (f) {
        fclose(f);
        printf("✅ Runtime file exists\n");
    } else {
        printf("❌ Runtime file not found\n");
    }
    
    return 0;
}
