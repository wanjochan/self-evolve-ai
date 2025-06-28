// Test 7: Basic struct definition and access
// Tests: Struct definition, member access

typedef struct {
    int x;
    int y;
} Point;

int main(void* program_data, size_t program_size) {
    Point p;
    p.x = 10;
    p.y = 20;
    return p.x + p.y;
}
