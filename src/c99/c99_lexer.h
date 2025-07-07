/**
 * c99_lexer.h - 完整的C99词法分析器
 * 
 * 实现完整的C99标准词法分析，替换pipeline_module.c中的简化版本
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
// C99 Token类型定义 (完整版)
// ===============================================

typedef enum {
    // 特殊token
    TOKEN_EOF = 0,
    TOKEN_ERROR,
    TOKEN_UNKNOWN,
    
    // 标识符和字面量
    TOKEN_IDENTIFIER,           // 标识符
    TOKEN_INTEGER_CONSTANT,     // 整数常量
    TOKEN_FLOATING_CONSTANT,    // 浮点常量
    TOKEN_CHARACTER_CONSTANT,   // 字符常量
    TOKEN_STRING_LITERAL,       // 字符串字面量
    
    // C99关键字 (32个)
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
    
    // 运算符
    TOKEN_PLUS,         // +
    TOKEN_MINUS,        // -
    TOKEN_STAR,         // *
    TOKEN_SLASH,        // /
    TOKEN_PERCENT,      // %
    TOKEN_PLUS_PLUS,    // ++
    TOKEN_MINUS_MINUS,  // --
    TOKEN_AMPERSAND,    // &
    TOKEN_PIPE,         // |
    TOKEN_CARET,        // ^
    TOKEN_TILDE,        // ~
    TOKEN_EXCLAMATION,  // !
    TOKEN_LESS,         // <
    TOKEN_GREATER,      // >
    TOKEN_LESS_EQUAL,   // <=
    TOKEN_GREATER_EQUAL,// >=
    TOKEN_EQUAL_EQUAL,  // ==
    TOKEN_NOT_EQUAL,    // !=
    TOKEN_LOGICAL_AND,  // &&
    TOKEN_LOGICAL_OR,   // ||
    TOKEN_QUESTION,     // ?
    TOKEN_COLON,        // :
    TOKEN_SEMICOLON,    // ;
    TOKEN_COMMA,        // ,
    TOKEN_EQUAL,        // =
    TOKEN_PLUS_EQUAL,   // +=
    TOKEN_MINUS_EQUAL,  // -=
    TOKEN_STAR_EQUAL,   // *=
    TOKEN_SLASH_EQUAL,  // /=
    TOKEN_PERCENT_EQUAL,// %=
    TOKEN_AMPERSAND_EQUAL, // &=
    TOKEN_PIPE_EQUAL,   // |=
    TOKEN_CARET_EQUAL,  // ^=
    TOKEN_LEFT_SHIFT,   // <<
    TOKEN_RIGHT_SHIFT,  // >>
    TOKEN_LEFT_SHIFT_EQUAL,  // <<=
    TOKEN_RIGHT_SHIFT_EQUAL, // >>=
    
    // 分隔符
    TOKEN_LPAREN,       // (
    TOKEN_RPAREN,       // )
    TOKEN_LBRACE,       // {
    TOKEN_RBRACE,       // }
    TOKEN_LBRACKET,     // [
    TOKEN_RBRACKET,     // ]
    TOKEN_DOT,          // .
    TOKEN_ARROW,        // ->
    TOKEN_ELLIPSIS,     // ... (C99)
    
    // 预处理器token
    TOKEN_HASH,         // #
    TOKEN_HASH_HASH,    // ##
    
    // 扩展token (用于错误恢复)
    TOKEN_NEWLINE,      // \n
    TOKEN_WHITESPACE,   // 空白字符
    TOKEN_COMMENT,      // 注释
    
    TOKEN_COUNT         // token类型总数
} TokenType;

// ===============================================
// Token结构定义
// ===============================================

typedef struct {
    TokenType type;         // token类型
    char* value;           // token值 (动态分配)
    size_t length;         // token长度
    
    // 位置信息
    int line;              // 行号 (1-based)
    int column;            // 列号 (1-based)
    size_t offset;         // 在源码中的偏移量
    
    // 扩展信息
    union {
        struct {
            long long int_value;    // 整数值
            int base;               // 进制 (8, 10, 16)
            bool is_unsigned;       // 是否无符号
            bool is_long;           // 是否long
            bool is_long_long;      // 是否long long
        } integer;
        
        struct {
            double float_value;     // 浮点值
            bool is_float;          // 是否float
            bool is_long_double;    // 是否long double
        } floating;
        
        struct {
            int char_value;         // 字符值
            bool is_wide;           // 是否宽字符
        } character;
        
        struct {
            bool is_wide;           // 是否宽字符串
            size_t raw_length;      // 原始长度(包含转义)
        } string;
    } data;
} Token;

// ===============================================
// 词法分析器结构定义
// ===============================================

typedef struct {
    // 源码信息
    const char* source;     // 源码指针
    size_t source_length;   // 源码长度
    size_t current;         // 当前位置
    
    // 位置跟踪
    int line;               // 当前行号
    int column;             // 当前列号
    int line_start;         // 当前行开始位置
    
    // 状态信息
    bool at_line_start;     // 是否在行首
    bool in_directive;      // 是否在预处理指令中
    
    // 错误处理
    char error_message[256]; // 错误信息
    bool has_error;         // 是否有错误
    
    // 配置选项
    bool skip_whitespace;   // 是否跳过空白字符
    bool skip_comments;     // 是否跳过注释
    bool track_newlines;    // 是否跟踪换行符
} C99Lexer;

// ===============================================
// 词法分析器API
// ===============================================

/**
 * 初始化词法分析器
 */
void c99_lexer_init(C99Lexer* lexer, const char* source, size_t length);

/**
 * 获取下一个token
 */
Token* c99_lexer_next_token(C99Lexer* lexer);

/**
 * 预览下一个token (不移动位置)
 */
Token* c99_lexer_peek_token(C99Lexer* lexer);

/**
 * 跳过空白字符和注释
 */
void c99_lexer_skip_whitespace(C99Lexer* lexer);

/**
 * 获取当前位置信息
 */
void c99_lexer_get_position(C99Lexer* lexer, int* line, int* column);

/**
 * 检查是否有错误
 */
bool c99_lexer_has_error(C99Lexer* lexer);

/**
 * 获取错误信息
 */
const char* c99_lexer_get_error(C99Lexer* lexer);

/**
 * 释放token
 */
void c99_token_free(Token* token);

/**
 * 获取token类型名称
 */
const char* c99_token_type_name(TokenType type);

/**
 * 检查是否为关键字
 */
bool c99_is_keyword(const char* identifier);

/**
 * 获取关键字token类型
 */
TokenType c99_get_keyword_type(const char* identifier);

#ifdef __cplusplus
}
#endif

#endif // C99_LEXER_H
