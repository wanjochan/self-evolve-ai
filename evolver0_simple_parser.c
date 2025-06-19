/**
 * evolver0_simple_parser.c - 简化的C语言解析器
 * 用于evolver0第零代编译器
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

// 简化的AST节点类型
typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
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
    AST_CALL
} SimpleNodeType;

// 简化的AST节点
typedef struct SimpleASTNode {
    SimpleNodeType type;
    union {
        long long int_value;
        char *str_value;
        struct {
            struct SimpleASTNode *left;
            struct SimpleASTNode *right;
            char op;  // '+', '-', '*', '/', '<', '>', '=', etc.
        } binary;
        struct {
            struct SimpleASTNode *operand;
            char op;  // '-', '!', '~', etc.
        } unary;
        struct {
            char *name;
            struct SimpleASTNode *params;
            struct SimpleASTNode *body;
        } function;
        struct {
            struct SimpleASTNode *value;
        } ret;
        struct {
            struct SimpleASTNode **statements;
            int count;
        } compound;
        struct {
            char *type;
            char *name;
            struct SimpleASTNode *init;
        } decl;
        struct {
            char *name;
            struct SimpleASTNode *value;
        } assign;
        struct {
            struct SimpleASTNode *cond;
            struct SimpleASTNode *then_stmt;
            struct SimpleASTNode *else_stmt;
        } if_stmt;
        struct {
            struct SimpleASTNode *cond;
            struct SimpleASTNode *body;
        } while_stmt;
        struct {
            struct SimpleASTNode *init;
            struct SimpleASTNode *cond;
            struct SimpleASTNode *inc;
            struct SimpleASTNode *body;
        } for_stmt;
        struct {
            char *name;
            struct SimpleASTNode **args;
            int arg_count;
        } call;
    } data;
} SimpleASTNode;

// Token结构体（如果还没有定义）
#ifndef TOKEN_DEFINED
#define TOKEN_DEFINED
typedef struct {
    int type;
    char *value;
    int line;
    int column;
} Token;
#endif

// 简单的解析器结构体
typedef struct {
    Token *tokens;
    int token_count;
    int current;
    char error_msg[256];
} SimpleParser;

// 创建AST节点
static SimpleASTNode* create_simple_node(SimpleNodeType type) {
    SimpleASTNode *node = (SimpleASTNode*)calloc(1, sizeof(SimpleASTNode));
    if (node) {
        node->type = type;
    }
    return node;
}

// 释放AST节点
static void free_simple_ast(SimpleASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_IDENTIFIER:
            free(node->data.str_value);
            break;
        case AST_BINARY_OP:
            free_simple_ast(node->data.binary.left);
            free_simple_ast(node->data.binary.right);
            break;
        case AST_UNARY_OP:
            free_simple_ast(node->data.unary.operand);
            break;
        case AST_FUNCTION:
            free(node->data.function.name);
            free_simple_ast(node->data.function.params);
            free_simple_ast(node->data.function.body);
            break;
        case AST_RETURN:
            free_simple_ast(node->data.ret.value);
            break;
        case AST_COMPOUND:
            for (int i = 0; i < node->data.compound.count; i++) {
                free_simple_ast(node->data.compound.statements[i]);
            }
            free(node->data.compound.statements);
            break;
        case AST_DECLARATION:
            free(node->data.decl.type);
            free(node->data.decl.name);
            free_simple_ast(node->data.decl.init);
            break;
        case AST_ASSIGNMENT:
            free(node->data.assign.name);
            free_simple_ast(node->data.assign.value);
            break;
        case AST_IF:
            free_simple_ast(node->data.if_stmt.cond);
            free_simple_ast(node->data.if_stmt.then_stmt);
            free_simple_ast(node->data.if_stmt.else_stmt);
            break;
        case AST_WHILE:
            free_simple_ast(node->data.while_stmt.cond);
            free_simple_ast(node->data.while_stmt.body);
            break;
        case AST_FOR:
            free_simple_ast(node->data.for_stmt.init);
            free_simple_ast(node->data.for_stmt.cond);
            free_simple_ast(node->data.for_stmt.inc);
            free_simple_ast(node->data.for_stmt.body);
            break;
        case AST_CALL:
            free(node->data.call.name);
            for (int i = 0; i < node->data.call.arg_count; i++) {
                free_simple_ast(node->data.call.args[i]);
            }
            free(node->data.call.args);
            break;
        default:
            break;
    }
    
    free(node);
}

// 前向声明
static SimpleASTNode* parse_expression(SimpleParser *parser);
static SimpleASTNode* parse_statement(SimpleParser *parser);
static SimpleASTNode* parse_compound_statement(SimpleParser *parser);

// 辅助函数
static int is_at_end(SimpleParser *parser) {
    return parser->current >= parser->token_count;
}

static Token* current_token(SimpleParser *parser) {
    if (is_at_end(parser)) return NULL;
    return &parser->tokens[parser->current];
}

static Token* advance(SimpleParser *parser) {
    if (!is_at_end(parser)) parser->current++;
    return &parser->tokens[parser->current - 1];
}

static int check(SimpleParser *parser, int type) {
    if (is_at_end(parser)) return 0;
    return current_token(parser)->type == type;
}

static int match(SimpleParser *parser, int type) {
    if (check(parser, type)) {
        advance(parser);
        return 1;
    }
    return 0;
}

// 解析主表达式
static SimpleASTNode* parse_primary(SimpleParser *parser) {
    Token *token = current_token(parser);
    if (!token) return NULL;
    
    // 整数字面量
    if (token->type == TOKEN_NUMBER) {
        advance(parser);
        SimpleASTNode *node = create_simple_node(AST_INTEGER);
        node->data.int_value = strtoll(token->value, NULL, 0);
        return node;
    }
    
    // 标识符或函数调用
    if (token->type == TOKEN_IDENTIFIER) {
        char *name = strdup(token->value);
        advance(parser);
        
        // 函数调用
        if (match(parser, TOKEN_LPAREN)) {
            SimpleASTNode *node = create_simple_node(AST_CALL);
            node->data.call.name = name;
            node->data.call.args = NULL;
            node->data.call.arg_count = 0;
            
            // 解析参数
            if (!check(parser, TOKEN_RPAREN)) {
                SimpleASTNode **args = malloc(sizeof(SimpleASTNode*) * 10);
                int count = 0;
                
                do {
                    args[count++] = parse_expression(parser);
                } while (match(parser, TOKEN_COMMA));
                
                node->data.call.args = args;
                node->data.call.arg_count = count;
            }
            
            if (!match(parser, TOKEN_RPAREN)) {
                free(name);
                free_simple_ast(node);
                return NULL;
            }
            
            return node;
        }
        
        // 普通标识符
        SimpleASTNode *node = create_simple_node(AST_IDENTIFIER);
        node->data.str_value = name;
        return node;
    }
    
    // 括号表达式
    if (match(parser, TOKEN_LPAREN)) {
        SimpleASTNode *expr = parse_expression(parser);
        if (!match(parser, TOKEN_RPAREN)) {
            free_simple_ast(expr);
            return NULL;
        }
        return expr;
    }
    
    return NULL;
}

// 解析一元表达式
static SimpleASTNode* parse_unary(SimpleParser *parser) {
    if (match(parser, TOKEN_MINUS) || match(parser, TOKEN_LOGICAL_NOT) || match(parser, TOKEN_BIT_NOT)) {
        Token *op = &parser->tokens[parser->current - 1];
        SimpleASTNode *node = create_simple_node(AST_UNARY_OP);
        node->data.unary.op = op->value[0];
        node->data.unary.operand = parse_unary(parser);
        return node;
    }
    
    return parse_primary(parser);
}

// 解析乘除表达式
static SimpleASTNode* parse_multiplicative(SimpleParser *parser) {
    SimpleASTNode *left = parse_unary(parser);
    
    while (match(parser, TOKEN_MULTIPLY) || match(parser, TOKEN_DIVIDE) || match(parser, TOKEN_MOD)) {
        Token *op = &parser->tokens[parser->current - 1];
        SimpleASTNode *node = create_simple_node(AST_BINARY_OP);
        node->data.binary.left = left;
        node->data.binary.op = op->value[0];
        node->data.binary.right = parse_unary(parser);
        left = node;
    }
    
    return left;
}

// 解析加减表达式
static SimpleASTNode* parse_additive(SimpleParser *parser) {
    SimpleASTNode *left = parse_multiplicative(parser);
    
    while (match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS)) {
        Token *op = &parser->tokens[parser->current - 1];
        SimpleASTNode *node = create_simple_node(AST_BINARY_OP);
        node->data.binary.left = left;
        node->data.binary.op = op->value[0];
        node->data.binary.right = parse_multiplicative(parser);
        left = node;
    }
    
    return left;
}

// 解析关系表达式
static SimpleASTNode* parse_relational(SimpleParser *parser) {
    SimpleASTNode *left = parse_additive(parser);
    
    while (match(parser, TOKEN_LESS) || match(parser, TOKEN_GREATER) || 
           match(parser, TOKEN_LESS_EQUAL) || match(parser, TOKEN_GREATER_EQUAL)) {
        Token *op = &parser->tokens[parser->current - 1];
        SimpleASTNode *node = create_simple_node(AST_BINARY_OP);
        node->data.binary.left = left;
        node->data.binary.op = op->type == TOKEN_LESS_EQUAL ? 'L' : 
                               op->type == TOKEN_GREATER_EQUAL ? 'G' : op->value[0];
        node->data.binary.right = parse_additive(parser);
        left = node;
    }
    
    return left;
}

// 解析相等表达式
static SimpleASTNode* parse_equality(SimpleParser *parser) {
    SimpleASTNode *left = parse_relational(parser);
    
    while (match(parser, TOKEN_EQUAL) || match(parser, TOKEN_NOT_EQUAL)) {
        Token *op = &parser->tokens[parser->current - 1];
        SimpleASTNode *node = create_simple_node(AST_BINARY_OP);
        node->data.binary.left = left;
        node->data.binary.op = op->type == TOKEN_EQUAL ? 'E' : 'N';
        node->data.binary.right = parse_relational(parser);
        left = node;
    }
    
    return left;
}

// 解析赋值表达式
static SimpleASTNode* parse_assignment(SimpleParser *parser) {
    SimpleASTNode *left = parse_equality(parser);
    
    if (match(parser, TOKEN_ASSIGN)) {
        if (left->type != AST_IDENTIFIER) {
            free_simple_ast(left);
            return NULL;
        }
        
        SimpleASTNode *node = create_simple_node(AST_ASSIGNMENT);
        node->data.assign.name = strdup(left->data.str_value);
        node->data.assign.value = parse_assignment(parser);
        free_simple_ast(left);
        return node;
    }
    
    return left;
}

// 解析表达式
static SimpleASTNode* parse_expression(SimpleParser *parser) {
    return parse_assignment(parser);
}

// 解析声明
static SimpleASTNode* parse_declaration(SimpleParser *parser) {
    // 简化：只支持 int 类型
    if (!match(parser, TOKEN_INT)) {
        return NULL;
    }
    
    Token *name_token = current_token(parser);
    if (!name_token || name_token->type != TOKEN_IDENTIFIER) {
        return NULL;
    }
    advance(parser);
    
    SimpleASTNode *node = create_simple_node(AST_DECLARATION);
    node->data.decl.type = strdup("int");
    node->data.decl.name = strdup(name_token->value);
    
    // 可选的初始化
    if (match(parser, TOKEN_ASSIGN)) {
        node->data.decl.init = parse_expression(parser);
    }
    
    match(parser, TOKEN_SEMICOLON);
    return node;
}

// 解析语句
static SimpleASTNode* parse_statement(SimpleParser *parser) {
    // return 语句
    if (match(parser, TOKEN_RETURN)) {
        SimpleASTNode *node = create_simple_node(AST_RETURN);
        if (!check(parser, TOKEN_SEMICOLON)) {
            node->data.ret.value = parse_expression(parser);
        }
        match(parser, TOKEN_SEMICOLON);
        return node;
    }
    
    // if 语句
    if (match(parser, TOKEN_IF)) {
        if (!match(parser, TOKEN_LPAREN)) return NULL;
        
        SimpleASTNode *node = create_simple_node(AST_IF);
        node->data.if_stmt.cond = parse_expression(parser);
        
        if (!match(parser, TOKEN_RPAREN)) {
            free_simple_ast(node);
            return NULL;
        }
        
        node->data.if_stmt.then_stmt = parse_statement(parser);
        
        if (match(parser, TOKEN_ELSE)) {
            node->data.if_stmt.else_stmt = parse_statement(parser);
        }
        
        return node;
    }
    
    // while 语句
    if (match(parser, TOKEN_WHILE)) {
        if (!match(parser, TOKEN_LPAREN)) return NULL;
        
        SimpleASTNode *node = create_simple_node(AST_WHILE);
        node->data.while_stmt.cond = parse_expression(parser);
        
        if (!match(parser, TOKEN_RPAREN)) {
            free_simple_ast(node);
            return NULL;
        }
        
        node->data.while_stmt.body = parse_statement(parser);
        return node;
    }
    
    // for 语句
    if (match(parser, TOKEN_FOR)) {
        if (!match(parser, TOKEN_LPAREN)) return NULL;
        
        SimpleASTNode *node = create_simple_node(AST_FOR);
        
        // 初始化部分
        if (!check(parser, TOKEN_SEMICOLON)) {
            if (check(parser, TOKEN_INT)) {
                node->data.for_stmt.init = parse_declaration(parser);
            } else {
                node->data.for_stmt.init = parse_expression(parser);
                match(parser, TOKEN_SEMICOLON);
            }
        } else {
            match(parser, TOKEN_SEMICOLON);
        }
        
        // 条件部分
        if (!check(parser, TOKEN_SEMICOLON)) {
            node->data.for_stmt.cond = parse_expression(parser);
        }
        match(parser, TOKEN_SEMICOLON);
        
        // 增量部分
        if (!check(parser, TOKEN_RPAREN)) {
            node->data.for_stmt.inc = parse_expression(parser);
        }
        
        if (!match(parser, TOKEN_RPAREN)) {
            free_simple_ast(node);
            return NULL;
        }
        
        node->data.for_stmt.body = parse_statement(parser);
        return node;
    }
    
    // 复合语句
    if (check(parser, TOKEN_LBRACE)) {
        return parse_compound_statement(parser);
    }
    
    // 声明
    if (check(parser, TOKEN_INT)) {
        return parse_declaration(parser);
    }
    
    // 表达式语句
    SimpleASTNode *expr = parse_expression(parser);
    if (expr) {
        match(parser, TOKEN_SEMICOLON);
        SimpleASTNode *node = create_simple_node(AST_EXPRESSION_STMT);
        node->data.ret.value = expr; // 复用 ret 结构
        return node;
    }
    
    return NULL;
}

// 解析复合语句
static SimpleASTNode* parse_compound_statement(SimpleParser *parser) {
    if (!match(parser, TOKEN_LBRACE)) {
        return NULL;
    }
    
    SimpleASTNode *node = create_simple_node(AST_COMPOUND);
    SimpleASTNode **stmts = malloc(sizeof(SimpleASTNode*) * 100);
    int count = 0;
    
    while (!check(parser, TOKEN_RBRACE) && !is_at_end(parser)) {
        SimpleASTNode *stmt = parse_statement(parser);
        if (stmt) {
            stmts[count++] = stmt;
        } else {
            // 跳过错误的语句
            advance(parser);
        }
    }
    
    match(parser, TOKEN_RBRACE);
    
    node->data.compound.statements = stmts;
    node->data.compound.count = count;
    
    return node;
}

// 解析函数
static SimpleASTNode* parse_function(SimpleParser *parser) {
    // 简化：只支持 int 返回类型
    if (!match(parser, TOKEN_INT)) {
        return NULL;
    }
    
    Token *name_token = current_token(parser);
    if (!name_token || name_token->type != TOKEN_IDENTIFIER) {
        return NULL;
    }
    advance(parser);
    
    if (!match(parser, TOKEN_LPAREN)) {
        return NULL;
    }
    
    // 简化：暂时不解析参数
    if (!match(parser, TOKEN_RPAREN)) {
        return NULL;
    }
    
    SimpleASTNode *body = parse_compound_statement(parser);
    if (!body) {
        return NULL;
    }
    
    SimpleASTNode *node = create_simple_node(AST_FUNCTION);
    node->data.function.name = strdup(name_token->value);
    node->data.function.params = NULL;
    node->data.function.body = body;
    
    return node;
}

// 解析程序
static SimpleASTNode* parse_program(SimpleParser *parser) {
    SimpleASTNode *node = create_simple_node(AST_PROGRAM);
    SimpleASTNode **functions = malloc(sizeof(SimpleASTNode*) * 10);
    int count = 0;
    
    while (!is_at_end(parser)) {
        SimpleASTNode *func = parse_function(parser);
        if (func) {
            functions[count++] = func;
        } else {
            // 跳过错误
            advance(parser);
        }
    }
    
    node->data.compound.statements = functions;
    node->data.compound.count = count;
    
    return node;
}

// 主解析函数
SimpleASTNode* parse_simple_c(Token *tokens, int token_count) {
    SimpleParser parser = {
        .tokens = tokens,
        .token_count = token_count,
        .current = 0,
        .error_msg = {0}
    };
    
    return parse_program(&parser);
}

// 打印AST（用于调试）
void print_simple_ast(SimpleASTNode *node, int indent) {
    if (!node) return;
    
    for (int i = 0; i < indent; i++) printf("  ");
    
    switch (node->type) {
        case AST_PROGRAM:
            printf("Program\n");
            for (int i = 0; i < node->data.compound.count; i++) {
                print_simple_ast(node->data.compound.statements[i], indent + 1);
            }
            break;
            
        case AST_FUNCTION:
            printf("Function: %s\n", node->data.function.name);
            print_simple_ast(node->data.function.body, indent + 1);
            break;
            
        case AST_COMPOUND:
            printf("Compound Statement\n");
            for (int i = 0; i < node->data.compound.count; i++) {
                print_simple_ast(node->data.compound.statements[i], indent + 1);
            }
            break;
            
        case AST_RETURN:
            printf("Return\n");
            if (node->data.ret.value) {
                print_simple_ast(node->data.ret.value, indent + 1);
            }
            break;
            
        case AST_INTEGER:
            printf("Integer: %lld\n", node->data.int_value);
            break;
            
        case AST_IDENTIFIER:
            printf("Identifier: %s\n", node->data.str_value);
            break;
            
        case AST_BINARY_OP:
            printf("Binary Op: %c\n", node->data.binary.op);
            print_simple_ast(node->data.binary.left, indent + 1);
            print_simple_ast(node->data.binary.right, indent + 1);
            break;
            
        case AST_UNARY_OP:
            printf("Unary Op: %c\n", node->data.unary.op);
            print_simple_ast(node->data.unary.operand, indent + 1);
            break;
            
        case AST_DECLARATION:
            printf("Declaration: %s %s\n", node->data.decl.type, node->data.decl.name);
            if (node->data.decl.init) {
                print_simple_ast(node->data.decl.init, indent + 1);
            }
            break;
            
        case AST_ASSIGNMENT:
            printf("Assignment: %s =\n", node->data.assign.name);
            print_simple_ast(node->data.assign.value, indent + 1);
            break;
            
        case AST_IF:
            printf("If\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Condition:\n");
            print_simple_ast(node->data.if_stmt.cond, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Then:\n");
            print_simple_ast(node->data.if_stmt.then_stmt, indent + 2);
            if (node->data.if_stmt.else_stmt) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Else:\n");
                print_simple_ast(node->data.if_stmt.else_stmt, indent + 2);
            }
            break;
            
        case AST_WHILE:
            printf("While\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Condition:\n");
            print_simple_ast(node->data.while_stmt.cond, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Body:\n");
            print_simple_ast(node->data.while_stmt.body, indent + 2);
            break;
            
        case AST_FOR:
            printf("For\n");
            if (node->data.for_stmt.init) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Init:\n");
                print_simple_ast(node->data.for_stmt.init, indent + 2);
            }
            if (node->data.for_stmt.cond) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Condition:\n");
                print_simple_ast(node->data.for_stmt.cond, indent + 2);
            }
            if (node->data.for_stmt.inc) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Increment:\n");
                print_simple_ast(node->data.for_stmt.inc, indent + 2);
            }
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Body:\n");
            print_simple_ast(node->data.for_stmt.body, indent + 2);
            break;
            
        case AST_EXPRESSION_STMT:
            printf("Expression Statement\n");
            print_simple_ast(node->data.ret.value, indent + 1);
            break;
            
        case AST_CALL:
            printf("Function Call: %s\n", node->data.call.name);
            for (int i = 0; i < node->data.call.arg_count; i++) {
                print_simple_ast(node->data.call.args[i], indent + 1);
            }
            break;
            
        default:
            printf("Unknown node type: %d\n", node->type);
            break;
    }
}