/**
 * evolver0.c - 第零代自举编译器
 * 目标：能够编译自身的最小C编译器
 * 基于evolver0_integrated.c改进
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>

// ====================================
// 基础定义
// ====================================

#define MAX_TOKENS 100000
#define MAX_CODE_SIZE 1048576  // 1MB

// Token类型定义
typedef enum {
    TOKEN_EOF = 0,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_CHAR_LITERAL,
    
    // 关键字
    TOKEN_INT, TOKEN_CHAR, TOKEN_VOID, TOKEN_RETURN, 
    TOKEN_IF, TOKEN_ELSE, TOKEN_WHILE, TOKEN_FOR, TOKEN_DO, 
    TOKEN_BREAK, TOKEN_CONTINUE, TOKEN_STRUCT, TOKEN_TYPEDEF,
    TOKEN_STATIC, TOKEN_EXTERN, TOKEN_CONST, TOKEN_SIZEOF,
    TOKEN_GOTO, TOKEN_SWITCH, TOKEN_CASE, TOKEN_DEFAULT,
    
    // 操作符
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULTIPLY, TOKEN_DIVIDE, TOKEN_MOD,
    TOKEN_ASSIGN, TOKEN_EQUAL, TOKEN_NOT_EQUAL,
    TOKEN_LESS, TOKEN_GREATER, TOKEN_LESS_EQUAL, TOKEN_GREATER_EQUAL,
    TOKEN_LOGICAL_AND, TOKEN_LOGICAL_OR, TOKEN_LOGICAL_NOT,
    TOKEN_BIT_AND, TOKEN_BIT_OR, TOKEN_BIT_XOR, TOKEN_BIT_NOT,
    TOKEN_LEFT_SHIFT, TOKEN_RIGHT_SHIFT,
    TOKEN_INCREMENT, TOKEN_DECREMENT,
    TOKEN_ARROW, TOKEN_DOT,
    TOKEN_PLUS_ASSIGN, TOKEN_MINUS_ASSIGN, TOKEN_MUL_ASSIGN, TOKEN_DIV_ASSIGN,
    TOKEN_MOD_ASSIGN, TOKEN_ADD_ASSIGN, TOKEN_SUB_ASSIGN,
    
    // 标点
    TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_LBRACE, TOKEN_RBRACE,
    TOKEN_LBRACKET, TOKEN_RBRACKET, TOKEN_SEMICOLON, TOKEN_COMMA,
    TOKEN_COLON, TOKEN_QUESTION,
    
    // 预处理
    TOKEN_HASH, TOKEN_INCLUDE, TOKEN_DEFINE, TOKEN_IFDEF, TOKEN_IFNDEF, TOKEN_ENDIF,
    
    TOKEN_UNKNOWN
} TokenType;

// Token结构
typedef struct {
    TokenType type;
    char *value;
    int line;
    int column;
    const char *filename;
} Token;

// 类型种类
typedef enum {
    TYPE_VOID,
    TYPE_CHAR,
    TYPE_SHORT,
    TYPE_INT,
    TYPE_LONG,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_POINTER,
    TYPE_ARRAY,
    TYPE_FUNCTION,
    TYPE_STRUCT,
    TYPE_UNION,
    TYPE_ENUM
} TypeKind;

// 类型信息
typedef struct TypeInfo {
    TypeKind kind;
    int size;
    int alignment;
    int is_signed;
    int is_const;
    int is_volatile;
    
    union {
        struct {
            struct TypeInfo *pointee;
        } pointer;
        
        struct {
            struct TypeInfo *element;
            int size;
            int is_vla;  // 变长数组
        } array;
        
        struct {
            struct TypeInfo *return_type;
            struct TypeInfo **param_types;
            int param_count;
            int is_variadic;
        } function;
        
        struct {
            char *name;
            struct {
                char *name;
                struct TypeInfo *type;
                int offset;
            } *members;
            int member_count;
        } struct_type;
    } data;
} TypeInfo;

// 操作符类型
typedef enum {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_LT, OP_GT, OP_LE, OP_GE, OP_EQ, OP_NE,
    OP_AND, OP_OR, OP_NOT,
    OP_BIT_AND, OP_BIT_OR, OP_BIT_XOR, OP_BIT_NOT,
    OP_LEFT_SHIFT, OP_RIGHT_SHIFT,
    OP_ASSIGN, OP_ADD_ASSIGN, OP_SUB_ASSIGN, OP_MUL_ASSIGN, OP_DIV_ASSIGN, OP_MOD_ASSIGN,
    OP_PRE_INC, OP_PRE_DEC, OP_POST_INC, OP_POST_DEC,
    OP_ADDR, OP_DEREF,
    OP_PLUS, OP_MINUS
} OperatorType;

// AST节点类型
typedef enum {
    // 基础节点
    AST_TRANSLATION_UNIT,
    AST_FUNCTION_DEF,
    AST_FUNCTION_DECL,
    AST_PARAM_DECL,
    AST_VAR_DECL,
    AST_TYPE_NAME,
    
    // 语句
    AST_COMPOUND_STMT,
    AST_EXPRESSION_STMT,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_DO_WHILE_STMT,
    AST_FOR_STMT,
    AST_RETURN_STMT,
    AST_BREAK_STMT,
    AST_CONTINUE_STMT,
    AST_GOTO_STMT,
    AST_LABEL_STMT,
    
    // 表达式
    AST_INTEGER_LITERAL,
    AST_STRING_LITERAL,
    AST_CHAR_LITERAL,
    AST_IDENTIFIER,
    AST_BINARY_EXPR,
    AST_UNARY_EXPR,
    AST_ASSIGNMENT_EXPR,
    AST_CALL_EXPR,
    AST_ARRAY_SUBSCRIPT_EXPR,
    AST_MEMBER_EXPR,
    AST_POST_INCREMENT_EXPR,
    AST_POST_DECREMENT_EXPR,
    AST_CAST_EXPR,
    AST_SIZEOF_EXPR,
    AST_CONDITIONAL_EXPR,
    
    // 旧的兼容类型
    AST_PROGRAM,
    AST_FUNCTION,
    AST_PARAMETER,
    AST_RETURN,
    AST_INTEGER,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_COMPOUND,
    AST_DECLARATION,
    AST_ASSIGNMENT,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_CALL,
    AST_ARRAY_ACCESS,
    AST_CAST,
    AST_SIZEOF,
    AST_TYPE,
    AST_BREAK,
    AST_CONTINUE
} ASTNodeType;

// AST节点结构
typedef struct ASTNode {
    ASTNodeType type;
    int line;
    int column;
    const char *filename;
    struct ASTNode *next;  // 用于链接多个声明
    
    // 类型信息
    TypeInfo *type_info;
    
    // 值
    union {
        long long int_val;
        double float_val;
        char *str_val;
    } value;
    
    // 子节点数据
    union {
        // 通用子节点
        struct {
            struct ASTNode **children;
            int child_count;
            int child_capacity;
        } generic;
        
        // 标识符
        struct {
            char *name;
            struct ASTNode *symbol;  // 符号表引用
        } identifier;
        
        // 二元表达式
        struct {
            struct ASTNode *left;
            struct ASTNode *right;
            OperatorType op;
        } binary;
        
        // 一元表达式
        struct {
            struct ASTNode *operand;
            OperatorType op;
        } unary;
        
        // 赋值表达式
        struct {
            struct ASTNode *left;
            struct ASTNode *right;
            OperatorType op;
        } assignment;
        
        // 函数定义/声明
        struct {
            char *name;
            TypeInfo *type;
            struct ASTNode **params;
            int param_count;
            struct ASTNode *body;
            int is_definition;
        } function;
        
        // 变量声明
        struct {
            char *name;
            struct ASTNode *init;
        } var_decl;
        
        // if语句
        struct {
            struct ASTNode *condition;
            struct ASTNode *then_stmt;
            struct ASTNode *else_stmt;
        } if_stmt;
        
        // while/do-while语句
        struct {
            struct ASTNode *condition;
            struct ASTNode *body;
            int is_do_while;
        } while_stmt;
        
        // for语句
        struct {
            struct ASTNode *init;
            struct ASTNode *condition;
            struct ASTNode *increment;
            struct ASTNode *body;
        } for_stmt;
        
        // return语句
        struct {
            struct ASTNode *value;
        } return_stmt;
        
        // goto/label语句
        struct {
            char *label;
            struct ASTNode *stmt;  // for label
        } goto_label;
        
        // 函数调用
        struct {
            struct ASTNode *function;
            struct ASTNode **args;
            int arg_count;
        } call;
        
        // 数组下标
        struct {
            struct ASTNode *array;
            struct ASTNode *index;
        } array_sub;
        
        // 成员访问
        struct {
            struct ASTNode *object;
            char *member;
            int is_arrow;
        } member;
        
        // 旧的兼容结构
        // 函数（旧）
        struct {
            char *name;
            char *return_type;
            struct ASTNode **params;
            int param_count;
            struct ASTNode *body;
        } old_function;
        
        // 参数（旧）
        struct {
            char *type;
            char *name;
        } param;
        
        // 返回语句（旧）
        struct {
            struct ASTNode *value;
        } ret;
        
        // 复合语句（旧）
        struct {
            struct ASTNode **statements;
            int count;
            int capacity;
        } compound;
        
        // 声明（旧）
        struct {
            char *type;
            char *name;
            struct ASTNode *init;
            int is_array;
            int array_size;
        } decl;
        
        // 赋值（旧）
        struct {
            struct ASTNode *target;
            struct ASTNode *value;
        } assign;
        
        // 函数调用（旧）
        struct {
            char *name;
            struct ASTNode **args;
            int arg_count;
        } old_call;
        
        // 数组访问（旧）
        struct {
            struct ASTNode *array;
            struct ASTNode *index;
        } array_access;
        
        // 类型转换（旧）
        struct {
            char *target_type;
            struct ASTNode *expr;
        } cast;
        
        // sizeof（旧）
        struct {
            char *type_name;
            struct ASTNode *expr;
        } sizeof_expr;
        
        // 类型（旧）
        struct {
            char *base_type;
            int pointer_level;
            int is_array;
            int array_size;
        } old_type;
    } data;
} ASTNode;

// ====================================
// 全局变量
// ====================================

typedef struct {
    const char *input_file;
    const char *output_file;
    bool verbose;
    bool dump_ast;
    bool dump_asm;
} CompilerOptions;

// ====================================
// 词法分析器
// ====================================

typedef struct {
    const char *source;
    size_t pos;
    size_t length;
    int line;
    int column;
    const char *filename;
} Lexer;

static void lexer_init(Lexer *lexer, const char *source, const char *filename) {
    lexer->source = source;
    lexer->pos = 0;
    lexer->length = strlen(source);
    lexer->line = 1;
    lexer->column = 1;
    lexer->filename = filename;
}

static void lexer_skip_whitespace(Lexer *lexer) {
    while (lexer->pos < lexer->length) {
        char c = lexer->source[lexer->pos];
        if (c == ' ' || c == '\t' || c == '\r') {
            lexer->pos++;
            lexer->column++;
        } else if (c == '\n') {
            lexer->pos++;
            lexer->line++;
            lexer->column = 1;
        } else if (c == '/' && lexer->pos + 1 < lexer->length) {
            if (lexer->source[lexer->pos + 1] == '/') {
                // 单行注释
                lexer->pos += 2;
                while (lexer->pos < lexer->length && lexer->source[lexer->pos] != '\n') {
                    lexer->pos++;
                }
            } else if (lexer->source[lexer->pos + 1] == '*') {
                // 多行注释
                lexer->pos += 2;
                while (lexer->pos + 1 < lexer->length) {
                    if (lexer->source[lexer->pos] == '*' && lexer->source[lexer->pos + 1] == '/') {
                        lexer->pos += 2;
                        break;
                    }
                    if (lexer->source[lexer->pos] == '\n') {
                        lexer->line++;
                        lexer->column = 1;
                    } else {
                        lexer->column++;
                    }
                    lexer->pos++;
                }
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

static Token* lexer_tokenize(const char *source, const char *filename, int *token_count) {
    Lexer lexer;
    lexer_init(&lexer, source, filename);
    
    Token *tokens = malloc(sizeof(Token) * MAX_TOKENS);
    int count = 0;
    
    while (lexer.pos < lexer.length && count < MAX_TOKENS - 1) {
        lexer_skip_whitespace(&lexer);
        if (lexer.pos >= lexer.length) break;
        
        Token *token = &tokens[count];
        token->line = lexer.line;
        token->column = lexer.column;
        token->filename = filename;
        
        char c = lexer.source[lexer.pos];
        
        // 标识符或关键字
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
            size_t start = lexer.pos;
            while (lexer.pos < lexer.length) {
                c = lexer.source[lexer.pos];
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
                    (c >= '0' && c <= '9') || c == '_') {
                    lexer.pos++;
                    lexer.column++;
                } else {
                    break;
                }
            }
            
            size_t len = lexer.pos - start;
            char *value = malloc(len + 1);
            memcpy(value, lexer.source + start, len);
            value[len] = '\0';
            
            // 检查关键字
            if (strcmp(value, "int") == 0) token->type = TOKEN_INT;
            else if (strcmp(value, "char") == 0) token->type = TOKEN_CHAR;
            else if (strcmp(value, "void") == 0) token->type = TOKEN_VOID;
            else if (strcmp(value, "return") == 0) token->type = TOKEN_RETURN;
            else if (strcmp(value, "if") == 0) token->type = TOKEN_IF;
            else if (strcmp(value, "else") == 0) token->type = TOKEN_ELSE;
            else if (strcmp(value, "while") == 0) token->type = TOKEN_WHILE;
            else if (strcmp(value, "for") == 0) token->type = TOKEN_FOR;
            else if (strcmp(value, "do") == 0) token->type = TOKEN_DO;
            else if (strcmp(value, "break") == 0) token->type = TOKEN_BREAK;
            else if (strcmp(value, "continue") == 0) token->type = TOKEN_CONTINUE;
            else if (strcmp(value, "struct") == 0) token->type = TOKEN_STRUCT;
            else if (strcmp(value, "typedef") == 0) token->type = TOKEN_TYPEDEF;
            else if (strcmp(value, "static") == 0) token->type = TOKEN_STATIC;
            else if (strcmp(value, "extern") == 0) token->type = TOKEN_EXTERN;
            else if (strcmp(value, "const") == 0) token->type = TOKEN_CONST;
            else if (strcmp(value, "sizeof") == 0) token->type = TOKEN_SIZEOF;
            else if (strcmp(value, "goto") == 0) token->type = TOKEN_GOTO;
            else if (strcmp(value, "switch") == 0) token->type = TOKEN_SWITCH;
            else if (strcmp(value, "case") == 0) token->type = TOKEN_CASE;
            else if (strcmp(value, "default") == 0) token->type = TOKEN_DEFAULT;
            else token->type = TOKEN_IDENTIFIER;
            
            token->value = value;
            count++;
        }
        // 数字
        else if (c >= '0' && c <= '9') {
            size_t start = lexer.pos;
            
            // 十六进制
            if (c == '0' && lexer.pos + 1 < lexer.length && 
                (lexer.source[lexer.pos + 1] == 'x' || lexer.source[lexer.pos + 1] == 'X')) {
                lexer.pos += 2;
                lexer.column += 2;
                while (lexer.pos < lexer.length && 
                       ((lexer.source[lexer.pos] >= '0' && lexer.source[lexer.pos] <= '9') ||
                        (lexer.source[lexer.pos] >= 'a' && lexer.source[lexer.pos] <= 'f') ||
                        (lexer.source[lexer.pos] >= 'A' && lexer.source[lexer.pos] <= 'F'))) {
                    lexer.pos++;
                    lexer.column++;
                }
            } else {
                // 十进制
                while (lexer.pos < lexer.length && lexer.source[lexer.pos] >= '0' && lexer.source[lexer.pos] <= '9') {
                    lexer.pos++;
                    lexer.column++;
                }
            }
            
            size_t len = lexer.pos - start;
            char *value = malloc(len + 1);
            memcpy(value, lexer.source + start, len);
            value[len] = '\0';
            
            token->type = TOKEN_NUMBER;
            token->value = value;
            count++;
        }
        // 字符串
        else if (c == '"') {
            lexer.pos++; // 跳过开始的引号
            lexer.column++;
            size_t start = lexer.pos;
            
            while (lexer.pos < lexer.length && lexer.source[lexer.pos] != '"') {
                if (lexer.source[lexer.pos] == '\\' && lexer.pos + 1 < lexer.length) {
                    lexer.pos += 2; // 跳过转义字符
                    lexer.column += 2;
                } else {
                    if (lexer.source[lexer.pos] == '\n') {
                        lexer.line++;
                        lexer.column = 1;
                    } else {
                        lexer.column++;
                    }
                    lexer.pos++;
                }
            }
            
            size_t len = lexer.pos - start;
            char *value = malloc(len + 1);
            memcpy(value, lexer.source + start, len);
            value[len] = '\0';
            
            if (lexer.pos < lexer.length) {
                lexer.pos++; // 跳过结束的引号
                lexer.column++;
            }
            
            token->type = TOKEN_STRING;
            token->value = value;
            count++;
        }
        // 字符字面量
        else if (c == '\'') {
            lexer.pos++; // 跳过开始的引号
            lexer.column++;
            size_t start = lexer.pos;
            
            if (lexer.pos < lexer.length && lexer.source[lexer.pos] == '\\') {
                lexer.pos += 2; // 转义字符
                lexer.column += 2;
            } else if (lexer.pos < lexer.length) {
                lexer.pos++;
                lexer.column++;
            }
            
            size_t len = lexer.pos - start;
            char *value = malloc(len + 1);
            memcpy(value, lexer.source + start, len);
            value[len] = '\0';
            
            if (lexer.pos < lexer.length && lexer.source[lexer.pos] == '\'') {
                lexer.pos++;
                lexer.column++;
            }
            
            token->type = TOKEN_CHAR_LITERAL;
            token->value = value;
            count++;
        }
        // 操作符和标点
        else {
            char next = (lexer.pos + 1 < lexer.length) ? lexer.source[lexer.pos + 1] : '\0';
            
            switch (c) {
                case '+':
                    if (next == '+') {
                        token->type = TOKEN_INCREMENT;
                        token->value = strdup("++");
                        lexer.pos += 2;
                        lexer.column += 2;
                    } else if (next == '=') {
                        token->type = TOKEN_PLUS_ASSIGN;
                        token->value = strdup("+=");
                        lexer.pos += 2;
                        lexer.column += 2;
                    } else {
                        token->type = TOKEN_PLUS;
                        token->value = strdup("+");
                        lexer.pos++;
                        lexer.column++;
                    }
                    count++;
                    break;
                    
                case '-':
                    if (next == '-') {
                        token->type = TOKEN_DECREMENT;
                        token->value = strdup("--");
                        lexer.pos += 2;
                        lexer.column += 2;
                    } else if (next == '=') {
                        token->type = TOKEN_MINUS_ASSIGN;
                        token->value = strdup("-=");
                        lexer.pos += 2;
                        lexer.column += 2;
                    } else if (next == '>') {
                        token->type = TOKEN_ARROW;
                        token->value = strdup("->");
                        lexer.pos += 2;
                        lexer.column += 2;
                    } else {
                        token->type = TOKEN_MINUS;
                        token->value = strdup("-");
                        lexer.pos++;
                        lexer.column++;
                    }
                    count++;
                    break;
                    
                case '*':
                    if (next == '=') {
                        token->type = TOKEN_MUL_ASSIGN;
                        token->value = strdup("*=");
                        lexer.pos += 2;
                        lexer.column += 2;
                    } else {
                        token->type = TOKEN_MULTIPLY;
                        token->value = strdup("*");
                        lexer.pos++;
                        lexer.column++;
                    }
                    count++;
                    break;
                    
                case '/':
                    if (next == '=') {
                        token->type = TOKEN_DIV_ASSIGN;
                        token->value = strdup("/=");
                        lexer.pos += 2;
                        lexer.column += 2;
                    } else {
                        token->type = TOKEN_DIVIDE;
                        token->value = strdup("/");
                        lexer.pos++;
                        lexer.column++;
                    }
                    count++;
                    break;
                    
                case '%':
                    if (next == '=') {
                        token->type = TOKEN_MOD_ASSIGN;
                        token->value = strdup("%=");
                        lexer.pos += 2;
                        lexer.column += 2;
                    } else {
                        token->type = TOKEN_MOD;
                        token->value = strdup("%");
                        lexer.pos++;
                        lexer.column++;
                    }
                    count++;
                    break;
                    
                case '=':
                    if (next == '=') {
                        token->type = TOKEN_EQUAL;
                        token->value = strdup("==");
                        lexer.pos += 2;
                        lexer.column += 2;
                    } else {
                        token->type = TOKEN_ASSIGN;
                        token->value = strdup("=");
                        lexer.pos++;
                        lexer.column++;
                    }
                    count++;
                    break;
                    
                case '!':
                    if (next == '=') {
                        token->type = TOKEN_NOT_EQUAL;
                        token->value = strdup("!=");
                        lexer.pos += 2;
                        lexer.column += 2;
                    } else {
                        token->type = TOKEN_LOGICAL_NOT;
                        token->value = strdup("!");
                        lexer.pos++;
                        lexer.column++;
                    }
                    count++;
                    break;
                    
                case '<':
                    if (next == '=') {
                        token->type = TOKEN_LESS_EQUAL;
                        token->value = strdup("<=");
                        lexer.pos += 2;
                        lexer.column += 2;
                    } else if (next == '<') {
                        token->type = TOKEN_LEFT_SHIFT;
                        token->value = strdup("<<");
                        lexer.pos += 2;
                        lexer.column += 2;
                    } else {
                        token->type = TOKEN_LESS;
                        token->value = strdup("<");
                        lexer.pos++;
                        lexer.column++;
                    }
                    count++;
                    break;
                    
                case '>':
                    if (next == '=') {
                        token->type = TOKEN_GREATER_EQUAL;
                        token->value = strdup(">=");
                        lexer.pos += 2;
                        lexer.column += 2;
                    } else if (next == '>') {
                        token->type = TOKEN_RIGHT_SHIFT;
                        token->value = strdup(">>");
                        lexer.pos += 2;
                        lexer.column += 2;
                    } else {
                        token->type = TOKEN_GREATER;
                        token->value = strdup(">");
                        lexer.pos++;
                        lexer.column++;
                    }
                    count++;
                    break;
                    
                case '&':
                    if (next == '&') {
                        token->type = TOKEN_LOGICAL_AND;
                        token->value = strdup("&&");
                        lexer.pos += 2;
                        lexer.column += 2;
                    } else {
                        token->type = TOKEN_BIT_AND;
                        token->value = strdup("&");
                        lexer.pos++;
                        lexer.column++;
                    }
                    count++;
                    break;
                    
                case '|':
                    if (next == '|') {
                        token->type = TOKEN_LOGICAL_OR;
                        token->value = strdup("||");
                        lexer.pos += 2;
                        lexer.column += 2;
                    } else {
                        token->type = TOKEN_BIT_OR;
                        token->value = strdup("|");
                        lexer.pos++;
                        lexer.column++;
                    }
                    count++;
                    break;
                    
                case '^':
                    token->type = TOKEN_BIT_XOR;
                    token->value = strdup("^");
                    lexer.pos++;
                    lexer.column++;
                    count++;
                    break;
                    
                case '~':
                    token->type = TOKEN_BIT_NOT;
                    token->value = strdup("~");
                    lexer.pos++;
                    lexer.column++;
                    count++;
                    break;
                    
                case '(':
                    token->type = TOKEN_LPAREN;
                    token->value = strdup("(");
                    lexer.pos++;
                    lexer.column++;
                    count++;
                    break;
                    
                case ')':
                    token->type = TOKEN_RPAREN;
                    token->value = strdup(")");
                    lexer.pos++;
                    lexer.column++;
                    count++;
                    break;
                    
                case '{':
                    token->type = TOKEN_LBRACE;
                    token->value = strdup("{");
                    lexer.pos++;
                    lexer.column++;
                    count++;
                    break;
                    
                case '}':
                    token->type = TOKEN_RBRACE;
                    token->value = strdup("}");
                    lexer.pos++;
                    lexer.column++;
                    count++;
                    break;
                    
                case '[':
                    token->type = TOKEN_LBRACKET;
                    token->value = strdup("[");
                    lexer.pos++;
                    lexer.column++;
                    count++;
                    break;
                    
                case ']':
                    token->type = TOKEN_RBRACKET;
                    token->value = strdup("]");
                    lexer.pos++;
                    lexer.column++;
                    count++;
                    break;
                    
                case ';':
                    token->type = TOKEN_SEMICOLON;
                    token->value = strdup(";");
                    lexer.pos++;
                    lexer.column++;
                    count++;
                    break;
                    
                case ',':
                    token->type = TOKEN_COMMA;
                    token->value = strdup(",");
                    lexer.pos++;
                    lexer.column++;
                    count++;
                    break;
                    
                case '.':
                    token->type = TOKEN_DOT;
                    token->value = strdup(".");
                    lexer.pos++;
                    lexer.column++;
                    count++;
                    break;
                    
                case ':':
                    token->type = TOKEN_COLON;
                    token->value = strdup(":");
                    lexer.pos++;
                    lexer.column++;
                    count++;
                    break;
                    
                case '?':
                    token->type = TOKEN_QUESTION;
                    token->value = strdup("?");
                    lexer.pos++;
                    lexer.column++;
                    count++;
                    break;
                    
                case '#':
                    token->type = TOKEN_HASH;
                    token->value = strdup("#");
                    lexer.pos++;
                    lexer.column++;
                    count++;
                    break;
                    
                default:
                    token->type = TOKEN_UNKNOWN;
                    char buf[2] = {c, '\0'};
                    token->value = strdup(buf);
                    lexer.pos++;
                    lexer.column++;
                    count++;
                    break;
            }
        }
    }
    
    // 添加EOF标记
    tokens[count].type = TOKEN_EOF;
    tokens[count].value = strdup("");
    tokens[count].line = lexer.line;
    tokens[count].column = lexer.column;
    tokens[count].filename = filename;
    count++;
    
    *token_count = count;
    return tokens;
}

static void token_free(Token *tokens, int count) {
    for (int i = 0; i < count; i++) {
        free(tokens[i].value);
    }
    free(tokens);
}

// ====================================
// 解析器
// ====================================

typedef struct {
    Token *tokens;
    int token_count;
    int current;
    char error_msg[256];
    
    // 符号表
    struct {
        char *names[1024];
        char *types[1024];
        int is_function[1024];
        int count;
    } symbols;
} Parser;

// ====================================
// AST节点管理
// ====================================

static ASTNode* ast_create_node(ASTNodeType type, int line, int column) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = type;
    node->line = line;
    node->column = column;
    return node;
}

// 创建带文件名的AST节点
static ASTNode* create_ast_node(ASTNodeType type, int line, int column, const char *filename) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = type;
    node->line = line;
    node->column = column;
    node->filename = filename;
    return node;
}

// 添加子节点
static void add_child(ASTNode *parent, ASTNode *child) {
    if (!parent || !child) return;
    
    if (parent->data.generic.child_count >= parent->data.generic.child_capacity) {
        int new_capacity = parent->data.generic.child_capacity ? parent->data.generic.child_capacity * 2 : 4;
        parent->data.generic.children = realloc(parent->data.generic.children, 
                                               sizeof(ASTNode*) * new_capacity);
        parent->data.generic.child_capacity = new_capacity;
    }
    
    parent->data.generic.children[parent->data.generic.child_count++] = child;
}

static void ast_free(ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_FUNCTION:
            free(node->data.old_function.name);
            free(node->data.old_function.return_type);
            for (int i = 0; i < node->data.old_function.param_count; i++) {
                ast_free(node->data.old_function.params[i]);
            }
            free(node->data.old_function.params);
            ast_free(node->data.old_function.body);
            break;
            
        case AST_PARAMETER:
            free(node->data.param.type);
            free(node->data.param.name);
            break;
            
        case AST_IDENTIFIER:
            free(node->data.identifier.name);
            break;
            
        case AST_BINARY_OP:
            ast_free(node->data.binary.left);
            ast_free(node->data.binary.right);
            break;
            
        case AST_UNARY_OP:
            ast_free(node->data.unary.operand);
            break;
            
        case AST_RETURN:
            ast_free(node->data.ret.value);
            break;
            
        case AST_COMPOUND:
            for (int i = 0; i < node->data.compound.count; i++) {
                ast_free(node->data.compound.statements[i]);
            }
            free(node->data.compound.statements);
            break;
            
        case AST_DECLARATION:
            free(node->data.decl.type);
            free(node->data.decl.name);
            ast_free(node->data.decl.init);
            break;
            
        case AST_ASSIGNMENT:
            ast_free(node->data.assign.target);
            ast_free(node->data.assign.value);
            break;
            
        case AST_IF:
            ast_free(node->data.if_stmt.condition);
            ast_free(node->data.if_stmt.then_stmt);
            ast_free(node->data.if_stmt.else_stmt);
            break;
            
        case AST_WHILE:
            ast_free(node->data.while_stmt.condition);
            ast_free(node->data.while_stmt.body);
            break;
            
        case AST_FOR:
            ast_free(node->data.for_stmt.init);
            ast_free(node->data.for_stmt.condition);
            ast_free(node->data.for_stmt.increment);
            ast_free(node->data.for_stmt.body);
            break;
            
        case AST_CALL:
            free(node->data.old_call.name);
            for (int i = 0; i < node->data.old_call.arg_count; i++) {
                ast_free(node->data.old_call.args[i]);
            }
            free(node->data.old_call.args);
            break;
            
        case AST_ARRAY_ACCESS:
            ast_free(node->data.array_access.array);
            ast_free(node->data.array_access.index);
            break;
            
        case AST_CAST:
            free(node->data.cast.target_type);
            ast_free(node->data.cast.expr);
            break;
            
        case AST_SIZEOF:
            free(node->data.sizeof_expr.type_name);
            ast_free(node->data.sizeof_expr.expr);
            break;
            
        case AST_TYPE:
            free(node->data.old_type.base_type);
            break;
            
        // 新的节点类型
        case AST_TRANSLATION_UNIT:
        case AST_COMPOUND_STMT:
            for (int i = 0; i < node->data.generic.child_count; i++) {
                ast_free(node->data.generic.children[i]);
            }
            free(node->data.generic.children);
            break;
            
        case AST_FUNCTION_DEF:
        case AST_FUNCTION_DECL:
            free(node->data.function.name);
            for (int i = 0; i < node->data.function.param_count; i++) {
                ast_free(node->data.function.params[i]);
            }
            free(node->data.function.params);
            ast_free(node->data.function.body);
            break;
            
        case AST_VAR_DECL:
            free(node->data.var_decl.name);
            ast_free(node->data.var_decl.init);
            break;
            
        case AST_BINARY_EXPR:
            ast_free(node->data.binary.left);
            ast_free(node->data.binary.right);
            break;
            
        case AST_UNARY_EXPR:
            ast_free(node->data.unary.operand);
            break;
            
        case AST_ASSIGNMENT_EXPR:
            ast_free(node->data.assignment.left);
            ast_free(node->data.assignment.right);
            break;
            
        case AST_CALL_EXPR:
            ast_free(node->data.call.function);
            for (int i = 0; i < node->data.call.arg_count; i++) {
                ast_free(node->data.call.args[i]);
            }
            free(node->data.call.args);
            break;
            
        case AST_STRING_LITERAL:
            free(node->value.str_val);
            break;
            
        default:
            break;
    }
    
    // 释放next链
    if (node->next) {
        ast_free(node->next);
    }
    
    free(node);
}

// 释放AST节点（新版本）
static void free_ast_node(ASTNode *node) {
    ast_free(node);
}

// 打印AST
static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

static void ast_print(ASTNode *node, int indent) {
    if (!node) {
        print_indent(indent);
        printf("(null)\n");
        return;
    }
    
    print_indent(indent);
    
    switch (node->type) {
        case AST_PROGRAM:
            printf("Program\n");
            for (int i = 0; i < node->data.compound.count; i++) {
                ast_print(node->data.compound.statements[i], indent + 1);
            }
            break;
            
        case AST_FUNCTION:
            printf("Function: %s %s\n", node->data.old_function.return_type, node->data.old_function.name);
            print_indent(indent + 1);
            printf("Parameters:\n");
            for (int i = 0; i < node->data.old_function.param_count; i++) {
                ast_print(node->data.old_function.params[i], indent + 2);
            }
            print_indent(indent + 1);
            printf("Body:\n");
            ast_print(node->data.old_function.body, indent + 2);
            break;
            
        case AST_PARAMETER:
            printf("Parameter: %s %s\n", node->data.param.type, node->data.param.name);
            break;
            
        case AST_RETURN:
            printf("Return\n");
            if (node->data.ret.value) {
                ast_print(node->data.ret.value, indent + 1);
            }
            break;
            
        case AST_INTEGER:
            printf("Integer: %lld\n", node->value.int_val);
            break;
            
        case AST_IDENTIFIER:
            printf("Identifier: %s\n", node->data.identifier.name);
            break;
            
        case AST_BINARY_OP:
            printf("BinaryOp: '%c'\n", (char)node->data.binary.op);
            ast_print(node->data.binary.left, indent + 1);
            ast_print(node->data.binary.right, indent + 1);
            break;
            
        case AST_UNARY_OP:
            printf("UnaryOp: '%c'\n", (char)node->data.unary.op);
            ast_print(node->data.unary.operand, indent + 1);
            break;
            
        case AST_COMPOUND:
            printf("Compound (%d statements)\n", node->data.compound.count);
            for (int i = 0; i < node->data.compound.count; i++) {
                ast_print(node->data.compound.statements[i], indent + 1);
            }
            break;
            
        case AST_DECLARATION:
            printf("Declaration: %s %s", node->data.decl.type, node->data.decl.name);
            if (node->data.decl.is_array) {
                printf("[%d]", node->data.decl.array_size);
            }
            printf("\n");
            if (node->data.decl.init) {
                print_indent(indent + 1);
                printf("Init:\n");
                ast_print(node->data.decl.init, indent + 2);
            }
            break;
            
        case AST_ASSIGNMENT:
            printf("Assignment\n");
            print_indent(indent + 1);
            printf("Target:\n");
            ast_print(node->data.assign.target, indent + 2);
            print_indent(indent + 1);
            printf("Value:\n");
            ast_print(node->data.assign.value, indent + 2);
            break;
            
        case AST_IF:
            printf("If\n");
            print_indent(indent + 1);
            printf("Condition:\n");
            ast_print(node->data.if_stmt.condition, indent + 2);
            print_indent(indent + 1);
            printf("Then:\n");
            ast_print(node->data.if_stmt.then_stmt, indent + 2);
            if (node->data.if_stmt.else_stmt) {
                print_indent(indent + 1);
                printf("Else:\n");
                ast_print(node->data.if_stmt.else_stmt, indent + 2);
            }
            break;
            
        case AST_WHILE:
            printf("While\n");
            print_indent(indent + 1);
            printf("Condition:\n");
            ast_print(node->data.while_stmt.condition, indent + 2);
            print_indent(indent + 1);
            printf("Body:\n");
            ast_print(node->data.while_stmt.body, indent + 2);
            break;
            
        case AST_FOR:
            printf("For\n");
            if (node->data.for_stmt.init) {
                print_indent(indent + 1);
                printf("Init:\n");
                ast_print(node->data.for_stmt.init, indent + 2);
            }
            if (node->data.for_stmt.condition) {
                print_indent(indent + 1);
                printf("Condition:\n");
                ast_print(node->data.for_stmt.condition, indent + 2);
            }
            if (node->data.for_stmt.increment) {
                print_indent(indent + 1);
                printf("Increment:\n");
                ast_print(node->data.for_stmt.increment, indent + 2);
            }
            print_indent(indent + 1);
            printf("Body:\n");
            ast_print(node->data.for_stmt.body, indent + 2);
            break;
            
        case AST_EXPRESSION_STMT:
            printf("ExpressionStatement\n");
            break;
            
        case AST_CALL:
            printf("Call: %s\n", node->data.old_call.name);
            for (int i = 0; i < node->data.old_call.arg_count; i++) {
                print_indent(indent + 1);
                printf("Arg %d:\n", i);
                ast_print(node->data.old_call.args[i], indent + 2);
            }
            break;
            
        case AST_ARRAY_ACCESS:
            printf("ArrayAccess\n");
            print_indent(indent + 1);
            printf("Array:\n");
            ast_print(node->data.array_access.array, indent + 2);
            print_indent(indent + 1);
            printf("Index:\n");
            ast_print(node->data.array_access.index, indent + 2);
            break;
            
        case AST_BREAK:
            printf("Break\n");
            break;
            
        case AST_CONTINUE:
            printf("Continue\n");
            break;
            
        default:
            printf("Unknown AST node type: %d\n", node->type);
            break;
    }
}

// Token辅助函数
static const char* token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_STRING: return "STRING";
        case TOKEN_INT: return "int";
        case TOKEN_RETURN: return "return";
        case TOKEN_IF: return "if";
        case TOKEN_ELSE: return "else";
        case TOKEN_WHILE: return "while";
        case TOKEN_FOR: return "for";
        case TOKEN_PLUS: return "+";
        case TOKEN_MINUS: return "-";
        case TOKEN_MULTIPLY: return "*";
        case TOKEN_DIVIDE: return "/";
        case TOKEN_ASSIGN: return "=";
        case TOKEN_EQUAL: return "==";
        case TOKEN_NOT_EQUAL: return "!=";
        case TOKEN_LESS: return "<";
        case TOKEN_GREATER: return ">";
        case TOKEN_LESS_EQUAL: return "<=";
        case TOKEN_GREATER_EQUAL: return ">=";
        case TOKEN_LPAREN: return "(";
        case TOKEN_RPAREN: return ")";
        case TOKEN_LBRACE: return "{";
        case TOKEN_RBRACE: return "}";
        case TOKEN_SEMICOLON: return ";";
        case TOKEN_COMMA: return ",";
        default: return "UNKNOWN";
    }
}

// ====================================
// 包含解析器实现
// ====================================

// 防止重复定义，先定义包含保护
#ifndef EVOLVER0_PARSER_INCLUDED
#define EVOLVER0_PARSER_INCLUDED
#include "evolver0_parser.inc.c"
#endif

// ====================================
// 代码生成器
// ====================================

// 删除这里的CodeGen定义，使用codegen模块中的定义

// ====================================
// 包含代码生成器实现
// ====================================

#ifndef EVOLVER0_CODEGEN_INCLUDED
#define EVOLVER0_CODEGEN_INCLUDED
#include "evolver0_codegen.inc.c"
#endif

// ====================================
// 包含ELF生成器实现
// ====================================

#ifndef EVOLVER0_ELF_INCLUDED
#define EVOLVER0_ELF_INCLUDED
#include "evolver0_elf.inc.c"
#endif

// ====================================
// 公共接口适配
// ====================================

// 适配旧接口到新接口
static ASTNode* parse_program(Parser *parser) {
    return parse_tokens(parser->tokens, parser->token_count);
}

// 适配代码生成接口
static bool codegen_program(ASTNode *ast, CodeGen *gen) {
    size_t code_size;
    uint8_t *code = generate_code(ast, &code_size);
    
    if (!code) return false;
    
    // 复制生成的代码到gen结构
    if (gen->capacity < code_size) {
        gen->capacity = code_size;
        gen->code = realloc(gen->code, gen->capacity);
    }
    
    memcpy(gen->code, code, code_size);
    gen->size = code_size;
    
    free(code);
    return true;
}

// 写入ELF文件
static int write_elf_file(const char *filename, unsigned char *code, size_t code_size) {
    return create_elf_executable(filename, code, code_size, 64);  // 64位
}

// ====================================
// 主函数
// ====================================

static char* read_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        return NULL;
    }
    
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *content = malloc(size + 1);
    if (!content) {
        fclose(f);
        return NULL;
    }
    
    size_t read_size = fread(content, 1, size, f);
    content[read_size] = '\0';
    
    fclose(f);
    return content;
}

static void print_usage(const char *program) {
    fprintf(stderr, "Usage: %s [options] <input.c> -o <output>\n", program);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -v, --verbose     Enable verbose output\n");
    fprintf(stderr, "  --dump-ast        Dump AST\n");
    fprintf(stderr, "  --dump-asm        Dump generated assembly\n");
    fprintf(stderr, "  -h, --help        Show this help\n");
}

int main(int argc, char *argv[]) {
    CompilerOptions options = {0};
    
    // 解析命令行参数
    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            options.output_file = argv[++i];
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            options.verbose = true;
        } else if (strcmp(argv[i], "--dump-ast") == 0) {
            options.dump_ast = true;
        } else if (strcmp(argv[i], "--dump-asm") == 0) {
            options.dump_asm = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (argv[i][0] != '-' && !options.input_file) {
            options.input_file = argv[i];
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
        i++;
    }
    
    if (!options.input_file || !options.output_file) {
        fprintf(stderr, "Error: Input and output files are required\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // 读取源文件
    char *source = read_file(options.input_file);
    if (!source) {
        fprintf(stderr, "Error: Cannot read file %s\n", options.input_file);
        return 1;
    }
    
    if (options.verbose) {
        printf("Compiling %s -> %s\n", options.input_file, options.output_file);
    }
    
    // 词法分析
    int token_count;
    Token *tokens = lexer_tokenize(source, options.input_file, &token_count);
    if (!tokens) {
        fprintf(stderr, "Lexical analysis failed\n");
        free(source);
        return 1;
    }
    
    if (options.verbose) {
        printf("Lexical analysis complete: %d tokens\n", token_count);
    }
    
    // 语法分析
    Parser parser = {
        .tokens = tokens,
        .token_count = token_count,
        .current = 0
    };
    
    ASTNode *ast = parse_program(&parser);
    if (!ast) {
        fprintf(stderr, "Syntax analysis failed: %s\n", parser.error_msg);
        token_free(tokens, token_count);
        free(source);
        return 1;
    }
    
    if (options.verbose) {
        printf("Syntax analysis complete\n");
    }
    
    if (options.dump_ast) {
        printf("\n=== AST ===\n");
        ast_print(ast, 0);
        printf("\n");
    }
    
    // 代码生成
    CodeGen gen = {0};
    gen.capacity = MAX_CODE_SIZE;
    gen.code = malloc(gen.capacity);
    
    if (!codegen_program(ast, &gen)) {
        fprintf(stderr, "Code generation failed\n");
        free(gen.code);
        ast_free(ast);
        token_free(tokens, token_count);
        free(source);
        return 1;
    }
    
    if (options.verbose) {
        printf("Code generation complete: %zu bytes\n", gen.size);
    }
    
    if (options.dump_asm) {
        printf("\n=== Generated Code ===\n");
        for (size_t i = 0; i < gen.size; i++) {
            printf("%02X ", gen.code[i]);
            if ((i + 1) % 16 == 0) printf("\n");
        }
        if (gen.size % 16 != 0) printf("\n");
        printf("\n");
    }
    
    // 生成ELF文件
    if (write_elf_file(options.output_file, gen.code, gen.size) != 0) {
        fprintf(stderr, "Failed to write output file\n");
        free(gen.code);
        ast_free(ast);
        token_free(tokens, token_count);
        free(source);
        return 1;
    }
    
    if (options.verbose) {
        printf("Successfully generated executable: %s\n", options.output_file);
    }
    
    // 清理
    free(gen.code);
    ast_free(ast);
    token_free(tokens, token_count);
    free(source);
    
    return 0;
}