/**
 * evolver0_token.h - Token定义
 */

#ifndef EVOLVER0_TOKEN_H
#define EVOLVER0_TOKEN_H

// Token类型
typedef enum {
    TOK_EOF = 0,
    TOK_IDENTIFIER,
    TOK_NUMBER,
    TOK_STRING,
    TOK_CHAR_LITERAL,
    
    // 关键字
    TOK_INT, TOK_CHAR, TOK_VOID, TOK_RETURN, 
    TOK_IF, TOK_ELSE, TOK_WHILE, TOK_FOR, TOK_DO, 
    TOK_BREAK, TOK_CONTINUE, TOK_STRUCT, TOK_TYPEDEF,
    TOK_STATIC, TOK_EXTERN, TOK_CONST, TOK_SIZEOF,
    
    // 操作符
    TOK_PLUS, TOK_MINUS, TOK_MULTIPLY, TOK_DIVIDE, TOK_MOD,
    TOK_ASSIGN, TOK_EQUAL, TOK_NOT_EQUAL,
    TOK_LESS, TOK_GREATER, TOK_LESS_EQUAL, TOK_GREATER_EQUAL,
    TOK_LOGICAL_AND, TOK_LOGICAL_OR, TOK_LOGICAL_NOT,
    TOK_BIT_AND, TOK_BIT_OR, TOK_BIT_XOR, TOK_BIT_NOT,
    TOK_LEFT_SHIFT, TOK_RIGHT_SHIFT,
    TOK_INCREMENT, TOK_DECREMENT,
    TOK_ARROW, TOK_DOT,
    TOK_PLUS_ASSIGN, TOK_MINUS_ASSIGN, TOK_MUL_ASSIGN, TOK_DIV_ASSIGN,
    
    // 标点
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE,
    TOK_LBRACKET, TOK_RBRACKET, TOK_SEMICOLON, TOK_COMMA,
    TOK_COLON, TOK_QUESTION,
    
    // 预处理
    TOK_HASH, TOK_INCLUDE, TOK_DEFINE, TOK_IFDEF, TOK_IFNDEF, TOK_ENDIF,
    
    TOK_UNKNOWN
} TokenType;

// Token结构
typedef struct {
    TokenType type;
    char *value;
    int line;
    int column;
    const char *filename;
} Token;

// Token操作函数
const char* token_type_to_string(TokenType type);
void token_free(Token *tokens, int count);

#endif // EVOLVER0_TOKEN_H