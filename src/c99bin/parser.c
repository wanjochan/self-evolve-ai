/**
 * parser.c - Enhanced C99 Syntax Parser Implementation
 * 
 * 完整的C99语法分析器实现，使用递归下降解析器
 */

#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// 前向声明
ASTNode* parse_function_definition_with_type(Parser* parser, BasicType return_type, const char* name);
ASTNode* ast_create_node(ASTNodeType type, int line, int column);

// AST节点类型名称表
static const char* ast_node_names[] = {
    "BINARY_OP", "UNARY_OP", "ASSIGNMENT", "FUNCTION_CALL", "IDENTIFIER",
    "INTEGER_LITERAL", "FLOAT_LITERAL", "STRING_LITERAL", "CHAR_LITERAL",
    "ARRAY_ACCESS", "MEMBER_ACCESS", "CONDITIONAL", "CAST", "SIZEOF",
    "COMPOUND_STMT", "IF_STMT", "WHILE_STMT", "FOR_STMT", "DO_WHILE_STMT",
    "SWITCH_STMT", "CASE_STMT", "DEFAULT_STMT", "BREAK_STMT", "CONTINUE_STMT",
    "RETURN_STMT", "GOTO_STMT", "LABEL_STMT", "EXPRESSION_STMT",
    "VARIABLE_DECL", "FUNCTION_DECL", "FUNCTION_DEF", "STRUCT_DECL",
    "UNION_DECL", "ENUM_DECL", "TYPEDEF_DECL",
    "POINTER_TYPE", "ARRAY_TYPE", "FUNCTION_TYPE", "STRUCT_TYPE",
    "UNION_TYPE", "ENUM_TYPE", "BASIC_TYPE",
    "TRANSLATION_UNIT", "PARAMETER_LIST", "ARGUMENT_LIST", "INITIALIZER_LIST",
    "ERROR"
};

// 二元操作符名称
static const char* binary_op_names[] = {
    "+", "-", "*", "/", "%", "==", "!=", "<", "<=", ">", ">=",
    "&&", "||", "&", "|", "^", "<<", ">>",
    "=", "+=", "-=", "*=", "/=", "%="
};

// 一元操作符名称
static const char* unary_op_names[] = {
    "+", "-", "!", "~", "++", "--", "++", "--", "&", "*"
};

// 基本类型名称
static const char* basic_type_names[] = {
    "void", "char", "short", "int", "long", "long long",
    "float", "double", "long double",
    "unsigned char", "unsigned short", "unsigned int",
    "unsigned long", "unsigned long long",
    "_Bool", "_Complex", "_Imaginary"
};

// 创建语法分析器
Parser* parser_create(Lexer* lexer) {
    if (!lexer) return NULL;
    
    Parser* parser = malloc(sizeof(Parser));
    if (!parser) return NULL;
    
    parser->lexer = lexer;
    parser->current_token = NULL;
    parser->next_token = NULL;
    parser->error_count = 0;
    parser->error_msg[0] = '\0';
    parser->in_function = 0;
    parser->in_loop = 0;
    parser->in_switch = 0;
    
    // 预读两个token
    parser_advance(parser);
    parser_advance(parser);
    
    return parser;
}

// 销毁语法分析器
void parser_destroy(Parser* parser) {
    if (parser) {
        if (parser->current_token) {
            token_destroy(parser->current_token);
        }
        if (parser->next_token) {
            token_destroy(parser->next_token);
        }
        free(parser);
    }
}

// 前进到下一个token
void parser_advance(Parser* parser) {
    if (!parser) return;
    
    if (parser->current_token) {
        token_destroy(parser->current_token);
    }
    
    parser->current_token = parser->next_token;
    parser->next_token = lexer_next_token(parser->lexer);
}

// 查看当前token类型
TokenType parser_peek(const Parser* parser) {
    return parser && parser->current_token ? parser->current_token->type : TOKEN_EOF;
}

// 检查并消费期望的token
int parser_expect_token(Parser* parser, TokenType expected) {
    if (!parser || !parser->current_token) {
        parser_error(parser, "Unexpected end of input");
        return 0;
    }
    
    if (parser->current_token->type != expected) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Expected '%s', got '%s'", 
                token_type_name(expected), token_type_name(parser->current_token->type));
        parser_error(parser, msg);
        return 0;
    }
    
    parser_advance(parser);
    return 1;
}

// 错误处理
void parser_error(Parser* parser, const char* message) {
    if (parser) {
        parser->error_count++;
        if (parser->current_token) {
            snprintf(parser->error_msg, sizeof(parser->error_msg),
                    "Line %d, Column %d: %s", 
                    parser->current_token->line, parser->current_token->column, message);
        } else {
            snprintf(parser->error_msg, sizeof(parser->error_msg), "Parser error: %s", message);
        }
    }
}

int parser_has_error(const Parser* parser) {
    return parser && parser->error_count > 0;
}

const char* parser_get_error(const Parser* parser) {
    return parser ? parser->error_msg : "No parser";
}

// 创建AST节点的基础函数
ASTNode* ast_create_node(ASTNodeType type, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) return NULL;
    
    memset(node, 0, sizeof(ASTNode));
    node->type = type;
    node->line = line;
    node->column = column;
    
    return node;
}

// AST节点创建函数实现
ASTNode* ast_create_binary_op(BinaryOpType op, ASTNode* left, ASTNode* right, int line, int column) {
    ASTNode* node = ast_create_node(AST_BINARY_OP, line, column);
    if (node) {
        node->binary_op.op = op;
        node->binary_op.left = left;
        node->binary_op.right = right;
    }
    return node;
}

ASTNode* ast_create_unary_op(UnaryOpType op, ASTNode* operand, int line, int column) {
    ASTNode* node = ast_create_node(AST_UNARY_OP, line, column);
    if (node) {
        node->unary_op.op = op;
        node->unary_op.operand = operand;
    }
    return node;
}

ASTNode* ast_create_identifier(const char* name, int line, int column) {
    ASTNode* node = ast_create_node(AST_IDENTIFIER, line, column);
    if (node && name) {
        node->identifier.name = malloc(strlen(name) + 1);
        if (node->identifier.name) {
            strcpy(node->identifier.name, name);
        }
    }
    return node;
}

ASTNode* ast_create_integer_literal(long long value, int line, int column) {
    ASTNode* node = ast_create_node(AST_INTEGER_LITERAL, line, column);
    if (node) {
        node->literal.int_value = value;
    }
    return node;
}

ASTNode* ast_create_float_literal(double value, int line, int column) {
    ASTNode* node = ast_create_node(AST_FLOAT_LITERAL, line, column);
    if (node) {
        node->literal.float_value = value;
    }
    return node;
}

ASTNode* ast_create_string_literal(const char* value, int line, int column) {
    ASTNode* node = ast_create_node(AST_STRING_LITERAL, line, column);
    if (node && value) {
        node->literal.string_value = malloc(strlen(value) + 1);
        if (node->literal.string_value) {
            strcpy(node->literal.string_value, value);
        }
    }
    return node;
}

ASTNode* ast_create_compound_stmt(int line, int column) {
    ASTNode* node = ast_create_node(AST_COMPOUND_STMT, line, column);
    if (node) {
        node->compound.statements = NULL;
        node->compound.statement_count = 0;
        node->compound.capacity = 0;
    }
    return node;
}

ASTNode* ast_create_if_stmt(ASTNode* condition, ASTNode* then_stmt, ASTNode* else_stmt, int line, int column) {
    ASTNode* node = ast_create_node(AST_IF_STMT, line, column);
    if (node) {
        node->if_stmt.condition = condition;
        node->if_stmt.then_stmt = then_stmt;
        node->if_stmt.else_stmt = else_stmt;
    }
    return node;
}

ASTNode* ast_create_while_stmt(ASTNode* condition, ASTNode* body, int line, int column) {
    ASTNode* node = ast_create_node(AST_WHILE_STMT, line, column);
    if (node) {
        node->while_stmt.condition = condition;
        node->while_stmt.body = body;
    }
    return node;
}

ASTNode* ast_create_for_stmt(ASTNode* init, ASTNode* condition, ASTNode* update, ASTNode* body, int line, int column) {
    ASTNode* node = ast_create_node(AST_FOR_STMT, line, column);
    if (node) {
        node->for_stmt.init = init;
        node->for_stmt.condition = condition;
        node->for_stmt.update = update;
        node->for_stmt.body = body;
    }
    return node;
}

ASTNode* ast_create_return_stmt(ASTNode* expression, int line, int column) {
    ASTNode* node = ast_create_node(AST_RETURN_STMT, line, column);
    if (node) {
        node->return_stmt.expression = expression;
    }
    return node;
}

ASTNode* ast_create_function_call(ASTNode* function, ASTNode* arguments, int line, int column) {
    ASTNode* node = ast_create_node(AST_FUNCTION_CALL, line, column);
    if (node) {
        node->function_call.function = function;
        node->function_call.arguments = arguments;
    }
    return node;
}

ASTNode* ast_create_var_decl(BasicType type, const char* name, ASTNode* initializer, int line, int column) {
    ASTNode* node = ast_create_node(AST_VARIABLE_DECL, line, column);
    if (node) {
        node->var_decl.type = type;
        if (name) {
            node->var_decl.name = malloc(strlen(name) + 1);
            if (node->var_decl.name) {
                strcpy(node->var_decl.name, name);
            }
        }
        node->var_decl.initializer = initializer;
        node->var_decl.is_pointer = 0;
        node->var_decl.pointer_level = 0;
    }
    return node;
}

ASTNode* ast_create_function_def(BasicType return_type, const char* name, ASTNode* parameters, ASTNode* body, int line, int column) {
    ASTNode* node = ast_create_node(AST_FUNCTION_DEF, line, column);
    if (node) {
        node->function_def.return_type = return_type;
        if (name) {
            node->function_def.name = malloc(strlen(name) + 1);
            if (node->function_def.name) {
                strcpy(node->function_def.name, name);
            }
        }
        node->function_def.parameters = parameters;
        node->function_def.body = body;
        node->function_def.return_is_pointer = 0;
    }
    return node;
}

// 复合语句辅助函数
void ast_compound_add_statement(ASTNode* compound, ASTNode* statement) {
    if (!compound || compound->type != AST_COMPOUND_STMT || !statement) {
        return;
    }
    
    // 动态扩展数组
    if (compound->compound.statement_count >= compound->compound.capacity) {
        int new_capacity = compound->compound.capacity == 0 ? 4 : compound->compound.capacity * 2;
        ASTNode** new_statements = realloc(compound->compound.statements, 
                                          new_capacity * sizeof(ASTNode*));
        if (!new_statements) return;
        
        compound->compound.statements = new_statements;
        compound->compound.capacity = new_capacity;
    }
    
    compound->compound.statements[compound->compound.statement_count++] = statement;
}

// Token到基本类型转换
BasicType token_to_basic_type(TokenType token) {
    switch (token) {
        case TOKEN_VOID: return TYPE_VOID;
        case TOKEN_CHAR: return TYPE_CHAR;
        case TOKEN_SHORT: return TYPE_SHORT;
        case TOKEN_INT: return TYPE_INT;
        case TOKEN_LONG: return TYPE_LONG;
        case TOKEN_FLOAT: return TYPE_FLOAT;
        case TOKEN_DOUBLE: return TYPE_DOUBLE;
        case TOKEN_SIGNED: return TYPE_INT;
        case TOKEN_UNSIGNED: return TYPE_UNSIGNED_INT;
        case TOKEN__BOOL: return TYPE_BOOL;
        case TOKEN__COMPLEX: return TYPE_COMPLEX;
        case TOKEN__IMAGINARY: return TYPE_IMAGINARY;
        default: return TYPE_INT;
    }
}

// 解析主入口
ASTNode* parser_parse(Parser* parser) {
    if (!parser) return NULL;
    return parse_translation_unit(parser);
}

// 解析翻译单元（整个程序）
ASTNode* parse_translation_unit(Parser* parser) {
    ASTNode* unit = ast_create_node(AST_TRANSLATION_UNIT, 1, 1);
    if (!unit) return NULL;
    
    // 初始化为复合语句来存储外部声明
    unit->compound.statements = NULL;
    unit->compound.statement_count = 0;
    unit->compound.capacity = 0;
    
    while (parser_peek(parser) != TOKEN_EOF) {
        ASTNode* external_decl = parse_external_declaration(parser);
        if (external_decl) {
            ast_compound_add_statement(unit, external_decl);
        } else {
            // 错误恢复：跳过到下一个分号或大括号
            while (parser_peek(parser) != TOKEN_EOF && 
                   parser_peek(parser) != TOKEN_SEMICOLON &&
                   parser_peek(parser) != TOKEN_RIGHT_BRACE) {
                parser_advance(parser);
            }
            if (parser_peek(parser) == TOKEN_SEMICOLON) {
                parser_advance(parser);
            }
        }
    }
    
    return unit;
}

// 解析外部声明（函数定义或变量声明）
ASTNode* parse_external_declaration(Parser* parser) {
    // 简化版本：只处理函数定义
    TokenType current = parser_peek(parser);
    
    // 跳过预处理器指令
    if (current == TOKEN_HASH) {
        while (parser_peek(parser) != TOKEN_NEWLINE && parser_peek(parser) != TOKEN_EOF) {
            parser_advance(parser);
        }
        if (parser_peek(parser) == TOKEN_NEWLINE) {
            parser_advance(parser);
        }
        return parse_external_declaration(parser);
    }
    
    // 检查是否为函数定义
    BasicType type = TYPE_INT;  // 默认类型
    if (current == TOKEN_INT || current == TOKEN_VOID || current == TOKEN_CHAR ||
        current == TOKEN_FLOAT || current == TOKEN_DOUBLE) {
        type = token_to_basic_type(current);
        parser_advance(parser);
    }
    
    if (parser_peek(parser) == TOKEN_IDENTIFIER) {
        Token* name_token = parser->current_token;
        parser_advance(parser);
        
        if (parser_peek(parser) == TOKEN_LEFT_PAREN) {
            // 函数定义
            return parse_function_definition_with_type(parser, type, name_token->value);
        } else {
            // 变量声明（简化处理）
            ASTNode* var_decl = ast_create_var_decl(type, name_token->value, NULL,
                                                   name_token->line, name_token->column);
            if (parser_peek(parser) == TOKEN_SEMICOLON) {
                parser_advance(parser);
            }
            return var_decl;
        }
    }
    
    parser_error(parser, "Expected external declaration");
    return NULL;
}

// 解析函数定义（带已知类型和名称）
ASTNode* parse_function_definition_with_type(Parser* parser, BasicType return_type, const char* name) {
    int line = parser->current_token ? parser->current_token->line : 1;
    int column = parser->current_token ? parser->current_token->column : 1;
    
    // 解析参数列表
    if (!parser_expect_token(parser, TOKEN_LEFT_PAREN)) {
        return NULL;
    }
    
    ASTNode* parameters = parse_parameter_list(parser);
    
    if (!parser_expect_token(parser, TOKEN_RIGHT_PAREN)) {
        if (parameters) ast_destroy(parameters);
        return NULL;
    }
    
    // 解析函数体
    ASTNode* body = parse_compound_statement(parser);
    if (!body) {
        if (parameters) ast_destroy(parameters);
        return NULL;
    }
    
    return ast_create_function_def(return_type, name, parameters, body, line, column);
}

// 解析参数列表
ASTNode* parse_parameter_list(Parser* parser) {
    ASTNode* param_list = ast_create_node(AST_PARAMETER_LIST, 
                                         parser->current_token->line, 
                                         parser->current_token->column);
    if (!param_list) return NULL;
    
    param_list->parameter_list.parameters = NULL;
    param_list->parameter_list.parameter_count = 0;
    param_list->parameter_list.capacity = 0;
    
    // 空参数列表
    if (parser_peek(parser) == TOKEN_RIGHT_PAREN) {
        return param_list;
    }
    
    // 解析参数
    do {
        // 简化：只处理基本类型参数
        BasicType type = TYPE_INT;
        if (parser_peek(parser) == TOKEN_INT || parser_peek(parser) == TOKEN_VOID ||
            parser_peek(parser) == TOKEN_CHAR || parser_peek(parser) == TOKEN_FLOAT) {
            type = token_to_basic_type(parser_peek(parser));
            parser_advance(parser);
        }
        
        if (parser_peek(parser) == TOKEN_IDENTIFIER) {
            ASTNode* param = ast_create_var_decl(type, parser->current_token->value, NULL,
                                               parser->current_token->line, 
                                               parser->current_token->column);
            ast_parameter_list_add(param_list, param);
            parser_advance(parser);
        }
        
        if (parser_peek(parser) == TOKEN_COMMA) {
            parser_advance(parser);
        } else {
            break;
        }
    } while (parser_peek(parser) != TOKEN_RIGHT_PAREN && parser_peek(parser) != TOKEN_EOF);
    
    return param_list;
}

void ast_parameter_list_add(ASTNode* param_list, ASTNode* parameter) {
    if (!param_list || param_list->type != AST_PARAMETER_LIST || !parameter) {
        return;
    }
    
    // 动态扩展数组
    if (param_list->parameter_list.parameter_count >= param_list->parameter_list.capacity) {
        int new_capacity = param_list->parameter_list.capacity == 0 ? 4 : param_list->parameter_list.capacity * 2;
        ASTNode** new_parameters = realloc(param_list->parameter_list.parameters, 
                                          new_capacity * sizeof(ASTNode*));
        if (!new_parameters) return;
        
        param_list->parameter_list.parameters = new_parameters;
        param_list->parameter_list.capacity = new_capacity;
    }
    
    param_list->parameter_list.parameters[param_list->parameter_list.parameter_count++] = parameter;
}

// 解析复合语句
ASTNode* parse_compound_statement(Parser* parser) {
    if (!parser_expect_token(parser, TOKEN_LEFT_BRACE)) {
        return NULL;
    }
    
    ASTNode* compound = ast_create_compound_stmt(parser->current_token->line, 
                                               parser->current_token->column);
    if (!compound) return NULL;
    
    while (parser_peek(parser) != TOKEN_RIGHT_BRACE && parser_peek(parser) != TOKEN_EOF) {
        ASTNode* stmt = parse_statement(parser);
        if (stmt) {
            ast_compound_add_statement(compound, stmt);
        } else {
            // 错误恢复
            while (parser_peek(parser) != TOKEN_SEMICOLON &&
                   parser_peek(parser) != TOKEN_RIGHT_BRACE &&
                   parser_peek(parser) != TOKEN_EOF) {
                parser_advance(parser);
            }
            if (parser_peek(parser) == TOKEN_SEMICOLON) {
                parser_advance(parser);
            }
        }
    }
    
    if (!parser_expect_token(parser, TOKEN_RIGHT_BRACE)) {
        ast_destroy(compound);
        return NULL;
    }
    
    return compound;
}

// 解析语句
ASTNode* parse_statement(Parser* parser) {
    switch (parser_peek(parser)) {
        case TOKEN_LEFT_BRACE:
            return parse_compound_statement(parser);
        case TOKEN_IF:
            return parse_if_statement(parser);
        case TOKEN_WHILE:
            return parse_while_statement(parser);
        case TOKEN_FOR:
            return parse_for_statement(parser);
        case TOKEN_RETURN:
            return parse_return_statement(parser);
        case TOKEN_INT:
        case TOKEN_CHAR:
        case TOKEN_FLOAT:
        case TOKEN_DOUBLE:
        case TOKEN_VOID:
            return parse_declaration(parser);
        default:
            return parse_expression_statement(parser);
    }
}

// 声明前向函数
ASTNode* parse_function_definition_with_type(Parser* parser, BasicType return_type, const char* name);

// 实现名称查找函数
const char* ast_node_type_name(ASTNodeType type) {
    if (type >= 0 && type < sizeof(ast_node_names) / sizeof(ast_node_names[0])) {
        return ast_node_names[type];
    }
    return "UNKNOWN";
}

const char* binary_op_name(BinaryOpType op) {
    if (op >= 0 && op < sizeof(binary_op_names) / sizeof(binary_op_names[0])) {
        return binary_op_names[op];
    }
    return "UNKNOWN";
}

const char* unary_op_name(UnaryOpType op) {
    if (op >= 0 && op < sizeof(unary_op_names) / sizeof(unary_op_names[0])) {
        return unary_op_names[op];
    }
    return "UNKNOWN";
}

const char* basic_type_name(BasicType type) {
    if (type >= 0 && type < sizeof(basic_type_names) / sizeof(basic_type_names[0])) {
        return basic_type_names[type];
    }
    return "UNKNOWN";
}

// AST销毁函数
void ast_destroy(ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_BINARY_OP:
            ast_destroy(node->binary_op.left);
            ast_destroy(node->binary_op.right);
            break;
        case AST_UNARY_OP:
            ast_destroy(node->unary_op.operand);
            break;
        case AST_IDENTIFIER:
            free(node->identifier.name);
            break;
        case AST_STRING_LITERAL:
            free(node->literal.string_value);
            break;
        case AST_COMPOUND_STMT:
        case AST_TRANSLATION_UNIT:
            for (int i = 0; i < node->compound.statement_count; i++) {
                ast_destroy(node->compound.statements[i]);
            }
            free(node->compound.statements);
            break;
        case AST_IF_STMT:
            ast_destroy(node->if_stmt.condition);
            ast_destroy(node->if_stmt.then_stmt);
            ast_destroy(node->if_stmt.else_stmt);
            break;
        case AST_WHILE_STMT:
            ast_destroy(node->while_stmt.condition);
            ast_destroy(node->while_stmt.body);
            break;
        case AST_FOR_STMT:
            ast_destroy(node->for_stmt.init);
            ast_destroy(node->for_stmt.condition);
            ast_destroy(node->for_stmt.update);
            ast_destroy(node->for_stmt.body);
            break;
        case AST_RETURN_STMT:
            ast_destroy(node->return_stmt.expression);
            break;
        case AST_FUNCTION_CALL:
            ast_destroy(node->function_call.function);
            ast_destroy(node->function_call.arguments);
            break;
        case AST_VARIABLE_DECL:
            free(node->var_decl.name);
            ast_destroy(node->var_decl.initializer);
            break;
        case AST_FUNCTION_DEF:
            free(node->function_def.name);
            ast_destroy(node->function_def.parameters);
            ast_destroy(node->function_def.body);
            break;
        case AST_PARAMETER_LIST:
            for (int i = 0; i < node->parameter_list.parameter_count; i++) {
                ast_destroy(node->parameter_list.parameters[i]);
            }
            free(node->parameter_list.parameters);
            break;
        case AST_ARGUMENT_LIST:
            for (int i = 0; i < node->argument_list.argument_count; i++) {
                ast_destroy(node->argument_list.arguments[i]);
            }
            free(node->argument_list.arguments);
            break;
        default:
            // 其他类型的清理
            break;
    }
    
    free(node);
}/**
 * parser_part2.c - Additional parsing functions for the C99 parser
 * This file contains the remaining parsing functions that didn't fit in parser.c
 */

#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// 前向声明
ASTNode* parse_declaration(Parser* parser);
ASTNode* parse_if_statement(Parser* parser);
ASTNode* parse_while_statement(Parser* parser);
ASTNode* parse_for_statement(Parser* parser);
ASTNode* parse_return_statement(Parser* parser);
ASTNode* parse_expression_statement(Parser* parser);
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
ASTNode* parse_argument_list(Parser* parser);

// 解析变量声明
ASTNode* parse_declaration(Parser* parser) {
    BasicType type = TYPE_INT;
    
    // 解析类型
    if (parser_peek(parser) == TOKEN_INT || parser_peek(parser) == TOKEN_CHAR ||
        parser_peek(parser) == TOKEN_FLOAT || parser_peek(parser) == TOKEN_DOUBLE ||
        parser_peek(parser) == TOKEN_VOID) {
        type = token_to_basic_type(parser_peek(parser));
        parser_advance(parser);
    }
    
    // 解析变量名
    if (parser_peek(parser) != TOKEN_IDENTIFIER) {
        parser_error(parser, "Expected identifier in declaration");
        return NULL;
    }
    
    Token* name_token = parser->current_token;
    parser_advance(parser);
    
    // 解析初始化器（可选）
    ASTNode* initializer = NULL;
    if (parser_peek(parser) == TOKEN_ASSIGN) {
        parser_advance(parser);
        initializer = parse_assignment_expression(parser);
    }
    
    // 期望分号
    if (!parser_expect_token(parser, TOKEN_SEMICOLON)) {
        if (initializer) ast_destroy(initializer);
        return NULL;
    }
    
    return ast_create_var_decl(type, name_token->value, initializer, 
                              name_token->line, name_token->column);
}

// 解析if语句
ASTNode* parse_if_statement(Parser* parser) {
    int line = parser->current_token->line;
    int column = parser->current_token->column;
    
    if (!parser_expect_token(parser, TOKEN_IF)) {
        return NULL;
    }
    
    if (!parser_expect_token(parser, TOKEN_LEFT_PAREN)) {
        return NULL;
    }
    
    ASTNode* condition = parse_expression(parser);
    if (!condition) {
        return NULL;
    }
    
    if (!parser_expect_token(parser, TOKEN_RIGHT_PAREN)) {
        ast_destroy(condition);
        return NULL;
    }
    
    ASTNode* then_stmt = parse_statement(parser);
    if (!then_stmt) {
        ast_destroy(condition);
        return NULL;
    }
    
    ASTNode* else_stmt = NULL;
    if (parser_peek(parser) == TOKEN_ELSE) {
        parser_advance(parser);
        else_stmt = parse_statement(parser);
        if (!else_stmt) {
            ast_destroy(condition);
            ast_destroy(then_stmt);
            return NULL;
        }
    }
    
    return ast_create_if_stmt(condition, then_stmt, else_stmt, line, column);
}

// 解析while语句
ASTNode* parse_while_statement(Parser* parser) {
    int line = parser->current_token->line;
    int column = parser->current_token->column;
    
    if (!parser_expect_token(parser, TOKEN_WHILE)) {
        return NULL;
    }
    
    if (!parser_expect_token(parser, TOKEN_LEFT_PAREN)) {
        return NULL;
    }
    
    ASTNode* condition = parse_expression(parser);
    if (!condition) {
        return NULL;
    }
    
    if (!parser_expect_token(parser, TOKEN_RIGHT_PAREN)) {
        ast_destroy(condition);
        return NULL;
    }
    
    parser->in_loop++;
    ASTNode* body = parse_statement(parser);
    parser->in_loop--;
    
    if (!body) {
        ast_destroy(condition);
        return NULL;
    }
    
    return ast_create_while_stmt(condition, body, line, column);
}

// 解析for语句
ASTNode* parse_for_statement(Parser* parser) {
    int line = parser->current_token->line;
    int column = parser->current_token->column;
    
    if (!parser_expect_token(parser, TOKEN_FOR)) {
        return NULL;
    }
    
    if (!parser_expect_token(parser, TOKEN_LEFT_PAREN)) {
        return NULL;
    }
    
    // 初始化表达式（可选）
    ASTNode* init = NULL;
    if (parser_peek(parser) != TOKEN_SEMICOLON) {
        if (parser_peek(parser) == TOKEN_INT || parser_peek(parser) == TOKEN_CHAR ||
            parser_peek(parser) == TOKEN_FLOAT || parser_peek(parser) == TOKEN_DOUBLE) {
            init = parse_declaration(parser);
            // 注意：declaration已经消费了分号
        } else {
            init = parse_expression(parser);
            if (!parser_expect_token(parser, TOKEN_SEMICOLON)) {
                if (init) ast_destroy(init);
                return NULL;
            }
        }
    } else {
        parser_advance(parser); // 跳过分号
    }
    
    // 条件表达式（可选）
    ASTNode* condition = NULL;
    if (parser_peek(parser) != TOKEN_SEMICOLON) {
        condition = parse_expression(parser);
    }
    if (!parser_expect_token(parser, TOKEN_SEMICOLON)) {
        if (init) ast_destroy(init);
        if (condition) ast_destroy(condition);
        return NULL;
    }
    
    // 更新表达式（可选）
    ASTNode* update = NULL;
    if (parser_peek(parser) != TOKEN_RIGHT_PAREN) {
        update = parse_expression(parser);
    }
    if (!parser_expect_token(parser, TOKEN_RIGHT_PAREN)) {
        if (init) ast_destroy(init);
        if (condition) ast_destroy(condition);
        if (update) ast_destroy(update);
        return NULL;
    }
    
    // 循环体
    parser->in_loop++;
    ASTNode* body = parse_statement(parser);
    parser->in_loop--;
    
    if (!body) {
        if (init) ast_destroy(init);
        if (condition) ast_destroy(condition);
        if (update) ast_destroy(update);
        return NULL;
    }
    
    return ast_create_for_stmt(init, condition, update, body, line, column);
}

// 解析return语句
ASTNode* parse_return_statement(Parser* parser) {
    int line = parser->current_token->line;
    int column = parser->current_token->column;
    
    if (!parser_expect_token(parser, TOKEN_RETURN)) {
        return NULL;
    }
    
    ASTNode* expression = NULL;
    if (parser_peek(parser) != TOKEN_SEMICOLON) {
        expression = parse_expression(parser);
    }
    
    if (!parser_expect_token(parser, TOKEN_SEMICOLON)) {
        if (expression) ast_destroy(expression);
        return NULL;
    }
    
    return ast_create_return_stmt(expression, line, column);
}

// 解析表达式语句
ASTNode* parse_expression_statement(Parser* parser) {
    ASTNode* expr = parse_expression(parser);
    
    if (!parser_expect_token(parser, TOKEN_SEMICOLON)) {
        if (expr) ast_destroy(expr);
        return NULL;
    }
    
    // 将表达式包装成语句
    ASTNode* stmt = ast_create_node(AST_EXPRESSION_STMT, 
                                   expr ? expr->line : 1, 
                                   expr ? expr->column : 1);
    if (stmt) {
        stmt->compound.statements = malloc(sizeof(ASTNode*));
        if (stmt->compound.statements) {
            stmt->compound.statements[0] = expr;
            stmt->compound.statement_count = 1;
            stmt->compound.capacity = 1;
        }
    }
    
    return stmt;
}

// 解析表达式
ASTNode* parse_expression(Parser* parser) {
    return parse_assignment_expression(parser);
}

// 解析赋值表达式
ASTNode* parse_assignment_expression(Parser* parser) {
    ASTNode* left = parse_logical_or_expression(parser);
    if (!left) return NULL;
    
    // 检查赋值操作符
    BinaryOpType assign_op;
    switch (parser_peek(parser)) {
        case TOKEN_ASSIGN:
            assign_op = BIN_OP_ASSIGN;
            break;
        case TOKEN_PLUS_ASSIGN:
            assign_op = BIN_OP_ADD_ASSIGN;
            break;
        case TOKEN_MINUS_ASSIGN:
            assign_op = BIN_OP_SUB_ASSIGN;
            break;
        case TOKEN_MULT_ASSIGN:
            assign_op = BIN_OP_MUL_ASSIGN;
            break;
        case TOKEN_DIV_ASSIGN:
            assign_op = BIN_OP_DIV_ASSIGN;
            break;
        case TOKEN_MOD_ASSIGN:
            assign_op = BIN_OP_MOD_ASSIGN;
            break;
        default:
            return left; // 不是赋值表达式
    }
    
    int line = parser->current_token->line;
    int column = parser->current_token->column;
    parser_advance(parser);
    
    ASTNode* right = parse_assignment_expression(parser);
    if (!right) {
        ast_destroy(left);
        return NULL;
    }
    
    return ast_create_binary_op(assign_op, left, right, line, column);
}

// 解析逻辑或表达式
ASTNode* parse_logical_or_expression(Parser* parser) {
    ASTNode* left = parse_logical_and_expression(parser);
    if (!left) return NULL;
    
    while (parser_peek(parser) == TOKEN_LOGICAL_OR) {
        int line = parser->current_token->line;
        int column = parser->current_token->column;
        parser_advance(parser);
        
        ASTNode* right = parse_logical_and_expression(parser);
        if (!right) {
            ast_destroy(left);
            return NULL;
        }
        
        left = ast_create_binary_op(BIN_OP_OR, left, right, line, column);
        if (!left) {
            ast_destroy(right);
            return NULL;
        }
    }
    
    return left;
}

// 解析逻辑与表达式
ASTNode* parse_logical_and_expression(Parser* parser) {
    ASTNode* left = parse_equality_expression(parser);
    if (!left) return NULL;
    
    while (parser_peek(parser) == TOKEN_LOGICAL_AND) {
        int line = parser->current_token->line;
        int column = parser->current_token->column;
        parser_advance(parser);
        
        ASTNode* right = parse_equality_expression(parser);
        if (!right) {
            ast_destroy(left);
            return NULL;
        }
        
        left = ast_create_binary_op(BIN_OP_AND, left, right, line, column);
        if (!left) {
            ast_destroy(right);
            return NULL;
        }
    }
    
    return left;
}

// 解析相等表达式
ASTNode* parse_equality_expression(Parser* parser) {
    ASTNode* left = parse_relational_expression(parser);
    if (!left) return NULL;
    
    while (parser_peek(parser) == TOKEN_EQUAL || parser_peek(parser) == TOKEN_NOT_EQUAL) {
        BinaryOpType op = (parser_peek(parser) == TOKEN_EQUAL) ? BIN_OP_EQ : BIN_OP_NE;
        int line = parser->current_token->line;
        int column = parser->current_token->column;
        parser_advance(parser);
        
        ASTNode* right = parse_relational_expression(parser);
        if (!right) {
            ast_destroy(left);
            return NULL;
        }
        
        left = ast_create_binary_op(op, left, right, line, column);
        if (!left) {
            ast_destroy(right);
            return NULL;
        }
    }
    
    return left;
}

// 解析关系表达式
ASTNode* parse_relational_expression(Parser* parser) {
    ASTNode* left = parse_additive_expression(parser);
    if (!left) return NULL;
    
    while (parser_peek(parser) == TOKEN_LESS || parser_peek(parser) == TOKEN_LESS_EQUAL ||
           parser_peek(parser) == TOKEN_GREATER || parser_peek(parser) == TOKEN_GREATER_EQUAL) {
        BinaryOpType op;
        switch (parser_peek(parser)) {
            case TOKEN_LESS: op = BIN_OP_LT; break;
            case TOKEN_LESS_EQUAL: op = BIN_OP_LE; break;
            case TOKEN_GREATER: op = BIN_OP_GT; break;
            case TOKEN_GREATER_EQUAL: op = BIN_OP_GE; break;
            default: op = BIN_OP_LT; break;
        }
        
        int line = parser->current_token->line;
        int column = parser->current_token->column;
        parser_advance(parser);
        
        ASTNode* right = parse_additive_expression(parser);
        if (!right) {
            ast_destroy(left);
            return NULL;
        }
        
        left = ast_create_binary_op(op, left, right, line, column);
        if (!left) {
            ast_destroy(right);
            return NULL;
        }
    }
    
    return left;
}

// 解析加法表达式
ASTNode* parse_additive_expression(Parser* parser) {
    ASTNode* left = parse_multiplicative_expression(parser);
    if (!left) return NULL;
    
    while (parser_peek(parser) == TOKEN_PLUS || parser_peek(parser) == TOKEN_MINUS) {
        BinaryOpType op = (parser_peek(parser) == TOKEN_PLUS) ? BIN_OP_ADD : BIN_OP_SUB;
        int line = parser->current_token->line;
        int column = parser->current_token->column;
        parser_advance(parser);
        
        ASTNode* right = parse_multiplicative_expression(parser);
        if (!right) {
            ast_destroy(left);
            return NULL;
        }
        
        left = ast_create_binary_op(op, left, right, line, column);
        if (!left) {
            ast_destroy(right);
            return NULL;
        }
    }
    
    return left;
}

// 解析乘法表达式
ASTNode* parse_multiplicative_expression(Parser* parser) {
    ASTNode* left = parse_unary_expression(parser);
    if (!left) return NULL;
    
    while (parser_peek(parser) == TOKEN_MULTIPLY || parser_peek(parser) == TOKEN_DIVIDE || 
           parser_peek(parser) == TOKEN_MODULO) {
        BinaryOpType op;
        switch (parser_peek(parser)) {
            case TOKEN_MULTIPLY: op = BIN_OP_MUL; break;
            case TOKEN_DIVIDE: op = BIN_OP_DIV; break;
            case TOKEN_MODULO: op = BIN_OP_MOD; break;
            default: op = BIN_OP_MUL; break;
        }
        
        int line = parser->current_token->line;
        int column = parser->current_token->column;
        parser_advance(parser);
        
        ASTNode* right = parse_unary_expression(parser);
        if (!right) {
            ast_destroy(left);
            return NULL;
        }
        
        left = ast_create_binary_op(op, left, right, line, column);
        if (!left) {
            ast_destroy(right);
            return NULL;
        }
    }
    
    return left;
}

// 解析一元表达式
ASTNode* parse_unary_expression(Parser* parser) {
    switch (parser_peek(parser)) {
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_LOGICAL_NOT:
        case TOKEN_BITWISE_NOT:
        case TOKEN_INCREMENT:
        case TOKEN_DECREMENT:
        case TOKEN_BITWISE_AND:
        case TOKEN_MULTIPLY: {
            UnaryOpType op;
            switch (parser_peek(parser)) {
                case TOKEN_PLUS: op = UNARY_OP_PLUS; break;
                case TOKEN_MINUS: op = UNARY_OP_MINUS; break;
                case TOKEN_LOGICAL_NOT: op = UNARY_OP_NOT; break;
                case TOKEN_BITWISE_NOT: op = UNARY_OP_BITWISE_NOT; break;
                case TOKEN_INCREMENT: op = UNARY_OP_PRE_INC; break;
                case TOKEN_DECREMENT: op = UNARY_OP_PRE_DEC; break;
                case TOKEN_BITWISE_AND: op = UNARY_OP_ADDR; break;
                case TOKEN_MULTIPLY: op = UNARY_OP_DEREF; break;
                default: op = UNARY_OP_PLUS; break;
            }
            
            int line = parser->current_token->line;
            int column = parser->current_token->column;
            parser_advance(parser);
            
            ASTNode* operand = parse_unary_expression(parser);
            if (!operand) return NULL;
            
            return ast_create_unary_op(op, operand, line, column);
        }
        default:
            return parse_postfix_expression(parser);
    }
}

// 解析后缀表达式
ASTNode* parse_postfix_expression(Parser* parser) {
    ASTNode* left = parse_primary_expression(parser);
    if (!left) return NULL;
    
    while (1) {
        switch (parser_peek(parser)) {
            case TOKEN_LEFT_PAREN: {
                // 函数调用
                int line = parser->current_token->line;
                int column = parser->current_token->column;
                parser_advance(parser);
                
                ASTNode* arguments = parse_argument_list(parser);
                
                if (!parser_expect_token(parser, TOKEN_RIGHT_PAREN)) {
                    ast_destroy(left);
                    if (arguments) ast_destroy(arguments);
                    return NULL;
                }
                
                left = ast_create_function_call(left, arguments, line, column);
                if (!left) return NULL;
                break;
            }
            case TOKEN_INCREMENT: {
                int line = parser->current_token->line;
                int column = parser->current_token->column;
                parser_advance(parser);
                left = ast_create_unary_op(UNARY_OP_POST_INC, left, line, column);
                if (!left) return NULL;
                break;
            }
            case TOKEN_DECREMENT: {
                int line = parser->current_token->line;
                int column = parser->current_token->column;
                parser_advance(parser);
                left = ast_create_unary_op(UNARY_OP_POST_DEC, left, line, column);
                if (!left) return NULL;
                break;
            }
            default:
                return left;
        }
    }
}

// 解析基本表达式
ASTNode* parse_primary_expression(Parser* parser) {
    switch (parser_peek(parser)) {
        case TOKEN_IDENTIFIER: {
            Token* token = parser->current_token;
            parser_advance(parser);
            return ast_create_identifier(token->value, token->line, token->column);
        }
        case TOKEN_INTEGER_CONSTANT: {
            Token* token = parser->current_token;
            parser_advance(parser);
            return ast_create_integer_literal(token->numeric.int_value, token->line, token->column);
        }
        case TOKEN_FLOAT_CONSTANT: {
            Token* token = parser->current_token;
            parser_advance(parser);
            return ast_create_float_literal(token->numeric.float_value, token->line, token->column);
        }
        case TOKEN_STRING_LITERAL: {
            Token* token = parser->current_token;
            parser_advance(parser);
            return ast_create_string_literal(token->value, token->line, token->column);
        }
        case TOKEN_CHAR_CONSTANT: {
            Token* token = parser->current_token;
            parser_advance(parser);
            ASTNode* node = ast_create_node(AST_CHAR_LITERAL, token->line, token->column);
            if (node) {
                node->literal.char_value = token->numeric.char_value;
            }
            return node;
        }
        case TOKEN_LEFT_PAREN: {
            parser_advance(parser);
            ASTNode* expr = parse_expression(parser);
            if (!expr) return NULL;
            
            if (!parser_expect_token(parser, TOKEN_RIGHT_PAREN)) {
                ast_destroy(expr);
                return NULL;
            }
            return expr;
        }
        default:
            parser_error(parser, "Expected primary expression");
            return NULL;
    }
}

// 解析参数列表（函数调用）
ASTNode* parse_argument_list(Parser* parser) {
    ASTNode* arg_list = ast_create_node(AST_ARGUMENT_LIST, 
                                       parser->current_token->line, 
                                       parser->current_token->column);
    if (!arg_list) return NULL;
    
    arg_list->argument_list.arguments = NULL;
    arg_list->argument_list.argument_count = 0;
    arg_list->argument_list.capacity = 0;
    
    // 空参数列表
    if (parser_peek(parser) == TOKEN_RIGHT_PAREN) {
        return arg_list;
    }
    
    // 解析参数
    do {
        ASTNode* arg = parse_assignment_expression(parser);
        if (!arg) {
            ast_destroy(arg_list);
            return NULL;
        }
        
        ast_argument_list_add(arg_list, arg);
        
        if (parser_peek(parser) == TOKEN_COMMA) {
            parser_advance(parser);
        } else {
            break;
        }
    } while (parser_peek(parser) != TOKEN_RIGHT_PAREN && parser_peek(parser) != TOKEN_EOF);
    
    return arg_list;
}

void ast_argument_list_add(ASTNode* arg_list, ASTNode* argument) {
    if (!arg_list || arg_list->type != AST_ARGUMENT_LIST || !argument) {
        return;
    }
    
    // 动态扩展数组
    if (arg_list->argument_list.argument_count >= arg_list->argument_list.capacity) {
        int new_capacity = arg_list->argument_list.capacity == 0 ? 4 : arg_list->argument_list.capacity * 2;
        ASTNode** new_arguments = realloc(arg_list->argument_list.arguments, 
                                         new_capacity * sizeof(ASTNode*));
        if (!new_arguments) return;
        
        arg_list->argument_list.arguments = new_arguments;
        arg_list->argument_list.capacity = new_capacity;
    }
    
    arg_list->argument_list.arguments[arg_list->argument_list.argument_count++] = argument;
}

// AST打印函数（用于调试）
void ast_print(const ASTNode* node, int indent) {
    if (!node) return;
    
    for (int i = 0; i < indent; i++) printf("  ");
    
    printf("%s", ast_node_type_name(node->type));
    
    switch (node->type) {
        case AST_IDENTIFIER:
            printf(" '%s'", node->identifier.name);
            break;
        case AST_INTEGER_LITERAL:
            printf(" %lld", node->literal.int_value);
            break;
        case AST_FLOAT_LITERAL:
            printf(" %g", node->literal.float_value);
            break;
        case AST_STRING_LITERAL:
            printf(" \"%s\"", node->literal.string_value);
            break;
        case AST_CHAR_LITERAL:
            printf(" '%c'", node->literal.char_value);
            break;
        case AST_BINARY_OP:
            printf(" %s", binary_op_name(node->binary_op.op));
            break;
        case AST_UNARY_OP:
            printf(" %s", unary_op_name(node->unary_op.op));
            break;
        case AST_VARIABLE_DECL:
            printf(" %s %s", basic_type_name(node->var_decl.type), node->var_decl.name);
            break;
        case AST_FUNCTION_DEF:
            printf(" %s %s", basic_type_name(node->function_def.return_type), node->function_def.name);
            break;
        default:
            break;
    }
    
    printf("\n");
    
    // 递归打印子节点
    switch (node->type) {
        case AST_BINARY_OP:
            ast_print(node->binary_op.left, indent + 1);
            ast_print(node->binary_op.right, indent + 1);
            break;
        case AST_UNARY_OP:
            ast_print(node->unary_op.operand, indent + 1);
            break;
        case AST_COMPOUND_STMT:
        case AST_TRANSLATION_UNIT:
            for (int i = 0; i < node->compound.statement_count; i++) {
                ast_print(node->compound.statements[i], indent + 1);
            }
            break;
        case AST_IF_STMT:
            printf("%*sCondition:\n", (indent + 1) * 2, "");
            ast_print(node->if_stmt.condition, indent + 2);
            printf("%*sThen:\n", (indent + 1) * 2, "");
            ast_print(node->if_stmt.then_stmt, indent + 2);
            if (node->if_stmt.else_stmt) {
                printf("%*sElse:\n", (indent + 1) * 2, "");
                ast_print(node->if_stmt.else_stmt, indent + 2);
            }
            break;
        case AST_WHILE_STMT:
            printf("%*sCondition:\n", (indent + 1) * 2, "");
            ast_print(node->while_stmt.condition, indent + 2);
            printf("%*sBody:\n", (indent + 1) * 2, "");
            ast_print(node->while_stmt.body, indent + 2);
            break;
        case AST_FOR_STMT:
            if (node->for_stmt.init) {
                printf("%*sInit:\n", (indent + 1) * 2, "");
                ast_print(node->for_stmt.init, indent + 2);
            }
            if (node->for_stmt.condition) {
                printf("%*sCondition:\n", (indent + 1) * 2, "");
                ast_print(node->for_stmt.condition, indent + 2);
            }
            if (node->for_stmt.update) {
                printf("%*sUpdate:\n", (indent + 1) * 2, "");
                ast_print(node->for_stmt.update, indent + 2);
            }
            printf("%*sBody:\n", (indent + 1) * 2, "");
            ast_print(node->for_stmt.body, indent + 2);
            break;
        case AST_RETURN_STMT:
            if (node->return_stmt.expression) {
                ast_print(node->return_stmt.expression, indent + 1);
            }
            break;
        case AST_FUNCTION_CALL:
            printf("%*sFunction:\n", (indent + 1) * 2, "");
            ast_print(node->function_call.function, indent + 2);
            if (node->function_call.arguments) {
                printf("%*sArguments:\n", (indent + 1) * 2, "");
                ast_print(node->function_call.arguments, indent + 2);
            }
            break;
        case AST_VARIABLE_DECL:
            if (node->var_decl.initializer) {
                printf("%*sInitializer:\n", (indent + 1) * 2, "");
                ast_print(node->var_decl.initializer, indent + 2);
            }
            break;
        case AST_FUNCTION_DEF:
            if (node->function_def.parameters) {
                printf("%*sParameters:\n", (indent + 1) * 2, "");
                ast_print(node->function_def.parameters, indent + 2);
            }
            printf("%*sBody:\n", (indent + 1) * 2, "");
            ast_print(node->function_def.body, indent + 2);
            break;
        case AST_PARAMETER_LIST:
            for (int i = 0; i < node->parameter_list.parameter_count; i++) {
                ast_print(node->parameter_list.parameters[i], indent + 1);
            }
            break;
        case AST_ARGUMENT_LIST:
            for (int i = 0; i < node->argument_list.argument_count; i++) {
                ast_print(node->argument_list.arguments[i], indent + 1);
            }
            break;
        case AST_EXPRESSION_STMT:
            if (node->compound.statement_count > 0) {
                ast_print(node->compound.statements[0], indent + 1);
            }
            break;
        default:
            break;
    }
}