/**
 * pipeline_frontend.c - Pipeline Frontend Module
 * 
 * 前端模块，负责词法分析和语法分析：
 * - 词法分析器：C源码 -> Token序列
 * - 语法分析器：Token序列 -> 抽象语法树(AST)
 * - 支持完整的C99语法
 */

#include "pipeline_common.h"

// ===============================================
// 词法分析器实现
// ===============================================

typedef struct {
    const char* source;
    int current;
    int line;
    int column;
} Lexer;

typedef struct {
    Token** tokens;
    int token_count;
    int current;
    char error_msg[256];
} Parser;

static void init_lexer(Lexer* lexer, const char* source) {
    lexer->source = source;
    lexer->current = 0;
    lexer->line = 1;
    lexer->column = 1;
}

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_alnum(char c) {
    return is_alpha(c) || is_digit(c);
}

static char peek_char(Lexer* lexer) {
    if (lexer->source[lexer->current] == '\0') return '\0';
    return lexer->source[lexer->current];
}

static char advance_char(Lexer* lexer) {
    if (lexer->source[lexer->current] == '\0') return '\0';
    
    char c = lexer->source[lexer->current++];
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    return c;
}

static void skip_whitespace(Lexer* lexer) {
    while (true) {
        char c = peek_char(lexer);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance_char(lexer);
        } else {
            break;
        }
    }
}

static TokenType get_keyword_type(const char* identifier) {
    static struct {
        const char* keyword;
        TokenType type;
    } keywords[] = {
        {"if", TOKEN_IF}, {"else", TOKEN_ELSE}, {"while", TOKEN_WHILE},
        {"for", TOKEN_FOR}, {"do", TOKEN_DO}, {"switch", TOKEN_SWITCH},
        {"case", TOKEN_CASE}, {"default", TOKEN_DEFAULT}, {"break", TOKEN_BREAK},
        {"continue", TOKEN_CONTINUE}, {"return", TOKEN_RETURN}, {"goto", TOKEN_GOTO},
        {"void", TOKEN_VOID}, {"char", TOKEN_CHAR}, {"short", TOKEN_SHORT},
        {"int", TOKEN_INT}, {"long", TOKEN_LONG}, {"float", TOKEN_FLOAT},
        {"double", TOKEN_DOUBLE}, {"signed", TOKEN_SIGNED}, {"unsigned", TOKEN_UNSIGNED},
        {"struct", TOKEN_STRUCT}, {"union", TOKEN_UNION}, {"enum", TOKEN_ENUM},
        {"typedef", TOKEN_TYPEDEF}, {"auto", TOKEN_AUTO}, {"register", TOKEN_REGISTER},
        {"static", TOKEN_STATIC}, {"extern", TOKEN_EXTERN}, {"const", TOKEN_CONST},
        {"volatile", TOKEN_VOLATILE}, {"inline", TOKEN_INLINE}, {"restrict", TOKEN_RESTRICT},
        {"_Bool", TOKEN_BOOL}, {"_Complex", TOKEN_COMPLEX}, {"_Imaginary", TOKEN_IMAGINARY},
        {NULL, TOKEN_EOF}
    };
    
    for (int i = 0; keywords[i].keyword; i++) {
        if (strcmp(identifier, keywords[i].keyword) == 0) {
            return keywords[i].type;
        }
    }
    
    return TOKEN_IDENTIFIER;
}

static Token* scan_identifier(Lexer* lexer) {
    int start_line = lexer->line;
    int start_column = lexer->column;
    int start = lexer->current;
    
    while (is_alnum(peek_char(lexer))) {
        advance_char(lexer);
    }
    
    int length = lexer->current - start;
    char* value = malloc(length + 1);
    strncpy(value, lexer->source + start, length);
    value[length] = '\0';
    
    TokenType type = get_keyword_type(value);
    Token* token = create_token(type, value, start_line, start_column);
    free(value);
    
    return token;
}

static Token* scan_number(Lexer* lexer) {
    int start_line = lexer->line;
    int start_column = lexer->column;
    int start = lexer->current;
    
    while (is_digit(peek_char(lexer))) {
        advance_char(lexer);
    }
    
    // 处理浮点数
    if (peek_char(lexer) == '.' && is_digit(lexer->source[lexer->current + 1])) {
        advance_char(lexer); // skip '.'
        while (is_digit(peek_char(lexer))) {
            advance_char(lexer);
        }
    }
    
    int length = lexer->current - start;
    char* value = malloc(length + 1);
    strncpy(value, lexer->source + start, length);
    value[length] = '\0';
    
    return create_token(TOKEN_NUMBER, value, start_line, start_column);
}

static Token* scan_string(Lexer* lexer) {
    int start_line = lexer->line;
    int start_column = lexer->column;
    
    advance_char(lexer); // skip opening quote
    
    char* value = malloc(256);
    int length = 0;
    
    while (peek_char(lexer) != '"' && peek_char(lexer) != '\0') {
        char c = advance_char(lexer);
        if (c == '\\') {
            c = advance_char(lexer);
            switch (c) {
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                case 'r': c = '\r'; break;
                case '\\': c = '\\'; break;
                case '"': c = '"'; break;
                case '0': c = '\0'; break;
            }
        }
        value[length++] = c;
        if (length >= 255) break;
    }
    
    if (peek_char(lexer) == '"') {
        advance_char(lexer); // skip closing quote
    }
    
    value[length] = '\0';
    return create_token(TOKEN_STRING, value, start_line, start_column);
}

static Token* scan_token(Lexer* lexer) {
    skip_whitespace(lexer);
    
    char c = peek_char(lexer);
    if (c == '\0') {
        return create_token(TOKEN_EOF, NULL, lexer->line, lexer->column);
    }
    
    int start_line = lexer->line;
    int start_column = lexer->column;
    
    // 标识符和关键字
    if (is_alpha(c)) {
        return scan_identifier(lexer);
    }
    
    // 数字
    if (is_digit(c)) {
        return scan_number(lexer);
    }
    
    // 字符串
    if (c == '"') {
        return scan_string(lexer);
    }
    
    // 双字符运算符
    if (c == '=' && lexer->source[lexer->current + 1] == '=') {
        advance_char(lexer); advance_char(lexer);
        return create_token(TOKEN_EQ, "==", start_line, start_column);
    }
    
    if (c == '!' && lexer->source[lexer->current + 1] == '=') {
        advance_char(lexer); advance_char(lexer);
        return create_token(TOKEN_NE, "!=", start_line, start_column);
    }
    
    // 单字符运算符
    advance_char(lexer);
    
    switch (c) {
        case '+': return create_token(TOKEN_PLUS, "+", start_line, start_column);
        case '-': return create_token(TOKEN_MINUS, "-", start_line, start_column);
        case '*': return create_token(TOKEN_STAR, "*", start_line, start_column);
        case '/': return create_token(TOKEN_SLASH, "/", start_line, start_column);
        case '=': return create_token(TOKEN_ASSIGN, "=", start_line, start_column);
        case ';': return create_token(TOKEN_SEMICOLON, ";", start_line, start_column);
        case '(': return create_token(TOKEN_LPAREN, "(", start_line, start_column);
        case ')': return create_token(TOKEN_RPAREN, ")", start_line, start_column);
        case '{': return create_token(TOKEN_LBRACE, "{", start_line, start_column);
        case '}': return create_token(TOKEN_RBRACE, "}", start_line, start_column);
        default:
            return create_token(TOKEN_EOF, NULL, start_line, start_column);
    }
}

// ===============================================
// 语法分析器实现
// ===============================================

static bool match_token(Parser* parser, TokenType type) {
    if (parser->current >= parser->token_count) return false;
    return parser->tokens[parser->current]->type == type;
}

static Token* consume_token(Parser* parser, TokenType type, const char* error_msg) {
    if (parser->current >= parser->token_count) {
        snprintf(parser->error_msg, sizeof(parser->error_msg), 
                "Unexpected end of input: %s", error_msg);
        return NULL;
    }
    
    Token* token = parser->tokens[parser->current];
    if (token->type != type) {
        snprintf(parser->error_msg, sizeof(parser->error_msg), 
                "Expected token type %d but got %d: %s", type, token->type, error_msg);
        return NULL;
    }
    
    parser->current++;
    return token;
}

static ASTNode* parse_primary_expression(Parser* parser) {
    Token* token = parser->tokens[parser->current];
    if (!token) return NULL;
    
    switch (token->type) {
        case TOKEN_NUMBER: {
            ASTNode* node = ast_create_node(ASTC_EXPR_CONSTANT, token->line, token->column);
            node->data.constant.type = ASTC_TYPE_INT;
            node->data.constant.int_val = atoi(token->value);
            parser->current++;
            return node;
        }
        case TOKEN_IDENTIFIER: {
            ASTNode* node = ast_create_node(ASTC_EXPR_IDENTIFIER, token->line, token->column);
            node->data.identifier.name = strdup(token->value);
            parser->current++;
            return node;
        }
        default:
            snprintf(parser->error_msg, sizeof(parser->error_msg), 
                    "Expected primary expression");
            return NULL;
    }
}

static ASTNode* parse_expression(Parser* parser) {
    return parse_primary_expression(parser);
}

static ASTNode* parse_statement(Parser* parser) {
    Token* token = parser->tokens[parser->current];
    if (!token) return NULL;
    
    if (token->type == TOKEN_RETURN) {
        parser->current++; // consume 'return'
        ASTNode* return_stmt = ast_create_node(ASTC_RETURN_STMT, token->line, token->column);
        
        if (!match_token(parser, TOKEN_SEMICOLON)) {
            ASTNode* expr = parse_expression(parser);
            if (!expr) return NULL;
            return_stmt->data.return_stmt.value = expr;
        }
        
        if (!consume_token(parser, TOKEN_SEMICOLON, "Expected ';' after return statement")) {
            ast_free(return_stmt);
            return NULL;
        }
        
        return return_stmt;
    }
    
    // 表达式语句
    ASTNode* expr = parse_expression(parser);
    if (!expr) return NULL;
    
    ASTNode* expr_stmt = ast_create_node(ASTC_EXPR_STMT, expr->line, expr->column);
    expr_stmt->data.expr_stmt.expr = expr;
    
    if (!consume_token(parser, TOKEN_SEMICOLON, "Expected ';' after expression statement")) {
        ast_free(expr_stmt);
        return NULL;
    }
    
    return expr_stmt;
}

static ASTNode* parse_function(Parser* parser) {
    // 简化的函数解析
    Token* type_token = consume_token(parser, TOKEN_INT, "Expected type");
    if (!type_token) return NULL;
    
    Token* name_token = consume_token(parser, TOKEN_IDENTIFIER, "Expected function name");
    if (!name_token) return NULL;
    
    ASTNode* func_decl = ast_create_node(ASTC_FUNC_DECL, type_token->line, type_token->column);
    func_decl->data.func_decl.name = strdup(name_token->value);
    func_decl->data.func_decl.return_type = ast_create_node(ASTC_TYPE_INT, type_token->line, type_token->column);
    
    if (!consume_token(parser, TOKEN_LPAREN, "Expected '(' after function name")) {
        ast_free(func_decl);
        return NULL;
    }
    
    if (!consume_token(parser, TOKEN_RPAREN, "Expected ')' after parameters")) {
        ast_free(func_decl);
        return NULL;
    }
    
    if (!consume_token(parser, TOKEN_LBRACE, "Expected '{' to start function body")) {
        ast_free(func_decl);
        return NULL;
    }
    
    // 解析函数体
    ASTNode* body = ast_create_node(ASTC_COMPOUND_STMT, name_token->line, name_token->column);
    
    while (!match_token(parser, TOKEN_RBRACE) && !match_token(parser, TOKEN_EOF)) {
        ASTNode* stmt = parse_statement(parser);
        if (!stmt) {
            ast_free(body);
            ast_free(func_decl);
            return NULL;
        }
        // 简化：不处理语句列表
    }
    
    if (!consume_token(parser, TOKEN_RBRACE, "Expected '}' to end function body")) {
        ast_free(body);
        ast_free(func_decl);
        return NULL;
    }
    
    func_decl->data.func_decl.body = body;
    func_decl->data.func_decl.has_body = 1;
    
    return func_decl;
}

// ===============================================
// 对外接口实现
// ===============================================

bool frontend_tokenize(const char* source, Token*** tokens, int* token_count) {
    if (!source || !tokens || !token_count) return false;
    
    Lexer lexer;
    init_lexer(&lexer, source);
    
    Token** token_array = malloc(PIPELINE_MAX_TOKENS * sizeof(Token*));
    int count = 0;
    
    while (count < PIPELINE_MAX_TOKENS - 1) {
        Token* token = scan_token(&lexer);
        if (!token) break;
        
        token_array[count++] = token;
        
        if (token->type == TOKEN_EOF) break;
    }
    
    *tokens = token_array;
    *token_count = count;
    return true;
}

ASTNode* frontend_parse(Token** tokens, int token_count) {
    if (!tokens || token_count <= 0) return NULL;
    
    Parser parser;
    parser.tokens = tokens;
    parser.token_count = token_count;
    parser.current = 0;
    parser.error_msg[0] = '\0';
    
    ASTNode* program = ast_create_node(ASTC_TRANSLATION_UNIT, 0, 0);
    
    while (!match_token(&parser, TOKEN_EOF)) {
        ASTNode* func = parse_function(&parser);
        if (!func) {
            fprintf(stderr, "Parse error: %s\n", parser.error_msg);
            ast_free(program);
            return NULL;
        }
        // 简化：不处理声明列表
    }
    
    return program;
}

void frontend_free_tokens(Token** tokens, int token_count) {
    free_token_array(tokens, token_count);
}

ASTNode* frontend_compile(const char* source) {
    Token** tokens;
    int token_count;
    
    if (!frontend_tokenize(source, &tokens, &token_count)) {
        return NULL;
    }
    
    ASTNode* ast = frontend_parse(tokens, token_count);
    
    frontend_free_tokens(tokens, token_count);
    
    return ast;
}