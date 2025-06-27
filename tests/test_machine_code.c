#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

int main() {
    // 测试我们生成的机器码是否正确
    // 这是tool_astc2bin生成的机器码
    unsigned char machine_code[] = {
        0x55,                    // push ebp
        0x89, 0xe5,             // mov ebp, esp
        0x8b, 0x45, 0x08,       // mov eax, [ebp+8]  ; 获取data参数
        0xb8, 0x05, 0x00, 0x00, 0x00,  // mov eax, 5
        0x5d,                    // pop ebp
        0xc3                     // ret
    };
    
    size_t code_size = sizeof(machine_code);
    
    printf("Testing machine code (%zu bytes):\n", code_size);
    for (size_t i = 0; i < code_size; i++) {
        printf("%02x ", machine_code[i]);
    }
    printf("\n");
    
    // 分配可执行内存
    void* exec_mem = VirtualAlloc(NULL, code_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!exec_mem) {
        printf("Failed to allocate executable memory\n");
        return 1;
    }
    
    // 复制机器码
    memcpy(exec_mem, machine_code, code_size);
    
    // 创建函数指针
    typedef int (*TestFunc)(void* data, size_t size);
    TestFunc test_func = (TestFunc)exec_mem;
    
    // 调用函数
    printf("Calling machine code function...\n");
    char dummy_data[] = "test";
    int result = test_func(dummy_data, 4);
    printf("Function returned: %d\n", result);
    
    // 清理
    VirtualFree(exec_mem, 0, MEM_RELEASE);
    
    return 0;
}
