#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

int main() {
    printf("========================================\n");
    printf("C99 STANDARD LIBRARY COMPLETENESS TEST\n");
    printf("========================================\n");
    
    // 测试字符串函数
    printf("Testing string functions...\n");
    
    char str1[100] = "Hello";
    char str2[100] = " World";
    char str3[100];
    
    // strcat测试
    strcpy(str3, str1);
    strcat(str3, str2);
    printf("strcat: '%s' + '%s' = '%s'\n", str1, str2, str3);
    
    // strchr测试
    char* pos = strchr(str3, 'W');
    if (pos) {
        printf("strchr: Found 'W' at position %ld\n", pos - str3);
    }
    
    // strstr测试
    char* substr = strstr(str3, "World");
    if (substr) {
        printf("strstr: Found 'World' at position %ld\n", substr - str3);
    }
    
    // 测试字符类型函数
    printf("\nTesting character type functions...\n");
    char test_char = 'A';
    printf("Character '%c':\n", test_char);
    printf("  isalpha: %d\n", isalpha(test_char));
    printf("  isdigit: %d\n", isdigit(test_char));
    printf("  isalnum: %d\n", isalnum(test_char));
    printf("  isupper: %d\n", isupper(test_char));
    printf("  islower: %d\n", islower(test_char));
    printf("  tolower: %c\n", tolower(test_char));
    
    // 测试数学函数
    printf("\nTesting math functions...\n");
    double x = 4.0;
    printf("sqrt(%.1f) = %.2f\n", x, sqrt(x));
    printf("sin(%.1f) = %.2f\n", x, sin(x));
    printf("cos(%.1f) = %.2f\n", x, cos(x));
    printf("log(%.1f) = %.2f\n", x, log(x));
    printf("exp(1.0) = %.2f\n", exp(1.0));
    printf("pow(2.0, 3.0) = %.2f\n", pow(2.0, 3.0));
    printf("floor(3.7) = %.2f\n", floor(3.7));
    printf("ceil(3.2) = %.2f\n", ceil(3.2));
    printf("fabs(-5.5) = %.2f\n", fabs(-5.5));
    
    // 测试转换函数
    printf("\nTesting conversion functions...\n");
    char num_str[] = "12345";
    printf("atoi('%s') = %d\n", num_str, atoi(num_str));
    printf("atol('%s') = %ld\n", num_str, atol(num_str));
    printf("atof('123.45') = %.2f\n", atof("123.45"));
    
    // 测试内存函数
    printf("\nTesting memory functions...\n");
    char buffer1[20] = "Test Buffer";
    char buffer2[20];
    
    memcpy(buffer2, buffer1, strlen(buffer1) + 1);
    printf("memcpy: '%s' copied to '%s'\n", buffer1, buffer2);
    
    memset(buffer2, 'X', 5);
    buffer2[5] = '\0';
    printf("memset: First 5 chars set to 'X': '%s'\n", buffer2);
    
    // 测试时间函数
    printf("\nTesting time functions...\n");
    time_t current_time = time(NULL);
    printf("Current time: %ld\n", (long)current_time);
    
    clock_t start_clock = clock();
    printf("Clock ticks: %ld\n", (long)start_clock);
    
    // 测试随机数
    printf("\nTesting random functions...\n");
    srand(12345);
    printf("Random numbers: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", rand() % 100);
    }
    printf("\n");
    
    // 测试文件操作（基础）
    printf("\nTesting file operations...\n");
    FILE* test_file = fopen("test_output.txt", "w");
    if (test_file) {
        fprintf(test_file, "Test file content\n");
        fputs("Another line\n", test_file);
        fclose(test_file);
        printf("File operations: SUCCESS\n");
    } else {
        printf("File operations: FAILED\n");
    }
    
    printf("\n========================================\n");
    printf("C99 STANDARD LIBRARY TEST COMPLETE\n");
    printf("========================================\n");
    printf("All major function categories tested!\n");
    printf("- String functions: strcat, strchr, strstr\n");
    printf("- Character types: isalpha, isdigit, isalnum, etc.\n");
    printf("- Math functions: sqrt, sin, cos, log, exp, pow, etc.\n");
    printf("- Conversion: atoi, atol, atof\n");
    printf("- Memory: memcpy, memset\n");
    printf("- Time: time, clock\n");
    printf("- Random: rand, srand\n");
    printf("- File I/O: fopen, fprintf, fputs, fclose\n");
    printf("========================================\n");
    
    return 0;
}
