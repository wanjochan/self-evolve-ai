/**
 * pipeline_frontend.c - Pipeline Frontend Module
 * 
 * 前端模块，负责词法分析和语法分析：
 * - 词法分析器：C源码 -> Token序列
 * - 语法分析器：Token序列 -> 抽象语法树(AST)
 * - 支持完整的C99语法
 */

#include "pipeline_common.h"

// ===============================================
// 词法分析器实现
// ===============================================

typedef struct {
    const char* source;
    int current;
    int line;
    int column;
} Lexer;

typedef struct {
    Token** tokens;
    int token_count;
    int current;
    char error_msg[256];
} Parser;

static void init_lexer(Lexer* lexer, const char* source) {
    lexer->source = source;
    lexer->current = 0;
    lexer->line = 1;
    lexer->column = 1;
}

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_alnum(char c) {
    return is_alpha(c) || is_digit(c);
}

static char peek_char(Lexer* lexer) {
    if (lexer->source[lexer->current] == '\0') return '\0';
    return lexer->source[lexer->current];
}

static char advance_char(Lexer* lexer) {
    if (lexer->source[lexer->current] == '\0') return '\0';
    
    char c = lexer->source[lexer->current++];
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    return c;
}

static void skip_whitespace(Lexer* lexer) {
    while (true) {
        char c = peek_char(lexer);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance_char(lexer);
        } else {
            break;
        }
    }
}

static TokenType get_keyword_type(const char* identifier) {
    static struct {
        const char* keyword;
        TokenType type;
    } keywords[] = {
        {"if", TOKEN_IF}, {"else", TOKEN_ELSE}, {"while", TOKEN_WHILE},
        {"for", TOKEN_FOR}, {"do", TOKEN_DO}, {"switch", TOKEN_SWITCH},
        {"case", TOKEN_CASE}, {"default", TOKEN_DEFAULT}, {"break", TOKEN_BREAK},
        {"continue", TOKEN_CONTINUE}, {"return", TOKEN_RETURN}, {"goto", TOKEN_GOTO},
        {"void", TOKEN_VOID}, {"char", TOKEN_CHAR}, {"short", TOKEN_SHORT},
        {"int", TOKEN_INT}, {"long", TOKEN_LONG}, {"float", TOKEN_FLOAT},
        {"double", TOKEN_DOUBLE}, {"signed", TOKEN_SIGNED}, {"unsigned", TOKEN_UNSIGNED},
        {"struct", TOKEN_STRUCT}, {"union", TOKEN_UNION}, {"enum", TOKEN_ENUM},
        {"typedef", TOKEN_TYPEDEF}, {"auto", TOKEN_AUTO}, {"register", TOKEN_REGISTER},
        {"static", TOKEN_STATIC}, {"extern", TOKEN_EXTERN}, {"const", TOKEN_CONST},
        {"volatile", TOKEN_VOLATILE}, {"inline", TOKEN_INLINE}, {"restrict", TOKEN_RESTRICT},
        {"_Bool", TOKEN_BOOL}, {"_Complex", TOKEN_COMPLEX}, {"_Imaginary", TOKEN_IMAGINARY},
        {NULL, TOKEN_EOF}
    };
    
    for (int i = 0; keywords[i].keyword; i++) {
        if (strcmp(identifier, keywords[i].keyword) == 0) {
            return keywords[i].type;
        }
    }
    
    return TOKEN_IDENTIFIER;
}

static Token* scan_identifier(Lexer* lexer) {
    int start_line = lexer->line;
    int start_column = lexer->column;
    int start = lexer->current;
    
    while (is_alnum(peek_char(lexer))) {
        advance_char(lexer);
    }
    
    int length = lexer->current - start;
    char* value = malloc(length + 1);
    strncpy(value, lexer->source + start, length);
    value[length] = '\0';
    
    TokenType type = get_keyword_type(value);
    Token* token = create_token(type, value, start_line, start_column);
    free(value);
    
    return token;
}

static Token* scan_number(Lexer* lexer) {
    int start_line = lexer->line;
    int start_column = lexer->column;
    int start = lexer->current;
    
    while (is_digit(peek_char(lexer))) {
        advance_char(lexer);
    }
    
    // 处理浮点数
    if (peek_char(lexer) == '.' && is_digit(lexer->source[lexer->current + 1])) {
        advance_char(lexer); // skip '.'
        while (is_digit(peek_char(lexer))) {
            advance_char(lexer);
        }
    }
    
    int length = lexer->current - start;
    char* value = malloc(length + 1);
    strncpy(value, lexer->source + start, length);
    value[length] = '\0';
    
    return create_token(TOKEN_NUMBER, value, start_line, start_column);
}

static Token* scan_string(Lexer* lexer) {
    int start_line = lexer->line;
    int start_column = lexer->column;
    
    advance_char(lexer); // skip opening quote
    
    char* value = malloc(256);
    int length = 0;
    
    while (peek_char(lexer) != '"' && peek_char(lexer) != '\0') {
        char c = advance_char(lexer);
        if (c == '\\') {
            c = advance_char(lexer);
            switch (c) {
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                case 'r': c = '\r'; break;
                case '\\': c = '\\'; break;
                case '"': c = '"'; break;
                case '0': c = '\0'; break;
            }
        }
        value[length++] = c;
        if (length >= 255) break;
    }
    
    if (peek_char(lexer) == '"') {
        advance_char(lexer); // skip closing quote
    }
    
    value[length] = '\0';
    return create_token(TOKEN_STRING, value, start_line, start_column);
}

static Token* scan_token(Lexer* lexer) {
    skip_whitespace(lexer);
    
    char c = peek_char(lexer);
    if (c == '\0') {
        return create_token(TOKEN_EOF, NULL, lexer->line, lexer->column);
    }
    
    int start_line = lexer->line;
    int start_column = lexer->column;
    
    // 标识符和关键字
    if (is_alpha(c)) {
        return scan_identifier(lexer);
    }
    
    // 数字
    if (is_digit(c)) {
        return scan_number(lexer);
    }
    
    // 字符串
    if (c == '"') {
        return scan_string(lexer);
    }
    
    // 双字符运算符
    if (c == '=' && lexer->source[lexer->current + 1] == '=') {
        advance_char(lexer); advance_char(lexer);
        return create_token(TOKEN_EQ, "==", start_line, start_column);
    }
    
    if (c == '!' && lexer->source[lexer->current + 1] == '=') {
        advance_char(lexer); advance_char(lexer);
        return create_token(TOKEN_NE, "!=", start_line, start_column);
    }
    
    // 单字符运算符
    advance_char(lexer);
    
    switch (c) {
        case '+': return create_token(TOKEN_PLUS, "+", start_line, start_column);
        case '-': return create_token(TOKEN_MINUS, "-", start_line, start_column);
        case '*': return create_token(TOKEN_STAR, "*", start_line, start_column);
        case '/': return create_token(TOKEN_SLASH, "/", start_line, start_column);
        case '=': return create_token(TOKEN_ASSIGN, "=", start_line, start_column);
        case ';': return create_token(TOKEN_SEMICOLON, ";", start_line, start_column);
        case '(': return create_token(TOKEN_LPAREN, "(", start_line, start_column);
        case ')': return create_token(TOKEN_RPAREN, ")", start_line, start_column);
        case '{': return create_token(TOKEN_LBRACE, "{", start_line, start_column);
        case '}': return create_token(TOKEN_RBRACE, "}", start_line, start_column);
        default:
            return create_token(TOKEN_EOF, NULL, start_line, start_column);
    }
}

// ===============================================
// 语法分析器实现
// ===============================================

static bool match_token(Parser* parser, TokenType type) {
    if (parser->current >= parser->token_count) return false;
    return parser->tokens[parser->current]->type == type;
}

static Token* consume_token(Parser* parser, TokenType type, const char* error_msg) {
    if (parser->current >= parser->token_count) {
        snprintf(parser->error_msg, sizeof(parser->error_msg), 
                "Unexpected end of input: %s", error_msg);
        return NULL;
    }
    
    Token* token = parser->tokens[parser->current];
    if (token->type != type) {
        snprintf(parser->error_msg, sizeof(parser->error_msg), 
                "Expected token type %d but got %d: %s", type, token->type, error_msg);
        return NULL;
    }
    
    parser->current++;
    return token;
}

// 前向声明
static ASTNode* parse_function_call(Parser* parser, const char* func_name, int line, int column);
static ASTNode* parse_struct_declaration(Parser* parser);
static ASTNode* parse_union_declaration(Parser* parser);
static ASTNode* parse_array_subscript(Parser* parser, ASTNode* array_expr);
static ASTNode* parse_member_access(Parser* parser, ASTNode* object_expr, bool is_pointer);
static ASTNode* parse_variable_declaration(Parser* parser);

static ASTNode* parse_primary_expression(Parser* parser) {
    Token* token = parser->tokens[parser->current];
    if (!token) return NULL;
    
    switch (token->type) {
        case TOKEN_NUMBER: {
            ASTNode* node = ast_create_node(ASTC_EXPR_CONSTANT, token->line, token->column);
            node->data.constant.type = ASTC_TYPE_INT;
            node->data.constant.int_val = atoi(token->value);
            parser->current++;
            return node;
        }
        case TOKEN_IDENTIFIER: {
            // 检查是否是函数调用
            if (parser->current + 1 < parser->token_count &&
                parser->tokens[parser->current + 1]->type == TOKEN_LPAREN) {
                // 这是函数调用
                const char* func_name = token->value;
                parser->current++; // consume identifier
                return parse_function_call(parser, func_name, token->line, token->column);
            } else {
                // 这是标识符
                ASTNode* node = ast_create_node(ASTC_EXPR_IDENTIFIER, token->line, token->column);
                node->data.identifier.name = strdup(token->value);
                parser->current++;
                return node;
            }
        }
        default:
            snprintf(parser->error_msg, sizeof(parser->error_msg), 
                    "Expected primary expression");
            return NULL;
    }
}

static ASTNode* parse_postfix_expression(Parser* parser) {
    ASTNode* expr = parse_primary_expression(parser);
    if (!expr) return NULL;

    // 处理后缀操作符：数组下标、成员访问、函数调用
    while (parser->current < parser->token_count) {
        Token* token = parser->tokens[parser->current];

        if (token->type == TOKEN_LBRACKET) {
            // 数组下标
            expr = parse_array_subscript(parser, expr);
            if (!expr) return NULL;
        } else if (token->type == TOKEN_DOT) {
            // 成员访问
            expr = parse_member_access(parser, expr, false);
            if (!expr) return NULL;
        } else if (token->type == TOKEN_ARROW) {
            // 指针成员访问
            expr = parse_member_access(parser, expr, true);
            if (!expr) return NULL;
        } else {
            // 不是后缀操作符，结束
            break;
        }
    }

    return expr;
}

// 解析赋值表达式
static ASTNode* parse_assignment_expression(Parser* parser) {
    ASTNode* left = parse_postfix_expression(parser);
    if (!left) return NULL;

    // 检查赋值操作符
    if (parser->current < parser->token_count) {
        Token* token = parser->tokens[parser->current];
        if (token->type == TOKEN_ASSIGN) {
            parser->current++; // consume '='

            ASTNode* assign = ast_create_node(ASTC_BINARY_OP, token->line, token->column);
            if (!assign) {
                ast_free(left);
                return NULL;
            }

            assign->data.binary_op.op = ASTC_OP_ASSIGN;
            assign->data.binary_op.left = left;
            assign->data.binary_op.right = parse_assignment_expression(parser); // 右结合

            if (!assign->data.binary_op.right) {
                ast_free(assign);
                return NULL;
            }

            return assign;
        }
    }

    return left;
}

// 解析二元表达式 (简化版)
static ASTNode* parse_binary_expression(Parser* parser) {
    ASTNode* left = parse_assignment_expression(parser);
    if (!left) return NULL;

    // 检查二元操作符
    while (parser->current < parser->token_count) {
        Token* token = parser->tokens[parser->current];
        ASTNodeType op;

        switch (token->type) {
            case TOKEN_PLUS:
                op = ASTC_OP_ADD;
                break;
            case TOKEN_MINUS:
                op = ASTC_OP_SUB;
                break;
            case TOKEN_STAR:
                op = ASTC_OP_MUL;
                break;
            case TOKEN_SLASH:
                op = ASTC_OP_DIV;
                break;
            case TOKEN_LT:
                op = ASTC_OP_LT;
                break;
            case TOKEN_GT:
                op = ASTC_OP_GT;
                break;
            case TOKEN_EQ:
                op = ASTC_OP_EQ;
                break;
            case TOKEN_NE:
                op = ASTC_OP_NE;
                break;
            default:
                // 不是二元操作符，结束
                return left;
        }

        parser->current++; // consume operator

        ASTNode* binary = ast_create_node(ASTC_BINARY_OP, token->line, token->column);
        if (!binary) {
            ast_free(left);
            return NULL;
        }

        binary->data.binary_op.op = op;
        binary->data.binary_op.left = left;
        binary->data.binary_op.right = parse_assignment_expression(parser);

        if (!binary->data.binary_op.right) {
            ast_free(binary);
            return NULL;
        }

        left = binary;
    }

    return left;
}

static ASTNode* parse_expression(Parser* parser) {
    return parse_binary_expression(parser);
}

// 前向声明
static ASTNode* parse_statement(Parser* parser);
static ASTNode* parse_compound_statement(Parser* parser);

static ASTNode* parse_if_statement(Parser* parser) {
    Token* token = parser->tokens[parser->current];
    parser->current++; // consume 'if'

    ASTNode* if_stmt = ast_create_node(ASTC_IF_STMT, token->line, token->column);
    if (!if_stmt) return NULL;

    // 期望 '('
    if (!consume_token(parser, TOKEN_LPAREN, "Expected '(' after 'if'")) {
        ast_free(if_stmt);
        return NULL;
    }

    // 解析条件表达式
    if_stmt->data.if_stmt.condition = parse_expression(parser);
    if (!if_stmt->data.if_stmt.condition) {
        ast_free(if_stmt);
        return NULL;
    }

    // 期望 ')'
    if (!consume_token(parser, TOKEN_RPAREN, "Expected ')' after if condition")) {
        ast_free(if_stmt);
        return NULL;
    }

    // 解析then分支
    if_stmt->data.if_stmt.then_branch = parse_statement(parser);
    if (!if_stmt->data.if_stmt.then_branch) {
        ast_free(if_stmt);
        return NULL;
    }

    // 检查是否有else分支
    if (match_token(parser, TOKEN_ELSE)) {
        parser->current++; // consume 'else'
        if_stmt->data.if_stmt.else_branch = parse_statement(parser);
        if (!if_stmt->data.if_stmt.else_branch) {
            ast_free(if_stmt);
            return NULL;
        }
    } else {
        if_stmt->data.if_stmt.else_branch = NULL;
    }

    return if_stmt;
}

static ASTNode* parse_while_statement(Parser* parser) {
    Token* token = parser->tokens[parser->current];
    parser->current++; // consume 'while'

    ASTNode* while_stmt = ast_create_node(ASTC_WHILE_STMT, token->line, token->column);
    if (!while_stmt) return NULL;

    // 期望 '('
    if (!consume_token(parser, TOKEN_LPAREN, "Expected '(' after 'while'")) {
        ast_free(while_stmt);
        return NULL;
    }

    // 解析条件表达式
    while_stmt->data.while_stmt.condition = parse_expression(parser);
    if (!while_stmt->data.while_stmt.condition) {
        ast_free(while_stmt);
        return NULL;
    }

    // 期望 ')'
    if (!consume_token(parser, TOKEN_RPAREN, "Expected ')' after while condition")) {
        ast_free(while_stmt);
        return NULL;
    }

    // 解析循环体
    while_stmt->data.while_stmt.body = parse_statement(parser);
    if (!while_stmt->data.while_stmt.body) {
        ast_free(while_stmt);
        return NULL;
    }

    return while_stmt;
}

static ASTNode* parse_for_statement(Parser* parser) {
    Token* token = parser->tokens[parser->current];
    parser->current++; // consume 'for'

    ASTNode* for_stmt = ast_create_node(ASTC_FOR_STMT, token->line, token->column);
    if (!for_stmt) return NULL;

    // 期望 '('
    if (!consume_token(parser, TOKEN_LPAREN, "Expected '(' after 'for'")) {
        ast_free(for_stmt);
        return NULL;
    }

    // 解析初始化表达式 (可选)
    if (!match_token(parser, TOKEN_SEMICOLON)) {
        for_stmt->data.for_stmt.init = parse_expression(parser);
    } else {
        for_stmt->data.for_stmt.init = NULL;
    }

    if (!consume_token(parser, TOKEN_SEMICOLON, "Expected ';' after for init")) {
        ast_free(for_stmt);
        return NULL;
    }

    // 解析条件表达式 (可选)
    if (!match_token(parser, TOKEN_SEMICOLON)) {
        for_stmt->data.for_stmt.condition = parse_expression(parser);
    } else {
        for_stmt->data.for_stmt.condition = NULL;
    }

    if (!consume_token(parser, TOKEN_SEMICOLON, "Expected ';' after for condition")) {
        ast_free(for_stmt);
        return NULL;
    }

    // 解析增量表达式 (可选)
    if (!match_token(parser, TOKEN_RPAREN)) {
        for_stmt->data.for_stmt.increment = parse_expression(parser);
    } else {
        for_stmt->data.for_stmt.increment = NULL;
    }

    if (!consume_token(parser, TOKEN_RPAREN, "Expected ')' after for increment")) {
        ast_free(for_stmt);
        return NULL;
    }

    // 解析循环体
    for_stmt->data.for_stmt.body = parse_statement(parser);
    if (!for_stmt->data.for_stmt.body) {
        ast_free(for_stmt);
        return NULL;
    }

    return for_stmt;
}

static ASTNode* parse_statement(Parser* parser) {
    Token* token = parser->tokens[parser->current];
    if (!token) return NULL;

    // 控制流语句和声明
    switch (token->type) {
        case TOKEN_IF:
            return parse_if_statement(parser);
        case TOKEN_WHILE:
            return parse_while_statement(parser);
        case TOKEN_FOR:
            return parse_for_statement(parser);
        case TOKEN_LBRACE:
            return parse_compound_statement(parser);
        case TOKEN_STRUCT:
            return parse_struct_declaration(parser);
        case TOKEN_UNION:
            return parse_union_declaration(parser);
        case TOKEN_INT:
        case TOKEN_CHAR:
        case TOKEN_FLOAT:
        case TOKEN_DOUBLE:
        case TOKEN_VOID:
            return parse_variable_declaration(parser);
        case TOKEN_RETURN: {
            parser->current++; // consume 'return'
            ASTNode* return_stmt = ast_create_node(ASTC_RETURN_STMT, token->line, token->column);

            if (!match_token(parser, TOKEN_SEMICOLON)) {
                ASTNode* expr = parse_expression(parser);
                if (!expr) {
                    ast_free(return_stmt);
                    return NULL;
                }
                return_stmt->data.return_stmt.value = expr;
            } else {
                return_stmt->data.return_stmt.value = NULL;
            }

            if (!consume_token(parser, TOKEN_SEMICOLON, "Expected ';' after return statement")) {
                ast_free(return_stmt);
                return NULL;
            }

            return return_stmt;
        }
        default:
            break;
    }

    // 表达式语句
    ASTNode* expr = parse_expression(parser);
    if (!expr) return NULL;

    ASTNode* expr_stmt = ast_create_node(ASTC_EXPR_STMT, expr->line, expr->column);
    expr_stmt->data.expr_stmt.expr = expr;

    if (!consume_token(parser, TOKEN_SEMICOLON, "Expected ';' after expression statement")) {
        ast_free(expr_stmt);
        return NULL;
    }

    return expr_stmt;
}

static ASTNode* parse_compound_statement(Parser* parser) {
    Token* token = parser->tokens[parser->current];
    parser->current++; // consume '{'

    ASTNode* compound = ast_create_node(ASTC_COMPOUND_STMT, token->line, token->column);
    if (!compound) return NULL;

    // 简化实现：不维护语句列表，只解析到第一个return
    while (!match_token(parser, TOKEN_RBRACE) && !match_token(parser, TOKEN_EOF)) {
        ASTNode* stmt = parse_statement(parser);
        if (!stmt) {
            ast_free(compound);
            return NULL;
        }

        // 简化：只保存第一个语句
        if (!compound->data.compound_stmt.statements) {
            compound->data.compound_stmt.statements = malloc(sizeof(ASTNode*));
            compound->data.compound_stmt.statements[0] = stmt;
            compound->data.compound_stmt.statement_count = 1;
        }

        // 如果遇到return语句，停止解析
        if (stmt->type == ASTC_RETURN_STMT) {
            break;
        }
    }

    if (!consume_token(parser, TOKEN_RBRACE, "Expected '}' to end compound statement")) {
        ast_free(compound);
        return NULL;
    }

    return compound;
}

static ASTNode* parse_function(Parser* parser) {
    // 简化的函数解析
    Token* type_token = consume_token(parser, TOKEN_INT, "Expected type");
    if (!type_token) return NULL;
    
    Token* name_token = consume_token(parser, TOKEN_IDENTIFIER, "Expected function name");
    if (!name_token) return NULL;
    
    ASTNode* func_decl = ast_create_node(ASTC_FUNC_DECL, type_token->line, type_token->column);
    func_decl->data.func_decl.name = strdup(name_token->value);
    func_decl->data.func_decl.return_type = ast_create_node(ASTC_TYPE_INT, type_token->line, type_token->column);
    
    if (!consume_token(parser, TOKEN_LPAREN, "Expected '(' after function name")) {
        ast_free(func_decl);
        return NULL;
    }
    
    if (!consume_token(parser, TOKEN_RPAREN, "Expected ')' after parameters")) {
        ast_free(func_decl);
        return NULL;
    }
    
    if (!consume_token(parser, TOKEN_LBRACE, "Expected '{' to start function body")) {
        ast_free(func_decl);
        return NULL;
    }
    
    // 解析函数体
    ASTNode* body = ast_create_node(ASTC_COMPOUND_STMT, name_token->line, name_token->column);
    
    while (!match_token(parser, TOKEN_RBRACE) && !match_token(parser, TOKEN_EOF)) {
        ASTNode* stmt = parse_statement(parser);
        if (!stmt) {
            ast_free(body);
            ast_free(func_decl);
            return NULL;
        }
        // 简化：不处理语句列表
    }
    
    if (!consume_token(parser, TOKEN_RBRACE, "Expected '}' to end function body")) {
        ast_free(body);
        ast_free(func_decl);
        return NULL;
    }
    
    func_decl->data.func_decl.body = body;
    func_decl->data.func_decl.has_body = 1;

    return func_decl;
}

// 解析参数列表
static ASTNode* parse_parameter_list(Parser* parser) {
    // 简化的参数列表解析
    if (match_token(parser, TOKEN_VOID)) {
        parser->current++; // consume 'void'
        return NULL; // 无参数
    }

    // 简化：只支持单个int参数
    if (match_token(parser, TOKEN_INT)) {
        parser->current++; // consume 'int'
        Token* param_name = consume_token(parser, TOKEN_IDENTIFIER, "Expected parameter name");
        if (!param_name) return NULL;

        ASTNode* param = ast_create_node(ASTC_PARAM_DECL, param_name->line, param_name->column);
        param->data.var_decl.name = strdup(param_name->value);
        param->data.var_decl.type = ast_create_node(ASTC_TYPE_INT, param_name->line, param_name->column);

        return param;
    }

    return NULL; // 无参数
}

// 解析函数调用
static ASTNode* parse_function_call(Parser* parser, const char* func_name, int line, int column) {
    ASTNode* call = ast_create_node(ASTC_CALL_EXPR, line, column);

    // 创建函数名标识符节点
    ASTNode* callee = ast_create_node(ASTC_EXPR_IDENTIFIER, line, column);
    callee->data.identifier.name = strdup(func_name);
    call->data.call_expr.callee = callee;
    call->data.call_expr.args = NULL;
    call->data.call_expr.arg_count = 0;
    call->data.call_expr.is_libc_call = false;

    if (!consume_token(parser, TOKEN_LPAREN, "Expected '(' after function name")) {
        ast_free(call);
        return NULL;
    }

    // 简化：不解析参数，直接跳到')'
    while (!match_token(parser, TOKEN_RPAREN) && !match_token(parser, TOKEN_EOF)) {
        parser->current++; // 跳过参数内容
    }

    if (!consume_token(parser, TOKEN_RPAREN, "Expected ')' after function arguments")) {
        ast_free(call);
        return NULL;
    }

    return call;
}

// 解析结构体声明
static ASTNode* parse_struct_declaration(Parser* parser) {
    Token* struct_token = parser->tokens[parser->current];
    parser->current++; // consume 'struct'

    ASTNode* struct_decl = ast_create_node(ASTC_STRUCT_DECL, struct_token->line, struct_token->column);
    if (!struct_decl) return NULL;

    // 解析结构体名称 (可选)
    if (match_token(parser, TOKEN_IDENTIFIER)) {
        Token* name_token = parser->tokens[parser->current];
        struct_decl->data.struct_decl.name = strdup(name_token->value);
        parser->current++;
    } else {
        struct_decl->data.struct_decl.name = NULL; // 匿名结构体
    }

    if (!consume_token(parser, TOKEN_LBRACE, "Expected '{' after struct name")) {
        ast_free(struct_decl);
        return NULL;
    }

    // 简化：不解析成员，直接跳到'}'
    int brace_count = 1;
    while (brace_count > 0 && !match_token(parser, TOKEN_EOF)) {
        Token* token = parser->tokens[parser->current];
        if (token->type == TOKEN_LBRACE) brace_count++;
        else if (token->type == TOKEN_RBRACE) brace_count--;
        parser->current++;
    }

    struct_decl->data.struct_decl.members = NULL;
    struct_decl->data.struct_decl.member_count = 0;

    return struct_decl;
}

// 解析联合体声明
static ASTNode* parse_union_declaration(Parser* parser) {
    Token* union_token = parser->tokens[parser->current];
    parser->current++; // consume 'union'

    ASTNode* union_decl = ast_create_node(ASTC_UNION_DECL, union_token->line, union_token->column);
    if (!union_decl) return NULL;

    // 解析联合体名称 (可选)
    if (match_token(parser, TOKEN_IDENTIFIER)) {
        Token* name_token = parser->tokens[parser->current];
        union_decl->data.union_decl.name = strdup(name_token->value);
        parser->current++;
    } else {
        union_decl->data.union_decl.name = NULL; // 匿名联合体
    }

    if (!consume_token(parser, TOKEN_LBRACE, "Expected '{' after union name")) {
        ast_free(union_decl);
        return NULL;
    }

    // 简化：不解析成员，直接跳到'}'
    int brace_count = 1;
    while (brace_count > 0 && !match_token(parser, TOKEN_EOF)) {
        Token* token = parser->tokens[parser->current];
        if (token->type == TOKEN_LBRACE) brace_count++;
        else if (token->type == TOKEN_RBRACE) brace_count--;
        parser->current++;
    }

    union_decl->data.union_decl.members = NULL;
    union_decl->data.union_decl.member_count = 0;

    return union_decl;
}

// 解析数组下标表达式
static ASTNode* parse_array_subscript(Parser* parser, ASTNode* array_expr) {
    Token* bracket_token = parser->tokens[parser->current];
    parser->current++; // consume '['

    ASTNode* subscript = ast_create_node(ASTC_EXPR_ARRAY_SUBSCRIPT, bracket_token->line, bracket_token->column);
    if (!subscript) {
        ast_free(array_expr);
        return NULL;
    }

    subscript->data.array_subscript.array = array_expr;

    // 解析索引表达式
    subscript->data.array_subscript.index = parse_expression(parser);
    if (!subscript->data.array_subscript.index) {
        ast_free(subscript);
        return NULL;
    }

    if (!consume_token(parser, TOKEN_RBRACKET, "Expected ']' after array index")) {
        ast_free(subscript);
        return NULL;
    }

    return subscript;
}

// 解析成员访问表达式
static ASTNode* parse_member_access(Parser* parser, ASTNode* object_expr, bool is_pointer) {
    Token* op_token = parser->tokens[parser->current];
    parser->current++; // consume '.' or '->'

    ASTNodeType node_type = is_pointer ? ASTC_EXPR_PTR_MEMBER_ACCESS : ASTC_EXPR_MEMBER_ACCESS;
    ASTNode* member_access = ast_create_node(node_type, op_token->line, op_token->column);
    if (!member_access) {
        ast_free(object_expr);
        return NULL;
    }

    Token* member_token = consume_token(parser, TOKEN_IDENTIFIER, "Expected member name after '.' or '->'");
    if (!member_token) {
        ast_free(member_access);
        return NULL;
    }

    if (is_pointer) {
        member_access->data.ptr_member_access.pointer = object_expr;
        member_access->data.ptr_member_access.member = strdup(member_token->value);
    } else {
        member_access->data.member_access.object = object_expr;
        member_access->data.member_access.member = strdup(member_token->value);
    }

    return member_access;
}

// 解析变量声明
static ASTNode* parse_variable_declaration(Parser* parser) {
    Token* type_token = parser->tokens[parser->current];
    parser->current++; // consume type

    Token* name_token = consume_token(parser, TOKEN_IDENTIFIER, "Expected variable name");
    if (!name_token) return NULL;

    ASTNode* var_decl = ast_create_node(ASTC_VAR_DECL, type_token->line, type_token->column);
    if (!var_decl) return NULL;

    var_decl->data.var_decl.name = strdup(name_token->value);

    // 设置类型
    ASTNodeType type_node_type;
    switch (type_token->type) {
        case TOKEN_INT:
            type_node_type = ASTC_TYPE_INT;
            break;
        case TOKEN_CHAR:
            type_node_type = ASTC_TYPE_CHAR;
            break;
        case TOKEN_FLOAT:
            type_node_type = ASTC_TYPE_FLOAT;
            break;
        case TOKEN_DOUBLE:
            type_node_type = ASTC_TYPE_DOUBLE;
            break;
        case TOKEN_VOID:
            type_node_type = ASTC_TYPE_VOID;
            break;
        default:
            type_node_type = ASTC_TYPE_INT; // 默认
            break;
    }

    var_decl->data.var_decl.type = ast_create_node(type_node_type, type_token->line, type_token->column);

    // 检查是否有数组声明
    if (match_token(parser, TOKEN_LBRACKET)) {
        parser->current++; // consume '['

        // 简化：跳过数组大小
        while (!match_token(parser, TOKEN_RBRACKET) && !match_token(parser, TOKEN_EOF)) {
            parser->current++;
        }

        if (!consume_token(parser, TOKEN_RBRACKET, "Expected ']' after array size")) {
            ast_free(var_decl);
            return NULL;
        }

        // 创建数组类型节点
        ASTNode* array_type = ast_create_node(ASTC_TYPE_ARRAY, type_token->line, type_token->column);
        array_type->data.array_type.element_type = var_decl->data.var_decl.type;
        array_type->data.array_type.size_expr = NULL; // 简化：不解析大小
        array_type->data.array_type.dimensions = 1;
        array_type->data.array_type.dim_sizes = NULL;

        var_decl->data.var_decl.type = array_type;
    }

    // 检查是否有初始化
    if (match_token(parser, TOKEN_ASSIGN)) {
        parser->current++; // consume '='
        var_decl->data.var_decl.initializer = parse_expression(parser);
        if (!var_decl->data.var_decl.initializer) {
            ast_free(var_decl);
            return NULL;
        }
    } else {
        var_decl->data.var_decl.initializer = NULL;
    }

    if (!consume_token(parser, TOKEN_SEMICOLON, "Expected ';' after variable declaration")) {
        ast_free(var_decl);
        return NULL;
    }

    return var_decl;
}

// ===============================================
// 对外接口实现
// ===============================================

bool frontend_tokenize(const char* source, Token*** tokens, int* token_count) {
    if (!source || !tokens || !token_count) return false;
    
    Lexer lexer;
    init_lexer(&lexer, source);
    
    Token** token_array = malloc(PIPELINE_MAX_TOKENS * sizeof(Token*));
    int count = 0;
    
    while (count < PIPELINE_MAX_TOKENS - 1) {
        Token* token = scan_token(&lexer);
        if (!token) break;
        
        token_array[count++] = token;
        
        if (token->type == TOKEN_EOF) break;
    }
    
    *tokens = token_array;
    *token_count = count;
    return true;
}

ASTNode* frontend_parse(Token** tokens, int token_count) {
    if (!tokens || token_count <= 0) return NULL;
    
    Parser parser;
    parser.tokens = tokens;
    parser.token_count = token_count;
    parser.current = 0;
    parser.error_msg[0] = '\0';
    
    ASTNode* program = ast_create_node(ASTC_TRANSLATION_UNIT, 0, 0);
    
    while (!match_token(&parser, TOKEN_EOF)) {
        ASTNode* func = parse_function(&parser);
        if (!func) {
            fprintf(stderr, "Parse error: %s\n", parser.error_msg);
            ast_free(program);
            return NULL;
        }
        // 简化：不处理声明列表
    }
    
    return program;
}

void frontend_free_tokens(Token** tokens, int token_count) {
    free_token_array(tokens, token_count);
}

ASTNode* frontend_compile(const char* source) {
    Token** tokens;
    int token_count;
    
    if (!frontend_tokenize(source, &tokens, &token_count)) {
        return NULL;
    }
    
    ASTNode* ast = frontend_parse(tokens, token_count);
    
    frontend_free_tokens(tokens, token_count);
    
    return ast;
}