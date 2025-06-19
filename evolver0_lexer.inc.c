/**
 * evolver0_lexer.inc.c - 词法分析器模块
 * 被 evolver0.c 包含使用
 */

#ifndef EVOLVER0_LEXER_INC_C
#define EVOLVER0_LEXER_INC_C

// ====================================
// Token 定义
// ====================================

// 使用evolver0.c中已有的TokenType定义
// 如果没有定义，使用简化版本
#ifndef TOKEN_EOF
typedef enum {
    TOKEN_EOF = 0,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_CHAR_LITERAL,
    
    // 关键字
    TOKEN_INT, TOKEN_CHAR, TOKEN_VOID, TOKEN_RETURN, 
    TOKEN_IF, TOKEN_ELSE, TOKEN_WHILE, TOKEN_FOR, TOKEN_DO, 
    TOKEN_BREAK, TOKEN_CONTINUE, TOKEN_SWITCH, TOKEN_CASE, TOKEN_DEFAULT,
    TOKEN_STRUCT, TOKEN_UNION, TOKEN_ENUM, TOKEN_TYPEDEF,
    TOKEN_STATIC, TOKEN_EXTERN, TOKEN_CONST, TOKEN_VOLATILE,
    TOKEN_SIZEOF, TOKEN_GOTO,
    
    // 操作符
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULTIPLY, TOKEN_DIVIDE, TOKEN_MOD,
    TOKEN_ASSIGN, TOKEN_ADD_ASSIGN, TOKEN_SUB_ASSIGN, TOKEN_MUL_ASSIGN,
    TOKEN_DIV_ASSIGN, TOKEN_MOD_ASSIGN,
    TOKEN_EQUAL, TOKEN_NOT_EQUAL,
    TOKEN_LESS, TOKEN_GREATER, TOKEN_LESS_EQUAL, TOKEN_GREATER_EQUAL,
    TOKEN_LOGICAL_AND, TOKEN_LOGICAL_OR, TOKEN_LOGICAL_NOT,
    TOKEN_BIT_AND, TOKEN_BIT_OR, TOKEN_BIT_XOR, TOKEN_BIT_NOT,
    TOKEN_LEFT_SHIFT, TOKEN_RIGHT_SHIFT,
    TOKEN_INCREMENT, TOKEN_DECREMENT,
    TOKEN_ARROW, TOKEN_DOT,
    TOKEN_QUESTION, TOKEN_COLON,
    
    // 标点
    TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_LBRACE, TOKEN_RBRACE,
    TOKEN_LBRACKET, TOKEN_RBRACKET, TOKEN_SEMICOLON, TOKEN_COMMA,
    TOKEN_ELLIPSIS,
    
    // 预处理
    TOKEN_HASH, TOKEN_INCLUDE, TOKEN_DEFINE, TOKEN_IFDEF, TOKEN_IFNDEF,
    TOKEN_ENDIF, TOKEN_UNDEF, TOKEN_PRAGMA,
    
    // 特殊
    TOKEN_UNKNOWN
} TokenType;
#endif

// Token 结构体
#ifndef TOKEN_STRUCT_DEFINED
#define TOKEN_STRUCT_DEFINED
typedef struct {
    TokenType type;
    char *value;
    int line;
    int column;
    const char *filename;
} Token;
#endif

// ====================================
// 词法分析器状态
// ====================================

typedef struct {
    const char *source;
    size_t pos;
    size_t length;
    int line;
    int column;
    const char *filename;
    
    // 错误处理
    char error_msg[256];
    int error_count;
    
    // 预处理器状态
    int in_preprocessor;
    int in_include;
} Lexer;

// ====================================
// 辅助函数
// ====================================

static void init_lexer(Lexer *lexer, const char *source, const char *filename) {
    lexer->source = source;
    lexer->pos = 0;
    lexer->length = strlen(source);
    lexer->line = 1;
    lexer->column = 1;
    lexer->filename = filename ? filename : "<input>";
    lexer->error_msg[0] = '\0';
    lexer->error_count = 0;
    lexer->in_preprocessor = 0;
    lexer->in_include = 0;
}

static int is_at_end(Lexer *lexer) {
    return lexer->pos >= lexer->length;
}

static char peek(Lexer *lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->source[lexer->pos];
}

static char peek_next(Lexer *lexer) {
    if (lexer->pos + 1 >= lexer->length) return '\0';
    return lexer->source[lexer->pos + 1];
}

static char advance(Lexer *lexer) {
    if (is_at_end(lexer)) return '\0';
    
    char c = lexer->source[lexer->pos++];
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    return c;
}

static int match(Lexer *lexer, char expected) {
    if (is_at_end(lexer)) return 0;
    if (lexer->source[lexer->pos] != expected) return 0;
    advance(lexer);
    return 1;
}

// ====================================
// 字符分类函数
// ====================================

static int is_digit(char c) {
    return c >= '0' && c <= '9';
}

static int is_hex_digit(char c) {
    return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static int is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int is_alnum(char c) {
    return is_alpha(c) || is_digit(c);
}

// ====================================
// 跳过空白和注释
// ====================================

static void skip_whitespace(Lexer *lexer) {
    while (!is_at_end(lexer)) {
        char c = peek(lexer);
        
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance(lexer);
                break;
                
            case '\n':
                if (lexer->in_preprocessor) {
                    return; // 预处理指令在换行处结束
                }
                advance(lexer);
                break;
                
            case '/':
                if (peek_next(lexer) == '/') {
                    // 单行注释
                    advance(lexer); // /
                    advance(lexer); // /
                    while (!is_at_end(lexer) && peek(lexer) != '\n') {
                        advance(lexer);
                    }
                } else if (peek_next(lexer) == '*') {
                    // 多行注释
                    advance(lexer); // /
                    advance(lexer); // *
                    while (!is_at_end(lexer)) {
                        if (peek(lexer) == '*' && peek_next(lexer) == '/') {
                            advance(lexer); // *
                            advance(lexer); // /
                            break;
                        }
                        advance(lexer);
                    }
                } else {
                    return;
                }
                break;
                
            default:
                return;
        }
    }
}

// ====================================
// Token 创建函数
// ====================================

static Token* create_token(Lexer *lexer, TokenType type, const char *start, size_t length) {
    Token *token = (Token*)malloc(sizeof(Token));
    if (!token) return NULL;
    
    token->type = type;
    token->value = (char*)malloc(length + 1);
    if (!token->value) {
        free(token);
        return NULL;
    }
    
    memcpy(token->value, start, length);
    token->value[length] = '\0';
    
    token->line = lexer->line;
    token->column = lexer->column - length;
    token->filename = lexer->filename;
    
    return token;
}

static Token* create_simple_token(Lexer *lexer, TokenType type, const char *value) {
    return create_token(lexer, type, value, strlen(value));
}

// ====================================
// 数字字面量
// ====================================

static Token* scan_number(Lexer *lexer) {
    const char *start = lexer->source + lexer->pos;
    
    // 检查十六进制
    if (peek(lexer) == '0' && (peek_next(lexer) == 'x' || peek_next(lexer) == 'X')) {
        advance(lexer); // 0
        advance(lexer); // x
        
        while (is_hex_digit(peek(lexer))) {
            advance(lexer);
        }
    } else {
        // 十进制
        while (is_digit(peek(lexer))) {
            advance(lexer);
        }
        
        // 浮点数支持
        if (peek(lexer) == '.' && is_digit(peek_next(lexer))) {
            advance(lexer); // .
            while (is_digit(peek(lexer))) {
                advance(lexer);
            }
        }
        
        // 科学计数法
        if (peek(lexer) == 'e' || peek(lexer) == 'E') {
            advance(lexer);
            if (peek(lexer) == '+' || peek(lexer) == '-') {
                advance(lexer);
            }
            while (is_digit(peek(lexer))) {
                advance(lexer);
            }
        }
    }
    
    // 后缀
    while (peek(lexer) == 'u' || peek(lexer) == 'U' || 
           peek(lexer) == 'l' || peek(lexer) == 'L' ||
           peek(lexer) == 'f' || peek(lexer) == 'F') {
        advance(lexer);
    }
    
    size_t length = (lexer->source + lexer->pos) - start;
    return create_token(lexer, TOKEN_NUMBER, start, length);
}

// ====================================
// 字符串和字符字面量
// ====================================

static Token* scan_string(Lexer *lexer) {
    const char *start = lexer->source + lexer->pos - 1; // 包含引号
    char quote = lexer->source[lexer->pos - 1];
    
    while (!is_at_end(lexer) && peek(lexer) != quote) {
        if (peek(lexer) == '\\' && !is_at_end(lexer)) {
            advance(lexer); // 跳过转义字符
        }
        advance(lexer);
    }
    
    if (is_at_end(lexer)) {
        lexer->error_count++;
        snprintf(lexer->error_msg, sizeof(lexer->error_msg), 
                 "未结束的字符串字面量");
        return NULL;
    }
    
    advance(lexer); // 结束引号
    
    size_t length = (lexer->source + lexer->pos) - start;
    TokenType type = (quote == '"') ? TOKEN_STRING : TOKEN_CHAR_LITERAL;
    return create_token(lexer, type, start, length);
}

// ====================================
// 标识符和关键字
// ====================================

static struct {
    const char *keyword;
    TokenType type;
} keywords[] = {
    {"int", TOKEN_INT},
    {"char", TOKEN_CHAR},
    {"void", TOKEN_VOID},
    {"return", TOKEN_RETURN},
    {"if", TOKEN_IF},
    {"else", TOKEN_ELSE},
    {"while", TOKEN_WHILE},
    {"for", TOKEN_FOR},
    {"do", TOKEN_DO},
    {"break", TOKEN_BREAK},
    {"continue", TOKEN_CONTINUE},
    {"switch", TOKEN_SWITCH},
    {"case", TOKEN_CASE},
    {"default", TOKEN_DEFAULT},
    {"struct", TOKEN_STRUCT},
    {"union", TOKEN_UNION},
    {"enum", TOKEN_ENUM},
    {"typedef", TOKEN_TYPEDEF},
    {"static", TOKEN_STATIC},
    {"extern", TOKEN_EXTERN},
    {"const", TOKEN_CONST},
    {"volatile", TOKEN_VOLATILE},
    {"sizeof", TOKEN_SIZEOF},
    {"goto", TOKEN_GOTO},
    {NULL, TOKEN_UNKNOWN}
};

static Token* scan_identifier(Lexer *lexer) {
    const char *start = lexer->source + lexer->pos;
    
    while (is_alnum(peek(lexer))) {
        advance(lexer);
    }
    
    size_t length = (lexer->source + lexer->pos) - start;
    
    // 检查是否是关键字
    for (int i = 0; keywords[i].keyword != NULL; i++) {
        if (length == strlen(keywords[i].keyword) &&
            strncmp(start, keywords[i].keyword, length) == 0) {
            return create_token(lexer, keywords[i].type, start, length);
        }
    }
    
    // 不是关键字，返回标识符
    return create_token(lexer, TOKEN_IDENTIFIER, start, length);
}

// ====================================
// 预处理指令
// ====================================

static Token* scan_preprocessor(Lexer *lexer) {
    // 跳过 #
    advance(lexer);
    lexer->in_preprocessor = 1;
    
    // 跳过空格
    while (peek(lexer) == ' ' || peek(lexer) == '\t') {
        advance(lexer);
    }
    
    const char *start = lexer->source + lexer->pos;
    
    // 读取预处理指令名
    while (is_alpha(peek(lexer))) {
        advance(lexer);
    }
    
    size_t length = (lexer->source + lexer->pos) - start;
    
    TokenType type = TOKEN_HASH;
    if (length == 7 && strncmp(start, "include", 7) == 0) {
        type = TOKEN_INCLUDE;
        lexer->in_include = 1;
    } else if (length == 6 && strncmp(start, "define", 6) == 0) {
        type = TOKEN_DEFINE;
    } else if (length == 5 && strncmp(start, "ifdef", 5) == 0) {
        type = TOKEN_IFDEF;
    } else if (length == 6 && strncmp(start, "ifndef", 6) == 0) {
        type = TOKEN_IFNDEF;
    } else if (length == 5 && strncmp(start, "endif", 5) == 0) {
        type = TOKEN_ENDIF;
    } else if (length == 5 && strncmp(start, "undef", 5) == 0) {
        type = TOKEN_UNDEF;
    } else if (length == 6 && strncmp(start, "pragma", 6) == 0) {
        type = TOKEN_PRAGMA;
    }
    
    return create_token(lexer, type, start, length);
}

// ====================================
// 主扫描函数
// ====================================

static Token* scan_token(Lexer *lexer) {
    skip_whitespace(lexer);
    
    if (is_at_end(lexer)) {
        return create_simple_token(lexer, TOKEN_EOF, "");
    }
    
    char c = advance(lexer);
    
    // 数字
    if (is_digit(c)) {
        lexer->pos--; // 回退
        lexer->column--;
        return scan_number(lexer);
    }
    
    // 标识符
    if (is_alpha(c)) {
        lexer->pos--; // 回退
        lexer->column--;
        return scan_identifier(lexer);
    }
    
    // 字符串和字符
    if (c == '"' || c == '\'') {
        return scan_string(lexer);
    }
    
    // 预处理指令
    if (c == '#' && lexer->column == 1) {
        lexer->pos--; // 回退
        lexer->column--;
        return scan_preprocessor(lexer);
    }
    
    // 操作符和标点符号
    switch (c) {
        // 单字符标记
        case '(': return create_simple_token(lexer, TOKEN_LPAREN, "(");
        case ')': return create_simple_token(lexer, TOKEN_RPAREN, ")");
        case '{': return create_simple_token(lexer, TOKEN_LBRACE, "{");
        case '}': return create_simple_token(lexer, TOKEN_RBRACE, "}");
        case '[': return create_simple_token(lexer, TOKEN_LBRACKET, "[");
        case ']': return create_simple_token(lexer, TOKEN_RBRACKET, "]");
        case ';': return create_simple_token(lexer, TOKEN_SEMICOLON, ";");
        case ',': return create_simple_token(lexer, TOKEN_COMMA, ",");
        case '?': return create_simple_token(lexer, TOKEN_QUESTION, "?");
        case ':': return create_simple_token(lexer, TOKEN_COLON, ":");
        case '~': return create_simple_token(lexer, TOKEN_BIT_NOT, "~");
        
        // 可能是多字符标记
        case '+':
            if (match(lexer, '+')) return create_simple_token(lexer, TOKEN_INCREMENT, "++");
            if (match(lexer, '=')) return create_simple_token(lexer, TOKEN_ADD_ASSIGN, "+=");
            return create_simple_token(lexer, TOKEN_PLUS, "+");
            
        case '-':
            if (match(lexer, '-')) return create_simple_token(lexer, TOKEN_DECREMENT, "--");
            if (match(lexer, '=')) return create_simple_token(lexer, TOKEN_SUB_ASSIGN, "-=");
            if (match(lexer, '>')) return create_simple_token(lexer, TOKEN_ARROW, "->");
            return create_simple_token(lexer, TOKEN_MINUS, "-");
            
        case '*':
            if (match(lexer, '=')) return create_simple_token(lexer, TOKEN_MUL_ASSIGN, "*=");
            return create_simple_token(lexer, TOKEN_MULTIPLY, "*");
            
        case '/':
            if (match(lexer, '=')) return create_simple_token(lexer, TOKEN_DIV_ASSIGN, "/=");
            return create_simple_token(lexer, TOKEN_DIVIDE, "/");
            
        case '%':
            if (match(lexer, '=')) return create_simple_token(lexer, TOKEN_MOD_ASSIGN, "%=");
            return create_simple_token(lexer, TOKEN_MOD, "%");
            
        case '=':
            if (match(lexer, '=')) return create_simple_token(lexer, TOKEN_EQUAL, "==");
            return create_simple_token(lexer, TOKEN_ASSIGN, "=");
            
        case '!':
            if (match(lexer, '=')) return create_simple_token(lexer, TOKEN_NOT_EQUAL, "!=");
            return create_simple_token(lexer, TOKEN_LOGICAL_NOT, "!");
            
        case '<':
            if (match(lexer, '<')) return create_simple_token(lexer, TOKEN_LEFT_SHIFT, "<<");
            if (match(lexer, '=')) return create_simple_token(lexer, TOKEN_LESS_EQUAL, "<=");
            return create_simple_token(lexer, TOKEN_LESS, "<");
            
        case '>':
            if (match(lexer, '>')) return create_simple_token(lexer, TOKEN_RIGHT_SHIFT, ">>");
            if (match(lexer, '=')) return create_simple_token(lexer, TOKEN_GREATER_EQUAL, ">=");
            return create_simple_token(lexer, TOKEN_GREATER, ">");
            
        case '&':
            if (match(lexer, '&')) return create_simple_token(lexer, TOKEN_LOGICAL_AND, "&&");
            return create_simple_token(lexer, TOKEN_BIT_AND, "&");
            
        case '|':
            if (match(lexer, '|')) return create_simple_token(lexer, TOKEN_LOGICAL_OR, "||");
            return create_simple_token(lexer, TOKEN_BIT_OR, "|");
            
        case '^':
            return create_simple_token(lexer, TOKEN_BIT_XOR, "^");
            
        case '.':
            if (peek(lexer) == '.' && peek_next(lexer) == '.') {
                advance(lexer);
                advance(lexer);
                return create_simple_token(lexer, TOKEN_ELLIPSIS, "...");
            }
            return create_simple_token(lexer, TOKEN_DOT, ".");
            
        default:
            return create_simple_token(lexer, TOKEN_UNKNOWN, &c);
    }
}

// ====================================
// 公共接口
// ====================================

static Token* tokenize_source(const char *source, const char *filename, int *token_count) {
    Lexer lexer;
    init_lexer(&lexer, source, filename);
    
    // 预分配token数组
    size_t capacity = 1000;
    Token *tokens = (Token*)malloc(sizeof(Token) * capacity);
    if (!tokens) return NULL;
    
    int count = 0;
    
    while (1) {
        if (count >= capacity - 1) {
            capacity *= 2;
            Token *new_tokens = (Token*)realloc(tokens, sizeof(Token) * capacity);
            if (!new_tokens) {
                // 释放已分配的tokens
                for (int i = 0; i < count; i++) {
                    free(tokens[i].value);
                }
                free(tokens);
                return NULL;
            }
            tokens = new_tokens;
        }
        
        Token *token = scan_token(&lexer);
        if (!token) {
            if (lexer.error_count > 0) {
                fprintf(stderr, "词法错误 %s:%d:%d: %s\n", 
                        lexer.filename, lexer.line, lexer.column, lexer.error_msg);
            }
            // 继续扫描
            continue;
        }
        
        tokens[count] = *token;
        free(token); // 只释放Token结构，不释放value
        
        if (tokens[count].type == TOKEN_EOF) {
            count++;
            break;
        }
        
        count++;
    }
    
    *token_count = count;
    return tokens;
}

static void free_tokens(Token *tokens, int count) {
    if (!tokens) return;
    
    for (int i = 0; i < count; i++) {
        free(tokens[i].value);
    }
    free(tokens);
}

#endif // EVOLVER0_LEXER_INC_C