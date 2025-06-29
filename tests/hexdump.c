#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <file>\n", argv[0]);
        return 1;
    }
    
    FILE* file = fopen(argv[1], "rb");
    if (!file) {
        printf("Cannot open file: %s\n", argv[1]);
        return 1;
    }
    
    unsigned char buffer[16];
    size_t bytes_read;
    size_t offset = 0;
    
    while ((bytes_read = fread(buffer, 1, 16, file)) > 0) {
        printf("%08zX: ", offset);
        
        // Print hex
        for (size_t i = 0; i < 16; i++) {
            if (i < bytes_read) {
                printf("%02X ", buffer[i]);
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
