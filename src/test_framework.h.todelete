/**
 * test_framework.h - 自动化测试框架
 * 
 * 支持AI进化过程中的自动验证和回归测试
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// 测试框架常量
// ===============================================

#define MAX_TEST_CASES 1000
#define MAX_TEST_NAME_LEN 128
#define MAX_TEST_DESCRIPTION_LEN 256
#define MAX_ERROR_MESSAGE_LEN 512
#define MAX_TEST_SUITES 100

// ===============================================
// 测试状态和结果
// ===============================================

typedef enum {
    TEST_STATUS_NOT_RUN = 0,
    TEST_STATUS_RUNNING,
    TEST_STATUS_PASSED,
    TEST_STATUS_FAILED,
    TEST_STATUS_SKIPPED,
    TEST_STATUS_ERROR
} TestStatus;

typedef enum {
    TEST_PRIORITY_LOW = 1,
    TEST_PRIORITY_NORMAL = 2,
    TEST_PRIORITY_HIGH = 3,
    TEST_PRIORITY_CRITICAL = 4
} TestPriority;

typedef enum {
    TEST_CATEGORY_UNIT = 1,
    TEST_CATEGORY_INTEGRATION = 2,
    TEST_CATEGORY_SYSTEM = 3,
    TEST_CATEGORY_REGRESSION = 4,
    TEST_CATEGORY_PERFORMANCE = 5,
    TEST_CATEGORY_EVOLUTION = 6
} TestCategory;

// ===============================================
// 测试用例结构
// ===============================================

typedef struct TestCase {
    uint32_t id;
    char name[MAX_TEST_NAME_LEN];
    char description[MAX_TEST_DESCRIPTION_LEN];
    TestCategory category;
    TestPriority priority;
    
    // 测试函数指针
    int (*test_function)(void);
    void (*setup_function)(void);
    void (*teardown_function)(void);
    
    // 测试结果
    TestStatus status;
    uint32_t execution_time_ms;
    char error_message[MAX_ERROR_MESSAGE_LEN];
    
    // 统计信息
    uint32_t run_count;
    uint32_t pass_count;
    uint32_t fail_count;
    
    // 依赖关系
    uint32_t* dependencies;
    uint32_t dependency_count;
    
    bool enabled;
    bool automated;
} TestCase;

typedef struct TestSuite {
    uint32_t id;
    char name[MAX_TEST_NAME_LEN];
    char description[MAX_TEST_DESCRIPTION_LEN];
    
    TestCase* test_cases;
    uint32_t test_count;
    uint32_t capacity;
    
    // 套件级别的setup/teardown
    void (*suite_setup)(void);
    void (*suite_teardown)(void);
    
    // 统计信息
    uint32_t total_tests;
    uint32_t passed_tests;
    uint32_t failed_tests;
    uint32_t skipped_tests;
    uint32_t execution_time_ms;
    
    bool enabled;
} TestSuite;

// ===============================================
// 测试框架主结构
// ===============================================

typedef struct TestFramework {
    TestSuite* test_suites;
    uint32_t suite_count;
    uint32_t suite_capacity;
    
    // 全局配置
    bool verbose_output;
    bool stop_on_first_failure;
    bool parallel_execution;
    uint32_t timeout_ms;
    
    // 过滤器
    TestCategory category_filter;
    TestPriority min_priority;
    char name_filter[MAX_TEST_NAME_LEN];
    
    // 全局统计
    uint32_t total_tests_run;
    uint32_t total_tests_passed;
    uint32_t total_tests_failed;
    uint32_t total_execution_time_ms;
    
    // 报告配置
    bool generate_xml_report;
    bool generate_html_report;
    bool generate_json_report;
    char report_directory[256];
    
    FILE* log_file;
} TestFramework;

// ===============================================
// 测试断言宏
// ===============================================

#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            test_framework_log_failure(__FILE__, __LINE__, #condition); \
            return -1; \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            test_framework_log_failure_with_values(__FILE__, __LINE__, \
                "Expected: %d, Actual: %d", (int)(expected), (int)(actual)); \
            return -1; \
        } \
    } while(0)

#define TEST_ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            test_framework_log_failure(__FILE__, __LINE__, #ptr " is NULL"); \
            return -1; \
        } \
    } while(0)

#define TEST_ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            test_framework_log_failure(__FILE__, __LINE__, #ptr " is not NULL"); \
            return -1; \
        } \
    } while(0)

#define TEST_ASSERT_STRING_EQUAL(expected, actual) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            test_framework_log_failure_with_strings(__FILE__, __LINE__, \
                "Expected: \"%s\", Actual: \"%s\"", (expected), (actual)); \
            return -1; \
        } \
    } while(0)

// ===============================================
// 函数声明
// ===============================================

/**
 * 初始化测试框架
 */
TestFramework* test_framework_init(void);

/**
 * 释放测试框架
 */
void test_framework_free(TestFramework* framework);

/**
 * 创建测试套件
 */
TestSuite* test_framework_create_suite(TestFramework* framework, 
                                      const char* name, 
                                      const char* description);

/**
 * 添加测试用例
 */
TestCase* test_framework_add_test(TestSuite* suite,
                                 const char* name,
                                 const char* description,
                                 int (*test_function)(void),
                                 TestCategory category,
                                 TestPriority priority);

/**
 * 运行所有测试
 */
int test_framework_run_all(TestFramework* framework);

/**
 * 运行指定套件
 */
int test_framework_run_suite(TestFramework* framework, const char* suite_name);

/**
 * 运行单个测试
 */
int test_framework_run_test(TestFramework* framework, 
                           const char* suite_name, 
                           const char* test_name);

/**
 * 生成测试报告
 */
int test_framework_generate_report(TestFramework* framework);

/**
 * 设置过滤器
 */
void test_framework_set_filter(TestFramework* framework,
                              TestCategory category,
                              TestPriority min_priority,
                              const char* name_pattern);

/**
 * 日志记录函数
 */
void test_framework_log_failure(const char* file, int line, const char* message);
void test_framework_log_failure_with_values(const char* file, int line, 
                                           const char* format, int expected, int actual);
void test_framework_log_failure_with_strings(const char* file, int line,
                                            const char* format, 
                                            const char* expected, 
                                            const char* actual);

/**
 * 性能测试支持
 */
uint32_t test_framework_get_time_ms(void);
void test_framework_start_timer(void);
uint32_t test_framework_stop_timer(void);

/**
 * 自动化测试发现
 */
int test_framework_discover_tests(TestFramework* framework, const char* directory);

/**
 * 回归测试支持
 */
int test_framework_run_regression_tests(TestFramework* framework);

/**
 * AI进化测试支持
 */
int test_framework_validate_evolution(TestFramework* framework,
                                     const char* old_version,
                                     const char* new_version);

/**
 * 编译器测试专用函数
 */
int test_framework_test_compiler(const char* source_file, const char* expected_output);
int test_framework_test_runtime(const char* astc_file, const char* expected_output);
int test_framework_test_self_compilation(const char* compiler_source);

#ifdef __cplusplus
}
#endif

#endif // TEST_FRAMEWORK_H
