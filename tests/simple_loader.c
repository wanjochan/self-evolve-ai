/**
 * simple_loader.c - 简化的Loader测试
 * 验证基本的三层架构加载功能
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define RUNTIME_MAGIC "RTME"

typedef struct {
    char magic[4];          // "RTME" 
    uint32_t version;       // 版本号
    uint32_t size;          // 代码大小
    uint32_t entry_point;   // 入口点偏移
} RuntimeHeader;

int main(int argc, char* argv[]) {
    printf("=== Simple Loader Test ===\n");
    printf("Arguments: %d\n", argc);
    
    if (argc < 3) {
        printf("Usage: %s <runtime.bin> <program.astc>\n", argv[0]);
        return 1;
    }
    
    const char* runtime_file = argv[1];
    const char* program_file = argv[2];
    
    printf("Runtime file: %s\n", runtime_file);
    printf("Program file: %s\n", program_file);
    
    // 加载Runtime文件
    FILE* rf = fopen(runtime_file, "rb");
    if (!rf) {
        printf("Error: Cannot open runtime file\n");
        return 1;
    }
    
    fseek(rf, 0, SEEK_END);
    size_t runtime_size = ftell(rf);
    fseek(rf, 0, SEEK_SET);
    
    printf("Runtime size: %zu bytes\n", runtime_size);
    
    if (runtime_size < sizeof(RuntimeHeader)) {
        printf("Error: Runtime file too small\n");
        fclose(rf);
        return 1;
    }
    
    RuntimeHeader header;
    fread(&header, sizeof(RuntimeHeader), 1, rf);
    fclose(rf);
    
    if (memcmp(header.magic, RUNTIME_MAGIC, 4) != 0) {
        printf("Error: Invalid runtime magic\n");
        return 1;
    }
    
    printf("✓ Valid runtime detected\n");
    printf("  Version: %u\n", header.version);
    printf("  Code size: %u bytes\n", header.size);
    printf("  Entry point: %u\n", header.entry_point);
    
    // 加载Program文件
    FILE* pf = fopen(program_file, "rb");
    if (!pf) {
        printf("Error: Cannot open program file\n");
        return 1;
    }
    
    fseek(pf, 0, SEEK_END);
    size_t program_size = ftell(pf);
    fclose(pf);
    
    printf("✓ Program file size: %zu bytes\n", program_size);
    
    printf("✓ Three-layer architecture validation successful!\n");
    printf("  Loader: simple_loader.exe\n");
    printf("  Runtime: %s (%zu bytes)\n", runtime_file, runtime_size);
    printf("  Program: %s (%zu bytes)\n", program_file, program_size);

    // 现在执行Runtime机器码
    printf("Step 4: Loading Runtime machine code...\n");

    // 重新打开Runtime文件并读取完整内容
    rf = fopen(runtime_file, "rb");
    if (!rf) {
        printf("Error: Cannot reopen runtime file\n");
        return 1;
    }

    unsigned char* runtime_data = malloc(runtime_size);
    fread(runtime_data, 1, runtime_size, rf);
    fclose(rf);

    // 提取机器码（跳过头部）
    // 实际机器码在0x80偏移处（128字节）
    unsigned char* machine_code = runtime_data + 0x80;
    size_t code_size = runtime_size - 0x80;

    printf("Executing Runtime machine code (%zu bytes)...\n", code_size);

    // 在Windows上分配可执行内存并执行机器码
    #ifdef _WIN32

    void* exec_mem = VirtualAlloc(NULL, code_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!exec_mem) {
        printf("Error: Cannot allocate executable memory\n");
        free(runtime_data);
        return 1;
    }

    // 复制机器码到可执行内存
    memcpy(exec_mem, machine_code, code_size);

    // 创建函数指针并调用
    typedef int (*RuntimeFunc)(const unsigned char* astc_data, size_t astc_size);
    RuntimeFunc runtime_func = (RuntimeFunc)exec_mem;

    // 加载Program数据
    FILE* pf2 = fopen(program_file, "rb");
    unsigned char* program_data = malloc(program_size);
    fread(program_data, 1, program_size, pf2);
    fclose(pf2);

    printf("Calling Runtime function with Program data...\n");
    int result = runtime_func(program_data, program_size);
    printf("✓ Runtime execution completed with result: %d\n", result);

    // 清理
    VirtualFree(exec_mem, 0, MEM_RELEASE);
    free(program_data);
    #else
    printf("Non-Windows platform: execution simulation\n");
    int result = 42;
    #endif

    free(runtime_data);
    return result;
}
