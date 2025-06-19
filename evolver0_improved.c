/**
 * evolver0_improved.c - 第零代自举编译器（改进版）
 * 
 * 这是evolver0的改进版本，整合了解析器和代码生成功能
 * 能够编译简单的C程序生成Linux x86-64可执行文件
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#define VERSION 0
#define MAX_TOKENS 10000
#define MAX_CODE_SIZE 65536

// ====================================
// 基本类型定义
// ====================================

typedef enum {
    TOKEN_EOF = 0,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    
    // 关键字
    TOKEN_INT,
    TOKEN_CHAR,
    TOKEN_VOID,
    TOKEN_RETURN,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    
    // 操作符
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_MOD,
    TOKEN_ASSIGN,
    TOKEN_EQUAL,
    TOKEN_NOT_EQUAL,
    TOKEN_LESS,
    TOKEN_GREATER,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER_EQUAL,
    
    // 分隔符
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    
    TOKEN_ERROR,
    TOKEN_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    char *value;
    int line;
    int column;
} Token;

// AST节点类型
typedef enum {
    NODE_TRANSLATION_UNIT,
    NODE_FUNCTION_DECL,
    NODE_VAR_DECL,
    NODE_COMPOUND_STMT,
    NODE_RETURN_STMT,
    NODE_IF_STMT,
    NODE_WHILE_STMT,
    NODE_FOR_STMT,
    NODE_EXPRESSION_STMT,
    NODE_BINARY_OP,
    NODE_UNARY_OP,
    NODE_IDENTIFIER,
    NODE_INTEGER_LITERAL,
    NODE_STRING_LITERAL,
    NODE_FUNCTION_CALL,
    NODE_TYPE_SPECIFIER
} NodeType;

typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_ASSIGN,
    OP_EQ,
    OP_NE,
    OP_LT,
    OP_GT,
    OP_LE,
    OP_GE
} BinaryOp;

typedef enum {
    TYPE_VOID,
    TYPE_CHAR,
    TYPE_INT
} BasicType;

// AST节点结构
typedef struct ASTNode {
    NodeType type;
    int line;
    int column;
    
    // 节点特定数据
    union {
        // 标识符
        struct {
            char *name;
        } identifier;
        
        // 整数字面量
        struct {
            long long value;
        } integer;
        
        // 字符串字面量
        struct {
            char *value;
        } string;
        
        // 二元操作
        struct {
            BinaryOp op;
            struct ASTNode *left;
            struct ASTNode *right;
        } binary_op;
        
        // 一元操作
        struct {
            int op; // 操作符类型
            struct ASTNode *operand;
        } unary_op;
        
        // 函数声明
        struct {
            char *name;
            struct ASTNode *return_type;
            struct ASTNode *body;
        } function;
        
        // 变量声明
        struct {
            char *name;
            struct ASTNode *type;
            struct ASTNode *init;
        } var_decl;
        
        // 复合语句
        struct {
            struct ASTNode **statements;
            int count;
        } compound;
        
        // return语句
        struct {
            struct ASTNode *value;
        } return_stmt;
        
        // if语句
        struct {
            struct ASTNode *condition;
            struct ASTNode *then_stmt;
            struct ASTNode *else_stmt;
        } if_stmt;
        
        // while语句
        struct {
            struct ASTNode *condition;
            struct ASTNode *body;
        } while_stmt;
        
        // 类型说明符
        struct {
            BasicType basic_type;
        } type_spec;
        
        // 翻译单元
        struct {
            struct ASTNode **declarations;
            int count;
        } translation_unit;
    } data;
} ASTNode;

// ====================================
// 全局变量
// ====================================

static Token *tokens = NULL;
static int token_count = 0;
static int current_token = 0;

// ====================================
// 词法分析器
// ====================================

static bool is_keyword(const char *str) {
    static const char *keywords[] = {
        "int", "char", "void", "return", "if", "else", "while", "for",
        NULL
    };
    
    for (int i = 0; keywords[i] != NULL; i++) {
        if (strcmp(str, keywords[i]) == 0) {
            return true;
        }
    }
    return false;
}

static TokenType get_keyword_type(const char *str) {
    if (strcmp(str, "int") == 0) return TOKEN_INT;
    if (strcmp(str, "char") == 0) return TOKEN_CHAR;
    if (strcmp(str, "void") == 0) return TOKEN_VOID;
    if (strcmp(str, "return") == 0) return TOKEN_RETURN;
    if (strcmp(str, "if") == 0) return TOKEN_IF;
    if (strcmp(str, "else") == 0) return TOKEN_ELSE;
    if (strcmp(str, "while") == 0) return TOKEN_WHILE;
    if (strcmp(str, "for") == 0) return TOKEN_FOR;
    return TOKEN_UNKNOWN;
}

static void add_token(TokenType type, const char *value, int line, int column) {
    if (token_count >= MAX_TOKENS) {
        fprintf(stderr, "错误：Token数量超过限制\n");
        exit(1);
    }
    
    tokens[token_count].type = type;
    tokens[token_count].value = value ? strdup(value) : NULL;
    tokens[token_count].line = line;
    tokens[token_count].column = column;
    token_count++;
}

static void tokenize(const char *source) {
    int line = 1;
    int column = 1;
    const char *p = source;
    
    tokens = malloc(sizeof(Token) * MAX_TOKENS);
    token_count = 0;
    
    while (*p) {
        // 跳过空白字符
        if (isspace(*p)) {
            if (*p == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
            p++;
            continue;
        }
        
        // 跳过单行注释
        if (*p == '/' && *(p + 1) == '/') {
            p += 2;
            while (*p && *p != '\n') {
                p++;
                column++;
            }
            continue;
        }
        
        // 跳过多行注释
        if (*p == '/' && *(p + 1) == '*') {
            p += 2;
            column += 2;
            while (*p && !(*p == '*' && *(p + 1) == '/')) {
                if (*p == '\n') {
                    line++;
                    column = 1;
                } else {
                    column++;
                }
                p++;
            }
            if (*p) {
                p += 2;
                column += 2;
            }
            continue;
        }
        
        // 标识符或关键字
        if (isalpha(*p) || *p == '_') {
            const char *start = p;
            int start_col = column;
            while (isalnum(*p) || *p == '_') {
                p++;
                column++;
            }
            
            char *str = strndup(start, p - start);
            if (is_keyword(str)) {
                add_token(get_keyword_type(str), str, line, start_col);
            } else {
                add_token(TOKEN_IDENTIFIER, str, line, start_col);
            }
            free(str);
            continue;
        }
        
        // 数字
        if (isdigit(*p)) {
            const char *start = p;
            int start_col = column;
            while (isdigit(*p)) {
                p++;
                column++;
            }
            
            char *str = strndup(start, p - start);
            add_token(TOKEN_NUMBER, str, line, start_col);
            free(str);
            continue;
        }
        
        // 字符串
        if (*p == '"') {
            const char *start = p + 1;
            int start_col = column;
            p++;
            column++;
            
            while (*p && *p != '"') {
                if (*p == '\\' && *(p + 1)) {
                    p += 2;
                    column += 2;
                } else {
                    p++;
                    column++;
                }
            }
            
            if (*p == '"') {
                char *str = strndup(start, p - start);
                add_token(TOKEN_STRING, str, line, start_col);
                free(str);
                p++;
                column++;
            }
            continue;
        }
        
        // 操作符和分隔符
        int start_col = column;
        switch (*p) {
            case '+':
                add_token(TOKEN_PLUS, "+", line, start_col);
                p++; column++;
                break;
            case '-':
                add_token(TOKEN_MINUS, "-", line, start_col);
                p++; column++;
                break;
            case '*':
                add_token(TOKEN_MULTIPLY, "*", line, start_col);
                p++; column++;
                break;
            case '/':
                add_token(TOKEN_DIVIDE, "/", line, start_col);
                p++; column++;
                break;
            case '%':
                add_token(TOKEN_MOD, "%", line, start_col);
                p++; column++;
                break;
            case '=':
                if (*(p + 1) == '=') {
                    add_token(TOKEN_EQUAL, "==", line, start_col);
                    p += 2; column += 2;
                } else {
                    add_token(TOKEN_ASSIGN, "=", line, start_col);
                    p++; column++;
                }
                break;
            case '!':
                if (*(p + 1) == '=') {
                    add_token(TOKEN_NOT_EQUAL, "!=", line, start_col);
                    p += 2; column += 2;
                } else {
                    add_token(TOKEN_UNKNOWN, "!", line, start_col);
                    p++; column++;
                }
                break;
            case '<':
                if (*(p + 1) == '=') {
                    add_token(TOKEN_LESS_EQUAL, "<=", line, start_col);
                    p += 2; column += 2;
                } else {
                    add_token(TOKEN_LESS, "<", line, start_col);
                    p++; column++;
                }
                break;
            case '>':
                if (*(p + 1) == '=') {
                    add_token(TOKEN_GREATER_EQUAL, ">=", line, start_col);
                    p += 2; column += 2;
                } else {
                    add_token(TOKEN_GREATER, ">", line, start_col);
                    p++; column++;
                }
                break;
            case '(':
                add_token(TOKEN_LPAREN, "(", line, start_col);
                p++; column++;
                break;
            case ')':
                add_token(TOKEN_RPAREN, ")", line, start_col);
                p++; column++;
                break;
            case '{':
                add_token(TOKEN_LBRACE, "{", line, start_col);
                p++; column++;
                break;
            case '}':
                add_token(TOKEN_RBRACE, "}", line, start_col);
                p++; column++;
                break;
            case ';':
                add_token(TOKEN_SEMICOLON, ";", line, start_col);
                p++; column++;
                break;
            case ',':
                add_token(TOKEN_COMMA, ",", line, start_col);
                p++; column++;
                break;
            default:
                fprintf(stderr, "未知字符: %c (行 %d, 列 %d)\n", *p, line, column);
                p++; column++;
                break;
        }
    }
    
    add_token(TOKEN_EOF, NULL, line, column);
}

// ====================================
// 解析器
// ====================================

static Token* current() {
    if (current_token < token_count) {
        return &tokens[current_token];
    }
    return NULL;
}

static Token* peek(int offset) {
    int pos = current_token + offset;
    if (pos < token_count) {
        return &tokens[pos];
    }
    return NULL;
}

static void advance() {
    if (current_token < token_count) {
        current_token++;
    }
}

static bool match(TokenType type) {
    Token *token = current();
    if (token && token->type == type) {
        advance();
        return true;
    }
    return false;
}

static bool check(TokenType type) {
    Token *token = current();
    return token && token->type == type;
}

static void error(const char *msg) {
    Token *token = current();
    if (token) {
        fprintf(stderr, "错误 (行 %d, 列 %d): %s\n", token->line, token->column, msg);
    } else {
        fprintf(stderr, "错误: %s\n", msg);
    }
    exit(1);
}

// 前向声明
static ASTNode* parse_expression();
static ASTNode* parse_statement();
static ASTNode* parse_declaration();

// 创建AST节点
static ASTNode* create_node(NodeType type) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = type;
    Token *token = current();
    if (token) {
        node->line = token->line;
        node->column = token->column;
    }
    return node;
}

// 解析基础表达式
static ASTNode* parse_primary() {
    Token *token = current();
    
    if (match(TOKEN_NUMBER)) {
        ASTNode *node = create_node(NODE_INTEGER_LITERAL);
        node->data.integer.value = strtoll(token->value, NULL, 0);
        return node;
    }
    
    if (match(TOKEN_IDENTIFIER)) {
        ASTNode *node = create_node(NODE_IDENTIFIER);
        node->data.identifier.name = strdup(token->value);
        return node;
    }
    
    if (match(TOKEN_STRING)) {
        ASTNode *node = create_node(NODE_STRING_LITERAL);
        node->data.string.value = strdup(token->value);
        return node;
    }
    
    if (match(TOKEN_LPAREN)) {
        ASTNode *expr = parse_expression();
        if (!match(TOKEN_RPAREN)) {
            error("期望 ')'");
        }
        return expr;
    }
    
    error("期望表达式");
    return NULL;
}

// 解析乘除表达式
static ASTNode* parse_multiplicative() {
    ASTNode *left = parse_primary();
    
    while (check(TOKEN_MULTIPLY) || check(TOKEN_DIVIDE) || check(TOKEN_MOD)) {
        Token *op_token = current();
        advance();
        
        ASTNode *right = parse_primary();
        ASTNode *node = create_node(NODE_BINARY_OP);
        
        switch (op_token->type) {
            case TOKEN_MULTIPLY: node->data.binary_op.op = OP_MUL; break;
            case TOKEN_DIVIDE: node->data.binary_op.op = OP_DIV; break;
            case TOKEN_MOD: node->data.binary_op.op = OP_MOD; break;
            default: break;
        }
        
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
        left = node;
    }
    
    return left;
}

// 解析加减表达式
static ASTNode* parse_additive() {
    ASTNode *left = parse_multiplicative();
    
    while (check(TOKEN_PLUS) || check(TOKEN_MINUS)) {
        Token *op_token = current();
        advance();
        
        ASTNode *right = parse_multiplicative();
        ASTNode *node = create_node(NODE_BINARY_OP);
        
        node->data.binary_op.op = (op_token->type == TOKEN_PLUS) ? OP_ADD : OP_SUB;
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
        left = node;
    }
    
    return left;
}

// 解析比较表达式
static ASTNode* parse_relational() {
    ASTNode *left = parse_additive();
    
    while (check(TOKEN_LESS) || check(TOKEN_GREATER) || 
           check(TOKEN_LESS_EQUAL) || check(TOKEN_GREATER_EQUAL)) {
        Token *op_token = current();
        advance();
        
        ASTNode *right = parse_additive();
        ASTNode *node = create_node(NODE_BINARY_OP);
        
        switch (op_token->type) {
            case TOKEN_LESS: node->data.binary_op.op = OP_LT; break;
            case TOKEN_GREATER: node->data.binary_op.op = OP_GT; break;
            case TOKEN_LESS_EQUAL: node->data.binary_op.op = OP_LE; break;
            case TOKEN_GREATER_EQUAL: node->data.binary_op.op = OP_GE; break;
            default: break;
        }
        
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
        left = node;
    }
    
    return left;
}

// 解析相等表达式
static ASTNode* parse_equality() {
    ASTNode *left = parse_relational();
    
    while (check(TOKEN_EQUAL) || check(TOKEN_NOT_EQUAL)) {
        Token *op_token = current();
        advance();
        
        ASTNode *right = parse_relational();
        ASTNode *node = create_node(NODE_BINARY_OP);
        
        node->data.binary_op.op = (op_token->type == TOKEN_EQUAL) ? OP_EQ : OP_NE;
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
        left = node;
    }
    
    return left;
}

// 解析赋值表达式
static ASTNode* parse_assignment() {
    ASTNode *left = parse_equality();
    
    if (match(TOKEN_ASSIGN)) {
        ASTNode *right = parse_assignment();
        ASTNode *node = create_node(NODE_BINARY_OP);
        node->data.binary_op.op = OP_ASSIGN;
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
        return node;
    }
    
    return left;
}

// 解析表达式
static ASTNode* parse_expression() {
    return parse_assignment();
}

// 解析类型说明符
static ASTNode* parse_type_specifier() {
    ASTNode *node = create_node(NODE_TYPE_SPECIFIER);
    
    if (match(TOKEN_VOID)) {
        node->data.type_spec.basic_type = TYPE_VOID;
    } else if (match(TOKEN_CHAR)) {
        node->data.type_spec.basic_type = TYPE_CHAR;
    } else if (match(TOKEN_INT)) {
        node->data.type_spec.basic_type = TYPE_INT;
    } else {
        error("期望类型说明符");
    }
    
    return node;
}

// 解析语句
static ASTNode* parse_statement() {
    // return语句
    if (match(TOKEN_RETURN)) {
        ASTNode *node = create_node(NODE_RETURN_STMT);
        if (!check(TOKEN_SEMICOLON)) {
            node->data.return_stmt.value = parse_expression();
        }
        if (!match(TOKEN_SEMICOLON)) {
            error("期望 ';'");
        }
        return node;
    }
    
    // if语句
    if (match(TOKEN_IF)) {
        ASTNode *node = create_node(NODE_IF_STMT);
        if (!match(TOKEN_LPAREN)) {
            error("期望 '('");
        }
        node->data.if_stmt.condition = parse_expression();
        if (!match(TOKEN_RPAREN)) {
            error("期望 ')'");
        }
        node->data.if_stmt.then_stmt = parse_statement();
        if (match(TOKEN_ELSE)) {
            node->data.if_stmt.else_stmt = parse_statement();
        }
        return node;
    }
    
    // while语句
    if (match(TOKEN_WHILE)) {
        ASTNode *node = create_node(NODE_WHILE_STMT);
        if (!match(TOKEN_LPAREN)) {
            error("期望 '('");
        }
        node->data.while_stmt.condition = parse_expression();
        if (!match(TOKEN_RPAREN)) {
            error("期望 ')'");
        }
        node->data.while_stmt.body = parse_statement();
        return node;
    }
    
    // 复合语句
    if (match(TOKEN_LBRACE)) {
        ASTNode *node = create_node(NODE_COMPOUND_STMT);
        
        // 动态数组存储语句
        int capacity = 16;
        node->data.compound.statements = malloc(sizeof(ASTNode*) * capacity);
        node->data.compound.count = 0;
        
        while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
            if (node->data.compound.count >= capacity) {
                capacity *= 2;
                node->data.compound.statements = realloc(
                    node->data.compound.statements,
                    sizeof(ASTNode*) * capacity
                );
            }
            
            // 解析声明或语句
            if (check(TOKEN_INT) || check(TOKEN_CHAR) || check(TOKEN_VOID)) {
                node->data.compound.statements[node->data.compound.count++] = parse_declaration();
            } else {
                node->data.compound.statements[node->data.compound.count++] = parse_statement();
            }
        }
        
        if (!match(TOKEN_RBRACE)) {
            error("期望 '}'");
        }
        
        return node;
    }
    
    // 表达式语句
    ASTNode *expr = parse_expression();
    if (!match(TOKEN_SEMICOLON)) {
        error("期望 ';'");
    }
    
    ASTNode *node = create_node(NODE_EXPRESSION_STMT);
    node->data.return_stmt.value = expr; // 复用return_stmt结构
    return node;
}

// 解析变量声明
static ASTNode* parse_var_declaration() {
    ASTNode *type = parse_type_specifier();
    
    if (!check(TOKEN_IDENTIFIER)) {
        error("期望标识符");
    }
    
    Token *name_token = current();
    advance();
    
    ASTNode *node = create_node(NODE_VAR_DECL);
    node->data.var_decl.name = strdup(name_token->value);
    node->data.var_decl.type = type;
    
    if (match(TOKEN_ASSIGN)) {
        node->data.var_decl.init = parse_expression();
    }
    
    if (!match(TOKEN_SEMICOLON)) {
        error("期望 ';'");
    }
    
    return node;
}

// 解析函数声明
static ASTNode* parse_function_declaration() {
    ASTNode *return_type = parse_type_specifier();
    
    if (!check(TOKEN_IDENTIFIER)) {
        error("期望函数名");
    }
    
    Token *name_token = current();
    advance();
    
    if (!match(TOKEN_LPAREN)) {
        error("期望 '('");
    }
    
    // TODO: 解析参数列表
    
    if (!match(TOKEN_RPAREN)) {
        error("期望 ')'");
    }
    
    ASTNode *node = create_node(NODE_FUNCTION_DECL);
    node->data.function.name = strdup(name_token->value);
    node->data.function.return_type = return_type;
    
    // 解析函数体
    node->data.function.body = parse_statement();
    
    return node;
}

// 解析声明
static ASTNode* parse_declaration() {
    // 预读以判断是函数还是变量
    Token *type_token = current();
    if (!type_token) {
        error("期望声明");
    }
    
    // 检查是否为函数声明
    if (peek(1) && peek(1)->type == TOKEN_IDENTIFIER &&
        peek(2) && peek(2)->type == TOKEN_LPAREN) {
        return parse_function_declaration();
    }
    
    // 否则是变量声明
    return parse_var_declaration();
}

// 解析翻译单元
static ASTNode* parse_translation_unit() {
    ASTNode *node = create_node(NODE_TRANSLATION_UNIT);
    
    int capacity = 16;
    node->data.translation_unit.declarations = malloc(sizeof(ASTNode*) * capacity);
    node->data.translation_unit.count = 0;
    
    while (!check(TOKEN_EOF)) {
        if (node->data.translation_unit.count >= capacity) {
            capacity *= 2;
            node->data.translation_unit.declarations = realloc(
                node->data.translation_unit.declarations,
                sizeof(ASTNode*) * capacity
            );
        }
        
        node->data.translation_unit.declarations[node->data.translation_unit.count++] = 
            parse_declaration();
    }
    
    return node;
}

// ====================================
// 代码生成
// ====================================

typedef struct {
    uint8_t *code;
    size_t size;
    size_t capacity;
    
    // 符号表
    struct {
        char *name;
        int offset;
    } locals[256];
    int local_count;
    int stack_offset;
} CodeGen;

static CodeGen* create_codegen() {
    CodeGen *gen = calloc(1, sizeof(CodeGen));
    gen->capacity = 4096;
    gen->code = malloc(gen->capacity);
    return gen;
}

static void emit(CodeGen *gen, uint8_t byte) {
    if (gen->size >= gen->capacity) {
        gen->capacity *= 2;
        gen->code = realloc(gen->code, gen->capacity);
    }
    gen->code[gen->size++] = byte;
}

static void emit_bytes(CodeGen *gen, const uint8_t *bytes, size_t count) {
    for (size_t i = 0; i < count; i++) {
        emit(gen, bytes[i]);
    }
}

static void emit_int32(CodeGen *gen, int32_t value) {
    emit(gen, value & 0xFF);
    emit(gen, (value >> 8) & 0xFF);
    emit(gen, (value >> 16) & 0xFF);
    emit(gen, (value >> 24) & 0xFF);
}

static void emit_int64(CodeGen *gen, int64_t value) {
    emit_int32(gen, value & 0xFFFFFFFF);
    emit_int32(gen, (value >> 32) & 0xFFFFFFFF);
}

// x86-64 指令编码
static void emit_push_rax(CodeGen *gen) {
    emit(gen, 0x50);
}

static void emit_pop_rax(CodeGen *gen) {
    emit(gen, 0x58);
}

static void emit_pop_rbx(CodeGen *gen) {
    emit(gen, 0x5B);
}

static void emit_mov_rax_imm64(CodeGen *gen, int64_t value) {
    emit(gen, 0x48);  // REX.W
    emit(gen, 0xB8);  // MOV rax, imm64
    emit_int64(gen, value);
}

static void emit_add_rax_rbx(CodeGen *gen) {
    emit(gen, 0x48);  // REX.W
    emit(gen, 0x01);  // ADD r/m64, r64
    emit(gen, 0xD8);  // ModRM: rax += rbx
}

static void emit_sub_rax_rbx(CodeGen *gen) {
    emit(gen, 0x48);  // REX.W
    emit(gen, 0x29);  // SUB r/m64, r64
    emit(gen, 0xD8);  // ModRM: rax -= rbx
}

static void emit_imul_rax_rbx(CodeGen *gen) {
    emit(gen, 0x48);  // REX.W
    emit(gen, 0x0F);  // Two-byte opcode
    emit(gen, 0xAF);  // IMUL r64, r/m64
    emit(gen, 0xC3);  // ModRM: rax *= rbx
}

static void emit_cmp_rax_rbx(CodeGen *gen) {
    emit(gen, 0x48);  // REX.W
    emit(gen, 0x39);  // CMP r/m64, r64
    emit(gen, 0xD8);  // ModRM: cmp rax, rbx
}

static void emit_sete_al(CodeGen *gen) {
    emit(gen, 0x0F);  // Two-byte opcode
    emit(gen, 0x94);  // SETE r/m8
    emit(gen, 0xC0);  // ModRM: al
}

static void emit_setne_al(CodeGen *gen) {
    emit(gen, 0x0F);  // Two-byte opcode
    emit(gen, 0x95);  // SETNE r/m8
    emit(gen, 0xC0);  // ModRM: al
}

static void emit_setl_al(CodeGen *gen) {
    emit(gen, 0x0F);  // Two-byte opcode
    emit(gen, 0x9C);  // SETL r/m8
    emit(gen, 0xC0);  // ModRM: al
}

static void emit_setg_al(CodeGen *gen) {
    emit(gen, 0x0F);  // Two-byte opcode
    emit(gen, 0x9F);  // SETG r/m8
    emit(gen, 0xC0);  // ModRM: al
}

static void emit_setle_al(CodeGen *gen) {
    emit(gen, 0x0F);  // Two-byte opcode
    emit(gen, 0x9E);  // SETLE r/m8
    emit(gen, 0xC0);  // ModRM: al
}

static void emit_setge_al(CodeGen *gen) {
    emit(gen, 0x0F);  // Two-byte opcode
    emit(gen, 0x9D);  // SETGE r/m8
    emit(gen, 0xC0);  // ModRM: al
}

static void emit_movzx_rax_al(CodeGen *gen) {
    emit(gen, 0x48);  // REX.W
    emit(gen, 0x0F);  // Two-byte opcode
    emit(gen, 0xB6);  // MOVZX r64, r/m8
    emit(gen, 0xC0);  // ModRM: rax, al
}

static void emit_syscall(CodeGen *gen) {
    emit(gen, 0x0F);
    emit(gen, 0x05);
}

// 前向声明
static void generate_expression(CodeGen *gen, ASTNode *node);
static void generate_statement(CodeGen *gen, ASTNode *node);

// 生成二元操作代码
static void generate_binary_op(CodeGen *gen, ASTNode *node) {
    // 生成左操作数
    generate_expression(gen, node->data.binary_op.left);
    emit_push_rax(gen);
    
    // 生成右操作数
    generate_expression(gen, node->data.binary_op.right);
    
    // 右操作数在 rax, 左操作数在栈上
    emit_pop_rbx(gen);  // 左操作数 -> rbx
    
    switch (node->data.binary_op.op) {
        case OP_ADD:
            emit_add_rax_rbx(gen);  // rax = rbx + rax
            break;
            
        case OP_SUB:
            // 需要交换操作数: rax = rbx - rax
            emit(gen, 0x48);  // REX.W
            emit(gen, 0x93);  // XCHG rax, rbx
            emit_sub_rax_rbx(gen);
            break;
            
        case OP_MUL:
            emit_imul_rax_rbx(gen);
            break;
            
        case OP_EQ:
            emit_cmp_rax_rbx(gen);
            emit_sete_al(gen);
            emit_movzx_rax_al(gen);
            break;
            
        case OP_NE:
            emit_cmp_rax_rbx(gen);
            emit_setne_al(gen);
            emit_movzx_rax_al(gen);
            break;
            
        case OP_LT:
            // 需要交换操作数
            emit(gen, 0x48);  // REX.W
            emit(gen, 0x93);  // XCHG rax, rbx
            emit_cmp_rax_rbx(gen);
            emit_setl_al(gen);
            emit_movzx_rax_al(gen);
            break;
            
        case OP_GT:
            // 需要交换操作数
            emit(gen, 0x48);  // REX.W
            emit(gen, 0x93);  // XCHG rax, rbx
            emit_cmp_rax_rbx(gen);
            emit_setg_al(gen);
            emit_movzx_rax_al(gen);
            break;
            
        case OP_LE:
            // 需要交换操作数
            emit(gen, 0x48);  // REX.W
            emit(gen, 0x93);  // XCHG rax, rbx
            emit_cmp_rax_rbx(gen);
            emit_setle_al(gen);
            emit_movzx_rax_al(gen);
            break;
            
        case OP_GE:
            // 需要交换操作数
            emit(gen, 0x48);  // REX.W
            emit(gen, 0x93);  // XCHG rax, rbx
            emit_cmp_rax_rbx(gen);
            emit_setge_al(gen);
            emit_movzx_rax_al(gen);
            break;
            
        default:
            fprintf(stderr, "不支持的二元操作: %d\n", node->data.binary_op.op);
            break;
    }
}

// 生成表达式代码
static void generate_expression(CodeGen *gen, ASTNode *node) {
    switch (node->type) {
        case NODE_INTEGER_LITERAL:
            emit_mov_rax_imm64(gen, node->data.integer.value);
            break;
            
        case NODE_IDENTIFIER:
            // TODO: 实现变量访问
            emit_mov_rax_imm64(gen, 0);
            break;
            
        case NODE_BINARY_OP:
            generate_binary_op(gen, node);
            break;
            
        default:
            fprintf(stderr, "不支持的表达式类型: %d\n", node->type);
            break;
    }
}

// 生成语句代码
static void generate_statement(CodeGen *gen, ASTNode *node) {
    switch (node->type) {
        case NODE_RETURN_STMT:
            if (node->data.return_stmt.value) {
                generate_expression(gen, node->data.return_stmt.value);
            } else {
                emit_mov_rax_imm64(gen, 0);
            }
            
            // 对于main函数，使用syscall退出
            emit(gen, 0x48);  // REX.W
            emit(gen, 0x89);  // MOV r/m64, r64
            emit(gen, 0xC7);  // ModRM: rdi = rax (退出码)
            
            emit_mov_rax_imm64(gen, 60);  // sys_exit
            emit_syscall(gen);
            break;
            
        case NODE_COMPOUND_STMT:
            for (int i = 0; i < node->data.compound.count; i++) {
                generate_statement(gen, node->data.compound.statements[i]);
            }
            break;
            
        case NODE_EXPRESSION_STMT:
            if (node->data.return_stmt.value) {
                generate_expression(gen, node->data.return_stmt.value);
            }
            break;
            
        case NODE_IF_STMT: {
            // 生成条件
            generate_expression(gen, node->data.if_stmt.condition);
            
            // 测试结果
            emit(gen, 0x48);  // REX.W
            emit(gen, 0x85);  // TEST r/m64, r64
            emit(gen, 0xC0);  // ModRM: test rax, rax
            
            // JZ到else分支或结束
            emit(gen, 0x0F);  // Two-byte opcode
            emit(gen, 0x84);  // JZ rel32
            size_t jz_offset = gen->size;
            emit_int32(gen, 0);  // 占位符
            
            // then分支
            generate_statement(gen, node->data.if_stmt.then_stmt);
            
            if (node->data.if_stmt.else_stmt) {
                // JMP到结束
                emit(gen, 0xE9);  // JMP rel32
                size_t jmp_offset = gen->size;
                emit_int32(gen, 0);  // 占位符
                
                // 修正JZ目标
                int32_t jz_target = gen->size - (jz_offset + 4);
                memcpy(gen->code + jz_offset, &jz_target, 4);
                
                // else分支
                generate_statement(gen, node->data.if_stmt.else_stmt);
                
                // 修正JMP目标
                int32_t jmp_target = gen->size - (jmp_offset + 4);
                memcpy(gen->code + jmp_offset, &jmp_target, 4);
            } else {
                // 修正JZ目标
                int32_t jz_target = gen->size - (jz_offset + 4);
                memcpy(gen->code + jz_offset, &jz_target, 4);
            }
            break;
        }
            
        case NODE_WHILE_STMT: {
            // 循环开始
            size_t loop_start = gen->size;
            
            // 生成条件
            generate_expression(gen, node->data.while_stmt.condition);
            
            // 测试结果
            emit(gen, 0x48);  // REX.W
            emit(gen, 0x85);  // TEST r/m64, r64
            emit(gen, 0xC0);  // ModRM: test rax, rax
            
            // JZ到循环结束
            emit(gen, 0x0F);  // Two-byte opcode
            emit(gen, 0x84);  // JZ rel32
            size_t jz_offset = gen->size;
            emit_int32(gen, 0);  // 占位符
            
            // 循环体
            generate_statement(gen, node->data.while_stmt.body);
            
            // JMP回循环开始
            emit(gen, 0xE9);  // JMP rel32
            int32_t jmp_target = loop_start - (gen->size + 4);
            emit_int32(gen, jmp_target);
            
            // 修正JZ目标
            int32_t jz_target = gen->size - (jz_offset + 4);
            memcpy(gen->code + jz_offset, &jz_target, 4);
            break;
        }
            
        default:
            break;
    }
}

// 生成函数代码
static void generate_function(CodeGen *gen, ASTNode *node) {
    if (node->data.function.body) {
        generate_statement(gen, node->data.function.body);
    }
}

// 生成整个程序
static int generate_code(ASTNode *ast, uint8_t **out_code, size_t *out_size) {
    CodeGen *gen = create_codegen();
    
    // 查找main函数
    int entry_offset = 0;
    bool found_main = false;
    
    for (int i = 0; i < ast->data.translation_unit.count; i++) {
        ASTNode *decl = ast->data.translation_unit.declarations[i];
        
        if (decl->type == NODE_FUNCTION_DECL &&
            strcmp(decl->data.function.name, "main") == 0) {
            entry_offset = gen->size;
            generate_function(gen, decl);
            found_main = true;
            break;
        }
    }
    
    if (!found_main) {
        fprintf(stderr, "错误：未找到main函数\n");
        free(gen->code);
        free(gen);
        return -1;
    }
    
    *out_code = gen->code;
    *out_size = gen->size;
    
    free(gen);
    return entry_offset;
}

// ====================================
// ELF生成
// ====================================

#define ELF_MAGIC 0x464C457F

typedef struct {
    uint32_t magic;
    uint8_t class;
    uint8_t data;
    uint8_t version;
    uint8_t osabi;
    uint8_t abiversion;
    uint8_t pad[7];
} __attribute__((packed)) ElfIdent;

typedef struct {
    ElfIdent ident;
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint64_t entry;
    uint64_t phoff;
    uint64_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
} __attribute__((packed)) Elf64Header;

typedef struct {
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t vaddr;
    uint64_t paddr;
    uint64_t filesz;
    uint64_t memsz;
    uint64_t align;
} __attribute__((packed)) Elf64Phdr;

static int write_elf(const char *filename, uint8_t *code, size_t code_size, int entry_offset) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("无法创建输出文件");
        return -1;
    }
    
    const uint64_t base_addr = 0x400000;
    size_t headers_size = sizeof(Elf64Header) + sizeof(Elf64Phdr);
    
    // ELF头
    Elf64Header ehdr = {
        .ident = {
            .magic = ELF_MAGIC,
            .class = 2,      // 64-bit
            .data = 1,       // little-endian
            .version = 1,    // current
            .osabi = 0,      // System V
        },
        .type = 2,           // ET_EXEC
        .machine = 62,       // EM_X86_64
        .version = 1,        // current
        .entry = base_addr + headers_size + entry_offset,
        .phoff = sizeof(Elf64Header),
        .shoff = 0,
        .flags = 0,
        .ehsize = sizeof(Elf64Header),
        .phentsize = sizeof(Elf64Phdr),
        .phnum = 1,
        .shentsize = 0,
        .shnum = 0,
        .shstrndx = 0,
    };
    
    // 程序头
    Elf64Phdr phdr = {
        .type = 1,           // PT_LOAD
        .flags = 5,          // PF_X | PF_R
        .offset = 0,
        .vaddr = base_addr,
        .paddr = base_addr,
        .filesz = headers_size + code_size,
        .memsz = headers_size + code_size,
        .align = 4096,
    };
    
    fwrite(&ehdr, sizeof(ehdr), 1, f);
    fwrite(&phdr, sizeof(phdr), 1, f);
    fwrite(code, code_size, 1, f);
    
    fclose(f);
    
    if (chmod(filename, 0755) != 0) {
        perror("无法设置可执行权限");
        return -1;
    }
    
    return 0;
}

// ====================================
// 主函数
// ====================================

static void free_ast(ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case NODE_IDENTIFIER:
            free(node->data.identifier.name);
            break;
            
        case NODE_STRING_LITERAL:
            free(node->data.string.value);
            break;
            
        case NODE_BINARY_OP:
            free_ast(node->data.binary_op.left);
            free_ast(node->data.binary_op.right);
            break;
            
        case NODE_UNARY_OP:
            free_ast(node->data.unary_op.operand);
            break;
            
        case NODE_FUNCTION_DECL:
            free(node->data.function.name);
            free_ast(node->data.function.return_type);
            free_ast(node->data.function.body);
            break;
            
        case NODE_VAR_DECL:
            free(node->data.var_decl.name);
            free_ast(node->data.var_decl.type);
            free_ast(node->data.var_decl.init);
            break;
            
        case NODE_COMPOUND_STMT:
            for (int i = 0; i < node->data.compound.count; i++) {
                free_ast(node->data.compound.statements[i]);
            }
            free(node->data.compound.statements);
            break;
            
        case NODE_RETURN_STMT:
        case NODE_EXPRESSION_STMT:
            free_ast(node->data.return_stmt.value);
            break;
            
        case NODE_IF_STMT:
            free_ast(node->data.if_stmt.condition);
            free_ast(node->data.if_stmt.then_stmt);
            free_ast(node->data.if_stmt.else_stmt);
            break;
            
        case NODE_WHILE_STMT:
            free_ast(node->data.while_stmt.condition);
            free_ast(node->data.while_stmt.body);
            break;
            
        case NODE_TRANSLATION_UNIT:
            for (int i = 0; i < node->data.translation_unit.count; i++) {
                free_ast(node->data.translation_unit.declarations[i]);
            }
            free(node->data.translation_unit.declarations);
            break;
            
        default:
            break;
    }
    
    free(node);
}

static void print_usage(const char *program_name) {
    printf("用法: %s <源文件> [-o <输出文件>]\n", program_name);
    printf("  编译C源文件生成可执行文件\n");
    printf("选项:\n");
    printf("  -o <文件>    指定输出文件名 (默认: a.out)\n");
    printf("  -h           显示此帮助信息\n");
}

int main(int argc, char *argv[]) {
    const char *input_file = NULL;
    const char *output_file = "a.out";
    
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                output_file = argv[++i];
            } else {
                fprintf(stderr, "错误: -o 需要参数\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "错误: 未知选项 '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        } else {
            if (input_file == NULL) {
                input_file = argv[i];
            } else {
                fprintf(stderr, "错误: 只能指定一个输入文件\n");
                return 1;
            }
        }
    }
    
    if (!input_file) {
        fprintf(stderr, "错误: 未指定输入文件\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // 读取源文件
    FILE *f = fopen(input_file, "r");
    if (!f) {
        perror("无法打开输入文件");
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *source = malloc(size + 1);
    fread(source, 1, size, f);
    source[size] = '\0';
    fclose(f);
    
    printf("=== evolver0 编译器 (改进版) ===\n");
    printf("编译: %s -> %s\n", input_file, output_file);
    
    // 词法分析
    printf("词法分析...\n");
    tokenize(source);
    free(source);
    
    // 语法分析
    printf("语法分析...\n");
    current_token = 0;
    ASTNode *ast = parse_translation_unit();
    
    // 代码生成
    printf("代码生成...\n");
    uint8_t *code;
    size_t code_size;
    int entry_offset = generate_code(ast, &code, &code_size);
    
    if (entry_offset < 0) {
        fprintf(stderr, "代码生成失败\n");
        free_ast(ast);
        return 1;
    }
    
    printf("生成了 %zu 字节的机器码\n", code_size);
    
    // 生成ELF文件
    printf("生成可执行文件...\n");
    if (write_elf(output_file, code, code_size, entry_offset) != 0) {
        fprintf(stderr, "生成可执行文件失败\n");
        free(code);
        free_ast(ast);
        return 1;
    }
    
    printf("✓ 成功生成可执行文件: %s\n", output_file);
    
    // 清理
    free(code);
    free_ast(ast);
    
    // 释放token
    for (int i = 0; i < token_count; i++) {
        free(tokens[i].value);
    }
    free(tokens);
    
    return 0;
}