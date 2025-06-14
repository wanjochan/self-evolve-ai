/**
 * bootstrap.h - 极简IR指令集和数据结构定义
 * 创世自举器 - 自主进化AI系统第一步
 */

#ifndef BOOTSTRAP_H
#define BOOTSTRAP_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 基本类型定义 */
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

/* 令牌类型 */
typedef enum {
    TOK_EOF = 0,
    TOK_IDENT,     /* 标识符 */
    TOK_NUMBER,    /* 数字 */
    TOK_STRING,    /* 字符串 */
    TOK_PLUS,      /* + */
    TOK_MINUS,     /* - */
    TOK_STAR,      /* * */
    TOK_SLASH,     /* / */
    TOK_LPAREN,    /* ( */
    TOK_RPAREN,    /* ) */
    TOK_LBRACE,    /* { */
    TOK_RBRACE,    /* } */
    TOK_SEMICOLON, /* ; */
    TOK_COMMA,     /* , */
    TOK_ASSIGN,    /* = */
    TOK_COLON,     /* : */
    TOK_IF,        /* if */
    TOK_ELSE,      /* else */
    TOK_WHILE,     /* while */
    TOK_RETURN,    /* return */
    TOK_FUNCTION,  /* function */
    TOK_VAR        /* var */
} TokenType;

/* 令牌结构 */
typedef struct {
    TokenType type;
    char *value;
    int line;
    int column;
} Token;

/* IR操作码 */
typedef enum {
    OP_NOP = 0,    /* 空操作 */
    OP_PUSH,       /* 压栈 */
    OP_POP,        /* 出栈 */
    OP_LOAD,       /* 加载 */
    OP_STORE,      /* 存储 */
    OP_ADD,        /* 加法 */
    OP_SUB,        /* 减法 */
    OP_MUL,        /* 乘法 */
    OP_DIV,        /* 除法 */
    OP_JMP,        /* 无条件跳转 */
    OP_JZ,         /* 为零跳转 */
    OP_JNZ,        /* 非零跳转 */
    OP_CALL,       /* 调用函数 */
    OP_RET,        /* 返回 */
    OP_MOV         /* 移动 */
} OpCode;

/* IR指令 */
typedef struct {
    OpCode op;
    u64 arg1;
    u64 arg2;
    u64 arg3;
} IRInst;

/* IR程序 */
typedef struct {
    IRInst *insts;
    u32 count;
    u32 capacity;
} IRProgram;

/* 符号表项 */
typedef struct {
    char *name;
    u32 addr;
    u32 size;
} Symbol;

/* 符号表 */
typedef struct {
    Symbol *symbols;
    u32 count;
    u32 capacity;
} SymbolTable;

/* 词法分析器 */
typedef struct {
    char *source;
    u32 pos;
    u32 line;
    u32 column;
    Token current;
} Lexer;

/* 语法分析器 */
typedef struct {
    Lexer *lexer;
    Token current;
    Token previous;
} Parser;

/* 代码生成器 */
typedef struct {
    IRProgram *program;
    SymbolTable *symbols;
    u8 *code;
    u32 code_size;
    u32 code_capacity;
} CodeGen;

/* 函数声明 */
/* 词法分析 */
Lexer* lexer_create(char *source);
void lexer_destroy(Lexer *lexer);
Token lexer_next_token(Lexer *lexer);

/* 语法分析 */
Parser* parser_create(Lexer *lexer);
void parser_destroy(Parser *parser);
IRProgram* parser_parse(Parser *parser);

/* IR生成 */
IRProgram* ir_program_create();
void ir_program_destroy(IRProgram *program);
void ir_program_add(IRProgram *program, OpCode op, u64 arg1, u64 arg2, u64 arg3);

/* 代码生成 */
CodeGen* codegen_create(IRProgram *program);
void codegen_destroy(CodeGen *codegen);
void codegen_generate_x86_64(CodeGen *codegen);
int codegen_write_elf(CodeGen *codegen, const char *filename);

#endif /* BOOTSTRAP_H */ 