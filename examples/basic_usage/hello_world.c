/**
 * Hello World 示例
 * 
 * 这是一个最基本的 Self-Evolving AI 程序示例，
 * 展示了如何使用 c99bin 编译器编译和运行程序。
 */

#include <stdio.h>

int main() {
    printf("Hello, Self-Evolving AI!\n");
    printf("这是您的第一个程序。\n");
    
    // 返回成功状态
    return 0;
}

/*
编译和运行说明：

1. 使用 c99bin 编译：
   ./c99bin.sh hello_world.c -o hello_world

2. 运行程序：
   ./hello_world

3. 预期输出：
   Hello, Self-Evolving AI!
   这是您的第一个程序。

4. 检查退出状态：
   echo $?
   # 应该输出 0
*/
