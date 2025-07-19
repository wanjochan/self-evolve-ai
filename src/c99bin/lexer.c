/**
 * lexer.c - Enhanced C99 Lexical Analyzer Implementation
 * 
 * 完整的C99词法分析器实现
 */

#include "lexer.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// C99关键字表
static const struct {
    const char* keyword;
    TokenType token;
} keywords[] = {
    {"auto", TOKEN_AUTO},
    {"break", TOKEN_BREAK},
    {"case", TOKEN_CASE},
    {"char", TOKEN_CHAR},
    {"const", TOKEN_CONST},
    {"continue", TOKEN_CONTINUE},
    {"default", TOKEN_DEFAULT},
    {"do", TOKEN_DO},
    {"double", TOKEN_DOUBLE},
    {"else", TOKEN_ELSE},
    {"enum", TOKEN_ENUM},
    {"extern", TOKEN_EXTERN},
    {"float", TOKEN_FLOAT},
    {"for", TOKEN_FOR},
    {"goto", TOKEN_GOTO},
    {"if", TOKEN_IF},
    {"inline", TOKEN_INLINE},
    {"int", TOKEN_INT},
    {"long", TOKEN_LONG},
    {"register", TOKEN_REGISTER},
    {"restrict", TOKEN_RESTRICT},
    {"return", TOKEN_RETURN},
    {"short", TOKEN_SHORT},
    {"signed", TOKEN_SIGNED},
    {"sizeof", TOKEN_SIZEOF},
    {"static", TOKEN_STATIC},
    {"struct", TOKEN_STRUCT},
    {"switch", TOKEN_SWITCH},
    {"typedef", TOKEN_TYPEDEF},
    {"union", TOKEN_UNION},
    {"unsigned", TOKEN_UNSIGNED},
    {"void", TOKEN_VOID},
    {"volatile", TOKEN_VOLATILE},
    {"while", TOKEN_WHILE},
    {"_Bool", TOKEN__BOOL},
    {"_Complex", TOKEN__COMPLEX},
    {"_Imaginary", TOKEN__IMAGINARY},
    {NULL, TOKEN_UNKNOWN}
};

// Token类型名称表（用于调试）
static const char* token_names[] = {
    "IDENTIFIER", "INTEGER_CONSTANT", "FLOAT_CONSTANT", "CHAR_CONSTANT", "STRING_LITERAL",
    "auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else",
    "enum", "extern", "float", "for", "goto", "if", "inline", "int", "long", "register",
    "restrict", "return", "short", "signed", "sizeof", "static", "struct", "switch",
    "typedef", "union", "unsigned", "void", "volatile", "while", "_Bool", "_Complex", "_Imaginary",
    "+", "-", "*", "/", "%", "=", "+=", "-=", "*=", "/=", "%=", "++", "--",
    "==", "!=", "<", ">", "<=", ">=", "&&", "||", "!", "&", "|", "^", "~",
    "<<", ">>", "&=", "|=", "^=", "<<=", ">>=", "->", ".", "?", ":",
    ";", ",", "(", ")", "{", "}", "[", "]", "#", "##",
    "NEWLINE", "WHITESPACE", "COMMENT", "EOF", "ERROR", "UNKNOWN"
};

// 创建词法分析器
Lexer* lexer_create(const char* source) {
    if (!source) return NULL;
    
    Lexer* lexer = malloc(sizeof(Lexer));
    if (!lexer) return NULL;
    
    lexer->source = source;
    lexer->position = 0;
    lexer->length = strlen(source);
    lexer->line = 1;
    lexer->column = 1;
    lexer->error_count = 0;
    lexer->error_msg[0] = '\0';
    
    return lexer;
}

// 销毁词法分析器
void lexer_destroy(Lexer* lexer) {
    if (lexer) {
        free(lexer);
    }
}

// 销毁token
void token_destroy(Token* token) {
    if (token) {
        if (token->value) {
            free(token->value);
        }
        free(token);
    }
}

// 获取当前字符
static char current_char(const Lexer* lexer) {
    if (lexer->position >= lexer->length) {
        return '\0';
    }
    return lexer->source[lexer->position];
}

// 获取下一个字符
static char peek_char(const Lexer* lexer, int offset) {
    size_t pos = lexer->position + offset;
    if (pos >= lexer->length) {
        return '\0';
    }
    return lexer->source[pos];
}

// 前进一个字符
static void advance_char(Lexer* lexer) {
    if (lexer->position < lexer->length) {
        if (lexer->source[lexer->position] == '\n') {
            lexer->line++;
            lexer->column = 1;
        } else {
            lexer->column++;
        }
        lexer->position++;
    }
}

// 创建token
static Token* create_token(TokenType type, const char* value, size_t length, int line, int column) {
    Token* token = malloc(sizeof(Token));
    if (!token) return NULL;
    
    token->type = type;
    token->length = length;
    token->line = line;
    token->column = column;
    
    if (value && length > 0) {
        token->value = malloc(length + 1);
        if (token->value) {
            strncpy(token->value, value, length);
            token->value[length] = '\0';
        }
    } else {
        token->value = NULL;
    }
    
    // 初始化数值联合体
    token->numeric.int_value = 0;
    
    return token;
}

// 跳过空白字符
void lexer_skip_whitespace(Lexer* lexer) {
    while (is_whitespace(current_char(lexer))) {
        advance_char(lexer);
    }
}

// 扫描标识符
static Token* scan_identifier(Lexer* lexer) {
    size_t start = lexer->position;
    int start_column = lexer->column;
    
    // 读取标识符字符
    while (is_identifier_char(current_char(lexer))) {
        advance_char(lexer);
    }
    
    size_t length = lexer->position - start;
    const char* identifier = &lexer->source[start];
    
    // 检查是否为关键字
    for (int i = 0; keywords[i].keyword; i++) {
        if (strncmp(identifier, keywords[i].keyword, length) == 0 && 
            strlen(keywords[i].keyword) == length) {
            return create_token(keywords[i].token, identifier, length, lexer->line, start_column);
        }
    }
    
    // 普通标识符
    return create_token(TOKEN_IDENTIFIER, identifier, length, lexer->line, start_column);
}

// 扫描数字常量
static Token* scan_number(Lexer* lexer) {
    size_t start = lexer->position;
    int start_column = lexer->column;
    int is_float = 0;
    
    // 处理十六进制
    if (current_char(lexer) == '0' && (peek_char(lexer, 1) == 'x' || peek_char(lexer, 1) == 'X')) {
        advance_char(lexer); // 跳过 '0'
        advance_char(lexer); // 跳过 'x'
        
        while (is_hex_digit(current_char(lexer))) {
            advance_char(lexer);
        }
    }
    // 处理八进制和十进制
    else {
        while (is_digit(current_char(lexer))) {
            advance_char(lexer);
        }
        
        // 检查小数点
        if (current_char(lexer) == '.') {
            is_float = 1;
            advance_char(lexer);
            while (is_digit(current_char(lexer))) {
                advance_char(lexer);
            }
        }
        
        // 检查指数部分
        if (current_char(lexer) == 'e' || current_char(lexer) == 'E') {
            is_float = 1;
            advance_char(lexer);
            if (current_char(lexer) == '+' || current_char(lexer) == '-') {
                advance_char(lexer);
            }
            while (is_digit(current_char(lexer))) {
                advance_char(lexer);
            }
        }
    }
    
    // 处理后缀
    char c = current_char(lexer);
    if (c == 'f' || c == 'F' || c == 'l' || c == 'L' || c == 'u' || c == 'U') {
        if (c == 'f' || c == 'F') is_float = 1;
        advance_char(lexer);
        // 可能有多个后缀
        c = current_char(lexer);
        if (c == 'l' || c == 'L' || c == 'u' || c == 'U') {
            advance_char(lexer);
        }
    }
    
    size_t length = lexer->position - start;
    const char* number = &lexer->source[start];
    
    Token* token = create_token(is_float ? TOKEN_FLOAT_CONSTANT : TOKEN_INTEGER_CONSTANT, 
                               number, length, lexer->line, start_column);
    
    // 解析数值
    if (token) {
        if (is_float) {
            token->numeric.float_value = strtod(number, NULL);
        } else {
            token->numeric.int_value = strtoll(number, NULL, 0);
        }
    }
    
    return token;
}

// 扫描字符常量
static Token* scan_char_constant(Lexer* lexer) {
    size_t start = lexer->position;
    int start_column = lexer->column;
    
    advance_char(lexer); // 跳过开始的单引号
    
    char char_value = 0;
    if (current_char(lexer) == '\\') {
        // 转义字符
        advance_char(lexer);
        switch (current_char(lexer)) {
            case 'n': char_value = '\n'; break;
            case 't': char_value = '\t'; break;
            case 'r': char_value = '\r'; break;
            case 'b': char_value = '\b'; break;
            case 'f': char_value = '\f'; break;
            case 'a': char_value = '\a'; break;
            case 'v': char_value = '\v'; break;
            case '\\': char_value = '\\'; break;
            case '\'': char_value = '\''; break;
            case '\"': char_value = '\"'; break;
            case '0': char_value = '\0'; break;
            default: char_value = current_char(lexer); break;
        }
        advance_char(lexer);
    } else {
        char_value = current_char(lexer);
        advance_char(lexer);
    }
    
    if (current_char(lexer) != '\'') {
        lexer_error(lexer, "Unterminated character constant");
        return create_token(TOKEN_ERROR, NULL, 0, lexer->line, start_column);
    }
    
    advance_char(lexer); // 跳过结束的单引号
    
    size_t length = lexer->position - start;
    Token* token = create_token(TOKEN_CHAR_CONSTANT, &lexer->source[start], length, lexer->line, start_column);
    if (token) {
        token->numeric.char_value = char_value;
    }
    
    return token;
}

// 扫描字符串字面量
static Token* scan_string_literal(Lexer* lexer) {
    size_t start = lexer->position;
    int start_column = lexer->column;
    
    advance_char(lexer); // 跳过开始的双引号
    
    while (current_char(lexer) != '\"' && current_char(lexer) != '\0') {
        if (current_char(lexer) == '\\') {
            advance_char(lexer); // 跳过转义字符
            if (current_char(lexer) != '\0') {
                advance_char(lexer);
            }
        } else {
            advance_char(lexer);
        }
    }
    
    if (current_char(lexer) != '\"') {
        lexer_error(lexer, "Unterminated string literal");
        return create_token(TOKEN_ERROR, NULL, 0, lexer->line, start_column);
    }
    
    advance_char(lexer); // 跳过结束的双引号
    
    size_t length = lexer->position - start;
    return create_token(TOKEN_STRING_LITERAL, &lexer->source[start], length, lexer->line, start_column);
}

// 扫描操作符
static Token* scan_operator(Lexer* lexer) {
    int start_column = lexer->column;
    char c = current_char(lexer);
    char next = peek_char(lexer, 1);
    
    switch (c) {
        case '+':
            advance_char(lexer);
            if (current_char(lexer) == '+') {
                advance_char(lexer);
                return create_token(TOKEN_INCREMENT, "++", 2, lexer->line, start_column);
            } else if (current_char(lexer) == '=') {
                advance_char(lexer);
                return create_token(TOKEN_PLUS_ASSIGN, "+=", 2, lexer->line, start_column);
            }
            return create_token(TOKEN_PLUS, "+", 1, lexer->line, start_column);
            
        case '-':
            advance_char(lexer);
            if (current_char(lexer) == '-') {
                advance_char(lexer);
                return create_token(TOKEN_DECREMENT, "--", 2, lexer->line, start_column);
            } else if (current_char(lexer) == '=') {
                advance_char(lexer);
                return create_token(TOKEN_MINUS_ASSIGN, "-=", 2, lexer->line, start_column);
            } else if (current_char(lexer) == '>') {
                advance_char(lexer);
                return create_token(TOKEN_ARROW, "->", 2, lexer->line, start_column);
            }
            return create_token(TOKEN_MINUS, "-", 1, lexer->line, start_column);
            
        case '*':
            advance_char(lexer);
            if (current_char(lexer) == '=') {
                advance_char(lexer);
                return create_token(TOKEN_MULT_ASSIGN, "*=", 2, lexer->line, start_column);
            }
            return create_token(TOKEN_MULTIPLY, "*", 1, lexer->line, start_column);
            
        case '/':
            advance_char(lexer);
            if (current_char(lexer) == '=') {
                advance_char(lexer);
                return create_token(TOKEN_DIV_ASSIGN, "/=", 2, lexer->line, start_column);
            }
            return create_token(TOKEN_DIVIDE, "/", 1, lexer->line, start_column);
            
        case '%':
            advance_char(lexer);
            if (current_char(lexer) == '=') {
                advance_char(lexer);
                return create_token(TOKEN_MOD_ASSIGN, "%=", 2, lexer->line, start_column);
            }
            return create_token(TOKEN_MODULO, "%", 1, lexer->line, start_column);
            
        case '=':
            advance_char(lexer);
            if (current_char(lexer) == '=') {
                advance_char(lexer);
                return create_token(TOKEN_EQUAL, "==", 2, lexer->line, start_column);
            }
            return create_token(TOKEN_ASSIGN, "=", 1, lexer->line, start_column);
            
        case '!':
            advance_char(lexer);
            if (current_char(lexer) == '=') {
                advance_char(lexer);
                return create_token(TOKEN_NOT_EQUAL, "!=", 2, lexer->line, start_column);
            }
            return create_token(TOKEN_LOGICAL_NOT, "!", 1, lexer->line, start_column);
            
        case '<':
            advance_char(lexer);
            if (current_char(lexer) == '=') {
                advance_char(lexer);
                return create_token(TOKEN_LESS_EQUAL, "<=", 2, lexer->line, start_column);
            } else if (current_char(lexer) == '<') {
                advance_char(lexer);
                if (current_char(lexer) == '=') {
                    advance_char(lexer);
                    return create_token(TOKEN_LEFT_SHIFT_ASSIGN, "<<=", 3, lexer->line, start_column);
                }
                return create_token(TOKEN_LEFT_SHIFT, "<<", 2, lexer->line, start_column);
            }
            return create_token(TOKEN_LESS, "<", 1, lexer->line, start_column);
            
        case '>':
            advance_char(lexer);
            if (current_char(lexer) == '=') {
                advance_char(lexer);
                return create_token(TOKEN_GREATER_EQUAL, ">=", 2, lexer->line, start_column);
            } else if (current_char(lexer) == '>') {
                advance_char(lexer);
                if (current_char(lexer) == '=') {
                    advance_char(lexer);
                    return create_token(TOKEN_RIGHT_SHIFT_ASSIGN, ">>=", 3, lexer->line, start_column);
                }
                return create_token(TOKEN_RIGHT_SHIFT, ">>", 2, lexer->line, start_column);
            }
            return create_token(TOKEN_GREATER, ">", 1, lexer->line, start_column);
            
        case '&':
            advance_char(lexer);
            if (current_char(lexer) == '&') {
                advance_char(lexer);
                return create_token(TOKEN_LOGICAL_AND, "&&", 2, lexer->line, start_column);
            } else if (current_char(lexer) == '=') {
                advance_char(lexer);
                return create_token(TOKEN_AND_ASSIGN, "&=", 2, lexer->line, start_column);
            }
            return create_token(TOKEN_BITWISE_AND, "&", 1, lexer->line, start_column);
            
        case '|':
            advance_char(lexer);
            if (current_char(lexer) == '|') {
                advance_char(lexer);
                return create_token(TOKEN_LOGICAL_OR, "||", 2, lexer->line, start_column);
            } else if (current_char(lexer) == '=') {
                advance_char(lexer);
                return create_token(TOKEN_OR_ASSIGN, "|=", 2, lexer->line, start_column);
            }
            return create_token(TOKEN_BITWISE_OR, "|", 1, lexer->line, start_column);
            
        case '^':
            advance_char(lexer);
            if (current_char(lexer) == '=') {
                advance_char(lexer);
                return create_token(TOKEN_XOR_ASSIGN, "^=", 2, lexer->line, start_column);
            }
            return create_token(TOKEN_BITWISE_XOR, "^", 1, lexer->line, start_column);
            
        case '~':
            advance_char(lexer);
            return create_token(TOKEN_BITWISE_NOT, "~", 1, lexer->line, start_column);
            
        case '.':
            advance_char(lexer);
            return create_token(TOKEN_DOT, ".", 1, lexer->line, start_column);
            
        case '?':
            advance_char(lexer);
            return create_token(TOKEN_QUESTION, "?", 1, lexer->line, start_column);
            
        case ':':
            advance_char(lexer);
            return create_token(TOKEN_COLON, ":", 1, lexer->line, start_column);
            
        case ';':
            advance_char(lexer);
            return create_token(TOKEN_SEMICOLON, ";", 1, lexer->line, start_column);
            
        case ',':
            advance_char(lexer);
            return create_token(TOKEN_COMMA, ",", 1, lexer->line, start_column);
            
        case '(':
            advance_char(lexer);
            return create_token(TOKEN_LEFT_PAREN, "(", 1, lexer->line, start_column);
            
        case ')':
            advance_char(lexer);
            return create_token(TOKEN_RIGHT_PAREN, ")", 1, lexer->line, start_column);
            
        case '{':
            advance_char(lexer);
            return create_token(TOKEN_LEFT_BRACE, "{", 1, lexer->line, start_column);
            
        case '}':
            advance_char(lexer);
            return create_token(TOKEN_RIGHT_BRACE, "}", 1, lexer->line, start_column);
            
        case '[':
            advance_char(lexer);
            return create_token(TOKEN_LEFT_BRACKET, "[", 1, lexer->line, start_column);
            
        case ']':
            advance_char(lexer);
            return create_token(TOKEN_RIGHT_BRACKET, "]", 1, lexer->line, start_column);
            
        case '#':
            advance_char(lexer);
            if (current_char(lexer) == '#') {
                advance_char(lexer);
                return create_token(TOKEN_DOUBLE_HASH, "##", 2, lexer->line, start_column);
            }
            return create_token(TOKEN_HASH, "#", 1, lexer->line, start_column);
            
        default:
            advance_char(lexer);
            return create_token(TOKEN_UNKNOWN, &c, 1, lexer->line, start_column);
    }
}

// 获取下一个token
Token* lexer_next_token(Lexer* lexer) {
    if (!lexer) return NULL;
    
    // 跳过空白字符
    lexer_skip_whitespace(lexer);
    
    // 检查EOF
    if (lexer->position >= lexer->length) {
        return create_token(TOKEN_EOF, NULL, 0, lexer->line, lexer->column);
    }
    
    char c = current_char(lexer);
    
    // 标识符和关键字
    if (is_identifier_start(c)) {
        return scan_identifier(lexer);
    }
    
    // 数字常量
    if (is_digit(c)) {
        return scan_number(lexer);
    }
    
    // 字符常量
    if (c == '\'') {
        return scan_char_constant(lexer);
    }
    
    // 字符串字面量
    if (c == '\"') {
        return scan_string_literal(lexer);
    }
    
    // 换行符
    if (c == '\n') {
        advance_char(lexer);
        return create_token(TOKEN_NEWLINE, "\n", 1, lexer->line - 1, lexer->column);
    }
    
    // 操作符和分隔符
    return scan_operator(lexer);
}

// 获取token类型名称
const char* token_type_name(TokenType type) {
    if (type >= 0 && type < sizeof(token_names) / sizeof(token_names[0])) {
        return token_names[type];
    }
    return "UNKNOWN";
}

// 辅助函数实现
int is_identifier_start(char c) {
    return isalpha(c) || c == '_';
}

int is_identifier_char(char c) {
    return isalnum(c) || c == '_';
}

int is_digit(char c) {
    return isdigit(c);
}

int is_hex_digit(char c) {
    return isxdigit(c);
}

int is_octal_digit(char c) {
    return c >= '0' && c <= '7';
}

int is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r';
}

// 错误处理
void lexer_error(Lexer* lexer, const char* message) {
    if (lexer) {
        lexer->error_count++;
        snprintf(lexer->error_msg, sizeof(lexer->error_msg), 
                "Line %d, Column %d: %s", lexer->line, lexer->column, message);
    }
}

void lexer_warning(Lexer* lexer, const char* message) {
    if (lexer) {
        printf("Warning - Line %d, Column %d: %s\n", lexer->line, lexer->column, message);
    }
}

int lexer_has_error(const Lexer* lexer) {
    return lexer && lexer->error_count > 0;
}

const char* lexer_get_error(const Lexer* lexer) {
    return lexer ? lexer->error_msg : "No lexer";
}