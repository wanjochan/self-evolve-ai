/**
 * c99_lexer.c - C99 Lexical Analyzer Implementation
 */

#include "c99_lexer.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

// Forward declarations
static Token* scan_identifier(LexerContext* lexer);
static Token* scan_number(LexerContext* lexer);
static Token* scan_string(LexerContext* lexer);
static Token* scan_character(LexerContext* lexer);
static Token* create_token(TokenType type, const char* value, size_t length, int line, int column);

// ===============================================
// Keyword Table
// ===============================================

typedef struct {
    const char* keyword;
    TokenType token_type;
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
    {NULL, TOKEN_EOF}
};

// ===============================================
// Helper Functions
// ===============================================

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_alnum(char c) {
    return is_alpha(c) || is_digit(c);
}

static bool is_hex_digit(char c) {
    return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static char peek_char(LexerContext* lexer, size_t offset) {
    size_t pos = lexer->position + offset;
    if (pos >= lexer->source_length) {
        return '\0';
    }
    return lexer->source[pos];
}

static char advance_char(LexerContext* lexer) {
    if (lexer->position >= lexer->source_length) {
        return '\0';
    }
    
    char c = lexer->source[lexer->position++];
    
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    
    return c;
}

static TokenType lookup_keyword(const char* identifier) {
    for (int i = 0; keywords[i].keyword != NULL; i++) {
        if (strcmp(identifier, keywords[i].keyword) == 0) {
            return keywords[i].token_type;
        }
    }
    return TOKEN_IDENTIFIER;
}

static void set_error(LexerContext* lexer, const char* message) {
    lexer->has_error = true;
    snprintf(lexer->error_message, sizeof(lexer->error_message), 
             "Line %d, Column %d: %s", lexer->line, lexer->column, message);
}

// ===============================================
// Token Creation Functions
// ===============================================

static Token* create_token(TokenType type, const char* value, size_t length, int line, int column) {
    Token* token = malloc(sizeof(Token));
    if (!token) return NULL;
    
    memset(token, 0, sizeof(Token));
    token->type = type;
    token->length = length;
    token->line = line;
    token->column = column;
    
    if (value && length > 0) {
        token->value = malloc(length + 1);
        if (token->value) {
            memcpy(token->value, value, length);
            token->value[length] = '\0';
        }
    }
    
    return token;
}

// ===============================================
// Main Lexer Functions
// ===============================================

LexerContext* lexer_create(const char* source, size_t length) {
    if (!source) return NULL;
    
    LexerContext* lexer = malloc(sizeof(LexerContext));
    if (!lexer) return NULL;
    
    memset(lexer, 0, sizeof(LexerContext));
    lexer->source = source;
    lexer->source_length = length;
    lexer->line = 1;
    lexer->column = 1;
    lexer->skip_whitespace = true;
    lexer->skip_comments = true;
    
    return lexer;
}

void lexer_destroy(LexerContext* lexer) {
    if (!lexer) return;
    
    if (lexer->current_token.value) {
        free(lexer->current_token.value);
    }
    
    free(lexer);
}

Token* lexer_next_token(LexerContext* lexer) {
    if (!lexer || lexer->has_error) {
        return NULL;
    }
    
    // Skip whitespace if enabled
    while (lexer->skip_whitespace && lexer->position < lexer->source_length) {
        char c = peek_char(lexer, 0);
        if (c == ' ' || c == '\t' || c == '\r' || (c == '\n' && !lexer->track_newlines)) {
            advance_char(lexer);
        } else {
            break;
        }
    }
    
    // Check for end of file
    if (lexer->position >= lexer->source_length) {
        return create_token(TOKEN_EOF, NULL, 0, lexer->line, lexer->column);
    }
    
    char c = peek_char(lexer, 0);
    
    // Identifiers and keywords
    if (is_alpha(c)) {
        return scan_identifier(lexer);
    }
    
    // Numbers
    if (is_digit(c)) {
        return scan_number(lexer);
    }
    
    // Single character tokens and operators
    int start_line = lexer->line;
    int start_column = lexer->column;
    
    switch (c) {
        case '+':
            advance_char(lexer);
            if (peek_char(lexer, 0) == '+') {
                advance_char(lexer);
                return create_token(TOKEN_INCREMENT, "++", 2, start_line, start_column);
            } else if (peek_char(lexer, 0) == '=') {
                advance_char(lexer);
                return create_token(TOKEN_PLUS_ASSIGN, "+=", 2, start_line, start_column);
            }
            return create_token(TOKEN_PLUS, "+", 1, start_line, start_column);
            
        case '-':
            advance_char(lexer);
            if (peek_char(lexer, 0) == '-') {
                advance_char(lexer);
                return create_token(TOKEN_DECREMENT, "--", 2, start_line, start_column);
            } else if (peek_char(lexer, 0) == '=') {
                advance_char(lexer);
                return create_token(TOKEN_MINUS_ASSIGN, "-=", 2, start_line, start_column);
            } else if (peek_char(lexer, 0) == '>') {
                advance_char(lexer);
                return create_token(TOKEN_ARROW, "->", 2, start_line, start_column);
            }
            return create_token(TOKEN_MINUS, "-", 1, start_line, start_column);
            
        case '*':
            advance_char(lexer);
            if (peek_char(lexer, 0) == '=') {
                advance_char(lexer);
                return create_token(TOKEN_MUL_ASSIGN, "*=", 2, start_line, start_column);
            }
            return create_token(TOKEN_MULTIPLY, "*", 1, start_line, start_column);
            
        case '/':
            advance_char(lexer);
            if (peek_char(lexer, 0) == '=') {
                advance_char(lexer);
                return create_token(TOKEN_DIV_ASSIGN, "/=", 2, start_line, start_column);
            }
            return create_token(TOKEN_DIVIDE, "/", 1, start_line, start_column);
            
        case '(':
            advance_char(lexer);
            return create_token(TOKEN_LPAREN, "(", 1, start_line, start_column);
            
        case ')':
            advance_char(lexer);
            return create_token(TOKEN_RPAREN, ")", 1, start_line, start_column);
            
        case '{':
            advance_char(lexer);
            return create_token(TOKEN_LBRACE, "{", 1, start_line, start_column);
            
        case '}':
            advance_char(lexer);
            return create_token(TOKEN_RBRACE, "}", 1, start_line, start_column);
            
        case ';':
            advance_char(lexer);
            return create_token(TOKEN_SEMICOLON, ";", 1, start_line, start_column);
            
        case ',':
            advance_char(lexer);
            return create_token(TOKEN_COMMA, ",", 1, start_line, start_column);
            
        default:
            advance_char(lexer);
            set_error(lexer, "Unexpected character");
            return NULL;
    }
}

const char* token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "ERROR";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_INTEGER_LITERAL: return "INTEGER";
        case TOKEN_FLOAT_LITERAL: return "FLOAT";
        case TOKEN_IF: return "IF";
        case TOKEN_ELSE: return "ELSE";
        case TOKEN_WHILE: return "WHILE";
        case TOKEN_PLUS: return "PLUS";
        case TOKEN_MINUS: return "MINUS";
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        default: return "UNKNOWN";
    }
}

void token_print(const Token* token) {
    if (!token) return;
    
    printf("Token: %s", token_type_name(token->type));
    if (token->value) {
        printf(" '%s'", token->value);
    }
    printf(" at %d:%d\n", token->line, token->column);
}

bool lexer_has_error(LexerContext* lexer) {
    return lexer && lexer->has_error;
}

const char* lexer_get_error(LexerContext* lexer) {
    return lexer ? lexer->error_message : "Invalid lexer";
}

// Placeholder implementations for missing functions
static Token* scan_identifier(LexerContext* lexer) {
    size_t start = lexer->position;
    int start_line = lexer->line;
    int start_column = lexer->column;
    
    while (is_alnum(peek_char(lexer, 0))) {
        advance_char(lexer);
    }
    
    size_t length = lexer->position - start;
    const char* identifier = lexer->source + start;
    
    char* temp = malloc(length + 1);
    if (!temp) return NULL;
    
    memcpy(temp, identifier, length);
    temp[length] = '\0';
    
    TokenType type = lookup_keyword(temp);
    Token* token = create_token(type, identifier, length, start_line, start_column);
    
    free(temp);
    return token;
}

static Token* scan_number(LexerContext* lexer) {
    size_t start = lexer->position;
    int start_line = lexer->line;
    int start_column = lexer->column;
    
    while (is_digit(peek_char(lexer, 0))) {
        advance_char(lexer);
    }
    
    return create_token(TOKEN_INTEGER_LITERAL, lexer->source + start, 
                       lexer->position - start, start_line, start_column);
}
