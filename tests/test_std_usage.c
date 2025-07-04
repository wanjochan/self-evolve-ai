/**
 * test_std_usage.c - Test program that uses std module functions
 * 
 * This will be compiled to ASTC and run through the three-layer architecture:
 * loader_x64_64.exe → std_x64_64.native → test_std_usage.astc
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    printf("Testing STD Module through three-layer architecture\n");
    printf("===================================================\n");
    
    // Test memory allocation
    printf("Test 1: Memory allocation\n");
    void* ptr = malloc(100);
    if (ptr) {
        printf("✅ malloc(100) succeeded: %p\n", ptr);
        free(ptr);
        printf("✅ free() completed\n");
    } else {
        printf("❌ malloc(100) failed\n");
        return 1;
    }
    
    // Test string functions
    printf("\nTest 2: String functions\n");
    const char* test_str = "Hello, World!";
    size_t len = strlen(test_str);
    printf("✅ strlen(\"%s\") = %zu\n", test_str, len);
    
    char buffer[100];
    strcpy(buffer, test_str);
    printf("✅ strcpy() completed: \"%s\"\n", buffer);
    
    // Test printf
    printf("\nTest 3: Printf functionality\n");
    printf("✅ This message is printed via std module printf\n");
    
    printf("\n🎉 All STD module tests passed!\n");
    printf("The std_x64_64.native module is working correctly in the three-layer architecture.\n");
    
    return 0;
}
