#include <stdio.h>
#include <stdlib.h>
int main() {
    char* ptr = malloc(100);
    if (ptr) {
        printf("malloc SUCCESS\n");
        free(ptr);
        printf("free SUCCESS\n");
    } else {
        printf("malloc FAILED\n");
    }
    return 0;
}
