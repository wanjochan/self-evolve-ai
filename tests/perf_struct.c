#include <stdio.h>

struct Point {
    int x, y;
};

int calculate_distance_squared(struct Point a, struct Point b) {
    int dx = a.x - b.x;
    int dy = a.y - b.y;
    return dx * dx + dy * dy;
}

int main() {
    struct Point points[100];
    
    for (int i = 0; i < 100; i++) {
        points[i].x = i;
        points[i].y = i * 2;
    }
    
    int total_distance = 0;
    for (int i = 0; i < 99; i++) {
        total_distance += calculate_distance_squared(points[i], points[i+1]);
    }
    
    printf("Total distance squared: %d\n", total_distance);
    return 0;
}
