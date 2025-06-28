// Test 6: Array declaration and access
// Tests: Array declaration, array access

int main(void* program_data, size_t program_size) {
    int numbers[3];
    numbers[0] = 10;
    numbers[1] = 20;
    numbers[2] = 30;
    return numbers[1];
}
