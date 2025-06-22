/**
 * evolver0_ast.h - AST节点定义
 */

#ifndef EVOLVER0_AST_H
#define EVOLVER0_AST_H

// AST节点类型
#ifndef EVOLVER0_AST_NODE_TYPE_DEFINED
#define EVOLVER0_AST_NODE_TYPE_DEFINED
typedef enum {
    // 基础节点
    AST_TRANSLATION_UNIT,
    AST_FUNCTION_DEF,
    AST_FUNCTION_DECL,
    AST_PARAM_DECL,
    AST_VAR_DECL,
    AST_TYPE_NAME,
    
    // 语句
    AST_COMPOUND_STMT,
    AST_EXPRESSION_STMT,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_DO_WHILE_STMT,
    AST_FOR_STMT,
    AST_RETURN_STMT,
    AST_BREAK_STMT,
    AST_CONTINUE_STMT,
    AST_GOTO_STMT,
    AST_LABEL_STMT,
    
    // 表达式
    AST_INTEGER_LITERAL,
    AST_STRING_LITERAL,
    AST_CHAR_LITERAL,
    AST_IDENTIFIER,
    AST_BINARY_EXPR,
    AST_UNARY_EXPR,
    AST_ASSIGNMENT_EXPR,
    AST_CALL_EXPR,
    AST_ARRAY_SUBSCRIPT_EXPR,
    AST_MEMBER_EXPR,
    AST_POST_INCREMENT_EXPR,
    AST_POST_DECREMENT_EXPR,
    AST_CAST_EXPR,
    AST_SIZEOF_EXPR,
    AST_CONDITIONAL_EXPR,
    
    // 旧的兼容类型
    AST_PROGRAM,
    AST_FUNCTION,
    AST_PARAMETER,
    AST_RETURN,
    AST_INTEGER,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_COMPOUND,
    AST_DECLARATION,
    AST_ASSIGNMENT,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_CALL,
    AST_ARRAY_ACCESS,
    AST_CAST,
    AST_SIZEOF,
    AST_TYPE,
    AST_BREAK,
    AST_CONTINUE
} ASTNodeType;
#endif

// AST创建和释放函数声明
struct ASTNode;
struct ASTNode* ast_create_node(ASTNodeType type, int line, int column);
void ast_free(struct ASTNode *node);
void ast_print(struct ASTNode *node, int indent);

#endif // EVOLVER0_AST_H