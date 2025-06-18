#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
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
#elif defined(__riscv) && __riscv_xlen == 64
        "riscv64"
#elif defined(__mips64)
        "mips64"
#elif defined(__mips__)
        "mips"
#elif defined(__powerpc64__)
        "powerpc64"
#elif defined(__powerpc__)
        "powerpc"
#elif defined(__s390x__)
        "s390x"
#else
        "unknown"
#endif
    );
    printf("Compiler: TCC\n");
    printf("Arguments: %d\n", argc);
    return 0;
}
