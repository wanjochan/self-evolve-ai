#include <stdio.h>
#include <stdlib.h>

struct Point {
    int x, y;
};

struct Rectangle {
    struct Point top_left;
    struct Point bottom_right;
};

int calculate_area(struct Rectangle rect) {
    int width = rect.bottom_right.x - rect.top_left.x;
    int height = rect.bottom_right.y - rect.top_left.y;
    return width * height;
}

void print_rectangle(struct Rectangle rect) {
    printf("Rectangle: (%d,%d) to (%d,%d), Area: %d\n",
           rect.top_left.x, rect.top_left.y,
           rect.bottom_right.x, rect.bottom_right.y,
           calculate_area(rect));
}

int main() {
    struct Rectangle rectangles[10];
    
    for (int i = 0; i < 10; i++) {
        rectangles[i].top_left.x = i;
        rectangles[i].top_left.y = i;
        rectangles[i].bottom_right.x = i + 10;
        rectangles[i].bottom_right.y = i + 10;
    }
    
    int total_area = 0;
    for (int i = 0; i < 10; i++) {
        print_rectangle(rectangles[i]);
        total_area += calculate_area(rectangles[i]);
    }
    
    printf("Total area: %d\n", total_area);
    return 0;
}
