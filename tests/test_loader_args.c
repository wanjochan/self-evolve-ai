/**
 * test_loader_args.c - 测试Loader的参数解析
 */

#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("=== 测试Loader参数解析 ===\n");
    
    // 模拟调用loader
    printf("模拟调用: evolver0_loader.exe -v evolver0_runtime.bin evolver0_program.astc\n");
    
    // 使用system调用来测试
    int result = system("evolver0_loader.exe -v evolver0_runtime.bin evolver0_program.astc");
    printf("Loader返回码: %d\n", result);
    
    if (result == 0) {
        printf("✅ Loader执行成功\n");
    } else {
        printf("❌ Loader执行失败\n");
        
        // 尝试不同的参数组合
        printf("\n尝试其他参数组合:\n");
        
        printf("1. 只有帮助参数:\n");
        result = system("evolver0_loader.exe -h");
        printf("返回码: %d\n", result);
        
        printf("2. 无参数:\n");
        result = system("evolver0_loader.exe");
        printf("返回码: %d\n", result);
        
        printf("3. 错误参数:\n");
        result = system("evolver0_loader.exe invalid.bin invalid.astc");
        printf("返回码: %d\n", result);
    }
    
    return 0;
}
