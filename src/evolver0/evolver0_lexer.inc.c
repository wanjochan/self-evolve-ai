/**
 * evolver0_lexer.inc.c - 词法分析器模块
 * 被 evolver0.c 包含使用
 */

#ifndef EVOLVER0_LEXER_INC_C
#define EVOLVER0_LEXER_INC_C

// 包含Token定义
#include "evolver0_token.h"

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

static int lexer_is_at_end(Lexer *lexer) {
    return lexer->pos >= lexer->length;
}

static char lexer_peek(Lexer *lexer) {
    if (lexer_is_at_end(lexer)) return '\0';
    return lexer->source[lexer->pos];
}

static char lexer_peek_next(Lexer *lexer) {
    if (lexer->pos + 1 >= lexer->length) return '\0';
    return lexer->source[lexer->pos + 1];
}

static char lexer_advance(Lexer *lexer) {
    if (lexer_is_at_end(lexer)) return '\0';
    
    char c = lexer->source[lexer->pos++];
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    return c;
}

static int lexer_match(Lexer *lexer, char expected) {
    if (lexer_is_at_end(lexer)) return 0;
    if (lexer->source[lexer->pos] != expected) return 0;
    lexer_advance(lexer);
    return 1;
}

// ====================================
// 字符分类函数
// ====================================

static int lexer_is_digit(char c) {
    return c >= '0' && c <= '9';
}

static int lexer_is_hex_digit(char c) {
    return lexer_is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static int lexer_is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int lexer_is_alnum(char c) {
    return lexer_is_alpha(c) || lexer_is_digit(c);
}

// ====================================
// 跳过空白和注释
// ====================================

static void skip_whitespace(Lexer *lexer) {
    while (!lexer_is_at_end(lexer)) {
        char c = lexer_peek(lexer);
        
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                lexer_advance(lexer);
                break;
                
            case '\n':
                if (lexer->in_preprocessor) {
                    return; // 预处理指令在换行处结束
                }
                lexer_advance(lexer);
                break;
                
            case '/':
                if (lexer_peek_next(lexer) == '/') {
                    // 单行注释
                    lexer_advance(lexer); // /
                    lexer_advance(lexer); // /
                    while (!lexer_is_at_end(lexer) && lexer_peek(lexer) != '\n') {
                        lexer_advance(lexer);
                    }
                } else if (lexer_peek_next(lexer) == '*') {
                    // 多行注释
                    lexer_advance(lexer); // /
                    lexer_advance(lexer); // *
                    while (!lexer_is_at_end(lexer)) {
                        if (lexer_peek(lexer) == '*' && lexer_peek_next(lexer) == '/') {
                            lexer_advance(lexer); // *
                            lexer_advance(lexer); // /
                            break;
                        }
                        lexer_advance(lexer);
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
    if (lexer_peek(lexer) == '0' && (lexer_peek_next(lexer) == 'x' || lexer_peek_next(lexer) == 'X')) {
        lexer_advance(lexer); // 0
        lexer_advance(lexer); // x
        
        while (lexer_is_hex_digit(lexer_peek(lexer))) {
            lexer_advance(lexer);
        }
    } else {
        // 十进制
        while (lexer_is_digit(lexer_peek(lexer))) {
            lexer_advance(lexer);
        }
        
        // 浮点数支持
        if (lexer_peek(lexer) == '.' && lexer_is_digit(lexer_peek_next(lexer))) {
            lexer_advance(lexer); // .
            while (lexer_is_digit(lexer_peek(lexer))) {
                lexer_advance(lexer);
            }
        }
        
        // 科学计数法
        if (lexer_peek(lexer) == 'e' || lexer_peek(lexer) == 'E') {
            lexer_advance(lexer);
            if (lexer_peek(lexer) == '+' || lexer_peek(lexer) == '-') {
                lexer_advance(lexer);
            }
            while (lexer_is_digit(lexer_peek(lexer))) {
                lexer_advance(lexer);
            }
        }
    }
    
    // 后缀
    while (lexer_peek(lexer) == 'u' || lexer_peek(lexer) == 'U' || 
           lexer_peek(lexer) == 'l' || lexer_peek(lexer) == 'L' ||
           lexer_peek(lexer) == 'f' || lexer_peek(lexer) == 'F') {
        lexer_advance(lexer);
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
    
    while (!lexer_is_at_end(lexer) && lexer_peek(lexer) != quote) {
        if (lexer_peek(lexer) == '\\' && !lexer_is_at_end(lexer)) {
            lexer_advance(lexer); // 跳过转义字符
        }
    while (!lexer_is_at_end(lexer) && lexer_peek(lexer) != quote) {
        if (lexer_peek(lexer) == '\\' && !lexer_is_at_end(lexer)) {
            lexer_advance(lexer); // 跳过转义字符
        }
        lexer_advance(lexer);
    }
    
    if (lexer_is_at_end(lexer)) {
        lexer->error_count++;
        snprintf(lexer->error_msg, sizeof(lexer->error_msg), 
                 "未结束的字符串字面量");
        return NULL;
    }
    
    lexer_advance(lexer); // 结束引号
    
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
} lexer_keywords[] = {
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
    
    while (lexer_is_alnum(lexer_peek(lexer))) {
        lexer_advance(lexer);
    }
    
    size_t length = (lexer->source + lexer->pos) - start;
    
    // 检查是否是关键字
    for (int i = 0; lexer_keywords[i].keyword != NULL; i++) {
        if (length == strlen(lexer_keywords[i].keyword) &&
            strncmp(start, lexer_keywords[i].keyword, length) == 0) {
            return create_token(lexer, lexer_keywords[i].type, start, length);
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
    lexer_advance(lexer);
    lexer->in_preprocessor = 1;
    
    // 跳过空格
    while (lexer_peek(lexer) == ' ' || lexer_peek(lexer) == '\t') {
        lexer_advance(lexer);
    }
    
    const char *start = lexer->source + lexer->pos;
    
    // 读取预处理指令名
    while (lexer_is_alpha(lexer_peek(lexer))) {
        lexer_advance(lexer);
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
    
    if (lexer_is_at_end(lexer)) {
        return create_simple_token(lexer, TOKEN_EOF, "");
    }
    
    char c = lexer_advance(lexer);
    
    // 数字
    if (lexer_is_digit(c)) {
        lexer->pos--; // 回退
        lexer->column--;
        return scan_number(lexer);
    }
    
    // 标识符
    if (lexer_is_alpha(c)) {
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
            if (lexer_match(lexer, '+')) return create_simple_token(lexer, TOKEN_INCREMENT, "++");
            if (lexer_match(lexer, '=')) return create_simple_token(lexer, TOKEN_ADD_ASSIGN, "+=");
            return create_simple_token(lexer, TOKEN_PLUS, "+");
            
        case '-':
            if (lexer_match(lexer, '-')) return create_simple_token(lexer, TOKEN_DECREMENT, "--");
            if (lexer_match(lexer, '=')) return create_simple_token(lexer, TOKEN_SUB_ASSIGN, "-=");
            if (lexer_match(lexer, '>')) return create_simple_token(lexer, TOKEN_ARROW, "->");
            return create_simple_token(lexer, TOKEN_MINUS, "-");
            
        case '*':
            if (lexer_match(lexer, '=')) return create_simple_token(lexer, TOKEN_MUL_ASSIGN, "*=");
            return create_simple_token(lexer, TOKEN_MULTIPLY, "*");
            
        case '/':
            if (lexer_match(lexer, '=')) return create_simple_token(lexer, TOKEN_DIV_ASSIGN, "/=");
            return create_simple_token(lexer, TOKEN_DIVIDE, "/");
            
        case '%':
            if (lexer_match(lexer, '=')) return create_simple_token(lexer, TOKEN_MOD_ASSIGN, "%=");
            return create_simple_token(lexer, TOKEN_MOD, "%");
            
        case '=':
            if (lexer_match(lexer, '=')) return create_simple_token(lexer, TOKEN_EQUAL, "==");
            return create_simple_token(lexer, TOKEN_ASSIGN, "=");
            
        case '!':
            if (lexer_match(lexer, '=')) return create_simple_token(lexer, TOKEN_NOT_EQUAL, "!=");
            return create_simple_token(lexer, TOKEN_LOGICAL_NOT, "!");
            
        case '<':
            if (lexer_match(lexer, '<')) return create_simple_token(lexer, TOKEN_LEFT_SHIFT, "<<");
            if (lexer_match(lexer, '=')) return create_simple_token(lexer, TOKEN_LESS_EQUAL, "<=");
            return create_simple_token(lexer, TOKEN_LESS, "<");
            
        case '>':
            if (lexer_match(lexer, '>')) return create_simple_token(lexer, TOKEN_RIGHT_SHIFT, ">>");
            if (lexer_match(lexer, '=')) return create_simple_token(lexer, TOKEN_GREATER_EQUAL, ">=");
            return create_simple_token(lexer, TOKEN_GREATER, ">");
            
        case '&':
            if (lexer_match(lexer, '&')) return create_simple_token(lexer, TOKEN_LOGICAL_AND, "&&");
            return create_simple_token(lexer, TOKEN_BIT_AND, "&");
            
        case '|':
            if (lexer_match(lexer, '|')) return create_simple_token(lexer, TOKEN_LOGICAL_OR, "||");
            return create_simple_token(lexer, TOKEN_BIT_OR, "|");
            
        case '^':
            return create_simple_token(lexer, TOKEN_BIT_XOR, "^");
            
        case '.':
            if (lexer_peek(lexer) == '.' && lexer_peek_next(lexer) == '.') {
                lexer_advance(lexer);
                lexer_advance(lexer);
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