#include <stdio.h>

int main() {
    printf("Hello from TCC on macOS ARM64!\n");
    
    #ifdef __APPLE__
    printf("Compiled on macOS\n");
    #endif
    
    #ifdef __aarch64__
    printf("Running on ARM64 architecture\n");
    #endif
    
    printf("Compiler: TCC\n");
    
    return 0;
} 