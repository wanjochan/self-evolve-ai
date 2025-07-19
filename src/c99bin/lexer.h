/**
 * lexer.h - Enhanced C99 Lexical Analyzer
 * 
 * 完整的C99词法分析器，支持所有C99标准的token类型
 */

#ifndef C99BIN_LEXER_H
#define C99BIN_LEXER_H

#include <stdio.h>
#include <stdint.h>

// Token 类型枚举 - 支持完整的C99标准
typedef enum {
    // 标识符和字面量
    TOKEN_IDENTIFIER,
    TOKEN_INTEGER_CONSTANT,
    TOKEN_FLOAT_CONSTANT,
    TOKEN_CHAR_CONSTANT,
    TOKEN_STRING_LITERAL,
    
    // C99关键字
    TOKEN_AUTO,
    TOKEN_BREAK,
    TOKEN_CASE,
    TOKEN_CHAR,
    TOKEN_CONST,
    TOKEN_CONTINUE,
    TOKEN_DEFAULT,
    TOKEN_DO,
    TOKEN_DOUBLE,
    TOKEN_ELSE,
    TOKEN_ENUM,
    TOKEN_EXTERN,
    TOKEN_FLOAT,
    TOKEN_FOR,
    TOKEN_GOTO,
    TOKEN_IF,
    TOKEN_INLINE,
    TOKEN_INT,
    TOKEN_LONG,
    TOKEN_REGISTER,
    TOKEN_RESTRICT,
    TOKEN_RETURN,
    TOKEN_SHORT,
    TOKEN_SIGNED,
    TOKEN_SIZEOF,
    TOKEN_STATIC,
    TOKEN_STRUCT,
    TOKEN_SWITCH,
    TOKEN_TYPEDEF,
    TOKEN_UNION,
    TOKEN_UNSIGNED,
    TOKEN_VOID,
    TOKEN_VOLATILE,
    TOKEN_WHILE,
    TOKEN__BOOL,
    TOKEN__COMPLEX,
    TOKEN__IMAGINARY,
    
    // 操作符
    TOKEN_PLUS,           // +
    TOKEN_MINUS,          // -
    TOKEN_MULTIPLY,       // *
    TOKEN_DIVIDE,         // /
    TOKEN_MODULO,         // %
    TOKEN_ASSIGN,         // =
    TOKEN_PLUS_ASSIGN,    // +=
    TOKEN_MINUS_ASSIGN,   // -=
    TOKEN_MULT_ASSIGN,    // *=
    TOKEN_DIV_ASSIGN,     // /=
    TOKEN_MOD_ASSIGN,     // %=
    TOKEN_INCREMENT,      // ++
    TOKEN_DECREMENT,      // --
    TOKEN_EQUAL,          // ==
    TOKEN_NOT_EQUAL,      // !=
    TOKEN_LESS,           // <
    TOKEN_GREATER,        // >
    TOKEN_LESS_EQUAL,     // <=
    TOKEN_GREATER_EQUAL,  // >=
    TOKEN_LOGICAL_AND,    // &&
    TOKEN_LOGICAL_OR,     // ||
    TOKEN_LOGICAL_NOT,    // !
    TOKEN_BITWISE_AND,    // &
    TOKEN_BITWISE_OR,     // |
    TOKEN_BITWISE_XOR,    // ^
    TOKEN_BITWISE_NOT,    // ~
    TOKEN_LEFT_SHIFT,     // <<
    TOKEN_RIGHT_SHIFT,    // >>
    TOKEN_AND_ASSIGN,     // &=
    TOKEN_OR_ASSIGN,      // |=
    TOKEN_XOR_ASSIGN,     // ^=
    TOKEN_LEFT_SHIFT_ASSIGN,  // <<=
    TOKEN_RIGHT_SHIFT_ASSIGN, // >>=
    TOKEN_ARROW,          // ->
    TOKEN_DOT,            // .
    TOKEN_QUESTION,       // ?
    TOKEN_COLON,          // :
    
    // 分隔符
    TOKEN_SEMICOLON,      // ;
    TOKEN_COMMA,          // ,
    TOKEN_LEFT_PAREN,     // (
    TOKEN_RIGHT_PAREN,    // )
    TOKEN_LEFT_BRACE,     // {
    TOKEN_RIGHT_BRACE,    // }
    TOKEN_LEFT_BRACKET,   // [
    TOKEN_RIGHT_BRACKET,  // ]
    
    // 预处理器
    TOKEN_HASH,           // #
    TOKEN_DOUBLE_HASH,    // ##
    
    // 特殊token
    TOKEN_NEWLINE,
    TOKEN_WHITESPACE,
    TOKEN_COMMENT,
    TOKEN_EOF,
    TOKEN_ERROR,
    TOKEN_UNKNOWN
} TokenType;

// Token结构
typedef struct {
    TokenType type;
    char* value;          // Token的字符串值
    size_t length;        // 长度
    int line;             // 行号
    int column;           // 列号
    // 对于数值常量的额外信息
    union {
        long long int_value;
        double float_value;
        char char_value;
    } numeric;
} Token;

// 词法分析器状态
typedef struct {
    const char* source;   // 源代码
    size_t position;      // 当前位置
    size_t length;        // 源代码长度
    int line;             // 当前行
    int column;           // 当前列
    int error_count;      // 错误计数
    char error_msg[256];  // 错误消息
} Lexer;

// 词法分析器函数接口
Lexer* lexer_create(const char* source);
void lexer_destroy(Lexer* lexer);
Token* lexer_next_token(Lexer* lexer);
void token_destroy(Token* token);
const char* token_type_name(TokenType type);
void lexer_skip_whitespace(Lexer* lexer);
int lexer_has_error(const Lexer* lexer);
const char* lexer_get_error(const Lexer* lexer);

// 辅助函数
int is_identifier_start(char c);
int is_identifier_char(char c);
int is_digit(char c);
int is_hex_digit(char c);
int is_octal_digit(char c);
int is_whitespace(char c);
const char* keyword_lookup(const char* identifier);

// 错误处理
void lexer_error(Lexer* lexer, const char* message);
void lexer_warning(Lexer* lexer, const char* message);

#endif // C99BIN_LEXER_H