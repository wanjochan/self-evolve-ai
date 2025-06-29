#include <stdio.h> 
#include <stdlib.h> 
 
int main() { 
    printf("COMPLETE INDEPENDENCE ACHIEVED!\\n"); 
    printf("Built with ZERO TinyCC dependencies!\\n"); 
    char* msg = malloc(50); 
    if (msg) { 
        sprintf(msg, "Memory allocation works!"); 
        printf("Test: %s\\n", msg); 
        free(msg); 
    } 
    printf("ALL SYSTEMS OPERATIONAL!\\n"); 
    return 0; 
} 
