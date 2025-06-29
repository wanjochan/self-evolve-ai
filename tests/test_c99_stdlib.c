#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

int main() {
    printf("=== C99 Standard Library Test ===\n");
    
    // Test string functions
    printf("Testing string functions:\n");
    char str1[] = "Hello";
    char str2[] = "World";
    printf("strlen(\"%s\") = %d\n", str1, strlen(str1));
    
    // Test character functions
    printf("\nTesting character functions:\n");
    char c = 'A';
    printf("isalpha('%c') = %d\n", c, isalpha(c));
    printf("tolower('%c') = '%c'\n", c, tolower(c));
    
    // Test conversion functions
    printf("\nTesting conversion functions:\n");
    char num_str[] = "123";
    int num = atoi(num_str);
    printf("atoi(\"%s\") = %d\n", num_str, num);
    
    // Test random functions
    printf("\nTesting random functions:\n");
    srand(42);
    int random_num = rand();
    printf("rand() = %d\n", random_num);
    
    printf("\nC99 Standard Library Test Complete!\n");
    return 0;
}
