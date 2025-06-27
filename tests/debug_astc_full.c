#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <file.astc>\n", argv[0]);
        return 1;
    }
    
    FILE* file = fopen(argv[1], "rb");
    if (!file) {
        printf("Cannot open file\n");
        return 1;
    }
    
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    unsigned char* data = malloc(size);
    fread(data, 1, size, file);
    fclose(file);
    
    printf("ASTC file size: %zu bytes\n", size);
    
    printf("\n32-bit words interpretation (all):\n");
    for (size_t i = 0; i + 4 <= size; i += 4) {
        uint32_t word = *(uint32_t*)(data + i);
        printf("Offset %04zx: %08x (%d)\n", i, word, (int32_t)word);
        
        // 查找可能的返回值42
        if (word == 42) {
            printf("  *** Found 42 at offset %04zx ***\n", i);
        }
    }
    
    free(data);
    return 0;
}
