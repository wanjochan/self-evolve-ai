/**
 * real_compile_test.c - 真实编译能力测试
 * 测试evolver0是否真的能编译C代码
 */

int add(int a, int b) {
    return a + b;
}

int main() {
    int result = add(10, 20);
    return result;
}
