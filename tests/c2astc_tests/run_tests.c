/**
 * run_tests.c - 运行所有C2ASTC测试
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c2astc.h"

// 打印分隔线
void print_separator() {
    printf("\n");
    for (int i = 0; i < 80; i++) printf("=");
    printf("\n\n");
}

// 打印AST节点信息
void print_ast_node(struct ASTNode *node, int indent) {
    if (!node) {
        printf("%*s(NULL)\n", indent * 2, "");
        return;
    }
    
    printf("%*sNode(type=%d, line=%d, col=%d)\n", 
           indent * 2, "", node->type, node->line, node->column);
}

// 测试基本功能
int test_basic_functionality() {
    printf("测试基本功能...\n");
    
    // 打印版本信息
    c2astc_print_version();
    printf("\n");
    
    // 获取默认选项
    C2AstcOptions options = c2astc_default_options();
    printf("默认选项:\n");
    printf("  优化级别: %s\n", options.optimize_level ? "开启" : "关闭");
    printf("  扩展支持: %s\n", options.enable_extensions ? "开启" : "关闭");
    printf("  调试信息: %s\n", options.emit_debug_info ? "开启" : "关闭");
    printf("\n");
    
    // 测试错误处理
    const char *error = c2astc_get_error();
    printf("错误处理测试: %s\n", error ? error : "无错误");
    
    return 1;
}

// 测试文件转换
int test_file_conversion(const char *filename) {
    printf("测试文件转换: %s\n", filename);
    
    // 使用默认选项
    C2AstcOptions options = c2astc_default_options();
    
    // 转换文件
    struct ASTNode *root = c2astc_convert_file(filename, &options);
    if (!root) {
        const char *error = c2astc_get_error();
        printf("转换失败: %s\n", error ? error : "未知错误");
        return 0;
    }
    
    printf("转换成功!\n");
    
    // 序列化为二进制
    size_t binary_size;
    unsigned char *binary = c2astc_serialize(root, &binary_size);
    if (!binary) {
        const char *error = c2astc_get_error();
        printf("序列化失败: %s\n", error ? error : "未知错误");
        ast_free(root);
        return 0;
    }
    
    printf("序列化成功! 二进制大小: %zu 字节\n", binary_size);
    
    // 反序列化
    struct ASTNode *deserialized = c2astc_deserialize(binary, binary_size);
    if (!deserialized) {
        const char *error = c2astc_get_error();
        printf("反序列化失败: %s\n", error ? error : "未知错误");
        c2astc_free(binary);
        ast_free(root);
        return 0;
    }
    
    printf("反序列化成功!\n");
    
    // 转换为WASM
    size_t wasm_size;
    unsigned char *wasm = c2astc_to_wasm(root, &options, &wasm_size);
    if (!wasm) {
        const char *error = c2astc_get_error();
        printf("WASM转换失败: %s\n", error ? error : "未知错误");
        ast_free(deserialized);
        c2astc_free(binary);
        ast_free(root);
        return 0;
    }
    
    printf("WASM转换成功! WASM大小: %zu 字节\n", wasm_size);
    
    // 清理资源
    c2astc_free(wasm);
    ast_free(deserialized);
    c2astc_free(binary);
    ast_free(root);
    
    return 1;
}

int main(int argc, char *argv[]) {
    printf("C2ASTC测试套件\n");
    printf("==============\n\n");
    
    // 测试基本功能
    if (!test_basic_functionality()) {
        printf("基本功能测试失败!\n");
        return 1;
    }
    
    print_separator();
    
    // 如果提供了命令行参数，测试指定文件
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (!test_file_conversion(argv[i])) {
                printf("文件转换测试失败: %s\n", argv[i]);
                return 1;
            }
            print_separator();
        }
    } else {
        // 测试内置的简单代码片段
        const char *simple_code = 
            "int main() { return 42; }";
        
        printf("测试简单代码片段转换:\n%s\n", simple_code);
        
        // 使用默认选项
        C2AstcOptions options = c2astc_default_options();
        
        // 转换代码
        struct ASTNode *root = c2astc_convert(simple_code, &options);
        if (!root) {
            const char *error = c2astc_get_error();
            printf("转换失败: %s\n", error ? error : "未知错误");
            return 1;
        }
        
        printf("转换成功!\n");
        ast_free(root);
    }
    
    printf("\n测试通过!\n");
    return 0;
} 