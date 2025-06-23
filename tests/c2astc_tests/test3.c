/**
 * test3.c - 测试C语言到WASM的转换
 */

#include <stdio.h>
#include <stdlib.h>
#include "c2astc.h"

// 辅助函数：将二进制数据保存到文件
int save_binary_to_file(const char *filename, const unsigned char *data, size_t size) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        printf("无法创建文件: %s\n", filename);
        return 0;
    }
    
    size_t written = fwrite(data, 1, size, fp);
    fclose(fp);
    
    if (written != size) {
        printf("写入文件失败: %s\n", filename);
        return 0;
    }
    
    return 1;
}

int main() {
    printf("测试3: C语言到WASM的转换\n");
    printf("=========================\n\n");
    
    // 简单的C代码，适合转换为WASM
    const char *test_code = 
        "// 简单的C程序，适合转换为WASM\n"
        "// 计算斐波那契数列\n\n"
        "int fibonacci(int n) {\n"
        "    if (n <= 0) return 0;\n"
        "    if (n == 1) return 1;\n"
        "    return fibonacci(n-1) + fibonacci(n-2);\n"
        "}\n\n"
        "// WASM导出函数\n"
        "__attribute__((export_name(\"fib\")))\n"
        "int fib(int n) {\n"
        "    return fibonacci(n);\n"
        "}\n";
    
    printf("测试代码:\n%s\n", test_code);
    
    // 使用默认选项
    C2AstcOptions options = c2astc_default_options();
    
    // 转换为ASTC
    printf("转换为ASTC...\n");
    struct ASTNode *ast = c2astc_convert(test_code, &options);
    
    if (!ast) {
        const char *error = c2astc_get_error();
        printf("转换失败: %s\n", error ? error : "未知错误");
        return 1;
    }
    
    printf("转换成功!\n\n");
    
    // 转换为WASM
    printf("转换为WASM...\n");
    size_t wasm_size;
    unsigned char *wasm = c2astc_to_wasm(ast, &options, &wasm_size);
    
    if (!wasm) {
        const char *error = c2astc_get_error();
        printf("WASM转换失败: %s\n", error ? error : "未知错误");
        ast_free(ast);
        return 1;
    }
    
    printf("WASM转换成功! WASM大小: %zu 字节\n", wasm_size);
    
    // 保存WASM到文件
    const char *wasm_filename = "fibonacci.wasm";
    printf("保存WASM到文件: %s\n", wasm_filename);
    
    if (save_binary_to_file(wasm_filename, wasm, wasm_size)) {
        printf("WASM文件保存成功!\n");
    } else {
        printf("WASM文件保存失败!\n");
    }
    
    // 清理资源
    c2astc_free(wasm);
    ast_free(ast);
    
    printf("\n测试3完成!\n");
    return 0;
} 