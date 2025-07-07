#include "core_test_framework.h"
#include "../src/core/astc.h"
#include "../src/core/module.h"
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

// 全局测试框架状态
TestFramework g_test_framework = {0};

// 模拟函数数组
MockFunction g_mock_functions[MAX_MOCK_FUNCTIONS] = {0};
int g_mock_function_count = 0;

// 测试框架初始化
void test_framework_init(bool verbose) {
    memset(&g_test_framework, 0, sizeof(TestFramework));
    g_test_framework.verbose = verbose;
    
    // 重置模拟函数
    memset(g_mock_functions, 0, sizeof(g_mock_functions));
    g_mock_function_count = 0;
    
    printf(ANSI_COLOR_MAGENTA "=== Core Test Framework Initialized ===" ANSI_COLOR_RESET "\n");
    if (verbose) {
        printf(ANSI_COLOR_YELLOW "Verbose mode enabled" ANSI_COLOR_RESET "\n");
    }
    printf("\n");
}

// 测试框架清理
void test_framework_cleanup(void) {
    // 清理模拟函数
    memset(g_mock_functions, 0, sizeof(g_mock_functions));
    g_mock_function_count = 0;
    
    printf(ANSI_COLOR_MAGENTA "=== Core Test Framework Cleaned Up ===" ANSI_COLOR_RESET "\n");
}

// 打印测试总结
void test_framework_print_summary(void) {
    printf(ANSI_COLOR_MAGENTA "=== Test Summary ===" ANSI_COLOR_RESET "\n");
    printf("Total tests: %d\n", g_test_framework.total_tests);
    printf("Passed: " ANSI_COLOR_GREEN "%d" ANSI_COLOR_RESET "\n", g_test_framework.passed_tests);
    printf("Failed: " ANSI_COLOR_RED "%d" ANSI_COLOR_RESET "\n", g_test_framework.failed_tests);
    
    if (g_test_framework.failed_tests == 0) {
        printf(ANSI_COLOR_GREEN "All tests passed! ✓" ANSI_COLOR_RESET "\n");
    } else {
        printf(ANSI_COLOR_RED "Some tests failed! ✗" ANSI_COLOR_RESET "\n");
    }
    printf("\n");
}

// 检查所有测试是否通过
bool test_framework_all_passed(void) {
    return g_test_framework.failed_tests == 0;
}

// 测试用内存分配
void* test_malloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

// 测试用内存释放
void test_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

// 测试用字符串复制
char* test_strdup(const char* str) {
    if (!str) return NULL;
    
    size_t len = strlen(str) + 1;
    char* copy = test_malloc(len);
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}

// 检查文件是否存在
bool test_file_exists(const char* path) {
    if (!path) return false;
    
    struct stat st;
    return stat(path, &st) == 0;
}

// 创建临时文件
bool test_create_temp_file(const char* content, char* temp_path, size_t path_size) {
    if (!content || !temp_path || path_size == 0) {
        return false;
    }
    
    // 生成临时文件名
    srand(time(NULL));
    snprintf(temp_path, path_size, "/tmp/core_test_%d_%d.tmp", getpid(), rand());
    
    // 创建文件
    FILE* file = fopen(temp_path, "w");
    if (!file) {
        return false;
    }
    
    // 写入内容
    size_t written = fwrite(content, 1, strlen(content), file);
    fclose(file);
    
    return written == strlen(content);
}

// 删除临时文件
void test_remove_temp_file(const char* path) {
    if (path && test_file_exists(path)) {
        unlink(path);
    }
}

// ===============================================
// 测试辅助函数 (不重复定义已在pipeline_module.c中的函数)
// ===============================================

// 模块符号解析函数 (修复test_module_system.c中的警告)
void* module_sym(Module* module, const char* symbol_name) {
    if (!module || !symbol_name) {
        return NULL;
    }
    
    // 使用已有的module_resolve函数
    return module_resolve(module, symbol_name);
} 