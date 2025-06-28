// Test 9: User-defined function call
// Tests: Function definition, function call, parameter passing

int add_numbers(int a, int b) {
    return a + b;
}

int main(void* program_data, size_t program_size) {
    int result = add_numbers(10, 20);
    return result;
}
