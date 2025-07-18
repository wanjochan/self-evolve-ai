/**
 * c2astc_simple.c - 简化的C到ASTC转换器（无setjmp/longjmp）
 * 
 * 用于诊断pipeline_compile挂起问题
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// 从c2astc.c复制的必要结构
typedef struct {
    int optimize_level;
    int enable_debug;
    int enable_warnings;
    char* output_file;
} CompileOptions;

// 模块加载函数声明
extern int module_system_init(void);
extern void* load_module(const char* path);
extern void* module_resolve(void* module, const char* symbol_name);

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("用法: %s <源文件> <输出文件>\n", argv[0]);
        return 1;
    }

    const char* c_file = argv[1];
    const char* astc_file = argv[2];

    printf("c2astc_simple: 输入文件: %s\n", c_file);
    printf("c2astc_simple: 输出文件: %s\n", astc_file);

    // 读取源文件
    FILE* file = fopen(c_file, "r");
    if (!file) {
        printf("错误: 无法打开源文件 %s\n", c_file);
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* source_code = malloc(file_size + 1);
    if (!source_code) {
        printf("错误: 内存分配失败\n");
        fclose(file);
        return 1;
    }

    size_t bytes_read = fread(source_code, 1, file_size, file);
    source_code[bytes_read] = '\0';
    fclose(file);

    printf("c2astc_simple: 读取了 %zu 字节的源代码\n", bytes_read);
    printf("c2astc_simple: 源代码内容: '%s'\n", source_code);

    // 初始化模块系统
    printf("c2astc_simple: 初始化模块系统...\n");
    if (module_system_init() != 0) {
        printf("错误: 模块系统初始化失败\n");
        free(source_code);
        return 1;
    }
    printf("c2astc_simple: 模块系统初始化成功\n");

    // 加载pipeline模块
    printf("c2astc_simple: 加载pipeline模块...\n");
    void* pipeline_module = load_module("./bin/pipeline");
    if (!pipeline_module) {
        printf("错误: 无法加载pipeline模块\n");
        free(source_code);
        return 1;
    }
    printf("c2astc_simple: pipeline模块加载成功\n");

    // 获取pipeline_compile函数
    printf("c2astc_simple: 解析pipeline_compile函数...\n");
    typedef int (*pipeline_compile_func)(const char*, CompileOptions*);
    pipeline_compile_func pipeline_compile = (pipeline_compile_func)module_resolve(pipeline_module, "pipeline_compile");
    
    if (!pipeline_compile) {
        printf("错误: 无法解析pipeline_compile函数\n");
        free(source_code);
        return 1;
    }
    printf("c2astc_simple: pipeline_compile函数解析成功: %p\n", (void*)pipeline_compile);

    // 设置编译选项
    CompileOptions options = {0};
    options.optimize_level = 0;
    options.enable_debug = 0;
    options.enable_warnings = 1;
    options.output_file = strdup(astc_file);

    printf("c2astc_simple: 准备调用pipeline_compile...\n");
    printf("c2astc_simple: 源代码指针: %p\n", (void*)source_code);
    printf("c2astc_simple: 选项指针: %p\n", (void*)&options);

    // 直接调用pipeline_compile（无setjmp/longjmp保护）
    printf("c2astc_simple: 调用pipeline_compile...\n");
    int result = pipeline_compile(source_code, &options);
    printf("c2astc_simple: pipeline_compile返回: %d\n", result);

    // 清理
    free(source_code);
    free(options.output_file);

    if (result) {
        printf("c2astc_simple: 编译成功\n");
        return 0;
    } else {
        printf("c2astc_simple: 编译失败\n");
        return 1;
    }
}
