#include "core_test_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 声明各个测试模块的运行函数
void run_astc_module_tests(void);
void run_module_system_tests(void);
void run_specific_modules_tests(void);

// 打印使用说明
void print_usage(const char* program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -v, --verbose    Enable verbose output\n");
    printf("  -h, --help       Show this help message\n");
    printf("  --astc           Run only ASTC module tests\n");
    printf("  --module         Run only module system tests\n");
    printf("  --specific       Run only specific modules tests\n");
    printf("  --all            Run all tests (default)\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s                    # Run all tests\n", program_name);
    printf("  %s -v                 # Run all tests with verbose output\n", program_name);
    printf("  %s --astc             # Run only ASTC module tests\n", program_name);
    printf("  %s --module -v        # Run only module system tests with verbose output\n", program_name);
}

// 解析命令行参数
typedef struct {
    bool verbose;
    bool help;
    bool run_astc;
    bool run_module;
    bool run_specific;
    bool run_all;
} TestOptions;

TestOptions parse_arguments(int argc, char* argv[]) {
    TestOptions options = {0};
    options.run_all = true;  // 默认运行所有测试
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            options.verbose = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            options.help = true;
        } else if (strcmp(argv[i], "--astc") == 0) {
            options.run_astc = true;
            options.run_all = false;
        } else if (strcmp(argv[i], "--module") == 0) {
            options.run_module = true;
            options.run_all = false;
        } else if (strcmp(argv[i], "--specific") == 0) {
            options.run_specific = true;
            options.run_all = false;
        } else if (strcmp(argv[i], "--all") == 0) {
            options.run_all = true;
        } else {
            printf("Unknown option: %s\n", argv[i]);
            options.help = true;
        }
    }
    
    return options;
}

// 运行测试套件
int run_test_suites(TestOptions options) {
    printf("Starting Core Module Tests...\n");
    printf("========================================\n");
    
    // 运行ASTC模块测试
    if (options.run_all || options.run_astc) {
        run_astc_module_tests();
    }
    
    // 运行模块系统测试
    if (options.run_all || options.run_module) {
        run_module_system_tests();
    }
    
    // 运行具体模块测试
    if (options.run_all || options.run_specific) {
        run_specific_modules_tests();
    }
    
    return 0;
}

// 主函数
int main(int argc, char* argv[]) {
    // 解析命令行参数
    TestOptions options = parse_arguments(argc, argv);
    
    // 显示帮助信息
    if (options.help) {
        print_usage(argv[0]);
        return 0;
    }
    
    // 初始化测试框架
    test_framework_init(options.verbose);
    
    // 运行测试
    int result = run_test_suites(options);
    
    // 打印测试总结
    test_framework_print_summary();
    
    // 清理测试框架
    test_framework_cleanup();
    
    // 返回测试结果
    return test_framework_all_passed() ? 0 : 1;
} 