/**
 * evolver0_ast.h - AST节点定义
 */

#ifndef EVOLVER0_AST_H
#define EVOLVER0_AST_H

// AST节点类型
typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_PARAMETER,
    AST_RETURN,
    AST_INTEGER,
    AST_IDENTIFIER,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_COMPOUND,
    AST_DECLARATION,
    AST_ASSIGNMENT,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_EXPRESSION_STMT,
    AST_CALL,
    AST_ARRAY_ACCESS,
    AST_CAST,
    AST_SIZEOF,
    AST_TYPE,
    AST_BREAK,
    AST_CONTINUE
} ASTNodeType;

// AST节点结构
typedef struct ASTNode {
    ASTNodeType type;
    int line;
    int column;
    
    union {
        // 字面量
        long long int_value;
        char *str_value;
        
        // 二元操作
        struct {
            struct ASTNode *left;
            struct ASTNode *right;
            char op;
        } binary;
        
        // 一元操作
        struct {
            struct ASTNode *operand;
            char op;
        } unary;
        
        // 函数
        struct {
            char *name;
            char *return_type;
            struct ASTNode **params;
            int param_count;
            struct ASTNode *body;
        } function;
        
        // 参数
        struct {
            char *type;
            char *name;
        } param;
        
        // 返回语句
        struct {
            struct ASTNode *value;
        } ret;
        
        // 复合语句
        struct {
            struct ASTNode **statements;
            int count;
            int capacity;
        } compound;
        
        // 声明
        struct {
            char *type;
            char *name;
            struct ASTNode *init;
            int is_array;
            int array_size;
        } decl;
        
        // 赋值
        struct {
            struct ASTNode *target;  // 可以是标识符或数组访问
            struct ASTNode *value;
        } assign;
        
        // if语句
        struct {
            struct ASTNode *cond;
            struct ASTNode *then_stmt;
            struct ASTNode *else_stmt;
        } if_stmt;
        
        // while语句
        struct {
            struct ASTNode *cond;
            struct ASTNode *body;
        } while_stmt;
        
        // for语句
        struct {
            struct ASTNode *init;
            struct ASTNode *cond;
            struct ASTNode *inc;
            struct ASTNode *body;
        } for_stmt;
        
        // 函数调用
        struct {
            char *name;
            struct ASTNode **args;
            int arg_count;
        } call;
        
        // 数组访问
        struct {
            struct ASTNode *array;
            struct ASTNode *index;
        } array_access;
        
        // 类型转换
        struct {
            char *target_type;
            struct ASTNode *expr;
        } cast;
        
        // sizeof
        struct {
            char *type_name;
            struct ASTNode *expr;
        } sizeof_expr;
        
        // 类型
        struct {
            char *base_type;
            int pointer_level;
            int is_array;
            int array_size;
        } type;
    } data;
} ASTNode;

// AST创建和释放函数
ASTNode* ast_create_node(ASTNodeType type, int line, int column);
void ast_free(ASTNode *node);
void ast_print(ASTNode *node, int indent);

#endif // EVOLVER0_AST_H