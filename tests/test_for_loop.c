/**
 * test_for_loop.c - 测试for循环序列化支持
 */

int main() {
    int sum = 0;
    
    // 测试for循环
    for (int i = 0; i < 10; i++) {
        sum = sum + i;
    }
    
    // 测试while循环
    int j = 0;
    while (j < 5) {
        sum = sum + j;
        j++;
    }
    
    return sum;
}
