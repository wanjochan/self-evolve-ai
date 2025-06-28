// Test 8: Basic pointer operations
// Tests: Pointer declaration, address-of, dereference

int main(void* program_data, size_t program_size) {
    int value = 42;
    int* ptr = &value;
    return *ptr;
}
