/**
 * libc_module.c - Simple libc Module for Testing
 * 
 * This will be compiled to libc_x64_64.native for testing
 */

#include "../src/core/include/module_attributes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

MODULE("libc")
VERSION(2, 0, 0)
AUTHOR("Self-Evolve AI")
DESCRIPTION("Standard C Library Forwarding Module")
LICENSE("MIT")
void libc_init(void) {
    // libc initialization
}

EXPORT_FUNC
void* libc_malloc(size_t size) {
    return malloc(size);
}

EXPORT_FUNC
void libc_free(void* ptr) {
    free(ptr);
}

EXPORT_FUNC
int libc_printf(const char* format, ...) {
    // Simple printf forwarding
    if (!format) return -1;
    
    // For testing, just return success
    return 0;
}

EXPORT_FUNC
char* libc_strcpy(char* dest, const char* src) {
    if (!dest || !src) return NULL;
    return strcpy(dest, src);
}

EXPORT_FUNC
size_t libc_strlen(const char* str) {
    if (!str) return 0;
    return strlen(str);
}

EXPORT_FUNC
int libc_strcmp(const char* str1, const char* str2) {
    if (!str1 || !str2) return -1;
    return strcmp(str1, str2);
}

EXPORT_FUNC
void libc_cleanup(void) {
    // libc cleanup
}

// Main entry point for the libc module
int main(int argc, char* argv[]) {
    libc_init();
    
    // Test basic functionality
    void* test_ptr = libc_malloc(100);
    if (test_ptr) {
        libc_free(test_ptr);
    }
    
    libc_cleanup();
    return 0;
}
