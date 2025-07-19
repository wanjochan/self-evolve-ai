/**
 * c99_lexer.c - 完整的C99词法分析器实现
 */

#include "c99_lexer.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

// ===============================================
// 关键字表
// ===============================================

typedef struct {
    const char* keyword;
    TokenType type;
} KeywordEntry;

static const KeywordEntry keywords[] = {
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
    {"_Bool", TOKEN_BOOL},
    {"_Complex", TOKEN_COMPLEX},
    {"_Imaginary", TOKEN_IMAGINARY},
    {NULL, TOKEN_UNKNOWN}
};

// ===============================================
// Token类型名称表
// ===============================================

static const char* token_names[] = {
    [TOKEN_EOF] = "EOF",
    [TOKEN_ERROR] = "ERROR",
    [TOKEN_UNKNOWN] = "UNKNOWN",
    [TOKEN_IDENTIFIER] = "IDENTIFIER",
    [TOKEN_INTEGER_CONSTANT] = "INTEGER_CONSTANT",
    [TOKEN_FLOATING_CONSTANT] = "FLOATING_CONSTANT",
    [TOKEN_CHARACTER_CONSTANT] = "CHARACTER_CONSTANT",
    [TOKEN_STRING_LITERAL] = "STRING_LITERAL",
    [TOKEN_AUTO] = "auto",
    [TOKEN_BREAK] = "break",
    [TOKEN_CASE] = "case",
    [TOKEN_CHAR] = "char",
    [TOKEN_CONST] = "const",
    [TOKEN_CONTINUE] = "continue",
    [TOKEN_DEFAULT] = "default",
    [TOKEN_DO] = "do",
    [TOKEN_DOUBLE] = "double",
    [TOKEN_ELSE] = "else",
    [TOKEN_ENUM] = "enum",
    [TOKEN_EXTERN] = "extern",
    [TOKEN_FLOAT] = "float",
    [TOKEN_FOR] = "for",
    [TOKEN_GOTO] = "goto",
    [TOKEN_IF] = "if",
    [TOKEN_INLINE] = "inline",
    [TOKEN_INT] = "int",
    [TOKEN_LONG] = "long",
    [TOKEN_REGISTER] = "register",
    [TOKEN_RESTRICT] = "restrict",
    [TOKEN_RETURN] = "return",
    [TOKEN_SHORT] = "short",
    [TOKEN_SIGNED] = "signed",
    [TOKEN_SIZEOF] = "sizeof",
    [TOKEN_STATIC] = "static",
    [TOKEN_STRUCT] = "struct",
    [TOKEN_SWITCH] = "switch",
    [TOKEN_TYPEDEF] = "typedef",
    [TOKEN_UNION] = "union",
    [TOKEN_UNSIGNED] = "unsigned",
    [TOKEN_VOID] = "void",
    [TOKEN_VOLATILE] = "volatile",
    [TOKEN_WHILE] = "while",
    [TOKEN_BOOL] = "_Bool",
    [TOKEN_COMPLEX] = "_Complex",
    [TOKEN_IMAGINARY] = "_Imaginary",
    [TOKEN_PLUS] = "+",
    [TOKEN_MINUS] = "-",
    [TOKEN_STAR] = "*",
    [TOKEN_SLASH] = "/",
    [TOKEN_PERCENT] = "%",
    [TOKEN_PLUS_PLUS] = "++",
    [TOKEN_MINUS_MINUS] = "--",
    [TOKEN_AMPERSAND] = "&",
    [TOKEN_PIPE] = "|",
    [TOKEN_CARET] = "^",
    [TOKEN_TILDE] = "~",
    [TOKEN_EXCLAMATION] = "!",
    [TOKEN_LESS] = "<",
    [TOKEN_GREATER] = ">",
    [TOKEN_LESS_EQUAL] = "<=",
    [TOKEN_GREATER_EQUAL] = ">=",
    [TOKEN_EQUAL_EQUAL] = "==",
    [TOKEN_NOT_EQUAL] = "!=",
    [TOKEN_LOGICAL_AND] = "&&",
    [TOKEN_LOGICAL_OR] = "||",
    [TOKEN_QUESTION] = "?",
    [TOKEN_COLON] = ":",
    [TOKEN_SEMICOLON] = ";",
    [TOKEN_COMMA] = ",",
    [TOKEN_EQUAL] = "=",
    [TOKEN_PLUS_EQUAL] = "+=",
    [TOKEN_MINUS_EQUAL] = "-=",
    [TOKEN_STAR_EQUAL] = "*=",
    [TOKEN_SLASH_EQUAL] = "/=",
    [TOKEN_PERCENT_EQUAL] = "%=",
    [TOKEN_AMPERSAND_EQUAL] = "&=",
    [TOKEN_PIPE_EQUAL] = "|=",
    [TOKEN_CARET_EQUAL] = "^=",
    [TOKEN_LEFT_SHIFT] = "<<",
    [TOKEN_RIGHT_SHIFT] = ">>",
    [TOKEN_LEFT_SHIFT_EQUAL] = "<<=",
    [TOKEN_RIGHT_SHIFT_EQUAL] = ">>=",
    [TOKEN_LPAREN] = "(",
    [TOKEN_RPAREN] = ")",
    [TOKEN_LBRACE] = "{",
    [TOKEN_RBRACE] = "}",
    [TOKEN_LBRACKET] = "[",
    [TOKEN_RBRACKET] = "]",
    [TOKEN_DOT] = ".",
    [TOKEN_ARROW] = "->",
    [TOKEN_ELLIPSIS] = "...",
    [TOKEN_HASH] = "#",
    [TOKEN_HASH_HASH] = "##",
    [TOKEN_NEWLINE] = "NEWLINE",
    [TOKEN_WHITESPACE] = "WHITESPACE",
    [TOKEN_COMMENT] = "COMMENT"
};

// ===============================================
// 辅助函数
// ===============================================

static char current_char(C99Lexer* lexer) {
    if (lexer->current >= lexer->source_length) {
        return '\0';
    }
    return lexer->source[lexer->current];
}

static char peek_char(C99Lexer* lexer, int offset) {
    size_t pos = lexer->current + offset;
    if (pos >= lexer->source_length) {
        return '\0';
    }
    return lexer->source[pos];
}

static void advance_char(C99Lexer* lexer) {
    if (lexer->current < lexer->source_length) {
        char c = lexer->source[lexer->current];
        lexer->current++;
        
        if (c == '\n') {
            lexer->line++;
            lexer->column = 1;
            lexer->line_start = lexer->current;
            lexer->at_line_start = true;
        } else {
            lexer->column++;
            lexer->at_line_start = false;
        }
    }
}

// 前向声明
static Token* scan_string_literal(C99Lexer* lexer);
static Token* scan_character_constant(C99Lexer* lexer);

static Token* create_token(TokenType type, const char* value, size_t length, 
                          int line, int column, size_t offset) {
    Token* token = malloc(sizeof(Token));
    if (!token) return NULL;
    
    token->type = type;
    token->length = length;
    token->line = line;
    token->column = column;
    token->offset = offset;
    
    if (value && length > 0) {
        token->value = malloc(length + 1);
        if (token->value) {
            memcpy(token->value, value, length);
            token->value[length] = '\0';
        }
    } else {
        token->value = NULL;
    }
    
    // 初始化扩展数据
    memset(&token->data, 0, sizeof(token->data));
    
    return token;
}

static void set_error(C99Lexer* lexer, const char* message) {
    lexer->has_error = true;
    strncpy(lexer->error_message, message, sizeof(lexer->error_message) - 1);
    lexer->error_message[sizeof(lexer->error_message) - 1] = '\0';
}

// ===============================================
// 词法分析器API实现
// ===============================================

void c99_lexer_init(C99Lexer* lexer, const char* source, size_t length) {
    lexer->source = source;
    lexer->source_length = length;
    lexer->current = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->line_start = 0;
    lexer->at_line_start = true;
    lexer->in_directive = false;
    lexer->has_error = false;
    lexer->error_message[0] = '\0';
    
    // 默认配置
    lexer->skip_whitespace = true;
    lexer->skip_comments = true;
    lexer->track_newlines = false;
}

bool c99_is_keyword(const char* identifier) {
    for (int i = 0; keywords[i].keyword; i++) {
        if (strcmp(identifier, keywords[i].keyword) == 0) {
            return true;
        }
    }
    return false;
}

TokenType c99_get_keyword_type(const char* identifier) {
    for (int i = 0; keywords[i].keyword; i++) {
        if (strcmp(identifier, keywords[i].keyword) == 0) {
            return keywords[i].type;
        }
    }
    return TOKEN_IDENTIFIER;
}

const char* c99_token_type_name(TokenType type) {
    if (type >= 0 && type < TOKEN_COUNT && token_names[type]) {
        return token_names[type];
    }
    return "UNKNOWN";
}

void c99_token_free(Token* token) {
    if (token) {
        if (token->value) {
            free(token->value);
        }
        free(token);
    }
}

bool c99_lexer_has_error(C99Lexer* lexer) {
    return lexer->has_error;
}

const char* c99_lexer_get_error(C99Lexer* lexer) {
    return lexer->error_message;
}

void c99_lexer_get_position(C99Lexer* lexer, int* line, int* column) {
    if (line) *line = lexer->line;
    if (column) *column = lexer->column;
}

// ===============================================
// Token识别函数
// ===============================================

static bool is_identifier_start(char c) {
    return isalpha(c) || c == '_';
}

static bool is_identifier_continue(char c) {
    return isalnum(c) || c == '_';
}

static Token* scan_identifier(C99Lexer* lexer) {
    size_t start = lexer->current;
    int start_line = lexer->line;
    int start_column = lexer->column;

    // 扫描标识符
    while (is_identifier_continue(current_char(lexer))) {
        advance_char(lexer);
    }

    size_t length = lexer->current - start;
    const char* text = &lexer->source[start];

    // 检查是否为关键字
    char* identifier = malloc(length + 1);
    if (!identifier) {
        set_error(lexer, "Memory allocation failed");
        return NULL;
    }

    memcpy(identifier, text, length);
    identifier[length] = '\0';

    TokenType type = c99_get_keyword_type(identifier);
    Token* token = create_token(type, text, length, start_line, start_column, start);

    free(identifier);
    return token;
}

static Token* scan_number(C99Lexer* lexer) {
    size_t start = lexer->current;
    int start_line = lexer->line;
    int start_column = lexer->column;

    bool is_float = false;
    int base = 10;

    // 检查进制前缀
    if (current_char(lexer) == '0') {
        advance_char(lexer);
        char next = current_char(lexer);
        if (next == 'x' || next == 'X') {
            base = 16;
            advance_char(lexer);
        } else if (isdigit(next)) {
            base = 8;
        }
    }

    // 扫描数字部分
    while (true) {
        char c = current_char(lexer);
        if (base == 16 && isxdigit(c)) {
            advance_char(lexer);
        } else if (base == 8 && c >= '0' && c <= '7') {
            advance_char(lexer);
        } else if (base == 10 && isdigit(c)) {
            advance_char(lexer);
        } else {
            break;
        }
    }

    // 检查小数点
    if (current_char(lexer) == '.' && base == 10) {
        is_float = true;
        advance_char(lexer);
        while (isdigit(current_char(lexer))) {
            advance_char(lexer);
        }
    }

    // 检查指数
    if ((current_char(lexer) == 'e' || current_char(lexer) == 'E') && base == 10) {
        is_float = true;
        advance_char(lexer);
        if (current_char(lexer) == '+' || current_char(lexer) == '-') {
            advance_char(lexer);
        }
        while (isdigit(current_char(lexer))) {
            advance_char(lexer);
        }
    }

    // 检查后缀
    bool is_unsigned = false;
    bool is_long = false;
    bool is_long_long = false;
    bool is_float_suffix = false;
    bool is_long_double = false;

    while (true) {
        char c = current_char(lexer);
        if (c == 'u' || c == 'U') {
            if (is_unsigned) break;
            is_unsigned = true;
            advance_char(lexer);
        } else if (c == 'l' || c == 'L') {
            if (is_float) {
                if (is_long_double) break;
                is_long_double = true;
                advance_char(lexer);
            } else {
                if (is_long_long) break;
                if (is_long) {
                    is_long_long = true;
                    is_long = false;
                } else {
                    is_long = true;
                }
                advance_char(lexer);
            }
        } else if ((c == 'f' || c == 'F') && is_float) {
            if (is_float_suffix) break;
            is_float_suffix = true;
            advance_char(lexer);
        } else {
            break;
        }
    }

    size_t length = lexer->current - start;
    const char* text = &lexer->source[start];

    TokenType type = is_float ? TOKEN_FLOATING_CONSTANT : TOKEN_INTEGER_CONSTANT;
    Token* token = create_token(type, text, length, start_line, start_column, start);

    if (token) {
        if (is_float) {
            token->data.floating.is_float = is_float_suffix;
            token->data.floating.is_long_double = is_long_double;
            // 解析浮点值
            char* endptr;
            token->data.floating.float_value = strtod(text, &endptr);
        } else {
            token->data.integer.base = base;
            token->data.integer.is_unsigned = is_unsigned;
            token->data.integer.is_long = is_long;
            token->data.integer.is_long_long = is_long_long;
            // 解析整数值
            char* endptr;
            if (is_unsigned) {
                token->data.integer.int_value = (long long)strtoull(text, &endptr, base);
            } else {
                token->data.integer.int_value = strtoll(text, &endptr, base);
            }
        }
    }

    return token;
}

void c99_lexer_skip_whitespace(C99Lexer* lexer) {
    while (true) {
        char c = current_char(lexer);
        if (c == ' ' || c == '\t' || c == '\r') {
            advance_char(lexer);
        } else if (c == '\n' && lexer->skip_whitespace) {
            advance_char(lexer);
        } else {
            break;
        }
    }
}

static Token* scan_operator(C99Lexer* lexer) {
    size_t start = lexer->current;
    int start_line = lexer->line;
    int start_column = lexer->column;

    char c = current_char(lexer);
    char next = peek_char(lexer, 1);
    char next2 = peek_char(lexer, 2);

    TokenType type = TOKEN_UNKNOWN;
    int length = 1;

    // 三字符操作符
    if (c == '<' && next == '<' && next2 == '=') {
        type = TOKEN_LEFT_SHIFT_EQUAL;
        length = 3;
    } else if (c == '>' && next == '>' && next2 == '=') {
        type = TOKEN_RIGHT_SHIFT_EQUAL;
        length = 3;
    } else if (c == '.' && next == '.' && next2 == '.') {
        type = TOKEN_ELLIPSIS;
        length = 3;
    }
    // 双字符操作符
    else if (c == '+' && next == '+') {
        type = TOKEN_PLUS_PLUS;
        length = 2;
    } else if (c == '-' && next == '-') {
        type = TOKEN_MINUS_MINUS;
        length = 2;
    } else if (c == '-' && next == '>') {
        type = TOKEN_ARROW;
        length = 2;
    } else if (c == '<' && next == '=') {
        type = TOKEN_LESS_EQUAL;
        length = 2;
    } else if (c == '>' && next == '=') {
        type = TOKEN_GREATER_EQUAL;
        length = 2;
    } else if (c == '=' && next == '=') {
        type = TOKEN_EQUAL_EQUAL;
        length = 2;
    } else if (c == '!' && next == '=') {
        type = TOKEN_NOT_EQUAL;
        length = 2;
    } else if (c == '&' && next == '&') {
        type = TOKEN_LOGICAL_AND;
        length = 2;
    } else if (c == '|' && next == '|') {
        type = TOKEN_LOGICAL_OR;
        length = 2;
    } else if (c == '<' && next == '<') {
        type = TOKEN_LEFT_SHIFT;
        length = 2;
    } else if (c == '>' && next == '>') {
        type = TOKEN_RIGHT_SHIFT;
        length = 2;
    } else if (c == '+' && next == '=') {
        type = TOKEN_PLUS_EQUAL;
        length = 2;
    } else if (c == '-' && next == '=') {
        type = TOKEN_MINUS_EQUAL;
        length = 2;
    } else if (c == '*' && next == '=') {
        type = TOKEN_STAR_EQUAL;
        length = 2;
    } else if (c == '/' && next == '=') {
        type = TOKEN_SLASH_EQUAL;
        length = 2;
    } else if (c == '%' && next == '=') {
        type = TOKEN_PERCENT_EQUAL;
        length = 2;
    } else if (c == '&' && next == '=') {
        type = TOKEN_AMPERSAND_EQUAL;
        length = 2;
    } else if (c == '|' && next == '=') {
        type = TOKEN_PIPE_EQUAL;
        length = 2;
    } else if (c == '^' && next == '=') {
        type = TOKEN_CARET_EQUAL;
        length = 2;
    } else if (c == '#' && next == '#') {
        type = TOKEN_HASH_HASH;
        length = 2;
    }
    // 单字符操作符
    else {
        switch (c) {
            case '+': type = TOKEN_PLUS; break;
            case '-': type = TOKEN_MINUS; break;
            case '*': type = TOKEN_STAR; break;
            case '/': type = TOKEN_SLASH; break;
            case '%': type = TOKEN_PERCENT; break;
            case '&': type = TOKEN_AMPERSAND; break;
            case '|': type = TOKEN_PIPE; break;
            case '^': type = TOKEN_CARET; break;
            case '~': type = TOKEN_TILDE; break;
            case '!': type = TOKEN_EXCLAMATION; break;
            case '<': type = TOKEN_LESS; break;
            case '>': type = TOKEN_GREATER; break;
            case '=': type = TOKEN_EQUAL; break;
            case '?': type = TOKEN_QUESTION; break;
            case ':': type = TOKEN_COLON; break;
            case ';': type = TOKEN_SEMICOLON; break;
            case ',': type = TOKEN_COMMA; break;
            case '(': type = TOKEN_LPAREN; break;
            case ')': type = TOKEN_RPAREN; break;
            case '{': type = TOKEN_LBRACE; break;
            case '}': type = TOKEN_RBRACE; break;
            case '[': type = TOKEN_LBRACKET; break;
            case ']': type = TOKEN_RBRACKET; break;
            case '.': type = TOKEN_DOT; break;
            case '#': type = TOKEN_HASH; break;
            default:
                set_error(lexer, "Unknown operator");
                return NULL;
        }
    }

    // 前进相应的字符数
    for (int i = 0; i < length; i++) {
        advance_char(lexer);
    }

    const char* text = &lexer->source[start];
    return create_token(type, text, length, start_line, start_column, start);
}

Token* c99_lexer_next_token(C99Lexer* lexer) {
    if (lexer->has_error) {
        return create_token(TOKEN_ERROR, NULL, 0, lexer->line, lexer->column, lexer->current);
    }

    // 跳过空白字符和注释
    if (lexer->skip_whitespace) {
        c99_lexer_skip_whitespace(lexer);
    }

    // 检查是否到达文件末尾
    if (lexer->current >= lexer->source_length) {
        return create_token(TOKEN_EOF, NULL, 0, lexer->line, lexer->column, lexer->current);
    }

    char c = current_char(lexer);

    // 标识符和关键字
    if (is_identifier_start(c)) {
        return scan_identifier(lexer);
    }

    // 数字常量
    if (isdigit(c) || (c == '.' && isdigit(peek_char(lexer, 1)))) {
        return scan_number(lexer);
    }

    // 字符串字面量
    if (c == '"') {
        return scan_string_literal(lexer);
    }

    // 字符常量
    if (c == '\'') {
        return scan_character_constant(lexer);
    }

    // 换行符
    if (c == '\n' && lexer->track_newlines) {
        advance_char(lexer);
        return create_token(TOKEN_NEWLINE, "\n", 1, lexer->line - 1, lexer->column, lexer->current - 1);
    }

    // 注释
    if (c == '/' && peek_char(lexer, 1) == '/') {
        // 单行注释
        size_t start = lexer->current;
        int start_line = lexer->line;
        int start_column = lexer->column;

        while (current_char(lexer) != '\n' && current_char(lexer) != '\0') {
            advance_char(lexer);
        }

        if (!lexer->skip_comments) {
            size_t length = lexer->current - start;
            const char* text = &lexer->source[start];
            return create_token(TOKEN_COMMENT, text, length, start_line, start_column, start);
        } else {
            return c99_lexer_next_token(lexer); // 递归获取下一个token
        }
    }

    if (c == '/' && peek_char(lexer, 1) == '*') {
        // 多行注释
        size_t start = lexer->current;
        int start_line = lexer->line;
        int start_column = lexer->column;

        advance_char(lexer); // 跳过 '/'
        advance_char(lexer); // 跳过 '*'

        while (true) {
            char curr = current_char(lexer);
            if (curr == '\0') {
                set_error(lexer, "Unterminated comment");
                return create_token(TOKEN_ERROR, NULL, 0, lexer->line, lexer->column, lexer->current);
            }

            if (curr == '*' && peek_char(lexer, 1) == '/') {
                advance_char(lexer); // 跳过 '*'
                advance_char(lexer); // 跳过 '/'
                break;
            }

            advance_char(lexer);
        }

        if (!lexer->skip_comments) {
            size_t length = lexer->current - start;
            const char* text = &lexer->source[start];
            return create_token(TOKEN_COMMENT, text, length, start_line, start_column, start);
        } else {
            return c99_lexer_next_token(lexer); // 递归获取下一个token
        }
    }

    // 操作符和分隔符
    if (strchr("+-*/%&|^~!<>=?:;,(){}[].#", c)) {
        return scan_operator(lexer);
    }

    // 未知字符
    set_error(lexer, "Unknown character");
    advance_char(lexer);
    return create_token(TOKEN_ERROR, &c, 1, lexer->line, lexer->column - 1, lexer->current - 1);
}

Token* c99_lexer_peek_token(C99Lexer* lexer) {
    // 保存当前状态
    size_t saved_current = lexer->current;
    int saved_line = lexer->line;
    int saved_column = lexer->column;
    int saved_line_start = lexer->line_start;
    bool saved_at_line_start = lexer->at_line_start;
    bool saved_in_directive = lexer->in_directive;

    // 获取下一个token
    Token* token = c99_lexer_next_token(lexer);

    // 恢复状态
    lexer->current = saved_current;
    lexer->line = saved_line;
    lexer->column = saved_column;
    lexer->line_start = saved_line_start;
    lexer->at_line_start = saved_at_line_start;
    lexer->in_directive = saved_in_directive;

    return token;
}

// 扫描字符串字面量
static Token* scan_string_literal(C99Lexer* lexer) {
    int start_line = lexer->line;
    int start_column = lexer->column;
    size_t start_pos = lexer->current;
    
    advance_char(lexer); // 跳过开始的 '"'
    
    while (lexer->current < lexer->source_length && peek_char(lexer, 0) != '"') {
        char c = peek_char(lexer, 0);
        if (c == '\\') {
            advance_char(lexer); // 跳过转义字符
            if (lexer->current < lexer->source_length) {
                advance_char(lexer); // 跳过被转义的字符
            }
        } else if (c == '\n') {
            // 字符串不能跨行
            break;
        } else {
            advance_char(lexer);
        }
    }
    
    if (lexer->current < lexer->source_length && peek_char(lexer, 0) == '"') {
        advance_char(lexer); // 跳过结束的 '"'
    }
    
    size_t length = lexer->current - start_pos;
    const char* text = lexer->source + start_pos;
    
    return create_token(TOKEN_STRING_LITERAL, text, length, start_line, start_column, start_pos);
}

// 扫描字符常量
static Token* scan_character_constant(C99Lexer* lexer) {
    int start_line = lexer->line;
    int start_column = lexer->column;
    size_t start_pos = lexer->current;
    
    advance_char(lexer); // 跳过开始的 '\''
    
    if (lexer->current < lexer->source_length && peek_char(lexer, 0) != '\'') {
        char c = peek_char(lexer, 0);
        if (c == '\\') {
            advance_char(lexer); // 跳过转义字符
            if (lexer->current < lexer->source_length) {
                advance_char(lexer); // 跳过被转义的字符
            }
        } else {
            advance_char(lexer);
        }
    }
    
    if (lexer->current < lexer->source_length && peek_char(lexer, 0) == '\'') {
        advance_char(lexer); // 跳过结束的 '\''
    }
    
    size_t length = lexer->current - start_pos;
    const char* text = lexer->source + start_pos;
    
    return create_token(TOKEN_CHARACTER_CONSTANT, text, length, start_line, start_column, start_pos);
}
