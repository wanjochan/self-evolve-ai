/**
 * hex_dump.c - 十六进制转储工具
 * 查看Runtime.bin的完整内容
 */

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <file>\n", argv[0]);
        return 1;
    }
    
    FILE* f = fopen(argv[1], "rb");
    if (!f) {
        printf("Cannot open file\n");
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    printf("=== Hex Dump of %s (%zu bytes) ===\n", argv[1], size);
    
    unsigned char buffer[16];
    size_t offset = 0;
    
    while (offset < size) {
        size_t read_size = fread(buffer, 1, 16, f);
        
        printf("%08zX: ", offset);
        
        // 十六进制
        for (size_t i = 0; i < 16; i++) {
            if (i < read_size) {
                printf("%02X ", buffer[i]);
            } else {
                printf("   ");
            }
        }
        
        printf(" ");
        
        // ASCII
        for (size_t i = 0; i < read_size; i++) {
            if (buffer[i] >= 32 && buffer[i] <= 126) {
                printf("%c", buffer[i]);
            } else {
                printf(".");
            }
        }
        
        printf("\n");
        offset += read_size;
        
        if (read_size < 16) break;
    }
    
    fclose(f);
    return 0;
}
