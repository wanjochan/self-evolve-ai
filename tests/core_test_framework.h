#ifndef CORE_TEST_FRAMEWORK_H
#define CORE_TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// 测试框架全局状态
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
    int current_suite_tests;
    int current_suite_passed;
    char current_suite_name[256];
    bool verbose;
} TestFramework;

extern TestFramework g_test_framework;

// 颜色输出宏
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// 测试套件宏
#define TEST_SUITE_START(name) \
    do { \
        strncpy(g_test_framework.current_suite_name, name, sizeof(g_test_framework.current_suite_name) - 1); \
        g_test_framework.current_suite_tests = 0; \
        g_test_framework.current_suite_passed = 0; \
        printf(ANSI_COLOR_CYAN "=== Test Suite: %s ===" ANSI_COLOR_RESET "\n", name); \
    } while(0)

#define TEST_SUITE_END() \
    do { \
        printf(ANSI_COLOR_CYAN "=== Suite %s: %d/%d tests passed ===" ANSI_COLOR_RESET "\n\n", \
               g_test_framework.current_suite_name, \
               g_test_framework.current_suite_passed, \
               g_test_framework.current_suite_tests); \
    } while(0)

// 测试用例宏 - 定义测试函数
#define TEST_CASE(name) bool name(void)

// 断言宏
#define ASSERT_TRUE(condition, ...) \
    do { \
        if (!(condition)) { \
            printf(ANSI_COLOR_RED "FAIL: %s:%d - Expected true, got false" ANSI_COLOR_RESET "\n", __FILE__, __LINE__); \
            g_test_framework.failed_tests++; \
            return false; \
        } \
    } while(0)

#define ASSERT_FALSE(condition, ...) \
    do { \
        if (condition) { \
            printf(ANSI_COLOR_RED "FAIL: %s:%d - Expected false, got true" ANSI_COLOR_RESET "\n", __FILE__, __LINE__); \
            g_test_framework.failed_tests++; \
            return false; \
        } \
    } while(0)

#define ASSERT_EQ(expected, actual, ...) \
    do { \
        if ((expected) != (actual)) { \
            printf(ANSI_COLOR_RED "FAIL: %s:%d - Expected %ld, got %ld" ANSI_COLOR_RESET "\n", __FILE__, __LINE__, (long)(expected), (long)(actual)); \
            g_test_framework.failed_tests++; \
            return false; \
        } \
    } while(0)

#define ASSERT_NE(expected, actual, ...) \
    do { \
        if ((expected) == (actual)) { \
            printf(ANSI_COLOR_RED "FAIL: %s:%d - Expected not equal to %ld, but got %ld" ANSI_COLOR_RESET "\n", __FILE__, __LINE__, (long)(expected), (long)(actual)); \
            g_test_framework.failed_tests++; \
            return false; \
        } \
    } while(0)

#define ASSERT_NULL(ptr, ...) \
    do { \
        if ((ptr) != NULL) { \
            printf(ANSI_COLOR_RED "FAIL: %s:%d - Expected NULL, got %p" ANSI_COLOR_RESET "\n", __FILE__, __LINE__, (void*)(ptr)); \
            g_test_framework.failed_tests++; \
            return false; \
        } \
    } while(0)

#define ASSERT_NOT_NULL(ptr, ...) \
    do { \
        if ((ptr) == NULL) { \
            printf(ANSI_COLOR_RED "FAIL: %s:%d - Expected not NULL, got NULL" ANSI_COLOR_RESET "\n", __FILE__, __LINE__); \
            g_test_framework.failed_tests++; \
            return false; \
        } \
    } while(0)

#define ASSERT_STR_EQ(expected, actual, ...) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            printf(ANSI_COLOR_RED "FAIL: %s:%d - Expected \"%s\", got \"%s\"" ANSI_COLOR_RESET "\n", __FILE__, __LINE__, (expected), (actual)); \
            g_test_framework.failed_tests++; \
            return false; \
        } \
    } while(0)

#define ASSERT_STR_NE(expected, actual, ...) \
    do { \
        if (strcmp((expected), (actual)) == 0) { \
            printf(ANSI_COLOR_RED "FAIL: %s:%d - Expected not equal to \"%s\", but got \"%s\"" ANSI_COLOR_RESET "\n", __FILE__, __LINE__, (expected), (actual)); \
            g_test_framework.failed_tests++; \
            return false; \
        } \
    } while(0)

#define ASSERT_GT(actual, expected, ...) \
    do { \
        if (!((actual) > (expected))) { \
            printf(ANSI_COLOR_RED "FAIL: %s:%d - Expected %ld > %ld" ANSI_COLOR_RESET "\n", __FILE__, __LINE__, (long)(actual), (long)(expected)); \
            g_test_framework.failed_tests++; \
            return false; \
        } \
    } while(0)

#define ASSERT_LT(actual, expected, ...) \
    do { \
        if (!((actual) < (expected))) { \
            printf(ANSI_COLOR_RED "FAIL: %s:%d - Expected %ld < %ld" ANSI_COLOR_RESET "\n", __FILE__, __LINE__, (long)(actual), (long)(expected)); \
            g_test_framework.failed_tests++; \
            return false; \
        } \
    } while(0)

#define ASSERT_GE(actual, expected, ...) \
    do { \
        if (!((actual) >= (expected))) { \
            printf(ANSI_COLOR_RED "FAIL: %s:%d - Expected %ld >= %ld" ANSI_COLOR_RESET "\n", __FILE__, __LINE__, (long)(actual), (long)(expected)); \
            g_test_framework.failed_tests++; \
            return false; \
        } \
    } while(0)

#define ASSERT_LE(actual, expected, ...) \
    do { \
        if (!((actual) <= (expected))) { \
            printf(ANSI_COLOR_RED "FAIL: %s:%d - Expected %ld <= %ld" ANSI_COLOR_RESET "\n", __FILE__, __LINE__, (long)(actual), (long)(expected)); \
            g_test_framework.failed_tests++; \
            return false; \
        } \
    } while(0)

// 测试成功宏
#define TEST_PASS() \
    do { \
        g_test_framework.passed_tests++; \
        g_test_framework.current_suite_passed++; \
        if (g_test_framework.verbose) { \
            printf(ANSI_COLOR_GREEN "PASS" ANSI_COLOR_RESET "\n"); \
        } \
        return true; \
    } while(0)

// 运行测试宏
#define RUN_TEST(test_func) \
    do { \
        g_test_framework.total_tests++; \
        g_test_framework.current_suite_tests++; \
        if (g_test_framework.verbose) { \
            printf(ANSI_COLOR_BLUE "Running test: %s" ANSI_COLOR_RESET "\n", #test_func); \
        } \
        if (test_func()) { \
            printf(ANSI_COLOR_GREEN "✓ " #test_func ANSI_COLOR_RESET "\n"); \
        } else { \
            printf(ANSI_COLOR_RED "✗ " #test_func ANSI_COLOR_RESET "\n"); \
        } \
    } while(0)

// 测试框架函数声明
void test_framework_init(bool verbose);
void test_framework_cleanup(void);
void test_framework_print_summary(void);
bool test_framework_all_passed(void);

// 测试工具函数
void* test_malloc(size_t size);
void test_free(void* ptr);
char* test_strdup(const char* str);
bool test_file_exists(const char* path);
bool test_create_temp_file(const char* content, char* temp_path, size_t path_size);
void test_remove_temp_file(const char* path);

// 模拟和桩函数支持
typedef struct {
    const char* function_name;
    void* original_func;
    void* mock_func;
    int call_count;
    bool is_mocked;
} MockFunction;

#define MAX_MOCK_FUNCTIONS 64
extern MockFunction g_mock_functions[MAX_MOCK_FUNCTIONS];
extern int g_mock_function_count;

// 模拟函数宏
#define MOCK_FUNCTION(func_name, mock_impl) \
    do { \
        if (g_mock_function_count < MAX_MOCK_FUNCTIONS) { \
            g_mock_functions[g_mock_function_count].function_name = #func_name; \
            g_mock_functions[g_mock_function_count].mock_func = (void*)mock_impl; \
            g_mock_functions[g_mock_function_count].call_count = 0; \
            g_mock_functions[g_mock_function_count].is_mocked = true; \
            g_mock_function_count++; \
        } \
    } while(0)

#define RESET_MOCKS() \
    do { \
        for (int i = 0; i < g_mock_function_count; i++) { \
            g_mock_functions[i].call_count = 0; \
            g_mock_functions[i].is_mocked = false; \
        } \
        g_mock_function_count = 0; \
    } while(0)

#define GET_MOCK_CALL_COUNT(func_name) \
    ({ \
        int count = 0; \
        for (int i = 0; i < g_mock_function_count; i++) { \
            if (strcmp(g_mock_functions[i].function_name, #func_name) == 0) { \
                count = g_mock_functions[i].call_count; \
                break; \
            } \
        } \
        count; \
    })

#endif // CORE_TEST_FRAMEWORK_H 