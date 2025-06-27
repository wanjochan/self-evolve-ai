/**
 * evolver1_loader.c - evolver1加载器
 * 
 * 基于evolver0_loader的改进版本，支持更复杂的ASTC程序加载
 * 主要改进：
 * 1. 支持更大的ASTC文件
 * 2. 改进的错误处理
 * 3. 更好的内存管理
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ASTC_SIZE (1024 * 1024)  // 1MB最大ASTC文件大小

// 简化的运行时接口
typedef struct {
    unsigned char* astc_data;
    size_t astc_size;
    int exit_code;
} RuntimeContext;

// 加载ASTC文件
int load_astc_file(const char* filename, unsigned char** data, size_t* size) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "错误: 无法打开ASTC文件 %s\n", filename);
        return 1;
    }
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (*size > MAX_ASTC_SIZE) {
        fprintf(stderr, "错误: ASTC文件过大 (%zu 字节)\n", *size);
        fclose(file);
        return 1;
    }
    
    // 分配内存并读取文件
    *data = malloc(*size);
    if (!*data) {
        fprintf(stderr, "错误: 内存分配失败\n");
        fclose(file);
        return 1;
    }
    
    size_t read_size = fread(*data, 1, *size, file);
    fclose(file);
    
    if (read_size != *size) {
        fprintf(stderr, "错误: 文件读取不完整\n");
        free(*data);
        return 1;
    }
    
    printf("✅ ASTC文件加载成功: %s (%zu 字节)\n", filename, *size);
    return 0;
}

// 加载运行时
int load_runtime(const char* runtime_file, unsigned char** runtime_data, size_t* runtime_size) {
    FILE* file = fopen(runtime_file, "rb");
    if (!file) {
        fprintf(stderr, "错误: 无法打开运行时文件 %s\n", runtime_file);
        return 1;
    }
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    *runtime_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // 分配内存并读取文件
    *runtime_data = malloc(*runtime_size);
    if (!*runtime_data) {
        fprintf(stderr, "错误: 运行时内存分配失败\n");
        fclose(file);
        return 1;
    }
    
    size_t read_size = fread(*runtime_data, 1, *runtime_size, file);
    fclose(file);
    
    if (read_size != *runtime_size) {
        fprintf(stderr, "错误: 运行时文件读取不完整\n");
        free(*runtime_data);
        return 1;
    }
    
    printf("✅ 运行时加载成功: %s (%zu 字节)\n", runtime_file, *runtime_size);
    return 0;
}

// 执行ASTC程序
int execute_astc(unsigned char* astc_data, size_t astc_size, unsigned char* runtime_data, size_t runtime_size) {
    printf("🚀 开始执行ASTC程序...\n");
    
    // 简化的执行逻辑
    // 在真实实现中，这里会调用运行时引擎
    
    // 验证ASTC格式
    if (astc_size < 8 || memcmp(astc_data, "ASTC", 4) != 0) {
        fprintf(stderr, "错误: 无效的ASTC格式\n");
        return 1;
    }
    
    printf("📊 ASTC版本: %d\n", *(int*)(astc_data + 4));
    printf("📊 ASTC数据大小: %zu 字节\n", astc_size);
    printf("📊 运行时大小: %zu 字节\n", runtime_size);
    
    // 模拟执行结果
    printf("✅ ASTC程序执行完成\n");
    return 42;  // 模拟返回值
}

int main(int argc, char* argv[]) {
    printf("evolver1_loader v1.0 - 改进的ASTC加载器\n");
    
    if (argc != 3) {
        printf("用法: %s <runtime.bin> <program.astc>\n", argv[0]);
        return 1;
    }
    
    const char* runtime_file = argv[1];
    const char* astc_file = argv[2];
    
    // 加载运行时
    unsigned char* runtime_data;
    size_t runtime_size;
    if (load_runtime(runtime_file, &runtime_data, &runtime_size) != 0) {
        return 1;
    }
    
    // 加载ASTC程序
    unsigned char* astc_data;
    size_t astc_size;
    if (load_astc_file(astc_file, &astc_data, &astc_size) != 0) {
        free(runtime_data);
        return 1;
    }
    
    // 执行程序
    int exit_code = execute_astc(astc_data, astc_size, runtime_data, runtime_size);
    
    // 清理资源
    free(astc_data);
    free(runtime_data);
    
    printf("🏁 程序退出，返回码: %d\n", exit_code);
    return exit_code;
}
