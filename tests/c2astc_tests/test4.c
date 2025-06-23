/**
 * test4.c - 测试文件转换功能
 */

#include <stdio.h>
#include <stdlib.h>
#include "c2astc.h"

// 辅助函数：创建测试C文件
int create_test_file(const char *filename, const char *content) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        printf("无法创建文件: %s\n", filename);
        return 0;
    }
    
    fprintf(fp, "%s", content);
    fclose(fp);
    return 1;
}

int main() {
    printf("测试4: 文件转换功能\n");
    printf("===================\n\n");
    
    // 创建测试文件
    const char *test_filename = "test_source.c";
    const char *test_content = 
        "#include <stdio.h>\n\n"
        "/**\n"
        " * 计算阶乘的函数\n"
        " */\n"
        "int factorial(int n) {\n"
        "    if (n <= 1) return 1;\n"
        "    return n * factorial(n-1);\n"
        "}\n\n"
        "int main() {\n"
        "    int n = 5;\n"
        "    printf(\"Factorial of %d is %d\\n\", n, factorial(n));\n"
        "    return 0;\n"
        "}\n";
    
    printf("创建测试文件: %s\n", test_filename);
    if (!create_test_file(test_filename, test_content)) {
        printf("创建测试文件失败!\n");
        return 1;
    }
    
    printf("测试文件内容:\n%s\n", test_content);
    
    // 使用默认选项
    C2AstcOptions options = c2astc_default_options();
    
    // 从文件转换为ASTC
    printf("从文件转换为ASTC...\n");
    struct ASTNode *ast = c2astc_convert_file(test_filename, &options);
    
    if (!ast) {
        const char *error = c2astc_get_error();
        printf("转换失败: %s\n", error ? error : "未知错误");
        return 1;
    }
    
    printf("转换成功!\n\n");
    
    // 打印AST结构
    printf("AST结构:\n");
    ast_print(ast, 0);
    
    // 序列化为二进制
    size_t binary_size;
    printf("\n序列化为二进制...\n");
    unsigned char *binary = c2astc_serialize(ast, &binary_size);
    
    if (!binary) {
        const char *error = c2astc_get_error();
        printf("序列化失败: %s\n", error ? error : "未知错误");
        ast_free(ast);
        return 1;
    }
    
    printf("序列化成功! 二进制大小: %zu 字节\n", binary_size);
    
    // 保存二进制到文件
    const char *binary_filename = "factorial.astc";
    FILE *fp = fopen(binary_filename, "wb");
    if (fp) {
        fwrite(binary, 1, binary_size, fp);
        fclose(fp);
        printf("二进制数据已保存到文件: %s\n", binary_filename);
    } else {
        printf("无法创建二进制文件: %s\n", binary_filename);
    }
    
    // 反序列化
    printf("\n反序列化...\n");
    struct ASTNode *ast2 = c2astc_deserialize(binary, binary_size);
    
    if (!ast2) {
        const char *error = c2astc_get_error();
        printf("反序列化失败: %s\n", error ? error : "未知错误");
        c2astc_free(binary);
        ast_free(ast);
        return 1;
    }
    
    printf("反序列化成功!\n");
    
    // 清理资源
    ast_free(ast2);
    c2astc_free(binary);
    ast_free(ast);
    
    printf("\n测试4完成!\n");
    return 0;
} 