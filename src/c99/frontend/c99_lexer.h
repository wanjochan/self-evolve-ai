/**
 * c99_lexer.h - C99 Lexical Analyzer
 * 
 * Complete C99 lexical analyzer supporting all C99 keywords, operators,
 * identifiers, and literals according to ISO/IEC 9899:1999 standard.
 */

#ifndef C99_LEXER_H
#define C99_LEXER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// Token Types
// ===============================================

typedef enum {
    // End of file/input
    TOKEN_EOF = 0,
    TOKEN_ERROR,
    
    // Literals
    TOKEN_INTEGER_LITERAL,
    TOKEN_FLOAT_LITERAL,
    TOKEN_CHAR_LITERAL,
    TOKEN_STRING_LITERAL,
    
    // Identifiers
    TOKEN_IDENTIFIER,
    
    // C99 Keywords
    TOKEN_AUTO,         // auto
    TOKEN_BREAK,        // break
    TOKEN_CASE,         // case
    TOKEN_CHAR,         // char
    TOKEN_CONST,        // const
    TOKEN_CONTINUE,     // continue
    TOKEN_DEFAULT,      // default
    TOKEN_DO,           // do
    TOKEN_DOUBLE,       // double
    TOKEN_ELSE,         // else
    TOKEN_ENUM,         // enum
    TOKEN_EXTERN,       // extern
    TOKEN_FLOAT,        // float
    TOKEN_FOR,          // for
    TOKEN_GOTO,         // goto
    TOKEN_IF,           // if
    TOKEN_INLINE,       // inline (C99)
    TOKEN_INT,          // int
    TOKEN_LONG,         // long
    TOKEN_REGISTER,     // register
    TOKEN_RESTRICT,     // restrict (C99)
    TOKEN_RETURN,       // return
    TOKEN_SHORT,        // short
    TOKEN_SIGNED,       // signed
    TOKEN_SIZEOF,       // sizeof
    TOKEN_STATIC,       // static
    TOKEN_STRUCT,       // struct
    TOKEN_SWITCH,       // switch
    TOKEN_TYPEDEF,      // typedef
    TOKEN_UNION,        // union
    TOKEN_UNSIGNED,     // unsigned
    TOKEN_VOID,         // void
    TOKEN_VOLATILE,     // volatile
    TOKEN_WHILE,        // while
    TOKEN_BOOL,         // _Bool (C99)
    TOKEN_COMPLEX,      // _Complex (C99)
    TOKEN_IMAGINARY,    // _Imaginary (C99)
    
    // Operators
    TOKEN_PLUS,         // +
    TOKEN_MINUS,        // -
    TOKEN_MULTIPLY,     // *
    TOKEN_DIVIDE,       // /
    TOKEN_MODULO,       // %
    TOKEN_ASSIGN,       // =
    TOKEN_PLUS_ASSIGN,  // +=
    TOKEN_MINUS_ASSIGN, // -=
    TOKEN_MUL_ASSIGN,   // *=
    TOKEN_DIV_ASSIGN,   // /=
    TOKEN_MOD_ASSIGN,   // %=
    TOKEN_INCREMENT,    // ++
    TOKEN_DECREMENT,    // --
    
    // Comparison operators
    TOKEN_EQUAL,        // ==
    TOKEN_NOT_EQUAL,    // !=
    TOKEN_LESS,         // <
    TOKEN_LESS_EQUAL,   // <=
    TOKEN_GREATER,      // >
    TOKEN_GREATER_EQUAL,// >=
    
    // Logical operators
    TOKEN_LOGICAL_AND,  // &&
    TOKEN_LOGICAL_OR,   // ||
    TOKEN_LOGICAL_NOT,  // !
    
    // Bitwise operators
    TOKEN_BITWISE_AND,  // &
    TOKEN_BITWISE_OR,   // |
    TOKEN_BITWISE_XOR,  // ^
    TOKEN_BITWISE_NOT,  // ~
    TOKEN_LEFT_SHIFT,   // <<
    TOKEN_RIGHT_SHIFT,  // >>
    TOKEN_AND_ASSIGN,   // &=
    TOKEN_OR_ASSIGN,    // |=
    TOKEN_XOR_ASSIGN,   // ^=
    TOKEN_LSHIFT_ASSIGN,// <<=
    TOKEN_RSHIFT_ASSIGN,// >>=
    
    // Punctuation
    TOKEN_SEMICOLON,    // ;
    TOKEN_COMMA,        // ,
    TOKEN_DOT,          // .
    TOKEN_ARROW,        // ->
    TOKEN_QUESTION,     // ?
    TOKEN_COLON,        // :
    
    // Brackets and parentheses
    TOKEN_LPAREN,       // (
    TOKEN_RPAREN,       // )
    TOKEN_LBRACE,       // {
    TOKEN_RBRACE,       // }
    TOKEN_LBRACKET,     // [
    TOKEN_RBRACKET,     // ]
    
    // Preprocessor (basic support)
    TOKEN_HASH,         // #
    TOKEN_DOUBLE_HASH,  // ##
    
    // Special tokens
    TOKEN_ELLIPSIS,     // ... (C99 variadic)
    TOKEN_NEWLINE,      // \n (for preprocessor)
    TOKEN_WHITESPACE,   // spaces, tabs (usually skipped)
    
    TOKEN_COUNT
} TokenType;

// ===============================================
// Token Structure
// ===============================================

typedef struct {
    TokenType type;
    char* value;            // Token text
    size_t length;          // Token length
    int line;               // Line number
    int column;             // Column number
    
    // For numeric literals
    union {
        long long int_value;
        double float_value;
    } numeric;
    
    // Token flags
    bool is_unsigned;       // For integer literals
    bool is_long;           // For integer/float literals
    bool is_long_long;      // For integer literals
    bool is_float;          // For float literals (f suffix)
    bool is_long_double;    // For float literals (L suffix)
} Token;

// ===============================================
// Lexer Context
// ===============================================

typedef struct {
    const char* source;     // Source code
    size_t source_length;   // Source length
    size_t position;        // Current position
    int line;               // Current line
    int column;             // Current column
    
    // Current token
    Token current_token;
    
    // Error handling
    char error_message[512];
    bool has_error;
    
    // Options
    bool skip_whitespace;   // Skip whitespace tokens
    bool skip_comments;     // Skip comment tokens
    bool track_newlines;    // Track newline tokens
} LexerContext;

// ===============================================
// Lexer Functions
// ===============================================

/**
 * Initialize lexer context
 */
LexerContext* lexer_create(const char* source, size_t length);

/**
 * Destroy lexer context
 */
void lexer_destroy(LexerContext* lexer);

/**
 * Get next token
 */
Token* lexer_next_token(LexerContext* lexer);

/**
 * Peek at next token without consuming it
 */
Token* lexer_peek_token(LexerContext* lexer);

/**
 * Check if current token matches expected type
 */
bool lexer_match(LexerContext* lexer, TokenType expected);

/**
 * Consume token if it matches expected type
 */
bool lexer_consume(LexerContext* lexer, TokenType expected);

/**
 * Get current token
 */
Token* lexer_current_token(LexerContext* lexer);

/**
 * Check if lexer has error
 */
bool lexer_has_error(LexerContext* lexer);

/**
 * Get error message
 */
const char* lexer_get_error(LexerContext* lexer);

/**
 * Reset lexer to beginning
 */
void lexer_reset(LexerContext* lexer);

/**
 * Get token type name (for debugging)
 */
const char* token_type_name(TokenType type);

/**
 * Print token (for debugging)
 */
void token_print(const Token* token);

/**
 * Free token resources
 */
void token_free(Token* token);

/**
 * Check if token is a keyword
 */
bool token_is_keyword(TokenType type);

/**
 * Check if token is an operator
 */
bool token_is_operator(TokenType type);

/**
 * Check if token is a literal
 */
bool token_is_literal(TokenType type);

#ifdef __cplusplus
}
#endif

#endif // C99_LEXER_H
