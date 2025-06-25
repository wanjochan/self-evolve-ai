/**
 * debug_astc_serialization.c - 诊断ASTC序列化/反序列化问题
 * 
 * 比较直接执行AST和序列化后执行的差异
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../runtime.h"
#include "../c2astc.h"

void compare_ast_execution() {
    printf("=== ASTC Serialization Debug ===\n");
    
    // 步骤1: 编译evolver0_program_minimal.c
    printf("Step 1: Compiling evolver0_program_minimal.c...\n");
    struct ASTNode* original_ast = c2astc_convert_file("../evolver0_program_minimal.c", NULL);
    
    if (!original_ast) {
        printf("❌ Failed to compile: %s\n", c2astc_get_error());
        return;
    }
    
    printf("✅ Original AST compiled\n");
    
    // 步骤2: 直接执行原始AST
    printf("\nStep 2: Executing original AST...\n");
    RuntimeVM vm1;
    runtime_init(&vm1);

    int result1 = -999; // 初始化为错误值
    if (runtime_load_program(&vm1, original_ast)) {
        result1 = runtime_execute(&vm1, "main");
        printf("Original AST execution result: %d\n", result1);
    } else {
        printf("❌ Failed to load original AST: %s\n", runtime_get_error(&vm1));
    }
    
    // 步骤3: 序列化AST
    printf("\nStep 3: Serializing AST...\n");
    size_t serialized_size;
    unsigned char* serialized_data = c2astc_serialize(original_ast, &serialized_size);
    
    if (!serialized_data) {
        printf("❌ Failed to serialize: %s\n", c2astc_get_error());
        runtime_destroy(&vm1);
        ast_free(original_ast);
        return;
    }
    
    printf("✅ AST serialized: %zu bytes\n", serialized_size);
    
    // 步骤4: 反序列化AST
    printf("\nStep 4: Deserializing AST...\n");
    struct ASTNode* deserialized_ast = c2astc_deserialize(serialized_data, serialized_size);
    
    if (!deserialized_ast) {
        printf("❌ Failed to deserialize: %s\n", c2astc_get_error());
        free(serialized_data);
        runtime_destroy(&vm1);
        ast_free(original_ast);
        return;
    }
    
    printf("✅ AST deserialized\n");
    
    // 步骤5: 执行反序列化的AST
    printf("\nStep 5: Executing deserialized AST...\n");
    RuntimeVM vm2;
    runtime_init(&vm2);

    int result2 = -999; // 初始化为错误值
    if (runtime_load_program(&vm2, deserialized_ast)) {
        result2 = runtime_execute(&vm2, "main");
        printf("Deserialized AST execution result: %d\n", result2);

        // 比较结果
        printf("\n=== Comparison ===\n");
        printf("Original result: %d\n", result1);
        printf("Deserialized result: %d\n", result2);
        
        if (result1 == result2) {
            printf("✅ Results match - serialization is working correctly\n");
        } else {
            printf("❌ Results differ - serialization has issues\n");
        }
    } else {
        printf("❌ Failed to load deserialized AST: %s\n", runtime_get_error(&vm2));
    }
    
    // 清理
    runtime_destroy(&vm1);
    runtime_destroy(&vm2);
    ast_free(original_ast);
    ast_free(deserialized_ast);
    free(serialized_data);
}

int main() {
    compare_ast_execution();
    return 0;
}
