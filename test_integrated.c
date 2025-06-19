int main() {
    int a = 10;
    int b = 20;
    int c = a + b;
    
    if (c > 25) {
        return c + 12;  // 应该返回42
    } else {
        return 0;
    }
}