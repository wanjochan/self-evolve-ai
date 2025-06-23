/**
 * run_tests.c - C2ASTC测试运行程序
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../c2astc.h"

// 测试文件列表
const char *test_files[] = {
    "test1.c",
    "test2.c", 
    "test3.c",
    "test4.c"
};
const int num_tests = sizeof(test_files) / sizeof(test_files[0]);

// 递归打印AST节点
void print_ast_node(struct ASTNode *node, int indent) {
    if (!node) return;
    
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
    
    // 打印节点类型信息
    printf("Node Type: %d\n", node->type);
    
    // TODO: 根据节点类型，打印特定信息并递归打印子节点
}

// 运行单个测试
int run_test(const char *filename) {
    printf("测试文件: %s\n", filename);
    
    // 设置选项
    C2AstcOptions options = c2astc_default_options();
    options.emit_debug_info = true;
    
    // 转换C代码到ASTC
    struct ASTNode *root = c2astc_convert_file(filename, &options);
    if (!root) {
        printf("转换失败: %s\n", c2astc_get_error());
        return 0;
    }
    
    // 打印ASTC
    printf("ASTC结构:\n");
    print_ast_node(root, 0);
    
    // 测试序列化和反序列化
    size_t binary_size = 0;
    unsigned char *binary = c2astc_serialize(root, &binary_size);
    if (!binary) {
        printf("序列化失败: %s\n", c2astc_get_error());
        ast_free(root);
        return 0;
    }
    
    printf("序列化大小: %zu 字节\n", binary_size);
    
    struct ASTNode *deserialized = c2astc_deserialize(binary, binary_size);
    if (!deserialized) {
        printf("反序列化失败: %s\n", c2astc_get_error());
        c2astc_free(binary);
        ast_free(root);
        return 0;
    }
    
    printf("反序列化成功，节点类型: %d\n", deserialized->type);
    
    // 测试WASM生成
    size_t wasm_size = 0;
    unsigned char *wasm = c2astc_to_wasm(root, &options, &wasm_size);
    if (!wasm) {
        printf("WASM生成失败: %s\n", c2astc_get_error());
        ast_free(deserialized);
        c2astc_free(binary);
        ast_free(root);
        return 0;
    }
    
    printf("WASM生成成功，大小: %zu 字节\n", wasm_size);
    
    // 清理资源
    c2astc_free(wasm);
    ast_free(deserialized);
    c2astc_free(binary);
    ast_free(root);
    
    printf("测试通过!\n\n");
    return 1;
}

int main(int argc, char *argv[]) {
    int passed = 0;
    
    c2astc_print_version();
    printf("\n运行测试...\n\n");
    
    if (argc > 1) {
        // 测试指定文件
        passed += run_test(argv[1]);
    } else {
        // 测试所有文件
        for (int i = 0; i < num_tests; i++) {
            char path[256];
            snprintf(path, sizeof(path), "tests/c2astc_tests/%s", test_files[i]);
            passed += run_test(path);
        }
    }
    
    printf("测试完成: %d/%d 通过\n", passed, argc > 1 ? 1 : num_tests);
    
    return passed == (argc > 1 ? 1 : num_tests) ? 0 : 1;
} 