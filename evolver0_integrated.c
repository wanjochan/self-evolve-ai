/**
 * evolver0_integrated.c - 整合版的evolver0编译器
 * 包含了简化的解析器和x86-64代码生成器
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <elf.h>

// ====================================
// 基础定义
// ====================================

#define MAX_TOKENS 10000
#define MAX_MACHINE_CODE 65536

// Token类型定义（简化版）
typedef enum {
    TOKEN_EOF = 0,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    
    // 关键字
    TOKEN_INT, TOKEN_RETURN, TOKEN_IF, TOKEN_ELSE,
    TOKEN_WHILE, TOKEN_FOR, TOKEN_DO, TOKEN_BREAK, TOKEN_CONTINUE,
    
    // 操作符
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULTIPLY, TOKEN_DIVIDE, TOKEN_MOD,
    TOKEN_ASSIGN, TOKEN_EQUAL, TOKEN_NOT_EQUAL,
    TOKEN_LESS, TOKEN_GREATER, TOKEN_LESS_EQUAL, TOKEN_GREATER_EQUAL,
    TOKEN_LOGICAL_AND, TOKEN_LOGICAL_OR, TOKEN_LOGICAL_NOT,
    TOKEN_BIT_AND, TOKEN_BIT_OR, TOKEN_BIT_XOR, TOKEN_BIT_NOT,
    TOKEN_INCREMENT, TOKEN_DECREMENT,
    
    // 标点
    TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_LBRACE, TOKEN_RBRACE,
    TOKEN_LBRACKET, TOKEN_RBRACKET, TOKEN_SEMICOLON, TOKEN_COMMA,
    
    // 特殊
    TOKEN_UNKNOWN
} TokenType;

// Token结构体
typedef struct {
    TokenType type;
    char *value;
    int line;
    int column;
} Token;

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
} ASTNodeType;

// 简化的AST节点
typedef struct ASTNode {
    ASTNodeType type;
    union {
        long long int_value;
        char *str_value;
        struct {
            struct ASTNode *left;
            struct ASTNode *right;
            char op;
        } binary;
        struct {
            struct ASTNode *operand;
            char op;
        } unary;
        struct {
            char *name;
            struct ASTNode *params;
            struct ASTNode *body;
        } function;
        struct {
            struct ASTNode *value;
        } ret;
        struct {
            struct ASTNode **statements;
            int count;
        } compound;
        struct {
            char *type;
            char *name;
            struct ASTNode *init;
        } decl;
        struct {
            char *name;
            struct ASTNode *value;
        } assign;
        struct {
            struct ASTNode *cond;
            struct ASTNode *then_stmt;
            struct ASTNode *else_stmt;
        } if_stmt;
        struct {
            struct ASTNode *cond;
            struct ASTNode *body;
        } while_stmt;
        struct {
            struct ASTNode *init;
            struct ASTNode *cond;
            struct ASTNode *inc;
            struct ASTNode *body;
        } for_stmt;
        struct {
            char *name;
            struct ASTNode **args;
            int arg_count;
        } call;
    } data;
} ASTNode;

// ====================================
// 词法分析器
// ====================================

typedef struct {
    const char *source;
    size_t pos;
    size_t length;
    int line;
    int column;
} Lexer;

static void init_lexer(Lexer *lexer, const char *source) {
    lexer->source = source;
    lexer->pos = 0;
    lexer->length = strlen(source);
    lexer->line = 1;
    lexer->column = 1;
}

static void skip_whitespace(Lexer *lexer) {
    while (lexer->pos < lexer->length) {
        char c = lexer->source[lexer->pos];
        if (c == ' ' || c == '\t' || c == '\r') {
            lexer->pos++;
            lexer->column++;
        } else if (c == '\n') {
            lexer->pos++;
            lexer->line++;
            lexer->column = 1;
        } else if (c == '/' && lexer->pos + 1 < lexer->length) {
            if (lexer->source[lexer->pos + 1] == '/') {
                // 单行注释
                lexer->pos += 2;
                while (lexer->pos < lexer->length && lexer->source[lexer->pos] != '\n') {
                    lexer->pos++;
                }
            } else if (lexer->source[lexer->pos + 1] == '*') {
                // 多行注释
                lexer->pos += 2;
                while (lexer->pos + 1 < lexer->length) {
                    if (lexer->source[lexer->pos] == '*' && lexer->source[lexer->pos + 1] == '/') {
                        lexer->pos += 2;
                        break;
                    }
                    if (lexer->source[lexer->pos] == '\n') {
                        lexer->line++;
                        lexer->column = 1;
                    } else {
                        lexer->column++;
                    }
                    lexer->pos++;
                }
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

static Token* tokenize(const char *source, int *token_count) {
    Lexer lexer;
    init_lexer(&lexer, source);
    
    Token *tokens = malloc(sizeof(Token) * MAX_TOKENS);
    int count = 0;
    
    while (lexer.pos < lexer.length && count < MAX_TOKENS - 1) {
        skip_whitespace(&lexer);
        if (lexer.pos >= lexer.length) break;
        
        Token *token = &tokens[count];
        token->line = lexer.line;
        token->column = lexer.column;
        
        char c = lexer.source[lexer.pos];
        
        // 标识符或关键字
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
            size_t start = lexer.pos;
            while (lexer.pos < lexer.length) {
                c = lexer.source[lexer.pos];
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
                    (c >= '0' && c <= '9') || c == '_') {
                    lexer.pos++;
                    lexer.column++;
                } else {
                    break;
                }
            }
            
            size_t len = lexer.pos - start;
            char *value = malloc(len + 1);
            memcpy(value, lexer.source + start, len);
            value[len] = '\0';
            
            // 检查关键字
            if (strcmp(value, "int") == 0) token->type = TOKEN_INT;
            else if (strcmp(value, "return") == 0) token->type = TOKEN_RETURN;
            else if (strcmp(value, "if") == 0) token->type = TOKEN_IF;
            else if (strcmp(value, "else") == 0) token->type = TOKEN_ELSE;
            else if (strcmp(value, "while") == 0) token->type = TOKEN_WHILE;
            else if (strcmp(value, "for") == 0) token->type = TOKEN_FOR;
            else if (strcmp(value, "do") == 0) token->type = TOKEN_DO;
            else if (strcmp(value, "break") == 0) token->type = TOKEN_BREAK;
            else if (strcmp(value, "continue") == 0) token->type = TOKEN_CONTINUE;
            else token->type = TOKEN_IDENTIFIER;
            
            token->value = value;
            count++;
        }
        // 数字
        else if (c >= '0' && c <= '9') {
            size_t start = lexer.pos;
            while (lexer.pos < lexer.length && lexer.source[lexer.pos] >= '0' && lexer.source[lexer.pos] <= '9') {
                lexer.pos++;
                lexer.column++;
            }
            
            size_t len = lexer.pos - start;
            char *value = malloc(len + 1);
            memcpy(value, lexer.source + start, len);
            value[len] = '\0';
            
            token->type = TOKEN_NUMBER;
            token->value = value;
            count++;
        }
        // 字符串
        else if (c == '"') {
            lexer.pos++; // 跳过开始的引号
            size_t start = lexer.pos;
            
            while (lexer.pos < lexer.length && lexer.source[lexer.pos] != '"') {
                if (lexer.source[lexer.pos] == '\\' && lexer.pos + 1 < lexer.length) {
                    lexer.pos += 2; // 跳过转义字符
                } else {
                    lexer.pos++;
                }
            }
            
            size_t len = lexer.pos - start;
            char *value = malloc(len + 1);
            memcpy(value, lexer.source + start, len);
            value[len] = '\0';
            
            if (lexer.pos < lexer.length) {
                lexer.pos++; // 跳过结束的引号
            }
            
            token->type = TOKEN_STRING;
            token->value = value;
            count++;
        }
        // 操作符和标点
        else {
            char next = (lexer.pos + 1 < lexer.length) ? lexer.source[lexer.pos + 1] : '\0';
            
            switch (c) {
                case '+':
                    if (next == '+') {
                        token->type = TOKEN_INCREMENT;
                        token->value = strdup("++");
                        lexer.pos += 2;
                    } else {
                        token->type = TOKEN_PLUS;
                        token->value = strdup("+");
                        lexer.pos++;
                    }
                    count++;
                    break;
                    
                case '-':
                    if (next == '-') {
                        token->type = TOKEN_DECREMENT;
                        token->value = strdup("--");
                        lexer.pos += 2;
                    } else {
                        token->type = TOKEN_MINUS;
                        token->value = strdup("-");
                        lexer.pos++;
                    }
                    count++;
                    break;
                    
                case '*':
                    token->type = TOKEN_MULTIPLY;
                    token->value = strdup("*");
                    lexer.pos++;
                    count++;
                    break;
                    
                case '/':
                    token->type = TOKEN_DIVIDE;
                    token->value = strdup("/");
                    lexer.pos++;
                    count++;
                    break;
                    
                case '%':
                    token->type = TOKEN_MOD;
                    token->value = strdup("%");
                    lexer.pos++;
                    count++;
                    break;
                    
                case '=':
                    if (next == '=') {
                        token->type = TOKEN_EQUAL;
                        token->value = strdup("==");
                        lexer.pos += 2;
                    } else {
                        token->type = TOKEN_ASSIGN;
                        token->value = strdup("=");
                        lexer.pos++;
                    }
                    count++;
                    break;
                    
                case '!':
                    if (next == '=') {
                        token->type = TOKEN_NOT_EQUAL;
                        token->value = strdup("!=");
                        lexer.pos += 2;
                    } else {
                        token->type = TOKEN_LOGICAL_NOT;
                        token->value = strdup("!");
                        lexer.pos++;
                    }
                    count++;
                    break;
                    
                case '<':
                    if (next == '=') {
                        token->type = TOKEN_LESS_EQUAL;
                        token->value = strdup("<=");
                        lexer.pos += 2;
                    } else {
                        token->type = TOKEN_LESS;
                        token->value = strdup("<");
                        lexer.pos++;
                    }
                    count++;
                    break;
                    
                case '>':
                    if (next == '=') {
                        token->type = TOKEN_GREATER_EQUAL;
                        token->value = strdup(">=");
                        lexer.pos += 2;
                    } else {
                        token->type = TOKEN_GREATER;
                        token->value = strdup(">");
                        lexer.pos++;
                    }
                    count++;
                    break;
                    
                case '&':
                    if (next == '&') {
                        token->type = TOKEN_LOGICAL_AND;
                        token->value = strdup("&&");
                        lexer.pos += 2;
                    } else {
                        token->type = TOKEN_BIT_AND;
                        token->value = strdup("&");
                        lexer.pos++;
                    }
                    count++;
                    break;
                    
                case '|':
                    if (next == '|') {
                        token->type = TOKEN_LOGICAL_OR;
                        token->value = strdup("||");
                        lexer.pos += 2;
                    } else {
                        token->type = TOKEN_BIT_OR;
                        token->value = strdup("|");
                        lexer.pos++;
                    }
                    count++;
                    break;
                    
                case '^':
                    token->type = TOKEN_BIT_XOR;
                    token->value = strdup("^");
                    lexer.pos++;
                    count++;
                    break;
                    
                case '~':
                    token->type = TOKEN_BIT_NOT;
                    token->value = strdup("~");
                    lexer.pos++;
                    count++;
                    break;
                    
                case '(':
                    token->type = TOKEN_LPAREN;
                    token->value = strdup("(");
                    lexer.pos++;
                    count++;
                    break;
                    
                case ')':
                    token->type = TOKEN_RPAREN;
                    token->value = strdup(")");
                    lexer.pos++;
                    count++;
                    break;
                    
                case '{':
                    token->type = TOKEN_LBRACE;
                    token->value = strdup("{");
                    lexer.pos++;
                    count++;
                    break;
                    
                case '}':
                    token->type = TOKEN_RBRACE;
                    token->value = strdup("}");
                    lexer.pos++;
                    count++;
                    break;
                    
                case '[':
                    token->type = TOKEN_LBRACKET;
                    token->value = strdup("[");
                    lexer.pos++;
                    count++;
                    break;
                    
                case ']':
                    token->type = TOKEN_RBRACKET;
                    token->value = strdup("]");
                    lexer.pos++;
                    count++;
                    break;
                    
                case ';':
                    token->type = TOKEN_SEMICOLON;
                    token->value = strdup(";");
                    lexer.pos++;
                    count++;
                    break;
                    
                case ',':
                    token->type = TOKEN_COMMA;
                    token->value = strdup(",");
                    lexer.pos++;
                    count++;
                    break;
                    
                default:
                    token->type = TOKEN_UNKNOWN;
                    token->value = malloc(2);
                    token->value[0] = c;
                    token->value[1] = '\0';
                    lexer.pos++;
                    count++;
                    break;
            }
        }
    }
    
    // 添加EOF token
    tokens[count].type = TOKEN_EOF;
    tokens[count].value = strdup("");
    tokens[count].line = lexer.line;
    tokens[count].column = lexer.column;
    count++;
    
    *token_count = count;
    return tokens;
}

// 释放tokens
static void free_tokens(Token *tokens, int count) {
    for (int i = 0; i < count; i++) {
        free(tokens[i].value);
    }
    free(tokens);
}

// ====================================
// 解析器
// ====================================

typedef struct {
    Token *tokens;
    int token_count;
    int current;
    char error_msg[256];
} Parser;

// 创建AST节点
static ASTNode* create_node(ASTNodeType type) {
    ASTNode *node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (node) {
        node->type = type;
    }
    return node;
}

// 释放AST节点
static void free_ast(ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_IDENTIFIER:
            free(node->data.str_value);
            break;
        case AST_BINARY_OP:
            free_ast(node->data.binary.left);
            free_ast(node->data.binary.right);
            break;
        case AST_UNARY_OP:
            free_ast(node->data.unary.operand);
            break;
        case AST_FUNCTION:
            free(node->data.function.name);
            free_ast(node->data.function.params);
            free_ast(node->data.function.body);
            break;
        case AST_RETURN:
            free_ast(node->data.ret.value);
            break;
        case AST_COMPOUND:
            for (int i = 0; i < node->data.compound.count; i++) {
                free_ast(node->data.compound.statements[i]);
            }
            free(node->data.compound.statements);
            break;
        case AST_DECLARATION:
            free(node->data.decl.type);
            free(node->data.decl.name);
            free_ast(node->data.decl.init);
            break;
        case AST_ASSIGNMENT:
            free(node->data.assign.name);
            free_ast(node->data.assign.value);
            break;
        case AST_IF:
            free_ast(node->data.if_stmt.cond);
            free_ast(node->data.if_stmt.then_stmt);
            free_ast(node->data.if_stmt.else_stmt);
            break;
        case AST_WHILE:
            free_ast(node->data.while_stmt.cond);
            free_ast(node->data.while_stmt.body);
            break;
        case AST_FOR:
            free_ast(node->data.for_stmt.init);
            free_ast(node->data.for_stmt.cond);
            free_ast(node->data.for_stmt.inc);
            free_ast(node->data.for_stmt.body);
            break;
        case AST_CALL:
            free(node->data.call.name);
            for (int i = 0; i < node->data.call.arg_count; i++) {
                free_ast(node->data.call.args[i]);
            }
            free(node->data.call.args);
            break;
        default:
            break;
    }
    
    free(node);
}

// 前向声明
static ASTNode* parse_expression(Parser *parser);
static ASTNode* parse_statement(Parser *parser);
static ASTNode* parse_compound_statement(Parser *parser);

// 解析辅助函数
static int is_at_end(Parser *parser) {
    return parser->current >= parser->token_count;
}

static Token* current_token(Parser *parser) {
    if (is_at_end(parser)) return NULL;
    return &parser->tokens[parser->current];
}

static Token* advance(Parser *parser) {
    if (!is_at_end(parser)) parser->current++;
    return &parser->tokens[parser->current - 1];
}

static int check(Parser *parser, TokenType type) {
    if (is_at_end(parser)) return 0;
    return current_token(parser)->type == type;
}

static int match(Parser *parser, TokenType type) {
    if (check(parser, type)) {
        advance(parser);
        return 1;
    }
    return 0;
}

// 解析主表达式
static ASTNode* parse_primary(Parser *parser) {
    Token *token = current_token(parser);
    if (!token) return NULL;
    
    // 整数字面量
    if (token->type == TOKEN_NUMBER) {
        advance(parser);
        ASTNode *node = create_node(AST_INTEGER);
        node->data.int_value = strtoll(token->value, NULL, 0);
        return node;
    }
    
    // 标识符或函数调用
    if (token->type == TOKEN_IDENTIFIER) {
        char *name = strdup(token->value);
        advance(parser);
        
        // 函数调用
        if (match(parser, TOKEN_LPAREN)) {
            ASTNode *node = create_node(AST_CALL);
            node->data.call.name = name;
            node->data.call.args = NULL;
            node->data.call.arg_count = 0;
            
            // 解析参数
            if (!check(parser, TOKEN_RPAREN)) {
                ASTNode **args = malloc(sizeof(ASTNode*) * 10);
                int count = 0;
                
                do {
                    args[count++] = parse_expression(parser);
                } while (match(parser, TOKEN_COMMA));
                
                node->data.call.args = args;
                node->data.call.arg_count = count;
            }
            
            if (!match(parser, TOKEN_RPAREN)) {
                free(name);
                free_ast(node);
                return NULL;
            }
            
            return node;
        }
        
        // 普通标识符
        ASTNode *node = create_node(AST_IDENTIFIER);
        node->data.str_value = name;
        return node;
    }
    
    // 括号表达式
    if (match(parser, TOKEN_LPAREN)) {
        ASTNode *expr = parse_expression(parser);
        if (!match(parser, TOKEN_RPAREN)) {
            free_ast(expr);
            return NULL;
        }
        return expr;
    }
    
    return NULL;
}

// 解析一元表达式
static ASTNode* parse_unary(Parser *parser) {
    if (match(parser, TOKEN_MINUS) || match(parser, TOKEN_LOGICAL_NOT) || match(parser, TOKEN_BIT_NOT)) {
        Token *op = &parser->tokens[parser->current - 1];
        ASTNode *node = create_node(AST_UNARY_OP);
        node->data.unary.op = op->value[0];
        node->data.unary.operand = parse_unary(parser);
        return node;
    }
    
    return parse_primary(parser);
}

// 解析乘除表达式
static ASTNode* parse_multiplicative(Parser *parser) {
    ASTNode *left = parse_unary(parser);
    
    while (match(parser, TOKEN_MULTIPLY) || match(parser, TOKEN_DIVIDE) || match(parser, TOKEN_MOD)) {
        Token *op = &parser->tokens[parser->current - 1];
        ASTNode *node = create_node(AST_BINARY_OP);
        node->data.binary.left = left;
        node->data.binary.op = op->value[0];
        node->data.binary.right = parse_unary(parser);
        left = node;
    }
    
    return left;
}

// 解析加减表达式
static ASTNode* parse_additive(Parser *parser) {
    ASTNode *left = parse_multiplicative(parser);
    
    while (match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS)) {
        Token *op = &parser->tokens[parser->current - 1];
        ASTNode *node = create_node(AST_BINARY_OP);
        node->data.binary.left = left;
        node->data.binary.op = op->value[0];
        node->data.binary.right = parse_multiplicative(parser);
        left = node;
    }
    
    return left;
}

// 解析关系表达式
static ASTNode* parse_relational(Parser *parser) {
    ASTNode *left = parse_additive(parser);
    
    while (match(parser, TOKEN_LESS) || match(parser, TOKEN_GREATER) || 
           match(parser, TOKEN_LESS_EQUAL) || match(parser, TOKEN_GREATER_EQUAL)) {
        Token *op = &parser->tokens[parser->current - 1];
        ASTNode *node = create_node(AST_BINARY_OP);
        node->data.binary.left = left;
        node->data.binary.op = op->type == TOKEN_LESS_EQUAL ? 'L' : 
                               op->type == TOKEN_GREATER_EQUAL ? 'G' : op->value[0];
        node->data.binary.right = parse_additive(parser);
        left = node;
    }
    
    return left;
}

// 解析相等表达式
static ASTNode* parse_equality(Parser *parser) {
    ASTNode *left = parse_relational(parser);
    
    while (match(parser, TOKEN_EQUAL) || match(parser, TOKEN_NOT_EQUAL)) {
        Token *op = &parser->tokens[parser->current - 1];
        ASTNode *node = create_node(AST_BINARY_OP);
        node->data.binary.left = left;
        node->data.binary.op = op->type == TOKEN_EQUAL ? 'E' : 'N';
        node->data.binary.right = parse_relational(parser);
        left = node;
    }
    
    return left;
}

// 解析赋值表达式
static ASTNode* parse_assignment(Parser *parser) {
    ASTNode *left = parse_equality(parser);
    
    if (match(parser, TOKEN_ASSIGN)) {
        if (left->type != AST_IDENTIFIER) {
            free_ast(left);
            return NULL;
        }
        
        ASTNode *node = create_node(AST_ASSIGNMENT);
        node->data.assign.name = strdup(left->data.str_value);
        node->data.assign.value = parse_assignment(parser);
        free_ast(left);
        return node;
    }
    
    return left;
}

// 解析表达式
static ASTNode* parse_expression(Parser *parser) {
    return parse_assignment(parser);
}

// 解析声明
static ASTNode* parse_declaration(Parser *parser) {
    // 简化：只支持 int 类型
    if (!match(parser, TOKEN_INT)) {
        return NULL;
    }
    
    Token *name_token = current_token(parser);
    if (!name_token || name_token->type != TOKEN_IDENTIFIER) {
        return NULL;
    }
    advance(parser);
    
    ASTNode *node = create_node(AST_DECLARATION);
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
static ASTNode* parse_statement(Parser *parser) {
    // return 语句
    if (match(parser, TOKEN_RETURN)) {
        ASTNode *node = create_node(AST_RETURN);
        if (!check(parser, TOKEN_SEMICOLON)) {
            node->data.ret.value = parse_expression(parser);
        }
        match(parser, TOKEN_SEMICOLON);
        return node;
    }
    
    // if 语句
    if (match(parser, TOKEN_IF)) {
        if (!match(parser, TOKEN_LPAREN)) return NULL;
        
        ASTNode *node = create_node(AST_IF);
        node->data.if_stmt.cond = parse_expression(parser);
        
        if (!match(parser, TOKEN_RPAREN)) {
            free_ast(node);
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
        
        ASTNode *node = create_node(AST_WHILE);
        node->data.while_stmt.cond = parse_expression(parser);
        
        if (!match(parser, TOKEN_RPAREN)) {
            free_ast(node);
            return NULL;
        }
        
        node->data.while_stmt.body = parse_statement(parser);
        return node;
    }
    
    // for 语句
    if (match(parser, TOKEN_FOR)) {
        if (!match(parser, TOKEN_LPAREN)) return NULL;
        
        ASTNode *node = create_node(AST_FOR);
        
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
            free_ast(node);
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
    ASTNode *expr = parse_expression(parser);
    if (expr) {
        match(parser, TOKEN_SEMICOLON);
        ASTNode *node = create_node(AST_EXPRESSION_STMT);
        node->data.ret.value = expr; // 复用 ret 结构
        return node;
    }
    
    return NULL;
}

// 解析复合语句
static ASTNode* parse_compound_statement(Parser *parser) {
    if (!match(parser, TOKEN_LBRACE)) {
        return NULL;
    }
    
    ASTNode *node = create_node(AST_COMPOUND);
    ASTNode **stmts = malloc(sizeof(ASTNode*) * 100);
    int count = 0;
    
    while (!check(parser, TOKEN_RBRACE) && !is_at_end(parser)) {
        ASTNode *stmt = parse_statement(parser);
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
static ASTNode* parse_function(Parser *parser) {
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
    
    ASTNode *body = parse_compound_statement(parser);
    if (!body) {
        return NULL;
    }
    
    ASTNode *node = create_node(AST_FUNCTION);
    node->data.function.name = strdup(name_token->value);
    node->data.function.params = NULL;
    node->data.function.body = body;
    
    return node;
}

// 解析程序
static ASTNode* parse_program(Parser *parser) {
    ASTNode *node = create_node(AST_PROGRAM);
    ASTNode **functions = malloc(sizeof(ASTNode*) * 10);
    int count = 0;
    
    while (!is_at_end(parser)) {
        ASTNode *func = parse_function(parser);
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
static ASTNode* parse(Token *tokens, int token_count) {
    Parser parser = {
        .tokens = tokens,
        .token_count = token_count,
        .current = 0,
        .error_msg = {0}
    };
    
    return parse_program(&parser);
}

// ====================================
// x86-64 代码生成器
// ====================================

typedef struct {
    unsigned char *code;
    size_t size;
    size_t capacity;
    
    // 局部变量
    struct {
        char *name;
        int offset;
    } *locals;
    int local_count;
    int local_capacity;
    int stack_offset;
} CodeBuffer;

// 创建代码缓冲区
static CodeBuffer* create_code_buffer() {
    CodeBuffer *buf = (CodeBuffer*)calloc(1, sizeof(CodeBuffer));
    buf->capacity = 1024;
    buf->code = (unsigned char*)malloc(buf->capacity);
    return buf;
}

// 释放代码缓冲区
static void free_code_buffer(CodeBuffer *buf) {
    if (!buf) return;
    
    free(buf->code);
    
    for (int i = 0; i < buf->local_count; i++) {
        free(buf->locals[i].name);
    }
    free(buf->locals);
    
    free(buf);
}

// 确保缓冲区有足够空间
static void ensure_capacity(CodeBuffer *buf, size_t needed) {
    if (buf->size + needed > buf->capacity) {
        while (buf->size + needed > buf->capacity) {
            buf->capacity *= 2;
        }
        buf->code = (unsigned char*)realloc(buf->code, buf->capacity);
    }
}

// 发出字节
static void emit_byte(CodeBuffer *buf, unsigned char byte) {
    ensure_capacity(buf, 1);
    buf->code[buf->size++] = byte;
}

// 发出32位整数（小端序）
static void emit_int32(CodeBuffer *buf, int value) {
    emit_byte(buf, value & 0xFF);
    emit_byte(buf, (value >> 8) & 0xFF);
    emit_byte(buf, (value >> 16) & 0xFF);
    emit_byte(buf, (value >> 24) & 0xFF);
}

// MOV RAX, imm32
static void emit_mov_rax_imm32(CodeBuffer *buf, int value) {
    emit_byte(buf, 0xB8);  // MOV EAX, imm32
    emit_int32(buf, value);
}

// PUSH RBP
static void emit_push_rbp(CodeBuffer *buf) {
    emit_byte(buf, 0x55);
}

// POP RBP
static void emit_pop_rbp(CodeBuffer *buf) {
    emit_byte(buf, 0x5D);
}

// MOV RBP, RSP
static void emit_mov_rbp_rsp(CodeBuffer *buf) {
    emit_byte(buf, 0x48);  // REX.W
    emit_byte(buf, 0x89);
    emit_byte(buf, 0xE5);
}

// MOV RSP, RBP
static void emit_mov_rsp_rbp(CodeBuffer *buf) {
    emit_byte(buf, 0x48);  // REX.W
    emit_byte(buf, 0x89);
    emit_byte(buf, 0xEC);
}

// RET
static void emit_ret(CodeBuffer *buf) {
    emit_byte(buf, 0xC3);
}

// SUB RSP, imm32
static void emit_sub_rsp_imm32(CodeBuffer *buf, int value) {
    emit_byte(buf, 0x48);  // REX.W
    emit_byte(buf, 0x81);
    emit_byte(buf, 0xEC);
    emit_int32(buf, value);
}

// 查找或创建局部变量
static int get_or_create_local(CodeBuffer *buf, const char *name) {
    // 查找现有变量
    for (int i = 0; i < buf->local_count; i++) {
        if (strcmp(buf->locals[i].name, name) == 0) {
            return buf->locals[i].offset;
        }
    }
    
    // 创建新变量
    if (buf->local_count >= buf->local_capacity) {
        buf->local_capacity = buf->local_capacity ? buf->local_capacity * 2 : 8;
        buf->locals = realloc(buf->locals, buf->local_capacity * sizeof(buf->locals[0]));
    }
    
    buf->stack_offset += 8;
    buf->locals[buf->local_count].name = strdup(name);
    buf->locals[buf->local_count].offset = buf->stack_offset;
    buf->local_count++;
    
    return buf->stack_offset;
}

// 前向声明
static void codegen_expr(CodeBuffer *buf, ASTNode *expr);
static void codegen_stmt(CodeBuffer *buf, ASTNode *stmt);

// 生成表达式代码（简化版）
static void codegen_expr(CodeBuffer *buf, ASTNode *expr) {
    if (!expr) return;
    
    switch (expr->type) {
        case AST_INTEGER:
            emit_mov_rax_imm32(buf, (int)expr->data.int_value);
            break;
            
        case AST_IDENTIFIER:
            // 简化：暂时只返回0
            emit_mov_rax_imm32(buf, 0);
            break;
            
        case AST_BINARY_OP:
            // 简化：只实现加法
            codegen_expr(buf, expr->data.binary.left);
            // 保存左操作数到栈上
            emit_byte(buf, 0x50);  // PUSH RAX
            
            codegen_expr(buf, expr->data.binary.right);
            // RAX = 右操作数
            // 从栈上恢复左操作数到RCX
            emit_byte(buf, 0x59);  // POP RCX
            
            if (expr->data.binary.op == '+') {
                // ADD RAX, RCX
                emit_byte(buf, 0x48);  // REX.W
                emit_byte(buf, 0x01);
                emit_byte(buf, 0xC8);
            } else if (expr->data.binary.op == '-') {
                // XCHG RAX, RCX
                emit_byte(buf, 0x48);  // REX.W
                emit_byte(buf, 0x91);
                // SUB RAX, RCX
                emit_byte(buf, 0x48);  // REX.W
                emit_byte(buf, 0x29);
                emit_byte(buf, 0xC8);
            } else if (expr->data.binary.op == '*') {
                // IMUL RAX, RCX
                emit_byte(buf, 0x48);  // REX.W
                emit_byte(buf, 0x0F);
                emit_byte(buf, 0xAF);
                emit_byte(buf, 0xC1);
            } else {
                // 其他操作符暂时返回0
                emit_mov_rax_imm32(buf, 0);
            }
            break;
            
        default:
            emit_mov_rax_imm32(buf, 0);
            break;
    }
}

// 生成语句代码
static void codegen_stmt(CodeBuffer *buf, ASTNode *stmt) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case AST_RETURN:
            if (stmt->data.ret.value) {
                codegen_expr(buf, stmt->data.ret.value);
            }
            
            // 检查是否在main函数中（简化：假设只有一个函数）
            // 在实际实现中，应该传递函数上下文
            // 这里我们假设如果没有函数序言（push rbp），就是main函数
            if (buf->size == 0 || buf->code[0] != 0x55) {
                // main函数，使用系统调用退出
                // RAX已经包含返回值，将其移到RDI
                emit_byte(buf, 0x48);  // REX.W
                emit_byte(buf, 0x89);  // MOV
                emit_byte(buf, 0xC7);  // RDI, RAX
                
                // mov rax, 60 (sys_exit)
                emit_byte(buf, 0x48);
                emit_byte(buf, 0xC7);
                emit_byte(buf, 0xC0);
                emit_byte(buf, 0x3C);
                emit_byte(buf, 0x00);
                emit_byte(buf, 0x00);
                emit_byte(buf, 0x00);
                
                // syscall
                emit_byte(buf, 0x0F);
                emit_byte(buf, 0x05);
            } else {
                // 普通函数，使用ret返回
                emit_mov_rsp_rbp(buf);
                emit_pop_rbp(buf);
                emit_ret(buf);
            }
            break;
            
        case AST_COMPOUND:
            for (int i = 0; i < stmt->data.compound.count; i++) {
                codegen_stmt(buf, stmt->data.compound.statements[i]);
            }
            break;
            
        case AST_EXPRESSION_STMT:
            codegen_expr(buf, stmt->data.ret.value);
            break;
            
        default:
            break;
    }
}

// 生成函数代码
static void codegen_function(CodeBuffer *buf, ASTNode *func) {
    if (func->type != AST_FUNCTION) return;
    
    // 检查是否是main函数
    int is_main = strcmp(func->data.function.name, "main") == 0;
    
    // 重置局部变量
    buf->local_count = 0;
    buf->stack_offset = 0;
    
    // 如果不是main函数，生成标准函数序言
    if (!is_main) {
        // 函数序言
        emit_push_rbp(buf);
        emit_mov_rbp_rsp(buf);
        
        // 为局部变量预留空间（简化：固定分配64字节）
        emit_sub_rsp_imm32(buf, 64);
    }
    
    // 生成函数体
    codegen_stmt(buf, func->data.function.body);
    
    // 如果是main函数，确保使用系统调用退出
    if (is_main) {
        // 如果函数体没有返回语句，默认返回0
        if (buf->size == 0 || (buf->size > 0 && buf->code[buf->size - 1] != 0x05)) {
            // mov rax, 60 (sys_exit)
            emit_byte(buf, 0x48);
            emit_byte(buf, 0xC7);
            emit_byte(buf, 0xC0);
            emit_byte(buf, 0x3C);
            emit_byte(buf, 0x00);
            emit_byte(buf, 0x00);
            emit_byte(buf, 0x00);
            
            // mov rdi, 0 (退出码)
            emit_byte(buf, 0x48);
            emit_byte(buf, 0xC7);
            emit_byte(buf, 0xC7);
            emit_byte(buf, 0x00);
            emit_byte(buf, 0x00);
            emit_byte(buf, 0x00);
            emit_byte(buf, 0x00);
            
            // syscall
            emit_byte(buf, 0x0F);
            emit_byte(buf, 0x05);
        }
    } else {
        // 非main函数，确保有返回
        if (buf->size == 0 || buf->code[buf->size - 1] != 0xC3) {
            emit_mov_rax_imm32(buf, 0);
            emit_mov_rsp_rbp(buf);
            emit_pop_rbp(buf);
            emit_ret(buf);
        }
    }
}

// 生成程序代码
static unsigned char* generate_code(ASTNode *ast, size_t *out_size) {
    if (!ast || ast->type != AST_PROGRAM) {
        return NULL;
    }
    
    CodeBuffer *buf = create_code_buffer();
    
    // 查找main函数并生成代码
    int main_found = 0;
    for (int i = 0; i < ast->data.compound.count; i++) {
        ASTNode *func = ast->data.compound.statements[i];
        if (func->type == AST_FUNCTION && strcmp(func->data.function.name, "main") == 0) {
            codegen_function(buf, func);
            main_found = 1;
            break;
        }
    }
    
    if (!main_found) {
        fprintf(stderr, "错误：未找到main函数\n");
        free_code_buffer(buf);
        return NULL;
    }
    
    // 返回生成的代码
    unsigned char *code = malloc(buf->size);
    memcpy(code, buf->code, buf->size);
    *out_size = buf->size;
    
    free_code_buffer(buf);
    return code;
}

// ====================================
// ELF 文件生成
// ====================================

static int write_elf_executable(const char *filename, unsigned char *code, size_t code_size) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("无法创建输出文件");
        return -1;
    }
    
    // 计算各部分的偏移和地址
    size_t ehdr_size = sizeof(Elf64_Ehdr);
    size_t phdr_size = sizeof(Elf64_Phdr);
    size_t headers_size = ehdr_size + phdr_size;
    
    // 代码段的文件偏移
    size_t code_offset = headers_size;
    
    // 虚拟地址
    Elf64_Addr base_addr = 0x400000;
    Elf64_Addr code_addr = base_addr + code_offset;
    
    // ELF 头
    Elf64_Ehdr ehdr = {0};
    ehdr.e_ident[EI_MAG0] = ELFMAG0;
    ehdr.e_ident[EI_MAG1] = ELFMAG1;
    ehdr.e_ident[EI_MAG2] = ELFMAG2;
    ehdr.e_ident[EI_MAG3] = ELFMAG3;
    ehdr.e_ident[EI_CLASS] = ELFCLASS64;
    ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
    ehdr.e_ident[EI_VERSION] = EV_CURRENT;
    ehdr.e_ident[EI_OSABI] = ELFOSABI_SYSV;
    ehdr.e_type = ET_EXEC;
    ehdr.e_machine = EM_X86_64;
    ehdr.e_version = EV_CURRENT;
    ehdr.e_entry = code_addr;  // 入口地址指向代码开始
    ehdr.e_phoff = ehdr_size;
    ehdr.e_shoff = 0;
    ehdr.e_flags = 0;
    ehdr.e_ehsize = ehdr_size;
    ehdr.e_phentsize = phdr_size;
    ehdr.e_phnum = 1;
    ehdr.e_shentsize = 0;
    ehdr.e_shnum = 0;
    ehdr.e_shstrndx = 0;
    
    // 程序头
    Elf64_Phdr phdr = {0};
    phdr.p_type = PT_LOAD;
    phdr.p_flags = PF_X | PF_R;
    phdr.p_offset = 0;
    phdr.p_vaddr = base_addr;
    phdr.p_paddr = base_addr;
    phdr.p_filesz = headers_size + code_size;
    phdr.p_memsz = phdr.p_filesz;
    phdr.p_align = 0x1000;
    
    // 写入ELF头
    fwrite(&ehdr, sizeof(ehdr), 1, f);
    
    // 写入程序头
    fwrite(&phdr, sizeof(phdr), 1, f);
    
    // 写入代码
    fwrite(code, code_size, 1, f);
    
    fclose(f);
    
    // 设置可执行权限
    if (chmod(filename, 0755) != 0) {
        perror("无法设置可执行权限");
        return -1;
    }
    
    return 0;
}

// ====================================
// 主函数
// ====================================

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "用法: %s <源文件> <输出文件>\n", argv[0]);
        return 1;
    }
    
    // 读取源文件
    FILE *f = fopen(argv[1], "r");
    if (!f) {
        perror("无法打开源文件");
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *source = malloc(size + 1);
    fread(source, 1, size, f);
    source[size] = '\0';
    fclose(f);
    
    printf("编译 %s -> %s\n", argv[1], argv[2]);
    
    // 词法分析
    int token_count;
    Token *tokens = tokenize(source, &token_count);
    printf("词法分析完成: %d tokens\n", token_count);
    
    // 语法分析
    ASTNode *ast = parse(tokens, token_count);
    if (!ast) {
        fprintf(stderr, "语法分析失败\n");
        free_tokens(tokens, token_count);
        free(source);
        return 1;
    }
    printf("语法分析完成\n");
    
    // 代码生成
    size_t code_size;
    unsigned char *code = generate_code(ast, &code_size);
    if (!code) {
        fprintf(stderr, "代码生成失败\n");
        free_ast(ast);
        free_tokens(tokens, token_count);
        free(source);
        return 1;
    }
    printf("代码生成完成: %zu 字节\n", code_size);
    
    // 生成可执行文件
    if (write_elf_executable(argv[2], code, code_size) != 0) {
        fprintf(stderr, "生成可执行文件失败\n");
        free(code);
        free_ast(ast);
        free_tokens(tokens, token_count);
        free(source);
        return 1;
    }
    
    printf("成功生成可执行文件: %s\n", argv[2]);
    
    // 清理
    free(code);
    free_ast(ast);
    free_tokens(tokens, token_count);
    free(source);
    
    return 0;
}