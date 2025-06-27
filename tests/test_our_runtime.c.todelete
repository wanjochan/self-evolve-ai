/**
 * test_our_runtime.c - 测试我们生成的Runtime机器码
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#endif

int main() {
    printf("Testing our generated Runtime machine code...\n");
    
    // 读取evolver0_runtime.bin
    FILE* fp = fopen("evolver0_runtime.bin", "rb");
    if (!fp) {
        printf("Cannot open evolver0_runtime.bin\n");
        return 1;
    }
    
    fseek(fp, 0, SEEK_END);
    size_t file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    uint8_t* data = malloc(file_size);
    fread(data, 1, file_size, fp);
    fclose(fp);
    
    // 解析头部
    uint32_t version = *((uint32_t*)(data + 4));
    uint32_t code_size = *((uint32_t*)(data + 8));
    uint32_t entry_offset = *((uint32_t*)(data + 12));
    
    printf("Runtime info:\n");
    printf("  File size: %zu bytes\n", file_size);
    printf("  Code size: %u bytes\n", code_size);
    printf("  Entry offset: %u\n", entry_offset);
    
    // 提取机器码
    uint8_t* machine_code = data + entry_offset;
    
    printf("Machine code bytes:\n");
    for (uint32_t i = 0; i < code_size; i++) {
        printf("%02x ", machine_code[i]);
    }
    printf("\n");
    
    #ifdef _WIN32
    // 测试执行机器码
    void* exec_mem = VirtualAlloc(NULL, code_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!exec_mem) {
        printf("Failed to allocate executable memory\n");
        free(data);
        return 1;
    }
    
    memcpy(exec_mem, machine_code, code_size);
    
    // 创建函数指针（无参数版本）
    typedef int (*TestFunc)(void);
    TestFunc test_func = (TestFunc)exec_mem;
    
    printf("Calling machine code...\n");
    
    int result = test_func();
    
    printf("Machine code returned: %d\n", result);
    
    VirtualFree(exec_mem, 0, MEM_RELEASE);
    #endif
    
    free(data);
    return 0;
}
