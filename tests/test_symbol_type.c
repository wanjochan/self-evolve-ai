// Test symbol table and type system
struct Point {
    int x;
    int y;
};

int global_var = 42;

int add(int a, int b) {
    int result = a + b;
    return result;
}

int main() {
    int local_var = 10;
    struct Point p;
    int *ptr = &local_var;
    int arr[5];
    
    // Test function call
    int sum = add(local_var, global_var);
    
    // Test struct member access (if supported)
    p.x = 1;
    p.y = 2;
    
    return sum;
}
