#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <signal.h>
#include <setjmp.h>

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
    printf("=== Pipeline Debug Test ===\n");
    
    // 测试源码
    const char* test_source = "int main() { return 42; }";
    printf("Test source: %s\n", test_source);
    printf("Source length: %zu\n", strlen(test_source));
    
    // 加载pipeline模块
    void* handle = dlopen("./bin/pipeline_x64_64.native", RTLD_LAZY);
    if (!handle) {
        printf("Failed to load pipeline module: %s\n", dlerror());
        return 1;
    }
    
    // 获取函数指针
    typedef int (*pipeline_compile_func)(const char*, CompileOptions*);
    pipeline_compile_func pipeline_compile = (pipeline_compile_func)dlsym(handle, "pipeline_compile");
    
    if (!pipeline_compile) {
        printf("Failed to resolve pipeline_compile: %s\n", dlerror());
        dlclose(handle);
        return 1;
    }
    
    printf("pipeline_compile function found at: %p\n", (void*)pipeline_compile);
    
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
    
    printf("About to call pipeline_compile...\n");
    
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
    
    dlclose(handle);
    
    if (result == 0) {
        printf("SUCCESS: pipeline_compile completed without errors\n");
    } else {
        printf("FAILED: pipeline_compile failed with result %d\n", result);
    }
    
    return result == 0 ? 0 : 1;
}
