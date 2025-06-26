/**
 * evolver0_runtime.c - 第零代Runtime实现
 * 
 * 这是evolver0的Runtime层，包含完整的ASTC虚拟机
 * 编译为无头二进制，由evolver0_loader加载执行
 * 
 * 职责：
 * 1. 实现完整的ASTC虚拟机
 * 2. 执行Program层的ASTC代码
 * 3. 提供系统调用接口
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// 引入我们已有的runtime核心
#include "runtime.h"
#include "astc.h"
#include "c2astc.h"

// ===============================================
// Runtime入口点和接口
// ===============================================

// Runtime的主入口点，由Loader调用
// 参数：ASTC程序数据和大小
int evolver0_runtime_main(const unsigned char* astc_data, size_t astc_size) {
    printf("Evolver0 Runtime starting...\n");
    printf("ASTC data size: %zu bytes\n", astc_size);
    
    // 初始化虚拟机
    RuntimeVM vm;
    if (!runtime_init(&vm)) {
        fprintf(stderr, "Runtime: Failed to initialize VM\n");
        return 1;
    }
    
    // 反序列化ASTC程序
    struct ASTNode* program = c2astc_deserialize(astc_data, astc_size);
    if (!program) {
        fprintf(stderr, "Runtime: Failed to deserialize ASTC program\n");
        runtime_destroy(&vm);
        return 1;
    }
    
    // 加载程序到虚拟机
    if (!runtime_load_program(&vm, program)) {
        fprintf(stderr, "Runtime: Failed to load program: %s\n", runtime_get_error(&vm));
        ast_free(program);
        runtime_destroy(&vm);
        return 1;
    }
    
    // 执行main函数
    printf("Runtime: Executing program...\n");
    int result = runtime_execute(&vm, "main");
    printf("Runtime: Program completed with result: %d\n", result);
    
    // 清理
    ast_free(program);
    runtime_destroy(&vm);
    
    return result;
}

// 编译服务参数结构
typedef struct {
    const char* source_code;
    const char* filename;
    char** output_data;
    size_t* output_size;
} CompileArgs;

// Runtime系统调用接口
int evolver0_runtime_syscall(int syscall_num, void* args) {
    switch (syscall_num) {
        case 1: // sys_write
            printf("Runtime syscall: write\n");
            return 0;
        case 2: // sys_read
            printf("Runtime syscall: read\n");
            return 0;
        case 3: // sys_compile_c_to_astc
            printf("Runtime syscall: compile C to ASTC\n");
            if (args) {
                CompileArgs* compile_args = (CompileArgs*)args;
                return runtime_compile_c_to_astc(compile_args->source_code,
                                                compile_args->filename,
                                                compile_args->output_data,
                                                compile_args->output_size);
            }
            return -1;
        case 4: // sys_file_read
            printf("Runtime syscall: file read\n");
            return 0;
        case 5: // sys_file_write
            printf("Runtime syscall: file write\n");
            return 0;
        default:
            printf("Runtime syscall: unknown %d\n", syscall_num);
            return -1;
    }
}

// Runtime编译服务实现
int runtime_compile_c_to_astc(const char* source_code, const char* filename, char** output_data, size_t* output_size) {
    printf("Runtime: 编译C源码为ASTC格式\n");
    printf("  源文件: %s\n", filename ? filename : "内存中的代码");
    printf("  源码长度: %zu 字节\n", source_code ? strlen(source_code) : 0);

    if (!source_code || !output_data || !output_size) {
        printf("  错误: 无效的参数\n");
        return -1;
    }

    // 使用c2astc库进行编译
    C2AstcOptions options = c2astc_default_options();
    struct ASTNode* ast = c2astc_convert(source_code, &options);

    if (!ast) {
        const char* error = c2astc_get_error();
        printf("  编译失败: %s\n", error ? error : "未知错误");
        return -1;
    }

    // 序列化AST为ASTC格式
    unsigned char* astc_data = c2astc_serialize(ast, output_size);
    if (!astc_data) {
        printf("  序列化失败\n");
        ast_free(ast);
        return -1;
    }

    // 分配输出缓冲区并复制数据
    *output_data = (char*)malloc(*output_size);
    if (!*output_data) {
        printf("  内存分配失败\n");
        free(astc_data);
        ast_free(ast);
        return -1;
    }

    memcpy(*output_data, astc_data, *output_size);

    // 清理
    free(astc_data);
    ast_free(ast);

    printf("  ✅ 编译成功，生成 %zu 字节的ASTC数据\n", *output_size);
    return 0;
}

// Runtime内存管理接口
void* evolver0_runtime_alloc(size_t size) {
    return malloc(size);
}

void evolver0_runtime_free(void* ptr) {
    free(ptr);
}

// ===============================================
// Runtime信息和版本
// ===============================================

const char* evolver0_runtime_version(void) {
    return "Evolver0 Runtime v1.0 - ASTC Virtual Machine";
}

uint32_t evolver0_runtime_get_version_number(void) {
    return 0x00010000; // 1.0.0
}

// ===============================================
// Runtime导出符号表
// ===============================================

// 这个结构定义了Runtime对外提供的接口
typedef struct {
    const char* name;
    void* function;
} RuntimeExport;

static const RuntimeExport runtime_exports[] = {
    {"main", evolver0_runtime_main},
    {"syscall", evolver0_runtime_syscall},
    {"alloc", evolver0_runtime_alloc},
    {"free", evolver0_runtime_free},
    {"version", evolver0_runtime_version},
    {"get_version_number", evolver0_runtime_get_version_number},
    {NULL, NULL} // 结束标记
};

// 获取Runtime导出函数
void* evolver0_runtime_get_export(const char* name) {
    for (int i = 0; runtime_exports[i].name; i++) {
        if (strcmp(runtime_exports[i].name, name) == 0) {
            return runtime_exports[i].function;
        }
    }
    return NULL;
}

// ===============================================
// Runtime初始化和清理
// ===============================================

// Runtime全局初始化
int evolver0_runtime_init(void) {
    printf("Evolver0 Runtime initializing...\n");
    // 这里可以进行全局初始化
    return 0;
}

// Runtime全局清理
void evolver0_runtime_cleanup(void) {
    printf("Evolver0 Runtime cleaning up...\n");
    // 这里可以进行全局清理
}

// ===============================================
// 编译为无头二进制的入口点
// ===============================================

// 当Runtime被编译为无头二进制时，这是真正的入口点
// 这个函数会被Loader通过函数指针调用
int _start(void) {
    // 这是无头二进制的入口点
    // 实际的参数会通过其他方式传递
    printf("Evolver0 Runtime binary entry point\n");
    return evolver0_runtime_init();
}

// ===============================================
// 测试和调试接口
// ===============================================

#ifdef EVOLVER0_RUNTIME_TEST
// 测试模式下的main函数
int main(int argc, char* argv[]) {
    printf("=== Evolver0 Runtime Test Mode ===\n");
    
    if (argc < 2) {
        printf("Usage: %s <astc_file>\n", argv[0]);
        return 1;
    }
    
    // 读取ASTC文件
    FILE* file = fopen(argv[1], "rb");
    if (!file) {
        fprintf(stderr, "Cannot open ASTC file: %s\n", argv[1]);
        return 1;
    }
    
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    unsigned char* data = malloc(size);
    fread(data, 1, size, file);
    fclose(file);
    
    // 执行Runtime
    int result = evolver0_runtime_main(data, size);
    
    free(data);
    return result;
}
#endif
