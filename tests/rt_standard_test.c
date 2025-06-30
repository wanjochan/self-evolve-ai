#include <stdio.h>

// 标准化.rt文件格式测试程序
int main() {
    printf("Standardized RT Format Test\n");
    printf("Version: 1.0.0\n");
    printf("Architecture: x86_64\n");
    printf("OS: Windows\n");
    printf("ABI: Win64\n");
    
    int test_value = 123;
    printf("Test value: %d\n", test_value);
    printf("RT format standardization test completed!\n");
    
    return test_value;
}
