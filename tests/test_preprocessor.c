/**
 * test_preprocessor.c - 测试预处理器功能
 */

#include <stdio.h>
#define MAX_SIZE 100
#define GREETING "Hello from preprocessor!"

#ifdef DEBUG
#define LOG(msg) printf("DEBUG: %s\n", msg)
#else
#define LOG(msg)
#endif

int main() {
    printf(GREETING "\n");
    
    int size = MAX_SIZE;
    printf("Max size is: %d\n", size);
    
    LOG("This is a debug message");
    
    return 42;
}
