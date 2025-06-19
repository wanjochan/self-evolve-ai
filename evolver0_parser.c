// evolver0_parser.c - C语言解析器模块
// 这是evolver0.c的解析器部分的完整实现

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// ==================== 类型定义 ====================

// Token类型定义（与evolver0.c一致）
typedef enum {
    TOKEN_EOF = 0,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_FLOAT_NUMBER,
    TOKEN_STRING,
    TOKEN_CHAR_LITERAL,
    
    // 数据类型关键字
    TOKEN_VOID, TOKEN_CHAR, TOKEN_SHORT, TOKEN_INT, TOKEN_LONG,
    TOKEN_FLOAT, TOKEN_DOUBLE, TOKEN_SIGNED, TOKEN_UNSIGNED, TOKEN_BOOL,
    
    // 存储类说明符
    TOKEN_TYPEDEF, TOKEN_EXTERN, TOKEN_STATIC, TOKEN_AUTO, TOKEN_REGISTER,
    
    // 类型限定符
    TOKEN_CONST, TOKEN_VOLATILE, TOKEN_RESTRICT, TOKEN_ATOMIC,
    
    // 函数说明符
    TOKEN_INLINE, TOKEN_NORETURN,
    
    // 控制流关键字
    TOKEN_IF, TOKEN_ELSE, TOKEN_SWITCH, TOKEN_CASE, TOKEN_DEFAULT,
    TOKEN_WHILE, TOKEN_DO, TOKEN_FOR, TOKEN_BREAK, TOKEN_CONTINUE,
    TOKEN_GOTO, TOKEN_RETURN,
    
    // 结构、联合、枚举
    TOKEN_STRUCT, TOKEN_UNION, TOKEN_ENUM, TOKEN_SIZEOF,
    
    // 标点符号
    TOKEN_LBRACE, TOKEN_RBRACE, TOKEN_LPAREN, TOKEN_RPAREN,
    TOKEN_LBRACKET, TOKEN_RBRACKET, TOKEN_SEMICOLON, TOKEN_COLON,
    TOKEN_COMMA, TOKEN_DOT, TOKEN_ELLIPSIS, TOKEN_QUESTION,
    TOKEN_ARROW,
    
    // 操作符
    // 赋值
    TOKEN_ASSIGN, TOKEN_ADD_ASSIGN, TOKEN_SUB_ASSIGN, TOKEN_MUL_ASSIGN,
    TOKEN_DIV_ASSIGN, TOKEN_MOD_ASSIGN, TOKEN_LEFT_SHIFT_ASSIGN,
    TOKEN_RIGHT_SHIFT_ASSIGN, TOKEN_BIT_AND_ASSIGN, TOKEN_BIT_XOR_ASSIGN,
    TOKEN_BIT_OR_ASSIGN,
    
    // 算术
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULTIPLY, TOKEN_DIVIDE, TOKEN_MOD,
    TOKEN_INCREMENT, TOKEN_DECREMENT,
    
    // 关系
    TOKEN_EQUAL, TOKEN_NOT_EQUAL, TOKEN_LESS, TOKEN_GREATER,
    TOKEN_LESS_EQUAL, TOKEN_GREATER_EQUAL,
    
    // 逻辑
    TOKEN_LOGICAL_AND, TOKEN_LOGICAL_OR, TOKEN_LOGICAL_NOT,
    
    // 位运算
    TOKEN_BIT_AND, TOKEN_BIT_OR, TOKEN_BIT_XOR, TOKEN_BIT_NOT,
    TOKEN_LEFT_SHIFT, TOKEN_RIGHT_SHIFT,
    
    // 错误
    TOKEN_ERROR
} TokenType;

// Token结构
typedef struct {
    TokenType type;
    char *value;
    int line;
    int column;
} Token;

// AST节点类型
typedef enum {
    // 程序结构
    AST_PROGRAM,
    AST_TRANSLATION_UNIT,
    
    // 声明
    AST_FUNCTION_DECL,
    AST_FUNCTION_DEF,
    AST_VAR_DECL,
    AST_TYPE_DECL,
    AST_STRUCT_DECL,
    AST_UNION_DECL,
    AST_ENUM_DECL,
    AST_TYPEDEF_DECL,
    
    // 语句
    AST_COMPOUND_STMT,
    AST_EXPR_STMT,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_DO_WHILE_STMT,
    AST_FOR_STMT,
    AST_RETURN_STMT,
    AST_BREAK_STMT,
    AST_CONTINUE_STMT,
    AST_GOTO_STMT,
    AST_LABEL_STMT,
    AST_SWITCH_STMT,
    AST_CASE_STMT,
    AST_DEFAULT_STMT,
    AST_NULL_STMT,
    
    // 表达式
    AST_BINARY_EXPR,
    AST_UNARY_EXPR,
    AST_TERNARY_EXPR,
    AST_ASSIGN_EXPR,
    AST_CALL_EXPR,
    AST_MEMBER_EXPR,
    AST_ARRAY_SUBSCRIPT,
    AST_CAST_EXPR,
    AST_SIZEOF_EXPR,
    AST_COMMA_EXPR,
    
    // 字面量和标识符
    AST_IDENTIFIER,
    AST_INT_LITERAL,
    AST_FLOAT_LITERAL,
    AST_STRING_LITERAL,
    AST_CHAR_LITERAL,
    
    // 类型
    AST_TYPE_SPEC,
    AST_POINTER_TYPE,
    AST_ARRAY_TYPE,
    AST_FUNCTION_TYPE,
    AST_STRUCT_TYPE,
    AST_UNION_TYPE,
    AST_ENUM_TYPE,
    
    // 其他
    AST_PARAM_LIST,
    AST_PARAM,
    AST_ARG_LIST,
    AST_INIT_LIST,
    AST_FIELD_LIST,
    AST_ENUMERATOR_LIST,
    AST_DECL_LIST,
    AST_STMT_LIST
} ASTNodeType;

// AST节点结构
typedef struct ASTNode {
    ASTNodeType type;
    int line;
    int column;
    
    // 节点数据
    union {
        // 字面量值
        struct {
            char *value;
            long long int_val;
            double float_val;
        } literal;
        
        // 标识符
        struct {
            char *name;
        } identifier;
        
        // 二元表达式
        struct {
            struct ASTNode *left;
            struct ASTNode *right;
            TokenType op;
        } binary;
        
        // 一元表达式
        struct {
            struct ASTNode *operand;
            TokenType op;
            int is_prefix;  // 前缀还是后缀
        } unary;
        
        // 三元表达式
        struct {
            struct ASTNode *condition;
            struct ASTNode *true_expr;
            struct ASTNode *false_expr;
        } ternary;
        
        // 函数调用
        struct {
            struct ASTNode *func;
            struct ASTNode *args;  // AST_ARG_LIST
        } call;
        
        // 成员访问
        struct {
            struct ASTNode *object;
            char *member;
            int is_pointer;  // -> 还是 .
        } member;
        
        // 数组下标
        struct {
            struct ASTNode *array;
            struct ASTNode *index;
        } array_subscript;
        
        // 类型转换
        struct {
            struct ASTNode *type;
            struct ASTNode *expr;
        } cast;
        
        // sizeof
        struct {
            struct ASTNode *operand;  // 可能是类型或表达式
            int is_type;  // 是类型还是表达式
        } sizeof_expr;
        
        // 复合语句
        struct {
            struct ASTNode *stmts;  // AST_STMT_LIST
        } compound;
        
        // if语句
        struct {
            struct ASTNode *condition;
            struct ASTNode *then_stmt;
            struct ASTNode *else_stmt;
        } if_stmt;
        
        // while语句
        struct {
            struct ASTNode *condition;
            struct ASTNode *body;
        } while_stmt;
        
        // for语句
        struct {
            struct ASTNode *init;
            struct ASTNode *condition;
            struct ASTNode *update;
            struct ASTNode *body;
        } for_stmt;
        
        // return语句
        struct {
            struct ASTNode *value;
        } return_stmt;
        
        // goto语句
        struct {
            char *label;
        } goto_stmt;
        
        // 标签语句
        struct {
            char *label;
            struct ASTNode *stmt;
        } label_stmt;
        
        // switch语句
        struct {
            struct ASTNode *expr;
            struct ASTNode *cases;  // case语句列表
        } switch_stmt;
        
        // case语句
        struct {
            struct ASTNode *value;
            struct ASTNode *stmts;
        } case_stmt;
        
        // 变量声明
        struct {
            struct ASTNode *type;
            char *name;
            struct ASTNode *init;
            int storage_class;
        } var_decl;
        
        // 函数声明/定义
        struct {
            struct ASTNode *return_type;
            char *name;
            struct ASTNode *params;  // AST_PARAM_LIST
            struct ASTNode *body;    // NULL表示声明，非NULL表示定义
            int storage_class;
            int is_inline;
            int is_noreturn;
        } func_decl;
        
        // 参数
        struct {
            struct ASTNode *type;
            char *name;
        } param;
        
        // 结构体/联合体/枚举
        struct {
            char *tag;
            struct ASTNode *fields;  // AST_FIELD_LIST或AST_ENUMERATOR_LIST
            int is_definition;
        } struct_decl;
        
        // typedef
        struct {
            struct ASTNode *type;
            char *name;
        } typedef_decl;
        
        // 类型说明符
        struct {
            TokenType basic_type;
            int is_unsigned;
            int is_signed;
            int is_long;
            int is_long_long;
            int is_short;
            int qualifiers;  // const, volatile, restrict等的位掩码
        } type_spec;
        
        // 指针类型
        struct {
            struct ASTNode *base_type;
            int qualifiers;
        } pointer;
        
        // 数组类型
        struct {
            struct ASTNode *element_type;
            struct ASTNode *size;  // NULL表示[]
            int is_static;
            int qualifiers;
        } array;
        
        // 函数类型
        struct {
            struct ASTNode *return_type;
            struct ASTNode *params;
            int is_variadic;
        } function_type;
        
        // 列表节点（用于各种列表）
        struct {
            struct ASTNode **items;
            int count;
            int capacity;
        } list;
    } data;
    
    // 下一个节点（用于链表）
    struct ASTNode *next;
} ASTNode;

// 解析器结构
typedef struct {
    Token *tokens;
    int token_count;
    int current;
    
    // 错误处理
    char *error_msg;
    int error_line;
    int error_column;
    
    // 符号表（简化版）
    struct {
        char **names;
        ASTNode **nodes;
        int count;
        int capacity;
    } symbols;
} Parser;

// ==================== 辅助函数 ====================

// 创建AST节点
static ASTNode *create_ast_node(ASTNodeType type, int line, int column) {
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "内存分配失败\n");
        exit(1);
    }
    node->type = type;
    node->line = line;
    node->column = column;
    return node;
}

// 创建列表节点
static ASTNode *create_list_node(ASTNodeType type, int line, int column) {
    ASTNode *node = create_ast_node(type, line, column);
    node->data.list.capacity = 8;
    node->data.list.items = (ASTNode **)calloc(node->data.list.capacity, sizeof(ASTNode *));
    node->data.list.count = 0;
    return node;
}

// 向列表添加项
static void list_add_item(ASTNode *list, ASTNode *item) {
    if (list->data.list.count >= list->data.list.capacity) {
        list->data.list.capacity *= 2;
        list->data.list.items = (ASTNode **)realloc(list->data.list.items,
                                                    list->data.list.capacity * sizeof(ASTNode *));
    }
    list->data.list.items[list->data.list.count++] = item;
}

// 释放AST节点
static void free_ast_node(ASTNode *node) {
    if (!node) return;
    
    // 根据节点类型释放相应的数据
    switch (node->type) {
        case AST_IDENTIFIER:
            free(node->data.identifier.name);
            break;
        case AST_INT_LITERAL:
        case AST_FLOAT_LITERAL:
        case AST_STRING_LITERAL:
        case AST_CHAR_LITERAL:
            free(node->data.literal.value);
            break;
        case AST_PROGRAM:
        case AST_TRANSLATION_UNIT:
        case AST_PARAM_LIST:
        case AST_ARG_LIST:
        case AST_INIT_LIST:
        case AST_FIELD_LIST:
        case AST_ENUMERATOR_LIST:
        case AST_DECL_LIST:
        case AST_STMT_LIST:
            for (int i = 0; i < node->data.list.count; i++) {
                free_ast_node(node->data.list.items[i]);
            }
            free(node->data.list.items);
            break;
        // ... 其他节点类型的释放逻辑
        default:
            // 递归释放子节点
            break;
    }
    
    free(node);
}

// ==================== 词法分析辅助函数 ====================

static Token *current_token(Parser *parser) {
    if (parser->current >= parser->token_count) {
        return &parser->tokens[parser->token_count - 1];  // EOF
    }
    return &parser->tokens[parser->current];
}

static Token *peek_token(Parser *parser, int offset) {
    int index = parser->current + offset;
    if (index >= parser->token_count) {
        return &parser->tokens[parser->token_count - 1];  // EOF
    }
    return &parser->tokens[index];
}

static bool match_token(Parser *parser, TokenType type) {
    if (current_token(parser)->type == type) {
        parser->current++;
        return true;
    }
    return false;
}

static bool check_token(Parser *parser, TokenType type) {
    return current_token(parser)->type == type;
}

static Token *consume_token(Parser *parser, TokenType type, const char *error_msg) {
    Token *token = current_token(parser);
    if (token->type != type) {
        parser->error_msg = strdup(error_msg);
        parser->error_line = token->line;
        parser->error_column = token->column;
        return NULL;
    }
    parser->current++;
    return token;
}

// ==================== 表达式解析 ====================

// 前向声明
static ASTNode *parse_expression(Parser *parser);
static ASTNode *parse_assignment_expr(Parser *parser);
static ASTNode *parse_conditional_expr(Parser *parser);
static ASTNode *parse_logical_or_expr(Parser *parser);
static ASTNode *parse_logical_and_expr(Parser *parser);
static ASTNode *parse_bitwise_or_expr(Parser *parser);
static ASTNode *parse_bitwise_xor_expr(Parser *parser);
static ASTNode *parse_bitwise_and_expr(Parser *parser);
static ASTNode *parse_equality_expr(Parser *parser);
static ASTNode *parse_relational_expr(Parser *parser);
static ASTNode *parse_shift_expr(Parser *parser);
static ASTNode *parse_additive_expr(Parser *parser);
static ASTNode *parse_multiplicative_expr(Parser *parser);
static ASTNode *parse_cast_expr(Parser *parser);
static ASTNode *parse_unary_expr(Parser *parser);
static ASTNode *parse_postfix_expr(Parser *parser);
static ASTNode *parse_primary_expr(Parser *parser);

// 解析主表达式（最高优先级）
static ASTNode *parse_primary_expr(Parser *parser) {
    Token *token = current_token(parser);
    
    switch (token->type) {
        case TOKEN_IDENTIFIER: {
            parser->current++;
            ASTNode *node = create_ast_node(AST_IDENTIFIER, token->line, token->column);
            node->data.identifier.name = strdup(token->value);
            return node;
        }
        
        case TOKEN_NUMBER: {
            parser->current++;
            ASTNode *node = create_ast_node(AST_INT_LITERAL, token->line, token->column);
            node->data.literal.value = strdup(token->value);
            node->data.literal.int_val = strtoll(token->value, NULL, 0);
            return node;
        }
        
        case TOKEN_FLOAT_NUMBER: {
            parser->current++;
            ASTNode *node = create_ast_node(AST_FLOAT_LITERAL, token->line, token->column);
            node->data.literal.value = strdup(token->value);
            node->data.literal.float_val = strtod(token->value, NULL);
            return node;
        }
        
        case TOKEN_STRING: {
            parser->current++;
            ASTNode *node = create_ast_node(AST_STRING_LITERAL, token->line, token->column);
            node->data.literal.value = strdup(token->value);
            return node;
        }
        
        case TOKEN_CHAR_LITERAL: {
            parser->current++;
            ASTNode *node = create_ast_node(AST_CHAR_LITERAL, token->line, token->column);
            node->data.literal.value = strdup(token->value);
            // 解析字符字面量的实际值
            if (token->value[1] == '\\') {
                // 转义字符
                switch (token->value[2]) {
                    case 'n': node->data.literal.int_val = '\n'; break;
                    case 't': node->data.literal.int_val = '\t'; break;
                    case 'r': node->data.literal.int_val = '\r'; break;
                    case '0': node->data.literal.int_val = '\0'; break;
                    case '\\': node->data.literal.int_val = '\\'; break;
                    case '\'': node->data.literal.int_val = '\''; break;
                    default: node->data.literal.int_val = token->value[2];
                }
            } else {
                node->data.literal.int_val = token->value[1];
            }
            return node;
        }
        
        case TOKEN_LPAREN: {
            parser->current++;
            ASTNode *expr = parse_expression(parser);
            if (!expr) return NULL;
            if (!consume_token(parser, TOKEN_RPAREN, "期望')'")) {
                free_ast_node(expr);
                return NULL;
            }
            return expr;
        }
        
        default:
            parser->error_msg = strdup("期望主表达式");
            parser->error_line = token->line;
            parser->error_column = token->column;
            return NULL;
    }
}

// 解析后缀表达式
static ASTNode *parse_postfix_expr(Parser *parser) {
    ASTNode *expr = parse_primary_expr(parser);
    if (!expr) return NULL;
    
    while (true) {
        Token *token = current_token(parser);
        
        switch (token->type) {
            case TOKEN_LBRACKET: {
                // 数组下标
                parser->current++;
                ASTNode *index = parse_expression(parser);
                if (!index) {
                    free_ast_node(expr);
                    return NULL;
                }
                if (!consume_token(parser, TOKEN_RBRACKET, "期望']'")) {
                    free_ast_node(expr);
                    free_ast_node(index);
                    return NULL;
                }
                
                ASTNode *subscript = create_ast_node(AST_ARRAY_SUBSCRIPT, token->line, token->column);
                subscript->data.array_subscript.array = expr;
                subscript->data.array_subscript.index = index;
                expr = subscript;
                break;
            }
            
            case TOKEN_LPAREN: {
                // 函数调用
                parser->current++;
                ASTNode *args = create_list_node(AST_ARG_LIST, token->line, token->column);
                
                // 解析参数列表
                if (!check_token(parser, TOKEN_RPAREN)) {
                    do {
                        ASTNode *arg = parse_assignment_expr(parser);
                        if (!arg) {
                            free_ast_node(expr);
                            free_ast_node(args);
                            return NULL;
                        }
                        list_add_item(args, arg);
                    } while (match_token(parser, TOKEN_COMMA));
                }
                
                if (!consume_token(parser, TOKEN_RPAREN, "期望')'")) {
                    free_ast_node(expr);
                    free_ast_node(args);
                    return NULL;
                }
                
                ASTNode *call = create_ast_node(AST_CALL_EXPR, token->line, token->column);
                call->data.call.func = expr;
                call->data.call.args = args;
                expr = call;
                break;
            }
            
            case TOKEN_DOT:
            case TOKEN_ARROW: {
                // 成员访问
                int is_pointer = (token->type == TOKEN_ARROW);
                parser->current++;
                
                Token *member_token = consume_token(parser, TOKEN_IDENTIFIER, "期望成员名");
                if (!member_token) {
                    free_ast_node(expr);
                    return NULL;
                }
                
                ASTNode *member = create_ast_node(AST_MEMBER_EXPR, token->line, token->column);
                member->data.member.object = expr;
                member->data.member.member = strdup(member_token->value);
                member->data.member.is_pointer = is_pointer;
                expr = member;
                break;
            }
            
            case TOKEN_INCREMENT:
            case TOKEN_DECREMENT: {
                // 后缀++/--
                parser->current++;
                ASTNode *unary = create_ast_node(AST_UNARY_EXPR, token->line, token->column);
                unary->data.unary.operand = expr;
                unary->data.unary.op = token->type;
                unary->data.unary.is_prefix = 0;
                expr = unary;
                break;
            }
            
            default:
                return expr;
        }
    }
}

// 解析一元表达式
static ASTNode *parse_unary_expr(Parser *parser) {
    Token *token = current_token(parser);
    
    switch (token->type) {
        case TOKEN_INCREMENT:
        case TOKEN_DECREMENT:
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_LOGICAL_NOT:
        case TOKEN_BIT_NOT:
        case TOKEN_MULTIPLY:  // 解引用 *
        case TOKEN_BIT_AND: { // 取地址 &
            parser->current++;
            ASTNode *operand = parse_cast_expr(parser);
            if (!operand) return NULL;
            
            ASTNode *unary = create_ast_node(AST_UNARY_EXPR, token->line, token->column);
            unary->data.unary.operand = operand;
            unary->data.unary.op = token->type;
            unary->data.unary.is_prefix = 1;
            return unary;
        }
        
        case TOKEN_SIZEOF: {
            parser->current++;
            ASTNode *sizeof_expr = create_ast_node(AST_SIZEOF_EXPR, token->line, token->column);
            
            if (match_token(parser, TOKEN_LPAREN)) {
                // 可能是sizeof(type)或sizeof(expr)
                // 这里简化处理，暂时只支持sizeof(expr)
                ASTNode *operand = parse_expression(parser);
                if (!operand) {
                    free_ast_node(sizeof_expr);
                    return NULL;
                }
                
                if (!consume_token(parser, TOKEN_RPAREN, "期望')'")) {
                    free_ast_node(sizeof_expr);
                    free_ast_node(operand);
                    return NULL;
                }
                
                sizeof_expr->data.sizeof_expr.operand = operand;
                sizeof_expr->data.sizeof_expr.is_type = 0;
            } else {
                // sizeof expr
                ASTNode *operand = parse_unary_expr(parser);
                if (!operand) {
                    free_ast_node(sizeof_expr);
                    return NULL;
                }
                sizeof_expr->data.sizeof_expr.operand = operand;
                sizeof_expr->data.sizeof_expr.is_type = 0;
            }
            
            return sizeof_expr;
        }
        
        default:
            return parse_postfix_expr(parser);
    }
}

// 解析类型转换表达式
static ASTNode *parse_cast_expr(Parser *parser) {
    // 简化版：暂时不实现类型转换
    return parse_unary_expr(parser);
}

// 解析乘法表达式
static ASTNode *parse_multiplicative_expr(Parser *parser) {
    ASTNode *left = parse_cast_expr(parser);
    if (!left) return NULL;
    
    while (true) {
        Token *token = current_token(parser);
        if (token->type != TOKEN_MULTIPLY && 
            token->type != TOKEN_DIVIDE && 
            token->type != TOKEN_MOD) {
            break;
        }
        
        parser->current++;
        ASTNode *right = parse_cast_expr(parser);
        if (!right) {
            free_ast_node(left);
            return NULL;
        }
        
        ASTNode *binary = create_ast_node(AST_BINARY_EXPR, token->line, token->column);
        binary->data.binary.left = left;
        binary->data.binary.right = right;
        binary->data.binary.op = token->type;
        left = binary;
    }
    
    return left;
}

// 解析加法表达式
static ASTNode *parse_additive_expr(Parser *parser) {
    ASTNode *left = parse_multiplicative_expr(parser);
    if (!left) return NULL;
    
    while (true) {
        Token *token = current_token(parser);
        if (token->type != TOKEN_PLUS && token->type != TOKEN_MINUS) {
            break;
        }
        
        parser->current++;
        ASTNode *right = parse_multiplicative_expr(parser);
        if (!right) {
            free_ast_node(left);
            return NULL;
        }
        
        ASTNode *binary = create_ast_node(AST_BINARY_EXPR, token->line, token->column);
        binary->data.binary.left = left;
        binary->data.binary.right = right;
        binary->data.binary.op = token->type;
        left = binary;
    }
    
    return left;
}

// 解析移位表达式
static ASTNode *parse_shift_expr(Parser *parser) {
    ASTNode *left = parse_additive_expr(parser);
    if (!left) return NULL;
    
    while (true) {
        Token *token = current_token(parser);
        if (token->type != TOKEN_LEFT_SHIFT && token->type != TOKEN_RIGHT_SHIFT) {
            break;
        }
        
        parser->current++;
        ASTNode *right = parse_additive_expr(parser);
        if (!right) {
            free_ast_node(left);
            return NULL;
        }
        
        ASTNode *binary = create_ast_node(AST_BINARY_EXPR, token->line, token->column);
        binary->data.binary.left = left;
        binary->data.binary.right = right;
        binary->data.binary.op = token->type;
        left = binary;
    }
    
    return left;
}

// 解析关系表达式
static ASTNode *parse_relational_expr(Parser *parser) {
    ASTNode *left = parse_shift_expr(parser);
    if (!left) return NULL;
    
    while (true) {
        Token *token = current_token(parser);
        if (token->type != TOKEN_LESS && 
            token->type != TOKEN_GREATER &&
            token->type != TOKEN_LESS_EQUAL &&
            token->type != TOKEN_GREATER_EQUAL) {
            break;
        }
        
        parser->current++;
        ASTNode *right = parse_shift_expr(parser);
        if (!right) {
            free_ast_node(left);
            return NULL;
        }
        
        ASTNode *binary = create_ast_node(AST_BINARY_EXPR, token->line, token->column);
        binary->data.binary.left = left;
        binary->data.binary.right = right;
        binary->data.binary.op = token->type;
        left = binary;
    }
    
    return left;
}

// 解析相等性表达式
static ASTNode *parse_equality_expr(Parser *parser) {
    ASTNode *left = parse_relational_expr(parser);
    if (!left) return NULL;
    
    while (true) {
        Token *token = current_token(parser);
        if (token->type != TOKEN_EQUAL && token->type != TOKEN_NOT_EQUAL) {
            break;
        }
        
        parser->current++;
        ASTNode *right = parse_relational_expr(parser);
        if (!right) {
            free_ast_node(left);
            return NULL;
        }
        
        ASTNode *binary = create_ast_node(AST_BINARY_EXPR, token->line, token->column);
        binary->data.binary.left = left;
        binary->data.binary.right = right;
        binary->data.binary.op = token->type;
        left = binary;
    }
    
    return left;
}

// 解析按位与表达式
static ASTNode *parse_bitwise_and_expr(Parser *parser) {
    ASTNode *left = parse_equality_expr(parser);
    if (!left) return NULL;
    
    while (match_token(parser, TOKEN_BIT_AND)) {
        Token *token = &parser->tokens[parser->current - 1];
        ASTNode *right = parse_equality_expr(parser);
        if (!right) {
            free_ast_node(left);
            return NULL;
        }
        
        ASTNode *binary = create_ast_node(AST_BINARY_EXPR, token->line, token->column);
        binary->data.binary.left = left;
        binary->data.binary.right = right;
        binary->data.binary.op = TOKEN_BIT_AND;
        left = binary;
    }
    
    return left;
}

// 解析按位异或表达式
static ASTNode *parse_bitwise_xor_expr(Parser *parser) {
    ASTNode *left = parse_bitwise_and_expr(parser);
    if (!left) return NULL;
    
    while (match_token(parser, TOKEN_BIT_XOR)) {
        Token *token = &parser->tokens[parser->current - 1];
        ASTNode *right = parse_bitwise_and_expr(parser);
        if (!right) {
            free_ast_node(left);
            return NULL;
        }
        
        ASTNode *binary = create_ast_node(AST_BINARY_EXPR, token->line, token->column);
        binary->data.binary.left = left;
        binary->data.binary.right = right;
        binary->data.binary.op = TOKEN_BIT_XOR;
        left = binary;
    }
    
    return left;
}

// 解析按位或表达式
static ASTNode *parse_bitwise_or_expr(Parser *parser) {
    ASTNode *left = parse_bitwise_xor_expr(parser);
    if (!left) return NULL;
    
    while (match_token(parser, TOKEN_BIT_OR)) {
        Token *token = &parser->tokens[parser->current - 1];
        ASTNode *right = parse_bitwise_xor_expr(parser);
        if (!right) {
            free_ast_node(left);
            return NULL;
        }
        
        ASTNode *binary = create_ast_node(AST_BINARY_EXPR, token->line, token->column);
        binary->data.binary.left = left;
        binary->data.binary.right = right;
        binary->data.binary.op = TOKEN_BIT_OR;
        left = binary;
    }
    
    return left;
}

// 解析逻辑与表达式
static ASTNode *parse_logical_and_expr(Parser *parser) {
    ASTNode *left = parse_bitwise_or_expr(parser);
    if (!left) return NULL;
    
    while (match_token(parser, TOKEN_LOGICAL_AND)) {
        Token *token = &parser->tokens[parser->current - 1];
        ASTNode *right = parse_bitwise_or_expr(parser);
        if (!right) {
            free_ast_node(left);
            return NULL;
        }
        
        ASTNode *binary = create_ast_node(AST_BINARY_EXPR, token->line, token->column);
        binary->data.binary.left = left;
        binary->data.binary.right = right;
        binary->data.binary.op = TOKEN_LOGICAL_AND;
        left = binary;
    }
    
    return left;
}

// 解析逻辑或表达式
static ASTNode *parse_logical_or_expr(Parser *parser) {
    ASTNode *left = parse_logical_and_expr(parser);
    if (!left) return NULL;
    
    while (match_token(parser, TOKEN_LOGICAL_OR)) {
        Token *token = &parser->tokens[parser->current - 1];
        ASTNode *right = parse_logical_and_expr(parser);
        if (!right) {
            free_ast_node(left);
            return NULL;
        }
        
        ASTNode *binary = create_ast_node(AST_BINARY_EXPR, token->line, token->column);
        binary->data.binary.left = left;
        binary->data.binary.right = right;
        binary->data.binary.op = TOKEN_LOGICAL_OR;
        left = binary;
    }
    
    return left;
}

// 解析条件表达式（三元运算符）
static ASTNode *parse_conditional_expr(Parser *parser) {
    ASTNode *condition = parse_logical_or_expr(parser);
    if (!condition) return NULL;
    
    if (match_token(parser, TOKEN_QUESTION)) {
        Token *token = &parser->tokens[parser->current - 1];
        ASTNode *true_expr = parse_expression(parser);
        if (!true_expr) {
            free_ast_node(condition);
            return NULL;
        }
        
        if (!consume_token(parser, TOKEN_COLON, "期望':'")) {
            free_ast_node(condition);
            free_ast_node(true_expr);
            return NULL;
        }
        
        ASTNode *false_expr = parse_conditional_expr(parser);
        if (!false_expr) {
            free_ast_node(condition);
            free_ast_node(true_expr);
            return NULL;
        }
        
        ASTNode *ternary = create_ast_node(AST_TERNARY_EXPR, token->line, token->column);
        ternary->data.ternary.condition = condition;
        ternary->data.ternary.true_expr = true_expr;
        ternary->data.ternary.false_expr = false_expr;
        return ternary;
    }
    
    return condition;
}

// 解析赋值表达式
static ASTNode *parse_assignment_expr(Parser *parser) {
    ASTNode *left = parse_conditional_expr(parser);
    if (!left) return NULL;
    
    Token *token = current_token(parser);
    
    // 检查是否是赋值操作符
    if (token->type >= TOKEN_ASSIGN && token->type <= TOKEN_BIT_OR_ASSIGN) {
        parser->current++;
        ASTNode *right = parse_assignment_expr(parser);
        if (!right) {
            free_ast_node(left);
            return NULL;
        }
        
        ASTNode *assign = create_ast_node(AST_ASSIGN_EXPR, token->line, token->column);
        assign->data.binary.left = left;
        assign->data.binary.right = right;
        assign->data.binary.op = token->type;
        return assign;
    }
    
    return left;
}

// 解析逗号表达式
static ASTNode *parse_expression(Parser *parser) {
    ASTNode *left = parse_assignment_expr(parser);
    if (!left) return NULL;
    
    if (match_token(parser, TOKEN_COMMA)) {
        Token *token = &parser->tokens[parser->current - 1];
        ASTNode *comma = create_ast_node(AST_COMMA_EXPR, token->line, token->column);
        ASTNode *list = create_list_node(AST_ARG_LIST, token->line, token->column);
        list_add_item(list, left);
        
        do {
            ASTNode *expr = parse_assignment_expr(parser);
            if (!expr) {
                free_ast_node(comma);
                free_ast_node(list);
                return NULL;
            }
            list_add_item(list, expr);
        } while (match_token(parser, TOKEN_COMMA));
        
        comma->data.binary.left = list;
        return comma;
    }
    
    return left;
}

// ==================== 语句解析 ====================

// 前向声明
static ASTNode *parse_statement(Parser *parser);
static ASTNode *parse_compound_statement(Parser *parser);
static ASTNode *parse_declaration(Parser *parser);

// 解析表达式语句
static ASTNode *parse_expression_statement(Parser *parser) {
    Token *start = current_token(parser);
    
    if (match_token(parser, TOKEN_SEMICOLON)) {
        // 空语句
        return create_ast_node(AST_NULL_STMT, start->line, start->column);
    }
    
    ASTNode *expr = parse_expression(parser);
    if (!expr) return NULL;
    
    if (!consume_token(parser, TOKEN_SEMICOLON, "期望';'")) {
        free_ast_node(expr);
        return NULL;
    }
    
    ASTNode *stmt = create_ast_node(AST_EXPR_STMT, start->line, start->column);
    stmt->data.return_stmt.value = expr;  // 复用return_stmt的value字段
    return stmt;
}

// 解析if语句
static ASTNode *parse_if_statement(Parser *parser) {
    Token *if_token = current_token(parser);
    parser->current++;  // 跳过'if'
    
    if (!consume_token(parser, TOKEN_LPAREN, "期望'('")) {
        return NULL;
    }
    
    ASTNode *condition = parse_expression(parser);
    if (!condition) return NULL;
    
    if (!consume_token(parser, TOKEN_RPAREN, "期望')'")) {
        free_ast_node(condition);
        return NULL;
    }
    
    ASTNode *then_stmt = parse_statement(parser);
    if (!then_stmt) {
        free_ast_node(condition);
        return NULL;
    }
    
    ASTNode *else_stmt = NULL;
    if (match_token(parser, TOKEN_ELSE)) {
        else_stmt = parse_statement(parser);
        if (!else_stmt) {
            free_ast_node(condition);
            free_ast_node(then_stmt);
            return NULL;
        }
    }
    
    ASTNode *if_stmt = create_ast_node(AST_IF_STMT, if_token->line, if_token->column);
    if_stmt->data.if_stmt.condition = condition;
    if_stmt->data.if_stmt.then_stmt = then_stmt;
    if_stmt->data.if_stmt.else_stmt = else_stmt;
    return if_stmt;
}

// 解析while语句
static ASTNode *parse_while_statement(Parser *parser) {
    Token *while_token = current_token(parser);
    parser->current++;  // 跳过'while'
    
    if (!consume_token(parser, TOKEN_LPAREN, "期望'('")) {
        return NULL;
    }
    
    ASTNode *condition = parse_expression(parser);
    if (!condition) return NULL;
    
    if (!consume_token(parser, TOKEN_RPAREN, "期望')'")) {
        free_ast_node(condition);
        return NULL;
    }
    
    ASTNode *body = parse_statement(parser);
    if (!body) {
        free_ast_node(condition);
        return NULL;
    }
    
    ASTNode *while_stmt = create_ast_node(AST_WHILE_STMT, while_token->line, while_token->column);
    while_stmt->data.while_stmt.condition = condition;
    while_stmt->data.while_stmt.body = body;
    return while_stmt;
}

// 解析do-while语句
static ASTNode *parse_do_while_statement(Parser *parser) {
    Token *do_token = current_token(parser);
    parser->current++;  // 跳过'do'
    
    ASTNode *body = parse_statement(parser);
    if (!body) return NULL;
    
    if (!consume_token(parser, TOKEN_WHILE, "期望'while'")) {
        free_ast_node(body);
        return NULL;
    }
    
    if (!consume_token(parser, TOKEN_LPAREN, "期望'('")) {
        free_ast_node(body);
        return NULL;
    }
    
    ASTNode *condition = parse_expression(parser);
    if (!condition) {
        free_ast_node(body);
        return NULL;
    }
    
    if (!consume_token(parser, TOKEN_RPAREN, "期望')'")) {
        free_ast_node(body);
        free_ast_node(condition);
        return NULL;
    }
    
    if (!consume_token(parser, TOKEN_SEMICOLON, "期望';'")) {
        free_ast_node(body);
        free_ast_node(condition);
        return NULL;
    }
    
    ASTNode *do_while_stmt = create_ast_node(AST_DO_WHILE_STMT, do_token->line, do_token->column);
    do_while_stmt->data.while_stmt.condition = condition;
    do_while_stmt->data.while_stmt.body = body;
    return do_while_stmt;
}

// 解析for语句
static ASTNode *parse_for_statement(Parser *parser) {
    Token *for_token = current_token(parser);
    parser->current++;  // 跳过'for'
    
    if (!consume_token(parser, TOKEN_LPAREN, "期望'('")) {
        return NULL;
    }
    
    // 初始化部分
    ASTNode *init = NULL;
    if (!check_token(parser, TOKEN_SEMICOLON)) {
        // 可能是声明或表达式
        if (check_token(parser, TOKEN_INT) || check_token(parser, TOKEN_CHAR) ||
            check_token(parser, TOKEN_FLOAT) || check_token(parser, TOKEN_DOUBLE) ||
            check_token(parser, TOKEN_VOID) || check_token(parser, TOKEN_STRUCT) ||
            check_token(parser, TOKEN_UNION) || check_token(parser, TOKEN_ENUM) ||
            check_token(parser, TOKEN_TYPEDEF) || check_token(parser, TOKEN_STATIC) ||
            check_token(parser, TOKEN_EXTERN) || check_token(parser, TOKEN_CONST) ||
            check_token(parser, TOKEN_VOLATILE)) {
            init = parse_declaration(parser);
        } else {
            init = parse_expression(parser);
            if (init && !consume_token(parser, TOKEN_SEMICOLON, "期望';'")) {
                free_ast_node(init);
                return NULL;
            }
        }
    } else {
        parser->current++;  // 跳过';'
    }
    
    // 条件部分
    ASTNode *condition = NULL;
    if (!check_token(parser, TOKEN_SEMICOLON)) {
        condition = parse_expression(parser);
        if (!condition) {
            free_ast_node(init);
            return NULL;
        }
    }
    
    if (!consume_token(parser, TOKEN_SEMICOLON, "期望';'")) {
        free_ast_node(init);
        free_ast_node(condition);
        return NULL;
    }
    
    // 更新部分
    ASTNode *update = NULL;
    if (!check_token(parser, TOKEN_RPAREN)) {
        update = parse_expression(parser);
        if (!update) {
            free_ast_node(init);
            free_ast_node(condition);
            return NULL;
        }
    }
    
    if (!consume_token(parser, TOKEN_RPAREN, "期望')'")) {
        free_ast_node(init);
        free_ast_node(condition);
        free_ast_node(update);
        return NULL;
    }
    
    // 循环体
    ASTNode *body = parse_statement(parser);
    if (!body) {
        free_ast_node(init);
        free_ast_node(condition);
        free_ast_node(update);
        return NULL;
    }
    
    ASTNode *for_stmt = create_ast_node(AST_FOR_STMT, for_token->line, for_token->column);
    for_stmt->data.for_stmt.init = init;
    for_stmt->data.for_stmt.condition = condition;
    for_stmt->data.for_stmt.update = update;
    for_stmt->data.for_stmt.body = body;
    return for_stmt;
}

// 解析return语句
static ASTNode *parse_return_statement(Parser *parser) {
    Token *return_token = current_token(parser);
    parser->current++;  // 跳过'return'
    
    ASTNode *value = NULL;
    if (!check_token(parser, TOKEN_SEMICOLON)) {
        value = parse_expression(parser);
        if (!value) return NULL;
    }
    
    if (!consume_token(parser, TOKEN_SEMICOLON, "期望';'")) {
        free_ast_node(value);
        return NULL;
    }
    
    ASTNode *return_stmt = create_ast_node(AST_RETURN_STMT, return_token->line, return_token->column);
    return_stmt->data.return_stmt.value = value;
    return return_stmt;
}

// 解析break语句
static ASTNode *parse_break_statement(Parser *parser) {
    Token *break_token = current_token(parser);
    parser->current++;  // 跳过'break'
    
    if (!consume_token(parser, TOKEN_SEMICOLON, "期望';'")) {
        return NULL;
    }
    
    return create_ast_node(AST_BREAK_STMT, break_token->line, break_token->column);
}

// 解析continue语句
static ASTNode *parse_continue_statement(Parser *parser) {
    Token *continue_token = current_token(parser);
    parser->current++;  // 跳过'continue'
    
    if (!consume_token(parser, TOKEN_SEMICOLON, "期望';'")) {
        return NULL;
    }
    
    return create_ast_node(AST_CONTINUE_STMT, continue_token->line, continue_token->column);
}

// 解析goto语句
static ASTNode *parse_goto_statement(Parser *parser) {
    Token *goto_token = current_token(parser);
    parser->current++;  // 跳过'goto'
    
    Token *label_token = consume_token(parser, TOKEN_IDENTIFIER, "期望标签名");
    if (!label_token) return NULL;
    
    if (!consume_token(parser, TOKEN_SEMICOLON, "期望';'")) {
        return NULL;
    }
    
    ASTNode *goto_stmt = create_ast_node(AST_GOTO_STMT, goto_token->line, goto_token->column);
    goto_stmt->data.goto_stmt.label = strdup(label_token->value);
    return goto_stmt;
}

// 解析switch语句
static ASTNode *parse_switch_statement(Parser *parser) {
    Token *switch_token = current_token(parser);
    parser->current++;  // 跳过'switch'
    
    if (!consume_token(parser, TOKEN_LPAREN, "期望'('")) {
        return NULL;
    }
    
    ASTNode *expr = parse_expression(parser);
    if (!expr) return NULL;
    
    if (!consume_token(parser, TOKEN_RPAREN, "期望')'")) {
        free_ast_node(expr);
        return NULL;
    }
    
    if (!consume_token(parser, TOKEN_LBRACE, "期望'{'")) {
        free_ast_node(expr);
        return NULL;
    }
    
    ASTNode *cases = create_list_node(AST_STMT_LIST, switch_token->line, switch_token->column);
    
    while (!check_token(parser, TOKEN_RBRACE) && !check_token(parser, TOKEN_EOF)) {
        if (check_token(parser, TOKEN_CASE)) {
            Token *case_token = current_token(parser);
            parser->current++;  // 跳过'case'
            
            ASTNode *value = parse_conditional_expr(parser);
            if (!value) {
                free_ast_node(expr);
                free_ast_node(cases);
                return NULL;
            }
            
            if (!consume_token(parser, TOKEN_COLON, "期望':'")) {
                free_ast_node(expr);
                free_ast_node(cases);
                free_ast_node(value);
                return NULL;
            }
            
            ASTNode *case_stmt = create_ast_node(AST_CASE_STMT, case_token->line, case_token->column);
            case_stmt->data.case_stmt.value = value;
            case_stmt->data.case_stmt.stmts = create_list_node(AST_STMT_LIST, case_token->line, case_token->column);
            
            // 解析case后的语句
            while (!check_token(parser, TOKEN_CASE) && 
                   !check_token(parser, TOKEN_DEFAULT) &&
                   !check_token(parser, TOKEN_RBRACE) &&
                   !check_token(parser, TOKEN_EOF)) {
                ASTNode *stmt = parse_statement(parser);
                if (!stmt) {
                    free_ast_node(expr);
                    free_ast_node(cases);
                    free_ast_node(case_stmt);
                    return NULL;
                }
                list_add_item(case_stmt->data.case_stmt.stmts, stmt);
            }
            
            list_add_item(cases, case_stmt);
        } else if (check_token(parser, TOKEN_DEFAULT)) {
            Token *default_token = current_token(parser);
            parser->current++;  // 跳过'default'
            
            if (!consume_token(parser, TOKEN_COLON, "期望':'")) {
                free_ast_node(expr);
                free_ast_node(cases);
                return NULL;
            }
            
            ASTNode *default_stmt = create_ast_node(AST_DEFAULT_STMT, default_token->line, default_token->column);
            default_stmt->data.case_stmt.value = NULL;
            default_stmt->data.case_stmt.stmts = create_list_node(AST_STMT_LIST, default_token->line, default_token->column);
            
            // 解析default后的语句
            while (!check_token(parser, TOKEN_CASE) && 
                   !check_token(parser, TOKEN_DEFAULT) &&
                   !check_token(parser, TOKEN_RBRACE) &&
                   !check_token(parser, TOKEN_EOF)) {
                ASTNode *stmt = parse_statement(parser);
                if (!stmt) {
                    free_ast_node(expr);
                    free_ast_node(cases);
                    free_ast_node(default_stmt);
                    return NULL;
                }
                list_add_item(default_stmt->data.case_stmt.stmts, stmt);
            }
            
            list_add_item(cases, default_stmt);
        } else {
            // 错误：switch体内只能有case和default
            parser->error_msg = strdup("switch语句体内只能包含case和default标签");
            parser->error_line = current_token(parser)->line;
            parser->error_column = current_token(parser)->column;
            free_ast_node(expr);
            free_ast_node(cases);
            return NULL;
        }
    }
    
    if (!consume_token(parser, TOKEN_RBRACE, "期望'}'")) {
        free_ast_node(expr);
        free_ast_node(cases);
        return NULL;
    }
    
    ASTNode *switch_stmt = create_ast_node(AST_SWITCH_STMT, switch_token->line, switch_token->column);
    switch_stmt->data.switch_stmt.expr = expr;
    switch_stmt->data.switch_stmt.cases = cases;
    return switch_stmt;
}

// 解析复合语句
static ASTNode *parse_compound_statement(Parser *parser) {
    Token *lbrace = current_token(parser);
    if (!consume_token(parser, TOKEN_LBRACE, "期望'{'")) {
        return NULL;
    }
    
    ASTNode *compound = create_ast_node(AST_COMPOUND_STMT, lbrace->line, lbrace->column);
    ASTNode *stmts = create_list_node(AST_STMT_LIST, lbrace->line, lbrace->column);
    compound->data.compound.stmts = stmts;
    
    while (!check_token(parser, TOKEN_RBRACE) && !check_token(parser, TOKEN_EOF)) {
        ASTNode *stmt = NULL;
        
        // 检查是否是声明
        if (check_token(parser, TOKEN_INT) || check_token(parser, TOKEN_CHAR) ||
            check_token(parser, TOKEN_FLOAT) || check_token(parser, TOKEN_DOUBLE) ||
            check_token(parser, TOKEN_VOID) || check_token(parser, TOKEN_STRUCT) ||
            check_token(parser, TOKEN_UNION) || check_token(parser, TOKEN_ENUM) ||
            check_token(parser, TOKEN_TYPEDEF) || check_token(parser, TOKEN_STATIC) ||
            check_token(parser, TOKEN_EXTERN) || check_token(parser, TOKEN_CONST) ||
            check_token(parser, TOKEN_VOLATILE)) {
            stmt = parse_declaration(parser);
        } else {
            stmt = parse_statement(parser);
        }
        
        if (!stmt) {
            free_ast_node(compound);
            return NULL;
        }
        
        list_add_item(stmts, stmt);
    }
    
    if (!consume_token(parser, TOKEN_RBRACE, "期望'}'")) {
        free_ast_node(compound);
        return NULL;
    }
    
    return compound;
}

// 解析语句
static ASTNode *parse_statement(Parser *parser) {
    Token *token = current_token(parser);
    
    switch (token->type) {
        case TOKEN_LBRACE:
            return parse_compound_statement(parser);
            
        case TOKEN_IF:
            return parse_if_statement(parser);
            
        case TOKEN_WHILE:
            return parse_while_statement(parser);
            
        case TOKEN_DO:
            return parse_do_while_statement(parser);
            
        case TOKEN_FOR:
            return parse_for_statement(parser);
            
        case TOKEN_SWITCH:
            return parse_switch_statement(parser);
            
        case TOKEN_RETURN:
            return parse_return_statement(parser);
            
        case TOKEN_BREAK:
            return parse_break_statement(parser);
            
        case TOKEN_CONTINUE:
            return parse_continue_statement(parser);
            
        case TOKEN_GOTO:
            return parse_goto_statement(parser);
            
        case TOKEN_IDENTIFIER:
            // 可能是标签或表达式语句
            if (peek_token(parser, 1)->type == TOKEN_COLON) {
                // 标签语句
                parser->current++;
                parser->current++;  // 跳过':'
                
                ASTNode *stmt = parse_statement(parser);
                if (!stmt) return NULL;
                
                ASTNode *label_stmt = create_ast_node(AST_LABEL_STMT, token->line, token->column);
                label_stmt->data.label_stmt.label = strdup(token->value);
                label_stmt->data.label_stmt.stmt = stmt;
                return label_stmt;
            }
            // 否则继续作为表达式语句处理
            
        default:
            return parse_expression_statement(parser);
    }
}

// ==================== 声明解析 ====================

// 前向声明
static ASTNode *parse_type_specifier(Parser *parser);
static ASTNode *parse_declarator(Parser *parser, ASTNode *base_type);

// 解析类型说明符
static ASTNode *parse_type_specifier(Parser *parser) {
    Token *token = current_token(parser);
    ASTNode *type = create_ast_node(AST_TYPE_SPEC, token->line, token->column);
    
    // 存储类说明符
    int storage_class = 0;
    while (true) {
        if (match_token(parser, TOKEN_TYPEDEF)) {
            storage_class |= 0x01;
        } else if (match_token(parser, TOKEN_EXTERN)) {
            storage_class |= 0x02;
        } else if (match_token(parser, TOKEN_STATIC)) {
            storage_class |= 0x04;
        } else if (match_token(parser, TOKEN_AUTO)) {
            storage_class |= 0x08;
        } else if (match_token(parser, TOKEN_REGISTER)) {
            storage_class |= 0x10;
        } else {
            break;
        }
    }
    
    // 类型限定符
    int qualifiers = 0;
    while (true) {
        if (match_token(parser, TOKEN_CONST)) {
            qualifiers |= 0x01;
        } else if (match_token(parser, TOKEN_VOLATILE)) {
            qualifiers |= 0x02;
        } else if (match_token(parser, TOKEN_RESTRICT)) {
            qualifiers |= 0x04;
        } else {
            break;
        }
    }
    
    // 基本类型
    TokenType basic_type = TOKEN_INT;  // 默认int
    int is_unsigned = 0;
    int is_signed = 0;
    int is_long = 0;
    int is_long_long = 0;
    int is_short = 0;
    
    while (true) {
        token = current_token(parser);
        
        if (match_token(parser, TOKEN_VOID)) {
            basic_type = TOKEN_VOID;
        } else if (match_token(parser, TOKEN_CHAR)) {
            basic_type = TOKEN_CHAR;
        } else if (match_token(parser, TOKEN_SHORT)) {
            is_short = 1;
        } else if (match_token(parser, TOKEN_INT)) {
            basic_type = TOKEN_INT;
        } else if (match_token(parser, TOKEN_LONG)) {
            if (is_long) {
                is_long_long = 1;
            } else {
                is_long = 1;
            }
        } else if (match_token(parser, TOKEN_FLOAT)) {
            basic_type = TOKEN_FLOAT;
        } else if (match_token(parser, TOKEN_DOUBLE)) {
            basic_type = TOKEN_DOUBLE;
        } else if (match_token(parser, TOKEN_SIGNED)) {
            is_signed = 1;
        } else if (match_token(parser, TOKEN_UNSIGNED)) {
            is_unsigned = 1;
        } else if (match_token(parser, TOKEN_STRUCT)) {
            // 结构体类型
            // TODO: 实现结构体解析
            basic_type = TOKEN_STRUCT;
            break;
        } else if (match_token(parser, TOKEN_UNION)) {
            // 联合体类型
            // TODO: 实现联合体解析
            basic_type = TOKEN_UNION;
            break;
        } else if (match_token(parser, TOKEN_ENUM)) {
            // 枚举类型
            // TODO: 实现枚举解析
            basic_type = TOKEN_ENUM;
            break;
        } else if (token->type == TOKEN_IDENTIFIER) {
            // 可能是typedef名称
            // TODO: 检查符号表
            break;
        } else {
            break;
        }
    }
    
    type->data.type_spec.basic_type = basic_type;
    type->data.type_spec.is_unsigned = is_unsigned;
    type->data.type_spec.is_signed = is_signed;
    type->data.type_spec.is_long = is_long;
    type->data.type_spec.is_long_long = is_long_long;
    type->data.type_spec.is_short = is_short;
    type->data.type_spec.qualifiers = qualifiers | (storage_class << 16);
    
    return type;
}

// 解析声明符（变量名、指针、数组、函数等）
static ASTNode *parse_declarator(Parser *parser, ASTNode *base_type) {
    // 处理指针
    ASTNode *type = base_type;
    while (match_token(parser, TOKEN_MULTIPLY)) {
        ASTNode *ptr_type = create_ast_node(AST_POINTER_TYPE, 
                                           current_token(parser)->line, 
                                           current_token(parser)->column);
        ptr_type->data.pointer.base_type = type;
        
        // 指针的类型限定符
        int qualifiers = 0;
        while (true) {
            if (match_token(parser, TOKEN_CONST)) {
                qualifiers |= 0x01;
            } else if (match_token(parser, TOKEN_VOLATILE)) {
                qualifiers |= 0x02;
            } else if (match_token(parser, TOKEN_RESTRICT)) {
                qualifiers |= 0x04;
            } else {
                break;
            }
        }
        ptr_type->data.pointer.qualifiers = qualifiers;
        type = ptr_type;
    }
    
    // 获取标识符
    Token *name_token = consume_token(parser, TOKEN_IDENTIFIER, "期望标识符");
    if (!name_token) {
        return NULL;
    }
    
    // 处理数组和函数
    while (true) {
        if (match_token(parser, TOKEN_LBRACKET)) {
            // 数组
            ASTNode *size = NULL;
            if (!check_token(parser, TOKEN_RBRACKET)) {
                size = parse_conditional_expr(parser);
                if (!size) {
                    return NULL;
                }
            }
            
            if (!consume_token(parser, TOKEN_RBRACKET, "期望']'")) {
                free_ast_node(size);
                return NULL;
            }
            
            ASTNode *array_type = create_ast_node(AST_ARRAY_TYPE,
                                                 name_token->line,
                                                 name_token->column);
            array_type->data.array.element_type = type;
            array_type->data.array.size = size;
            type = array_type;
        } else if (match_token(parser, TOKEN_LPAREN)) {
            // 函数
            ASTNode *params = create_list_node(AST_PARAM_LIST,
                                             name_token->line,
                                             name_token->column);
            
            // 解析参数列表
            if (!check_token(parser, TOKEN_RPAREN)) {
                if (check_token(parser, TOKEN_VOID) && peek_token(parser, 1)->type == TOKEN_RPAREN) {
                    // void参数
                    parser->current++;
                } else {
                    do {
                        ASTNode *param_type = parse_type_specifier(parser);
                        if (!param_type) {
                            free_ast_node(params);
                            return NULL;
                        }
                        
                        // 可能有参数名
                        if (check_token(parser, TOKEN_IDENTIFIER)) {
                            Token *param_name = current_token(parser);
                            parser->current++;
                            
                            ASTNode *param = create_ast_node(AST_PARAM,
                                                            param_name->line,
                                                            param_name->column);
                            param->data.param.type = param_type;
                            param->data.param.name = strdup(param_name->value);
                            list_add_item(params, param);
                        } else {
                            // 抽象声明符（只有类型，没有名字）
                            ASTNode *param = create_ast_node(AST_PARAM,
                                                            param_type->line,
                                                            param_type->column);
                            param->data.param.type = param_type;
                            param->data.param.name = NULL;
                            list_add_item(params, param);
                        }
                    } while (match_token(parser, TOKEN_COMMA));
                    
                    // 检查可变参数
                    if (check_token(parser, TOKEN_COMMA)) {
                        parser->current++;
                        if (!consume_token(parser, TOKEN_ELLIPSIS, "期望'...'")) {
                            free_ast_node(params);
                            return NULL;
                        }
                        // TODO: 标记为可变参数函数
                    }
                }
            }
            
            if (!consume_token(parser, TOKEN_RPAREN, "期望')'")) {
                free_ast_node(params);
                return NULL;
            }
            
            ASTNode *func_type = create_ast_node(AST_FUNCTION_TYPE,
                                               name_token->line,
                                               name_token->column);
            func_type->data.function_type.return_type = type;
            func_type->data.function_type.params = params;
            type = func_type;
        } else {
            break;
        }
    }
    
    // 创建声明节点
    ASTNode *decl = create_ast_node(AST_VAR_DECL, name_token->line, name_token->column);
    decl->data.var_decl.type = type;
    decl->data.var_decl.name = strdup(name_token->value);
    
    return decl;
}

// 解析声明
static ASTNode *parse_declaration(Parser *parser) {
    // 解析类型说明符
    ASTNode *type_spec = parse_type_specifier(parser);
    if (!type_spec) return NULL;
    
    // 检查是否只是类型声明（如struct定义）
    if (match_token(parser, TOKEN_SEMICOLON)) {
        // TODO: 处理纯类型声明
        return type_spec;
    }
    
    // 解析声明符列表
    ASTNode *decl_list = create_list_node(AST_DECL_LIST, 
                                        type_spec->line, 
                                        type_spec->column);
    
    do {
        ASTNode *declarator = parse_declarator(parser, type_spec);
        if (!declarator) {
            free_ast_node(decl_list);
            free_ast_node(type_spec);
            return NULL;
        }
        
        // 处理初始化器
        if (match_token(parser, TOKEN_ASSIGN)) {
            ASTNode *init = parse_assignment_expr(parser);
            if (!init) {
                free_ast_node(decl_list);
                free_ast_node(type_spec);
                free_ast_node(declarator);
                return NULL;
            }
            declarator->data.var_decl.init = init;
        }
        
        // 检查是否是函数定义
        if (check_token(parser, TOKEN_LBRACE)) {
            // 函数定义
            if (declarator->data.var_decl.type->type == AST_FUNCTION_TYPE) {
                ASTNode *body = parse_compound_statement(parser);
                if (!body) {
                    free_ast_node(decl_list);
                    free_ast_node(type_spec);
                    free_ast_node(declarator);
                    return NULL;
                }
                
                // 转换为函数定义节点
                ASTNode *func_def = create_ast_node(AST_FUNCTION_DEF, 
                                                  declarator->line, 
                                                  declarator->column);
                func_def->data.func_decl.return_type = declarator->data.var_decl.type->data.function_type.return_type;
                func_def->data.func_decl.name = declarator->data.var_decl.name;
                func_def->data.func_decl.params = declarator->data.var_decl.type->data.function_type.params;
                func_def->data.func_decl.body = body;
                
                // 设置存储类等属性
                int storage_class = (type_spec->data.type_spec.qualifiers >> 16) & 0xFFFF;
                func_def->data.func_decl.storage_class = storage_class;
                
                free(declarator);  // 只释放外层节点，内部数据已转移
                return func_def;
            }
        }
        
        list_add_item(decl_list, declarator);
        
    } while (match_token(parser, TOKEN_COMMA));
    
    if (!consume_token(parser, TOKEN_SEMICOLON, "期望';'")) {
        free_ast_node(decl_list);
        free_ast_node(type_spec);
        return NULL;
    }
    
    // 如果只有一个声明，直接返回该声明
    if (decl_list->data.list.count == 1) {
        ASTNode *single_decl = decl_list->data.list.items[0];
        free(decl_list->data.list.items);
        free(decl_list);
        return single_decl;
    }
    
    return decl_list;
}

// ==================== 顶层解析 ====================

// 解析翻译单元（整个源文件）
static ASTNode *parse_translation_unit(Parser *parser) {
    ASTNode *unit = create_ast_node(AST_TRANSLATION_UNIT, 1, 1);
    ASTNode *decls = create_list_node(AST_DECL_LIST, 1, 1);
    unit->data.list = decls->data.list;
    free(decls);  // 只要数据，不要节点本身
    
    while (!check_token(parser, TOKEN_EOF)) {
        ASTNode *decl = parse_declaration(parser);
        if (!decl) {
            // 错误恢复：跳到下一个分号或大括号
            while (!check_token(parser, TOKEN_EOF) &&
                   !check_token(parser, TOKEN_SEMICOLON) &&
                   !check_token(parser, TOKEN_LBRACE)) {
                parser->current++;
            }
            if (check_token(parser, TOKEN_SEMICOLON)) {
                parser->current++;
            }
            continue;
        }
        
        list_add_item(unit, decl);
    }
    
    return unit;
}

// ==================== 公共接口 ====================

// 创建解析器
Parser *create_parser(Token *tokens, int token_count) {
    Parser *parser = (Parser *)calloc(1, sizeof(Parser));
    if (!parser) return NULL;
    
    parser->tokens = tokens;
    parser->token_count = token_count;
    parser->current = 0;
    
    // 初始化符号表
    parser->symbols.capacity = 64;
    parser->symbols.names = (char **)calloc(parser->symbols.capacity, sizeof(char *));
    parser->symbols.nodes = (ASTNode **)calloc(parser->symbols.capacity, sizeof(ASTNode *));
    parser->symbols.count = 0;
    
    return parser;
}

// 释放解析器
void free_parser(Parser *parser) {
    if (!parser) return;
    
    // 释放符号表
    for (int i = 0; i < parser->symbols.count; i++) {
        free(parser->symbols.names[i]);
    }
    free(parser->symbols.names);
    free(parser->symbols.nodes);
    
    // 释放错误信息
    free(parser->error_msg);
    
    free(parser);
}

// 执行解析
ASTNode *parse(Parser *parser) {
    return parse_translation_unit(parser);
}

// 打印AST（用于调试）
static void print_ast_indent(ASTNode *node, int indent);

static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

static void print_ast_node(ASTNode *node, int indent) {
    if (!node) {
        print_indent(indent);
        printf("(null)\n");
        return;
    }
    
    print_indent(indent);
    
    switch (node->type) {
        case AST_TRANSLATION_UNIT:
            printf("TranslationUnit\n");
            for (int i = 0; i < node->data.list.count; i++) {
                print_ast_node(node->data.list.items[i], indent + 1);
            }
            break;
            
        case AST_FUNCTION_DEF:
            printf("FunctionDef: %s\n", node->data.func_decl.name);
            print_indent(indent + 1);
            printf("ReturnType:\n");
            print_ast_node(node->data.func_decl.return_type, indent + 2);
            if (node->data.func_decl.params && 
                node->data.func_decl.params->data.list.count > 0) {
                print_indent(indent + 1);
                printf("Parameters:\n");
                print_ast_node(node->data.func_decl.params, indent + 2);
            }
            print_indent(indent + 1);
            printf("Body:\n");
            print_ast_node(node->data.func_decl.body, indent + 2);
            break;
            
        case AST_VAR_DECL:
            printf("VarDecl: %s\n", node->data.var_decl.name);
            print_indent(indent + 1);
            printf("Type:\n");
            print_ast_node(node->data.var_decl.type, indent + 2);
            if (node->data.var_decl.init) {
                print_indent(indent + 1);
                printf("Init:\n");
                print_ast_node(node->data.var_decl.init, indent + 2);
            }
            break;
            
        case AST_TYPE_SPEC:
            printf("TypeSpec: ");
            switch (node->data.type_spec.basic_type) {
                case TOKEN_VOID: printf("void"); break;
                case TOKEN_CHAR: printf("char"); break;
                case TOKEN_INT: printf("int"); break;
                case TOKEN_FLOAT: printf("float"); break;
                case TOKEN_DOUBLE: printf("double"); break;
                default: printf("unknown"); break;
            }
            if (node->data.type_spec.is_unsigned) printf(" unsigned");
            if (node->data.type_spec.is_long) printf(" long");
            if (node->data.type_spec.is_long_long) printf(" long long");
            if (node->data.type_spec.is_short) printf(" short");
            printf("\n");
            break;
            
        case AST_POINTER_TYPE:
            printf("PointerType\n");
            print_ast_node(node->data.pointer.base_type, indent + 1);
            break;
            
        case AST_ARRAY_TYPE:
            printf("ArrayType\n");
            print_indent(indent + 1);
            printf("ElementType:\n");
            print_ast_node(node->data.array.element_type, indent + 2);
            if (node->data.array.size) {
                print_indent(indent + 1);
                printf("Size:\n");
                print_ast_node(node->data.array.size, indent + 2);
            }
            break;
            
        case AST_FUNCTION_TYPE:
            printf("FunctionType\n");
            print_indent(indent + 1);
            printf("ReturnType:\n");
            print_ast_node(node->data.function_type.return_type, indent + 2);
            if (node->data.function_type.params) {
                print_indent(indent + 1);
                printf("Parameters:\n");
                print_ast_node(node->data.function_type.params, indent + 2);
            }
            break;
            
        case AST_PARAM_LIST:
            printf("ParamList\n");
            for (int i = 0; i < node->data.list.count; i++) {
                print_ast_node(node->data.list.items[i], indent + 1);
            }
            break;
            
        case AST_PARAM:
            printf("Param");
            if (node->data.param.name) {
                printf(": %s", node->data.param.name);
            }
            printf("\n");
            if (node->data.param.type) {
                print_ast_node(node->data.param.type, indent + 1);
            }
            break;
            
        case AST_COMPOUND_STMT:
            printf("CompoundStmt\n");
            print_ast_node(node->data.compound.stmts, indent + 1);
            break;
            
        case AST_STMT_LIST:
            printf("StmtList\n");
            for (int i = 0; i < node->data.list.count; i++) {
                print_ast_node(node->data.list.items[i], indent + 1);
            }
            break;
            
        case AST_EXPR_STMT:
            printf("ExprStmt\n");
            print_ast_node(node->data.return_stmt.value, indent + 1);
            break;
            
        case AST_RETURN_STMT:
            printf("ReturnStmt\n");
            if (node->data.return_stmt.value) {
                print_ast_node(node->data.return_stmt.value, indent + 1);
            }
            break;
            
        case AST_IF_STMT:
            printf("IfStmt\n");
            print_indent(indent + 1);
            printf("Condition:\n");
            print_ast_node(node->data.if_stmt.condition, indent + 2);
            print_indent(indent + 1);
            printf("Then:\n");
            print_ast_node(node->data.if_stmt.then_stmt, indent + 2);
            if (node->data.if_stmt.else_stmt) {
                print_indent(indent + 1);
                printf("Else:\n");
                print_ast_node(node->data.if_stmt.else_stmt, indent + 2);
            }
            break;
            
        case AST_WHILE_STMT:
            printf("WhileStmt\n");
            print_indent(indent + 1);
            printf("Condition:\n");
            print_ast_node(node->data.while_stmt.condition, indent + 2);
            print_indent(indent + 1);
            printf("Body:\n");
            print_ast_node(node->data.while_stmt.body, indent + 2);
            break;
            
        case AST_FOR_STMT:
            printf("ForStmt\n");
            if (node->data.for_stmt.init) {
                print_indent(indent + 1);
                printf("Init:\n");
                print_ast_node(node->data.for_stmt.init, indent + 2);
            }
            if (node->data.for_stmt.condition) {
                print_indent(indent + 1);
                printf("Condition:\n");
                print_ast_node(node->data.for_stmt.condition, indent + 2);
            }
            if (node->data.for_stmt.update) {
                print_indent(indent + 1);
                printf("Update:\n");
                print_ast_node(node->data.for_stmt.update, indent + 2);
            }
            print_indent(indent + 1);
            printf("Body:\n");
            print_ast_node(node->data.for_stmt.body, indent + 2);
            break;
            
        case AST_BINARY_EXPR:
            printf("BinaryExpr: ");
            switch (node->data.binary.op) {
                case TOKEN_PLUS: printf("+"); break;
                case TOKEN_MINUS: printf("-"); break;
                case TOKEN_MULTIPLY: printf("*"); break;
                case TOKEN_DIVIDE: printf("/"); break;
                case TOKEN_MOD: printf("%%"); break;
                case TOKEN_EQUAL: printf("=="); break;
                case TOKEN_NOT_EQUAL: printf("!="); break;
                case TOKEN_LESS: printf("<"); break;
                case TOKEN_GREATER: printf(">"); break;
                case TOKEN_LESS_EQUAL: printf("<="); break;
                case TOKEN_GREATER_EQUAL: printf(">="); break;
                case TOKEN_LOGICAL_AND: printf("&&"); break;
                case TOKEN_LOGICAL_OR: printf("||"); break;
                case TOKEN_BIT_AND: printf("&"); break;
                case TOKEN_BIT_OR: printf("|"); break;
                case TOKEN_BIT_XOR: printf("^"); break;
                case TOKEN_LEFT_SHIFT: printf("<<"); break;
                case TOKEN_RIGHT_SHIFT: printf(">>"); break;
                default: printf("?"); break;
            }
            printf("\n");
            print_ast_node(node->data.binary.left, indent + 1);
            print_ast_node(node->data.binary.right, indent + 1);
            break;
            
        case AST_UNARY_EXPR:
            printf("UnaryExpr: ");
            switch (node->data.unary.op) {
                case TOKEN_PLUS: printf("+"); break;
                case TOKEN_MINUS: printf("-"); break;
                case TOKEN_INCREMENT: printf("++"); break;
                case TOKEN_DECREMENT: printf("--"); break;
                case TOKEN_LOGICAL_NOT: printf("!"); break;
                case TOKEN_BIT_NOT: printf("~"); break;
                case TOKEN_MULTIPLY: printf("*"); break;  // 解引用
                case TOKEN_BIT_AND: printf("&"); break;   // 取地址
                default: printf("?"); break;
            }
            if (!node->data.unary.is_prefix) printf(" (postfix)");
            printf("\n");
            print_ast_node(node->data.unary.operand, indent + 1);
            break;
            
        case AST_ASSIGN_EXPR:
            printf("AssignExpr: ");
            switch (node->data.binary.op) {
                case TOKEN_ASSIGN: printf("="); break;
                case TOKEN_ADD_ASSIGN: printf("+="); break;
                case TOKEN_SUB_ASSIGN: printf("-="); break;
                case TOKEN_MUL_ASSIGN: printf("*="); break;
                case TOKEN_DIV_ASSIGN: printf("/="); break;
                case TOKEN_MOD_ASSIGN: printf("%%="); break;
                default: printf("?="); break;
            }
            printf("\n");
            print_ast_node(node->data.binary.left, indent + 1);
            print_ast_node(node->data.binary.right, indent + 1);
            break;
            
        case AST_CALL_EXPR:
            printf("CallExpr\n");
            print_indent(indent + 1);
            printf("Function:\n");
            print_ast_node(node->data.call.func, indent + 2);
            if (node->data.call.args && node->data.call.args->data.list.count > 0) {
                print_indent(indent + 1);
                printf("Arguments:\n");
                print_ast_node(node->data.call.args, indent + 2);
            }
            break;
            
        case AST_ARG_LIST:
            printf("ArgList\n");
            for (int i = 0; i < node->data.list.count; i++) {
                print_ast_node(node->data.list.items[i], indent + 1);
            }
            break;
            
        case AST_IDENTIFIER:
            printf("Identifier: %s\n", node->data.identifier.name);
            break;
            
        case AST_INT_LITERAL:
            printf("IntLiteral: %s (%lld)\n", 
                   node->data.literal.value, 
                   node->data.literal.int_val);
            break;
            
        case AST_FLOAT_LITERAL:
            printf("FloatLiteral: %s (%f)\n", 
                   node->data.literal.value, 
                   node->data.literal.float_val);
            break;
            
        case AST_STRING_LITERAL:
            printf("StringLiteral: %s\n", node->data.literal.value);
            break;
            
        case AST_CHAR_LITERAL:
            printf("CharLiteral: %s (%lld)\n", 
                   node->data.literal.value, 
                   node->data.literal.int_val);
            break;
            
        default:
            printf("UnknownNode (type=%d)\n", node->type);
            break;
    }
}

void print_ast(ASTNode *root) {
    if (!root) {
        printf("AST is NULL\n");
        return;
    }
    
    printf("=== Abstract Syntax Tree ===\n");
    print_ast_node(root, 0);
    printf("===========================\n");
}

// ==================== 测试代码 ====================

#ifdef TEST_PARSER

// 简单的测试词法分析器
static int simple_tokenize(const char *source, Token **out_tokens) {
    // 这里需要实现一个简单的词法分析器
    // 为了测试，我们可以手动创建一些token
    Token *tokens = (Token *)calloc(1000, sizeof(Token));
    int count = 0;
    
    // 示例：手动创建一些token来测试
    // int main() { return 42; }
    
    tokens[count].type = TOKEN_INT;
    tokens[count].value = "int";
    tokens[count].line = 1;
    tokens[count].column = 1;
    count++;
    
    tokens[count].type = TOKEN_IDENTIFIER;
    tokens[count].value = "main";
    tokens[count].line = 1;
    tokens[count].column = 5;
    count++;
    
    tokens[count].type = TOKEN_LPAREN;
    tokens[count].value = "(";
    tokens[count].line = 1;
    tokens[count].column = 9;
    count++;
    
    tokens[count].type = TOKEN_RPAREN;
    tokens[count].value = ")";
    tokens[count].line = 1;
    tokens[count].column = 10;
    count++;
    
    tokens[count].type = TOKEN_LBRACE;
    tokens[count].value = "{";
    tokens[count].line = 1;
    tokens[count].column = 12;
    count++;
    
    tokens[count].type = TOKEN_RETURN;
    tokens[count].value = "return";
    tokens[count].line = 1;
    tokens[count].column = 14;
    count++;
    
    tokens[count].type = TOKEN_NUMBER;
    tokens[count].value = "42";
    tokens[count].line = 1;
    tokens[count].column = 21;
    count++;
    
    tokens[count].type = TOKEN_SEMICOLON;
    tokens[count].value = ";";
    tokens[count].line = 1;
    tokens[count].column = 23;
    count++;
    
    tokens[count].type = TOKEN_RBRACE;
    tokens[count].value = "}";
    tokens[count].line = 1;
    tokens[count].column = 25;
    count++;
    
    tokens[count].type = TOKEN_EOF;
    tokens[count].value = "";
    tokens[count].line = 1;
    tokens[count].column = 26;
    count++;
    
    *out_tokens = tokens;
    return count;
}

int main() {
    const char *source = "int main() { return 42; }";
    
    // 词法分析
    Token *tokens;
    int token_count = simple_tokenize(source, &tokens);
    
    printf("=== Tokens ===\n");
    for (int i = 0; i < token_count; i++) {
        printf("Token %d: type=%d, value='%s', line=%d, col=%d\n",
               i, tokens[i].type, tokens[i].value, tokens[i].line, tokens[i].column);
    }
    printf("\n");
    
    // 语法分析
    Parser *parser = create_parser(tokens, token_count);
    if (!parser) {
        fprintf(stderr, "Failed to create parser\n");
        return 1;
    }
    
    ASTNode *ast = parse(parser);
    if (!ast) {
        fprintf(stderr, "Parse error: %s at line %d, column %d\n",
                parser->error_msg ? parser->error_msg : "Unknown error",
                parser->error_line,
                parser->error_column);
        free_parser(parser);
        free(tokens);
        return 1;
    }
    
    // 打印AST
    print_ast(ast);
    
    // 清理
    free_ast_node(ast);
    free_parser(parser);
    free(tokens);
    
    return 0;
}

#endif // TEST_PARSER