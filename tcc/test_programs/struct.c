#include <stdio.h>

struct Point {
    int x, y;
};

int main() {
    struct Point p = {10, 20};
    printf("Point: (%d, %d)\n", p.x, p.y);
    printf("TCC struct test passed!\n");
    return 0;
}
