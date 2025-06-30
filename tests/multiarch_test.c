#include <stdio.h>

int main() {
    printf("Multi-architecture JIT compiler test!\n");
    printf("Testing x86_64 code generation...\n");
    
    int x = 10;
    int y = 20;
    int result = x + y;
    
    printf("Result: %d + %d = %d\n", x, y, result);
    printf("Multi-arch test completed!\n");
    
    return 0;
}
