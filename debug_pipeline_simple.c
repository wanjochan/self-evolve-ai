#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

// 直接包含pipeline模块的函数声明
extern int pipeline_compile(const char* source_code, void* options);

// 简单的编译选项结构
typedef struct {
    int optimize_level;
    int enable_debug;
    int enable_warnings;
    char output_file[256];
} CompileOptions;

// 信号处理
static jmp_buf jump_buffer;
static int signal_received = 0;

void signal_handler(int sig) {
    signal_received = sig;
    longjmp(jump_buffer, 1);
}

int main() {
    printf("=== Direct Pipeline Function Test ===\n");
    
    // 测试源码
    const char* test_source = "int main() { return 42; }";
    printf("Test source: %s\n", test_source);
    printf("Source length: %zu\n", strlen(test_source));
    
    // 设置编译选项
    CompileOptions options = {0};
    options.optimize_level = 0;
    options.enable_debug = 0;
    options.enable_warnings = 1;
    strcpy(options.output_file, "/tmp/debug_test.astc");
    
    // 设置信号处理
    signal(SIGSEGV, signal_handler);
    signal(SIGBUS, signal_handler);
    signal(SIGFPE, signal_handler);
    
    printf("About to call pipeline_compile directly...\n");
    
    int result = -1;
    if (setjmp(jump_buffer) == 0) {
        // 正常执行路径
        printf("Calling pipeline_compile with source: '%s'\n", test_source);
        result = pipeline_compile(test_source, &options);
        printf("pipeline_compile returned: %d\n", result);
    } else {
        // 信号处理路径
        printf("Signal %d caught during pipeline_compile call\n", signal_received);
        result = -1;
    }
    
    // 恢复默认信号处理
    signal(SIGSEGV, SIG_DFL);
    signal(SIGBUS, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    
    if (result == 0) {
        printf("SUCCESS: pipeline_compile completed without errors\n");
    } else {
        printf("FAILED: pipeline_compile failed with result %d\n", result);
    }
    
    return result == 0 ? 0 : 1;
}
