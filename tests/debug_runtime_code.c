/**
 * debug_runtime_code.c - 调试Runtime机器码
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int main() {
    printf("Debugging Runtime machine code...\n");
    
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
    
    printf("Runtime binary size: %zu bytes\n", file_size);
    
    // 检查头部
    if (memcmp(data, "RTME", 4) == 0) {
        uint32_t version = *((uint32_t*)(data + 4));
        uint32_t code_size = *((uint32_t*)(data + 8));
        uint32_t entry_offset = *((uint32_t*)(data + 12));

        printf("Raw header bytes:\n");
        for (int i = 0; i < 32; i++) {
            printf("%02x ", data[i]);
            if ((i + 1) % 16 == 0) printf("\n");
        }
        
        printf("Header:\n");
        printf("  Magic: RTME\n");
        printf("  Version: %u\n", version);
        printf("  Code size: %u bytes\n", code_size);
        printf("  Entry offset: %u\n", entry_offset);
        
        // 显示机器码
        uint8_t* machine_code = data + entry_offset;
        printf("\nMachine code (%u bytes):\n", code_size);
        for (uint32_t i = 0; i < code_size && i < 32; i++) {
            printf("%02x ", machine_code[i]);
            if ((i + 1) % 16 == 0) printf("\n");
        }
        if (code_size > 32) printf("...\n");
        else printf("\n");
        
    } else {
        printf("Invalid runtime format\n");
    }
    
    free(data);
    return 0;
}
