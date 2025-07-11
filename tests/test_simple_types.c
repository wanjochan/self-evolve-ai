// Simple test for complex types
struct Point {
    int x;
    int y;
};

union Data {
    int i;
    float f;
};

int main() {
    int a = 10;
    int* ptr = &a;
    int arr[3];
    
    struct Point p;
    p.x = 5;
    p.y = 10;
    
    union Data data;
    data.i = 42;
    
    a += 5;
    
    return 0;
}
