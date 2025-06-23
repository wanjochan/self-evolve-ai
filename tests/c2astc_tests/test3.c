/**
 * test3.c - 结构体和指针测试
 */

typedef struct {
    int x;
    int y;
} Point;

void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

Point create_point(int x, int y) {
    Point p;
    p.x = x;
    p.y = y;
    return p;
}

int main() {
    int a = 5;
    int b = 10;
    swap(&a, &b);
    
    Point p = create_point(a, b);
    
    return p.x + p.y > 10 ? 1 : 0;
} 