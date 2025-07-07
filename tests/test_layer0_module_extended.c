#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "../src/core/module.h"

// 外部声明layer0模块
extern Module module_layer0;

// 内存池类型定义（从layer0_module.c复制）
typedef enum {
    MEMORY_POOL_GENERAL = 0,
    MEMORY_POOL_BYTECODE,
    MEMORY_POOL_JIT,
    MEMORY_POOL_MODULES,
    MEMORY_POOL_TEMP,
    MEMORY_POOL_C99_LEXER,
    MEMORY_POOL_C99_PARSER,
    MEMORY_POOL_C99_CODEGEN,
    MEMORY_POOL_COUNT
} MemoryPoolType;

int test_memory_management() {
    printf("=== Layer0 Memory Management Tests ===\n");
    
    // 获取内存管理函数
    void* alloc_func = module_layer0.resolve("memory_alloc");
    void* (*memory_alloc)(size_t, MemoryPoolType) = (void* (*)(size_t, MemoryPoolType))alloc_func;
    
    void* free_func = module_layer0.resolve("memory_free");
    void (*memory_free)(void*, MemoryPoolType) = (void (*)(void*, MemoryPoolType))free_func;
    
    void* stats_func = module_layer0.resolve("memory_get_stats");
    void (*memory_get_stats)(MemoryPoolType, size_t*, size_t*, size_t*) = 
        (void (*)(MemoryPoolType, size_t*, size_t*, size_t*))stats_func;
    
    if (!memory_alloc || !memory_free) {
        printf("   ERROR: Memory management functions not available\n");
        return 1;
    }
    
    printf("1. Testing basic memory allocation...\n");
    
    // 测试不同大小的内存分配
    size_t test_sizes[] = {16, 64, 256, 1024, 4096};
    void* ptrs[5];
    
    for (int i = 0; i < 5; i++) {
        ptrs[i] = memory_alloc(test_sizes[i], MEMORY_POOL_GENERAL);
        if (!ptrs[i]) {
            printf("   ✗ Failed to allocate %zu bytes\n", test_sizes[i]);
            return 1;
        }
        
        // 测试写入内存
        memset(ptrs[i], 0xAA, test_sizes[i]);
        printf("   ✓ Allocated and wrote %zu bytes\n", test_sizes[i]);
    }
    
    printf("2. Testing memory pool allocation...\n");
    
    // 测试不同内存池
    MemoryPoolType pools[] = {
        MEMORY_POOL_GENERAL,
        MEMORY_POOL_BYTECODE,
        MEMORY_POOL_JIT,
        MEMORY_POOL_MODULES,
        MEMORY_POOL_TEMP
    };
    
    const char* pool_names[] = {
        "GENERAL",
        "BYTECODE", 
        "JIT",
        "MODULES",
        "TEMP"
    };
    
    void* pool_ptrs[5];
    for (int i = 0; i < 5; i++) {
        pool_ptrs[i] = memory_alloc(1024, pools[i]);
        if (pool_ptrs[i]) {
            printf("   ✓ Allocated from %s pool\n", pool_names[i]);
        } else {
            printf("   ✗ Failed to allocate from %s pool\n", pool_names[i]);
        }
    }
    
    printf("3. Testing memory statistics...\n");
    
    if (memory_get_stats) {
        for (int i = 0; i < 5; i++) {
            size_t allocated, used, peak;
            memory_get_stats(pools[i], &allocated, &used, &peak);
            printf("   %s pool - Allocated: %zu, Used: %zu, Peak: %zu\n", 
                   pool_names[i], allocated, used, peak);
        }
    } else {
        printf("   WARNING: Memory statistics not available\n");
    }
    
    printf("4. Testing memory deallocation...\n");
    
    // 释放基本分配的内存
    for (int i = 0; i < 5; i++) {
        if (ptrs[i]) {
            memory_free(ptrs[i], MEMORY_POOL_GENERAL);
            printf("   ✓ Freed %zu bytes\n", test_sizes[i]);
        }
    }
    
    // 释放池分配的内存
    for (int i = 0; i < 5; i++) {
        if (pool_ptrs[i]) {
            memory_free(pool_ptrs[i], pools[i]);
            printf("   ✓ Freed from %s pool\n", pool_names[i]);
        }
    }
    
    printf("   ✓ Memory management tests completed\n");
    return 0;
}

int test_utility_functions() {
    printf("\n=== Layer0 Utility Functions Tests ===\n");
    
    // 获取工具函数
    void* detect_arch_func = module_layer0.resolve("detect_architecture");
    const char* (*detect_architecture)(void) = (const char* (*)(void))detect_arch_func;
    
    void* file_exists_func = module_layer0.resolve("file_exists");
    bool (*file_exists)(const char*) = (bool (*)(const char*))file_exists_func;
    
    void* get_file_size_func = module_layer0.resolve("get_file_size");
    long (*get_file_size)(const char*) = (long (*)(const char*))get_file_size_func;
    
    void* safe_strncpy_func = module_layer0.resolve("safe_strncpy");
    char* (*safe_strncpy)(char*, const char*, size_t) = 
        (char* (*)(char*, const char*, size_t))safe_strncpy_func;
    
    printf("1. Testing architecture detection...\n");
    if (detect_architecture) {
        const char* arch = detect_architecture();
        if (arch) {
            printf("   ✓ Detected architecture: %s\n", arch);
        } else {
            printf("   ✗ Architecture detection failed\n");
            return 1;
        }
    } else {
        printf("   WARNING: Architecture detection not available\n");
    }
    
    printf("2. Testing file operations...\n");
    if (file_exists) {
        // 测试存在的文件
        if (file_exists("README.md")) {
            printf("   ✓ Correctly detected existing file\n");
        } else {
            printf("   WARNING: Could not detect README.md\n");
        }
        
        // 测试不存在的文件
        if (!file_exists("nonexistent_file_12345.txt")) {
            printf("   ✓ Correctly detected non-existing file\n");
        } else {
            printf("   ✗ Incorrectly detected non-existing file\n");
            return 1;
        }
    } else {
        printf("   WARNING: File existence check not available\n");
    }
    
    if (get_file_size) {
        long size = get_file_size("README.md");
        if (size > 0) {
            printf("   ✓ Got file size: %ld bytes\n", size);
        } else {
            printf("   WARNING: Could not get file size\n");
        }
    } else {
        printf("   WARNING: File size function not available\n");
    }
    
    printf("3. Testing string operations...\n");
    if (safe_strncpy) {
        char buffer[100];
        const char* test_str = "Hello, World!";
        
        char* result = safe_strncpy(buffer, test_str, sizeof(buffer));
        if (result && strcmp(buffer, test_str) == 0) {
            printf("   ✓ Safe string copy works correctly\n");
        } else {
            printf("   ✗ Safe string copy failed\n");
            return 1;
        }
        
        // 测试缓冲区溢出保护
        char small_buffer[5];
        safe_strncpy(small_buffer, test_str, sizeof(small_buffer));
        if (strlen(small_buffer) < sizeof(small_buffer)) {
            printf("   ✓ Buffer overflow protection works\n");
        } else {
            printf("   ✗ Buffer overflow protection failed\n");
            return 1;
        }
    } else {
        printf("   WARNING: Safe string copy not available\n");
    }
    
    printf("   ✓ Utility functions tests completed\n");
    return 0;
}

int test_dynamic_loading() {
    printf("\n=== Layer0 Dynamic Loading Tests ===\n");
    
    // 获取动态加载函数
    void* dlopen_func = module_layer0.resolve("dlopen_wrapper");
    void* (*dlopen_wrapper)(const char*, int) = (void* (*)(const char*, int))dlopen_func;
    
    void* dlsym_func = module_layer0.resolve("dlsym_wrapper");
    void* (*dlsym_wrapper)(void*, const char*) = (void* (*)(void*, const char*))dlsym_func;
    
    void* dlclose_func = module_layer0.resolve("dlclose_wrapper");
    int (*dlclose_wrapper)(void*) = (int (*)(void*))dlclose_func;
    
    void* dlerror_func = module_layer0.resolve("dlerror_wrapper");
    char* (*dlerror_wrapper)(void) = (char* (*)(void))dlerror_func;
    
    if (!dlopen_wrapper || !dlsym_wrapper || !dlclose_wrapper) {
        printf("   WARNING: Dynamic loading functions not available\n");
        return 0;
    }
    
    printf("1. Testing library loading...\n");
    
    // 尝试加载标准C库
    const char* test_libs[] = {
        "libc.so.6",     // Linux
        "libm.so.6",     // Math library
        "/lib/x86_64-linux-gnu/libc.so.6"  // Full path
    };
    
    void* handle = NULL;
    for (int i = 0; i < 3; i++) {
        handle = dlopen_wrapper(test_libs[i], 2);  // RTLD_NOW = 2
        if (handle) {
            printf("   ✓ Successfully loaded %s\n", test_libs[i]);
            break;
        }
    }
    
    if (!handle) {
        printf("   WARNING: Could not load any test library\n");
        return 0;
    }
    
    printf("2. Testing symbol resolution...\n");
    
    // 尝试解析一些标准函数
    const char* test_symbols[] = {
        "strlen",
        "malloc",
        "free",
        "printf"
    };
    
    for (int i = 0; i < 4; i++) {
        void* symbol = dlsym_wrapper(handle, test_symbols[i]);
        if (symbol) {
            printf("   ✓ Resolved symbol: %s\n", test_symbols[i]);
        } else {
            printf("   WARNING: Could not resolve symbol: %s\n", test_symbols[i]);
        }
    }
    
    printf("3. Testing error handling...\n");
    
    // 尝试解析不存在的符号
    void* bad_symbol = dlsym_wrapper(handle, "nonexistent_function_12345");
    if (!bad_symbol) {
        if (dlerror_wrapper) {
            char* error = dlerror_wrapper();
            if (error) {
                printf("   ✓ Error handling works: %s\n", error);
            } else {
                printf("   ✓ Error handling works (no error message)\n");
            }
        } else {
            printf("   ✓ Symbol correctly not found\n");
        }
    } else {
        printf("   ✗ Non-existent symbol incorrectly found\n");
    }
    
    printf("4. Testing library unloading...\n");
    
    int result = dlclose_wrapper(handle);
    if (result == 0) {
        printf("   ✓ Library unloaded successfully\n");
    } else {
        printf("   WARNING: Library unload returned %d\n", result);
    }
    
    printf("   ✓ Dynamic loading tests completed\n");
    return 0;
}

int test_error_handling() {
    printf("\n=== Layer0 Error Handling Tests ===\n");
    
    // 获取内存管理函数
    void* alloc_func = module_layer0.resolve("memory_alloc");
    void* (*memory_alloc)(size_t, MemoryPoolType) = (void* (*)(size_t, MemoryPoolType))alloc_func;
    
    void* free_func = module_layer0.resolve("memory_free");
    void (*memory_free)(void*, MemoryPoolType) = (void (*)(void*, MemoryPoolType))free_func;
    
    if (!memory_alloc || !memory_free) {
        printf("   WARNING: Memory functions not available for error testing\n");
        return 0;
    }
    
    printf("1. Testing large allocation handling...\n");
    
    // 尝试分配非常大的内存块
    size_t huge_size = SIZE_MAX / 2;
    void* huge_ptr = memory_alloc(huge_size, MEMORY_POOL_GENERAL);
    if (!huge_ptr) {
        printf("   ✓ Large allocation properly rejected\n");
    } else {
        printf("   WARNING: Large allocation succeeded (unexpected)\n");
        memory_free(huge_ptr, MEMORY_POOL_GENERAL);
    }
    
    printf("2. Testing NULL pointer handling...\n");
    
    // 测试释放NULL指针
    memory_free(NULL, MEMORY_POOL_GENERAL);  // 应该不会崩溃
    printf("   ✓ NULL pointer free handled safely\n");
    
    printf("3. Testing invalid pool handling...\n");
    
    // 测试无效的内存池类型
    void* ptr = memory_alloc(100, (MemoryPoolType)999);
    if (!ptr) {
        printf("   ✓ Invalid pool type properly rejected\n");
    } else {
        printf("   WARNING: Invalid pool type accepted\n");
        memory_free(ptr, MEMORY_POOL_GENERAL);
    }
    
    printf("   ✓ Error handling tests completed\n");
    return 0;
}

int main() {
    printf("=== Extended Layer0 Module Test ===\n");
    
    // 1. 初始化模块
    printf("1. Initializing layer0 module...\n");
    if (module_layer0.init() != 0) {
        printf("ERROR: Failed to initialize layer0 module\n");
        return 1;
    }
    printf("   ✓ Layer0 module initialized successfully\n");
    
    int total_failures = 0;
    
    // 2. 运行内存管理测试
    total_failures += test_memory_management();
    
    // 3. 测试工具函数
    total_failures += test_utility_functions();
    
    // 4. 测试动态加载
    total_failures += test_dynamic_loading();
    
    // 5. 测试错误处理
    total_failures += test_error_handling();
    
    // 6. 清理
    printf("\n=== Cleanup ===\n");
    module_layer0.cleanup();
    printf("   ✓ Cleanup completed\n");
    
    printf("\n=== Extended Layer0 Test Summary ===\n");
    if (total_failures == 0) {
        printf("✓ All extended layer0 tests passed!\n");
    } else {
        printf("✗ %d test(s) failed\n", total_failures);
    }
    
    return total_failures;
}
