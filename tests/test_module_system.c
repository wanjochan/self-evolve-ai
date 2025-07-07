#include "core_test_framework.h"
#include "../src/core/module.h"
#include <dlfcn.h>

// 测试模块状态枚举
TEST_CASE(test_module_state) {
    // 测试模块状态枚举值
    ASSERT_EQ(0, MODULE_UNLOADED);
    ASSERT_EQ(1, MODULE_LOADING);
    ASSERT_EQ(2, MODULE_READY);
    ASSERT_EQ(3, MODULE_ERROR);
    
    TEST_PASS();
}

// 测试模块结构体
TEST_CASE(test_module_struct) {
    // 创建模块结构体
    Module module = {0};
    
    // 设置基本属性
    module.name = "test_module";
    module.path = "/test/path";
    module.state = MODULE_READY;
    module.error = NULL;
    
    // 验证设置
    ASSERT_STR_EQ("test_module", module.name);
    ASSERT_STR_EQ("/test/path", module.path);
    ASSERT_EQ(MODULE_READY, module.state);
    ASSERT_NULL(module.error);
    
    TEST_PASS();
}

// 测试智能路径解析
TEST_CASE(test_resolve_native_file) {
    // 测试基本路径解析
    char* resolved = resolve_native_file("./test_module");
    ASSERT_NOT_NULL(resolved);
    
    // 检查是否包含架构后缀
    // 注意：实际的架构后缀取决于当前系统
    ASSERT_TRUE(strstr(resolved, "test_module") != NULL);
    ASSERT_TRUE(strstr(resolved, ".native") != NULL);
    
    // 清理
    free(resolved);
    
    // 测试包路径
    char* resolved_pkg = resolve_native_file("pipeline/frontend");
    ASSERT_NOT_NULL(resolved_pkg);
    ASSERT_TRUE(strstr(resolved_pkg, "pipeline/frontend") != NULL);
    ASSERT_TRUE(strstr(resolved_pkg, ".native") != NULL);
    
    // 清理
    free(resolved_pkg);
    
    TEST_PASS();
}

// 测试模块系统初始化
TEST_CASE(test_module_system_init) {
    // 测试模块系统初始化
    int result = module_system_init();
    ASSERT_EQ(0, result);
    
    // 清理
    module_system_cleanup();
    
    TEST_PASS();
}

// 测试模块系统清理
TEST_CASE(test_module_system_cleanup) {
    // 初始化后清理
    module_system_init();
    module_system_cleanup();  // 应该不会崩溃
    
    TEST_PASS();
}

// 测试模块加载（基本功能）
TEST_CASE(test_module_load_basic) {
    // 初始化模块系统
    module_system_init();
    
    // 测试加载不存在的模块
    Module* module = module_load("nonexistent_module");
    ASSERT_NULL(module);
    
    // 清理
    module_system_cleanup();
    
    TEST_PASS();
}

// 测试模块获取
TEST_CASE(test_module_get) {
    // 初始化模块系统
    module_system_init();
    
    // 测试获取不存在的模块
    Module* module = module_get("nonexistent_module");
    ASSERT_NULL(module);
    
    // 清理
    module_system_cleanup();
    
    TEST_PASS();
}

// 测试模块符号解析（基本功能）
TEST_CASE(test_module_resolve_basic) {
    // 测试NULL模块
    void* symbol = module_resolve(NULL, "test_symbol");
    ASSERT_NULL(symbol);
    
    // 创建模拟模块
    Module mock_module = {0};
    mock_module.name = "mock_module";
    mock_module.state = MODULE_READY;
    
    // 测试无resolve函数的模块
    void* symbol2 = module_resolve(&mock_module, "test_symbol");
    ASSERT_NULL(symbol2);
    
    TEST_PASS();
}

// 测试模块卸载
TEST_CASE(test_module_unload) {
    // 测试卸载NULL模块
    module_unload(NULL);  // 应该不会崩溃
    
    // 创建模拟模块 - 使用动态分配的字符串
    Module* mock_module = malloc(sizeof(Module));
    memset(mock_module, 0, sizeof(Module));
    mock_module->name = strdup("mock_module");
    mock_module->path = strdup("mock_module");
    mock_module->state = MODULE_READY;
    
    // 测试卸载模块
    module_unload(mock_module);  // 应该不会崩溃
    
    TEST_PASS();
}

// 测试按需加载模块（基本功能）
TEST_CASE(test_load_module_basic) {
    // 初始化模块系统
    module_system_init();
    
    // 测试加载不存在的模块
    Module* module = load_module("./nonexistent");
    ASSERT_NULL(module);
    
    // 清理
    module_system_cleanup();
    
    TEST_PASS();
}

// 模拟resolve函数用于测试
void* mock_resolve_function(const char* symbol) {
    if (strcmp(symbol, "test_function") == 0) {
        return (void*)0x12345678;  // 返回模拟地址
    }
    return NULL;
}

// 测试模块符号解析功能
TEST_CASE(test_module_symbol_resolution) {
    // 创建模拟模块
    Module mock_module = {0};
    mock_module.name = "mock_module";
    mock_module.state = MODULE_READY;
    mock_module.resolve = mock_resolve_function;
    
    // 测试符号解析
    void* symbol = module_resolve(&mock_module, "test_function");
    ASSERT_NOT_NULL(symbol);
    ASSERT_EQ((void*)0x12345678, symbol);
    
    // 测试不存在的符号
    void* null_symbol = module_resolve(&mock_module, "nonexistent");
    ASSERT_NULL(null_symbol);
    
    TEST_PASS();
}

// 测试模块符号接口
TEST_CASE(test_module_sym_interface) {
    // 创建模拟模块
    Module mock_module = {0};
    mock_module.name = "mock_module";
    mock_module.state = MODULE_READY;
    mock_module.resolve = mock_resolve_function;
    
    // 测试sym函数
    void* symbol = module_sym(&mock_module, "test_function");
    ASSERT_NOT_NULL(symbol);
    ASSERT_EQ((void*)0x12345678, symbol);
    
    // 测试不存在的符号
    void* null_symbol = module_sym(&mock_module, "nonexistent");
    ASSERT_NULL(null_symbol);
    
    TEST_PASS();
}

// 模拟初始化和清理函数
int mock_init_function(void) {
    return 0;  // 成功
}

void mock_cleanup_function(void) {
    // 清理操作
}

// 测试模块生命周期
TEST_CASE(test_module_lifecycle) {
    // 创建模拟模块
    Module mock_module = {0};
    mock_module.name = "mock_module";
    mock_module.state = MODULE_READY;
    mock_module.init = mock_init_function;
    mock_module.cleanup = mock_cleanup_function;
    
    // 测试初始化
    if (mock_module.init) {
        int result = mock_module.init();
        ASSERT_EQ(0, result);
    }
    
    // 测试清理
    if (mock_module.cleanup) {
        mock_module.cleanup();  // 应该不会崩溃
    }
    
    TEST_PASS();
}

// 测试模块错误处理
TEST_CASE(test_module_error_handling) {
    // 创建有错误的模块
    Module error_module = {0};
    error_module.name = "error_module";
    error_module.state = MODULE_ERROR;
    error_module.error = "Test error message";
    
    // 验证错误状态
    ASSERT_EQ(MODULE_ERROR, error_module.state);
    ASSERT_STR_EQ("Test error message", error_module.error);
    
    // 测试错误模块的符号解析
    void* symbol = module_resolve(&error_module, "test_symbol");
    ASSERT_NULL(symbol);  // 错误模块应该无法解析符号
    
    TEST_PASS();
}

// 测试模块路径处理
TEST_CASE(test_module_path_handling) {
    // 测试相对路径
    char* resolved1 = resolve_native_file("./module");
    ASSERT_NOT_NULL(resolved1);
    ASSERT_TRUE(strstr(resolved1, "module") != NULL);
    free(resolved1);
    
    // 测试绝对路径（如果支持）
    char* resolved2 = resolve_native_file("/absolute/path/module");
    ASSERT_NOT_NULL(resolved2);
    ASSERT_TRUE(strstr(resolved2, "module") != NULL);
    free(resolved2);
    
    TEST_PASS();
}

// 运行所有模块系统测试
void run_module_system_tests(void) {
    TEST_SUITE_START("Module System Tests");
    
    RUN_TEST(test_module_state);
    RUN_TEST(test_module_struct);
    RUN_TEST(test_resolve_native_file);
    RUN_TEST(test_module_system_init);
    RUN_TEST(test_module_system_cleanup);
    RUN_TEST(test_module_load_basic);
    RUN_TEST(test_module_get);
    RUN_TEST(test_module_resolve_basic);
    RUN_TEST(test_module_unload);
    RUN_TEST(test_load_module_basic);
    RUN_TEST(test_module_symbol_resolution);
    RUN_TEST(test_module_sym_interface);
    RUN_TEST(test_module_lifecycle);
    RUN_TEST(test_module_error_handling);
    RUN_TEST(test_module_path_handling);
    
    TEST_SUITE_END();
} 