#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE* file = fopen("tests/debug_astc_runtime.c.astc", "rb");
    if (!file) {
        printf("Cannot open ASTC file\n");
        return 1;
    }
    
    printf("ASTC File Hexdump:\n");
    
    unsigned char buffer[16];
    size_t bytes_read;
    size_t offset = 0;
    
    while ((bytes_read = fread(buffer, 1, 16, file)) > 0) {
        printf("%08zx: ", offset);
        
        // Print hex bytes
        for (size_t i = 0; i < 16; i++) {
            if (i < bytes_read) {
                printf("%02x ", buffer[i]);
            } else {
                printf("   ");
            }
        }
        
        printf(" ");
        
        // Print ASCII
        for (size_t i = 0; i < bytes_read; i++) {
            if (buffer[i] >= 32 && buffer[i] <= 126) {
                printf("%c", buffer[i]);
            } else {
                printf(".");
            }
        }
        
        printf("\n");
        offset += bytes_read;
    }
    
    fclose(file);
    return 0;
}
