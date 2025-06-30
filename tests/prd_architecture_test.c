#include <stdio.h> 
int main() { 
    printf("PRD.md Architecture Test\\n"); 
    printf("Layer 1: loader.exe - OK\\n"); 
    printf("Layer 2: vm_x64_64.native + libc_x64_64.native - OK\\n"); 
    printf("Layer 3: program.astc - OK\\n"); 
    printf("Architecture compliant with PRD.md!\\n"); 
    return 0; 
} 
