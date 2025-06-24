/**
 * test_c2astc.c - c2astc模块的测试程序
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../astc.h"
#include "../../c2astc.h"

// 测试用例
const char *test_cases[] = {
    // 测试1：简单变量声明
    "int main() { int a = 10; return 0; }",
    
    // 测试2：简单函数调用
    "int foo(int a, int b) { return a + b; }",
    
    // 测试3：复杂表达式
    "int calc(int a, int b) { return (a + b) * (a - b) / 2; }",
    
    // 测试4：if语句
    "int max(int a, int b) { if (a > b) return a; else return b; }"
};

// 打印缩进
void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

// 测试c2astc_convert函数
void test_convert() {
    printf("===== 测试 c2astc_convert =====\n");
    
    for (int i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        printf("测试用例 %d:\n", i + 1);
        printf("源代码: %s\n", test_cases[i]);
        
        // 转换为ASTC
        C2AstcOptions options = c2astc_default_options();
        struct ASTNode *root = c2astc_convert(test_cases[i], &options);
        
        if (root) {
            printf("转换成功!\n");
            printf("ASTC树:\n");
            ast_print(root, 0);
            ast_free(root);
        } else {
            printf("转换失败: %s\n", c2astc_get_error());
        }
        
        printf("\n");
    }
}

/**
 * 测试从文件转换
 */
void test_convert_file(const char *filename) {
    printf("测试从文件转换: %s\n", filename);
    
    // 使用默认选项
    C2AstcOptions options = c2astc_default_options();
    
    // 转换文件
    struct ASTNode *root = c2astc_convert_file(filename, &options);
    if (!root) {
        printf("转换失败: %s\n", c2astc_get_error());
        return;
    }
    
    // 打印AST
    printf("转换成功，AST结构:\n");
    ast_print(root, 0);
    
    // 释放AST
    ast_free(root);
    
    printf("\n");
}

// 测试c2astc_serialize和c2astc_deserialize函数
void test_serialize_deserialize() {
    printf("===== 测试 c2astc_serialize 和 c2astc_deserialize =====\n");
    
    // 创建一个简单的ASTC节点
    struct ASTNode *node = ast_create_node(ASTC_TRANSLATION_UNIT, 1, 1);
    if (!node) {
        printf("创建节点失败\n");
        return;
    }
    
    // 序列化
    size_t size = 0;
    unsigned char *binary = c2astc_serialize(node, &size);
    if (!binary) {
        printf("序列化失败: %s\n", c2astc_get_error());
        ast_free(node);
        return;
    }
    
    printf("序列化成功，大小: %zu 字节\n", size);
    
    // 反序列化
    struct ASTNode *deserialized = c2astc_deserialize(binary, size);
    if (!deserialized) {
        printf("反序列化失败: %s\n", c2astc_get_error());
        ast_free(node);
        free(binary);
        return;
    }
    
    printf("反序列化成功\n");
    printf("原始节点类型: %d\n", node->type);
    printf("反序列化节点类型: %d\n", deserialized->type);
    
    // 释放资源
    ast_free(node);
    ast_free(deserialized);
    free(binary);
    
    printf("\n");
}

// 读取文件内容
static char* read_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "无法打开文件: %s\n", filename);
        return NULL;
    }
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // 分配内存
    char *buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fprintf(stderr, "内存分配失败\n");
        fclose(file);
        return NULL;
    }
    
    // 读取文件内容
    size_t bytes_read = fread(buffer, 1, size, file);
    buffer[bytes_read] = '\0';
    
    fclose(file);
    return buffer;
}

// 测试C到ASTC的转换
static int test_c2astc(const char *source_code) {
    printf("测试C到ASTC的转换...\n");
    
    // 转换代码
    C2AstcOptions options = c2astc_default_options();
    struct ASTNode *root = c2astc_convert(source_code, &options);
    
    if (!root) {
        const char *error = c2astc_get_error();
        fprintf(stderr, "转换失败: %s\n", error ? error : "未知错误");
        return 0;
    }
    
    // 打印AST
    printf("AST结构:\n");
    ast_print(root, 0);
    
    // 释放资源
    ast_free(root);
    
    return 1;
}

// 测试序列化和反序列化
static int test_serialization(const char *source_code) {
    printf("\n测试序列化和反序列化...\n");
    
    // 转换代码
    C2AstcOptions options = c2astc_default_options();
    struct ASTNode *root = c2astc_convert(source_code, &options);
    
    if (!root) {
        const char *error = c2astc_get_error();
        fprintf(stderr, "转换失败: %s\n", error ? error : "未知错误");
        return 0;
    }
    
    // 序列化
    size_t binary_size;
    unsigned char *binary = c2astc_serialize(root, &binary_size);
    
    if (!binary) {
        const char *error = c2astc_get_error();
        fprintf(stderr, "序列化失败: %s\n", error ? error : "未知错误");
        ast_free(root);
        return 0;
    }
    
    printf("序列化成功，二进制大小: %zu 字节\n", binary_size);
    
    // 反序列化
    struct ASTNode *deserialized = c2astc_deserialize(binary, binary_size);
    
    if (!deserialized) {
        const char *error = c2astc_get_error();
        fprintf(stderr, "反序列化失败: %s\n", error ? error : "未知错误");
        free(binary);
        ast_free(root);
        return 0;
    }
    
    printf("反序列化成功\n");
    
    // 释放资源
    free(binary);
    ast_free(root);
    ast_free(deserialized);
    
    return 1;
}

// 测试复杂类型
static int test_complex_types(const char *source_code) {
    printf("\n测试复杂类型...\n");
    
    // 转换代码
    C2AstcOptions options = c2astc_default_options();
    struct ASTNode *root = c2astc_convert(source_code, &options);
    
    if (!root) {
        const char *error = c2astc_get_error();
        fprintf(stderr, "转换失败: %s\n", error ? error : "未知错误");
        return 0;
    }
    
    // 打印AST
    printf("复杂类型AST结构:\n");
    ast_print(root, 0);
    
    // 释放资源
    ast_free(root);
    
    return 1;
}

int main(int argc, char *argv[]) {
    printf("C2ASTC 测试程序\n\n");
    
    // 打印版本信息
    c2astc_print_version();
    printf("\n");
    
    // 测试c2astc_convert函数
    test_convert();
    
    // 测试c2astc_convert_file函数
    if (argc > 1) {
        test_convert_file(argv[1]);
    } else {
        printf("提示: 可以通过命令行参数指定要测试的C源文件\n");
        printf("示例: %s complex_test.c\n\n", argv[0]);
    }
    
    // 测试序列化和反序列化
    test_serialize_deserialize();
    
    // 默认测试代码
    const char *default_source = 
        "int add(int a, int b) {\n"
        "    return a + b;\n"
        "}\n"
        "\n"
        "int main() {\n"
        "    int x = 10;\n"
        "    int y = 20;\n"
        "    int z = add(x, y);\n"
        "    return z;\n"
        "}\n";
    
    char *source_code = NULL;
    char *complex_types_code = NULL;
    
    // 检查是否提供了文件名
    if (argc > 1) {
        source_code = read_file(argv[1]);
        if (!source_code) {
            fprintf(stderr, "使用默认测试代码...\n");
            source_code = strdup(default_source);
        }
    } else {
        printf("使用默认测试代码...\n");
        source_code = strdup(default_source);
    }
    
    // 尝试读取复杂类型测试文件
    complex_types_code = read_file("complex_types_test.c");
    if (!complex_types_code) {
        fprintf(stderr, "无法读取复杂类型测试文件，跳过复杂类型测试...\n");
    }
    
    // 运行测试
    int success = 1;
    success &= test_c2astc(source_code);
    success &= test_serialization(source_code);
    
    if (complex_types_code) {
        success &= test_complex_types(complex_types_code);
        free(complex_types_code);
    }
    
    // 释放资源
    free(source_code);
    
    return success ? 0 : 1;
} 