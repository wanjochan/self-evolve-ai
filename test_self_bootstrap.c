/**
 * test_self_bootstrap.c - 直接测试自举编译功能
 * 
 * 这个程序直接调用evolver0_program.c中的自举编译函数，
 * 绕过ASTC虚拟机的复杂性，验证自举编译逻辑是否正确。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// 从evolver0_program.c复制必要的函数声明
int generate_evolver1_loader_source();
int generate_evolver1_runtime_source();
int generate_evolver1_program_source();
int self_bootstrap();

int test_main() {
    printf("=== 直接测试自举编译功能 ===\n");
    printf("测试目标: 验证evolver0能否生成evolver1源代码\n\n");

    // 创建evolver1目录（如果不存在）
    system("mkdir src\\evolver1 2>nul");

    printf("步骤1: 测试生成evolver1_loader源代码...\n");
    int result1 = generate_evolver1_loader_source();
    if (result1 == 0) {
        printf("✅ evolver1_loader.c 生成成功\n");
    } else {
        printf("❌ evolver1_loader.c 生成失败\n");
    }

    printf("\n步骤2: 测试生成evolver1_runtime源代码...\n");
    int result2 = generate_evolver1_runtime_source();
    if (result2 == 0) {
        printf("✅ evolver1_runtime.c 生成成功\n");
    } else {
        printf("❌ evolver1_runtime.c 生成失败\n");
    }

    printf("\n步骤3: 测试生成evolver1_program源代码...\n");
    int result3 = generate_evolver1_program_source();
    if (result3 == 0) {
        printf("✅ evolver1_program.c 生成成功\n");
    } else {
        printf("❌ evolver1_program.c 生成失败\n");
    }

    printf("\n步骤4: 执行完整自举编译测试...\n");
    int bootstrap_result = self_bootstrap();
    if (bootstrap_result == 100) {
        printf("✅ 自举编译测试成功\n");
    } else {
        printf("❌ 自举编译测试失败，返回值: %d\n", bootstrap_result);
    }

    printf("\n=== 测试结果总结 ===\n");
    printf("Loader生成: %s\n", result1 == 0 ? "成功" : "失败");
    printf("Runtime生成: %s\n", result2 == 0 ? "成功" : "失败");
    printf("Program生成: %s\n", result3 == 0 ? "成功" : "失败");
    printf("自举编译: %s\n", bootstrap_result == 100 ? "成功" : "失败");

    if (result1 == 0 && result2 == 0 && result3 == 0 && bootstrap_result == 100) {
        printf("\n🎉 所有测试通过！evolver0具备完整的自举编译能力！\n");
        return 0;
    } else {
        printf("\n⚠️ 部分测试失败，需要进一步调试自举编译功能。\n");
        return 1;
    }
}

// 为了编译这个测试程序，我们需要包含evolver0_program.c中的函数实现
// 但为了避免重复定义，我们使用条件编译

#ifdef INCLUDE_EVOLVER0_PROGRAM_FUNCTIONS

// 这里应该包含evolver0_program.c中的函数实现
// 但由于文件太大，我们将使用链接的方式

#endif
