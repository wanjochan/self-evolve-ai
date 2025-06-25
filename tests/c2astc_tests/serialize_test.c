// 测试ASTC序列化和反序列化功能
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../c2astc.h"
#include "../../astc.h"

// 创建一个简单的二元操作AST节点
struct ASTNode* create_binary_op_test() {
    // 创建一个表达式: (a + b) * c
    // 创建标识符节点a
    struct ASTNode* a = ast_create_node(ASTC_EXPR_IDENTIFIER, 1, 1);
    a->data.identifier.name = strdup("a");
    
    // 创建标识符节点b
    struct ASTNode* b = ast_create_node(ASTC_EXPR_IDENTIFIER, 1, 5);
    b->data.identifier.name = strdup("b");
    
    // 创建标识符节点c
    struct ASTNode* c = ast_create_node(ASTC_EXPR_IDENTIFIER, 1, 11);
    c->data.identifier.name = strdup("c");
    
    // 创建二元操作节点a + b
    struct ASTNode* add = ast_create_node(ASTC_BINARY_OP, 1, 3);
    add->data.binary_op.op = ASTC_OP_ADD;
    add->data.binary_op.left = a;
    add->data.binary_op.right = b;
    
    // 创建二元操作节点(a + b) * c
    struct ASTNode* mul = ast_create_node(ASTC_BINARY_OP, 1, 9);
    mul->data.binary_op.op = ASTC_OP_MUL;
    mul->data.binary_op.left = add;
    mul->data.binary_op.right = c;
    
    return mul;
}

// 创建一个函数调用AST节点
struct ASTNode* create_call_expr_test() {
    // 创建一个函数调用: foo(a, 10, "hello")
    // 创建标识符节点foo
    struct ASTNode* foo = ast_create_node(ASTC_EXPR_IDENTIFIER, 1, 1);
    foo->data.identifier.name = strdup("foo");
    
    // 创建标识符节点a
    struct ASTNode* a = ast_create_node(ASTC_EXPR_IDENTIFIER, 1, 5);
    a->data.identifier.name = strdup("a");
    
    // 创建整数常量节点10
    struct ASTNode* ten = ast_create_node(ASTC_EXPR_CONSTANT, 1, 8);
    ten->data.constant.type = ASTC_TYPE_INT;
    ten->data.constant.int_val = 10;
    
    // 创建字符串字面量节点"hello"
    struct ASTNode* hello = ast_create_node(ASTC_EXPR_STRING_LITERAL, 1, 12);
    hello->data.string_literal.value = strdup("hello");
    
    // 创建函数调用节点foo(a, 10, "hello")
    struct ASTNode* call = ast_create_node(ASTC_CALL_EXPR, 1, 3);
    call->data.call_expr.callee = foo;
    call->data.call_expr.arg_count = 3;
    call->data.call_expr.args = (struct ASTNode**)malloc(3 * sizeof(struct ASTNode*));
    call->data.call_expr.args[0] = a;
    call->data.call_expr.args[1] = ten;
    call->data.call_expr.args[2] = hello;
    
    return call;
}

// 打印AST节点的基本信息
void print_node_info(struct ASTNode* node) {
    if (!node) {
        printf("节点为NULL\n");
        return;
    }
    
    printf("节点类型: %d\n", node->type);
    printf("行号: %d\n", node->line);
    printf("列号: %d\n", node->column);
    
    switch (node->type) {
        case ASTC_EXPR_IDENTIFIER:
            printf("标识符名称: %s\n", node->data.identifier.name);
            break;
            
        case ASTC_EXPR_CONSTANT:
            if (node->data.constant.type == ASTC_TYPE_INT) {
                printf("常量值(整数): %lld\n", node->data.constant.int_val);
            } else {
                printf("常量值(浮点): %f\n", node->data.constant.float_val);
            }
            break;
            
        case ASTC_EXPR_STRING_LITERAL:
            printf("字符串值: %s\n", node->data.string_literal.value);
            break;
            
        case ASTC_BINARY_OP:
            printf("二元操作符: %d\n", node->data.binary_op.op);
            printf("左操作数: ");
            print_node_info(node->data.binary_op.left);
            printf("右操作数: ");
            print_node_info(node->data.binary_op.right);
            break;
            
        case ASTC_UNARY_OP:
            printf("一元操作符: %d\n", node->data.unary_op.op);
            printf("操作数: ");
            print_node_info(node->data.unary_op.operand);
            break;
            
        case ASTC_CALL_EXPR:
            printf("函数调用:\n");
            printf("被调用函数: ");
            print_node_info(node->data.call_expr.callee);
            printf("参数数量: %d\n", node->data.call_expr.arg_count);
            for (int i = 0; i < node->data.call_expr.arg_count; i++) {
                printf("参数 %d: ", i);
                print_node_info(node->data.call_expr.args[i]);
            }
            break;
            
        default:
            printf("其他节点类型\n");
            break;
    }
}

// 比较两个AST节点是否相等
int compare_nodes(struct ASTNode* a, struct ASTNode* b) {
    if (!a && !b) return 1;
    if (!a || !b) return 0;
    
    if (a->type != b->type) return 0;
    
    switch (a->type) {
        case ASTC_EXPR_IDENTIFIER:
            return strcmp(a->data.identifier.name, b->data.identifier.name) == 0;
            
        case ASTC_EXPR_CONSTANT:
            if (a->data.constant.type != b->data.constant.type) return 0;
            if (a->data.constant.type == ASTC_TYPE_INT) {
                return a->data.constant.int_val == b->data.constant.int_val;
            } else {
                return a->data.constant.float_val == b->data.constant.float_val;
            }
            
        case ASTC_EXPR_STRING_LITERAL:
            return strcmp(a->data.string_literal.value, b->data.string_literal.value) == 0;
            
        case ASTC_BINARY_OP:
            if (a->data.binary_op.op != b->data.binary_op.op) return 0;
            if (!compare_nodes(a->data.binary_op.left, b->data.binary_op.left)) return 0;
            return compare_nodes(a->data.binary_op.right, b->data.binary_op.right);
            
        case ASTC_UNARY_OP:
            if (a->data.unary_op.op != b->data.unary_op.op) return 0;
            return compare_nodes(a->data.unary_op.operand, b->data.unary_op.operand);
            
        case ASTC_CALL_EXPR:
            if (!compare_nodes(a->data.call_expr.callee, b->data.call_expr.callee)) return 0;
            if (a->data.call_expr.arg_count != b->data.call_expr.arg_count) return 0;
            for (int i = 0; i < a->data.call_expr.arg_count; i++) {
                if (!compare_nodes(a->data.call_expr.args[i], b->data.call_expr.args[i])) return 0;
            }
            return 1;
            
        default:
            // 对于其他节点类型，只比较基本信息
            return 1;
    }
}

// 测试序列化和反序列化
void test_serialize_deserialize(struct ASTNode* node, const char* test_name) {
    printf("===== 测试 %s =====\n", test_name);
    
    // 打印原始节点信息
    printf("原始节点信息:\n");
    print_node_info(node);
    
    // 序列化
    size_t size;
    unsigned char* binary = c2astc_serialize(node, &size);
    if (!binary) {
        printf("序列化失败: %s\n", c2astc_get_error());
        return;
    }
    
    printf("序列化成功，二进制大小: %zu 字节\n", size);
    
    // 反序列化
    struct ASTNode* deserialized = c2astc_deserialize(binary, size);
    if (!deserialized) {
        printf("反序列化失败: %s\n", c2astc_get_error());
        free(binary);
        return;
    }
    
    printf("反序列化成功\n");
    
    // 打印反序列化后的节点信息
    printf("反序列化后的节点信息:\n");
    print_node_info(deserialized);
    
    // 比较原始节点和反序列化后的节点
    int equal = compare_nodes(node, deserialized);
    printf("比较结果: %s\n", equal ? "相同" : "不同");
    
    // 释放资源
    free(binary);
    ast_free(deserialized);
}

// 测试序列化和反序列化二元操作节点
void test_binary_op_serialization() {
    printf("\n===== 测试二元操作序列化/反序列化 =====\n");
    
    // 创建测试节点
    struct ASTNode* original = create_binary_op_test();
    printf("原始节点:\n");
    print_node_info(original);
    
    // 序列化
    size_t binary_size = 0;
    unsigned char* binary_data = c2astc_serialize(original, &binary_size);
    if (!binary_data) {
        printf("序列化失败!\n");
        ast_free(original);
        return;
    }
    
    printf("\n序列化成功，二进制数据大小: %zu 字节\n", binary_size);
    
    // 反序列化
    struct ASTNode* deserialized = c2astc_deserialize(binary_data, binary_size);
    if (!deserialized) {
        printf("反序列化失败!\n");
        c2astc_free(binary_data);
        ast_free(original);
        return;
    }
    
    printf("\n反序列化后的节点:\n");
    print_node_info(deserialized);
    
    // 释放资源
    c2astc_free(binary_data);
    ast_free(original);
    ast_free(deserialized);
}

// 测试序列化和反序列化函数调用节点
void test_call_expr_serialization() {
    printf("\n===== 测试函数调用序列化/反序列化 =====\n");
    
    // 创建测试节点
    struct ASTNode* original = create_call_expr_test();
    printf("原始节点:\n");
    print_node_info(original);
    
    // 序列化
    size_t binary_size = 0;
    unsigned char* binary_data = c2astc_serialize(original, &binary_size);
    if (!binary_data) {
        printf("序列化失败!\n");
        ast_free(original);
        return;
    }
    
    printf("\n序列化成功，二进制数据大小: %zu 字节\n", binary_size);
    
    // 反序列化
    struct ASTNode* deserialized = c2astc_deserialize(binary_data, binary_size);
    if (!deserialized) {
        printf("反序列化失败!\n");
        c2astc_free(binary_data);
        ast_free(original);
        return;
    }
    
    printf("\n反序列化后的节点:\n");
    print_node_info(deserialized);
    
    // 释放资源
    c2astc_free(binary_data);
    ast_free(original);
    ast_free(deserialized);
}

int main() {
    printf("===== ASTC序列化/反序列化测试程序 =====\n");
    
    // 测试二元操作序列化/反序列化
    test_binary_op_serialization();
    
    // 测试函数调用序列化/反序列化
    test_call_expr_serialization();
    
    printf("\n===== 测试完成 =====\n");
    return 0;
} 