/**
 * test2.c - 循环结构测试
 */

int sum(int n) {
    int result = 0;
    for (int i = 1; i <= n; i++) {
        result += i;
    }
    return result;
}

int main() {
    int x = 10;
    int total = sum(x);
    
    int i = 0;
    while (i < 5) {
        total += i;
        i++;
    }
    
    return total > 50 ? 1 : 0;
} 