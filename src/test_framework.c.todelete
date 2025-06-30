/**
 * test_framework.c - 自动化测试框架实现
 */

#include "test_framework.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ===============================================
// 全局变量
// ===============================================

static uint32_t g_timer_start = 0;
static FILE* g_current_log = NULL;

// ===============================================
// 时间相关函数
// ===============================================

uint32_t test_framework_get_time_ms(void) {
    return (uint32_t)(clock() * 1000 / CLOCKS_PER_SEC);
}

void test_framework_start_timer(void) {
    g_timer_start = test_framework_get_time_ms();
}

uint32_t test_framework_stop_timer(void) {
    return test_framework_get_time_ms() - g_timer_start;
}

// ===============================================
// 日志记录函数
// ===============================================

void test_framework_log_failure(const char* file, int line, const char* message) {
    printf("FAIL: %s:%d - %s\n", file, line, message);
    if (g_current_log) {
        fprintf(g_current_log, "FAIL: %s:%d - %s\n", file, line, message);
    }
}

void test_framework_log_failure_with_values(const char* file, int line, 
                                           const char* format, int expected, int actual) {
    printf("FAIL: %s:%d - ", file, line);
    printf(format, expected, actual);
    printf("\n");
    
    if (g_current_log) {
        fprintf(g_current_log, "FAIL: %s:%d - ", file, line);
        fprintf(g_current_log, format, expected, actual);
        fprintf(g_current_log, "\n");
    }
}

void test_framework_log_failure_with_strings(const char* file, int line,
                                            const char* format, 
                                            const char* expected, 
                                            const char* actual) {
    printf("FAIL: %s:%d - ", file, line);
    printf(format, expected, actual);
    printf("\n");
    
    if (g_current_log) {
        fprintf(g_current_log, "FAIL: %s:%d - ", file, line);
        fprintf(g_current_log, format, expected, actual);
        fprintf(g_current_log, "\n");
    }
}

// ===============================================
// 测试框架初始化
// ===============================================

TestFramework* test_framework_init(void) {
    TestFramework* framework = calloc(1, sizeof(TestFramework));
    if (!framework) return NULL;
    
    framework->suite_capacity = 10;
    framework->test_suites = calloc(framework->suite_capacity, sizeof(TestSuite));
    if (!framework->test_suites) {
        free(framework);
        return NULL;
    }
    
    // 设置默认配置
    framework->verbose_output = true;
    framework->stop_on_first_failure = false;
    framework->parallel_execution = false;
    framework->timeout_ms = 30000; // 30秒超时
    
    framework->category_filter = 0; // 所有类别
    framework->min_priority = TEST_PRIORITY_LOW;
    strcpy(framework->name_filter, "");
    
    framework->generate_xml_report = true;
    framework->generate_html_report = false;
    framework->generate_json_report = false;
    strcpy(framework->report_directory, "tests/reports/");
    
    // 打开日志文件
    framework->log_file = fopen("tests/test_framework.log", "w");
    g_current_log = framework->log_file;
    
    printf("Test framework initialized\n");
    return framework;
}

void test_framework_free(TestFramework* framework) {
    if (!framework) return;
    
    // 释放所有测试套件
    for (uint32_t i = 0; i < framework->suite_count; i++) {
        TestSuite* suite = &framework->test_suites[i];
        if (suite->test_cases) {
            for (uint32_t j = 0; j < suite->test_count; j++) {
                TestCase* test = &suite->test_cases[j];
                if (test->dependencies) {
                    free(test->dependencies);
                }
            }
            free(suite->test_cases);
        }
    }
    
    free(framework->test_suites);
    
    if (framework->log_file) {
        fclose(framework->log_file);
    }
    
    free(framework);
    g_current_log = NULL;
}

// ===============================================
// 测试套件和用例管理
// ===============================================

TestSuite* test_framework_create_suite(TestFramework* framework, 
                                      const char* name, 
                                      const char* description) {
    if (!framework || !name) return NULL;
    
    // 扩容检查
    if (framework->suite_count >= framework->suite_capacity) {
        framework->suite_capacity *= 2;
        TestSuite* new_suites = realloc(framework->test_suites, 
                                       framework->suite_capacity * sizeof(TestSuite));
        if (!new_suites) return NULL;
        framework->test_suites = new_suites;
    }
    
    TestSuite* suite = &framework->test_suites[framework->suite_count];
    memset(suite, 0, sizeof(TestSuite));
    
    suite->id = framework->suite_count;
    strncpy(suite->name, name, MAX_TEST_NAME_LEN - 1);
    if (description) {
        strncpy(suite->description, description, MAX_TEST_DESCRIPTION_LEN - 1);
    }
    
    suite->capacity = 10;
    suite->test_cases = calloc(suite->capacity, sizeof(TestCase));
    if (!suite->test_cases) return NULL;
    
    suite->enabled = true;
    
    framework->suite_count++;
    
    printf("Created test suite: %s\n", name);
    return suite;
}

TestCase* test_framework_add_test(TestSuite* suite,
                                 const char* name,
                                 const char* description,
                                 int (*test_function)(void),
                                 TestCategory category,
                                 TestPriority priority) {
    if (!suite || !name || !test_function) return NULL;
    
    // 扩容检查
    if (suite->test_count >= suite->capacity) {
        suite->capacity *= 2;
        TestCase* new_tests = realloc(suite->test_cases, 
                                     suite->capacity * sizeof(TestCase));
        if (!new_tests) return NULL;
        suite->test_cases = new_tests;
    }
    
    TestCase* test = &suite->test_cases[suite->test_count];
    memset(test, 0, sizeof(TestCase));
    
    test->id = suite->test_count;
    strncpy(test->name, name, MAX_TEST_NAME_LEN - 1);
    if (description) {
        strncpy(test->description, description, MAX_TEST_DESCRIPTION_LEN - 1);
    }
    
    test->test_function = test_function;
    test->category = category;
    test->priority = priority;
    test->status = TEST_STATUS_NOT_RUN;
    test->enabled = true;
    test->automated = true;
    
    suite->test_count++;
    suite->total_tests++;
    
    printf("Added test: %s to suite %s\n", name, suite->name);
    return test;
}

// ===============================================
// 测试执行
// ===============================================

static int run_single_test(TestCase* test) {
    if (!test || !test->enabled || !test->test_function) {
        return -1;
    }
    
    printf("Running test: %s... ", test->name);
    
    test->status = TEST_STATUS_RUNNING;
    test->run_count++;
    
    // 执行setup
    if (test->setup_function) {
        test->setup_function();
    }
    
    // 开始计时
    test_framework_start_timer();
    
    // 执行测试
    int result = test->test_function();
    
    // 停止计时
    test->execution_time_ms = test_framework_stop_timer();
    
    // 执行teardown
    if (test->teardown_function) {
        test->teardown_function();
    }
    
    // 更新状态
    if (result == 0) {
        test->status = TEST_STATUS_PASSED;
        test->pass_count++;
        printf("PASSED (%u ms)\n", test->execution_time_ms);
    } else {
        test->status = TEST_STATUS_FAILED;
        test->fail_count++;
        printf("FAILED (%u ms)\n", test->execution_time_ms);
    }
    
    return result;
}

int test_framework_run_suite(TestFramework* framework, const char* suite_name) {
    if (!framework || !suite_name) return -1;
    
    // 查找测试套件
    TestSuite* suite = NULL;
    for (uint32_t i = 0; i < framework->suite_count; i++) {
        if (strcmp(framework->test_suites[i].name, suite_name) == 0) {
            suite = &framework->test_suites[i];
            break;
        }
    }
    
    if (!suite) {
        printf("Test suite not found: %s\n", suite_name);
        return -1;
    }
    
    printf("Running test suite: %s\n", suite->name);
    
    // 执行套件setup
    if (suite->suite_setup) {
        suite->suite_setup();
    }
    
    // 开始计时
    test_framework_start_timer();
    
    int failed_tests = 0;
    
    // 运行所有测试
    for (uint32_t i = 0; i < suite->test_count; i++) {
        TestCase* test = &suite->test_cases[i];
        
        if (run_single_test(test) != 0) {
            failed_tests++;
            if (framework->stop_on_first_failure) {
                break;
            }
        }
    }
    
    // 停止计时
    suite->execution_time_ms = test_framework_stop_timer();
    
    // 执行套件teardown
    if (suite->suite_teardown) {
        suite->suite_teardown();
    }
    
    // 更新统计
    suite->passed_tests = suite->test_count - failed_tests;
    suite->failed_tests = failed_tests;
    
    printf("Suite %s completed: %u/%u tests passed\n", 
           suite->name, suite->passed_tests, suite->test_count);
    
    return failed_tests;
}

int test_framework_run_all(TestFramework* framework) {
    if (!framework) return -1;
    
    printf("=== Running All Test Suites ===\n");
    
    test_framework_start_timer();
    
    int total_failed = 0;
    
    for (uint32_t i = 0; i < framework->suite_count; i++) {
        TestSuite* suite = &framework->test_suites[i];
        if (suite->enabled) {
            int failed = test_framework_run_suite(framework, suite->name);
            if (failed > 0) {
                total_failed += failed;
            }
        }
    }
    
    framework->total_execution_time_ms = test_framework_stop_timer();
    
    // 计算总体统计
    framework->total_tests_run = 0;
    framework->total_tests_passed = 0;
    framework->total_tests_failed = 0;
    
    for (uint32_t i = 0; i < framework->suite_count; i++) {
        TestSuite* suite = &framework->test_suites[i];
        framework->total_tests_run += suite->test_count;
        framework->total_tests_passed += suite->passed_tests;
        framework->total_tests_failed += suite->failed_tests;
    }
    
    printf("\n=== Test Summary ===\n");
    printf("Total tests: %u\n", framework->total_tests_run);
    printf("Passed: %u\n", framework->total_tests_passed);
    printf("Failed: %u\n", framework->total_tests_failed);
    printf("Success rate: %.1f%%\n", 
           (float)framework->total_tests_passed * 100.0f / framework->total_tests_run);
    printf("Total time: %u ms\n", framework->total_execution_time_ms);
    
    return total_failed;
}

// ===============================================
// 报告生成
// ===============================================

int test_framework_generate_report(TestFramework* framework) {
    if (!framework) return -1;

    printf("\n=== Generating Test Report ===\n");

    // 创建报告目录
    system("mkdir tests\\reports 2>nul");

    // 生成XML报告
    if (framework->generate_xml_report) {
        FILE* xml_file = fopen("tests/reports/test_report.xml", "w");
        if (xml_file) {
            fprintf(xml_file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
            fprintf(xml_file, "<testsuites>\n");

            for (uint32_t i = 0; i < framework->suite_count; i++) {
                TestSuite* suite = &framework->test_suites[i];
                fprintf(xml_file, "  <testsuite name=\"%s\" tests=\"%u\" failures=\"%u\" time=\"%u\">\n",
                       suite->name, suite->test_count, suite->failed_tests, suite->execution_time_ms);

                for (uint32_t j = 0; j < suite->test_count; j++) {
                    TestCase* test = &suite->test_cases[j];
                    fprintf(xml_file, "    <testcase name=\"%s\" time=\"%u\"",
                           test->name, test->execution_time_ms);

                    if (test->status == TEST_STATUS_FAILED) {
                        fprintf(xml_file, ">\n      <failure>%s</failure>\n    </testcase>\n",
                               test->error_message);
                    } else {
                        fprintf(xml_file, "/>\n");
                    }
                }

                fprintf(xml_file, "  </testsuite>\n");
            }

            fprintf(xml_file, "</testsuites>\n");
            fclose(xml_file);
            printf("XML report generated: tests/reports/test_report.xml\n");
        }
    }

    // 生成简单的文本报告
    FILE* txt_file = fopen("tests/reports/test_report.txt", "w");
    if (txt_file) {
        fprintf(txt_file, "=== Test Framework Report ===\n");
        fprintf(txt_file, "Generated: %s\n", __DATE__ " " __TIME__);
        fprintf(txt_file, "\nSummary:\n");
        fprintf(txt_file, "Total tests: %u\n", framework->total_tests_run);
        fprintf(txt_file, "Passed: %u\n", framework->total_tests_passed);
        fprintf(txt_file, "Failed: %u\n", framework->total_tests_failed);
        fprintf(txt_file, "Success rate: %.1f%%\n",
               (float)framework->total_tests_passed * 100.0f / framework->total_tests_run);
        fprintf(txt_file, "Total time: %u ms\n", framework->total_execution_time_ms);

        fprintf(txt_file, "\nDetailed Results:\n");
        for (uint32_t i = 0; i < framework->suite_count; i++) {
            TestSuite* suite = &framework->test_suites[i];
            fprintf(txt_file, "\nSuite: %s\n", suite->name);

            for (uint32_t j = 0; j < suite->test_count; j++) {
                TestCase* test = &suite->test_cases[j];
                fprintf(txt_file, "  %s: %s (%u ms)\n",
                       test->name,
                       test->status == TEST_STATUS_PASSED ? "PASSED" : "FAILED",
                       test->execution_time_ms);
            }
        }

        fclose(txt_file);
        printf("Text report generated: tests/reports/test_report.txt\n");
    }

    return 0;
}
