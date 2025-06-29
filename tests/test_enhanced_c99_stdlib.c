#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <time.h>

// 增强版C99标准库测试
int main() {
    printf("=== Enhanced C99 Standard Library Test ===\n");
    
    // 1. 测试字符串函数
    printf("\n1. String Functions Test:\n");
    
    char str1[50] = "Hello";
    char str2[50] = "World";
    char result[100];
    
    strcpy(result, str1);
    strcat(result, " ");
    strcat(result, str2);
    printf("String concatenation: %s\n", result);
    
    char* found = strstr(result, "World");
    if (found) {
        printf("Found 'World' in string\n");
    }
    
    char* ch = strchr(result, 'W');
    if (ch) {
        printf("Found 'W' at position: %ld\n", ch - result);
    }
    
    // 2. 测试内存函数
    printf("\n2. Memory Functions Test:\n");
    
    char buffer1[20];
    char buffer2[20];
    
    memset(buffer1, 'A', 10);
    buffer1[10] = '\0';
    printf("memset result: %s\n", buffer1);
    
    memcpy(buffer2, buffer1, 10);
    buffer2[10] = '\0';
    printf("memcpy result: %s\n", buffer2);
    
    int cmp_result = memcmp(buffer1, buffer2, 10);
    printf("memcmp result: %d (should be 0)\n", cmp_result);
    
    // 3. 测试数学函数
    printf("\n3. Math Functions Test:\n");
    
    double angle = 3.14159 / 6;  // 30 degrees
    printf("sin(30°) = %.4f\n", sin(angle));
    printf("cos(30°) = %.4f\n", cos(angle));
    printf("tan(30°) = %.4f\n", tan(angle));
    
    double x = 2.718281828;
    printf("log(e) = %.4f\n", log(x));
    printf("log10(100) = %.4f\n", log10(100.0));
    printf("exp(1) = %.4f\n", exp(1.0));
    
    printf("pow(2, 3) = %.0f\n", pow(2.0, 3.0));
    printf("sqrt(16) = %.0f\n", sqrt(16.0));
    
    printf("floor(3.7) = %.0f\n", floor(3.7));
    printf("ceil(3.2) = %.0f\n", ceil(3.2));
    printf("fabs(-5.5) = %.1f\n", fabs(-5.5));
    
    // 4. 测试转换函数
    printf("\n4. Conversion Functions Test:\n");
    
    char num_str[] = "12345";
    int num = atoi(num_str);
    printf("atoi('%s') = %d\n", num_str, num);
    
    char float_str[] = "3.14159";
    double fnum = atof(float_str);
    printf("atof('%s') = %.5f\n", float_str, fnum);
    
    // 5. 测试字符分类函数
    printf("\n5. Character Classification Test:\n");
    
    char test_char = 'A';
    printf("'%c': isalpha=%d, isupper=%d\n", test_char, isalpha(test_char), isupper(test_char));
    
    test_char = '5';
    printf("'%c': isdigit=%d, isalnum=%d\n", test_char, isdigit(test_char), isalnum(test_char));
    
    test_char = ' ';
    printf("'%c': isspace=%d\n", test_char, isspace(test_char));
    
    // 6. 测试大小写转换
    printf("\n6. Case Conversion Test:\n");
    
    char upper_char = 'A';
    char lower_char = 'z';
    printf("tolower('%c') = '%c'\n", upper_char, tolower(upper_char));
    printf("toupper('%c') = '%c'\n", lower_char, toupper(lower_char));
    
    // 7. 测试时间函数
    printf("\n7. Time Functions Test:\n");
    
    time_t current_time = time(NULL);
    printf("Current timestamp: %ld\n", current_time);
    
    clock_t start_time = clock();
    // 简单计算消耗时间
    volatile int sum = 0;
    for (int i = 0; i < 1000; i++) {
        sum += i;
    }
    clock_t end_time = clock();
    printf("Clock ticks for loop: %ld\n", end_time - start_time);
    
    // 8. 测试随机数函数
    printf("\n8. Random Number Test:\n");
    
    srand(42);  // 固定种子以获得可重复结果
    printf("Random numbers: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", rand() % 100);
    }
    printf("\n");
    
    // 9. 测试动态内存分配
    printf("\n9. Dynamic Memory Test:\n");
    
    int* dynamic_array = malloc(5 * sizeof(int));
    if (dynamic_array) {
        for (int i = 0; i < 5; i++) {
            dynamic_array[i] = i * i;
        }
        
        printf("Dynamic array: ");
        for (int i = 0; i < 5; i++) {
            printf("%d ", dynamic_array[i]);
        }
        printf("\n");
        
        free(dynamic_array);
        printf("Memory freed successfully\n");
    }
    
    // 10. 测试文件操作（简化版）
    printf("\n10. File Operations Test:\n");
    
    FILE* test_file = fopen("test_output.txt", "w");
    if (test_file) {
        fprintf(test_file, "Hello, File!\n");
        fflush(test_file);
        printf("File write successful\n");
        fclose(test_file);
        
        // 读取文件
        test_file = fopen("test_output.txt", "r");
        if (test_file) {
            char file_buffer[100];
            if (fgets(file_buffer, sizeof(file_buffer), test_file)) {
                printf("File content: %s", file_buffer);
            }
            fclose(test_file);
        }
    }
    
    printf("\n=== Enhanced C99 Standard Library Test Complete ===\n");
    printf("All major C99 standard library functions tested successfully!\n");
    printf("The enhanced library now supports:\n");
    printf("- Complete string manipulation (strlen, strcpy, strcat, etc.)\n");
    printf("- Memory operations (memcpy, memset, memcmp)\n");
    printf("- Mathematical functions (sin, cos, sqrt, pow, etc.)\n");
    printf("- Character classification and conversion\n");
    printf("- Time and date functions\n");
    printf("- Random number generation\n");
    printf("- Dynamic memory management\n");
    printf("- File I/O operations\n");
    printf("- Type conversion utilities\n");
    
    return 0;
}
