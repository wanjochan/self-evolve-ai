#include <stdio.h>

int main() {
    printf("Hello from TCC cross-compiler!\n");
    printf("Architecture: %s\n", 
#ifdef __x86_64__
        "x86_64"
#elif defined(__i386__)
        "i386"
#elif defined(__aarch64__)
        "aarch64"
#elif defined(__arm__)
        "arm"
#else
        "unknown"
#endif
    );
    printf("Compiler: TCC\n");
    return 0;
}
