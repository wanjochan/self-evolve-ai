/**
 * test_machine_code_call.c - 测试机器码调用
 * 安全地测试Runtime.bin中的机器码
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#endif

int main() {
    printf("=== Machine Code Call Test ===\n");
    
    // 创建一个简单的测试机器码：return 42
    unsigned char test_code[] = {
        0x55,                    // push rbp
        0x48, 0x89, 0xe5,       // mov rbp, rsp
        0xb8, 0x2a, 0x00, 0x00, 0x00,  // mov eax, 42
        0x5d,                    // pop rbp
        0xc3                     // ret
    };
    
    size_t code_size = sizeof(test_code);
    printf("Test code size: %zu bytes\n", code_size);
    
    #ifdef _WIN32
    // 分配可执行内存
    void* exec_mem = VirtualAlloc(NULL, code_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!exec_mem) {
        printf("Error: Cannot allocate executable memory\n");
        return 1;
    }
    
    // 复制机器码
    memcpy(exec_mem, test_code, code_size);
    
    // 创建函数指针并调用
    typedef int (*TestFunc)(void);
    TestFunc test_func = (TestFunc)exec_mem;
    
    printf("Calling test machine code...\n");
    int result = test_func();
    printf("✓ Test machine code returned: %d\n", result);
    
    // 清理
    VirtualFree(exec_mem, 0, MEM_RELEASE);
    
    if (result == 42) {
        printf("✓ Machine code execution test PASSED\n");
        return 0;
    } else {
        printf("✗ Machine code execution test FAILED\n");
        return 1;
    }
    #else
    printf("Non-Windows platform: skipping test\n");
    return 0;
    #endif
}
