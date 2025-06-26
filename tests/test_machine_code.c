/**
 * test_machine_code.c - 测试机器码执行
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#endif

int main() {
    printf("Testing machine code execution...\n");
    
    // 简单的x64机器码：返回123
    uint8_t machine_code[] = {
        0x55,                           // push rbp
        0x48, 0x89, 0xe5,               // mov rbp, rsp
        0xb8, 0x7b, 0x00, 0x00, 0x00,   // mov eax, 123
        0x5d,                           // pop rbp
        0xc3                            // ret
    };
    
    size_t code_size = sizeof(machine_code);
    
    #ifdef _WIN32
    // 分配可执行内存
    void* exec_mem = VirtualAlloc(NULL, code_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!exec_mem) {
        printf("Failed to allocate executable memory\n");
        return 1;
    }
    
    // 复制机器码
    memcpy(exec_mem, machine_code, code_size);
    
    // 创建函数指针
    typedef int (*TestFunc)(void);
    TestFunc test_func = (TestFunc)exec_mem;
    
    printf("Calling machine code function...\n");
    
    // 调用机器码
    int result = test_func();
    
    printf("Machine code returned: %d\n", result);
    
    // 清理
    VirtualFree(exec_mem, 0, MEM_RELEASE);
    
    if (result == 123) {
        printf("✅ Machine code execution successful!\n");
        return 0;
    } else {
        printf("❌ Machine code execution failed!\n");
        return 1;
    }
    #else
    printf("Non-Windows platform\n");
    return 0;
    #endif
}
