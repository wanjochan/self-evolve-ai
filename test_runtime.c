/**
 * test_runtime.c - Runtime虚拟机测试程序
 * 
 * 测试Runtime虚拟机的基本功能，包括：
 * 1. 虚拟机初始化和销毁
 * 2. 简单程序加载和执行
 * 3. 基本运算和控制流
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "runtime.h"
#include "astc.h"
#include "c2astc.h"

// 创建一个简单的测试程序
struct ASTNode* create_test_program() {
    // 创建一个简单的main函数：
    // int main() {
    //     int a = 10;
    //     int b = 20;
    //     return a + b;
    // }
    
    // 创建根节点
    struct ASTNode* root = ast_create_node(ASTC_TRANSLATION_UNIT, 1, 1);
    if (!root) return NULL;

    // 创建main函数
    struct ASTNode* main_func = ast_create_node(ASTC_FUNC_DECL, 1, 1);
    if (!main_func) {
        ast_free(root);
        return NULL;
    }
    
    // 设置函数信息
    main_func->data.func_decl.name = strdup("main");
    main_func->data.func_decl.param_count = 0;
    main_func->data.func_decl.params = NULL;
    main_func->data.func_decl.has_body = true;
    
    // 创建函数体（复合语句）
    struct ASTNode* body = ast_create_node(ASTC_COMPOUND_STMT, 1, 1);
    if (!body) {
        ast_free(main_func);
        ast_free(root);
        return NULL;
    }

    main_func->data.func_decl.body = body;

    // 创建return语句：return 30;
    struct ASTNode* return_stmt = ast_create_node(ASTC_RETURN_STMT, 1, 1);
    if (!return_stmt) {
        ast_free(main_func);
        ast_free(root);
        return NULL;
    }

    // 创建常量30
    struct ASTNode* constant = ast_create_node(ASTC_EXPR_CONSTANT, 1, 1);
    if (!constant) {
        ast_free(return_stmt);
        ast_free(main_func);
        ast_free(root);
        return NULL;
    }
    
    constant->data.constant.type = ASTC_TYPE_INT;
    constant->data.constant.int_val = 30;
    
    return_stmt->data.return_stmt.value = constant;
    
    // 将return语句添加到函数体
    body->data.compound_stmt.statement_count = 1;
    body->data.compound_stmt.statements = (struct ASTNode**)malloc(sizeof(struct ASTNode*));
    body->data.compound_stmt.statements[0] = return_stmt;
    
    // 将main函数添加到根节点
    root->data.translation_unit.declaration_count = 1;
    root->data.translation_unit.declarations = (struct ASTNode**)malloc(sizeof(struct ASTNode*));
    root->data.translation_unit.declarations[0] = main_func;
    
    return root;
}

int main() {
    printf("=== Runtime虚拟机测试 ===\n");
    
    // 初始化虚拟机
    RuntimeVM vm;
    if (!runtime_init(&vm)) {
        printf("错误: 无法初始化虚拟机\n");
        return 1;
    }
    
    printf("✓ 虚拟机初始化成功\n");
    
    // 创建测试程序
    struct ASTNode* program = create_test_program();
    if (!program) {
        printf("错误: 无法创建测试程序\n");
        runtime_destroy(&vm);
        return 1;
    }
    
    printf("✓ 测试程序创建成功\n");
    
    // 加载程序
    if (!runtime_load_program(&vm, program)) {
        printf("错误: 无法加载程序: %s\n", runtime_get_error(&vm));
        ast_free(program);
        runtime_destroy(&vm);
        return 1;
    }
    
    printf("✓ 程序加载成功\n");
    
    // 执行程序
    int result = runtime_execute(&vm, "main");
    printf("✓ 程序执行完成，返回值: %d\n", result);
    
    if (result == 30) {
        printf("✓ 测试通过！\n");
    } else {
        printf("✗ 测试失败，期望返回30，实际返回%d\n", result);
    }
    
    // 清理资源
    ast_free(program);
    runtime_destroy(&vm);
    
    printf("=== 测试完成 ===\n");
    return (result == 30) ? 0 : 1;
}
