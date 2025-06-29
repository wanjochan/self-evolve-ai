#include <stdio.h>

int main() {
    int result = 10 + 20 * 3;  // 应该被折叠为 70
    printf("Result: %d\n", result);
    return 0;
}
