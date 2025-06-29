#include <stdio.h>

int main(int argc, char* argv[]) {
    printf("Testing complex function parameters\n");
    printf("argc = %d\n", argc);
    if (argc > 0) {
        printf("argv[0] = %s\n", argv[0]);
    }
    return 0;
}

void test_function(const char* str, int* ptr, float values[]) {
    printf("Complex function with multiple parameter types\n");
}
