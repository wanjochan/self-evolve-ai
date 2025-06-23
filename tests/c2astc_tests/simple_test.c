/**
 * simple_test.c - 简单测试程序，测试编译系统
 */

#include <stdio.h>
#include "../../c2astc.h"

int main() {
    printf("C2ASTC 简单测试\n");
    
    // 打印版本信息
    c2astc_print_version();
    
    // 获取默认选项
    C2AstcOptions options = c2astc_default_options();
    printf("默认选项:\n");
    printf("  优化级别: %s\n", options.optimize_level ? "开启" : "关闭");
    printf("  扩展支持: %s\n", options.enable_extensions ? "开启" : "关闭");
    printf("  调试信息: %s\n", options.emit_debug_info ? "开启" : "关闭");
    
    // 测试错误处理
    printf("\n错误处理测试: %s\n", c2astc_get_error() ? c2astc_get_error() : "无错误");
    
    printf("\n测试通过!\n");
    return 0;
} 