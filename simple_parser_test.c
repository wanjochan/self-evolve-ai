#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// 简化的Token类型
typedef enum {
    TOKEN_EOF = 0,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_INT,
    TOKEN_RETURN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_SEMICOLON,
    TOKEN_ASSIGN,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    char *value;
    int line;
} Token;

// 简化的AST节点类型
typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_RETURN_STMT,
    AST_VAR_DECL,
    AST_BINARY_OP,
    AST_NUMBER,
    AST_IDENTIFIER
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    char *value;
    struct ASTNode *left;
    struct ASTNode *right;
    struct ASTNode *next;
} ASTNode;

// 简化的编译器结构
typedef struct {
    Token *tokens;
    int token_count;
    int current_token;
    const char *source;
} SimpleCompiler;

#define MAX_TOKENS 1000

// 词法分析器
static bool is_keyword(const char *str) {
    return strcmp(str, "int") == 0 || strcmp(str, "return") == 0;
}

static TokenType get_keyword_type(const char *str) {
    if (strcmp(str, "int") == 0) return TOKEN_INT;
    if (strcmp(str, "return") == 0) return TOKEN_RETURN;
    return TOKEN_IDENTIFIER;
}

static int tokenize(SimpleCompiler *compiler) {
    const char *p = compiler->source;
    int token_count = 0;
    int line = 1;
    
    compiler->tokens = malloc(MAX_TOKENS * sizeof(Token));
    
    while (*p && token_count < MAX_TOKENS - 1) {
        // 跳过空白字符
        while (isspace(*p)) {
            if (*p == '\n') line++;
            p++;
        }
        
        if (*p == '\0') break;
        
        Token *token = &compiler->tokens[token_count++];
        token->line = line;
        
        // 标识符和关键字
        if (isalpha(*p) || *p == '_') {
            const char *start = p;
            while (isalnum(*p) || *p == '_') p++;
            
            int len = p - start;
            token->value = malloc(len + 1);
            strncpy(token->value, start, len);
            token->value[len] = '\0';
            
            if (is_keyword(token->value)) {
                token->type = get_keyword_type(token->value);
            } else {
                token->type = TOKEN_IDENTIFIER;
            }
        }
        // 数字
        else if (isdigit(*p)) {
            const char *start = p;
            while (isdigit(*p)) p++;
            
            int len = p - start;
            token->value = malloc(len + 1);
            strncpy(token->value, start, len);
            token->value[len] = '\0';
            token->type = TOKEN_NUMBER;
        }
        // 单字符操作符
        else {
            token->value = malloc(2);
            token->value[0] = *p;
            token->value[1] = '\0';
            
            switch (*p) {
                case '{': token->type = TOKEN_LBRACE; break;
                case '}': token->type = TOKEN_RBRACE; break;
                case '(': token->type = TOKEN_LPAREN; break;
                case ')': token->type = TOKEN_RPAREN; break;
                case ';': token->type = TOKEN_SEMICOLON; break;
                case '=': token->type = TOKEN_ASSIGN; break;
                case '+': token->type = TOKEN_PLUS; break;
                case '-': token->type = TOKEN_MINUS; break;
                case '*': token->type = TOKEN_MULTIPLY; break;
                case '/': token->type = TOKEN_DIVIDE; break;
                default: token->type = TOKEN_ERROR; break;
            }
            p++;
        }
    }
    
    // EOF token
    compiler->tokens[token_count].type = TOKEN_EOF;
    compiler->tokens[token_count].value = NULL;
    
    compiler->token_count = token_count;
    return token_count;
}

// 语法分析器辅助函数
static Token* current_token(SimpleCompiler *compiler) {
    if (compiler->current_token < compiler->token_count) {
        return &compiler->tokens[compiler->current_token];
    }
    return &compiler->tokens[compiler->token_count]; // EOF token
}

static void advance_token(SimpleCompiler *compiler) {
    if (compiler->current_token < compiler->token_count) {
        compiler->current_token++;
    }
}

static bool match_token(SimpleCompiler *compiler, TokenType type) {
    Token *token = current_token(compiler);
    return token->type == type;
}

static bool consume_token(SimpleCompiler *compiler, TokenType type) {
    if (match_token(compiler, type)) {
        advance_token(compiler);
        return true;
    }
    return false;
}

// AST节点创建
static ASTNode* create_ast_node(ASTNodeType type, const char *value) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = type;
    if (value) {
        node->value = strdup(value);
    }
    return node;
}

// 语法分析器
static ASTNode* parse_expression(SimpleCompiler *compiler);

static ASTNode* parse_primary(SimpleCompiler *compiler) {
    Token *token = current_token(compiler);
    
    if (token->type == TOKEN_NUMBER) {
        ASTNode *node = create_ast_node(AST_NUMBER, token->value);
        advance_token(compiler);
        return node;
    }
    
    if (token->type == TOKEN_IDENTIFIER) {
        ASTNode *node = create_ast_node(AST_IDENTIFIER, token->value);
        advance_token(compiler);
        return node;
    }
    
    if (token->type == TOKEN_LPAREN) {
        advance_token(compiler);
        ASTNode *node = parse_expression(compiler);
        consume_token(compiler, TOKEN_RPAREN);
        return node;
    }
    
    printf("Error: Unexpected token in primary expression: %s\n", token->value);
    return NULL;
}

static ASTNode* parse_expression(SimpleCompiler *compiler) {
    ASTNode *left = parse_primary(compiler);
    
    Token *op_token = current_token(compiler);
    if (op_token->type == TOKEN_PLUS || op_token->type == TOKEN_MINUS ||
        op_token->type == TOKEN_MULTIPLY || op_token->type == TOKEN_DIVIDE) {
        
        advance_token(compiler);
        ASTNode *right = parse_expression(compiler);
        
        ASTNode *binary_op = create_ast_node(AST_BINARY_OP, op_token->value);
        binary_op->left = left;
        binary_op->right = right;
        return binary_op;
    }
    
    return left;
}

static ASTNode* parse_statement(SimpleCompiler *compiler) {
    if (match_token(compiler, TOKEN_RETURN)) {
        advance_token(compiler);
        ASTNode *expr = parse_expression(compiler);
        consume_token(compiler, TOKEN_SEMICOLON);
        
        ASTNode *return_stmt = create_ast_node(AST_RETURN_STMT, "return");
        return_stmt->left = expr;
        return return_stmt;
    }
    
    // Variable declaration: int x = expr;
    if (match_token(compiler, TOKEN_INT)) {
        advance_token(compiler);
        Token *name_token = current_token(compiler);
        if (!consume_token(compiler, TOKEN_IDENTIFIER)) {
            printf("Error: Expected identifier after int\n");
            return NULL;
        }
        
        ASTNode *var_decl = create_ast_node(AST_VAR_DECL, name_token->value);
        
        if (consume_token(compiler, TOKEN_ASSIGN)) {
            var_decl->right = parse_expression(compiler);
        }
        
        consume_token(compiler, TOKEN_SEMICOLON);
        return var_decl;
    }
    
    printf("Error: Unknown statement\n");
    return NULL;
}

static ASTNode* parse_function(SimpleCompiler *compiler) {
    if (!consume_token(compiler, TOKEN_INT)) {
        printf("Error: Expected return type\n");
        return NULL;
    }
    
    Token *name_token = current_token(compiler);
    if (!consume_token(compiler, TOKEN_IDENTIFIER)) {
        printf("Error: Expected function name\n");
        return NULL;
    }
    
    if (!consume_token(compiler, TOKEN_LPAREN)) {
        printf("Error: Expected '(' after function name\n");
        return NULL;
    }
    
    if (!consume_token(compiler, TOKEN_RPAREN)) {
        printf("Error: Expected ')' - parameters not supported yet\n");
        return NULL;
    }
    
    if (!consume_token(compiler, TOKEN_LBRACE)) {
        printf("Error: Expected '{' to start function body\n");
        return NULL;
    }
    
    ASTNode *function = create_ast_node(AST_FUNCTION, name_token->value);
    ASTNode *statements = NULL;
    ASTNode *last_stmt = NULL;
    
    while (!match_token(compiler, TOKEN_RBRACE) && !match_token(compiler, TOKEN_EOF)) {
        ASTNode *stmt = parse_statement(compiler);
        if (stmt) {
            if (!statements) {
                statements = stmt;
                last_stmt = stmt;
            } else {
                last_stmt->next = stmt;
                last_stmt = stmt;
            }
        }
    }
    
    if (!consume_token(compiler, TOKEN_RBRACE)) {
        printf("Error: Expected '}' to end function body\n");
        return NULL;
    }
    
    function->left = statements;
    return function;
}

static ASTNode* parse_program(SimpleCompiler *compiler) {
    ASTNode *program = create_ast_node(AST_PROGRAM, "program");
    ASTNode *functions = NULL;
    ASTNode *last_func = NULL;
    
    compiler->current_token = 0;
    
    while (!match_token(compiler, TOKEN_EOF)) {
        ASTNode *func = parse_function(compiler);
        if (func) {
            if (!functions) {
                functions = func;
                last_func = func;
            } else {
                last_func->next = func;
                last_func = func;
            }
        } else {
            break;
        }
    }
    
    program->left = functions;
    return program;
}

// AST打印函数
static void print_ast(ASTNode *node, int depth) {
    if (!node) return;
    
    for (int i = 0; i < depth; i++) printf("  ");
    
    switch (node->type) {
        case AST_PROGRAM: printf("PROGRAM\n"); break;
        case AST_FUNCTION: printf("FUNCTION: %s\n", node->value); break;
        case AST_RETURN_STMT: printf("RETURN\n"); break;
        case AST_VAR_DECL: printf("VAR_DECL: %s\n", node->value); break;
        case AST_BINARY_OP: printf("BINARY_OP: %s\n", node->value); break;
        case AST_NUMBER: printf("NUMBER: %s\n", node->value); break;
        case AST_IDENTIFIER: printf("IDENTIFIER: %s\n", node->value); break;
    }
    
    if (node->left) print_ast(node->left, depth + 1);
    if (node->right) print_ast(node->right, depth + 1);
    if (node->next) print_ast(node->next, depth);
}

// 主函数
int main() {
    const char *source = 
        "int main() {\n"
        "    int x = 42;\n"
        "    int y = x + 10;\n"
        "    return y * 2;\n"
        "}\n";
    
    printf("源代码:\n%s\n", source);
    printf("===================\n");
    
    SimpleCompiler compiler = {0};
    compiler.source = source;
    
    // 词法分析
    printf("开始词法分析...\n");
    int token_count = tokenize(&compiler);
    printf("✓ 词法分析完成，生成 %d 个token\n\n", token_count);
    
    // 打印tokens
    printf("Tokens:\n");
    for (int i = 0; i < token_count; i++) {
        printf("  %d: %s (type=%d)\n", i, 
               compiler.tokens[i].value ? compiler.tokens[i].value : "EOF", 
               compiler.tokens[i].type);
    }
    printf("\n");
    
    // 语法分析
    printf("开始语法分析...\n");
    ASTNode *ast = parse_program(&compiler);
    
    if (ast) {
        printf("✓ 语法分析完成\n\n");
        printf("AST结构:\n");
        print_ast(ast, 0);
        
        printf("\n✓ 语法分析器测试成功！\n");
    } else {
        printf("✗ 语法分析失败\n");
        return 1;
    }
    
    return 0;
}