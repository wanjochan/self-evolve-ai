/**
 * malloc_test.c - 测试malloc/free等内存管理函数
 */

#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("=== Memory Management Test ===\n");
    
    // 测试malloc
    printf("Testing malloc(100)...\n");
    char* buffer = malloc(100);
    if (buffer) {
        printf("✅ malloc(100) successful: %p\n", buffer);
        
        // 测试sprintf
        sprintf(buffer, "Hello from malloc'd memory!");
        printf("✅ sprintf successful: %s\n", buffer);
        
        // 测试free
        printf("Testing free()...\n");
        free(buffer);
        printf("✅ free() successful\n");
    } else {
        printf("❌ malloc(100) failed\n");
        return 1;
    }
    
    printf("=== All Memory Tests Passed! ===\n");
    return 0;
}
