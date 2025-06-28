#include <stdio.h>

struct Point {
    int x;
    int y;
};

int main() {
    struct Point p;
    p.x = 10;
    p.y = 20;
    
    int arr[3];
    arr[0] = 1;
    arr[1] = 2;
    arr[2] = 3;
    
    printf("Point: (%d, %d)\n", p.x, p.y);
    printf("Array: [%d, %d, %d]\n", arr[0], arr[1], arr[2]);
    
    return 0;
}
