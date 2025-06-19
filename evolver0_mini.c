/**
 * evolver0_mini.c - 最小化的C编译器
 * 支持基本的int main函数、return语句、算术表达式和控制流
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <elf.h>
#include <sys/stat.h>

// ====================================
// 词法分析器
// ====================================

typedef enum {
    TOK_EOF,
    TOK_INT,
    TOK_RETURN,
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_FOR,
    TOK_IDENTIFIER,
    TOK_NUMBER,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_ASSIGN,
    TOK_EQ,
    TOK_NE,
    TOK_LT,
    TOK_GT,
    TOK_LE,
    TOK_GE,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_SEMICOLON,
    TOK_COMMA
} TokenType;

typedef struct {
    TokenType type;
    char *text;
    int value;
    int line;
    int column;
} Token;

typedef struct {
    const char *input;
    int pos;
    int line;
    int column;
    Token current;
} Lexer;

static void lexer_init(Lexer *lex, const char *input) {
    lex->input = input;
    lex->pos = 0;
    lex->line = 1;
    lex->column = 1;
}

static void skip_whitespace(Lexer *lex) {
    while (lex->input[lex->pos] && isspace(lex->input[lex->pos])) {
        if (lex->input[lex->pos] == '\n') {
            lex->line++;
            lex->column = 1;
        } else {
            lex->column++;
        }
        lex->pos++;
    }
    
    // 跳过注释
    if (lex->input[lex->pos] == '/' && lex->input[lex->pos + 1] == '/') {
        while (lex->input[lex->pos] && lex->input[lex->pos] != '\n') {
            lex->pos++;
        }
    }
}

static Token next_token(Lexer *lex) {
    skip_whitespace(lex);
    
    Token tok = {0};
    tok.line = lex->line;
    tok.column = lex->column;
    
    if (!lex->input[lex->pos]) {
        tok.type = TOK_EOF;
        return tok;
    }
    
    // 数字
    if (isdigit(lex->input[lex->pos])) {
        int start = lex->pos;
        int value = 0;
        while (isdigit(lex->input[lex->pos])) {
            value = value * 10 + (lex->input[lex->pos] - '0');
            lex->pos++;
            lex->column++;
        }
        tok.type = TOK_NUMBER;
        tok.value = value;
        int len = lex->pos - start;
        tok.text = malloc(len + 1);
        memcpy(tok.text, &lex->input[start], len);
        tok.text[len] = '\0';
        return tok;
    }
    
    // 标识符和关键字
    if (isalpha(lex->input[lex->pos]) || lex->input[lex->pos] == '_') {
        int start = lex->pos;
        while (isalnum(lex->input[lex->pos]) || lex->input[lex->pos] == '_') {
            lex->pos++;
            lex->column++;
        }
        int len = lex->pos - start;
        tok.text = malloc(len + 1);
        memcpy(tok.text, &lex->input[start], len);
        tok.text[len] = '\0';
        
        // 检查关键字
        if (strcmp(tok.text, "int") == 0) tok.type = TOK_INT;
        else if (strcmp(tok.text, "return") == 0) tok.type = TOK_RETURN;
        else if (strcmp(tok.text, "if") == 0) tok.type = TOK_IF;
        else if (strcmp(tok.text, "else") == 0) tok.type = TOK_ELSE;
        else if (strcmp(tok.text, "while") == 0) tok.type = TOK_WHILE;
        else if (strcmp(tok.text, "for") == 0) tok.type = TOK_FOR;
        else tok.type = TOK_IDENTIFIER;
        
        return tok;
    }
    
    // 双字符操作符
    if (lex->input[lex->pos] == '=' && lex->input[lex->pos + 1] == '=') {
        tok.type = TOK_EQ;
        tok.text = strdup("==");
        lex->pos += 2;
        lex->column += 2;
        return tok;
    }
    if (lex->input[lex->pos] == '!' && lex->input[lex->pos + 1] == '=') {
        tok.type = TOK_NE;
        tok.text = strdup("!=");
        lex->pos += 2;
        lex->column += 2;
        return tok;
    }
    if (lex->input[lex->pos] == '<' && lex->input[lex->pos + 1] == '=') {
        tok.type = TOK_LE;
        tok.text = strdup("<=");
        lex->pos += 2;
        lex->column += 2;
        return tok;
    }
    if (lex->input[lex->pos] == '>' && lex->input[lex->pos + 1] == '=') {
        tok.type = TOK_GE;
        tok.text = strdup(">=");
        lex->pos += 2;
        lex->column += 2;
        return tok;
    }
    
    // 单字符操作符
    char c = lex->input[lex->pos++];
    lex->column++;
    tok.text = malloc(2);
    tok.text[0] = c;
    tok.text[1] = '\0';
    
    switch (c) {
        case '+': tok.type = TOK_PLUS; break;
        case '-': tok.type = TOK_MINUS; break;
        case '*': tok.type = TOK_STAR; break;
        case '/': tok.type = TOK_SLASH; break;
        case '=': tok.type = TOK_ASSIGN; break;
        case '<': tok.type = TOK_LT; break;
        case '>': tok.type = TOK_GT; break;
        case '(': tok.type = TOK_LPAREN; break;
        case ')': tok.type = TOK_RPAREN; break;
        case '{': tok.type = TOK_LBRACE; break;
        case '}': tok.type = TOK_RBRACE; break;
        case ';': tok.type = TOK_SEMICOLON; break;
        case ',': tok.type = TOK_COMMA; break;
        default:
            fprintf(stderr, "未知字符: %c\n", c);
            exit(1);
    }
    
    return tok;
}

static Token peek_token(Lexer *lex) {
    return lex->current;
}

static Token consume_token(Lexer *lex) {
    Token tok = lex->current;
    lex->current = next_token(lex);
    return tok;
}

// ====================================
// AST节点
// ====================================

typedef enum {
    NODE_PROGRAM,
    NODE_FUNCTION,
    NODE_COMPOUND,
    NODE_RETURN,
    NODE_IF,
    NODE_WHILE,
    NODE_FOR,
    NODE_DECLARATION,
    NODE_ASSIGN,
    NODE_BINARY,
    NODE_UNARY,
    NODE_NUMBER,
    NODE_IDENTIFIER,
    NODE_EXPRESSION_STMT
} NodeType;

typedef struct ASTNode {
    NodeType type;
    union {
        // PROGRAM
        struct {
            struct ASTNode **functions;
            int count;
        } program;
        
        // FUNCTION
        struct {
            char *name;
            struct ASTNode *body;
        } function;
        
        // COMPOUND
        struct {
            struct ASTNode **stmts;
            int count;
        } compound;
        
        // RETURN
        struct {
            struct ASTNode *value;
        } ret;
        
        // IF
        struct {
            struct ASTNode *cond;
            struct ASTNode *then_stmt;
            struct ASTNode *else_stmt;
        } if_stmt;
        
        // WHILE
        struct {
            struct ASTNode *cond;
            struct ASTNode *body;
        } while_stmt;
        
        // FOR
        struct {
            struct ASTNode *init;
            struct ASTNode *cond;
            struct ASTNode *update;
            struct ASTNode *body;
        } for_stmt;
        
        // DECLARATION
        struct {
            char *name;
            struct ASTNode *init;
        } decl;
        
        // ASSIGN
        struct {
            char *name;
            struct ASTNode *value;
        } assign;
        
        // BINARY
        struct {
            TokenType op;
            struct ASTNode *left;
            struct ASTNode *right;
        } binary;
        
        // UNARY
        struct {
            TokenType op;
            struct ASTNode *operand;
        } unary;
        
        // NUMBER
        int number;
        
        // IDENTIFIER
        char *identifier;
        
        // EXPRESSION_STMT
        struct {
            struct ASTNode *expr;
        } expr_stmt;
    } data;
} ASTNode;

// ====================================
// 解析器
// ====================================

typedef struct {
    Lexer *lexer;
    Token current;
} Parser;

static void parser_init(Parser *parser, Lexer *lexer) {
    parser->lexer = lexer;
    parser->current = next_token(lexer);
    lexer->current = parser->current;
}

static bool match(Parser *parser, TokenType type) {
    if (parser->current.type == type) {
        parser->current = consume_token(parser->lexer);
        return true;
    }
    return false;
}

static void expect(Parser *parser, TokenType type, const char *msg) {
    if (!match(parser, type)) {
        fprintf(stderr, "错误: %s (行 %d, 列 %d)\n", 
                msg, parser->current.line, parser->current.column);
        exit(1);
    }
}

// 前向声明
static ASTNode *parse_expression(Parser *parser);
static ASTNode *parse_statement(Parser *parser);

// 解析基础表达式
static ASTNode *parse_primary(Parser *parser) {
    if (parser->current.type == TOK_NUMBER) {
        ASTNode *node = malloc(sizeof(ASTNode));
        node->type = NODE_NUMBER;
        node->data.number = parser->current.value;
        match(parser, TOK_NUMBER);
        return node;
    }
    
    if (parser->current.type == TOK_IDENTIFIER) {
        ASTNode *node = malloc(sizeof(ASTNode));
        node->type = NODE_IDENTIFIER;
        node->data.identifier = strdup(parser->current.text);
        match(parser, TOK_IDENTIFIER);
        return node;
    }
    
    if (match(parser, TOK_LPAREN)) {
        ASTNode *expr = parse_expression(parser);
        expect(parser, TOK_RPAREN, "期望 ')'");
        return expr;
    }
    
    fprintf(stderr, "期望表达式\n");
    exit(1);
}

// 解析一元表达式
static ASTNode *parse_unary(Parser *parser) {
    if (parser->current.type == TOK_MINUS) {
        TokenType op = parser->current.type;
        match(parser, op);
        ASTNode *node = malloc(sizeof(ASTNode));
        node->type = NODE_UNARY;
        node->data.unary.op = op;
        node->data.unary.operand = parse_unary(parser);
        return node;
    }
    
    return parse_primary(parser);
}

// 解析乘除表达式
static ASTNode *parse_multiplicative(Parser *parser) {
    ASTNode *left = parse_unary(parser);
    
    while (parser->current.type == TOK_STAR || parser->current.type == TOK_SLASH) {
        TokenType op = parser->current.type;
        match(parser, op);
        ASTNode *node = malloc(sizeof(ASTNode));
        node->type = NODE_BINARY;
        node->data.binary.op = op;
        node->data.binary.left = left;
        node->data.binary.right = parse_unary(parser);
        left = node;
    }
    
    return left;
}

// 解析加减表达式
static ASTNode *parse_additive(Parser *parser) {
    ASTNode *left = parse_multiplicative(parser);
    
    while (parser->current.type == TOK_PLUS || parser->current.type == TOK_MINUS) {
        TokenType op = parser->current.type;
        match(parser, op);
        ASTNode *node = malloc(sizeof(ASTNode));
        node->type = NODE_BINARY;
        node->data.binary.op = op;
        node->data.binary.left = left;
        node->data.binary.right = parse_multiplicative(parser);
        left = node;
    }
    
    return left;
}

// 解析比较表达式
static ASTNode *parse_relational(Parser *parser) {
    ASTNode *left = parse_additive(parser);
    
    while (parser->current.type == TOK_LT || parser->current.type == TOK_GT ||
           parser->current.type == TOK_LE || parser->current.type == TOK_GE) {
        TokenType op = parser->current.type;
        match(parser, op);
        ASTNode *node = malloc(sizeof(ASTNode));
        node->type = NODE_BINARY;
        node->data.binary.op = op;
        node->data.binary.left = left;
        node->data.binary.right = parse_additive(parser);
        left = node;
    }
    
    return left;
}

// 解析相等表达式
static ASTNode *parse_equality(Parser *parser) {
    ASTNode *left = parse_relational(parser);
    
    while (parser->current.type == TOK_EQ || parser->current.type == TOK_NE) {
        TokenType op = parser->current.type;
        match(parser, op);
        ASTNode *node = malloc(sizeof(ASTNode));
        node->type = NODE_BINARY;
        node->data.binary.op = op;
        node->data.binary.left = left;
        node->data.binary.right = parse_relational(parser);
        left = node;
    }
    
    return left;
}

// 解析赋值表达式
static ASTNode *parse_assignment(Parser *parser) {
    ASTNode *left = parse_equality(parser);
    
    if (parser->current.type == TOK_ASSIGN && left->type == NODE_IDENTIFIER) {
        match(parser, TOK_ASSIGN);
        ASTNode *node = malloc(sizeof(ASTNode));
        node->type = NODE_ASSIGN;
        node->data.assign.name = strdup(left->data.identifier);
        node->data.assign.value = parse_assignment(parser);
        free(left->data.identifier);
        free(left);
        return node;
    }
    
    return left;
}

// 解析表达式
static ASTNode *parse_expression(Parser *parser) {
    return parse_assignment(parser);
}

// 解析声明
static ASTNode *parse_declaration(Parser *parser) {
    expect(parser, TOK_INT, "期望 'int'");
    
    char *name = strdup(parser->current.text);
    expect(parser, TOK_IDENTIFIER, "期望标识符");
    
    ASTNode *init = NULL;
    if (match(parser, TOK_ASSIGN)) {
        init = parse_expression(parser);
    }
    
    expect(parser, TOK_SEMICOLON, "期望 ';'");
    
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_DECLARATION;
    node->data.decl.name = name;
    node->data.decl.init = init;
    
    return node;
}

// 解析复合语句
static ASTNode *parse_compound_statement(Parser *parser) {
    expect(parser, TOK_LBRACE, "期望 '{'");
    
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_COMPOUND;
    node->data.compound.stmts = malloc(sizeof(ASTNode*) * 100);
    node->data.compound.count = 0;
    
    while (parser->current.type != TOK_RBRACE && parser->current.type != TOK_EOF) {
        node->data.compound.stmts[node->data.compound.count++] = parse_statement(parser);
    }
    
    expect(parser, TOK_RBRACE, "期望 '}'");
    
    return node;
}

// 解析语句
static ASTNode *parse_statement(Parser *parser) {
    // return语句
    if (match(parser, TOK_RETURN)) {
        ASTNode *value = NULL;
        if (parser->current.type != TOK_SEMICOLON) {
            value = parse_expression(parser);
        }
        expect(parser, TOK_SEMICOLON, "期望 ';'");
        
        ASTNode *node = malloc(sizeof(ASTNode));
        node->type = NODE_RETURN;
        node->data.ret.value = value;
        return node;
    }
    
    // if语句
    if (match(parser, TOK_IF)) {
        expect(parser, TOK_LPAREN, "期望 '('");
        ASTNode *cond = parse_expression(parser);
        expect(parser, TOK_RPAREN, "期望 ')'");
        
        ASTNode *then_stmt = parse_statement(parser);
        ASTNode *else_stmt = NULL;
        
        if (match(parser, TOK_ELSE)) {
            else_stmt = parse_statement(parser);
        }
        
        ASTNode *node = malloc(sizeof(ASTNode));
        node->type = NODE_IF;
        node->data.if_stmt.cond = cond;
        node->data.if_stmt.then_stmt = then_stmt;
        node->data.if_stmt.else_stmt = else_stmt;
        return node;
    }
    
    // while语句
    if (match(parser, TOK_WHILE)) {
        expect(parser, TOK_LPAREN, "期望 '('");
        ASTNode *cond = parse_expression(parser);
        expect(parser, TOK_RPAREN, "期望 ')'");
        
        ASTNode *body = parse_statement(parser);
        
        ASTNode *node = malloc(sizeof(ASTNode));
        node->type = NODE_WHILE;
        node->data.while_stmt.cond = cond;
        node->data.while_stmt.body = body;
        return node;
    }
    
    // for语句
    if (match(parser, TOK_FOR)) {
        expect(parser, TOK_LPAREN, "期望 '('");
        
        ASTNode *init = NULL;
        if (parser->current.type == TOK_INT) {
            init = parse_declaration(parser);
        } else if (parser->current.type != TOK_SEMICOLON) {
            init = parse_expression(parser);
            expect(parser, TOK_SEMICOLON, "期望 ';'");
            ASTNode *expr_stmt = malloc(sizeof(ASTNode));
            expr_stmt->type = NODE_EXPRESSION_STMT;
            expr_stmt->data.expr_stmt.expr = init;
            init = expr_stmt;
        } else {
            match(parser, TOK_SEMICOLON);
        }
        
        ASTNode *cond = NULL;
        if (parser->current.type != TOK_SEMICOLON) {
            cond = parse_expression(parser);
        }
        expect(parser, TOK_SEMICOLON, "期望 ';'");
        
        ASTNode *update = NULL;
        if (parser->current.type != TOK_RPAREN) {
            update = parse_expression(parser);
        }
        expect(parser, TOK_RPAREN, "期望 ')'");
        
        ASTNode *body = parse_statement(parser);
        
        ASTNode *node = malloc(sizeof(ASTNode));
        node->type = NODE_FOR;
        node->data.for_stmt.init = init;
        node->data.for_stmt.cond = cond;
        node->data.for_stmt.update = update;
        node->data.for_stmt.body = body;
        return node;
    }
    
    // 复合语句
    if (parser->current.type == TOK_LBRACE) {
        return parse_compound_statement(parser);
    }
    
    // 声明
    if (parser->current.type == TOK_INT) {
        return parse_declaration(parser);
    }
    
    // 表达式语句
    ASTNode *expr = parse_expression(parser);
    expect(parser, TOK_SEMICOLON, "期望 ';'");
    
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_EXPRESSION_STMT;
    node->data.expr_stmt.expr = expr;
    return node;
}

// 解析函数
static ASTNode *parse_function(Parser *parser) {
    expect(parser, TOK_INT, "期望 'int'");
    
    char *name = strdup(parser->current.text);
    expect(parser, TOK_IDENTIFIER, "期望函数名");
    
    expect(parser, TOK_LPAREN, "期望 '('");
    expect(parser, TOK_RPAREN, "期望 ')'");
    
    ASTNode *body = parse_compound_statement(parser);
    
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_FUNCTION;
    node->data.function.name = name;
    node->data.function.body = body;
    
    return node;
}

// 解析程序
static ASTNode *parse_program(Parser *parser) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_PROGRAM;
    node->data.program.functions = malloc(sizeof(ASTNode*) * 10);
    node->data.program.count = 0;
    
    while (parser->current.type != TOK_EOF) {
        node->data.program.functions[node->data.program.count++] = parse_function(parser);
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
    
    // 局部变量
    struct {
        char *name;
        int offset;
    } locals[100];
    int local_count;
    int stack_offset;
} CodeGen;

static void emit_byte(CodeGen *gen, uint8_t byte) {
    if (gen->size >= gen->capacity) {
        gen->capacity = gen->capacity ? gen->capacity * 2 : 1024;
        gen->code = realloc(gen->code, gen->capacity);
    }
    gen->code[gen->size++] = byte;
}

static void emit_bytes(CodeGen *gen, const uint8_t *bytes, size_t count) {
    for (size_t i = 0; i < count; i++) {
        emit_byte(gen, bytes[i]);
    }
}

static void emit_int32(CodeGen *gen, int32_t value) {
    emit_byte(gen, value & 0xFF);
    emit_byte(gen, (value >> 8) & 0xFF);
    emit_byte(gen, (value >> 16) & 0xFF);
    emit_byte(gen, (value >> 24) & 0xFF);
}

// 查找局部变量
static int find_local(CodeGen *gen, const char *name) {
    for (int i = 0; i < gen->local_count; i++) {
        if (strcmp(gen->locals[i].name, name) == 0) {
            return gen->locals[i].offset;
        }
    }
    return 0;
}

// 添加局部变量
static int add_local(CodeGen *gen, const char *name) {
    gen->stack_offset -= 8;
    gen->locals[gen->local_count].name = strdup(name);
    gen->locals[gen->local_count].offset = gen->stack_offset;
    gen->local_count++;
    return gen->stack_offset;
}

// 前向声明
static void gen_expression(CodeGen *gen, ASTNode *node);
static void gen_statement(CodeGen *gen, ASTNode *node);

// 生成二元操作
static void gen_binary_op(CodeGen *gen, ASTNode *node) {
    // 计算左操作数
    gen_expression(gen, node->data.binary.left);
    
    // push rax
    emit_byte(gen, 0x50);
    
    // 计算右操作数
    gen_expression(gen, node->data.binary.right);
    
    // pop rcx
    emit_byte(gen, 0x59);
    
    switch (node->data.binary.op) {
        case TOK_PLUS:
            // add rax, rcx
            emit_bytes(gen, (uint8_t[]){0x48, 0x01, 0xC8}, 3);
            break;
            
        case TOK_MINUS:
            // sub rcx, rax
            emit_bytes(gen, (uint8_t[]){0x48, 0x29, 0xC1}, 3);
            // mov rax, rcx
            emit_bytes(gen, (uint8_t[]){0x48, 0x89, 0xC8}, 3);
            break;
            
        case TOK_STAR:
            // imul rax, rcx
            emit_bytes(gen, (uint8_t[]){0x48, 0x0F, 0xAF, 0xC1}, 4);
            break;
            
        case TOK_SLASH:
            // xchg rax, rcx
            emit_bytes(gen, (uint8_t[]){0x48, 0x91}, 2);
            // cqo
            emit_bytes(gen, (uint8_t[]){0x48, 0x99}, 2);
            // idiv rcx
            emit_bytes(gen, (uint8_t[]){0x48, 0xF7, 0xF9}, 3);
            break;
            
        case TOK_LT:
        case TOK_GT:
        case TOK_LE:
        case TOK_GE:
        case TOK_EQ:
        case TOK_NE:
            // cmp rcx, rax
            emit_bytes(gen, (uint8_t[]){0x48, 0x39, 0xC1}, 3);
            
            // 清零rax
            emit_bytes(gen, (uint8_t[]){0x31, 0xC0}, 2);
            
            // 设置条件码
            switch (node->data.binary.op) {
                case TOK_LT: emit_bytes(gen, (uint8_t[]){0x0F, 0x9C, 0xC0}, 3); break; // setl al
                case TOK_GT: emit_bytes(gen, (uint8_t[]){0x0F, 0x9F, 0xC0}, 3); break; // setg al
                case TOK_LE: emit_bytes(gen, (uint8_t[]){0x0F, 0x9E, 0xC0}, 3); break; // setle al
                case TOK_GE: emit_bytes(gen, (uint8_t[]){0x0F, 0x9D, 0xC0}, 3); break; // setge al
                case TOK_EQ: emit_bytes(gen, (uint8_t[]){0x0F, 0x94, 0xC0}, 3); break; // sete al
                case TOK_NE: emit_bytes(gen, (uint8_t[]){0x0F, 0x95, 0xC0}, 3); break; // setne al
            }
            break;
    }
}

// 生成表达式
static void gen_expression(CodeGen *gen, ASTNode *node) {
    switch (node->type) {
        case NODE_NUMBER:
            // mov rax, imm32
            emit_bytes(gen, (uint8_t[]){0x48, 0xC7, 0xC0}, 3);
            emit_int32(gen, node->data.number);
            break;
            
        case NODE_IDENTIFIER: {
            int offset = find_local(gen, node->data.identifier);
            if (offset) {
                // mov rax, [rbp + offset]
                emit_bytes(gen, (uint8_t[]){0x48, 0x8B, 0x45}, 3);
                emit_byte(gen, offset & 0xFF);
            } else {
                fprintf(stderr, "未定义的变量: %s\n", node->data.identifier);
                exit(1);
            }
            break;
        }
        
        case NODE_ASSIGN: {
            gen_expression(gen, node->data.assign.value);
            
            int offset = find_local(gen, node->data.assign.name);
            if (!offset) {
                fprintf(stderr, "未定义的变量: %s\n", node->data.assign.name);
                exit(1);
            }
            
            // mov [rbp + offset], rax
            emit_bytes(gen, (uint8_t[]){0x48, 0x89, 0x45}, 3);
            emit_byte(gen, offset & 0xFF);
            break;
        }
        
        case NODE_BINARY:
            gen_binary_op(gen, node);
            break;
            
        case NODE_UNARY:
            gen_expression(gen, node->data.unary.operand);
            if (node->data.unary.op == TOK_MINUS) {
                // neg rax
                emit_bytes(gen, (uint8_t[]){0x48, 0xF7, 0xD8}, 3);
            }
            break;
    }
}

// 生成语句
static void gen_statement(CodeGen *gen, ASTNode *node) {
    switch (node->type) {
        case NODE_RETURN:
            if (node->data.ret.value) {
                gen_expression(gen, node->data.ret.value);
            } else {
                // mov rax, 0
                emit_bytes(gen, (uint8_t[]){0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00}, 7);
            }
            
            // 使用系统调用退出
            // mov rdi, rax
            emit_bytes(gen, (uint8_t[]){0x48, 0x89, 0xC7}, 3);
            // mov rax, 60 (exit)
            emit_bytes(gen, (uint8_t[]){0x48, 0xC7, 0xC0, 0x3C, 0x00, 0x00, 0x00}, 7);
            // syscall
            emit_bytes(gen, (uint8_t[]){0x0F, 0x05}, 2);
            break;
            
        case NODE_COMPOUND:
            for (int i = 0; i < node->data.compound.count; i++) {
                gen_statement(gen, node->data.compound.stmts[i]);
            }
            break;
            
        case NODE_DECLARATION: {
            int offset = add_local(gen, node->data.decl.name);
            if (node->data.decl.init) {
                gen_expression(gen, node->data.decl.init);
                // mov [rbp + offset], rax
                emit_bytes(gen, (uint8_t[]){0x48, 0x89, 0x45}, 3);
                emit_byte(gen, offset & 0xFF);
            }
            break;
        }
        
        case NODE_EXPRESSION_STMT:
            gen_expression(gen, node->data.expr_stmt.expr);
            break;
            
        case NODE_IF: {
            gen_expression(gen, node->data.if_stmt.cond);
            
            // test rax, rax
            emit_bytes(gen, (uint8_t[]){0x48, 0x85, 0xC0}, 3);
            
            // je else_label
            emit_bytes(gen, (uint8_t[]){0x0F, 0x84}, 2);
            size_t else_jump = gen->size;
            emit_int32(gen, 0);
            
            gen_statement(gen, node->data.if_stmt.then_stmt);
            
            size_t end_jump = 0;
            if (node->data.if_stmt.else_stmt) {
                // jmp end_label
                emit_byte(gen, 0xE9);
                end_jump = gen->size;
                emit_int32(gen, 0);
            }
            
            // else_label:
            *(int32_t*)(gen->code + else_jump) = gen->size - else_jump - 4;
            
            if (node->data.if_stmt.else_stmt) {
                gen_statement(gen, node->data.if_stmt.else_stmt);
                // end_label:
                *(int32_t*)(gen->code + end_jump) = gen->size - end_jump - 4;
            }
            break;
        }
        
        case NODE_WHILE: {
            // loop_start:
            size_t loop_start = gen->size;
            
            gen_expression(gen, node->data.while_stmt.cond);
            
            // test rax, rax
            emit_bytes(gen, (uint8_t[]){0x48, 0x85, 0xC0}, 3);
            
            // je loop_end
            emit_bytes(gen, (uint8_t[]){0x0F, 0x84}, 2);
            size_t end_jump = gen->size;
            emit_int32(gen, 0);
            
            gen_statement(gen, node->data.while_stmt.body);
            
            // jmp loop_start
            emit_byte(gen, 0xE9);
            emit_int32(gen, loop_start - gen->size - 4);
            
            // loop_end:
            *(int32_t*)(gen->code + end_jump) = gen->size - end_jump - 4;
            break;
        }
        
        case NODE_FOR: {
            // 初始化
            if (node->data.for_stmt.init) {
                gen_statement(gen, node->data.for_stmt.init);
            }
            
            // loop_start:
            size_t loop_start = gen->size;
            
            // 条件
            if (node->data.for_stmt.cond) {
                gen_expression(gen, node->data.for_stmt.cond);
                
                // test rax, rax
                emit_bytes(gen, (uint8_t[]){0x48, 0x85, 0xC0}, 3);
                
                // je loop_end
                emit_bytes(gen, (uint8_t[]){0x0F, 0x84}, 2);
                size_t end_jump = gen->size;
                emit_int32(gen, 0);
                
                gen_statement(gen, node->data.for_stmt.body);
                
                // 更新
                if (node->data.for_stmt.update) {
                    gen_expression(gen, node->data.for_stmt.update);
                }
                
                // jmp loop_start
                emit_byte(gen, 0xE9);
                emit_int32(gen, loop_start - gen->size - 4);
                
                // loop_end:
                *(int32_t*)(gen->code + end_jump) = gen->size - end_jump - 4;
            } else {
                // 无限循环
                gen_statement(gen, node->data.for_stmt.body);
                
                if (node->data.for_stmt.update) {
                    gen_expression(gen, node->data.for_stmt.update);
                }
                
                // jmp loop_start
                emit_byte(gen, 0xE9);
                emit_int32(gen, loop_start - gen->size - 4);
            }
            break;
        }
    }
}

// 生成函数
static void gen_function(CodeGen *gen, ASTNode *node) {
    // 重置局部变量
    gen->local_count = 0;
    gen->stack_offset = 0;
    
    // 函数序言
    // push rbp
    emit_byte(gen, 0x55);
    // mov rbp, rsp
    emit_bytes(gen, (uint8_t[]){0x48, 0x89, 0xE5}, 3);
    // sub rsp, 256
    emit_bytes(gen, (uint8_t[]){0x48, 0x81, 0xEC, 0x00, 0x01, 0x00, 0x00}, 7);
    
    // 生成函数体
    gen_statement(gen, node->data.function.body);
    
    // 默认返回0
    emit_bytes(gen, (uint8_t[]){0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00}, 7);
    // mov rdi, rax
    emit_bytes(gen, (uint8_t[]){0x48, 0x89, 0xC7}, 3);
    // mov rax, 60
    emit_bytes(gen, (uint8_t[]){0x48, 0xC7, 0xC0, 0x3C, 0x00, 0x00, 0x00}, 7);
    // syscall
    emit_bytes(gen, (uint8_t[]){0x0F, 0x05}, 2);
}

// 生成程序
static void gen_program(CodeGen *gen, ASTNode *node) {
    for (int i = 0; i < node->data.program.count; i++) {
        if (strcmp(node->data.program.functions[i]->data.function.name, "main") == 0) {
            gen_function(gen, node->data.program.functions[i]);
            break;
        }
    }
}

// ====================================
// ELF生成
// ====================================

static void write_elf(const char *filename, uint8_t *code, size_t code_size) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("fopen");
        exit(1);
    }
    
    // ELF头
    Elf64_Ehdr ehdr = {0};
    ehdr.e_ident[0] = 0x7f;
    ehdr.e_ident[1] = 'E';
    ehdr.e_ident[2] = 'L';
    ehdr.e_ident[3] = 'F';
    ehdr.e_ident[4] = 2;  // 64位
    ehdr.e_ident[5] = 1;  // 小端
    ehdr.e_ident[6] = 1;  // 当前版本
    ehdr.e_ident[7] = 0;  // System V ABI
    ehdr.e_type = ET_EXEC;
    ehdr.e_machine = EM_X86_64;
    ehdr.e_version = 1;
    ehdr.e_entry = 0x401000;
    ehdr.e_phoff = sizeof(Elf64_Ehdr);
    ehdr.e_shoff = 0;
    ehdr.e_flags = 0;
    ehdr.e_ehsize = sizeof(Elf64_Ehdr);
    ehdr.e_phentsize = sizeof(Elf64_Phdr);
    ehdr.e_phnum = 1;
    ehdr.e_shentsize = 0;
    ehdr.e_shnum = 0;
    ehdr.e_shstrndx = 0;
    
    // 程序头
    Elf64_Phdr phdr = {0};
    phdr.p_type = PT_LOAD;
    phdr.p_flags = PF_X | PF_R;
    phdr.p_offset = 0;
    phdr.p_vaddr = 0x400000;
    phdr.p_paddr = 0x400000;
    phdr.p_filesz = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr) + code_size;
    phdr.p_memsz = phdr.p_filesz;
    phdr.p_align = 0x1000;
    
    fwrite(&ehdr, sizeof(ehdr), 1, f);
    fwrite(&phdr, sizeof(phdr), 1, f);
    fwrite(code, code_size, 1, f);
    
    fclose(f);
    chmod(filename, 0755);
}

// ====================================
// 主函数
// ====================================

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "用法: %s <源文件> <输出文件>\n", argv[0]);
        return 1;
    }
    
    // 读取源文件
    FILE *f = fopen(argv[1], "r");
    if (!f) {
        perror("fopen");
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *source = malloc(size + 1);
    fread(source, 1, size, f);
    source[size] = '\0';
    fclose(f);
    
    // 词法分析
    Lexer lexer;
    lexer_init(&lexer, source);
    
    // 语法分析
    Parser parser;
    parser_init(&parser, &lexer);
    
    ASTNode *ast = parse_program(&parser);
    
    // 代码生成
    CodeGen gen = {0};
    gen_program(&gen, ast);
    
    // 生成ELF文件
    write_elf(argv[2], gen.code, gen.size);
    
    printf("编译成功: %s\n", argv[2]);
    
    // 清理
    free(source);
    free(gen.code);
    
    return 0;
}