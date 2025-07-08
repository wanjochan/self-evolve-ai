/**
 * c99_stdlib_test.c - C99 Standard Library Functions Test
 * 
 * This program tests various C99 standard library functions including:
 * - String manipulation functions
 * - Memory management functions
 * - Mathematical functions
 * - Input/output functions
 * - Character classification functions
 * - Time and date functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>
#include <limits.h>
#include <float.h>

// Function prototypes
void test_string_functions(void);
void test_memory_functions(void);
void test_math_functions(void);
void test_io_functions(void);
void test_character_functions(void);
void test_time_functions(void);
void test_conversion_functions(void);
void test_assertion_functions(void);

int main(void) {
    printf("=== C99 Standard Library Functions Test ===\n\n");
    
    test_string_functions();
    test_memory_functions();
    test_math_functions();
    test_io_functions();
    test_character_functions();
    test_time_functions();
    test_conversion_functions();
    test_assertion_functions();
    
    printf("\n=== All standard library tests completed ===\n");
    return 0;
}

void test_string_functions(void) {
    printf("1. Testing String Functions:\n");
    
    char str1[100] = "Hello";
    char str2[100] = "World";
    char str3[100];
    
    // String length
    printf("   strlen(\"%s\") = %zu\n", str1, strlen(str1));
    
    // String copy
    strcpy(str3, str1);
    printf("   strcpy result: \"%s\"\n", str3);
    
    // String concatenation
    strcat(str1, " ");
    strcat(str1, str2);
    printf("   strcat result: \"%s\"\n", str1);
    
    // String comparison
    int cmp = strcmp("apple", "banana");
    printf("   strcmp(\"apple\", \"banana\") = %d\n", cmp);
    
    // String search
    char *pos = strstr(str1, "World");
    if (pos) {
        printf("   strstr found \"World\" at position: %ld\n", pos - str1);
    }
    
    // Character search
    char *ch_pos = strchr(str1, 'W');
    if (ch_pos) {
        printf("   strchr found 'W' at position: %ld\n", ch_pos - str1);
    }
    
    // String tokenization
    char test_str[] = "apple,banana,cherry";
    char *token = strtok(test_str, ",");
    printf("   strtok tokens: ");
    while (token) {
        printf("\"%s\" ", token);
        token = strtok(NULL, ",");
    }
    printf("\n");
}

void test_memory_functions(void) {
    printf("\n2. Testing Memory Functions:\n");
    
    // Dynamic memory allocation
    int *arr = malloc(5 * sizeof(int));
    if (arr) {
        printf("   malloc: allocated array of 5 integers\n");
        
        // Initialize array
        for (int i = 0; i < 5; i++) {
            arr[i] = i * i;
        }
        
        printf("   Array contents: ");
        for (int i = 0; i < 5; i++) {
            printf("%d ", arr[i]);
        }
        printf("\n");
        
        // Reallocate memory
        arr = realloc(arr, 10 * sizeof(int));
        if (arr) {
            printf("   realloc: expanded to 10 integers\n");
            
            // Initialize new elements
            for (int i = 5; i < 10; i++) {
                arr[i] = i * i;
            }
        }
        
        free(arr);
        printf("   free: memory deallocated\n");
    }
    
    // Memory operations
    char buffer1[20] = "Hello World";
    char buffer2[20];
    
    // Memory copy
    memcpy(buffer2, buffer1, strlen(buffer1) + 1);
    printf("   memcpy result: \"%s\"\n", buffer2);
    
    // Memory move (overlapping regions)
    memmove(buffer1 + 2, buffer1, 5);
    printf("   memmove result: \"%s\"\n", buffer1);
    
    // Memory set
    memset(buffer2, 'X', 5);
    buffer2[5] = '\0';
    printf("   memset result: \"%s\"\n", buffer2);
    
    // Memory compare
    char buf1[] = "test";
    char buf2[] = "test";
    int mem_cmp = memcmp(buf1, buf2, 4);
    printf("   memcmp(\"%s\", \"%s\") = %d\n", buf1, buf2, mem_cmp);
}

void test_math_functions(void) {
    printf("\n3. Testing Math Functions:\n");
    
    double x = 2.5, y = 3.0;
    
    // Basic math functions
    printf("   sqrt(%.1f) = %.3f\n", x * x, sqrt(x * x));
    printf("   pow(%.1f, %.1f) = %.3f\n", x, y, pow(x, y));
    printf("   exp(%.1f) = %.3f\n", 1.0, exp(1.0));
    printf("   log(%.3f) = %.3f\n", exp(1.0), log(exp(1.0)));
    
    // Trigonometric functions
    double angle = 3.14159 / 4;  // 45 degrees in radians
    printf("   sin(π/4) = %.3f\n", sin(angle));
    printf("   cos(π/4) = %.3f\n", cos(angle));
    printf("   tan(π/4) = %.3f\n", tan(angle));
    
    // Rounding functions
    double val = 3.7;
    printf("   floor(%.1f) = %.0f\n", val, floor(val));
    printf("   ceil(%.1f) = %.0f\n", val, ceil(val));
    printf("   round(%.1f) = %.0f\n", val, round(val));
    
    // Absolute value
    printf("   abs(-42) = %d\n", abs(-42));
    printf("   fabs(-3.14) = %.2f\n", fabs(-3.14));
    
    // Random numbers
    srand((unsigned int)time(NULL));
    printf("   Random numbers: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", rand() % 100);
    }
    printf("\n");
}

void test_io_functions(void) {
    printf("\n4. Testing I/O Functions:\n");
    
    // Formatted output
    int num = 42;
    float fnum = 3.14159f;
    char str[] = "test";
    
    printf("   printf formatting:\n");
    printf("     Integer: %d, %x, %o\n", num, num, num);
    printf("     Float: %.2f, %e, %g\n", fnum, fnum, fnum);
    printf("     String: '%s', '%10s', '%-10s'\n", str, str, str);
    
    // String formatting
    char buffer[100];
    int len = sprintf(buffer, "Formatted: %d + %.2f = %.2f", num, fnum, num + fnum);
    printf("   sprintf result (%d chars): %s\n", len, buffer);
    
    // Character I/O simulation (without actual input)
    printf("   Character I/O functions available: getchar, putchar, etc.\n");
    
    // File I/O would require actual files, so we just mention it
    printf("   File I/O functions available: fopen, fclose, fread, fwrite, etc.\n");
}

void test_character_functions(void) {
    printf("\n5. Testing Character Functions:\n");
    
    char test_chars[] = "Hello123 World!";
    
    printf("   Character classification for \"%s\":\n", test_chars);
    
    for (int i = 0; test_chars[i]; i++) {
        char c = test_chars[i];
        printf("     '%c': ", c);
        
        if (isalpha(c)) printf("alpha ");
        if (isdigit(c)) printf("digit ");
        if (isalnum(c)) printf("alnum ");
        if (isspace(c)) printf("space ");
        if (ispunct(c)) printf("punct ");
        if (isupper(c)) printf("upper ");
        if (islower(c)) printf("lower ");
        
        printf("\n");
    }
    
    // Character conversion
    printf("   Character conversion:\n");
    printf("     toupper('a') = '%c'\n", toupper('a'));
    printf("     tolower('Z') = '%c'\n", tolower('Z'));
}

void test_time_functions(void) {
    printf("\n6. Testing Time Functions:\n");
    
    // Current time
    time_t current_time = time(NULL);
    printf("   Current time (seconds since epoch): %ld\n", (long)current_time);
    
    // Time string
    char *time_str = ctime(&current_time);
    if (time_str) {
        // Remove newline from ctime result
        time_str[strlen(time_str) - 1] = '\0';
        printf("   Current time string: %s\n", time_str);
    }
    
    // Broken-down time
    struct tm *tm_info = localtime(&current_time);
    if (tm_info) {
        printf("   Broken-down time: %04d-%02d-%02d %02d:%02d:%02d\n",
               tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
               tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    }
    
    // Clock for measuring execution time
    clock_t start = clock();
    
    // Simulate some work
    volatile int sum = 0;
    for (int i = 0; i < 100000; i++) {
        sum += i;
    }
    
    clock_t end = clock();
    double cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("   CPU time for loop: %.6f seconds\n", cpu_time);
}

void test_conversion_functions(void) {
    printf("\n7. Testing Conversion Functions:\n");
    
    // String to number conversions
    char *str_int = "12345";
    char *str_float = "3.14159";
    char *str_hex = "0xFF";
    
    int int_val = atoi(str_int);
    double float_val = atof(str_float);
    long hex_val = strtol(str_hex, NULL, 0);
    
    printf("   atoi(\"%s\") = %d\n", str_int, int_val);
    printf("   atof(\"%s\") = %.5f\n", str_float, float_val);
    printf("   strtol(\"%s\", NULL, 0) = %ld\n", str_hex, hex_val);
    
    // Advanced string to number conversion
    char *endptr;
    double val = strtod("123.45abc", &endptr);
    printf("   strtod(\"123.45abc\") = %.2f, remaining: \"%s\"\n", val, endptr);
    
    // Number to string conversion (using sprintf)
    char buffer[50];
    sprintf(buffer, "%d", 12345);
    printf("   Number to string: %d -> \"%s\"\n", 12345, buffer);
}

void test_assertion_functions(void) {
    printf("\n8. Testing Assertion Functions:\n");
    
    // Basic assertions (these should pass)
    assert(1 == 1);
    assert(strlen("hello") == 5);
    assert(2 + 2 == 4);
    
    printf("   All assertions passed successfully\n");
    
    // Show limits and constants
    printf("   System limits:\n");
    printf("     INT_MAX = %d\n", INT_MAX);
    printf("     INT_MIN = %d\n", INT_MIN);
    printf("     CHAR_MAX = %d\n", CHAR_MAX);
    printf("     CHAR_MIN = %d\n", CHAR_MIN);
    
    printf("   Float limits:\n");
    printf("     FLT_MAX = %e\n", FLT_MAX);
    printf("     FLT_MIN = %e\n", FLT_MIN);
    printf("     DBL_MAX = %e\n", DBL_MAX);
    printf("     DBL_MIN = %e\n", DBL_MIN);
}
