// Test enhanced compiler features
int main(void* program_data, size_t program_size) {
    int arr[5];
    int x = 10;
    
    // Test array access
    arr[0] = x;
    arr[1] = x + 5;
    
    // Test arithmetic
    int result = arr[0] + arr[1];
    
    // Test control flow
    if (result > 20) {
        printf("Result is large: %d\n", result);
    }
    
    return result;
}
