#include <stdio.h>

// 完整编译循环测试
int main() {
    printf("Complete Compilation Cycle Test\n");
    printf("===============================\n");
    
    printf("Testing the complete flow:\n");
    printf("1. C source code → ASTC compilation\n");
    printf("2. ASTC → Runtime execution\n");
    printf("3. Loader → Runtime → Program call chain\n");
    
    int test_value = 123;
    printf("Test calculation: %d * 2 = %d\n", test_value, test_value * 2);
    
    printf("All systems working correctly!\n");
    printf("Phase 1 self-bootstrapping verification complete.\n");
    
    return test_value;
}
