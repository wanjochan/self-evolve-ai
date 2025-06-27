/**
 * test_simple_runtime.c - 测试简单的Runtime机器码
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#endif

int main() {
    printf("Creating and testing simple runtime...\n");
    
    // 手动创建正确的Runtime.bin格式
    uint8_t header[64];
    memset(header, 0, 64);
    
    // 设置头部
    memcpy(header, "RTME", 4);                    // Magic
    *((uint32_t*)(header + 4)) = 1;              // Version
    *((uint32_t*)(header + 8)) = 11;             // Code size
    *((uint32_t*)(header + 12)) = 64;            // Entry offset
    memcpy(header + 16, "EVOLVER0_RUNTIME", 16); // Runtime ID
    
    // 简单的机器码：返回42
    uint8_t machine_code[] = {
        0x55,                           // push rbp
        0x48, 0x89, 0xe5,               // mov rbp, rsp
        0xb8, 0x2a, 0x00, 0x00, 0x00,   // mov eax, 42
        0x5d,                           // pop rbp
        0xc3                            // ret
    };
    
    // 写入测试文件
    FILE* fp = fopen("tests/simple_runtime.bin", "wb");
    if (!fp) {
        printf("Cannot create test file\n");
        return 1;
    }
    
    fwrite(header, 1, 64, fp);
    fwrite(machine_code, 1, sizeof(machine_code), fp);
    fclose(fp);
    
    printf("Created simple_runtime.bin (%zu bytes)\n", 64 + sizeof(machine_code));
    
    // 测试执行
    #ifdef _WIN32
    void* exec_mem = VirtualAlloc(NULL, sizeof(machine_code), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!exec_mem) {
        printf("Failed to allocate executable memory\n");
        return 1;
    }
    
    memcpy(exec_mem, machine_code, sizeof(machine_code));
    
    typedef int (*TestFunc)(void);
    TestFunc test_func = (TestFunc)exec_mem;
    
    printf("Calling simple machine code...\n");
    int result = test_func();
    
    printf("Simple machine code returned: %d\n", result);
    
    VirtualFree(exec_mem, 0, MEM_RELEASE);
    
    if (result == 42) {
        printf("✅ Simple runtime test successful!\n");
        return 0;
    } else {
        printf("❌ Simple runtime test failed!\n");
        return 1;
    }
    #else
    printf("Non-Windows platform\n");
    return 0;
    #endif
}
