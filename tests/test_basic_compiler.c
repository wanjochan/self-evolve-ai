/**
 * test_basic_compiler.c - 测试基本编译器功能
 * 
 * 这个测试验证evolver0的基本C编译能力
 */

int add(int a, int b) {
    return a + b;
}

int subtract(int a, int b) {
    return a - b;
}

int multiply(int a, int b) {
    return a * b;
}

int main() {
    int x = 10;
    int y = 5;
    
    int sum = add(x, y);        // 15
    int diff = subtract(x, y);  // 5
    int prod = multiply(x, y);  // 50
    
    // 测试控制流
    if (sum > diff) {
        prod = prod + 1;        // 51
    }
    
    // 测试循环
    int count = 0;
    while (count < 3) {
        prod = prod + count;    // 51 + 0 + 1 + 2 = 54
        count++;
    }
    
    return prod; // 应该返回54
}
