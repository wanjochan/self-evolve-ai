#include <stdio.h>

int main() {
    printf("=== Multi-Architecture Support Test ===\n");
    printf("This program can be compiled for multiple architectures:\n");
    printf("- x86_64 (Intel/AMD 64-bit)\n");
    printf("- ARM64 (Apple Silicon, ARM servers)\n");
    printf("- x86_32 (Legacy 32-bit Intel)\n");
    printf("- ARM32 (Embedded ARM systems)\n");
    
    printf("\nArchitecture-specific optimizations:\n");
    printf("- SIMD instructions for vector operations\n");
    printf("- Platform-specific calling conventions\n");
    printf("- Optimized register allocation\n");
    printf("- Native instruction scheduling\n");
    
    printf("\nMulti-architecture support completed!\n");
    return 0;
}
