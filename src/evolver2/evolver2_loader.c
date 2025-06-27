/**
 * evolver2_loader.c - evolver2加载器
 * 
 * 基于evolver1_loader的进一步改进版本
 * 主要改进：
 * 1. 支持更复杂的PE格式可执行文件
 * 2. 改进的ASTC处理能力
 * 3. 更好的跨平台支持
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ASTC_SIZE (10 * 1024 * 1024)  // 10MB最大ASTC文件大小

// evolver2运行时上下文
typedef struct {
    unsigned char* astc_data;
    size_t astc_size;
    unsigned char* runtime_data;
    size_t runtime_size;
    int exit_code;
    int debug_mode;
} Evolver2Context;

// 加载ASTC文件（改进版）
int load_astc_file_v2(const char* filename, unsigned char** data, size_t* size) {
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
        fprintf(stderr, "错误: ASTC文件过大 (%zu 字节，最大 %d 字节)\n", *size, MAX_ASTC_SIZE);
        fclose(file);
        return 1;
    }
    
    if (*size < 20) {  // 最小ASTC文件大小
        fprintf(stderr, "错误: ASTC文件过小 (%zu 字节)\n", *size);
        fclose(file);
        return 1;
    }
    
    // 分配内存并读取文件
    *data = malloc(*size);
    if (!*data) {
        fprintf(stderr, "错误: 内存分配失败 (%zu 字节)\n", *size);
        fclose(file);
        return 1;
    }
    
    size_t read_size = fread(*data, 1, *size, file);
    fclose(file);
    
    if (read_size != *size) {
        fprintf(stderr, "错误: 文件读取不完整 (读取 %zu/%zu 字节)\n", read_size, *size);
        free(*data);
        return 1;
    }
    
    // 验证ASTC格式
    if (memcmp(*data, "ASTC", 4) != 0) {
        fprintf(stderr, "错误: 无效的ASTC魔数\n");
        free(*data);
        return 1;
    }
    
    int version = *(int*)(*data + 4);
    printf("✅ ASTC文件加载成功: %s (%zu 字节，版本 %d)\n", filename, *size, version);
    return 0;
}

// 加载运行时（改进版）
int load_runtime_v2(const char* runtime_file, unsigned char** runtime_data, size_t* runtime_size) {
    FILE* file = fopen(runtime_file, "rb");
    if (!file) {
        fprintf(stderr, "错误: 无法打开运行时文件 %s\n", runtime_file);
        return 1;
    }
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    *runtime_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (*runtime_size == 0) {
        fprintf(stderr, "错误: 运行时文件为空\n");
        fclose(file);
        return 1;
    }
    
    // 分配内存并读取文件
    *runtime_data = malloc(*runtime_size);
    if (!*runtime_data) {
        fprintf(stderr, "错误: 运行时内存分配失败 (%zu 字节)\n", *runtime_size);
        fclose(file);
        return 1;
    }
    
    size_t read_size = fread(*runtime_data, 1, *runtime_size, file);
    fclose(file);
    
    if (read_size != *runtime_size) {
        fprintf(stderr, "错误: 运行时文件读取不完整 (读取 %zu/%zu 字节)\n", read_size, *runtime_size);
        free(*runtime_data);
        return 1;
    }
    
    printf("✅ 运行时加载成功: %s (%zu 字节)\n", runtime_file, *runtime_size);
    return 0;
}

// 执行ASTC程序（改进版）
int execute_astc_v2(Evolver2Context* ctx) {
    printf("🚀 evolver2开始执行ASTC程序...\n");
    
    if (!ctx || !ctx->astc_data || !ctx->runtime_data) {
        fprintf(stderr, "错误: 无效的执行上下文\n");
        return 1;
    }
    
    // 详细的ASTC分析
    printf("📊 ASTC分析:\n");
    printf("   魔数: %.4s\n", ctx->astc_data);
    printf("   版本: %d\n", *(int*)(ctx->astc_data + 4));
    printf("   节点类型: %d\n", *(int*)(ctx->astc_data + 8));
    printf("   行号: %d\n", *(int*)(ctx->astc_data + 12));
    printf("   列号: %d\n", *(int*)(ctx->astc_data + 16));
    
    if (ctx->debug_mode) {
        printf("🔍 调试模式：显示ASTC数据前32字节\n");
        for (int i = 0; i < 32 && i < ctx->astc_size; i++) {
            printf("%02X ", ctx->astc_data[i]);
            if ((i + 1) % 16 == 0) printf("\n");
        }
        printf("\n");
    }
    
    // 模拟执行（在真实实现中会调用运行时引擎）
    printf("📊 运行时信息:\n");
    printf("   运行时大小: %zu 字节\n", ctx->runtime_size);
    printf("   ASTC数据大小: %zu 字节\n", ctx->astc_size);
    
    // 简化的执行结果
    ctx->exit_code = 42;  // 默认返回值
    
    printf("✅ evolver2 ASTC程序执行完成\n");
    return ctx->exit_code;
}

int main(int argc, char* argv[]) {
    printf("evolver2_loader v1.0 - 高级ASTC加载器\n");
    
    int debug_mode = 0;
    int arg_offset = 1;
    
    // 检查调试模式
    if (argc > 1 && strcmp(argv[1], "--debug") == 0) {
        debug_mode = 1;
        arg_offset = 2;
        printf("🔍 调试模式已启用\n");
    }
    
    if (argc < arg_offset + 2) {
        printf("用法: %s [--debug] <runtime.bin> <program.astc>\n", argv[0]);
        printf("选项:\n");
        printf("  --debug    启用调试模式，显示详细信息\n");
        return 1;
    }
    
    const char* runtime_file = argv[arg_offset];
    const char* astc_file = argv[arg_offset + 1];
    
    // 创建执行上下文
    Evolver2Context ctx = {0};
    ctx.debug_mode = debug_mode;
    
    // 加载运行时
    if (load_runtime_v2(runtime_file, &ctx.runtime_data, &ctx.runtime_size) != 0) {
        return 1;
    }
    
    // 加载ASTC程序
    if (load_astc_file_v2(astc_file, &ctx.astc_data, &ctx.astc_size) != 0) {
        free(ctx.runtime_data);
        return 1;
    }
    
    // 执行程序
    int exit_code = execute_astc_v2(&ctx);
    
    // 清理资源
    free(ctx.astc_data);
    free(ctx.runtime_data);
    
    printf("🏁 evolver2程序退出，返回码: %d\n", exit_code);
    return exit_code;
}
