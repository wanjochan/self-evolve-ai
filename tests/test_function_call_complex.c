int add(int a, int b) {
    return a + b;
}

int main() {
    int result = add(10, 20);
    printf("Result: %d\n", result);
    
    // Test multiple function calls
    int len = strlen("Hello");
    int sum = add(len, 5);
    printf("Sum: %d\n", sum);
    
    return 0;
}
