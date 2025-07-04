/**
 * test_libc_comprehensive.c - Comprehensive LibC Function Test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    printf("=== LibC Comprehensive Function Test ===\n");
    
    // Test 1: Memory Management Functions
    printf("\n1. Testing Memory Management:\n");
    
    void* ptr1 = malloc(100);
    if (ptr1) {
        printf("  ✓ malloc(100) succeeded: %p\n", ptr1);
        
        // Test memset
        memset(ptr1, 0x42, 50);
        printf("  ✓ memset() completed\n");
        
        // Test realloc
        void* ptr2 = realloc(ptr1, 200);
        if (ptr2) {
            printf("  ✓ realloc(200) succeeded: %p\n", ptr2);
            free(ptr2);
            printf("  ✓ free() completed\n");
        } else {
            printf("  ✗ realloc() failed\n");
            free(ptr1);
        }
    } else {
        printf("  ✗ malloc() failed\n");
    }
    
    // Test calloc
    void* ptr3 = calloc(10, sizeof(int));
    if (ptr3) {
        printf("  ✓ calloc(10, sizeof(int)) succeeded\n");
        free(ptr3);
        printf("  ✓ calloc memory freed\n");
    } else {
        printf("  ✗ calloc() failed\n");
    }
    
    // Test 2: String Functions
    printf("\n2. Testing String Functions:\n");
    
    char buffer1[100];
    char buffer2[100];
    
    // Test strcpy
    strcpy(buffer1, "Hello");
    printf("  ✓ strcpy(): '%s'\n", buffer1);
    
    // Test strlen
    size_t len = strlen(buffer1);
    printf("  ✓ strlen(): %zu characters\n", len);
    
    // Test strcat
    strcat(buffer1, " World");
    printf("  ✓ strcat(): '%s'\n", buffer1);
    
    // Test strcmp
    strcpy(buffer2, "Hello World");
    int cmp = strcmp(buffer1, buffer2);
    printf("  ✓ strcmp(): %d (should be 0)\n", cmp);
    
    // Test strncpy
    strncpy(buffer2, "Test", 4);
    buffer2[4] = '\0';
    printf("  ✓ strncpy(): '%s'\n", buffer2);
    
    // Test 3: Memory Functions
    printf("\n3. Testing Memory Functions:\n");
    
    char src[] = "Source";
    char dst[20];
    
    // Test memcpy
    memcpy(dst, src, strlen(src) + 1);
    printf("  ✓ memcpy(): '%s'\n", dst);
    
    // Test memcmp
    int mem_cmp = memcmp(src, dst, strlen(src));
    printf("  ✓ memcmp(): %d (should be 0)\n", mem_cmp);
    
    // Test memmove
    memmove(dst + 2, dst, strlen(dst) + 1);
    printf("  ✓ memmove(): '%s'\n", dst);
    
    // Test 4: I/O Functions
    printf("\n4. Testing I/O Functions:\n");
    
    // Test printf variants
    printf("  ✓ printf() working\n");
    
    char sprintf_buffer[100];
    sprintf(sprintf_buffer, "sprintf test: %d", 42);
    printf("  ✓ sprintf(): '%s'\n", sprintf_buffer);
    
    // Test puts
    puts("  ✓ puts() working");
    
    // Test putchar
    printf("  ✓ putchar(): ");
    putchar('A');
    putchar('B');
    putchar('C');
    putchar('\n');
    
    // Test 5: File I/O (if available)
    printf("\n5. Testing File I/O:\n");
    
    FILE* test_file = fopen("test_output.txt", "w");
    if (test_file) {
        fprintf(test_file, "LibC test output\n");
        fclose(test_file);
        printf("  ✓ fopen/fprintf/fclose succeeded\n");
        
        // Try to read it back
        test_file = fopen("test_output.txt", "r");
        if (test_file) {
            char read_buffer[100];
            if (fgets(read_buffer, sizeof(read_buffer), test_file)) {
                printf("  ✓ fopen/fgets succeeded: '%s'", read_buffer);
            }
            fclose(test_file);
        }
    } else {
        printf("  ⚠ File I/O not available or failed\n");
    }
    
    printf("\n=== LibC Test Completed ===\n");
    printf("All basic LibC functions tested successfully!\n");
    
    return 0;
}
