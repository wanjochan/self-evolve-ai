/**
 * debug_runtime_load.c - 调试Runtime加载Program的问题
 */

#include <stdio.h>
#include <stdlib.h>
#include "runtime.h"
#include "c2astc.h"

void debug_ast_structure(struct ASTNode* node, int depth) {
    if (!node) {
        printf("NULL node\n");
        return;
    }
    
    for (int i = 0; i < depth; i++) printf("  ");
    printf("Node type: %d", node->type);
    
    if (node->type == ASTC_TRANSLATION_UNIT) {
        printf(" (TRANSLATION_UNIT, declarations: %d)", node->data.translation_unit.declaration_count);
    } else if (node->type == ASTC_FUNC_DECL) {
        printf(" (FUNC_DECL, name: %s)", node->data.func_decl.name ? node->data.func_decl.name : "NULL");
    } else if (node->type == ASTC_VAR_DECL) {
        printf(" (VAR_DECL, name: %s)", node->data.var_decl.name ? node->data.var_decl.name : "NULL");
    }
    
    printf("\n");
    
    // 递归打印子节点
    if (depth < 2) {  // 限制深度
        if (node->type == ASTC_TRANSLATION_UNIT) {
            for (int i = 0; i < node->data.translation_unit.declaration_count; i++) {
                debug_ast_structure(node->data.translation_unit.declarations[i], depth + 1);
            }
        }
    }
}

int main() {
    printf("=== Runtime Load Debug ===\n");
    
    // 步骤1: 编译evolver0_program.c
    printf("Step 1: Compiling evolver0_program.c...\n");
    struct ASTNode* ast = c2astc_convert_file("../evolver0_program.c", NULL);
    
    if (!ast) {
        printf("❌ Failed to compile: %s\n", c2astc_get_error());
        return 1;
    }
    
    printf("✅ Compilation successful\n");
    
    // 步骤2: 分析AST结构
    printf("\nStep 2: Analyzing AST structure...\n");
    debug_ast_structure(ast, 0);
    
    // 步骤3: 初始化Runtime
    printf("\nStep 3: Initializing Runtime...\n");
    RuntimeVM vm;
    if (!runtime_init(&vm)) {
        printf("❌ Failed to initialize Runtime\n");
        ast_free(ast);
        return 1;
    }
    
    printf("✅ Runtime initialized\n");
    
    // 步骤4: 加载程序到Runtime
    printf("\nStep 4: Loading program to Runtime...\n");
    if (!runtime_load_program(&vm, ast)) {
        printf("❌ Failed to load program: %s\n", runtime_get_error(&vm));
        runtime_destroy(&vm);
        ast_free(ast);
        return 1;
    }
    
    printf("✅ Program loaded successfully\n");
    printf("Functions loaded: %zu\n", vm.functions.count);
    
    // 步骤5: 列出加载的函数
    printf("\nStep 5: Listing loaded functions...\n");
    for (size_t i = 0; i < vm.functions.count; i++) {
        printf("  Function %zu: %s\n", i, vm.functions.entries[i].name);
    }
    
    // 步骤6: 查找main函数
    printf("\nStep 6: Looking for main function...\n");
    RuntimeFunctionEntry* main_func = NULL;
    for (size_t i = 0; i < vm.functions.count; i++) {
        if (strcmp(vm.functions.entries[i].name, "main") == 0) {
            main_func = &vm.functions.entries[i];
            break;
        }
    }
    
    if (main_func) {
        printf("✅ Found main function\n");
        
        // 步骤7: 尝试执行main函数
        printf("\nStep 7: Executing main function...\n");
        int result = runtime_execute(&vm, "main");
        printf("Execution result: %d\n", result);
        
        if (result == -1) {
            printf("Error: %s\n", runtime_get_error(&vm));
        }
    } else {
        printf("❌ main function not found\n");
    }
    
    // 清理
    runtime_destroy(&vm);
    ast_free(ast);
    
    printf("\n=== Debug Complete ===\n");
    return 0;
}
