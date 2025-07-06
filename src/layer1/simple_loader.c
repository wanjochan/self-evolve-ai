/**
 * simple_loader.c - 简单的Layer 1加载器
 *
 * 基于模块系统的加载器实现，符合PRD.md中定义的三层架构的Layer 1。
 * 使用我们自己的module.h/module.c系统，而不是直接调用libc。
 */

#include "../core/module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ===============================================
// 架构检测
// ===============================================
#if defined(__x86_64__) || defined(_M_X64)
    #define ARCH_NAME "x86_64"
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define ARCH_NAME "arm64"
#elif defined(__i386__) || defined(_M_IX86)
    #define ARCH_NAME "x86_32"
#else
    #error "不支持的架构"
#endif

// ===============================================
// 简单加载器实现
// ===============================================

/**
 * 使用模块系统加载VM模块并执行ASTC程序
 */
int load_and_execute_program(const char* program_path, int argc, char* argv[]) {
    printf("Simple Loader: 初始化模块系统\n");

    // 初始化模块系统
    if (module_init() != 0) {
        printf("错误: 模块系统初始化失败\n");
        return 1;
    }

    // 构建VM模块名称
    char vm_module_name[256];
    snprintf(vm_module_name, sizeof(vm_module_name), "vm_%s", ARCH_NAME);

    printf("Simple Loader: 加载VM模块 '%s'\n", vm_module_name);

    // 加载VM模块
    Module* vm_module = module_load(vm_module_name);
    if (!vm_module) {
        printf("错误: 无法加载VM模块 '%s'\n", vm_module_name);
        printf("提示: 请确保 vm_%s.native 文件存在\n", ARCH_NAME);
        module_cleanup();
        return 1;
    }

    printf("Simple Loader: VM模块加载成功\n");

    // 查找VM执行函数
    // 尝试不同的函数名称
    typedef int (*vm_execute_func_t)(const char*, int, char**);
    vm_execute_func_t vm_execute = NULL;

    // 尝试标准的VM执行函数
    vm_execute = (vm_execute_func_t)module_resolve(vm_module, "vm_execute_astc");
    if (!vm_execute) {
        vm_execute = (vm_execute_func_t)module_resolve(vm_module, "execute_astc");
    }
    if (!vm_execute) {
        vm_execute = (vm_execute_func_t)module_resolve(vm_module, "main");
    }
    if (!vm_execute) {
        vm_execute = (vm_execute_func_t)module_resolve(vm_module, "native_main");
    }

    if (!vm_execute) {
        printf("错误: VM模块中未找到执行函数\n");
        printf("尝试查找的函数: vm_execute_astc, execute_astc, main, native_main\n");
        module_cleanup();
        return 1;
    }

    printf("Simple Loader: 找到VM执行函数，开始执行程序: %s\n", program_path);

    // 执行ASTC程序
    int result = vm_execute(program_path, argc, argv);

    printf("Simple Loader: 程序执行完成，返回值: %d\n", result);

    // 清理模块系统
    module_cleanup();

    return result;
}

// ===============================================
// 主函数
// ===============================================

int main(int argc, char* argv[]) {
    printf("Self-Evolve AI Simple Loader v1.0\n");
    printf("==================================\n");
    printf("架构: %s\n", ARCH_NAME);
    printf("PRD三层架构 - Layer 1 (Loader)\n");
    printf("\n");

    if (argc < 2) {
        printf("用法: %s <程序.astc> [参数...]\n", argv[0]);
        printf("\n");
        printf("说明:\n");
        printf("  这是PRD.md定义的三层架构中的Layer 1 (Loader)\n");
        printf("  它会自动加载对应架构的VM模块 (Layer 2)\n");
        printf("  然后执行指定的ASTC程序 (Layer 3)\n");
        printf("\n");
        printf("示例:\n");
        printf("  %s hello.astc\n", argv[0]);
        printf("  %s test.astc arg1 arg2\n", argv[0]);
        return 1;
    }

    const char* program_path = argv[1];

    printf("Layer 1 (Loader): %s\n", argv[0]);
    printf("Layer 2 (VM):     vm_%s.native\n", ARCH_NAME);
    printf("Layer 3 (Program): %s\n", program_path);
    printf("\n");

    // 执行程序
    int result = load_and_execute_program(program_path, argc - 1, argv + 1);

    printf("\n");
    if (result == 0) {
        printf("✓ 程序执行成功\n");
    } else {
        printf("✗ 程序执行失败 (返回值: %d)\n", result);
    }

    return result;
}
