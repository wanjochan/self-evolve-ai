/**
 * run_bootstrap_test.c - 运行自举编译测试的包装器
 */

#include <stdio.h>
#include <stdlib.h>

// 声明测试函数
int test_main();

int main() {
    printf("=== 启动自举编译测试 ===\n");
    return test_main();
}
