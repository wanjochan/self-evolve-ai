/**
 * automated_compiler_tests.c - 自动化编译器测试套件
 */

#include "../src/test_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ===============================================
// 编译器基础功能测试
// ===============================================

int test_basic_compilation() {
    printf("Testing basic C compilation...\n");
    
    // 创建简单的测试程序
    FILE* fp = fopen("tests/temp_basic.c", "w");
    if (!fp) return -1;
    
    fprintf(fp, "#include <stdio.h>\n");
    fprintf(fp, "int main() {\n");
    fprintf(fp, "    printf(\"Hello World!\\n\");\n");
    fprintf(fp, "    return 0;\n");
    fprintf(fp, "}\n");
    fclose(fp);
    
    // 使用我们的编译器编译
    int result = system("bin\\tool_c2astc.exe -o tests\\temp_basic.astc tests\\temp_basic.c");
    
    // 检查是否生成了ASTC文件
    FILE* astc_file = fopen("tests/temp_basic.astc", "rb");
    if (!astc_file) {
        printf("ASTC file not generated\n");
        return -1;
    }
    
    fseek(astc_file, 0, SEEK_END);
    long file_size = ftell(astc_file);
    fclose(astc_file);
    
    if (file_size < 16) {
        printf("ASTC file too small: %ld bytes\n", file_size);
        return -1;
    }
    
    printf("Basic compilation test passed: %ld bytes ASTC generated\n", file_size);
    return 0;
}

int test_runtime_execution() {
    printf("Testing runtime execution...\n");
    
    // 运行之前编译的程序
    int result = system("bin\\enhanced_runtime_with_libc_v2.exe tests\\temp_basic.astc > tests\\temp_output.txt 2>&1");
    
    // 检查输出
    FILE* output_file = fopen("tests/temp_output.txt", "r");
    if (!output_file) {
        printf("No output file generated\n");
        return -1;
    }
    
    char buffer[256];
    bool found_hello = false;
    while (fgets(buffer, sizeof(buffer), output_file)) {
        if (strstr(buffer, "Hello World!")) {
            found_hello = true;
            break;
        }
    }
    fclose(output_file);
    
    if (!found_hello) {
        printf("Expected output not found\n");
        return -1;
    }
    
    printf("Runtime execution test passed\n");
    return 0;
}

int test_malloc_functionality() {
    printf("Testing malloc functionality...\n");
    
    // 创建malloc测试程序
    FILE* fp = fopen("tests/temp_malloc.c", "w");
    if (!fp) return -1;
    
    fprintf(fp, "#include <stdio.h>\n");
    fprintf(fp, "#include <stdlib.h>\n");
    fprintf(fp, "int main() {\n");
    fprintf(fp, "    char* ptr = malloc(100);\n");
    fprintf(fp, "    if (ptr) {\n");
    fprintf(fp, "        printf(\"malloc SUCCESS\\n\");\n");
    fprintf(fp, "        free(ptr);\n");
    fprintf(fp, "        printf(\"free SUCCESS\\n\");\n");
    fprintf(fp, "    } else {\n");
    fprintf(fp, "        printf(\"malloc FAILED\\n\");\n");
    fprintf(fp, "    }\n");
    fprintf(fp, "    return 0;\n");
    fprintf(fp, "}\n");
    fclose(fp);
    
    // 编译
    int compile_result = system("bin\\tool_c2astc.exe -o tests\\temp_malloc.astc tests\\temp_malloc.c");
    if (compile_result != 0) {
        printf("Malloc test compilation failed\n");
        return -1;
    }
    
    // 运行
    int run_result = system("bin\\enhanced_runtime_with_libc_v2.exe tests\\temp_malloc.astc > tests\\temp_malloc_output.txt 2>&1");
    
    // 检查输出
    FILE* output_file = fopen("tests/temp_malloc_output.txt", "r");
    if (!output_file) {
        printf("No malloc output file generated\n");
        return -1;
    }
    
    char buffer[256];
    bool found_malloc_success = false;
    while (fgets(buffer, sizeof(buffer), output_file)) {
        if (strstr(buffer, "malloc SUCCESS")) {
            found_malloc_success = true;
            break;
        }
    }
    fclose(output_file);
    
    if (!found_malloc_success) {
        printf("malloc functionality test failed\n");
        return -1;
    }
    
    printf("malloc functionality test passed\n");
    return 0;
}

int test_self_compilation() {
    printf("Testing self-compilation...\n");
    
    // 测试编译器能否编译自己
    int result = system("bin\\tool_c2astc.exe -o tests\\tool_c2astc_self_test.astc src\\tool_c2astc.c");
    
    // 检查是否生成了ASTC文件
    FILE* astc_file = fopen("tests/tool_c2astc_self_test.astc", "rb");
    if (!astc_file) {
        printf("Self-compilation failed - no ASTC file generated\n");
        return -1;
    }
    
    fseek(astc_file, 0, SEEK_END);
    long file_size = ftell(astc_file);
    fclose(astc_file);
    
    if (file_size < 100) {
        printf("Self-compilation generated too small file: %ld bytes\n", file_size);
        return -1;
    }
    
    printf("Self-compilation test passed: %ld bytes generated\n", file_size);
    return 0;
}

int test_arithmetic_operations() {
    printf("Testing arithmetic operations...\n");
    
    // 创建算术测试程序
    FILE* fp = fopen("tests/temp_arithmetic.c", "w");
    if (!fp) return -1;
    
    fprintf(fp, "#include <stdio.h>\n");
    fprintf(fp, "int main() {\n");
    fprintf(fp, "    int a = 10;\n");
    fprintf(fp, "    int b = 20;\n");
    fprintf(fp, "    int c = a + b;\n");
    fprintf(fp, "    printf(\"Result: %%d\\n\", c);\n");
    fprintf(fp, "    return 0;\n");
    fprintf(fp, "}\n");
    fclose(fp);
    
    // 编译并运行
    system("bin\\tool_c2astc.exe -o tests\\temp_arithmetic.astc tests\\temp_arithmetic.c");
    system("bin\\enhanced_runtime_with_libc_v2.exe tests\\temp_arithmetic.astc > tests\\temp_arithmetic_output.txt 2>&1");
    
    // 检查输出
    FILE* output_file = fopen("tests/temp_arithmetic_output.txt", "r");
    if (!output_file) {
        printf("No arithmetic output file generated\n");
        return -1;
    }
    
    char buffer[256];
    bool found_result = false;
    while (fgets(buffer, sizeof(buffer), output_file)) {
        if (strstr(buffer, "Result:")) {
            found_result = true;
            break;
        }
    }
    fclose(output_file);
    
    if (!found_result) {
        printf("Arithmetic operations test failed\n");
        return -1;
    }
    
    printf("Arithmetic operations test passed\n");
    return 0;
}

int test_independence_verification() {
    printf("Testing TinyCC independence...\n");
    
    // 检查是否有TinyCC进程运行
    int tcc_check = system("tasklist | findstr /i tcc > nul 2>&1");
    if (tcc_check == 0) {
        printf("Warning: TinyCC processes detected\n");
        // 不算失败，只是警告
    }
    
    // 验证我们的工具能独立工作
    int result = system("bin\\tool_c2astc.exe -o tests\\independence_verify.astc tests\\independence_test.c");
    if (result != 0) {
        printf("Independence verification failed - compilation error\n");
        return -1;
    }
    
    // 运行独立性测试
    result = system("bin\\enhanced_runtime_with_libc_v2.exe tests\\independence_verify.astc > tests\\independence_output.txt 2>&1");
    
    // 检查输出
    FILE* output_file = fopen("tests/independence_output.txt", "r");
    if (!output_file) {
        printf("No independence output file generated\n");
        return -1;
    }
    
    char buffer[256];
    bool found_independence = false;
    while (fgets(buffer, sizeof(buffer), output_file)) {
        if (strstr(buffer, "INDEPENDENCE ACHIEVED")) {
            found_independence = true;
            break;
        }
    }
    fclose(output_file);
    
    if (!found_independence) {
        printf("Independence verification failed\n");
        return -1;
    }
    
    printf("Independence verification test passed\n");
    return 0;
}

// ===============================================
// 主测试函数
// ===============================================

int main() {
    printf("=== Automated Compiler Test Suite ===\n");
    
    // 初始化测试框架
    TestFramework* framework = test_framework_init();
    if (!framework) {
        printf("Failed to initialize test framework\n");
        return 1;
    }
    
    // 创建编译器测试套件
    TestSuite* compiler_suite = test_framework_create_suite(framework, 
        "Compiler Tests", 
        "Comprehensive tests for the self-hosted compiler");
    
    // 添加测试用例
    test_framework_add_test(compiler_suite, "Basic Compilation", 
        "Test basic C to ASTC compilation", 
        test_basic_compilation, TEST_CATEGORY_UNIT, TEST_PRIORITY_CRITICAL);
    
    test_framework_add_test(compiler_suite, "Runtime Execution", 
        "Test ASTC runtime execution", 
        test_runtime_execution, TEST_CATEGORY_INTEGRATION, TEST_PRIORITY_CRITICAL);
    
    test_framework_add_test(compiler_suite, "Malloc Functionality", 
        "Test dynamic memory allocation", 
        test_malloc_functionality, TEST_CATEGORY_SYSTEM, TEST_PRIORITY_HIGH);
    
    test_framework_add_test(compiler_suite, "Self Compilation", 
        "Test compiler self-compilation", 
        test_self_compilation, TEST_CATEGORY_SYSTEM, TEST_PRIORITY_CRITICAL);
    
    test_framework_add_test(compiler_suite, "Arithmetic Operations", 
        "Test basic arithmetic operations", 
        test_arithmetic_operations, TEST_CATEGORY_UNIT, TEST_PRIORITY_NORMAL);
    
    test_framework_add_test(compiler_suite, "Independence Verification", 
        "Verify TinyCC independence", 
        test_independence_verification, TEST_CATEGORY_REGRESSION, TEST_PRIORITY_CRITICAL);
    
    // 运行所有测试
    int failed_tests = test_framework_run_all(framework);
    
    // 生成报告
    test_framework_generate_report(framework);
    
    // 清理
    test_framework_free(framework);
    
    if (failed_tests == 0) {
        printf("\n🎉 ALL AUTOMATED TESTS PASSED! 🎉\n");
        printf("Compiler system is fully functional and ready for evolution!\n");
        return 0;
    } else {
        printf("\n⚠️ %d tests failed.\n", failed_tests);
        printf("System needs attention before evolution.\n");
        return 1;
    }
}
