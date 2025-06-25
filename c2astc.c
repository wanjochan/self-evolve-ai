/**
 * c2astc.c - C语言到ASTC（Abstract Syntax Tree for Compilation）的转换库
 * 
 * 该库实现了C语言源代码到ASTC表示的转换。
 * ASTC是一种抽象语法树表示，用于编译和执行C语言程序。
 * 
 * 主要功能：
 * 1. 词法分析：将C语言源代码转换为Token序列
 * 2. 语法分析：将Token序列解析为抽象语法树
 * 3. ASTC生成：将抽象语法树转换为ASTC表示
 * 4. 序列化：将ASTC表示序列化为二进制格式
 * 5. 反序列化：从二进制格式反序列化为ASTC表示
 * 
 * 支持的C语言特性：
 * - 基本类型（int、float、char等）
 * - 复杂类型（结构体、联合体、枚举）
 * - 指针类型
 * - 数组类型
 * - 函数指针类型
 * - 数组访问表达式
 * - 结构体和联合体成员访问表达式
 * - 控制流语句（if、while、for、return、break、continue）
 * - 函数声明和调用
 * 
 * 版本：0.9.5
 * 日期：2023-11-06
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>

// 包含AST定义
#include "astc.h"
#include "c2astc.h"
#include "evolver0_token.h"

// ===============================================
// 错误处理
// ===============================================

static char last_error[256] = {0};

static void set_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(last_error, sizeof(last_error), format, args);
    va_end(args);
}

const char* c2astc_get_error(void) {
    return last_error[0] ? last_error : NULL;
}

// ===============================================
// 内存管理
// ===============================================

void c2astc_free(void *ptr) {
    free(ptr);
}

// ===============================================
// 词法分析器
// ===============================================

// Token结构定义
// 使用evolver0_token.h中的Token结构

// 创建Token
static Token* create_token(TokenType type, const char *value, int line, int column, const char *filename) {
    Token *token = (Token*)malloc(sizeof(Token));
    if (token) {
        token->type = type;
        token->value = value ? strdup(value) : NULL;
        token->line = line;
        token->column = column;
        token->filename = filename;
    }
    return token;
}

// 释放Token
static void free_token(Token *token) {
    if (token) {
        if (token->value) free(token->value);
        free(token);
    }
}

// 词法分析器上下文
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

// 初始化词法分析器
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

// 跳过空白和注释
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

// 关键字查找表
static struct {
    const char *keyword;
    TokenType type;
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
    {"int", TOKEN_INT},
    {"long", TOKEN_LONG},
    {"register", TOKEN_REGISTER},
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
    {NULL, TOKEN_EOF}
};

// 检查标识符是否为关键字
static TokenType check_keyword(const char *identifier) {
    for (int i = 0; keywords[i].keyword != NULL; i++) {
        if (strcmp(identifier, keywords[i].keyword) == 0) {
            return keywords[i].type;
        }
    }
    return TOKEN_IDENTIFIER;
}

// 扫描标识符
static Token* scan_identifier(Lexer *lexer) {
    size_t start = lexer->pos;
    int start_line = lexer->line;
    int start_column = lexer->column;
    
    while (!lexer_is_at_end(lexer) && lexer_is_alnum(lexer_peek(lexer))) {
        lexer_advance(lexer);
    }
    
    size_t length = lexer->pos - start;
    char *value = (char*)malloc(length + 1);
    if (!value) return NULL;
    
    strncpy(value, lexer->source + start, length);
    value[length] = '\0';
    
    TokenType type = check_keyword(value);
    Token *token = create_token(type, value, start_line, start_column, lexer->filename);
    free(value); // create_token已经复制了value
    return token;
}

// 扫描数字
static Token* scan_number(Lexer *lexer) {
    size_t start = lexer->pos;
    int start_line = lexer->line;
    int start_column = lexer->column;
    int is_float = 0;
    
    // 整数部分
    while (!lexer_is_at_end(lexer) && lexer_is_digit(lexer_peek(lexer))) {
        lexer_advance(lexer);
    }
    
    // 小数部分
    if (lexer_peek(lexer) == '.' && lexer_is_digit(lexer_peek_next(lexer))) {
        is_float = 1;
        lexer_advance(lexer); // .
        
        while (!lexer_is_at_end(lexer) && lexer_is_digit(lexer_peek(lexer))) {
            lexer_advance(lexer);
        }
    }
    
    // 指数部分
    if ((lexer_peek(lexer) == 'e' || lexer_peek(lexer) == 'E')) {
        is_float = 1;
        lexer_advance(lexer); // e/E
        
        if (lexer_peek(lexer) == '+' || lexer_peek(lexer) == '-') {
            lexer_advance(lexer); // +/-
        }
        
        if (!lexer_is_digit(lexer_peek(lexer))) {
            // 错误：指数部分缺少数字
            return NULL;
        }
        
        while (!lexer_is_at_end(lexer) && lexer_is_digit(lexer_peek(lexer))) {
            lexer_advance(lexer);
        }
    }
    
    // 后缀
    if (lexer_peek(lexer) == 'f' || lexer_peek(lexer) == 'F' ||
        lexer_peek(lexer) == 'l' || lexer_peek(lexer) == 'L') {
        lexer_advance(lexer);
    }
    
    size_t length = lexer->pos - start;
    char *value = (char*)malloc(length + 1);
    if (!value) return NULL;
    
    strncpy(value, lexer->source + start, length);
    value[length] = '\0';
    
    Token *token = create_token(TOKEN_NUMBER, value, start_line, start_column, lexer->filename);
    free(value); // create_token已经复制了value
    return token;
}

// 扫描字符串字面量
static Token* scan_string(Lexer *lexer) {
    size_t start = lexer->pos;
    int start_line = lexer->line;
    int start_column = lexer->column;
    
    lexer_advance(lexer); // "
    
    while (!lexer_is_at_end(lexer) && lexer_peek(lexer) != '"') {
        if (lexer_peek(lexer) == '\\') {
            lexer_advance(lexer); // 转义字符
            if (lexer_is_at_end(lexer)) break;
        }
        lexer_advance(lexer);
    }
    
    if (lexer_is_at_end(lexer)) {
        // 错误：未闭合的字符串
        return NULL;
    }
    
    lexer_advance(lexer); // "
    
    size_t length = lexer->pos - start;
    char *value = (char*)malloc(length + 1);
    if (!value) return NULL;
    
    strncpy(value, lexer->source + start, length);
    value[length] = '\0';
    
    Token *token = create_token(TOKEN_STRING_LITERAL, value, start_line, start_column, lexer->filename);
    free(value); // create_token已经复制了value
    return token;
}

// 扫描字符字面量
static Token* scan_char(Lexer *lexer) {
    size_t start = lexer->pos;
    int start_line = lexer->line;
    int start_column = lexer->column;
    
    lexer_advance(lexer); // '
    
    while (!lexer_is_at_end(lexer) && lexer_peek(lexer) != '\'') {
        if (lexer_peek(lexer) == '\\') {
            lexer_advance(lexer); // 转义字符
            if (lexer_is_at_end(lexer)) break;
        }
        lexer_advance(lexer);
    }
    
    if (lexer_is_at_end(lexer)) {
        // 错误：未闭合的字符字面量
        return NULL;
    }
    
    lexer_advance(lexer); // '
    
    size_t length = lexer->pos - start;
    char *value = (char*)malloc(length + 1);
    if (!value) return NULL;
    
    strncpy(value, lexer->source + start, length);
    value[length] = '\0';
    
    Token *token = create_token(TOKEN_CHAR_LITERAL, value, start_line, start_column, lexer->filename);
    free(value); // create_token已经复制了value
    return token;
}

// 扫描下一个Token
static Token* scan_token(Lexer *lexer) {
    skip_whitespace(lexer);
    
    if (lexer_is_at_end(lexer)) {
        return create_token(TOKEN_EOF, NULL, lexer->line, lexer->column, lexer->filename);
    }
    
    char c = lexer_peek(lexer);
    
    // 标识符或关键字
    if (lexer_is_alpha(c)) {
        return scan_identifier(lexer);
    }
    
    // 数字
    if (lexer_is_digit(c)) {
        return scan_number(lexer);
    }
    
    // 单字符Token
    int start_line = lexer->line;
    int start_column = lexer->column;
    lexer_advance(lexer);
    
    switch (c) {
        case '(': return create_token(TOKEN_LPAREN, "(", start_line, start_column, lexer->filename);
        case ')': return create_token(TOKEN_RPAREN, ")", start_line, start_column, lexer->filename);
        case '{': return create_token(TOKEN_LBRACE, "{", start_line, start_column, lexer->filename);
        case '}': return create_token(TOKEN_RBRACE, "}", start_line, start_column, lexer->filename);
        case '[': return create_token(TOKEN_LBRACKET, "[", start_line, start_column, lexer->filename);
        case ']': return create_token(TOKEN_RBRACKET, "]", start_line, start_column, lexer->filename);
        case ',': return create_token(TOKEN_COMMA, ",", start_line, start_column, lexer->filename);
        case '.': return create_token(TOKEN_DOT, ".", start_line, start_column, lexer->filename);
        case ';': return create_token(TOKEN_SEMICOLON, ";", start_line, start_column, lexer->filename);
        case ':': return create_token(TOKEN_COLON, ":", start_line, start_column, lexer->filename);
        case '~': return create_token(TOKEN_TILDE, "~", start_line, start_column, lexer->filename);
        
        // 可能是双字符Token
        case '!':
            if (lexer_match(lexer, '=')) {
                return create_token(TOKEN_NE, "!=", start_line, start_column, lexer->filename);
            } else {
                return create_token(TOKEN_BANG, "!", start_line, start_column, lexer->filename);
            }
        case '=':
            if (lexer_match(lexer, '=')) {
                return create_token(TOKEN_EQ, "==", start_line, start_column, lexer->filename);
            } else {
                return create_token(TOKEN_ASSIGN, "=", start_line, start_column, lexer->filename);
            }
        case '<':
            if (lexer_match(lexer, '=')) {
                return create_token(TOKEN_LE, "<=", start_line, start_column, lexer->filename);
            } else if (lexer_match(lexer, '<')) {
                return create_token(TOKEN_SHL, "<<", start_line, start_column, lexer->filename);
            } else {
                return create_token(TOKEN_LT, "<", start_line, start_column, lexer->filename);
            }
        case '>':
            if (lexer_match(lexer, '=')) {
                return create_token(TOKEN_GE, ">=", start_line, start_column, lexer->filename);
            } else if (lexer_match(lexer, '>')) {
                return create_token(TOKEN_SHR, ">>", start_line, start_column, lexer->filename);
            } else {
                return create_token(TOKEN_GT, ">", start_line, start_column, lexer->filename);
            }
        case '+':
            if (lexer_match(lexer, '+')) {
                return create_token(TOKEN_INC, "++", start_line, start_column, lexer->filename);
            } else if (lexer_match(lexer, '=')) {
                return create_token(TOKEN_ADD_ASSIGN, "+=", start_line, start_column, lexer->filename);
            } else {
                return create_token(TOKEN_PLUS, "+", start_line, start_column, lexer->filename);
            }
        case '-':
            if (lexer_match(lexer, '-')) {
                return create_token(TOKEN_DEC, "--", start_line, start_column, lexer->filename);
            } else if (lexer_match(lexer, '=')) {
                return create_token(TOKEN_SUB_ASSIGN, "-=", start_line, start_column, lexer->filename);
            } else if (lexer_match(lexer, '>')) {
                return create_token(TOKEN_ARROW, "->", start_line, start_column, lexer->filename);
            } else {
                return create_token(TOKEN_MINUS, "-", start_line, start_column, lexer->filename);
            }
        case '*':
            if (lexer_match(lexer, '=')) {
                return create_token(TOKEN_MUL_ASSIGN, "*=", start_line, start_column, lexer->filename);
            } else {
                return create_token(TOKEN_STAR, "*", start_line, start_column, lexer->filename);
            }
        case '/':
            if (lexer_match(lexer, '=')) {
                return create_token(TOKEN_DIV_ASSIGN, "/=", start_line, start_column, lexer->filename);
            } else {
                return create_token(TOKEN_SLASH, "/", start_line, start_column, lexer->filename);
            }
        case '%':
            if (lexer_match(lexer, '=')) {
                return create_token(TOKEN_MOD_ASSIGN, "%=", start_line, start_column, lexer->filename);
            } else {
                return create_token(TOKEN_PERCENT, "%", start_line, start_column, lexer->filename);
            }
        case '&':
            if (lexer_match(lexer, '&')) {
                return create_token(TOKEN_LOGICAL_AND, "&&", start_line, start_column, lexer->filename);
            } else if (lexer_match(lexer, '=')) {
                return create_token(TOKEN_AND_ASSIGN, "&=", start_line, start_column, lexer->filename);
            } else {
                return create_token(TOKEN_AMPERSAND, "&", start_line, start_column, lexer->filename);
            }
        case '|':
            if (lexer_match(lexer, '|')) {
                return create_token(TOKEN_LOGICAL_OR, "||", start_line, start_column, lexer->filename);
            } else if (lexer_match(lexer, '=')) {
                return create_token(TOKEN_OR_ASSIGN, "|=", start_line, start_column, lexer->filename);
            } else {
                return create_token(TOKEN_PIPE, "|", start_line, start_column, lexer->filename);
            }
        case '^':
            if (lexer_match(lexer, '=')) {
                return create_token(TOKEN_XOR_ASSIGN, "^=", start_line, start_column, lexer->filename);
            } else {
                return create_token(TOKEN_CARET, "^", start_line, start_column, lexer->filename);
            }
            
        // 字符串和字符字面量
        case '"': return scan_string(lexer);
        case '\'': return scan_char(lexer);
            
        // 预处理指令
        case '#':
            lexer->in_preprocessor = 1;
            return create_token(TOKEN_HASH, "#", start_line, start_column, lexer->filename);
    }
    
    // 未知字符
    char unknown[2] = {c, '\0'};
    return create_token(TOKEN_UNKNOWN, unknown, start_line, start_column, lexer->filename);
}

// 词法分析
static Token** tokenize(Lexer *lexer, int *token_count) {
    Token **tokens = NULL;
    int capacity = 0;
    *token_count = 0;
    
    while (1) {
        Token *token = scan_token(lexer);
        if (!token) break;
        
        // 动态扩展tokens数组
        if (*token_count >= capacity) {
            capacity = capacity == 0 ? 8 : capacity * 2;
            Token **new_tokens = (Token**)realloc(tokens, capacity * sizeof(Token*));
            if (!new_tokens) {
                // 释放已分配的资源
                for (int i = 0; i < *token_count; i++) {
                    free_token(tokens[i]);
                }
                free(tokens);
                free_token(token);
                return NULL;
            }
            tokens = new_tokens;
        }
        
        tokens[(*token_count)++] = token;
        
        if (token->type == TOKEN_EOF) break;
    }
    
    return tokens;
}

// 释放tokens数组
static void free_tokens(Token **tokens, int token_count) {
    if (!tokens) return;
    
    for (int i = 0; i < token_count; i++) {
        free_token(tokens[i]);
    }
    
    free(tokens);
}

// ===============================================
// ASTC节点创建和管理
// ===============================================

// 创建AST节点
struct ASTNode* ast_create_node(ASTNodeType type, int line, int column) {
    // 现在ASTNode已经完整定义，可以使用sizeof
    struct ASTNode *node = (struct ASTNode*)malloc(sizeof(struct ASTNode));
    if (node) {
        memset(node, 0, sizeof(struct ASTNode));
        node->type = type;
        node->line = line;
        node->column = column;
    }
    return node;
}

// 释放AST节点及其子节点
void ast_free(struct ASTNode *node) {
    if (!node) return;
    
    // 根据节点类型释放资源
    switch (node->type) {
        case ASTC_EXPR_IDENTIFIER:
            if (node->data.identifier.name) free(node->data.identifier.name);
            break;
        case ASTC_EXPR_STRING_LITERAL:
            if (node->data.string_literal.value) free(node->data.string_literal.value);
            break;
        case ASTC_BINARY_OP:
            ast_free(node->data.binary_op.left);
            ast_free(node->data.binary_op.right);
            break;
        case ASTC_UNARY_OP:
            ast_free(node->data.unary_op.operand);
            break;
        case ASTC_CALL_EXPR:
            ast_free(node->data.call_expr.callee);
            for (int i = 0; i < node->data.call_expr.arg_count; i++) {
                ast_free(node->data.call_expr.args[i]);
            }
            free(node->data.call_expr.args);
            break;
        case ASTC_TRANSLATION_UNIT:
            for (int i = 0; i < node->data.translation_unit.declaration_count; i++) {
                ast_free(node->data.translation_unit.declarations[i]);
            }
            free(node->data.translation_unit.declarations);
            break;
        case ASTC_FUNC_DECL:
            if (node->data.func_decl.name) free(node->data.func_decl.name);
            ast_free(node->data.func_decl.return_type);
            for (int i = 0; i < node->data.func_decl.param_count; i++) {
                ast_free(node->data.func_decl.params[i]);
            }
            free(node->data.func_decl.params);
            if (node->data.func_decl.has_body) {
                ast_free(node->data.func_decl.body);
            }
            break;
        case ASTC_VAR_DECL:
            if (node->data.var_decl.name) free(node->data.var_decl.name);
            ast_free(node->data.var_decl.type);
            if (node->data.var_decl.initializer) {
                ast_free(node->data.var_decl.initializer);
            }
            break;
        case ASTC_STRUCT_DECL:
            if (node->data.struct_decl.name) free(node->data.struct_decl.name);
            for (int i = 0; i < node->data.struct_decl.member_count; i++) {
                ast_free(node->data.struct_decl.members[i]);
            }
            free(node->data.struct_decl.members);
            break;
        case ASTC_UNION_DECL:
            if (node->data.union_decl.name) free(node->data.union_decl.name);
            for (int i = 0; i < node->data.union_decl.member_count; i++) {
                ast_free(node->data.union_decl.members[i]);
            }
            free(node->data.union_decl.members);
            break;
        case ASTC_ENUM_DECL:
            if (node->data.enum_decl.name) free(node->data.enum_decl.name);
            for (int i = 0; i < node->data.enum_decl.constant_count; i++) {
                ast_free(node->data.enum_decl.constants[i]);
            }
            free(node->data.enum_decl.constants);
            break;
        case ASTC_ENUM_CONSTANT:
            // 释放枚举常量
            if (node->data.enum_constant.name) free(node->data.enum_constant.name);
            ast_free(node->data.enum_constant.value);
            break;
        case ASTC_COMPOUND_STMT:
            // 释放复合语句
            for (int i = 0; i < node->data.compound_stmt.statement_count; i++) {
                ast_free(node->data.compound_stmt.statements[i]);
            }
            free(node->data.compound_stmt.statements);
            break;
        case ASTC_IF_STMT:
            // 释放if语句
            ast_free(node->data.if_stmt.condition);
            ast_free(node->data.if_stmt.then_branch);
            ast_free(node->data.if_stmt.else_branch);
            break;
        case ASTC_WHILE_STMT:
            // 释放while语句
            ast_free(node->data.while_stmt.condition);
            ast_free(node->data.while_stmt.body);
            break;
        case ASTC_FOR_STMT:
            // 释放for语句
            ast_free(node->data.for_stmt.init);
            ast_free(node->data.for_stmt.condition);
            ast_free(node->data.for_stmt.increment);
            ast_free(node->data.for_stmt.body);
            break;
        case ASTC_RETURN_STMT:
            // 释放return语句
            ast_free(node->data.return_stmt.value);
            break;
        case ASTC_EXPR_STMT:
            // 释放表达式语句
            ast_free(node->data.expr_stmt.expr);
            break;
        case ASTC_FUNCTION_TYPE:
            // 释放函数指针类型
            ast_free(node->data.function_type.return_type);
            if (node->data.function_type.param_types) {
                for (int i = 0; i < node->data.function_type.param_count; i++) {
                    ast_free(node->data.function_type.param_types[i]);
                }
                free(node->data.function_type.param_types);
            }
            break;
        case ASTC_EXPR_ARRAY_SUBSCRIPT:
            // 释放数组访问表达式
            ast_free(node->data.array_subscript.array);
            ast_free(node->data.array_subscript.index);
            break;
        case ASTC_EXPR_MEMBER_ACCESS:
            // 释放成员访问表达式
            ast_free(node->data.member_access.object);
            if (node->data.member_access.member) free(node->data.member_access.member);
            break;
        case ASTC_EXPR_PTR_MEMBER_ACCESS:
            // 释放指针成员访问表达式
            ast_free(node->data.ptr_member_access.pointer);
            if (node->data.ptr_member_access.member) free(node->data.ptr_member_access.member);
            break;
        default:
            // 其他节点类型的释放逻辑
            break;
    }
    
    free(node);
}

// ===============================================
// 直接从Token创建ASTC节点
// ===============================================

// 从标识符Token创建ASTC标识符节点
struct ASTNode* create_identifier_node(const char *name, int line, int column) {
    struct ASTNode *node = ast_create_node(ASTC_EXPR_IDENTIFIER, line, column);
    if (node) {
        node->data.identifier.name = strdup(name);
    }
    return node;
}

// 从数字Token创建ASTC常量节点
static struct ASTNode* create_number_node(const char *value, int line, int column) {
    struct ASTNode *node = ast_create_node(ASTC_EXPR_CONSTANT, line, column);
    if (node) {
        // 检查是否为浮点数
        if (strchr(value, '.') || strchr(value, 'e') || strchr(value, 'E')) {
            node->data.constant.type = ASTC_TYPE_FLOAT;
            node->data.constant.float_val = atof(value);
        } else {
            node->data.constant.type = ASTC_TYPE_INT;
            node->data.constant.int_val = atoll(value);
        }
    }
    return node;
}

// 从字符串Token创建ASTC字符串字面量节点
struct ASTNode* create_string_node(const char *value, int line, int column) {
    struct ASTNode *node = ast_create_node(ASTC_EXPR_STRING_LITERAL, line, column);
    if (node) {
        // 去除引号
        size_t len = strlen(value);
        if (len >= 2 && value[0] == '"' && value[len-1] == '"') {
            char *str = (char*)malloc(len - 1);
            if (str) {
                strncpy(str, value + 1, len - 2);
                str[len - 2] = '\0';
                node->data.string_literal.value = str;
            }
        } else {
            node->data.string_literal.value = strdup(value);
        }
    }
    return node;
}

// 创建二元操作表达式节点
static struct ASTNode* create_binary_expr(ASTNodeType op, struct ASTNode *left, struct ASTNode *right, int line, int column) {
    struct ASTNode *node = ast_create_node(ASTC_BINARY_OP, line, column);
    if (node) {
        node->data.binary_op.op = op;
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
    }
    return node;
}

// 创建一元操作表达式节点
static struct ASTNode* create_unary_expr(ASTNodeType op, struct ASTNode *operand, int line, int column) {
    struct ASTNode *node = ast_create_node(ASTC_UNARY_OP, line, column);
    if (node) {
        node->data.unary_op.op = op;
        node->data.unary_op.operand = operand;
    }
    return node;
}

// 创建函数调用表达式节点
static struct ASTNode* create_call_expr(struct ASTNode *callee, struct ASTNode **args, int arg_count, int line, int column) {
    struct ASTNode *node = ast_create_node(ASTC_CALL_EXPR, line, column);
    if (node) {
        node->data.call_expr.callee = callee;
        node->data.call_expr.args = args;
        node->data.call_expr.arg_count = arg_count;
    }
    return node;
}

// ===============================================
// 解析器
// ===============================================

// 解析器上下文
typedef struct {
    Token **tokens;
    int token_count;
    int current;
    
    // 错误处理
    char error_msg[256];
    int error_count;
    
    // 符号表
    struct {
        char *names[1024];
        struct ASTNode *nodes[1024];
        int count;
    } symbols;
} Parser;

// 初始化解析器
static void init_parser(Parser *parser, Token **tokens, int token_count) {
    parser->tokens = tokens;
    parser->token_count = token_count;
    parser->current = 0;
    parser->error_msg[0] = '\0';
    parser->error_count = 0;
    parser->symbols.count = 0;
}

// 检查当前Token类型
static int check(Parser *parser, TokenType type) {
    if (parser->current >= parser->token_count) return 0;
    return parser->tokens[parser->current]->type == type;
}

// 前进到下一个Token
static Token* advance(Parser *parser) {
    if (parser->current < parser->token_count) {
        return parser->tokens[parser->current++];
    }
    return NULL;
}

// 获取当前Token
static Token* peek(Parser *parser) {
    if (parser->current >= parser->token_count) return NULL;
    return parser->tokens[parser->current];
}

// 获取前看n个Token
static Token* peek_n(Parser *parser, int n) {
    if (parser->current + n >= parser->token_count) return NULL;
    return parser->tokens[parser->current + n];
}

// 匹配并消耗一个Token
static int match(Parser *parser, TokenType type) {
    if (check(parser, type)) {
        advance(parser);
        return 1;
    }
    return 0;
}

// 报告解析错误
static void parser_error(Parser *parser, const char *message) {
    Token *token = peek(parser);
    if (token) {
        snprintf(parser->error_msg, sizeof(parser->error_msg), 
                 "%s:%d:%d: %s", token->filename, token->line, token->column, message);
    } else {
        snprintf(parser->error_msg, sizeof(parser->error_msg), "%s", message);
    }
    parser->error_count++;
}

// 前向声明解析函数
static struct ASTNode* parse_expression(Parser *parser);
static struct ASTNode* parse_statement(Parser *parser);
static struct ASTNode* parse_declaration(Parser *parser);
static struct ASTNode* parse_primary(Parser *parser);
static struct ASTNode* parse_unary(Parser *parser);
static struct ASTNode* parse_multiplicative(Parser *parser);
static struct ASTNode* parse_additive(Parser *parser);
static struct ASTNode* parse_relational(Parser *parser);
static struct ASTNode* parse_equality(Parser *parser);
static struct ASTNode* parse_logical_and(Parser *parser);
static struct ASTNode* parse_logical_or(Parser *parser);
static struct ASTNode* parse_assignment(Parser *parser);
static struct ASTNode* parse_compound_statement(Parser *parser);
static struct ASTNode* parse_if_statement(Parser *parser);
static struct ASTNode* parse_while_statement(Parser *parser);
static struct ASTNode* parse_for_statement(Parser *parser);
static struct ASTNode* parse_return_statement(Parser *parser);
static struct ASTNode* parse_expression_statement(Parser *parser);
static struct ASTNode* parse_struct_or_union(Parser *parser);
static struct ASTNode* parse_enum(Parser *parser);
static struct ASTNode* parse_pointer_type(Parser *parser, struct ASTNode *base_type);
static struct ASTNode* parse_array_type(Parser *parser, struct ASTNode *element_type);
static struct ASTNode* parse_function_type(Parser *parser, struct ASTNode *return_type);

// 解析复合语句 { ... }
static struct ASTNode* parse_compound_statement(Parser *parser) {
    if (!match(parser, TOKEN_LBRACE)) {
        parser_error(parser, "预期左花括号");
        return NULL;
    }
    
    struct ASTNode *compound = ast_create_node(ASTC_COMPOUND_STMT, peek(parser)->line, peek(parser)->column);
    if (!compound) return NULL;
    
    // 初始化语句列表
    compound->data.compound_stmt.statements = NULL;
    compound->data.compound_stmt.statement_count = 0;
    int capacity = 0;
    
    // 解析语句列表
    while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
        struct ASTNode *stmt = NULL;
        
        // 检查是声明还是语句
        Token *token = peek(parser);
        if (!token) break;
        
        switch (token->type) {
            case TOKEN_VOID:
            case TOKEN_CHAR:
            case TOKEN_SHORT:
            case TOKEN_INT:
            case TOKEN_LONG:
            case TOKEN_FLOAT:
            case TOKEN_DOUBLE:
            case TOKEN_SIGNED:
            case TOKEN_UNSIGNED:
            case TOKEN_STRUCT:
            case TOKEN_UNION:
            case TOKEN_ENUM:
                // 声明
                stmt = parse_declaration(parser);
                break;
                
            default:
                // 语句
                stmt = parse_statement(parser);
                break;
        }
        
        if (!stmt) {
            // 尝试跳过错误，继续解析
            if (!check(parser, TOKEN_EOF)) {
                advance(parser);
                continue;
            } else {
                break;
            }
        }
        
        // 动态扩展语句数组
        if (compound->data.compound_stmt.statement_count >= capacity) {
            capacity = capacity == 0 ? 8 : capacity * 2;
            struct ASTNode **new_statements = (struct ASTNode **)realloc(
                compound->data.compound_stmt.statements, 
                capacity * sizeof(struct ASTNode *)
            );
            
            if (!new_statements) {
                parser_error(parser, "内存分配失败");
                // 释放已分配的资源
                for (int i = 0; i < compound->data.compound_stmt.statement_count; i++) {
                    ast_free(compound->data.compound_stmt.statements[i]);
                }
                free(compound->data.compound_stmt.statements);
                ast_free(compound);
                ast_free(stmt);
                return NULL;
            }
            
            compound->data.compound_stmt.statements = new_statements;
        }
        
        // 添加语句到复合语句
        compound->data.compound_stmt.statements[compound->data.compound_stmt.statement_count++] = stmt;
    }
    
    if (!match(parser, TOKEN_RBRACE)) {
        parser_error(parser, "预期右花括号");
        // 释放已分配的资源
        for (int i = 0; i < compound->data.compound_stmt.statement_count; i++) {
            ast_free(compound->data.compound_stmt.statements[i]);
        }
        free(compound->data.compound_stmt.statements);
        ast_free(compound);
        return NULL;
    }
    
    return compound;
}

// 解析if语句
static struct ASTNode* parse_if_statement(Parser *parser) {
    if (!match(parser, TOKEN_IF)) {
        parser_error(parser, "预期if关键字");
        return NULL;
    }
    
    Token *token = peek(parser);
    struct ASTNode *if_stmt = ast_create_node(ASTC_IF_STMT, token->line, token->column);
    if (!if_stmt) return NULL;
    
    // 解析条件表达式
    if (!match(parser, TOKEN_LPAREN)) {
        parser_error(parser, "预期左括号");
        ast_free(if_stmt);
        return NULL;
    }
    
    struct ASTNode *condition = parse_expression(parser);
    if (!condition) {
        ast_free(if_stmt);
        return NULL;
    }
    
    if (!match(parser, TOKEN_RPAREN)) {
        parser_error(parser, "预期右括号");
        ast_free(condition);
        ast_free(if_stmt);
        return NULL;
    }
    
    // 解析then分支
    struct ASTNode *then_branch = parse_statement(parser);
    if (!then_branch) {
        ast_free(condition);
        ast_free(if_stmt);
        return NULL;
    }
    
    // 解析else分支（如果有）
    struct ASTNode *else_branch = NULL;
    if (match(parser, TOKEN_ELSE)) {
        else_branch = parse_statement(parser);
        if (!else_branch) {
            ast_free(condition);
            ast_free(then_branch);
            ast_free(if_stmt);
            return NULL;
        }
    }
    
    // 设置if语句的字段
    if_stmt->data.if_stmt.condition = condition;
    if_stmt->data.if_stmt.then_branch = then_branch;
    if_stmt->data.if_stmt.else_branch = else_branch;
    
    return if_stmt;
}

// 解析while语句
static struct ASTNode* parse_while_statement(Parser *parser) {
    if (!match(parser, TOKEN_WHILE)) {
        parser_error(parser, "预期while关键字");
        return NULL;
    }
    
    Token *token = peek(parser);
    struct ASTNode *while_stmt = ast_create_node(ASTC_WHILE_STMT, token->line, token->column);
    if (!while_stmt) return NULL;
    
    // 解析条件表达式
    if (!match(parser, TOKEN_LPAREN)) {
        parser_error(parser, "预期左括号");
        ast_free(while_stmt);
        return NULL;
    }
    
    struct ASTNode *condition = parse_expression(parser);
    if (!condition) {
        ast_free(while_stmt);
        return NULL;
    }
    
    if (!match(parser, TOKEN_RPAREN)) {
        parser_error(parser, "预期右括号");
        ast_free(condition);
        ast_free(while_stmt);
        return NULL;
    }
    
    // 解析循环体
    struct ASTNode *body = parse_statement(parser);
    if (!body) {
        ast_free(condition);
        ast_free(while_stmt);
        return NULL;
    }
    
    // 设置while语句的字段
    while_stmt->data.while_stmt.condition = condition;
    while_stmt->data.while_stmt.body = body;
    
    return while_stmt;
}

// 解析for语句
static struct ASTNode* parse_for_statement(Parser *parser) {
    if (!match(parser, TOKEN_FOR)) {
        parser_error(parser, "预期for关键字");
        return NULL;
    }
    
    Token *token = peek(parser);
    struct ASTNode *for_stmt = ast_create_node(ASTC_FOR_STMT, token->line, token->column);
    if (!for_stmt) return NULL;
    
    // 解析for语句的三个部分
    if (!match(parser, TOKEN_LPAREN)) {
        parser_error(parser, "预期左括号");
        ast_free(for_stmt);
        return NULL;
    }
    
    // 初始化部分
    struct ASTNode *init = NULL;
    if (!check(parser, TOKEN_SEMICOLON)) {
        // 检查是声明还是表达式
        Token *token = peek(parser);
        if (!token) {
            ast_free(for_stmt);
            return NULL;
        }
        
        switch (token->type) {
            case TOKEN_VOID:
            case TOKEN_CHAR:
            case TOKEN_SHORT:
            case TOKEN_INT:
            case TOKEN_LONG:
            case TOKEN_FLOAT:
            case TOKEN_DOUBLE:
            case TOKEN_SIGNED:
            case TOKEN_UNSIGNED:
            case TOKEN_STRUCT:
            case TOKEN_UNION:
            case TOKEN_ENUM:
                // 声明
                init = parse_declaration(parser);
                break;
                
            default:
                // 表达式
                init = parse_expression(parser);
                if (init && !match(parser, TOKEN_SEMICOLON)) {
                    parser_error(parser, "预期分号");
                    ast_free(init);
                    ast_free(for_stmt);
                    return NULL;
                }
                break;
        }
        
        if (!init) {
            ast_free(for_stmt);
            return NULL;
        }
    } else {
        // 空初始化
        match(parser, TOKEN_SEMICOLON);
    }
    
    // 条件部分
    struct ASTNode *condition = NULL;
    if (!check(parser, TOKEN_SEMICOLON)) {
        condition = parse_expression(parser);
        if (!condition) {
            ast_free(init);
            ast_free(for_stmt);
            return NULL;
        }
    }
    
    if (!match(parser, TOKEN_SEMICOLON)) {
        parser_error(parser, "预期分号");
        ast_free(init);
        ast_free(condition);
        ast_free(for_stmt);
        return NULL;
    }
    
    // 递增部分
    struct ASTNode *increment = NULL;
    if (!check(parser, TOKEN_RPAREN)) {
        increment = parse_expression(parser);
        if (!increment) {
            ast_free(init);
            ast_free(condition);
            ast_free(for_stmt);
            return NULL;
        }
    }
    
    if (!match(parser, TOKEN_RPAREN)) {
        parser_error(parser, "预期右括号");
        ast_free(init);
        ast_free(condition);
        ast_free(increment);
        ast_free(for_stmt);
        return NULL;
    }
    
    // 解析循环体
    struct ASTNode *body = parse_statement(parser);
    if (!body) {
        ast_free(init);
        ast_free(condition);
        ast_free(increment);
        ast_free(for_stmt);
        return NULL;
    }
    
    // 设置for语句的字段
    for_stmt->data.for_stmt.init = init;
    for_stmt->data.for_stmt.condition = condition;
    for_stmt->data.for_stmt.increment = increment;
    for_stmt->data.for_stmt.body = body;
    
    return for_stmt;
}

// 解析表达式语句
static struct ASTNode* parse_expression_statement(Parser *parser) {
    struct ASTNode *expr = parse_expression(parser);
    if (!expr) return NULL;
    
    if (!match(parser, TOKEN_SEMICOLON)) {
        parser_error(parser, "预期分号");
        ast_free(expr);
        return NULL;
    }
    
    struct ASTNode *expr_stmt = ast_create_node(ASTC_EXPR_STMT, expr->line, expr->column);
    if (!expr_stmt) {
        ast_free(expr);
        return NULL;
    }
    
    expr_stmt->data.expr_stmt.expr = expr;
    
    return expr_stmt;
}

// 解析return语句
static struct ASTNode* parse_return_statement(Parser *parser) {
    if (!match(parser, TOKEN_RETURN)) {
        parser_error(parser, "预期return关键字");
        return NULL;
    }
    
    Token *token = peek(parser);
    struct ASTNode *return_stmt = ast_create_node(ASTC_RETURN_STMT, token->line, token->column);
    if (!return_stmt) return NULL;
    
    // 解析返回值（如果有）
    if (!check(parser, TOKEN_SEMICOLON)) {
        struct ASTNode *value = parse_expression(parser);
        if (!value) {
            ast_free(return_stmt);
            return NULL;
        }
        
        return_stmt->data.return_stmt.value = value;
    } else {
        return_stmt->data.return_stmt.value = NULL;
    }
    
    if (!match(parser, TOKEN_SEMICOLON)) {
        parser_error(parser, "预期分号");
        ast_free(return_stmt->data.return_stmt.value);
        ast_free(return_stmt);
        return NULL;
    }
    
    return return_stmt;
}

// 解析语句
static struct ASTNode* parse_statement(Parser *parser) {
    Token *token = peek(parser);
    if (!token) return NULL;
    
    switch (token->type) {
        case TOKEN_LBRACE:
            return parse_compound_statement(parser);
        case TOKEN_IF:
            return parse_if_statement(parser);
        case TOKEN_WHILE:
            return parse_while_statement(parser);
        case TOKEN_FOR:
            return parse_for_statement(parser);
        case TOKEN_RETURN:
            return parse_return_statement(parser);
        case TOKEN_BREAK:
            // 解析break语句
            advance(parser);
            if (!match(parser, TOKEN_SEMICOLON)) {
                parser_error(parser, "预期分号");
                return NULL;
            }
            return ast_create_node(ASTC_BREAK_STMT, token->line, token->column);
        case TOKEN_CONTINUE:
            // 解析continue语句
            advance(parser);
            if (!match(parser, TOKEN_SEMICOLON)) {
                parser_error(parser, "预期分号");
                return NULL;
            }
            return ast_create_node(ASTC_CONTINUE_STMT, token->line, token->column);
        default:
            // 表达式语句
            return parse_expression_statement(parser);
    }
}

// 解析翻译单元
static struct ASTNode* parse_translation_unit(Parser *parser) {
    struct ASTNode *root = ast_create_node(ASTC_TRANSLATION_UNIT, 0, 0);
    if (!root) return NULL;
    
    // 初始化声明列表
    root->data.translation_unit.declarations = NULL;
    root->data.translation_unit.declaration_count = 0;
    int capacity = 0;
    
    // 解析顶层声明
    while (parser->current < parser->token_count) {
        struct ASTNode *decl = parse_declaration(parser);
        if (!decl) {
            // 尝试跳过错误，继续解析
            Token *token = peek(parser);
            if (token && token->type != TOKEN_EOF) {
                // 跳过当前Token，尝试继续解析
                advance(parser);
                continue;
            } else {
                break;
            }
        }
        
        // 动态扩展声明数组
        if (root->data.translation_unit.declaration_count >= capacity) {
            capacity = capacity == 0 ? 8 : capacity * 2;
            struct ASTNode **new_declarations = (struct ASTNode **)realloc(
                root->data.translation_unit.declarations, 
                capacity * sizeof(struct ASTNode *)
            );
            
            if (!new_declarations) {
                parser_error(parser, "内存分配失败");
                // 释放已分配的资源
                for (int i = 0; i < root->data.translation_unit.declaration_count; i++) {
                    ast_free(root->data.translation_unit.declarations[i]);
                }
                free(root->data.translation_unit.declarations);
                ast_free(root);
                ast_free(decl);
                return NULL;
            }
            
            root->data.translation_unit.declarations = new_declarations;
        }
        
        // 添加声明到翻译单元
        root->data.translation_unit.declarations[root->data.translation_unit.declaration_count++] = decl;
    }
    
    return root;
}

// 解析结构体或联合体声明
static struct ASTNode* parse_struct_or_union(Parser *parser) {
    // 检查是结构体还是联合体
    ASTNodeType type;
    Token *start_token = peek(parser);
    if (!start_token) return NULL;
    
    if (match(parser, TOKEN_STRUCT)) {
        type = ASTC_STRUCT_DECL;
    } else if (match(parser, TOKEN_UNION)) {
        type = ASTC_UNION_DECL;
    } else {
        parser_error(parser, "预期struct或union关键字");
        return NULL;
    }
    
    struct ASTNode *decl = ast_create_node(type, start_token->line, start_token->column);
    if (!decl) return NULL;
    
    // 检查是否有标识符
    if (check(parser, TOKEN_IDENTIFIER)) {
        Token *token = advance(parser);
        if (type == ASTC_STRUCT_DECL) {
            decl->data.struct_decl.name = strdup(token->value);
            if (!decl->data.struct_decl.name) {
                parser_error(parser, "内存分配失败");
                ast_free(decl);
                return NULL;
            }
        } else { // ASTC_UNION_DECL
            decl->data.union_decl.name = strdup(token->value);
            if (!decl->data.union_decl.name) {
                parser_error(parser, "内存分配失败");
                ast_free(decl);
                return NULL;
            }
        }
    } else {
        if (type == ASTC_STRUCT_DECL) {
            decl->data.struct_decl.name = NULL;
        } else { // ASTC_UNION_DECL
            decl->data.union_decl.name = NULL;
        }
    }
    
    // 检查是否有定义体
    if (match(parser, TOKEN_LBRACE)) {
        // 初始化成员列表
        if (type == ASTC_STRUCT_DECL) {
            decl->data.struct_decl.members = NULL;
            decl->data.struct_decl.member_count = 0;
        } else { // ASTC_UNION_DECL
            decl->data.union_decl.members = NULL;
            decl->data.union_decl.member_count = 0;
        }
        int capacity = 0;
        
        // 解析成员列表
        while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
            struct ASTNode *member = parse_declaration(parser);
            if (!member) {
                // 尝试跳过错误，继续解析
                if (!check(parser, TOKEN_EOF)) {
                    advance(parser);
                    continue;
                } else {
                    break;
                }
            }
            
            // 动态扩展成员数组
            if ((type == ASTC_STRUCT_DECL && decl->data.struct_decl.member_count >= capacity) ||
                (type == ASTC_UNION_DECL && decl->data.union_decl.member_count >= capacity)) {
                capacity = capacity == 0 ? 8 : capacity * 2;
                struct ASTNode **new_members = NULL;
                
                if (type == ASTC_STRUCT_DECL) {
                    new_members = (struct ASTNode **)realloc(
                        decl->data.struct_decl.members, 
                        capacity * sizeof(struct ASTNode *)
                    );
                } else { // ASTC_UNION_DECL
                    new_members = (struct ASTNode **)realloc(
                        decl->data.union_decl.members, 
                        capacity * sizeof(struct ASTNode *)
                    );
                }
                
                if (!new_members) {
                    parser_error(parser, "内存分配失败");
                    // 释放已分配的资源
                    if (type == ASTC_STRUCT_DECL) {
                        for (int i = 0; i < decl->data.struct_decl.member_count; i++) {
                            ast_free(decl->data.struct_decl.members[i]);
                        }
                        free(decl->data.struct_decl.members);
                    } else { // ASTC_UNION_DECL
                        for (int i = 0; i < decl->data.union_decl.member_count; i++) {
                            ast_free(decl->data.union_decl.members[i]);
                        }
                        free(decl->data.union_decl.members);
                    }
                    ast_free(decl);
                    ast_free(member);
                    return NULL;
                }
                
                if (type == ASTC_STRUCT_DECL) {
                    decl->data.struct_decl.members = new_members;
                } else { // ASTC_UNION_DECL
                    decl->data.union_decl.members = new_members;
                }
            }
            
            // 添加成员到结构体/联合体
            if (type == ASTC_STRUCT_DECL) {
                decl->data.struct_decl.members[decl->data.struct_decl.member_count++] = member;
            } else { // ASTC_UNION_DECL
                decl->data.union_decl.members[decl->data.union_decl.member_count++] = member;
            }
        }
        
        if (!match(parser, TOKEN_RBRACE)) {
            parser_error(parser, "预期右花括号");
            // 释放已分配的资源
            if (type == ASTC_STRUCT_DECL) {
                for (int i = 0; i < decl->data.struct_decl.member_count; i++) {
                    ast_free(decl->data.struct_decl.members[i]);
                }
                free(decl->data.struct_decl.members);
            } else { // ASTC_UNION_DECL
                for (int i = 0; i < decl->data.union_decl.member_count; i++) {
                    ast_free(decl->data.union_decl.members[i]);
                }
                free(decl->data.union_decl.members);
            }
            ast_free(decl);
            return NULL;
        }
    } else {
        // 没有定义体
        if (type == ASTC_STRUCT_DECL) {
            decl->data.struct_decl.members = NULL;
            decl->data.struct_decl.member_count = 0;
        } else { // ASTC_UNION_DECL
            decl->data.union_decl.members = NULL;
            decl->data.union_decl.member_count = 0;
        }
    }
    
    return decl;
}

// 解析枚举声明
static struct ASTNode* parse_enum(Parser *parser) {
    if (!match(parser, TOKEN_ENUM)) {
        parser_error(parser, "预期enum关键字");
        return NULL;
    }
    
    Token *token = peek(parser);
    struct ASTNode *decl = ast_create_node(ASTC_ENUM_DECL, token->line, token->column);
    if (!decl) return NULL;
    
    // 检查是否有标识符
    if (check(parser, TOKEN_IDENTIFIER)) {
        token = advance(parser);
        decl->data.enum_decl.name = strdup(token->value);
        if (!decl->data.enum_decl.name) {
            parser_error(parser, "内存分配失败");
            ast_free(decl);
            return NULL;
        }
    } else {
        decl->data.enum_decl.name = NULL;
    }
    
    // 检查是否有定义体
    if (match(parser, TOKEN_LBRACE)) {
        // 解析枚举常量列表
        decl->data.enum_decl.constants = NULL;
        decl->data.enum_decl.constant_count = 0;
        int capacity = 0;
        
        do {
            if (check(parser, TOKEN_RBRACE)) break;
            
            // 解析枚举常量
            if (!check(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "预期标识符");
                // 释放已分配的资源
                for (int i = 0; i < decl->data.enum_decl.constant_count; i++) {
                    ast_free(decl->data.enum_decl.constants[i]);
                }
                free(decl->data.enum_decl.constants);
                ast_free(decl);
                return NULL;
            }
            
            token = advance(parser);
            struct ASTNode *constant = ast_create_node(ASTC_ENUM_CONSTANT, token->line, token->column);
            if (!constant) {
                // 释放已分配的资源
                for (int i = 0; i < decl->data.enum_decl.constant_count; i++) {
                    ast_free(decl->data.enum_decl.constants[i]);
                }
                free(decl->data.enum_decl.constants);
                ast_free(decl);
                return NULL;
            }
            
            constant->data.enum_constant.name = strdup(token->value);
            if (!constant->data.enum_constant.name) {
                parser_error(parser, "内存分配失败");
                ast_free(constant);
                // 释放已分配的资源
                for (int i = 0; i < decl->data.enum_decl.constant_count; i++) {
                    ast_free(decl->data.enum_decl.constants[i]);
                }
                free(decl->data.enum_decl.constants);
                ast_free(decl);
                return NULL;
            }
            
            // 检查是否有初始化值
            if (match(parser, TOKEN_ASSIGN)) {
                constant->data.enum_constant.has_value = 1;
                struct ASTNode *value = parse_expression(parser);
                if (!value) {
                    ast_free(constant);
                    // 释放已分配的资源
                    for (int i = 0; i < decl->data.enum_decl.constant_count; i++) {
                        ast_free(decl->data.enum_decl.constants[i]);
                    }
                    free(decl->data.enum_decl.constants);
                    ast_free(decl);
                    return NULL;
                }
                
                constant->data.enum_constant.value = value;
            } else {
                constant->data.enum_constant.has_value = 0;
                constant->data.enum_constant.value = NULL;
            }
            
            // 动态扩展常量数组
            if (decl->data.enum_decl.constant_count >= capacity) {
                capacity = capacity == 0 ? 8 : capacity * 2;
                struct ASTNode **new_constants = (struct ASTNode **)realloc(
                    decl->data.enum_decl.constants, 
                    capacity * sizeof(struct ASTNode *)
                );
                
                if (!new_constants) {
                    parser_error(parser, "内存分配失败");
                    ast_free(constant);
                    // 释放已分配的资源
                    for (int i = 0; i < decl->data.enum_decl.constant_count; i++) {
                        ast_free(decl->data.enum_decl.constants[i]);
                    }
                    free(decl->data.enum_decl.constants);
                    ast_free(decl);
                    return NULL;
                }
                
                decl->data.enum_decl.constants = new_constants;
            }
            
            // 添加常量到枚举
            decl->data.enum_decl.constants[decl->data.enum_decl.constant_count++] = constant;
            
        } while (match(parser, TOKEN_COMMA));
        
        if (!match(parser, TOKEN_RBRACE)) {
            parser_error(parser, "预期右花括号");
            // 释放已分配的资源
            for (int i = 0; i < decl->data.enum_decl.constant_count; i++) {
                ast_free(decl->data.enum_decl.constants[i]);
            }
            free(decl->data.enum_decl.constants);
            ast_free(decl);
            return NULL;
        }
    } else {
        decl->data.enum_decl.constants = NULL;
        decl->data.enum_decl.constant_count = 0;
    }
    
    return decl;
}

// 更新解析声明函数
static struct ASTNode* parse_declaration(Parser *parser) {
    // 检查是否为结构体、联合体或枚举
    Token *token = peek(parser);
    if (!token) return NULL;
    
    if (token->type == TOKEN_STRUCT || token->type == TOKEN_UNION) {
        return parse_struct_or_union(parser);
    } else if (token->type == TOKEN_ENUM) {
        return parse_enum(parser);
    }
    
    // 解析类型说明符
    struct ASTNode *type_node = NULL;
    
    // 检查是否为类型关键字
    switch (token->type) {
        case TOKEN_VOID:
        case TOKEN_CHAR:
        case TOKEN_SHORT:
        case TOKEN_INT:
        case TOKEN_LONG:
        case TOKEN_FLOAT:
        case TOKEN_DOUBLE:
        case TOKEN_SIGNED:
        case TOKEN_UNSIGNED:
            // 创建类型节点
            type_node = ast_create_node(ASTC_TYPE_SPECIFIER, token->line, token->column);
            if (!type_node) return NULL;
            
            // 设置类型
            switch (token->type) {
                case TOKEN_VOID: type_node->data.type_specifier.type = ASTC_TYPE_VOID; break;
                case TOKEN_CHAR: type_node->data.type_specifier.type = ASTC_TYPE_CHAR; break;
                case TOKEN_SHORT: type_node->data.type_specifier.type = ASTC_TYPE_SHORT; break;
                case TOKEN_INT: type_node->data.type_specifier.type = ASTC_TYPE_INT; break;
                case TOKEN_LONG: type_node->data.type_specifier.type = ASTC_TYPE_LONG; break;
                case TOKEN_FLOAT: type_node->data.type_specifier.type = ASTC_TYPE_FLOAT; break;
                case TOKEN_DOUBLE: type_node->data.type_specifier.type = ASTC_TYPE_DOUBLE; break;
                case TOKEN_SIGNED: type_node->data.type_specifier.type = ASTC_TYPE_SIGNED; break;
                case TOKEN_UNSIGNED: type_node->data.type_specifier.type = ASTC_TYPE_UNSIGNED; break;
                default:
                    ast_free(type_node);
                    return NULL;
            }
            
            advance(parser); // 消耗类型关键字
            break;
            
        default:
            // 不是声明
            return NULL;
    }
    
    // 检查是否为指针类型
    type_node = parse_pointer_type(parser, type_node);
    if (!type_node) return NULL;
    
    // 解析声明符
    token = peek(parser);
    if (!token || token->type != TOKEN_IDENTIFIER) {
        parser_error(parser, "预期标识符");
        ast_free(type_node);
        return NULL;
    }
    
    char *name = strdup(token->value);
    if (!name) {
        parser_error(parser, "内存分配失败");
        ast_free(type_node);
        return NULL;
    }
    
    advance(parser); // 消耗标识符
    
    // 检查是否为数组类型
    if (check(parser, TOKEN_LBRACKET)) {
        type_node = parse_array_type(parser, type_node);
        if (!type_node) {
            free(name);
            return NULL;
        }
    }
    
    // 检查是否为函数指针类型
    if (check(parser, TOKEN_LPAREN) && peek_n(parser, 1) && peek_n(parser, 1)->type == TOKEN_STAR) {
        type_node = parse_function_type(parser, type_node);
        if (!type_node) {
            free(name);
            return NULL;
        }
    }
    
    // 检查是变量声明还是函数声明
    if (match(parser, TOKEN_LPAREN)) {
        // 函数声明
        struct ASTNode *func_decl = ast_create_node(ASTC_FUNC_DECL, token->line, token->column);
        if (!func_decl) {
            free(name);
            ast_free(type_node);
            return NULL;
        }
        
        func_decl->data.func_decl.name = name;
        func_decl->data.func_decl.return_type = type_node;
        
        // 解析参数列表
        func_decl->data.func_decl.params = NULL;
        func_decl->data.func_decl.param_count = 0;
        
        if (!check(parser, TOKEN_RPAREN)) {
            // 有参数
            struct ASTNode **params = NULL;
            int param_count = 0;
            int capacity = 0;
            
            do {
                // 解析参数声明
                struct ASTNode *param = parse_declaration(parser);
                if (!param) {
                    // 释放已分配的资源
                    for (int i = 0; i < param_count; i++) {
                        ast_free(params[i]);
                    }
                    free(params);
                    ast_free(func_decl);
                    return NULL;
                }
                
                // 动态扩展参数数组
                if (param_count >= capacity) {
                    capacity = capacity == 0 ? 4 : capacity * 2;
                    struct ASTNode **new_params = (struct ASTNode **)realloc(params, capacity * sizeof(struct ASTNode *));
                    if (!new_params) {
                        parser_error(parser, "内存分配失败");
                        // 释放已分配的资源
                        for (int i = 0; i < param_count; i++) {
                            ast_free(params[i]);
                        }
                        free(params);
                        ast_free(func_decl);
                        return NULL;
                    }
                    params = new_params;
                }
                
                params[param_count++] = param;
            } while (match(parser, TOKEN_COMMA));
            
            func_decl->data.func_decl.params = params;
            func_decl->data.func_decl.param_count = param_count;
        }
        
        if (!match(parser, TOKEN_RPAREN)) {
            parser_error(parser, "预期右括号");
            ast_free(func_decl);
            return NULL;
        }
        
        // 检查是函数定义还是声明
        if (check(parser, TOKEN_LBRACE)) {
            // 函数定义
            func_decl->data.func_decl.has_body = 1;
            
            // 解析函数体
            struct ASTNode *body = parse_compound_statement(parser);
            if (!body) {
                ast_free(func_decl);
                return NULL;
            }
            
            func_decl->data.func_decl.body = body;
        } else {
            // 函数声明
            func_decl->data.func_decl.has_body = 0;
            func_decl->data.func_decl.body = NULL;
            
            if (!match(parser, TOKEN_SEMICOLON)) {
                parser_error(parser, "预期分号");
                ast_free(func_decl);
                return NULL;
            }
        }
        
        return func_decl;
    } else {
        // 变量声明
        struct ASTNode *var_decl = ast_create_node(ASTC_VAR_DECL, token->line, token->column);
        if (!var_decl) {
            free(name);
            ast_free(type_node);
            return NULL;
        }
        
        var_decl->data.var_decl.name = name;
        var_decl->data.var_decl.type = type_node;
        
        // 检查是否有初始化器
        if (match(parser, TOKEN_ASSIGN)) {
            // 解析初始化表达式
            struct ASTNode *init = parse_expression(parser);
            if (!init) {
                ast_free(var_decl);
                return NULL;
            }
            
            var_decl->data.var_decl.initializer = init;
        } else {
            var_decl->data.var_decl.initializer = NULL;
        }
        
        if (!match(parser, TOKEN_SEMICOLON)) {
            parser_error(parser, "预期分号");
            ast_free(var_decl);
            return NULL;
        }
        
        return var_decl;
    }
}

// ===============================================
// C2ASTC API实现
// ===============================================

// 默认选项
C2AstcOptions c2astc_default_options(void) {
    C2AstcOptions options;
    options.optimize_level = false;
    options.enable_extensions = true;
    options.emit_debug_info = true;
    return options;
}

// 打印版本信息
void c2astc_print_version(void) {
    printf("C to ASTC Converter v0.1\n");
    printf("Part of Self-Evolve AI System\n");
}

// 从文件加载C源代码并转换为ASTC
struct ASTNode* c2astc_convert_file(const char *filename, const C2AstcOptions *options) {
    // 尝试直接打开文件
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        // 如果失败，尝试替换路径分隔符
        char fixed_filename[1024];
        strncpy(fixed_filename, filename, sizeof(fixed_filename) - 1);
        fixed_filename[sizeof(fixed_filename) - 1] = '\0';
        
        // 将所有反斜杠转换为正斜杠
        for (char *p = fixed_filename; *p; p++) {
            if (*p == '\\') *p = '/';
        }
        
        fp = fopen(fixed_filename, "rb");
        if (!fp) {
            set_error("无法打开文件: %s", filename);
            return NULL;
        }
    }
    
    // 获取文件大小
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(fp);
        set_error("文件为空或无法获取文件大小: %s", filename);
        return NULL;
    }
    
    // 分配内存并读取文件内容
    char *source = (char *)malloc(file_size + 1);
    if (!source) {
        fclose(fp);
        set_error("内存分配失败");
        return NULL;
    }
    
    size_t bytes_read = fread(source, 1, file_size, fp);
    fclose(fp);
    
    if (bytes_read != (size_t)file_size) {
        free(source);
        set_error("读取文件失败: %s", filename);
        return NULL;
    }
    
    source[file_size] = '\0';
    
    // 转换代码
    struct ASTNode *root = c2astc_convert(source, options);
    
    free(source);
    return root;
}

// 将C源代码转换为ASTC
struct ASTNode* c2astc_convert(const char *source, const C2AstcOptions *options) {
    if (!source) {
        set_error("源代码为NULL");
        return NULL;
    }
    
    // 使用默认选项
    C2AstcOptions default_options = c2astc_default_options();
    if (!options) options = &default_options;
    
    // 1. 词法分析
    Lexer lexer;
    init_lexer(&lexer, source, NULL);
    
    int token_count = 0;
    Token **tokens = tokenize(&lexer, &token_count);
    if (!tokens) {
        set_error("词法分析失败: %s", lexer.error_msg);
        return NULL;
    }
    
    // 2. 语法分析
    Parser parser;
    init_parser(&parser, tokens, token_count);
    
    struct ASTNode *root = parse_translation_unit(&parser);
    if (!root) {
        set_error("语法分析失败: %s", parser.error_msg);
        free_tokens(tokens, token_count);
        return NULL;
    }
    
    // 释放tokens
    free_tokens(tokens, token_count);
    
    // 3. 语义分析和优化
    if (options->optimize_level) {
        // TODO: 实现优化
    }
    
    return root;
}

// 序列化ASTC为二进制格式
unsigned char* c2astc_serialize(struct ASTNode *node, size_t *out_size) {
    if (!node || !out_size) {
        set_error("无效的参数");
        return NULL;
    }
    
    // 序列化格式：
    // 1. 魔数: 'ASTC' (4字节)
    // 2. 版本: 1 (4字节)
    // 3. 节点类型 (4字节)
    // 4. 行号 (4字节)
    // 5. 列号 (4字节)
    // 6. 节点特定数据 (变长)
    
    // 先分配一个基本大小的缓冲区
    size_t buffer_size = 1024;
    unsigned char *buffer = (unsigned char*)malloc(buffer_size);
    if (!buffer) {
        set_error("内存分配失败");
        return NULL;
    }
    
    // 写入魔数和版本
    memcpy(buffer, "ASTC", 4);
    *((int*)(buffer + 4)) = 1; // 版本1
    
    // 写入节点类型、行号和列号
    *((int*)(buffer + 8)) = node->type;
    *((int*)(buffer + 12)) = node->line;
    *((int*)(buffer + 16)) = node->column;
    
    // 根据节点类型写入特定数据
    size_t pos = 20; // 当前写入位置
    
    // 这里只实现了部分节点类型的序列化，完整实现需要处理所有类型
    switch (node->type) {
        case ASTC_EXPR_IDENTIFIER:
            // 写入标识符名称
            if (node->data.identifier.name) {
                size_t name_len = strlen(node->data.identifier.name) + 1; // 包含null终止符
                
                // 确保缓冲区足够大
                if (pos + 4 + name_len > buffer_size) {
                    buffer_size = pos + 4 + name_len + 1024; // 额外分配一些空间
                    unsigned char *new_buffer = (unsigned char*)realloc(buffer, buffer_size);
                    if (!new_buffer) {
                        free(buffer);
                        set_error("内存分配失败");
                        return NULL;
                    }
                    buffer = new_buffer;
                }
                
                // 写入名称长度和名称
                *((int*)(buffer + pos)) = (int)name_len;
                pos += 4;
                memcpy(buffer + pos, node->data.identifier.name, name_len);
                pos += name_len;
            } else {
                // 名称为NULL
                *((int*)(buffer + pos)) = 0;
                pos += 4;
            }
            break;
            
        case ASTC_EXPR_CONSTANT:
            // 写入常量类型和值
            *((int*)(buffer + pos)) = node->data.constant.type;
            pos += 4;
            
            if (node->data.constant.type == ASTC_TYPE_INT) {
                *((int64_t*)(buffer + pos)) = node->data.constant.int_val;
                pos += 8;
            } else {
                *((double*)(buffer + pos)) = node->data.constant.float_val;
                pos += 8;
            }
            break;
            
        case ASTC_EXPR_STRING_LITERAL:
            // 写入字符串值
            if (node->data.string_literal.value) {
                size_t str_len = strlen(node->data.string_literal.value) + 1; // 包含null终止符
                
                // 确保缓冲区足够大
                if (pos + 4 + str_len > buffer_size) {
                    buffer_size = pos + 4 + str_len + 1024; // 额外分配一些空间
                    unsigned char *new_buffer = (unsigned char*)realloc(buffer, buffer_size);
                    if (!new_buffer) {
                        free(buffer);
                        set_error("内存分配失败");
                        return NULL;
                    }
                    buffer = new_buffer;
                }
                
                // 写入字符串长度和内容
                *((int*)(buffer + pos)) = (int)str_len;
                pos += 4;
                memcpy(buffer + pos, node->data.string_literal.value, str_len);
                pos += str_len;
            } else {
                // 字符串为NULL
                *((int*)(buffer + pos)) = 0;
                pos += 4;
            }
            break;
            
        case ASTC_BINARY_OP:
            // 写入操作符类型
            *((int*)(buffer + pos)) = node->data.binary_op.op;
            pos += 4;
            
            // 递归序列化左操作数
            if (node->data.binary_op.left) {
                size_t left_size;
                unsigned char *left_data = c2astc_serialize(node->data.binary_op.left, &left_size);
                if (!left_data) {
                    free(buffer);
                    return NULL;
                }
                
                // 确保缓冲区足够大
                if (pos + 4 + left_size > buffer_size) {
                    buffer_size = pos + 4 + left_size + 1024;
                    unsigned char *new_buffer = (unsigned char*)realloc(buffer, buffer_size);
                    if (!new_buffer) {
                        free(left_data);
                        free(buffer);
                        set_error("内存分配失败");
                        return NULL;
                    }
                    buffer = new_buffer;
                }
                
                // 写入左操作数大小和数据
                *((int*)(buffer + pos)) = (int)left_size;
                pos += 4;
                memcpy(buffer + pos, left_data, left_size);
                pos += left_size;
                free(left_data);
            } else {
                // 左操作数为NULL
                *((int*)(buffer + pos)) = 0;
                pos += 4;
            }
            
            // 递归序列化右操作数
            if (node->data.binary_op.right) {
                size_t right_size;
                unsigned char *right_data = c2astc_serialize(node->data.binary_op.right, &right_size);
                if (!right_data) {
                    free(buffer);
                    return NULL;
                }
                
                // 确保缓冲区足够大
                if (pos + 4 + right_size > buffer_size) {
                    buffer_size = pos + 4 + right_size + 1024;
                    unsigned char *new_buffer = (unsigned char*)realloc(buffer, buffer_size);
                    if (!new_buffer) {
                        free(right_data);
                        free(buffer);
                        set_error("内存分配失败");
                        return NULL;
                    }
                    buffer = new_buffer;
                }
                
                // 写入右操作数大小和数据
                *((int*)(buffer + pos)) = (int)right_size;
                pos += 4;
                memcpy(buffer + pos, right_data, right_size);
                pos += right_size;
                free(right_data);
            } else {
                // 右操作数为NULL
                *((int*)(buffer + pos)) = 0;
                pos += 4;
            }
            break;
            
        case ASTC_UNARY_OP:
            // 写入操作符类型
            *((int*)(buffer + pos)) = node->data.unary_op.op;
            pos += 4;
            
            // 递归序列化操作数
            if (node->data.unary_op.operand) {
                size_t operand_size;
                unsigned char *operand_data = c2astc_serialize(node->data.unary_op.operand, &operand_size);
                if (!operand_data) {
                    free(buffer);
                    return NULL;
                }
                
                // 确保缓冲区足够大
                if (pos + 4 + operand_size > buffer_size) {
                    buffer_size = pos + 4 + operand_size + 1024;
                    unsigned char *new_buffer = (unsigned char*)realloc(buffer, buffer_size);
                    if (!new_buffer) {
                        free(operand_data);
                        free(buffer);
                        set_error("内存分配失败");
                        return NULL;
                    }
                    buffer = new_buffer;
                }
                
                // 写入操作数大小和数据
                *((int*)(buffer + pos)) = (int)operand_size;
                pos += 4;
                memcpy(buffer + pos, operand_data, operand_size);
                pos += operand_size;
                free(operand_data);
            } else {
                // 操作数为NULL
                *((int*)(buffer + pos)) = 0;
                pos += 4;
            }
            break;
            
        case ASTC_CALL_EXPR:
            // 递归序列化被调用函数
            if (node->data.call_expr.callee) {
                size_t callee_size;
                unsigned char *callee_data = c2astc_serialize(node->data.call_expr.callee, &callee_size);
                if (!callee_data) {
                    free(buffer);
                    return NULL;
                }
                
                // 确保缓冲区足够大
                if (pos + 4 + callee_size > buffer_size) {
                    buffer_size = pos + 4 + callee_size + 1024;
                    unsigned char *new_buffer = (unsigned char*)realloc(buffer, buffer_size);
                    if (!new_buffer) {
                        free(callee_data);
                        free(buffer);
                        set_error("内存分配失败");
                        return NULL;
                    }
                    buffer = new_buffer;
                }
                
                // 写入被调用函数大小和数据
                *((int*)(buffer + pos)) = (int)callee_size;
                pos += 4;
                memcpy(buffer + pos, callee_data, callee_size);
                pos += callee_size;
                free(callee_data);
            } else {
                // 被调用函数为NULL
                *((int*)(buffer + pos)) = 0;
                pos += 4;
            }
            
            // 写入参数数量
            *((int*)(buffer + pos)) = node->data.call_expr.arg_count;
            pos += 4;
            
            // 递归序列化参数
            for (int i = 0; i < node->data.call_expr.arg_count; i++) {
                if (node->data.call_expr.args[i]) {
                    size_t arg_size;
                    unsigned char *arg_data = c2astc_serialize(node->data.call_expr.args[i], &arg_size);
                    if (!arg_data) {
                        free(buffer);
                        return NULL;
                    }
                    
                    // 确保缓冲区足够大
                    if (pos + 4 + arg_size > buffer_size) {
                        buffer_size = pos + 4 + arg_size + 1024;
                        unsigned char *new_buffer = (unsigned char*)realloc(buffer, buffer_size);
                        if (!new_buffer) {
                            free(arg_data);
                            free(buffer);
                            set_error("内存分配失败");
                            return NULL;
                        }
                        buffer = new_buffer;
                    }
                    
                    // 写入参数大小和数据
                    *((int*)(buffer + pos)) = (int)arg_size;
                    pos += 4;
                    memcpy(buffer + pos, arg_data, arg_size);
                    pos += arg_size;
                    free(arg_data);
                } else {
                    // 参数为NULL
                    *((int*)(buffer + pos)) = 0;
                    pos += 4;
                }
            }
            break;
            
        case ASTC_TRANSLATION_UNIT:
            // 序列化翻译单元
            // 写入声明数量
            *((int*)(buffer + pos)) = node->data.translation_unit.declaration_count;
            pos += 4;

            // 递归序列化所有声明
            for (int i = 0; i < node->data.translation_unit.declaration_count; i++) {
                size_t child_size;
                unsigned char* child_data = c2astc_serialize(node->data.translation_unit.declarations[i], &child_size);
                if (child_data) {
                    // 确保缓冲区足够大
                    if (pos + 4 + child_size > buffer_size) {
                        buffer_size = pos + 4 + child_size + 1024;
                        unsigned char *new_buffer = (unsigned char*)realloc(buffer, buffer_size);
                        if (!new_buffer) {
                            free(buffer);
                            free(child_data);
                            set_error("内存分配失败");
                            return NULL;
                        }
                        buffer = new_buffer;
                    }

                    // 写入子节点大小和数据
                    *((int*)(buffer + pos)) = (int)child_size;
                    pos += 4;
                    memcpy(buffer + pos, child_data, child_size);
                    pos += child_size;
                    free(child_data);
                } else {
                    // 子节点序列化失败
                    *((int*)(buffer + pos)) = 0;
                    pos += 4;
                }
            }
            break;

        case ASTC_FUNC_DECL:
            // 序列化函数声明
            if (node->data.func_decl.name) {
                size_t name_len = strlen(node->data.func_decl.name) + 1;

                // 确保缓冲区足够大
                if (pos + 4 + name_len > buffer_size) {
                    buffer_size = pos + 4 + name_len + 1024;
                    unsigned char *new_buffer = (unsigned char*)realloc(buffer, buffer_size);
                    if (!new_buffer) {
                        free(buffer);
                        set_error("内存分配失败");
                        return NULL;
                    }
                    buffer = new_buffer;
                }

                // 写入函数名长度和名称
                *((int*)(buffer + pos)) = (int)name_len;
                pos += 4;
                memcpy(buffer + pos, node->data.func_decl.name, name_len);
                pos += name_len;
            } else {
                // 函数名为NULL
                *((int*)(buffer + pos)) = 0;
                pos += 4;
            }

            // 写入参数数量
            *((int*)(buffer + pos)) = node->data.func_decl.param_count;
            pos += 4;

            // 写入是否有函数体
            *((int*)(buffer + pos)) = node->data.func_decl.has_body ? 1 : 0;
            pos += 4;

            // 序列化函数体
            if (node->data.func_decl.has_body && node->data.func_decl.body) {
                size_t body_size;
                unsigned char* body_data = c2astc_serialize(node->data.func_decl.body, &body_size);
                if (body_data) {
                    // 确保缓冲区足够大
                    if (pos + 4 + body_size > buffer_size) {
                        buffer_size = pos + 4 + body_size + 1024;
                        unsigned char *new_buffer = (unsigned char*)realloc(buffer, buffer_size);
                        if (!new_buffer) {
                            free(buffer);
                            free(body_data);
                            set_error("内存分配失败");
                            return NULL;
                        }
                        buffer = new_buffer;
                    }

                    // 写入函数体大小和数据
                    *((int*)(buffer + pos)) = (int)body_size;
                    pos += 4;
                    memcpy(buffer + pos, body_data, body_size);
                    pos += body_size;
                    free(body_data);
                } else {
                    // 函数体序列化失败
                    *((int*)(buffer + pos)) = 0;
                    pos += 4;
                }
            } else {
                // 没有函数体
                *((int*)(buffer + pos)) = 0;
                pos += 4;
            }

            break;

        case ASTC_COMPOUND_STMT:
            // 序列化复合语句
            // 写入语句数量
            *((int*)(buffer + pos)) = node->data.compound_stmt.statement_count;
            pos += 4;

            // 递归序列化所有语句
            for (int i = 0; i < node->data.compound_stmt.statement_count; i++) {
                size_t stmt_size;
                unsigned char* stmt_data = c2astc_serialize(node->data.compound_stmt.statements[i], &stmt_size);
                if (stmt_data) {
                    // 确保缓冲区足够大
                    if (pos + 4 + stmt_size > buffer_size) {
                        buffer_size = pos + 4 + stmt_size + 1024;
                        unsigned char *new_buffer = (unsigned char*)realloc(buffer, buffer_size);
                        if (!new_buffer) {
                            free(buffer);
                            free(stmt_data);
                            set_error("内存分配失败");
                            return NULL;
                        }
                        buffer = new_buffer;
                    }

                    // 写入语句大小和数据
                    *((int*)(buffer + pos)) = (int)stmt_size;
                    pos += 4;
                    memcpy(buffer + pos, stmt_data, stmt_size);
                    pos += stmt_size;
                    free(stmt_data);
                } else {
                    // 语句序列化失败
                    *((int*)(buffer + pos)) = 0;
                    pos += 4;
                }
            }
            break;

        case ASTC_RETURN_STMT:
            // 序列化return语句
            if (node->data.return_stmt.value) {
                size_t value_size;
                unsigned char* value_data = c2astc_serialize(node->data.return_stmt.value, &value_size);
                if (value_data) {
                    // 确保缓冲区足够大
                    if (pos + 4 + value_size > buffer_size) {
                        buffer_size = pos + 4 + value_size + 1024;
                        unsigned char *new_buffer = (unsigned char*)realloc(buffer, buffer_size);
                        if (!new_buffer) {
                            free(buffer);
                            free(value_data);
                            set_error("内存分配失败");
                            return NULL;
                        }
                        buffer = new_buffer;
                    }

                    // 写入返回值大小和数据
                    *((int*)(buffer + pos)) = (int)value_size;
                    pos += 4;
                    memcpy(buffer + pos, value_data, value_size);
                    pos += value_size;
                    free(value_data);
                } else {
                    // 返回值序列化失败
                    *((int*)(buffer + pos)) = 0;
                    pos += 4;
                }
            } else {
                // 没有返回值
                *((int*)(buffer + pos)) = 0;
                pos += 4;
            }
            break;

        case ASTC_VAR_DECL:
            // 序列化变量声明
            if (node->data.var_decl.name) {
                size_t name_len = strlen(node->data.var_decl.name) + 1;

                // 确保缓冲区足够大
                if (pos + 4 + name_len > buffer_size) {
                    buffer_size = pos + 4 + name_len + 1024;
                    unsigned char *new_buffer = (unsigned char*)realloc(buffer, buffer_size);
                    if (!new_buffer) {
                        free(buffer);
                        set_error("内存分配失败");
                        return NULL;
                    }
                    buffer = new_buffer;
                }

                // 写入变量名长度和名称
                *((int*)(buffer + pos)) = (int)name_len;
                pos += 4;
                memcpy(buffer + pos, node->data.var_decl.name, name_len);
                pos += name_len;
            } else {
                // 变量名为NULL
                *((int*)(buffer + pos)) = 0;
                pos += 4;
            }

            // 序列化初始化表达式
            if (node->data.var_decl.initializer) {
                size_t init_size;
                unsigned char* init_data = c2astc_serialize(node->data.var_decl.initializer, &init_size);
                if (init_data) {
                    // 确保缓冲区足够大
                    if (pos + 4 + init_size > buffer_size) {
                        buffer_size = pos + 4 + init_size + 1024;
                        unsigned char *new_buffer = (unsigned char*)realloc(buffer, buffer_size);
                        if (!new_buffer) {
                            free(buffer);
                            free(init_data);
                            set_error("内存分配失败");
                            return NULL;
                        }
                        buffer = new_buffer;
                    }

                    // 写入初始化表达式大小和数据
                    *((int*)(buffer + pos)) = (int)init_size;
                    pos += 4;
                    memcpy(buffer + pos, init_data, init_size);
                    pos += init_size;
                    free(init_data);
                } else {
                    // 初始化表达式序列化失败
                    *((int*)(buffer + pos)) = 0;
                    pos += 4;
                }
            } else {
                // 没有初始化表达式
                *((int*)(buffer + pos)) = 0;
                pos += 4;
            }
            break;

        case ASTC_IF_STMT:
            // 序列化if语句
            // 序列化条件表达式
            if (node->data.if_stmt.condition) {
                size_t cond_size;
                unsigned char* cond_data = c2astc_serialize(node->data.if_stmt.condition, &cond_size);
                if (cond_data) {
                    // 确保缓冲区足够大
                    if (pos + 4 + cond_size > buffer_size) {
                        buffer_size = pos + 4 + cond_size + 1024;
                        unsigned char *new_buffer = (unsigned char*)realloc(buffer, buffer_size);
                        if (!new_buffer) {
                            free(buffer);
                            free(cond_data);
                            set_error("内存分配失败");
                            return NULL;
                        }
                        buffer = new_buffer;
                    }

                    // 写入条件表达式大小和数据
                    *((int*)(buffer + pos)) = (int)cond_size;
                    pos += 4;
                    memcpy(buffer + pos, cond_data, cond_size);
                    pos += cond_size;
                    free(cond_data);
                } else {
                    *((int*)(buffer + pos)) = 0;
                    pos += 4;
                }
            } else {
                *((int*)(buffer + pos)) = 0;
                pos += 4;
            }

            // 序列化then分支
            if (node->data.if_stmt.then_branch) {
                size_t then_size;
                unsigned char* then_data = c2astc_serialize(node->data.if_stmt.then_branch, &then_size);
                if (then_data) {
                    if (pos + 4 + then_size > buffer_size) {
                        buffer_size = pos + 4 + then_size + 1024;
                        unsigned char *new_buffer = (unsigned char*)realloc(buffer, buffer_size);
                        if (!new_buffer) {
                            free(buffer);
                            free(then_data);
                            set_error("内存分配失败");
                            return NULL;
                        }
                        buffer = new_buffer;
                    }

                    *((int*)(buffer + pos)) = (int)then_size;
                    pos += 4;
                    memcpy(buffer + pos, then_data, then_size);
                    pos += then_size;
                    free(then_data);
                } else {
                    *((int*)(buffer + pos)) = 0;
                    pos += 4;
                }
            } else {
                *((int*)(buffer + pos)) = 0;
                pos += 4;
            }

            // 序列化else分支
            if (node->data.if_stmt.else_branch) {
                size_t else_size;
                unsigned char* else_data = c2astc_serialize(node->data.if_stmt.else_branch, &else_size);
                if (else_data) {
                    if (pos + 4 + else_size > buffer_size) {
                        buffer_size = pos + 4 + else_size + 1024;
                        unsigned char *new_buffer = (unsigned char*)realloc(buffer, buffer_size);
                        if (!new_buffer) {
                            free(buffer);
                            free(else_data);
                            set_error("内存分配失败");
                            return NULL;
                        }
                        buffer = new_buffer;
                    }

                    *((int*)(buffer + pos)) = (int)else_size;
                    pos += 4;
                    memcpy(buffer + pos, else_data, else_size);
                    pos += else_size;
                    free(else_data);
                } else {
                    *((int*)(buffer + pos)) = 0;
                    pos += 4;
                }
            } else {
                *((int*)(buffer + pos)) = 0;
                pos += 4;
            }
            break;

        // 其他节点类型的序列化...
        // 完整实现需要处理所有节点类型

        default:
            // 对于其他类型，暂时只序列化基本信息
            break;
    }
    
    // 调整缓冲区大小为实际使用的大小
    unsigned char *final_buffer = (unsigned char*)realloc(buffer, pos);
    if (!final_buffer) {
        free(buffer);
        set_error("内存分配失败");
        return NULL;
    }
    
    *out_size = pos;
    return final_buffer;
}

// 反序列化二进制格式为ASTC
struct ASTNode* c2astc_deserialize(const unsigned char *binary, size_t size) {
    if (!binary || size < 20) { // 至少需要包含魔数、版本、类型、行号和列号
        set_error("无效的二进制数据");
        return NULL;
    }
    
    // 验证魔数
    if (memcmp(binary, "ASTC", 4) != 0) {
        set_error("无效的ASTC格式");
        return NULL;
    }
    
    // 验证版本
    int version = *((int*)(binary + 4));
    if (version != 1) {
        set_error("不支持的ASTC版本");
        return NULL;
    }
    
    // 读取节点类型、行号和列号
    ASTNodeType type = *((int*)(binary + 8));
    int line = *((int*)(binary + 12));
    int column = *((int*)(binary + 16));
    
    // 创建节点
    struct ASTNode *node = ast_create_node(type, line, column);
    if (!node) {
        return NULL;
    }
    
    // 根据节点类型读取特定数据
    size_t pos = 20; // 当前读取位置
    
    // 这里只实现了部分节点类型的反序列化，完整实现需要处理所有类型
    switch (type) {
        case ASTC_EXPR_IDENTIFIER:
            // 读取标识符名称
            if (pos + 4 <= size) {
                int name_len = *((int*)(binary + pos));
                pos += 4;
                
                if (name_len > 0 && pos + name_len <= size) {
                    node->data.identifier.name = (char*)malloc(name_len);
                    if (!node->data.identifier.name) {
                        ast_free(node);
                        set_error("内存分配失败");
                        return NULL;
                    }
                    
                    memcpy(node->data.identifier.name, binary + pos, name_len);
                    pos += name_len;
                } else {
                    node->data.identifier.name = NULL;
                }
            }
            break;
            
        case ASTC_EXPR_CONSTANT:
            // 读取常量类型和值
            if (pos + 4 <= size) {
                node->data.constant.type = *((int*)(binary + pos));
                pos += 4;
                
                if (pos + 8 <= size) {
                    if (node->data.constant.type == ASTC_TYPE_INT) {
                        node->data.constant.int_val = *((int64_t*)(binary + pos));
                    } else {
                        node->data.constant.float_val = *((double*)(binary + pos));
                    }
                    pos += 8;
                }
            }
            break;
            
        case ASTC_EXPR_STRING_LITERAL:
            // 读取字符串值
            if (pos + 4 <= size) {
                int str_len = *((int*)(binary + pos));
                pos += 4;
                
                if (str_len > 0 && pos + str_len <= size) {
                    node->data.string_literal.value = (char*)malloc(str_len);
                    if (!node->data.string_literal.value) {
                        ast_free(node);
                        set_error("内存分配失败");
                        return NULL;
                    }
                    
                    memcpy(node->data.string_literal.value, binary + pos, str_len);
                    pos += str_len;
                } else {
                    node->data.string_literal.value = NULL;
                }
            }
            break;
            
        case ASTC_BINARY_OP:
            // 读取操作符类型
            if (pos + 4 <= size) {
                node->data.binary_op.op = *((int*)(binary + pos));
                pos += 4;
                
                // 读取左操作数
                if (pos + 4 <= size) {
                    int left_size = *((int*)(binary + pos));
                    pos += 4;
                    
                    if (left_size > 0 && pos + left_size <= size) {
                        node->data.binary_op.left = c2astc_deserialize(binary + pos, left_size);
                        if (!node->data.binary_op.left) {
                            ast_free(node);
                            return NULL;
                        }
                        pos += left_size;
                    } else {
                        node->data.binary_op.left = NULL;
                    }
                }
                
                // 读取右操作数
                if (pos + 4 <= size) {
                    int right_size = *((int*)(binary + pos));
                    pos += 4;
                    
                    if (right_size > 0 && pos + right_size <= size) {
                        node->data.binary_op.right = c2astc_deserialize(binary + pos, right_size);
                        if (!node->data.binary_op.right) {
                            ast_free(node);
                            return NULL;
                        }
                        pos += right_size;
                    } else {
                        node->data.binary_op.right = NULL;
                    }
                }
            }
            break;
            
        case ASTC_UNARY_OP:
            // 读取操作符类型
            if (pos + 4 <= size) {
                node->data.unary_op.op = *((int*)(binary + pos));
                pos += 4;
                
                // 读取操作数
                if (pos + 4 <= size) {
                    int operand_size = *((int*)(binary + pos));
                    pos += 4;
                    
                    if (operand_size > 0 && pos + operand_size <= size) {
                        node->data.unary_op.operand = c2astc_deserialize(binary + pos, operand_size);
                        if (!node->data.unary_op.operand) {
                            ast_free(node);
                            return NULL;
                        }
                        pos += operand_size;
                    } else {
                        node->data.unary_op.operand = NULL;
                    }
                }
            }
            break;
            
        case ASTC_CALL_EXPR:
            // 读取被调用函数
            if (pos + 4 <= size) {
                int callee_size = *((int*)(binary + pos));
                pos += 4;
                
                if (callee_size > 0 && pos + callee_size <= size) {
                    node->data.call_expr.callee = c2astc_deserialize(binary + pos, callee_size);
                    if (!node->data.call_expr.callee) {
                        ast_free(node);
                        return NULL;
                    }
                    pos += callee_size;
                } else {
                    node->data.call_expr.callee = NULL;
                }
            }
            
            // 读取参数数量
            if (pos + 4 <= size) {
                node->data.call_expr.arg_count = *((int*)(binary + pos));
                pos += 4;
                
                if (node->data.call_expr.arg_count > 0) {
                    // 分配参数数组
                    node->data.call_expr.args = (struct ASTNode **)malloc(node->data.call_expr.arg_count * sizeof(struct ASTNode *));
                    if (!node->data.call_expr.args) {
                        ast_free(node);
                        set_error("内存分配失败");
                        return NULL;
                    }
                    
                    // 读取参数
                    for (int i = 0; i < node->data.call_expr.arg_count; i++) {
                        if (pos + 4 <= size) {
                            int arg_size = *((int*)(binary + pos));
                            pos += 4;
                            
                            if (arg_size > 0 && pos + arg_size <= size) {
                                node->data.call_expr.args[i] = c2astc_deserialize(binary + pos, arg_size);
                                if (!node->data.call_expr.args[i]) {
                                    // 释放已分配的资源
                                    for (int j = 0; j < i; j++) {
                                        ast_free(node->data.call_expr.args[j]);
                                    }
                                    free(node->data.call_expr.args);
                                    ast_free(node);
                                    return NULL;
                                }
                                pos += arg_size;
                            } else {
                                node->data.call_expr.args[i] = NULL;
                            }
                        }
                    }
                } else {
                    node->data.call_expr.args = NULL;
                }
            }
            break;
            
        case ASTC_TRANSLATION_UNIT:
            // 反序列化翻译单元
            if (pos + 4 <= size) {
                node->data.translation_unit.declaration_count = *((int*)(binary + pos));
                pos += 4;

                // 分配声明数组
                if (node->data.translation_unit.declaration_count > 0) {
                    node->data.translation_unit.declarations = (struct ASTNode**)malloc(
                        node->data.translation_unit.declaration_count * sizeof(struct ASTNode*));

                    if (!node->data.translation_unit.declarations) {
                        ast_free(node);
                        set_error("内存分配失败");
                        return NULL;
                    }

                    // 反序列化所有子声明
                    for (int i = 0; i < node->data.translation_unit.declaration_count; i++) {
                        if (pos + 4 <= size) {
                            int child_size = *((int*)(binary + pos));
                            pos += 4;

                            if (child_size > 0 && pos + child_size <= size) {
                                node->data.translation_unit.declarations[i] = c2astc_deserialize(binary + pos, child_size);
                                pos += child_size;

                                if (!node->data.translation_unit.declarations[i]) {
                                    // 子节点反序列化失败，清理已分配的内存
                                    for (int j = 0; j < i; j++) {
                                        ast_free(node->data.translation_unit.declarations[j]);
                                    }
                                    free(node->data.translation_unit.declarations);
                                    ast_free(node);
                                    return NULL;
                                }
                            } else {
                                node->data.translation_unit.declarations[i] = NULL;
                            }
                        } else {
                            node->data.translation_unit.declarations[i] = NULL;
                        }
                    }
                } else {
                    node->data.translation_unit.declarations = NULL;
                }
            }
            break;

        case ASTC_FUNC_DECL:
            // 反序列化函数声明
            if (pos + 4 <= size) {
                int name_len = *((int*)(binary + pos));
                pos += 4;

                if (name_len > 0 && pos + name_len <= size) {
                    node->data.func_decl.name = (char*)malloc(name_len);
                    if (!node->data.func_decl.name) {
                        ast_free(node);
                        set_error("内存分配失败");
                        return NULL;
                    }

                    memcpy(node->data.func_decl.name, binary + pos, name_len);
                    pos += name_len;
                } else {
                    node->data.func_decl.name = NULL;
                }
            }

            // 读取参数数量
            if (pos + 4 <= size) {
                node->data.func_decl.param_count = *((int*)(binary + pos));
                pos += 4;
            }

            // 读取是否有函数体
            if (pos + 4 <= size) {
                node->data.func_decl.has_body = (*((int*)(binary + pos)) != 0);
                pos += 4;
            }

            // 反序列化函数体
            if (pos + 4 <= size) {
                int body_size = *((int*)(binary + pos));
                pos += 4;

                if (body_size > 0 && pos + body_size <= size) {
                    node->data.func_decl.body = c2astc_deserialize(binary + pos, body_size);
                    pos += body_size;

                    if (!node->data.func_decl.body) {
                        // 函数体反序列化失败
                        if (node->data.func_decl.name) {
                            free(node->data.func_decl.name);
                        }
                        ast_free(node);
                        return NULL;
                    }
                } else {
                    node->data.func_decl.body = NULL;
                }
            }

            // 初始化其他字段为NULL/0
            node->data.func_decl.return_type = NULL;
            node->data.func_decl.params = NULL;

            break;

        case ASTC_COMPOUND_STMT:
            // 反序列化复合语句
            if (pos + 4 <= size) {
                node->data.compound_stmt.statement_count = *((int*)(binary + pos));
                pos += 4;

                // 分配语句数组
                if (node->data.compound_stmt.statement_count > 0) {
                    node->data.compound_stmt.statements = (struct ASTNode**)malloc(
                        node->data.compound_stmt.statement_count * sizeof(struct ASTNode*));

                    if (!node->data.compound_stmt.statements) {
                        ast_free(node);
                        set_error("内存分配失败");
                        return NULL;
                    }

                    // 反序列化所有语句
                    for (int i = 0; i < node->data.compound_stmt.statement_count; i++) {
                        if (pos + 4 <= size) {
                            int stmt_size = *((int*)(binary + pos));
                            pos += 4;

                            if (stmt_size > 0 && pos + stmt_size <= size) {
                                node->data.compound_stmt.statements[i] = c2astc_deserialize(binary + pos, stmt_size);
                                pos += stmt_size;

                                if (!node->data.compound_stmt.statements[i]) {
                                    // 语句反序列化失败，清理已分配的内存
                                    for (int j = 0; j < i; j++) {
                                        ast_free(node->data.compound_stmt.statements[j]);
                                    }
                                    free(node->data.compound_stmt.statements);
                                    ast_free(node);
                                    return NULL;
                                }
                            } else {
                                node->data.compound_stmt.statements[i] = NULL;
                            }
                        } else {
                            node->data.compound_stmt.statements[i] = NULL;
                        }
                    }
                } else {
                    node->data.compound_stmt.statements = NULL;
                }
            }
            break;

        case ASTC_RETURN_STMT:
            // 反序列化return语句
            if (pos + 4 <= size) {
                int value_size = *((int*)(binary + pos));
                pos += 4;

                if (value_size > 0 && pos + value_size <= size) {
                    node->data.return_stmt.value = c2astc_deserialize(binary + pos, value_size);
                    pos += value_size;

                    if (!node->data.return_stmt.value) {
                        ast_free(node);
                        return NULL;
                    }
                } else {
                    node->data.return_stmt.value = NULL;
                }
            }
            break;

        case ASTC_VAR_DECL:
            // 反序列化变量声明
            if (pos + 4 <= size) {
                int name_len = *((int*)(binary + pos));
                pos += 4;

                if (name_len > 0 && pos + name_len <= size) {
                    node->data.var_decl.name = (char*)malloc(name_len);
                    if (!node->data.var_decl.name) {
                        ast_free(node);
                        set_error("内存分配失败");
                        return NULL;
                    }

                    memcpy(node->data.var_decl.name, binary + pos, name_len);
                    pos += name_len;
                } else {
                    node->data.var_decl.name = NULL;
                }
            }

            // 反序列化初始化表达式
            if (pos + 4 <= size) {
                int init_size = *((int*)(binary + pos));
                pos += 4;

                if (init_size > 0 && pos + init_size <= size) {
                    node->data.var_decl.initializer = c2astc_deserialize(binary + pos, init_size);
                    pos += init_size;

                    if (!node->data.var_decl.initializer) {
                        if (node->data.var_decl.name) {
                            free(node->data.var_decl.name);
                        }
                        ast_free(node);
                        return NULL;
                    }
                } else {
                    node->data.var_decl.initializer = NULL;
                }
            }

            // 初始化其他字段
            node->data.var_decl.type = NULL;
            break;

        case ASTC_IF_STMT:
            // 反序列化if语句
            // 反序列化条件表达式
            if (pos + 4 <= size) {
                int cond_size = *((int*)(binary + pos));
                pos += 4;

                if (cond_size > 0 && pos + cond_size <= size) {
                    node->data.if_stmt.condition = c2astc_deserialize(binary + pos, cond_size);
                    pos += cond_size;

                    if (!node->data.if_stmt.condition) {
                        ast_free(node);
                        return NULL;
                    }
                } else {
                    node->data.if_stmt.condition = NULL;
                }
            }

            // 反序列化then分支
            if (pos + 4 <= size) {
                int then_size = *((int*)(binary + pos));
                pos += 4;

                if (then_size > 0 && pos + then_size <= size) {
                    node->data.if_stmt.then_branch = c2astc_deserialize(binary + pos, then_size);
                    pos += then_size;

                    if (!node->data.if_stmt.then_branch) {
                        if (node->data.if_stmt.condition) {
                            ast_free(node->data.if_stmt.condition);
                        }
                        ast_free(node);
                        return NULL;
                    }
                } else {
                    node->data.if_stmt.then_branch = NULL;
                }
            }

            // 反序列化else分支
            if (pos + 4 <= size) {
                int else_size = *((int*)(binary + pos));
                pos += 4;

                if (else_size > 0 && pos + else_size <= size) {
                    node->data.if_stmt.else_branch = c2astc_deserialize(binary + pos, else_size);
                    pos += else_size;

                    if (!node->data.if_stmt.else_branch) {
                        if (node->data.if_stmt.condition) {
                            ast_free(node->data.if_stmt.condition);
                        }
                        if (node->data.if_stmt.then_branch) {
                            ast_free(node->data.if_stmt.then_branch);
                        }
                        ast_free(node);
                        return NULL;
                    }
                } else {
                    node->data.if_stmt.else_branch = NULL;
                }
            }
            break;

        // 其他节点类型的反序列化...
        // 完整实现需要处理所有节点类型

        default:
            // 对于其他类型，暂时只反序列化基本信息
            break;
    }
    
    return node;
}

unsigned char* c2astc(struct ASTNode *node, const C2AstcOptions *options, size_t *out_size) {
    if (!node || !out_size) {
        set_error("无效的参数");
        return NULL;
    }
    
    // 使用默认选项
    C2AstcOptions default_options = c2astc_default_options();
    if (!options) options = &default_options;
    
    // 创建一个最小的模块
    // 格式: 
    // - 魔数: \0asm (4字节)
    // - 版本: 01 00 00 00 (4字节，小端序表示版本1)
    *out_size = 8;
    unsigned char *rt = (unsigned char*)malloc(*out_size);
    if (!rt) {
        set_error("内存分配失败");
        return NULL;
    }
    
    rt[0] = 0x00;
    rt[1] = 0x61;
    rt[2] = 0x73;
    rt[3] = 0x6D;
    rt[4] = 0x01;
    rt[5] = 0x00;
    rt[6] = 0x00;
    rt[7] = 0x00;
    
    return rt;
}

// 调试函数：将节点打印为文本
void ast_print(struct ASTNode *node, int indent) {
    if (!node) return;
    
    for (int i = 0; i < indent; i++) printf("  ");
    
    // 根据节点类型打印信息
    switch (node->type) {
        case ASTC_TRANSLATION_UNIT:
            printf("TranslationUnit\n");
            for (int i = 0; i < node->data.translation_unit.declaration_count; i++) {
                ast_print(node->data.translation_unit.declarations[i], indent + 1);
            }
            break;
        case ASTC_EXPR_IDENTIFIER:
            printf("Identifier: %s\n", node->data.identifier.name);
            break;
        case ASTC_EXPR_CONSTANT:
            if (node->data.constant.type == ASTC_TYPE_INT) {
                printf("Constant: %lld\n", (long long)node->data.constant.int_val);
            } else {
                printf("Constant: %f\n", node->data.constant.float_val);
            }
            break;
        case ASTC_EXPR_STRING_LITERAL:
            printf("String: \"%s\"\n", node->data.string_literal.value);
            break;
        case ASTC_BINARY_OP:
            printf("BinaryOp: ");
            switch (node->data.binary_op.op) {
                case ASTC_OP_ADD: printf("+\n"); break;
                case ASTC_OP_SUB: printf("-\n"); break;
                case ASTC_OP_MUL: printf("*\n"); break;
                case ASTC_OP_DIV: printf("/\n"); break;
                case ASTC_OP_MOD: printf("%%\n"); break;
                case ASTC_OP_EQ: printf("==\n"); break;
                case ASTC_OP_NE: printf("!=\n"); break;
                case ASTC_OP_LT: printf("<\n"); break;
                case ASTC_OP_LE: printf("<=\n"); break;
                case ASTC_OP_GT: printf(">\n"); break;
                case ASTC_OP_GE: printf(">=\n"); break;
                case ASTC_OP_AND: printf("&\n"); break;
                case ASTC_OP_OR: printf("|\n"); break;
                case ASTC_OP_XOR: printf("^\n"); break;
                case ASTC_OP_LOGICAL_AND: printf("&&\n"); break;
                case ASTC_OP_LOGICAL_OR: printf("||\n"); break;
                case ASTC_OP_ASSIGN: printf("=\n"); break;
                default: printf("Unknown\n"); break;
            }
            ast_print(node->data.binary_op.left, indent + 1);
            ast_print(node->data.binary_op.right, indent + 1);
            break;
        case ASTC_UNARY_OP:
            printf("UnaryOp: ");
            switch (node->data.unary_op.op) {
                case ASTC_OP_NEG: printf("-\n"); break;
                case ASTC_OP_POS: printf("+\n"); break;
                case ASTC_OP_NOT: printf("!\n"); break;
                case ASTC_OP_BITWISE_NOT: printf("~\n"); break;
                default: printf("Unknown\n"); break;
            }
            ast_print(node->data.unary_op.operand, indent + 1);
            break;
        case ASTC_CALL_EXPR:
            printf("CallExpr\n");
            ast_print(node->data.call_expr.callee, indent + 1);
            for (int i = 0; i < node->data.call_expr.arg_count; i++) {
                ast_print(node->data.call_expr.args[i], indent + 1);
            }
            break;
        case ASTC_FUNC_DECL:
            printf("FunctionDecl: %s\n", node->data.func_decl.name);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("ReturnType:\n");
            ast_print(node->data.func_decl.return_type, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Parameters:\n");
            for (int i = 0; i < node->data.func_decl.param_count; i++) {
                ast_print(node->data.func_decl.params[i], indent + 2);
            }
            if (node->data.func_decl.has_body) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Body:\n");
                ast_print(node->data.func_decl.body, indent + 2);
            }
            break;
        case ASTC_VAR_DECL:
            printf("VarDecl: %s\n", node->data.var_decl.name);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Type:\n");
            ast_print(node->data.var_decl.type, indent + 2);
            if (node->data.var_decl.initializer) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Initializer:\n");
                ast_print(node->data.var_decl.initializer, indent + 2);
            }
            break;
        case ASTC_TYPE_SPECIFIER:
            printf("TypeSpecifier: ");
            switch (node->data.type_specifier.type) {
                case ASTC_TYPE_VOID: printf("void\n"); break;
                case ASTC_TYPE_CHAR: printf("char\n"); break;
                case ASTC_TYPE_SHORT: printf("short\n"); break;
                case ASTC_TYPE_INT: printf("int\n"); break;
                case ASTC_TYPE_LONG: printf("long\n"); break;
                case ASTC_TYPE_FLOAT: printf("float\n"); break;
                case ASTC_TYPE_DOUBLE: printf("double\n"); break;
                case ASTC_TYPE_SIGNED: printf("signed\n"); break;
                case ASTC_TYPE_UNSIGNED: printf("unsigned\n"); break;
                default: printf("Unknown\n"); break;
            }
            break;
        case ASTC_STRUCT_DECL:
            printf("StructDecl: %s\n", node->data.struct_decl.name ? node->data.struct_decl.name : "<anonymous>");
            for (int i = 0; i < node->data.struct_decl.member_count; i++) {
                for (int j = 0; j < indent + 1; j++) printf("  ");
                printf("Member %d:\n", i);
                ast_print(node->data.struct_decl.members[i], indent + 2);
            }
            break;
        case ASTC_UNION_DECL:
            printf("UnionDecl: %s\n", node->data.union_decl.name ? node->data.union_decl.name : "<anonymous>");
            for (int i = 0; i < node->data.union_decl.member_count; i++) {
                for (int j = 0; j < indent + 1; j++) printf("  ");
                printf("Member %d:\n", i);
                ast_print(node->data.union_decl.members[i], indent + 2);
            }
            break;
        case ASTC_ENUM_DECL:
            printf("EnumDecl: %s\n", node->data.enum_decl.name ? node->data.enum_decl.name : "<anonymous>");
            for (int i = 0; i < node->data.enum_decl.constant_count; i++) {
                ast_print(node->data.enum_decl.constants[i], indent + 1);
            }
            break;
        case ASTC_ENUM_CONSTANT:
            printf("EnumConstant: %s\n", node->data.enum_constant.name);
            if (node->data.enum_constant.has_value) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Value:\n");
                ast_print(node->data.enum_constant.value, indent + 2);
            }
            break;
        case ASTC_COMPOUND_STMT:
            printf("CompoundStmt\n");
            for (int i = 0; i < node->data.compound_stmt.statement_count; i++) {
                ast_print(node->data.compound_stmt.statements[i], indent + 1);
            }
            break;
        case ASTC_IF_STMT:
            printf("IfStmt\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Condition:\n");
            ast_print(node->data.if_stmt.condition, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Then:\n");
            ast_print(node->data.if_stmt.then_branch, indent + 2);
            if (node->data.if_stmt.else_branch) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Else:\n");
                ast_print(node->data.if_stmt.else_branch, indent + 2);
            }
            break;
        case ASTC_WHILE_STMT:
            printf("WhileStmt\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Condition:\n");
            ast_print(node->data.while_stmt.condition, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Body:\n");
            ast_print(node->data.while_stmt.body, indent + 2);
            break;
        case ASTC_FOR_STMT:
            printf("ForStmt\n");
            if (node->data.for_stmt.init) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Init:\n");
                ast_print(node->data.for_stmt.init, indent + 2);
            }
            if (node->data.for_stmt.condition) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Condition:\n");
                ast_print(node->data.for_stmt.condition, indent + 2);
            }
            if (node->data.for_stmt.increment) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Increment:\n");
                ast_print(node->data.for_stmt.increment, indent + 2);
            }
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Body:\n");
            ast_print(node->data.for_stmt.body, indent + 2);
            break;
        case ASTC_RETURN_STMT:
            printf("ReturnStmt\n");
            if (node->data.return_stmt.value) {
                ast_print(node->data.return_stmt.value, indent + 1);
            }
            break;
        case ASTC_BREAK_STMT:
            printf("BreakStmt\n");
            break;
        case ASTC_CONTINUE_STMT:
            printf("ContinueStmt\n");
            break;
        case ASTC_EXPR_STMT:
            printf("ExprStmt\n");
            ast_print(node->data.expr_stmt.expr, indent + 1);
            break;
        case ASTC_POINTER_TYPE:
            printf("PointerType (level: %d)\n", node->data.pointer_type.pointer_level);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("BaseType:\n");
            ast_print(node->data.pointer_type.base_type, indent + 2);
            break;
        case ASTC_ARRAY_TYPE:
            printf("ArrayType (dimensions: %d)\n", node->data.array_type.dimensions);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("ElementType:\n");
            ast_print(node->data.array_type.element_type, indent + 2);
            
            if (node->data.array_type.size_expr) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Size:\n");
                ast_print(node->data.array_type.size_expr, indent + 2);
            }
            
            if (node->data.array_type.dim_sizes) {
                for (int i = 0; i < node->data.array_type.dimensions; i++) {
                    if (node->data.array_type.dim_sizes[i]) {
                        for (int j = 0; j < indent + 1; j++) printf("  ");
                        printf("Dimension %d Size:\n", i);
                        ast_print(node->data.array_type.dim_sizes[i], indent + 2);
                    }
                }
            }
            break;
        case ASTC_FUNCTION_TYPE:
            printf("FunctionType (variadic: %s)\n", node->data.function_type.is_variadic ? "true" : "false");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("ReturnType:\n");
            ast_print(node->data.function_type.return_type, indent + 2);
            
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Parameters:\n");
            for (int i = 0; i < node->data.function_type.param_count; i++) {
                for (int j = 0; j < indent + 2; j++) printf("  ");
                printf("Parameter %d:\n", i);
                ast_print(node->data.function_type.param_types[i], indent + 3);
            }
            break;
        case ASTC_EXPR_ARRAY_SUBSCRIPT:
            printf("ArraySubscript\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Array:\n");
            ast_print(node->data.array_subscript.array, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Index:\n");
            ast_print(node->data.array_subscript.index, indent + 2);
            break;
        case ASTC_EXPR_MEMBER_ACCESS:
            printf("MemberAccess (member: %s)\n", node->data.member_access.member);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Object:\n");
            ast_print(node->data.member_access.object, indent + 2);
            break;
        case ASTC_EXPR_PTR_MEMBER_ACCESS:
            printf("PointerMemberAccess (member: %s)\n", node->data.ptr_member_access.member);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Pointer:\n");
            ast_print(node->data.ptr_member_access.pointer, indent + 2);
            break;
        default:
            printf("Node(type=%d)\n", node->type);
            break;
    }
} 

// 解析指针类型
static struct ASTNode* parse_pointer_type(Parser *parser, struct ASTNode *base_type) {
    int pointer_level = 0;
    
    // 计算指针层级
    while (match(parser, TOKEN_STAR)) {
        pointer_level++;
    }
    
    if (pointer_level == 0) {
        return base_type; // 不是指针类型，直接返回基本类型
    }
    
    // 创建指针类型节点
    struct ASTNode *pointer_type = ast_create_node(ASTC_POINTER_TYPE, base_type->line, base_type->column);
    if (!pointer_type) {
        ast_free(base_type);
        return NULL;
    }
    
    pointer_type->data.pointer_type.base_type = base_type;
    pointer_type->data.pointer_type.pointer_level = pointer_level;
    
    return pointer_type;
}

// 解析数组类型
static struct ASTNode* parse_array_type(Parser *parser, struct ASTNode *element_type) {
    // 检查是否为数组类型
    if (!match(parser, TOKEN_LBRACKET)) {
        return element_type; // 不是数组类型，直接返回元素类型
    }
    
    // 创建数组类型节点
    struct ASTNode *array_type = ast_create_node(ASTC_ARRAY_TYPE, element_type->line, element_type->column);
    if (!array_type) {
        ast_free(element_type);
        return NULL;
    }
    
    array_type->data.array_type.element_type = element_type;
    array_type->data.array_type.dimensions = 1;
    array_type->data.array_type.dim_sizes = NULL;
    
    // 解析数组大小表达式（如果有）
    if (!check(parser, TOKEN_RBRACKET)) {
        array_type->data.array_type.size_expr = parse_expression(parser);
        if (!array_type->data.array_type.size_expr) {
            ast_free(array_type);
            return NULL;
        }
    } else {
        // 未指定大小的数组
        array_type->data.array_type.size_expr = NULL;
    }
    
    if (!match(parser, TOKEN_RBRACKET)) {
        parser_error(parser, "预期右方括号");
        ast_free(array_type);
        return NULL;
    }
    
    // 处理多维数组
    if (check(parser, TOKEN_LBRACKET)) {
        // 递归解析多维数组
        struct ASTNode *multi_array = parse_array_type(parser, array_type);
        if (!multi_array) {
            return NULL;
        }
        
        // 更新维度信息
        if (multi_array->type == ASTC_ARRAY_TYPE) {
            multi_array->data.array_type.dimensions = array_type->data.array_type.dimensions + 1;
            
            // 分配维度大小数组
            multi_array->data.array_type.dim_sizes = (struct ASTNode **)malloc(
                multi_array->data.array_type.dimensions * sizeof(struct ASTNode *)
            );
            
            if (!multi_array->data.array_type.dim_sizes) {
                parser_error(parser, "内存分配失败");
                ast_free(multi_array);
                return NULL;
            }
            
            // 第一维度的大小
            multi_array->data.array_type.dim_sizes[0] = array_type->data.array_type.size_expr;
            
            // 复制其他维度的大小
            for (int i = 1; i < multi_array->data.array_type.dimensions; i++) {
                if (i < array_type->data.array_type.dimensions && array_type->data.array_type.dim_sizes) {
                    multi_array->data.array_type.dim_sizes[i] = array_type->data.array_type.dim_sizes[i-1];
                } else {
                    multi_array->data.array_type.dim_sizes[i] = NULL;
                }
            }
        }
        
        return multi_array;
    }
    
    return array_type;
}

// 解析函数指针类型
static struct ASTNode* parse_function_type(Parser *parser, struct ASTNode *return_type) {
    // 检查是否为函数指针类型
    if (!match(parser, TOKEN_LPAREN)) {
        return return_type; // 不是函数指针类型，直接返回返回类型
    }
    
    // 检查是否是函数指针声明（如 int (*func)(int, int)）
    if (match(parser, TOKEN_STAR)) {
        // 跳过标识符，因为我们只关心类型
        if (check(parser, TOKEN_IDENTIFIER)) {
            advance(parser);
        }
        
        if (!match(parser, TOKEN_RPAREN)) {
            parser_error(parser, "预期右括号");
            ast_free(return_type);
            return NULL;
        }
        
        if (!match(parser, TOKEN_LPAREN)) {
            parser_error(parser, "预期左括号");
            ast_free(return_type);
            return NULL;
        }
    } else {
        // 这可能是普通函数声明，而不是函数指针类型
        parser_error(parser, "预期星号(*)表示函数指针");
        ast_free(return_type);
        return NULL;
    }
    
    // 创建函数指针类型节点
    struct ASTNode *function_type = ast_create_node(ASTC_FUNCTION_TYPE, return_type->line, return_type->column);
    if (!function_type) {
        ast_free(return_type);
        return NULL;
    }
    
    function_type->data.function_type.return_type = return_type;
    function_type->data.function_type.param_types = NULL;
    function_type->data.function_type.param_count = 0;
    function_type->data.function_type.is_variadic = 0;
    
    // 解析参数类型列表
    if (!check(parser, TOKEN_RPAREN)) {
        // 有参数
        struct ASTNode **param_types = NULL;
        int param_count = 0;
        int capacity = 0;
        
        do {
            // 检查是否为可变参数
            if (match(parser, TOKEN_DOT)) {
                if (!match(parser, TOKEN_DOT) || !match(parser, TOKEN_DOT)) {
                    parser_error(parser, "预期省略号(...)");
                    // 释放已分配的资源
                    for (int i = 0; i < param_count; i++) {
                        ast_free(param_types[i]);
                    }
                    free(param_types);
                    ast_free(function_type);
                    return NULL;
                }
                
                function_type->data.function_type.is_variadic = 1;
                break;
            }
            
            // 解析参数类型
            struct ASTNode *param_type = parse_declaration(parser);
            if (!param_type) {
                // 释放已分配的资源
                for (int i = 0; i < param_count; i++) {
                    ast_free(param_types[i]);
                }
                free(param_types);
                ast_free(function_type);
                return NULL;
            }
            
            // 动态扩展参数类型数组
            if (param_count >= capacity) {
                capacity = capacity == 0 ? 4 : capacity * 2;
                struct ASTNode **new_param_types = (struct ASTNode **)realloc(param_types, capacity * sizeof(struct ASTNode *));
                if (!new_param_types) {
                    parser_error(parser, "内存分配失败");
                    // 释放已分配的资源
                    for (int i = 0; i < param_count; i++) {
                        ast_free(param_types[i]);
                    }
                    free(param_types);
                    ast_free(function_type);
                    ast_free(param_type);
                    return NULL;
                }
                param_types = new_param_types;
            }
            
            param_types[param_count++] = param_type;
        } while (match(parser, TOKEN_COMMA));
        
        function_type->data.function_type.param_types = param_types;
        function_type->data.function_type.param_count = param_count;
    }
    
    if (!match(parser, TOKEN_RPAREN)) {
        parser_error(parser, "预期右括号");
        // 释放已分配的资源
        for (int i = 0; i < function_type->data.function_type.param_count; i++) {
            ast_free(function_type->data.function_type.param_types[i]);
        }
        free(function_type->data.function_type.param_types);
        ast_free(function_type);
        return NULL;
    }
    
    return function_type;
}

// 解析基本表达式
static struct ASTNode* parse_primary(Parser *parser) {
    Token *token = peek(parser);
    if (!token) return NULL;
    
    struct ASTNode *node = NULL;
    
    switch (token->type) {
        case TOKEN_IDENTIFIER: {
            // 标识符
            char *name = strdup(token->value);
            if (!name) {
                parser_error(parser, "内存分配失败");
                return NULL;
            }
            
            node = create_identifier_node(name, token->line, token->column);
            if (!node) {
                free(name);
                return NULL;
            }
            
            advance(parser); // 消耗标识符
            break;
        }
        
        case TOKEN_NUMBER: {
            // 数字常量
            node = create_number_node(token->value, token->line, token->column);
            if (!node) {
                return NULL;
            }
            
            advance(parser); // 消耗数字
            break;
        }
        
        case TOKEN_STRING_LITERAL: {
            // 字符串字面量
            node = create_string_node(token->value, token->line, token->column);
            if (!node) {
                return NULL;
            }
            
            advance(parser); // 消耗字符串
            break;
        }
        
        case TOKEN_LPAREN: {
            // 括号表达式
            advance(parser); // 消耗左括号
            
            node = parse_expression(parser);
            if (!node) {
                return NULL;
            }
            
            if (!match(parser, TOKEN_RPAREN)) {
                parser_error(parser, "预期右括号");
                ast_free(node);
                return NULL;
            }
            
            break;
        }
        
        default:
            parser_error(parser, "预期表达式");
            return NULL;
    }
    
    return node;
}

// 解析后缀表达式
static struct ASTNode* parse_postfix(Parser *parser) {
    struct ASTNode *expr = parse_primary(parser);
    if (!expr) return NULL;
    
    while (1) {
        Token *token = peek(parser);
        if (!token) break;
        
        if (token->type == TOKEN_LBRACKET) {
            // 数组访问
            advance(parser); // 消耗左方括号
            
            // 解析索引表达式
            struct ASTNode *index = parse_expression(parser);
            if (!index) {
                ast_free(expr);
                return NULL;
            }
            
            if (!match(parser, TOKEN_RBRACKET)) {
                parser_error(parser, "预期右方括号");
                ast_free(expr);
                ast_free(index);
                return NULL;
            }
            
            // 创建数组访问节点
            struct ASTNode *array_access = ast_create_node(ASTC_EXPR_ARRAY_SUBSCRIPT, expr->line, expr->column);
            if (!array_access) {
                ast_free(expr);
                ast_free(index);
                return NULL;
            }
            
            array_access->data.array_subscript.array = expr;
            array_access->data.array_subscript.index = index;
            
            expr = array_access;
        } else if (token->type == TOKEN_DOT) {
            // 成员访问
            advance(parser); // 消耗点号
            
            // 获取成员名
            if (!check(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "预期成员名");
                ast_free(expr);
                return NULL;
            }
            
            token = advance(parser);
            char *member_name = strdup(token->value);
            if (!member_name) {
                parser_error(parser, "内存分配失败");
                ast_free(expr);
                return NULL;
            }
            
            // 创建成员访问节点
            struct ASTNode *member_access = ast_create_node(ASTC_EXPR_MEMBER_ACCESS, expr->line, expr->column);
            if (!member_access) {
                free(member_name);
                ast_free(expr);
                return NULL;
            }
            
            member_access->data.member_access.object = expr;
            member_access->data.member_access.member = member_name;
            
            expr = member_access;
        } else if (token->type == TOKEN_ARROW) {
            // 指针成员访问
            advance(parser); // 消耗箭头
            
            // 获取成员名
            if (!check(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "预期成员名");
                ast_free(expr);
                return NULL;
            }
            
            token = advance(parser);
            char *member_name = strdup(token->value);
            if (!member_name) {
                parser_error(parser, "内存分配失败");
                ast_free(expr);
                return NULL;
            }
            
            // 创建指针成员访问节点
            struct ASTNode *ptr_member_access = ast_create_node(ASTC_EXPR_PTR_MEMBER_ACCESS, expr->line, expr->column);
            if (!ptr_member_access) {
                free(member_name);
                ast_free(expr);
                return NULL;
            }
            
            ptr_member_access->data.ptr_member_access.pointer = expr;
            ptr_member_access->data.ptr_member_access.member = member_name;
            
            expr = ptr_member_access;
        } else if (token->type == TOKEN_LPAREN) {
            // 函数调用
            advance(parser); // 消耗左括号
            
            // 解析参数列表
            struct ASTNode **args = NULL;
            int arg_count = 0;
            int capacity = 0;
            
            if (!check(parser, TOKEN_RPAREN)) {
                do {
                    struct ASTNode *arg = parse_expression(parser);
                    if (!arg) {
                        // 释放已分配的资源
                        for (int i = 0; i < arg_count; i++) {
                            ast_free(args[i]);
                        }
                        free(args);
                        ast_free(expr);
                        return NULL;
                    }
                    
                    // 动态扩展参数数组
                    if (arg_count >= capacity) {
                        capacity = capacity == 0 ? 4 : capacity * 2;
                        struct ASTNode **new_args = (struct ASTNode **)realloc(args, capacity * sizeof(struct ASTNode *));
                        if (!new_args) {
                            parser_error(parser, "内存分配失败");
                            // 释放已分配的资源
                            for (int i = 0; i < arg_count; i++) {
                                ast_free(args[i]);
                            }
                            free(args);
                            ast_free(expr);
                            ast_free(arg);
                            return NULL;
                        }
                        args = new_args;
                    }
                    
                    args[arg_count++] = arg;
                } while (match(parser, TOKEN_COMMA));
            }
            
            if (!match(parser, TOKEN_RPAREN)) {
                parser_error(parser, "预期右括号");
                // 释放已分配的资源
                for (int i = 0; i < arg_count; i++) {
                    ast_free(args[i]);
                }
                free(args);
                ast_free(expr);
                return NULL;
            }
            
            // 创建函数调用节点
            struct ASTNode *call = create_call_expr(expr, args, arg_count, expr->line, expr->column);
            if (!call) {
                // 释放已分配的资源
                for (int i = 0; i < arg_count; i++) {
                    ast_free(args[i]);
                }
                free(args);
                ast_free(expr);
                return NULL;
            }
            
            expr = call;
        } else {
            break;
        }
    }
    
    return expr;
}

// 解析一元表达式
static struct ASTNode* parse_unary(Parser *parser) {
    Token *token = peek(parser);
    if (!token) return NULL;
    
    struct ASTNode *node = NULL;
    
    switch (token->type) {
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_BANG:
        case TOKEN_TILDE:
        case TOKEN_STAR:
        case TOKEN_AMPERSAND: {
            // 一元操作符
            ASTNodeType op;
            switch (token->type) {
                case TOKEN_PLUS: op = ASTC_OP_POS; break;
                case TOKEN_MINUS: op = ASTC_OP_NEG; break;
                case TOKEN_BANG: op = ASTC_OP_NOT; break;
                case TOKEN_TILDE: op = ASTC_OP_BITWISE_NOT; break;
                case TOKEN_STAR: op = ASTC_OP_DEREF; break;
                case TOKEN_AMPERSAND: op = ASTC_OP_ADDR; break;
                default: op = ASTC_OP_UNKNOWN; break;
            }
            
            int line = token->line;
            int column = token->column;
            
            advance(parser); // 消耗操作符
            
            struct ASTNode *operand = parse_unary(parser);
            if (!operand) {
                return NULL;
            }
            
            node = create_unary_expr(op, operand, line, column);
            break;
        }
        
        default:
            // 不是一元操作符，解析后缀表达式
            node = parse_postfix(parser);
            break;
    }
    
    return node;
}

// 解析乘法表达式
static struct ASTNode* parse_multiplicative(Parser *parser) {
    struct ASTNode *left = parse_unary(parser);
    if (!left) return NULL;
    
    while (1) {
        Token *token = peek(parser);
        if (!token) break;
        
        ASTNodeType op;
        switch (token->type) {
            case TOKEN_STAR: op = ASTC_OP_MUL; break;
            case TOKEN_SLASH: op = ASTC_OP_DIV; break;
            case TOKEN_PERCENT: op = ASTC_OP_MOD; break;
            default: return left;
        }
        
        int line = token->line;
        int column = token->column;
        
        advance(parser); // 消耗操作符
        
        struct ASTNode *right = parse_unary(parser);
        if (!right) {
            ast_free(left);
            return NULL;
        }
        
        struct ASTNode *binary = create_binary_expr(op, left, right, line, column);
        if (!binary) {
            ast_free(left);
            ast_free(right);
            return NULL;
        }
        
        left = binary;
    }
    
    return left;
}

// 解析加法表达式
static struct ASTNode* parse_additive(Parser *parser) {
    struct ASTNode *left = parse_multiplicative(parser);
    if (!left) return NULL;
    
    while (1) {
        Token *token = peek(parser);
        if (!token) break;
        
        ASTNodeType op;
        switch (token->type) {
            case TOKEN_PLUS: op = ASTC_OP_ADD; break;
            case TOKEN_MINUS: op = ASTC_OP_SUB; break;
            default: return left;
        }
        
        int line = token->line;
        int column = token->column;
        
        advance(parser); // 消耗操作符
        
        struct ASTNode *right = parse_multiplicative(parser);
        if (!right) {
            ast_free(left);
            return NULL;
        }
        
        struct ASTNode *binary = create_binary_expr(op, left, right, line, column);
        if (!binary) {
            ast_free(left);
            ast_free(right);
            return NULL;
        }
        
        left = binary;
    }
    
    return left;
}

// 解析关系表达式
static struct ASTNode* parse_relational(Parser *parser) {
    struct ASTNode *left = parse_additive(parser);
    if (!left) return NULL;
    
    while (1) {
        Token *token = peek(parser);
        if (!token) break;
        
        ASTNodeType op;
        switch (token->type) {
            case TOKEN_LT: op = ASTC_OP_LT; break;
            case TOKEN_LE: op = ASTC_OP_LE; break;
            case TOKEN_GT: op = ASTC_OP_GT; break;
            case TOKEN_GE: op = ASTC_OP_GE; break;
            default: return left;
        }
        
        int line = token->line;
        int column = token->column;
        
        advance(parser); // 消耗操作符
        
        struct ASTNode *right = parse_additive(parser);
        if (!right) {
            ast_free(left);
            return NULL;
        }
        
        struct ASTNode *binary = create_binary_expr(op, left, right, line, column);
        if (!binary) {
            ast_free(left);
            ast_free(right);
            return NULL;
        }
        
        left = binary;
    }
    
    return left;
}

// 解析相等表达式
static struct ASTNode* parse_equality(Parser *parser) {
    struct ASTNode *left = parse_relational(parser);
    if (!left) return NULL;
    
    while (1) {
        Token *token = peek(parser);
        if (!token) break;
        
        ASTNodeType op;
        switch (token->type) {
            case TOKEN_EQ: op = ASTC_OP_EQ; break;
            case TOKEN_NE: op = ASTC_OP_NE; break;
            default: return left;
        }
        
        int line = token->line;
        int column = token->column;
        
        advance(parser); // 消耗操作符
        
        struct ASTNode *right = parse_relational(parser);
        if (!right) {
            ast_free(left);
            return NULL;
        }
        
        struct ASTNode *binary = create_binary_expr(op, left, right, line, column);
        if (!binary) {
            ast_free(left);
            ast_free(right);
            return NULL;
        }
        
        left = binary;
    }
    
    return left;
}

// 解析逻辑与表达式
static struct ASTNode* parse_logical_and(Parser *parser) {
    struct ASTNode *left = parse_equality(parser);
    if (!left) return NULL;
    
    while (match(parser, TOKEN_LOGICAL_AND)) {
        int line = peek(parser)->line;
        int column = peek(parser)->column;
        
        struct ASTNode *right = parse_equality(parser);
        if (!right) {
            ast_free(left);
            return NULL;
        }
        
        struct ASTNode *binary = create_binary_expr(ASTC_OP_LOGICAL_AND, left, right, line, column);
        if (!binary) {
            ast_free(left);
            ast_free(right);
            return NULL;
        }
        
        left = binary;
    }
    
    return left;
}

// 解析逻辑或表达式
static struct ASTNode* parse_logical_or(Parser *parser) {
    struct ASTNode *left = parse_logical_and(parser);
    if (!left) return NULL;
    
    while (match(parser, TOKEN_LOGICAL_OR)) {
        int line = peek(parser)->line;
        int column = peek(parser)->column;
        
        struct ASTNode *right = parse_logical_and(parser);
        if (!right) {
            ast_free(left);
            return NULL;
        }
        
        struct ASTNode *binary = create_binary_expr(ASTC_OP_LOGICAL_OR, left, right, line, column);
        if (!binary) {
            ast_free(left);
            ast_free(right);
            return NULL;
        }
        
        left = binary;
    }
    
    return left;
}

// 解析赋值表达式
static struct ASTNode* parse_assignment(Parser *parser) {
    struct ASTNode *left = parse_logical_or(parser);
    if (!left) return NULL;
    
    Token *token = peek(parser);
    if (token && token->type == TOKEN_ASSIGN) {
        int line = token->line;
        int column = token->column;
        
        advance(parser); // 消耗等号
        
        struct ASTNode *right = parse_assignment(parser);
        if (!right) {
            ast_free(left);
            return NULL;
        }
        
        struct ASTNode *assign = create_binary_expr(ASTC_OP_ASSIGN, left, right, line, column);
        if (!assign) {
            ast_free(left);
            ast_free(right);
            return NULL;
        }
        
        return assign;
    }
    
    return left;
}

// 解析表达式
static struct ASTNode* parse_expression(Parser *parser) {
    return parse_assignment(parser);
}

// 创建一元操作节点
struct ASTNode* create_unary_op_node(int op, struct ASTNode *operand, int line, int column) {
    struct ASTNode *node = ast_create_node(ASTC_UNARY_OP, line, column);
    if (node) {
        node->data.unary_op.op = op;
        node->data.unary_op.operand = operand;
    }
    return node;
}


