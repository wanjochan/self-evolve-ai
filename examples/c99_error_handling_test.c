/**
 * c99_error_handling_test.c - C99 Error Handling Test
 * 
 * This program tests various error handling mechanisms in C99:
 * - Error return codes
 * - errno global variable
 * - Signal handling
 * - Assertion failures
 * - Memory allocation failures
 * - File operation errors
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <setjmp.h>
#include <limits.h>
#include <stdint.h>
#include <math.h>

// Error codes
typedef enum {
    ERR_SUCCESS = 0,
    ERR_INVALID_PARAM = -1,
    ERR_OUT_OF_MEMORY = -2,
    ERR_FILE_NOT_FOUND = -3,
    ERR_PERMISSION_DENIED = -4,
    ERR_BUFFER_OVERFLOW = -5,
    ERR_DIVISION_BY_ZERO = -6
} ErrorCode;

// Global error state
static jmp_buf error_jump_buffer;
static int error_recovery_enabled = 0;

// Function prototypes
void test_return_code_errors(void);
void test_errno_errors(void);
void test_signal_handling(void);
void test_assertion_errors(void);
void test_memory_errors(void);
void test_file_errors(void);
void test_exception_simulation(void);

// Utility functions
const char* error_code_to_string(ErrorCode code);
ErrorCode safe_divide(int a, int b, int *result);
ErrorCode safe_string_copy(char *dest, size_t dest_size, const char *src);
void signal_handler(int sig);
void error_handler(const char *message, ErrorCode code);

int main(void) {
    printf("=== C99 Error Handling Test ===\n\n");
    
    test_return_code_errors();
    test_errno_errors();
    test_signal_handling();
    test_assertion_errors();
    test_memory_errors();
    test_file_errors();
    test_exception_simulation();
    
    printf("\n=== All error handling tests completed ===\n");
    return 0;
}

void test_return_code_errors(void) {
    printf("1. Testing Return Code Error Handling:\n");
    
    int result;
    ErrorCode err;
    
    // Test successful operation
    err = safe_divide(10, 2, &result);
    if (err == ERR_SUCCESS) {
        printf("   10 / 2 = %d (success)\n", result);
    } else {
        printf("   Division failed: %s\n", error_code_to_string(err));
    }
    
    // Test division by zero
    err = safe_divide(10, 0, &result);
    if (err == ERR_SUCCESS) {
        printf("   10 / 0 = %d (success)\n", result);
    } else {
        printf("   Division failed: %s\n", error_code_to_string(err));
    }
    
    // Test string copy operations
    char buffer[10];
    
    err = safe_string_copy(buffer, sizeof(buffer), "Hello");
    if (err == ERR_SUCCESS) {
        printf("   String copy successful: \"%s\"\n", buffer);
    } else {
        printf("   String copy failed: %s\n", error_code_to_string(err));
    }
    
    err = safe_string_copy(buffer, sizeof(buffer), "This string is too long for the buffer");
    if (err == ERR_SUCCESS) {
        printf("   String copy successful: \"%s\"\n", buffer);
    } else {
        printf("   String copy failed: %s\n", error_code_to_string(err));
    }
}

void test_errno_errors(void) {
    printf("\n2. Testing errno Error Handling:\n");
    
    // Clear errno
    errno = 0;
    
    // Test file operations that might fail
    FILE *file = fopen("/nonexistent/path/file.txt", "r");
    if (file == NULL) {
        printf("   fopen failed: %s (errno: %d)\n", strerror(errno), errno);
    } else {
        printf("   fopen succeeded unexpectedly\n");
        fclose(file);
    }
    
    // Test memory allocation with large size
    errno = 0;
    void *ptr = malloc(SIZE_MAX);
    if (ptr == NULL) {
        printf("   malloc failed: %s (errno: %d)\n", strerror(errno), errno);
    } else {
        printf("   malloc succeeded with SIZE_MAX\n");
        free(ptr);
    }
    
    // Test mathematical errors
    errno = 0;
    double result = sqrt(-1.0);
    if (errno != 0) {
        printf("   sqrt(-1) failed: %s (errno: %d), result: %f\n", 
               strerror(errno), errno, result);
    } else {
        printf("   sqrt(-1) = %f (no error)\n", result);
    }
}

void test_signal_handling(void) {
    printf("\n3. Testing Signal Handling:\n");
    
    // Install signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("   Signal handlers installed for SIGINT and SIGTERM\n");
    printf("   (Note: Actual signal testing requires external triggers)\n");
    
    // Simulate signal handling
    printf("   Simulating signal handling...\n");
    
    // Test raising a signal (commented out to avoid termination)
    // raise(SIGINT);
    
    printf("   Signal handling test completed\n");
}

void test_assertion_errors(void) {
    printf("\n4. Testing Assertion Error Handling:\n");
    
    // Test successful assertions
    assert(1 == 1);
    assert(strlen("test") == 4);
    printf("   Basic assertions passed\n");
    
    // Test assertion with complex condition
    int array[] = {1, 2, 3, 4, 5};
    size_t array_size = sizeof(array) / sizeof(array[0]);
    assert(array_size == 5);
    printf("   Array size assertion passed\n");
    
    // Note: We don't test failing assertions as they would terminate the program
    printf("   (Note: Failing assertions would terminate the program)\n");
    
    // Show how to disable assertions
    #ifdef NDEBUG
    printf("   Assertions are disabled (NDEBUG defined)\n");
    #else
    printf("   Assertions are enabled\n");
    #endif
}

void test_memory_errors(void) {
    printf("\n5. Testing Memory Error Handling:\n");
    
    // Test normal allocation
    void *ptr1 = malloc(1024);
    if (ptr1) {
        printf("   malloc(1024) succeeded\n");
        free(ptr1);
        printf("   Memory freed successfully\n");
    } else {
        printf("   malloc(1024) failed\n");
    }
    
    // Test large allocation that might fail
    size_t huge_size = SIZE_MAX / 2;
    void *ptr2 = malloc(huge_size);
    if (ptr2) {
        printf("   malloc(%zu) succeeded unexpectedly\n", huge_size);
        free(ptr2);
    } else {
        printf("   malloc(%zu) failed as expected\n", huge_size);
    }
    
    // Test realloc with NULL pointer
    void *ptr3 = realloc(NULL, 100);
    if (ptr3) {
        printf("   realloc(NULL, 100) succeeded\n");
        
        // Test realloc expansion
        ptr3 = realloc(ptr3, 200);
        if (ptr3) {
            printf("   realloc expansion to 200 succeeded\n");
        } else {
            printf("   realloc expansion failed\n");
        }
        
        free(ptr3);
    } else {
        printf("   realloc(NULL, 100) failed\n");
    }
    
    // Test calloc
    int *arr = calloc(10, sizeof(int));
    if (arr) {
        printf("   calloc(10, sizeof(int)) succeeded\n");
        
        // Verify zero initialization
        int all_zero = 1;
        for (int i = 0; i < 10; i++) {
            if (arr[i] != 0) {
                all_zero = 0;
                break;
            }
        }
        printf("   calloc zero-initialization: %s\n", all_zero ? "verified" : "failed");
        
        free(arr);
    } else {
        printf("   calloc(10, sizeof(int)) failed\n");
    }
}

void test_file_errors(void) {
    printf("\n6. Testing File Error Handling:\n");
    
    // Test opening non-existent file
    FILE *file1 = fopen("nonexistent_file.txt", "r");
    if (file1) {
        printf("   Opening non-existent file succeeded unexpectedly\n");
        fclose(file1);
    } else {
        printf("   Opening non-existent file failed as expected: %s\n", strerror(errno));
    }
    
    // Test creating and writing to a file
    FILE *file2 = fopen("test_output.txt", "w");
    if (file2) {
        printf("   Created test file successfully\n");
        
        int write_result = fprintf(file2, "Test data\n");
        if (write_result > 0) {
            printf("   Write to file succeeded (%d characters)\n", write_result);
        } else {
            printf("   Write to file failed\n");
        }
        
        fclose(file2);
        
        // Try to remove the file
        if (remove("test_output.txt") == 0) {
            printf("   Test file removed successfully\n");
        } else {
            printf("   Failed to remove test file: %s\n", strerror(errno));
        }
    } else {
        printf("   Failed to create test file: %s\n", strerror(errno));
    }
    
    // Test reading from invalid file descriptor
    printf("   (Note: Additional file error tests would require specific file system conditions)\n");
}

void test_exception_simulation(void) {
    printf("\n7. Testing Exception Simulation (setjmp/longjmp):\n");
    
    error_recovery_enabled = 1;
    
    if (setjmp(error_jump_buffer) == 0) {
        printf("   Setting up error recovery point\n");
        
        // Simulate an error condition
        error_handler("Simulated critical error", ERR_OUT_OF_MEMORY);
        
        printf("   This line should not be reached\n");
    } else {
        printf("   Recovered from error using longjmp\n");
    }
    
    error_recovery_enabled = 0;
    printf("   Exception simulation completed\n");
}

// Utility function implementations
const char* error_code_to_string(ErrorCode code) {
    switch (code) {
        case ERR_SUCCESS: return "Success";
        case ERR_INVALID_PARAM: return "Invalid parameter";
        case ERR_OUT_OF_MEMORY: return "Out of memory";
        case ERR_FILE_NOT_FOUND: return "File not found";
        case ERR_PERMISSION_DENIED: return "Permission denied";
        case ERR_BUFFER_OVERFLOW: return "Buffer overflow";
        case ERR_DIVISION_BY_ZERO: return "Division by zero";
        default: return "Unknown error";
    }
}

ErrorCode safe_divide(int a, int b, int *result) {
    if (result == NULL) {
        return ERR_INVALID_PARAM;
    }
    
    if (b == 0) {
        return ERR_DIVISION_BY_ZERO;
    }
    
    *result = a / b;
    return ERR_SUCCESS;
}

ErrorCode safe_string_copy(char *dest, size_t dest_size, const char *src) {
    if (dest == NULL || src == NULL) {
        return ERR_INVALID_PARAM;
    }
    
    if (strlen(src) >= dest_size) {
        return ERR_BUFFER_OVERFLOW;
    }
    
    strcpy(dest, src);
    return ERR_SUCCESS;
}

void signal_handler(int sig) {
    printf("   Signal %d received\n", sig);
    
    switch (sig) {
        case SIGINT:
            printf("   Handling SIGINT (Interrupt)\n");
            break;
        case SIGTERM:
            printf("   Handling SIGTERM (Termination)\n");
            break;
        default:
            printf("   Handling unknown signal\n");
            break;
    }
}

void error_handler(const char *message, ErrorCode code) {
    printf("   Error occurred: %s (%s)\n", message, error_code_to_string(code));
    
    if (error_recovery_enabled) {
        printf("   Performing error recovery...\n");
        longjmp(error_jump_buffer, 1);
    } else {
        printf("   No error recovery available\n");
    }
}
