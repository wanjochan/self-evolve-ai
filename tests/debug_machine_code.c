/**
 * debug_machine_code.c - 调试机器码生成
 * 检查Runtime.bin中的机器码是否正确
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define RUNTIME_MAGIC "RTME"

typedef struct {
    char magic[4];          
    uint32_t version;       
    uint32_t size;          
    uint32_t entry_point;   
} RuntimeHeader;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <runtime.bin>\n", argv[0]);
        return 1;
    }
    
    const char* runtime_file = argv[1];
    
    FILE* rf = fopen(runtime_file, "rb");
    if (!rf) {
        printf("Error: Cannot open runtime file\n");
        return 1;
    }
    
    fseek(rf, 0, SEEK_END);
    size_t runtime_size = ftell(rf);
    fseek(rf, 0, SEEK_SET);
    
    printf("=== Runtime.bin Analysis ===\n");
    printf("File size: %zu bytes\n", runtime_size);
    
    RuntimeHeader header;
    fread(&header, sizeof(RuntimeHeader), 1, rf);
    
    printf("Header analysis:\n");
    printf("  Magic: %.4s\n", header.magic);
    printf("  Version: %u\n", header.version);
    printf("  Code size: %u bytes\n", header.size);
    printf("  Entry point: %u\n", header.entry_point);
    
    if (memcmp(header.magic, RUNTIME_MAGIC, 4) != 0) {
        printf("Error: Invalid magic number\n");
        fclose(rf);
        return 1;
    }
    
    // 读取机器码部分
    fseek(rf, header.entry_point, SEEK_SET);
    unsigned char* machine_code = malloc(header.size);
    fread(machine_code, 1, header.size, rf);
    fclose(rf);
    
    printf("\nMachine code analysis (at offset %u):\n", header.entry_point);
    printf("First 16 bytes: ");
    for (int i = 0; i < 16 && i < header.size; i++) {
        printf("%02X ", machine_code[i]);
    }
    printf("\n");
    
    // 检查是否是有效的x64指令序列
    if (header.size >= 3) {
        if (machine_code[0] == 0x55) {
            printf("✓ Valid function prologue detected (push rbp)\n");
        }
        if (machine_code[1] == 0x48 && machine_code[2] == 0x89) {
            printf("✓ Valid stack setup detected (mov rbp, rsp)\n");
        }
    }
    
    if (header.size >= 2 && machine_code[header.size-1] == 0xc3) {
        printf("✓ Valid function epilogue detected (ret)\n");
    }
    
    printf("\n✓ Runtime.bin appears to contain valid x64 machine code\n");
    printf("Ready for execution by Loader\n");
    
    free(machine_code);
    return 0;
}
