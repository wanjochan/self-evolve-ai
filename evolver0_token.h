/**
 * evolver0_token.h - 词法标记定义
 */

#ifndef EVOLVER0_TOKEN_H
#define EVOLVER0_TOKEN_H

// 标记类型
typedef enum {
    // 结束标记
    TOKEN_EOF = 0,
    
    // 字面量
    TOKEN_IDENTIFIER,     // 标识符
    TOKEN_NUMBER,         // 数字（整数或浮点数）
    TOKEN_CHAR_LITERAL,   // 字符字面量
    TOKEN_STRING_LITERAL, // 字符串字面量
    
    // 运算符
    TOKEN_PLUS,          // +
    TOKEN_MINUS,         // -
    TOKEN_STAR,          // *
    TOKEN_SLASH,         // /
    TOKEN_PERCENT,       // %
    TOKEN_AMPERSAND,     // &
    TOKEN_PIPE,          // |
    TOKEN_CARET,         // ^
    TOKEN_TILDE,         // ~
    TOKEN_BANG,          // !
    TOKEN_ASSIGN,        // =
    
    // 复合运算符
    TOKEN_EQ,            // ==
    TOKEN_NE,            // !=
    TOKEN_LT,            // <
    TOKEN_LE,            // <=
    TOKEN_GT,            // >
    TOKEN_GE,            // >=
    TOKEN_LOGICAL_AND,   // &&
    TOKEN_LOGICAL_OR,    // ||
    TOKEN_INC,           // ++
    TOKEN_DEC,           // --
    TOKEN_ADD_ASSIGN,    // +=
    TOKEN_SUB_ASSIGN,    // -=
    TOKEN_MUL_ASSIGN,    // *=
    TOKEN_DIV_ASSIGN,    // /=
    TOKEN_MOD_ASSIGN,    // %=
    TOKEN_AND_ASSIGN,    // &=
    TOKEN_OR_ASSIGN,     // |=
    TOKEN_XOR_ASSIGN,    // ^=
    TOKEN_SHL,           // <<
    TOKEN_SHR,           // >>
    TOKEN_SHL_ASSIGN,    // <<=
    TOKEN_SHR_ASSIGN,    // >>=
    TOKEN_ARROW,         // ->
    
    // 标点符号
    TOKEN_LPAREN,        // (
    TOKEN_RPAREN,        // )
    TOKEN_LBRACE,        // {
    TOKEN_RBRACE,        // }
    TOKEN_LBRACKET,      // [
    TOKEN_RBRACKET,      // ]
    TOKEN_SEMICOLON,     // ;
    TOKEN_COMMA,         // ,
    TOKEN_DOT,           // .
    TOKEN_QUESTION,      // ?
    TOKEN_COLON,         // :
    TOKEN_HASH,          // #
    
    // 关键字
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_DO,
    TOKEN_SWITCH,
    TOKEN_CASE,
    TOKEN_DEFAULT,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_RETURN,
    TOKEN_GOTO,
    TOKEN_TYPEDEF,
    TOKEN_EXTERN,
    TOKEN_STATIC,
    TOKEN_AUTO,
    TOKEN_REGISTER,
    TOKEN_CONST,
    TOKEN_VOLATILE,
    TOKEN_INLINE,
    TOKEN_RESTRICT,
    TOKEN_VOID,
    TOKEN_CHAR,
    TOKEN_SHORT,
    TOKEN_INT,
    TOKEN_LONG,
    TOKEN_FLOAT,
    TOKEN_DOUBLE,
    TOKEN_SIGNED,
    TOKEN_UNSIGNED,
    TOKEN_STRUCT,
    TOKEN_UNION,
    TOKEN_ENUM,
    TOKEN_SIZEOF,
    
    // 特殊标记
    TOKEN_ERROR,
    TOKEN_UNKNOWN
} TokenType;

// 标记结构
typedef struct {
    TokenType type;     // 标记类型
    char *value;        // 标记值（字符串表示）
    int line;           // 行号
    int column;         // 列号
    const char *filename; // 源文件名
} Token;

#endif // EVOLVER0_TOKEN_H 