/**
 * debug_ast_structure.c - 详细分析AST结构
 * 
 * 分析evolver0_program.c的AST结构，找出缺少序列化的节点类型
 */

#include <stdio.h>
#include <stdlib.h>
#include "../runtime.h"
#include "../c2astc.h"

void print_ast_detailed(struct ASTNode* node, int depth) {
    if (!node) {
        for (int i = 0; i < depth; i++) printf("  ");
        printf("NULL node\n");
        return;
    }
    
    for (int i = 0; i < depth; i++) printf("  ");
    printf("Node type: %d", node->type);
    
    switch (node->type) {
        case ASTC_TRANSLATION_UNIT:
            printf(" (TRANSLATION_UNIT, declarations: %d)\n", node->data.translation_unit.declaration_count);
            for (int i = 0; i < node->data.translation_unit.declaration_count && i < 5; i++) {
                print_ast_detailed(node->data.translation_unit.declarations[i], depth + 1);
            }
            break;
            
        case ASTC_FUNC_DECL:
            printf(" (FUNC_DECL, name: %s, has_body: %s)\n", 
                   node->data.func_decl.name ? node->data.func_decl.name : "NULL",
                   node->data.func_decl.has_body ? "yes" : "no");
            if (node->data.func_decl.body && depth < 3) {
                printf("    Function body:\n");
                print_ast_detailed(node->data.func_decl.body, depth + 2);
            }
            break;
            
        case ASTC_COMPOUND_STMT:
            printf(" (COMPOUND_STMT, statements: %d)\n", node->data.compound_stmt.statement_count);
            for (int i = 0; i < node->data.compound_stmt.statement_count && i < 3; i++) {
                print_ast_detailed(node->data.compound_stmt.statements[i], depth + 1);
            }
            break;
            
        case ASTC_RETURN_STMT:
            printf(" (RETURN_STMT)\n");
            if (node->data.return_stmt.value && depth < 4) {
                printf("    Return value:\n");
                print_ast_detailed(node->data.return_stmt.value, depth + 2);
            }
            break;
            
        case ASTC_CALL_EXPR:
            printf(" (CALL_EXPR, args: %d)\n", node->data.call_expr.arg_count);
            break;
            
        case ASTC_VAR_DECL:
            printf(" (VAR_DECL, name: %s)\n",
                   node->data.var_decl.name ? node->data.var_decl.name : "NULL");
            break;

        case ASTC_IF_STMT:
            printf(" (IF_STMT)\n");
            break;

        case ASTC_EXPR_STMT:
            printf(" (EXPR_STMT)\n");
            break;

        default:
            printf(" (UNKNOWN TYPE: %d)\n", node->type);
            break;
    }
}

int main() {
    printf("=== AST Structure Analysis ===\n");
    
    // 编译evolver0_program.c
    struct ASTNode* ast = c2astc_convert_file("../evolver0_program.c", NULL);
    
    if (!ast) {
        printf("❌ Failed to compile: %s\n", c2astc_get_error());
        return 1;
    }
    
    printf("✅ AST compiled successfully\n");
    printf("\nDetailed AST structure:\n");
    print_ast_detailed(ast, 0);
    
    // 清理
    ast_free(ast);
    
    return 0;
}
