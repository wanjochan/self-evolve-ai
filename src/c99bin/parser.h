/**
 * parser.h - Enhanced C99 Syntax Parser
 * 
 * 完整的C99语法分析器，支持复杂的C语言语法结构
 */

#ifndef C99BIN_PARSER_H
#define C99BIN_PARSER_H

#include "lexer.h"
#include <stdint.h>

// AST节点类型
typedef enum {
    // 表达式节点
    AST_BINARY_OP,          // 二元操作符
    AST_UNARY_OP,           // 一元操作符
    AST_ASSIGNMENT,         // 赋值表达式
    AST_FUNCTION_CALL,      // 函数调用
    AST_IDENTIFIER,         // 标识符
    AST_INTEGER_LITERAL,    // 整数字面量
    AST_FLOAT_LITERAL,      // 浮点字面量
    AST_STRING_LITERAL,     // 字符串字面量
    AST_CHAR_LITERAL,       // 字符字面量
    AST_ARRAY_ACCESS,       // 数组访问
    AST_MEMBER_ACCESS,      // 成员访问
    AST_CONDITIONAL,        // 三元条件表达式
    AST_CAST,               // 类型转换
    AST_SIZEOF,             // sizeof表达式
    
    // 语句节点
    AST_COMPOUND_STMT,      // 复合语句
    AST_IF_STMT,            // if语句
    AST_WHILE_STMT,         // while语句
    AST_FOR_STMT,           // for语句
    AST_DO_WHILE_STMT,      // do-while语句
    AST_SWITCH_STMT,        // switch语句
    AST_CASE_STMT,          // case语句
    AST_DEFAULT_STMT,       // default语句
    AST_BREAK_STMT,         // break语句
    AST_CONTINUE_STMT,      // continue语句
    AST_RETURN_STMT,        // return语句
    AST_GOTO_STMT,          // goto语句
    AST_LABEL_STMT,         // 标签语句
    AST_EXPRESSION_STMT,    // 表达式语句
    
    // 声明节点
    AST_VARIABLE_DECL,      // 变量声明
    AST_FUNCTION_DECL,      // 函数声明
    AST_FUNCTION_DEF,       // 函数定义
    AST_STRUCT_DECL,        // 结构体声明
    AST_UNION_DECL,         // 联合体声明
    AST_ENUM_DECL,          // 枚举声明
    AST_TYPEDEF_DECL,       // 类型定义
    
    // 类型节点
    AST_POINTER_TYPE,       // 指针类型
    AST_ARRAY_TYPE,         // 数组类型
    AST_FUNCTION_TYPE,      // 函数类型
    AST_STRUCT_TYPE,        // 结构体类型
    AST_UNION_TYPE,         // 联合体类型
    AST_ENUM_TYPE,          // 枚举类型
    AST_BASIC_TYPE,         // 基本类型
    
    // 程序结构
    AST_TRANSLATION_UNIT,   // 翻译单元
    AST_PARAMETER_LIST,     // 参数列表
    AST_ARGUMENT_LIST,      // 参数列表
    AST_INITIALIZER_LIST,   // 初始化列表
    
    AST_ERROR               // 错误节点
} ASTNodeType;

// 二元操作符类型
typedef enum {
    BIN_OP_ADD, BIN_OP_SUB, BIN_OP_MUL, BIN_OP_DIV, BIN_OP_MOD,
    BIN_OP_EQ, BIN_OP_NE, BIN_OP_LT, BIN_OP_LE, BIN_OP_GT, BIN_OP_GE,
    BIN_OP_AND, BIN_OP_OR, BIN_OP_BITWISE_AND, BIN_OP_BITWISE_OR, BIN_OP_BITWISE_XOR,
    BIN_OP_LEFT_SHIFT, BIN_OP_RIGHT_SHIFT,
    BIN_OP_ASSIGN, BIN_OP_ADD_ASSIGN, BIN_OP_SUB_ASSIGN, BIN_OP_MUL_ASSIGN,
    BIN_OP_DIV_ASSIGN, BIN_OP_MOD_ASSIGN
} BinaryOpType;

// 一元操作符类型
typedef enum {
    UNARY_OP_PLUS, UNARY_OP_MINUS, UNARY_OP_NOT, UNARY_OP_BITWISE_NOT,
    UNARY_OP_PRE_INC, UNARY_OP_PRE_DEC, UNARY_OP_POST_INC, UNARY_OP_POST_DEC,
    UNARY_OP_ADDR, UNARY_OP_DEREF
} UnaryOpType;

// 基本类型
typedef enum {
    TYPE_VOID, TYPE_CHAR, TYPE_SHORT, TYPE_INT, TYPE_LONG, TYPE_LONG_LONG,
    TYPE_FLOAT, TYPE_DOUBLE, TYPE_LONG_DOUBLE,
    TYPE_UNSIGNED_CHAR, TYPE_UNSIGNED_SHORT, TYPE_UNSIGNED_INT, 
    TYPE_UNSIGNED_LONG, TYPE_UNSIGNED_LONG_LONG,
    TYPE_BOOL, TYPE_COMPLEX, TYPE_IMAGINARY
} BasicType;

// 前向声明
typedef struct ASTNode ASTNode;

// AST节点结构
struct ASTNode {
    ASTNodeType type;
    int line;
    int column;
    
    union {
        // 二元操作
        struct {
            BinaryOpType op;
            ASTNode* left;
            ASTNode* right;
        } binary_op;
        
        // 一元操作
        struct {
            UnaryOpType op;
            ASTNode* operand;
        } unary_op;
        
        // 函数调用
        struct {
            ASTNode* function;
            ASTNode* arguments;  // AST_ARGUMENT_LIST
        } function_call;
        
        // 标识符
        struct {
            char* name;
        } identifier;
        
        // 字面量
        struct {
            union {
                long long int_value;
                double float_value;
                char* string_value;
                char char_value;
            };
        } literal;
        
        // 复合语句
        struct {
            ASTNode** statements;
            int statement_count;
            int capacity;
        } compound;
        
        // if语句
        struct {
            ASTNode* condition;
            ASTNode* then_stmt;
            ASTNode* else_stmt;  // 可选
        } if_stmt;
        
        // while语句
        struct {
            ASTNode* condition;
            ASTNode* body;
        } while_stmt;
        
        // for语句
        struct {
            ASTNode* init;       // 可选
            ASTNode* condition;  // 可选
            ASTNode* update;     // 可选
            ASTNode* body;
        } for_stmt;
        
        // return语句
        struct {
            ASTNode* expression;  // 可选
        } return_stmt;
        
        // 变量声明
        struct {
            BasicType type;
            char* name;
            ASTNode* initializer;  // 可选
            int is_pointer;
            int pointer_level;
        } var_decl;
        
        // 函数定义
        struct {
            BasicType return_type;
            char* name;
            ASTNode* parameters;   // AST_PARAMETER_LIST
            ASTNode* body;         // AST_COMPOUND_STMT
            int return_is_pointer;
        } function_def;
        
        // 参数列表
        struct {
            ASTNode** parameters;
            int parameter_count;
            int capacity;
        } parameter_list;
        
        // 参数列表
        struct {
            ASTNode** arguments;
            int argument_count;
            int capacity;
        } argument_list;
    };
};

// 语法分析器状态
typedef struct {
    Lexer* lexer;
    Token* current_token;
    Token* next_token;
    int error_count;
    char error_msg[512];
    
    // 解析状态
    int in_function;
    int in_loop;
    int in_switch;
} Parser;

// 语法分析器函数接口
Parser* parser_create(Lexer* lexer);
void parser_destroy(Parser* parser);
ASTNode* parser_parse(Parser* parser);
void ast_destroy(ASTNode* node);

// AST节点创建函数
ASTNode* ast_create_binary_op(BinaryOpType op, ASTNode* left, ASTNode* right, int line, int column);
ASTNode* ast_create_unary_op(UnaryOpType op, ASTNode* operand, int line, int column);
ASTNode* ast_create_identifier(const char* name, int line, int column);
ASTNode* ast_create_integer_literal(long long value, int line, int column);
ASTNode* ast_create_float_literal(double value, int line, int column);
ASTNode* ast_create_string_literal(const char* value, int line, int column);
ASTNode* ast_create_compound_stmt(int line, int column);
ASTNode* ast_create_if_stmt(ASTNode* condition, ASTNode* then_stmt, ASTNode* else_stmt, int line, int column);
ASTNode* ast_create_while_stmt(ASTNode* condition, ASTNode* body, int line, int column);
ASTNode* ast_create_for_stmt(ASTNode* init, ASTNode* condition, ASTNode* update, ASTNode* body, int line, int column);
ASTNode* ast_create_return_stmt(ASTNode* expression, int line, int column);
ASTNode* ast_create_function_call(ASTNode* function, ASTNode* arguments, int line, int column);
ASTNode* ast_create_var_decl(BasicType type, const char* name, ASTNode* initializer, int line, int column);
ASTNode* ast_create_function_def(BasicType return_type, const char* name, ASTNode* parameters, ASTNode* body, int line, int column);

// 复合语句辅助函数
void ast_compound_add_statement(ASTNode* compound, ASTNode* statement);
void ast_parameter_list_add(ASTNode* param_list, ASTNode* parameter);
void ast_argument_list_add(ASTNode* arg_list, ASTNode* argument);

// 解析函数（递归下降解析器）
ASTNode* parse_translation_unit(Parser* parser);
ASTNode* parse_external_declaration(Parser* parser);
ASTNode* parse_function_definition(Parser* parser);
ASTNode* parse_declaration(Parser* parser);
ASTNode* parse_statement(Parser* parser);
ASTNode* parse_compound_statement(Parser* parser);
ASTNode* parse_expression_statement(Parser* parser);
ASTNode* parse_if_statement(Parser* parser);
ASTNode* parse_while_statement(Parser* parser);
ASTNode* parse_for_statement(Parser* parser);
ASTNode* parse_return_statement(Parser* parser);
ASTNode* parse_expression(Parser* parser);
ASTNode* parse_assignment_expression(Parser* parser);
ASTNode* parse_logical_or_expression(Parser* parser);
ASTNode* parse_logical_and_expression(Parser* parser);
ASTNode* parse_equality_expression(Parser* parser);
ASTNode* parse_relational_expression(Parser* parser);
ASTNode* parse_additive_expression(Parser* parser);
ASTNode* parse_multiplicative_expression(Parser* parser);
ASTNode* parse_unary_expression(Parser* parser);
ASTNode* parse_postfix_expression(Parser* parser);
ASTNode* parse_primary_expression(Parser* parser);
ASTNode* parse_parameter_list(Parser* parser);
ASTNode* parse_argument_list(Parser* parser);

// 工具函数
int parser_has_error(const Parser* parser);
const char* parser_get_error(const Parser* parser);
void parser_error(Parser* parser, const char* message);
int parser_expect_token(Parser* parser, TokenType expected);
void parser_advance(Parser* parser);
TokenType parser_peek(const Parser* parser);

// AST遍历和调试
void ast_print(const ASTNode* node, int indent);
const char* ast_node_type_name(ASTNodeType type);
const char* binary_op_name(BinaryOpType op);
const char* unary_op_name(UnaryOpType op);

// 类型信息
BasicType token_to_basic_type(TokenType token);
const char* basic_type_name(BasicType type);

#endif // C99BIN_PARSER_H