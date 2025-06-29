/**
 * simple_malloc_test.c - 简化的malloc测试
 */

#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Testing malloc...\n");
    
    char* buffer = malloc(100);
    printf("malloc returned: %p\n", buffer);
    
    if (buffer) {
        printf("malloc SUCCESS!\n");
        free(buffer);
        printf("free SUCCESS!\n");
    } else {
        printf("malloc FAILED!\n");
    }
    
    printf("Memory test complete.\n");
    return 0;
}
