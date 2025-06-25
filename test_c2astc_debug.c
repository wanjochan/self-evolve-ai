/**
 * test_c2astc_debug.c - 调试c2astc编译evolver0_program.c的问题
 */

#include <stdio.h>
#include <stdlib.h>
#include "c2astc.h"

void print_ast_info(struct ASTNode* node, int depth) {
    if (!node) return;
    
    for (int i = 0; i < depth; i++) printf("  ");
    printf("Node type: %d\n", node->type);
    
    // 递归打印子节点（简化版本）
    if (depth < 3) {  // 限制深度避免过多输出
        // 这里需要根据具体的AST结构来遍历子节点
        // 由于AST结构复杂，这里只是示例
    }
}

int main() {
    printf("=== C2ASTC Debug Test ===\n");
    printf("Testing compilation of evolver0_program.c...\n");
    
    // 测试编译evolver0_program.c
    struct ASTNode* ast = c2astc_convert_file("evolver0_program.c", NULL);
    
    if (!ast) {
        printf("❌ Failed to compile evolver0_program.c\n");
        printf("Error: %s\n", c2astc_get_error());
        return 1;
    }
    
    printf("✅ Successfully compiled evolver0_program.c\n");
    printf("AST root node type: %d\n", ast->type);
    
    // 打印AST结构信息
    print_ast_info(ast, 0);
    
    // 测试序列化
    size_t serialized_size;
    unsigned char* serialized_data = c2astc_serialize(ast, &serialized_size);
    
    if (!serialized_data) {
        printf("❌ Failed to serialize AST\n");
        printf("Error: %s\n", c2astc_get_error());
        ast_free(ast);
        return 1;
    }
    
    printf("✅ Successfully serialized AST\n");
    printf("Serialized size: %zu bytes\n", serialized_size);
    
    // 测试反序列化
    struct ASTNode* deserialized_ast = c2astc_deserialize(serialized_data, serialized_size);
    
    if (!deserialized_ast) {
        printf("❌ Failed to deserialize AST\n");
        printf("Error: %s\n", c2astc_get_error());
        free(serialized_data);
        ast_free(ast);
        return 1;
    }
    
    printf("✅ Successfully deserialized AST\n");
    printf("Deserialized root node type: %d\n", deserialized_ast->type);
    
    // 清理
    ast_free(ast);
    ast_free(deserialized_ast);
    free(serialized_data);
    
    printf("\n=== C2ASTC Debug Test Complete ===\n");
    return 0;
}
