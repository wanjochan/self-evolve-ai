/**
 * c2astc_test.c - C到ASTC转换库的测试程序
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c2astc.h"

// 测试用例
static const char *test_cases[] = {
    // 测试用例1: 简单的Hello World程序
    "int main() {\n"
    "    printf(\"Hello, World!\\n\");\n"
    "    return 0;\n"
    "}\n",
    
    // 测试用例2: 包含基本算术和控制流的程序
    "int sum(int n) {\n"
    "    int result = 0;\n"
    "    for (int i = 1; i <= n; i++) {\n"
    "        result += i;\n"
    "    }\n"
    "    return result;\n"
    "}\n"
    "\n"
    "int factorial(int n) {\n"
    "    if (n <= 1) return 1;\n"
    "    return n * factorial(n-1);\n"
    "}\n"
    "\n"
    "int main() {\n"
    "    int a = 5;\n"
    "    printf(\"Sum of 1 to %d: %d\\n\", a, sum(a));\n"
    "    printf(\"Factorial of %d: %d\\n\", a, factorial(a));\n"
    "    return 0;\n"
    "}\n",
    
    // 测试用例3: 包含结构体和指针的程序
    "typedef struct {\n"
    "    int x;\n"
    "    int y;\n"
    "} Point;\n"
    "\n"
    "void swap(int *a, int *b) {\n"
    "    int temp = *a;\n"
    "    *a = *b;\n"
    "    *b = temp;\n"
    "}\n"
    "\n"
    "Point create_point(int x, int y) {\n"
    "    Point p;\n"
    "    p.x = x;\n"
    "    p.y = y;\n"
    "    return p;\n"
    "}\n"
    "\n"
    "int main() {\n"
    "    int a = 5, b = 10;\n"
    "    swap(&a, &b);\n"
    "    Point p = create_point(a, b);\n"
    "    printf(\"Point: (%d, %d)\\n\", p.x, p.y);\n"
    "    return 0;\n"
    "}\n"
};

// 测试用例数量
#define NUM_TEST_CASES (sizeof(test_cases) / sizeof(test_cases[0]))

/**
 * 打印缩进
 */
static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

/**
 * 打印节点类型名称
 */
static const char* get_node_type_name(ASTNodeType type) {
    // 根据节点类型返回名称
    switch (type) {
        case ASTC_TRANSLATION_UNIT: return "TRANSLATION_UNIT";
        case ASTC_FUNCTION_DEF: return "FUNCTION_DEF";
        case ASTC_FUNCTION_DECL: return "FUNCTION_DECL";
        case ASTC_VAR_DECL: return "VAR_DECL";
        case ASTC_PARAM_DECL: return "PARAM_DECL";
        case ASTC_STRUCT_DECL: return "STRUCT_DECL";
        case ASTC_UNION_DECL: return "UNION_DECL";
        case ASTC_ENUM_DECL: return "ENUM_DECL";
        case ASTC_TYPEDEF_DECL: return "TYPEDEF_DECL";
        case ASTC_PRIMITIVE_TYPE: return "PRIMITIVE_TYPE";
        case ASTC_POINTER_TYPE: return "POINTER_TYPE";
        case ASTC_ARRAY_TYPE: return "ARRAY_TYPE";
        case ASTC_FUNCTION_TYPE: return "FUNCTION_TYPE";
        case ASTC_IF_STMT: return "IF_STMT";
        case ASTC_SWITCH_STMT: return "SWITCH_STMT";
        case ASTC_CASE_STMT: return "CASE_STMT";
        case ASTC_DEFAULT_STMT: return "DEFAULT_STMT";
        case ASTC_WHILE_STMT: return "WHILE_STMT";
        case ASTC_DO_STMT: return "DO_STMT";
        case ASTC_FOR_STMT: return "FOR_STMT";
        case ASTC_GOTO_STMT: return "GOTO_STMT";
        case ASTC_LABEL_STMT: return "LABEL_STMT";
        case ASTC_CONTINUE_STMT: return "CONTINUE_STMT";
        case ASTC_BREAK_STMT: return "BREAK_STMT";
        case ASTC_RETURN_STMT: return "RETURN_STMT";
        case ASTC_IDENTIFIER: return "IDENTIFIER";
        case ASTC_CONSTANT: return "CONSTANT";
        case ASTC_STRING_LITERAL: return "STRING_LITERAL";
        case ASTC_UNARY_OP: return "UNARY_OP";
        case ASTC_BINARY_OP: return "BINARY_OP";
        case ASTC_TERNARY_OP: return "TERNARY_OP";
        case ASTC_CALL_EXPR: return "CALL_EXPR";
        case ASTC_ARRAY_SUBSCRIPT: return "ARRAY_SUBSCRIPT";
        case ASTC_MEMBER_ACCESS: return "MEMBER_ACCESS";
        case ASTC_PTR_MEMBER_ACCESS: return "PTR_MEMBER_ACCESS";
        case ASTC_CAST_EXPR: return "CAST_EXPR";
        case ASTC_EXPR_IDENTIFIER: return "EXPR_IDENTIFIER";
        case ASTC_EXPR_CONSTANT: return "EXPR_CONSTANT";
        case ASTC_EXPR_STRING_LITERAL: return "EXPR_STRING_LITERAL";
        case ASTC_EXPR_COMPOUND_LITERAL: return "EXPR_COMPOUND_LITERAL";
        case ASTC_EXPR_FUNC_CALL: return "EXPR_FUNC_CALL";
        default: return "UNKNOWN";
    }
}

/**
 * 自定义打印ASTC节点
 */
static void print_astc_node(struct ASTNode *node, int indent) {
    if (!node) return;
    
    print_indent(indent);
    printf("%s", get_node_type_name(node->type));
    
    // 根据节点类型打印相关信息
    switch (node->type) {
        case ASTC_EXPR_IDENTIFIER:
            printf(" (name: %s)", node->data.identifier.name);
            break;
        case ASTC_EXPR_CONSTANT:
            if (node->data.constant.type == ASTC_TYPE_INT) {
                printf(" (value: %ld)", (long)node->data.constant.int_val);
            } else if (node->data.constant.type == ASTC_TYPE_FLOAT) {
                printf(" (value: %f)", node->data.constant.float_val);
            }
            break;
        case ASTC_EXPR_STRING_LITERAL:
            printf(" (value: \"%s\")", node->data.string_literal.value);
            break;
        case ASTC_FUNCTION_DEF:
            printf(" (name: %s)", node->data.function_def.name);
            break;
        case ASTC_VAR_DECL:
            printf(" (name: %s)", node->data.var_decl.name);
            break;
        default:
            break;
    }
    
    printf("\n");
    
    // 递归打印子节点
    // 注意：这里需要根据不同节点类型处理不同的子节点结构
    // 以下代码只是示例，实际实现需要根据ASTC节点结构进行调整
    /*
    switch (node->type) {
        case ASTC_BINARY_OP:
            print_astc_node(node->data.binary_op.left, indent + 1);
            print_astc_node(node->data.binary_op.right, indent + 1);
            break;
        case ASTC_UNARY_OP:
            print_astc_node(node->data.unary_op.operand, indent + 1);
            break;
        case ASTC_FUNCTION_DEF:
            print_astc_node(node->data.function_def.return_type, indent + 1);
            for (int i = 0; i < node->data.function_def.param_count; i++) {
                print_astc_node(node->data.function_def.params[i], indent + 1);
            }
            print_astc_node(node->data.function_def.body, indent + 1);
            break;
        // ... 其他节点类型处理
        default:
            break;
    }
    */
}

/**
 * 测试WASM序列化
 */
static void test_wasm_output(struct ASTNode *node) {
    printf("\n=== 测试WASM生成 ===\n");
    
    C2AstcOptions options = c2astc_default_options();
    options.enable_extensions = true;
    
    size_t wasm_size = 0;
    unsigned char *wasm_binary = c2astc_to_wasm(node, &options, &wasm_size);
    
    if (wasm_binary) {
        printf("生成WASM成功，大小: %zu 字节\n", wasm_size);
        
        printf("WASM二进制头部: ");
        const int show_bytes = wasm_size > 16 ? 16 : wasm_size;
        for (int i = 0; i < show_bytes; i++) {
            printf("%02x ", wasm_binary[i]);
        }
        printf("\n");
        
        c2astc_free(wasm_binary);
    } else {
        printf("生成WASM失败: %s\n", c2astc_get_error());
    }
}

/**
 * 运行测试用例
 */
static void run_test_case(int index, const char *source) {
    printf("\n===== 测试用例 #%d =====\n", index + 1);
    printf("源代码:\n%s\n", source);
    
    C2AstcOptions options = c2astc_default_options();
    struct ASTNode *root = c2astc_convert(source, &options);
    
    if (root) {
        printf("转换成功！输出ASTC结构:\n");
        print_astc_node(root, 0);
        
        // 测试WASM输出
        test_wasm_output(root);
        
        // 测试序列化/反序列化
        printf("\n=== 测试序列化/反序列化 ===\n");
        size_t binary_size = 0;
        unsigned char *binary = c2astc_serialize(root, &binary_size);
        
        if (binary) {
            printf("序列化成功，大小: %zu 字节\n", binary_size);
            struct ASTNode *deserialized = c2astc_deserialize(binary, binary_size);
            
            if (deserialized) {
                printf("反序列化成功！输出反序列化后的ASTC结构:\n");
                print_astc_node(deserialized, 0);
                
                // 释放资源
                ast_free(deserialized);
            } else {
                printf("反序列化失败: %s\n", c2astc_get_error());
            }
            
            c2astc_free(binary);
        } else {
            printf("序列化失败: %s\n", c2astc_get_error());
        }
        
        ast_free(root);
    } else {
        printf("转换失败: %s\n", c2astc_get_error());
    }
}

/**
 * 主函数
 */
int main(int argc, char *argv[]) {
    c2astc_print_version();
    
    printf("\n===== 运行内建测试用例 =====\n");
    
    for (int i = 0; i < NUM_TEST_CASES; i++) {
        run_test_case(i, test_cases[i]);
    }
    
    // 如果命令行提供了C源文件，也测试它
    if (argc > 1) {
        printf("\n===== 测试文件 %s =====\n", argv[1]);
        
        C2AstcOptions options = c2astc_default_options();
        struct ASTNode *root = c2astc_convert_file(argv[1], &options);
        
        if (root) {
            printf("转换成功！输出ASTC结构:\n");
            print_astc_node(root, 0);
            ast_free(root);
        } else {
            printf("转换失败: %s\n", c2astc_get_error());
        }
    }
    
    return 0;
} 