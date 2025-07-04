#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
int main() { 
    printf("Testing LibC functions...\\n"); 
    char* ptr = malloc(100); 
    strcpy(ptr, "Hello LibC!"); 
    printf("String: %s\\n", ptr); 
    free(ptr); 
    return 0; 
} 
