/**
 * evolver0_parser.inc.c - 解析器模块
 * 这个文件被 evolver0.c 包含，提供完整的C语言解析功能
 */

// ====================================
// 解析器结构体和辅助函数
// ====================================

typedef struct Parser {
    Token *tokens;          // Token数组
    int token_count;        // Token总数
    int current;            // 当前Token索引
    int error_count;        // 错误计数
    char error_msg[256];    // 错误信息
    
    // 简单的符号表
    struct {
        char *names[1024];
        ASTNode *nodes[1024];
        int count;
    } symbols;
} Parser;

// 前向声明
static ASTNode* parse_expression(Parser *parser);
static ASTNode* parse_statement(Parser *parser);
static ASTNode* parse_declaration(Parser *parser);
static ASTNode* parse_type_specifier(Parser *parser);
static ASTNode* parse_declarator(Parser *parser, ASTNode *base_type);
static ASTNode* parse_compound_statement(Parser *parser);
static ASTNode* parse_assignment_expression(Parser *parser);

// ====================================
// 解析器辅助函数
// ====================================

static Token* current_token(Parser *parser) {
    if (parser->current >= parser->token_count) {
        return &parser->tokens[parser->token_count - 1]; // EOF
    }
    return &parser->tokens[parser->current];
}

static Token* peek_token(Parser *parser, int offset) {
    int index = parser->current + offset;
    if (index >= parser->token_count) {
        return &parser->tokens[parser->token_count - 1]; // EOF
    }
    if (index < 0) {
        return &parser->tokens[0];
    }
    return &parser->tokens[index];
}

static bool match(Parser *parser, TokenType type) {
    if (current_token(parser)->type == type) {
        parser->current++;
        return true;
    }
    return false;
}

static bool check(Parser *parser, TokenType type) {
    return current_token(parser)->type == type;
}

static void advance(Parser *parser) {
    if (parser->current < parser->token_count - 1) {
        parser->current++;
    }
}

static void parser_error(Parser *parser, const char *msg) {
    Token *token = current_token(parser);
    snprintf(parser->error_msg, sizeof(parser->error_msg),
             "Error at line %d: %s (got '%s')", 
             token->line, msg, token->value ? token->value : "EOF");
    parser->error_count++;
    fprintf(stderr, "%s\n", parser->error_msg);
}

static bool expect(Parser *parser, TokenType type, const char *msg) {
    if (!match(parser, type)) {
        parser_error(parser, msg);
        return false;
    }
    return true;
}

// 同步到下一个语句或声明
static void synchronize(Parser *parser) {
    while (!check(parser, TOKEN_EOF)) {
        if (check(parser, TOKEN_SEMICOLON)) {
            advance(parser);
            return;
        }
        
        switch (current_token(parser)->type) {
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_FOR:
            case TOKEN_DO:
            case TOKEN_SWITCH:
            case TOKEN_RETURN:
            case TOKEN_BREAK:
            case TOKEN_CONTINUE:
            case TOKEN_INT:
            case TOKEN_CHAR:
            case TOKEN_FLOAT:
            case TOKEN_DOUBLE:
            case TOKEN_VOID:
            case TOKEN_STRUCT:
            case TOKEN_UNION:
            case TOKEN_ENUM:
            case TOKEN_TYPEDEF:
            case TOKEN_STATIC:
            case TOKEN_EXTERN:
            case TOKEN_CONST:
                return;
            default:
                advance(parser);
        }
    }
}

// ====================================
// 表达式解析
// ====================================

// 解析基本表达式
static ASTNode* parse_primary_expression(Parser *parser) {
    Token *token = current_token(parser);
    
    switch (token->type) {
        case TOKEN_IDENTIFIER: {
            ASTNode *node = create_identifier_node(token->value, token->line, 0);
            advance(parser);
            return node;
        }
        
        case TOKEN_NUMBER: {
            long long value = strtoll(token->value, NULL, 0);
            ASTNode *node = create_integer_literal(value, token->line, 0);
            advance(parser);
            return node;
        }
        
        case TOKEN_FLOAT_NUMBER: {
            double value = strtod(token->value, NULL);
            ASTNode *node = create_ast_node(NODE_FLOAT_LITERAL, token->line, 0);
            node->value.float_val = value;
            advance(parser);
            return node;
        }
        
        case TOKEN_STRING: {
            ASTNode *node = create_ast_node(NODE_STRING_LITERAL, token->line, 0);
            node->value.str_val.str = strdup(token->value);
            node->value.str_val.len = strlen(token->value);
            advance(parser);
            return node;
        }
        
        case TOKEN_CHAR_LITERAL: {
            ASTNode *node = create_ast_node(NODE_CHAR_LITERAL, token->line, 0);
            node->value.int_val = token->value[0];
            advance(parser);
            return node;
        }
        
        case TOKEN_LPAREN: {
            advance(parser);
            ASTNode *expr = parse_expression(parser);
            expect(parser, TOKEN_RPAREN, "Expected ')' after expression");
            return expr;
        }
        
        default:
            parser_error(parser, "Expected primary expression");
            return NULL;
    }
}

// 解析后缀表达式
static ASTNode* parse_postfix_expression(Parser *parser) {
    ASTNode *expr = parse_primary_expression(parser);
    
    while (expr) {
        Token *token = current_token(parser);
        
        switch (token->type) {
            case TOKEN_LBRACKET: {
                advance(parser);
                ASTNode *index = parse_expression(parser);
                expect(parser, TOKEN_RBRACKET, "Expected ']' after array index");
                expr = create_array_access(expr, index, token->line, 0);
                break;
            }
            
            case TOKEN_LPAREN: {
                advance(parser);
                ASTNode **args = NULL;
                int num_args = 0;
                int capacity = 0;
                
                if (!check(parser, TOKEN_RPAREN)) {
                    do {
                        if (num_args >= capacity) {
                            capacity = capacity ? capacity * 2 : 4;
                            args = realloc(args, capacity * sizeof(ASTNode*));
                        }
                        args[num_args++] = parse_assignment_expression(parser);
                    } while (match(parser, TOKEN_COMMA));
                }
                
                expect(parser, TOKEN_RPAREN, "Expected ')' after function arguments");
                
                ASTNode *call = create_ast_node(NODE_FUNCTION_CALL, token->line, 0);
                call->data.expr.lhs = expr;
                call->data.expr.args = args;
                call->data.expr.num_args = num_args;
                expr = call;
                break;
            }
            
            case TOKEN_DOT: {
                advance(parser);
                expect(parser, TOKEN_IDENTIFIER, "Expected member name after '.'");
                Token *member = peek_token(parser, -1);
                
                ASTNode *access = create_ast_node(NODE_MEMBER_ACCESS, token->line, 0);
                access->data.expr.lhs = expr;
                access->data.expr.rhs = create_identifier_node(member->value, member->line, 0);
                expr = access;
                break;
            }
            
            case TOKEN_ARROW: {
                advance(parser);
                expect(parser, TOKEN_IDENTIFIER, "Expected member name after '->'");
                Token *member = peek_token(parser, -1);
                
                ASTNode *access = create_ast_node(NODE_MEMBER_ACCESS, token->line, 0);
                access->data.expr.lhs = expr;
                access->data.expr.rhs = create_identifier_node(member->value, member->line, 0);
                access->data.expr.expr_type = EXPR_MEMBER_ACCESS; // 标记为指针成员访问
                expr = access;
                break;
            }
            
            case TOKEN_INCREMENT: {
                advance(parser);
                ASTNode *inc = create_ast_node(NODE_UNARY_OP, token->line, 0);
                inc->data.expr.expr_type = EXPR_UNARY;
                inc->data.expr.lhs = expr;
                expr = inc;
                break;
            }
            
            case TOKEN_DECREMENT: {
                advance(parser);
                ASTNode *dec = create_ast_node(NODE_UNARY_OP, token->line, 0);
                dec->data.expr.expr_type = EXPR_UNARY;
                dec->data.expr.lhs = expr;
                expr = dec;
                break;
            }
            
            default:
                return expr;
        }
    }
    
    return expr;
}

// 解析一元表达式
static ASTNode* parse_unary_expression(Parser *parser) {
    Token *token = current_token(parser);
    
    switch (token->type) {
        case TOKEN_INCREMENT:
        case TOKEN_DECREMENT:
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_BIT_NOT:
        case TOKEN_LOGICAL_NOT:
        case TOKEN_BIT_AND:  // Address-of
        case TOKEN_MULTIPLY: { // Dereference
            advance(parser);
            ASTNode *operand = parse_unary_expression(parser);
            ASTNode *unary = create_ast_node(NODE_UNARY_OP, token->line, 0);
            unary->data.expr.lhs = operand;
            
            // 设置操作符类型
            switch (token->type) {
                case TOKEN_INCREMENT: unary->data.expr.expr_type = EXPR_UNARY; break;
                case TOKEN_DECREMENT: unary->data.expr.expr_type = EXPR_UNARY; break;
                case TOKEN_PLUS: unary->data.expr.expr_type = EXPR_UNARY; break;
                case TOKEN_MINUS: unary->data.expr.expr_type = EXPR_UNARY; break;
                case TOKEN_BIT_NOT: unary->data.expr.expr_type = EXPR_UNARY; break;
                case TOKEN_LOGICAL_NOT: unary->data.expr.expr_type = EXPR_UNARY; break;
                case TOKEN_BIT_AND: unary->data.expr.expr_type = EXPR_UNARY; break;
                case TOKEN_MULTIPLY: unary->data.expr.expr_type = EXPR_UNARY; break;
            }
            
            return unary;
        }
        
        case TOKEN_SIZEOF: {
            advance(parser);
            ASTNode *sizeof_expr = create_ast_node(NODE_SIZEOF_EXPR, token->line, 0);
            
            if (match(parser, TOKEN_LPAREN)) {
                // sizeof(type) or sizeof(expr)
                // 简化处理：假设是表达式
                sizeof_expr->data.expr.lhs = parse_expression(parser);
                expect(parser, TOKEN_RPAREN, "Expected ')' after sizeof");
            } else {
                sizeof_expr->data.expr.lhs = parse_unary_expression(parser);
            }
            
            return sizeof_expr;
        }
        
        case TOKEN_LPAREN: {
            // 可能是类型转换
            int saved_pos = parser->current;
            advance(parser);
            
            // 尝试解析类型
            ASTNode *type = parse_type_specifier(parser);
            if (type && match(parser, TOKEN_RPAREN)) {
                // 是类型转换
                ASTNode *cast = create_ast_node(NODE_CAST_EXPR, token->line, 0);
                cast->data.expr.cast_type = type;
                cast->data.expr.lhs = parse_unary_expression(parser);
                return cast;
            }
            
            // 不是类型转换，回退
            parser->current = saved_pos;
            return parse_postfix_expression(parser);
        }
        
        default:
            return parse_postfix_expression(parser);
    }
}

// 解析乘法表达式
static ASTNode* parse_multiplicative_expression(Parser *parser) {
    ASTNode *left = parse_unary_expression(parser);
    
    while (left) {
        Token *op = current_token(parser);
        BinaryOp op_type;
        
        switch (op->type) {
            case TOKEN_MULTIPLY: op_type = OP_MUL; break;
            case TOKEN_DIVIDE: op_type = OP_DIV; break;
            case TOKEN_MOD: op_type = OP_MOD; break;
            default: return left;
        }
        
        advance(parser);
        ASTNode *right = parse_unary_expression(parser);
        left = create_binary_op(op_type, left, right, op->line, 0);
    }
    
    return left;
}

// 解析加法表达式
static ASTNode* parse_additive_expression(Parser *parser) {
    ASTNode *left = parse_multiplicative_expression(parser);
    
    while (left) {
        Token *op = current_token(parser);
        BinaryOp op_type;
        
        switch (op->type) {
            case TOKEN_PLUS: op_type = OP_ADD; break;
            case TOKEN_MINUS: op_type = OP_SUB; break;
            default: return left;
        }
        
        advance(parser);
        ASTNode *right = parse_multiplicative_expression(parser);
        left = create_binary_op(op_type, left, right, op->line, 0);
    }
    
    return left;
}

// 解析移位表达式
static ASTNode* parse_shift_expression(Parser *parser) {
    ASTNode *left = parse_additive_expression(parser);
    
    while (left) {
        Token *op = current_token(parser);
        BinaryOp op_type;
        
        switch (op->type) {
            case TOKEN_LEFT_SHIFT: op_type = OP_LEFT_SHIFT; break;
            case TOKEN_RIGHT_SHIFT: op_type = OP_RIGHT_SHIFT; break;
            default: return left;
        }
        
        advance(parser);
        ASTNode *right = parse_additive_expression(parser);
        left = create_binary_op(op_type, left, right, op->line, 0);
    }
    
    return left;
}

// 解析关系表达式
static ASTNode* parse_relational_expression(Parser *parser) {
    ASTNode *left = parse_shift_expression(parser);
    
    while (left) {
        Token *op = current_token(parser);
        BinaryOp op_type;
        
        switch (op->type) {
            case TOKEN_LESS: op_type = OP_LT; break;
            case TOKEN_GREATER: op_type = OP_GT; break;
            case TOKEN_LESS_EQUAL: op_type = OP_LE; break;
            case TOKEN_GREATER_EQUAL: op_type = OP_GE; break;
            default: return left;
        }
        
        advance(parser);
        ASTNode *right = parse_shift_expression(parser);
        left = create_binary_op(op_type, left, right, op->line, 0);
    }
    
    return left;
}

// 解析相等表达式
static ASTNode* parse_equality_expression(Parser *parser) {
    ASTNode *left = parse_relational_expression(parser);
    
    while (left) {
        Token *op = current_token(parser);
        BinaryOp op_type;
        
        switch (op->type) {
            case TOKEN_EQUAL: op_type = OP_EQ; break;
            case TOKEN_NOT_EQUAL: op_type = OP_NE; break;
            default: return left;
        }
        
        advance(parser);
        ASTNode *right = parse_relational_expression(parser);
        left = create_binary_op(op_type, left, right, op->line, 0);
    }
    
    return left;
}

// 解析按位与表达式
static ASTNode* parse_and_expression(Parser *parser) {
    ASTNode *left = parse_equality_expression(parser);
    
    while (match(parser, TOKEN_BIT_AND)) {
        Token *op = peek_token(parser, -1);
        ASTNode *right = parse_equality_expression(parser);
        left = create_binary_op(OP_BIT_AND, left, right, op->line, 0);
    }
    
    return left;
}

// 解析按位异或表达式
static ASTNode* parse_exclusive_or_expression(Parser *parser) {
    ASTNode *left = parse_and_expression(parser);
    
    while (match(parser, TOKEN_BIT_XOR)) {
        Token *op = peek_token(parser, -1);
        ASTNode *right = parse_and_expression(parser);
        left = create_binary_op(OP_BIT_XOR, left, right, op->line, 0);
    }
    
    return left;
}

// 解析按位或表达式
static ASTNode* parse_inclusive_or_expression(Parser *parser) {
    ASTNode *left = parse_exclusive_or_expression(parser);
    
    while (match(parser, TOKEN_BIT_OR)) {
        Token *op = peek_token(parser, -1);
        ASTNode *right = parse_exclusive_or_expression(parser);
        left = create_binary_op(OP_BIT_OR, left, right, op->line, 0);
    }
    
    return left;
}

// 解析逻辑与表达式
static ASTNode* parse_logical_and_expression(Parser *parser) {
    ASTNode *left = parse_inclusive_or_expression(parser);
    
    while (match(parser, TOKEN_LOGICAL_AND)) {
        Token *op = peek_token(parser, -1);
        ASTNode *right = parse_inclusive_or_expression(parser);
        left = create_binary_op(OP_LOGICAL_AND, left, right, op->line, 0);
    }
    
    return left;
}

// 解析逻辑或表达式
static ASTNode* parse_logical_or_expression(Parser *parser) {
    ASTNode *left = parse_logical_and_expression(parser);
    
    while (match(parser, TOKEN_LOGICAL_OR)) {
        Token *op = peek_token(parser, -1);
        ASTNode *right = parse_logical_and_expression(parser);
        left = create_binary_op(OP_LOGICAL_OR, left, right, op->line, 0);
    }
    
    return left;
}

// 解析条件表达式
static ASTNode* parse_conditional_expression(Parser *parser) {
    ASTNode *cond = parse_logical_or_expression(parser);
    
    if (match(parser, TOKEN_QUESTION)) {
        Token *op = peek_token(parser, -1);
        ASTNode *then_expr = parse_expression(parser);
        expect(parser, TOKEN_COLON, "Expected ':' in conditional expression");
        ASTNode *else_expr = parse_conditional_expression(parser);
        
        ASTNode *ternary = create_ast_node(NODE_TERNARY_OP, op->line, 0);
        ternary->data.expr.cond = cond;
        ternary->data.expr.lhs = then_expr;
        ternary->data.expr.rhs = else_expr;
        return ternary;
    }
    
    return cond;
}

// 解析赋值表达式
static ASTNode* parse_assignment_expression(Parser *parser) {
    ASTNode *left = parse_conditional_expression(parser);
    
    Token *op = current_token(parser);
    BinaryOp op_type;
    
    switch (op->type) {
        case TOKEN_ASSIGN: op_type = OP_ASSIGN; break;
        case TOKEN_ADD_ASSIGN: op_type = OP_ADD_ASSIGN; break;
        case TOKEN_SUB_ASSIGN: op_type = OP_SUB_ASSIGN; break;
        case TOKEN_MUL_ASSIGN: op_type = OP_MUL_ASSIGN; break;
        case TOKEN_DIV_ASSIGN: op_type = OP_DIV_ASSIGN; break;
        case TOKEN_MOD_ASSIGN: op_type = OP_MOD_ASSIGN; break;
        case TOKEN_LEFT_SHIFT_ASSIGN: op_type = OP_LEFT_SHIFT_ASSIGN; break;
        case TOKEN_RIGHT_SHIFT_ASSIGN: op_type = OP_RIGHT_SHIFT_ASSIGN; break;
        case TOKEN_BIT_AND_ASSIGN: op_type = OP_BIT_AND_ASSIGN; break;
        case TOKEN_BIT_XOR_ASSIGN: op_type = OP_BIT_XOR_ASSIGN; break;
        case TOKEN_BIT_OR_ASSIGN: op_type = OP_BIT_OR_ASSIGN; break;
        default: return left;
    }
    
    advance(parser);
    ASTNode *right = parse_assignment_expression(parser);
    
    ASTNode *assign = create_ast_node(NODE_ASSIGNMENT, op->line, 0);
    assign->data.expr.lhs = left;
    assign->data.expr.rhs = right;
    return assign;
}

// 解析逗号表达式
static ASTNode* parse_expression(Parser *parser) {
    ASTNode *left = parse_assignment_expression(parser);
    
    if (match(parser, TOKEN_COMMA)) {
        Token *op = peek_token(parser, -1);
        ASTNode *comma = create_ast_node(NODE_COMMA_EXPR, op->line, 0);
        comma->data.expr.lhs = left;
        comma->data.expr.rhs = parse_expression(parser);
        return comma;
    }
    
    return left;
}

// ====================================
// 语句解析
// ====================================

// 解析表达式语句
static ASTNode* parse_expression_statement(Parser *parser) {
    if (match(parser, TOKEN_SEMICOLON)) {
        return NULL; // 空语句
    }
    
    ASTNode *expr = parse_expression(parser);
    expect(parser, TOKEN_SEMICOLON, "Expected ';' after expression");
    
    ASTNode *stmt = create_ast_node(NODE_EXPRESSION_STMT, expr ? expr->line : 0, 0);
    stmt->data.stmt.cond = expr;
    return stmt;
}

// 解析if语句
static ASTNode* parse_if_statement(Parser *parser) {
    Token *if_token = current_token(parser);
    advance(parser); // skip 'if'
    
    expect(parser, TOKEN_LPAREN, "Expected '(' after 'if'");
    ASTNode *cond = parse_expression(parser);
    expect(parser, TOKEN_RPAREN, "Expected ')' after if condition");
    
    ASTNode *then_stmt = parse_statement(parser);
    ASTNode *else_stmt = NULL;
    
    if (match(parser, TOKEN_ELSE)) {
        else_stmt = parse_statement(parser);
    }
    
    return create_if_stmt(cond, then_stmt, else_stmt, if_token->line, 0);
}

// 解析while语句
static ASTNode* parse_while_statement(Parser *parser) {
    Token *while_token = current_token(parser);
    advance(parser); // skip 'while'
    
    expect(parser, TOKEN_LPAREN, "Expected '(' after 'while'");
    ASTNode *cond = parse_expression(parser);
    expect(parser, TOKEN_RPAREN, "Expected ')' after while condition");
    
    ASTNode *body = parse_statement(parser);
    
    return create_while_loop(cond, body, while_token->line, 0);
}

// 解析do-while语句
static ASTNode* parse_do_while_statement(Parser *parser) {
    Token *do_token = current_token(parser);
    advance(parser); // skip 'do'
    
    ASTNode *body = parse_statement(parser);
    
    expect(parser, TOKEN_WHILE, "Expected 'while' after do body");
    expect(parser, TOKEN_LPAREN, "Expected '(' after 'while'");
    ASTNode *cond = parse_expression(parser);
    expect(parser, TOKEN_RPAREN, "Expected ')' after while condition");
    expect(parser, TOKEN_SEMICOLON, "Expected ';' after do-while");
    
    ASTNode *do_while = create_ast_node(NODE_DO_WHILE_STMT, do_token->line, 0);
    do_while->data.stmt.cond = cond;
    do_while->data.stmt.body = body;
    return do_while;
}

// 解析for语句
static ASTNode* parse_for_statement(Parser *parser) {
    Token *for_token = current_token(parser);
    advance(parser); // skip 'for'
    
    expect(parser, TOKEN_LPAREN, "Expected '(' after 'for'");
    
    // 初始化部分
    ASTNode *init = NULL;
    if (!check(parser, TOKEN_SEMICOLON)) {
        // 可能是声明或表达式
        if (check(parser, TOKEN_INT) || check(parser, TOKEN_CHAR) || 
            check(parser, TOKEN_FLOAT) || check(parser, TOKEN_DOUBLE) ||
            check(parser, TOKEN_VOID) || check(parser, TOKEN_STRUCT) ||
            check(parser, TOKEN_UNION) || check(parser, TOKEN_ENUM) ||
            check(parser, TOKEN_TYPEDEF) || check(parser, TOKEN_CONST) ||
            check(parser, TOKEN_VOLATILE) || check(parser, TOKEN_STATIC) ||
            check(parser, TOKEN_EXTERN)) {
            init = parse_declaration(parser);
        } else {
            init = parse_expression(parser);
            expect(parser, TOKEN_SEMICOLON, "Expected ';' after for init");
        }
    } else {
        advance(parser); // skip ';'
    }
    
    // 条件部分
    ASTNode *cond = NULL;
    if (!check(parser, TOKEN_SEMICOLON)) {
        cond = parse_expression(parser);
    }
    expect(parser, TOKEN_SEMICOLON, "Expected ';' after for condition");
    
    // 更新部分
    ASTNode *update = NULL;
    if (!check(parser, TOKEN_RPAREN)) {
        update = parse_expression(parser);
    }
    expect(parser, TOKEN_RPAREN, "Expected ')' after for clauses");
    
    // 循环体
    ASTNode *body = parse_statement(parser);
    
    ASTNode *for_stmt = create_ast_node(NODE_FOR_STMT, for_token->line, 0);
    for_stmt->data.stmt.init = init;
    for_stmt->data.stmt.cond = cond;
    for_stmt->data.stmt.inc = update;
    for_stmt->data.stmt.body = body;
    return for_stmt;
}

// 解析switch语句
static ASTNode* parse_switch_statement(Parser *parser) {
    Token *switch_token = current_token(parser);
    advance(parser); // skip 'switch'
    
    expect(parser, TOKEN_LPAREN, "Expected '(' after 'switch'");
    ASTNode *expr = parse_expression(parser);
    expect(parser, TOKEN_RPAREN, "Expected ')' after switch expression");
    
    ASTNode *body = parse_statement(parser);
    
    ASTNode *switch_stmt = create_ast_node(NODE_SWITCH_STMT, switch_token->line, 0);
    switch_stmt->data.stmt.cond = expr;
    switch_stmt->data.stmt.body = body;
    return switch_stmt;
}

// 解析case标签
static ASTNode* parse_case_statement(Parser *parser) {
    Token *case_token = current_token(parser);
    advance(parser); // skip 'case'
    
    ASTNode *expr = parse_conditional_expression(parser);
    expect(parser, TOKEN_COLON, "Expected ':' after case label");
    
    ASTNode *stmt = parse_statement(parser);
    
    ASTNode *case_stmt = create_ast_node(NODE_CASE_STMT, case_token->line, 0);
    case_stmt->data.stmt.cond = expr;
    case_stmt->data.stmt.body = stmt;
    return case_stmt;
}

// 解析default标签
static ASTNode* parse_default_statement(Parser *parser) {
    Token *default_token = current_token(parser);
    advance(parser); // skip 'default'
    
    expect(parser, TOKEN_COLON, "Expected ':' after default label");
    
    ASTNode *stmt = parse_statement(parser);
    
    ASTNode *default_stmt = create_ast_node(NODE_DEFAULT_STMT, default_token->line, 0);
    default_stmt->data.stmt.body = stmt;
    return default_stmt;
}

// 解析break语句
static ASTNode* parse_break_statement(Parser *parser) {
    Token *break_token = current_token(parser);
    advance(parser); // skip 'break'
    
    expect(parser, TOKEN_SEMICOLON, "Expected ';' after break");
    
    return create_ast_node(NODE_BREAK_STMT, break_token->line, 0);
}

// 解析continue语句
static ASTNode* parse_continue_statement(Parser *parser) {
    Token *continue_token = current_token(parser);
    advance(parser); // skip 'continue'
    
    expect(parser, TOKEN_SEMICOLON, "Expected ';' after continue");
    
    return create_ast_node(NODE_CONTINUE_STMT, continue_token->line, 0);
}

// 解析return语句
static ASTNode* parse_return_statement(Parser *parser) {
    Token *return_token = current_token(parser);
    advance(parser); // skip 'return'
    
    ASTNode *expr = NULL;
    if (!check(parser, TOKEN_SEMICOLON)) {
        expr = parse_expression(parser);
    }
    
    expect(parser, TOKEN_SEMICOLON, "Expected ';' after return");
    
    return create_return_stmt(expr, return_token->line, 0);
}

// 解析goto语句
static ASTNode* parse_goto_statement(Parser *parser) {
    Token *goto_token = current_token(parser);
    advance(parser); // skip 'goto'
    
    expect(parser, TOKEN_IDENTIFIER, "Expected label after goto");
    Token *label = peek_token(parser, -1);
    
    expect(parser, TOKEN_SEMICOLON, "Expected ';' after goto");
    
    ASTNode *goto_stmt = create_ast_node(NODE_GOTO_STMT, goto_token->line, 0);
    goto_stmt->data.stmt.label = strdup(label->value);
    return goto_stmt;
}

// 解析标签语句
static ASTNode* parse_labeled_statement(Parser *parser) {
    Token *label = current_token(parser);
    advance(parser); // skip identifier
    advance(parser); // skip ':'
    
    ASTNode *stmt = parse_statement(parser);
    
    ASTNode *labeled = create_ast_node(NODE_LABEL_STMT, label->line, 0);
    labeled->data.stmt.label = strdup(label->value);
    labeled->data.stmt.body = stmt;
    return labeled;
}

// 解析复合语句
static ASTNode* parse_compound_statement(Parser *parser) {
    Token *lbrace = current_token(parser);
    expect(parser, TOKEN_LBRACE, "Expected '{'");
    
    ASTNode *compound = create_ast_node(NODE_COMPOUND_STMT, lbrace->line, 0);
    ASTNode **stmts = NULL;
    int count = 0;
    int capacity = 0;
    
    while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
        ASTNode *stmt = NULL;
        
        // 检查是否是声明
        if (check(parser, TOKEN_INT) || check(parser, TOKEN_CHAR) || 
            check(parser, TOKEN_FLOAT) || check(parser, TOKEN_DOUBLE) ||
            check(parser, TOKEN_VOID) || check(parser, TOKEN_STRUCT) ||
            check(parser, TOKEN_UNION) || check(parser, TOKEN_ENUM) ||
            check(parser, TOKEN_TYPEDEF) || check(parser, TOKEN_CONST) ||
            check(parser, TOKEN_VOLATILE) || check(parser, TOKEN_STATIC) ||
            check(parser, TOKEN_EXTERN)) {
            stmt = parse_declaration(parser);
        } else {
            stmt = parse_statement(parser);
        }
        
        if (stmt) {
            if (count >= capacity) {
                capacity = capacity ? capacity * 2 : 8;
                stmts = realloc(stmts, capacity * sizeof(ASTNode*));
            }
            stmts[count++] = stmt;
        }
    }
    
    expect(parser, TOKEN_RBRACE, "Expected '}'");
    
    compound->data.stmt.body = (ASTNode*)stmts; // 临时存储
    compound->data.stmt.has_else = count; // 临时存储计数
    return compound;
}

// 解析语句
static ASTNode* parse_statement(Parser *parser) {
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
            
        case TOKEN_CASE:
            return parse_case_statement(parser);
            
        case TOKEN_DEFAULT:
            return parse_default_statement(parser);
            
        case TOKEN_BREAK:
            return parse_break_statement(parser);
            
        case TOKEN_CONTINUE:
            return parse_continue_statement(parser);
            
        case TOKEN_RETURN:
            return parse_return_statement(parser);
            
        case TOKEN_GOTO:
            return parse_goto_statement(parser);
            
        case TOKEN_IDENTIFIER:
            // 检查是否是标签
            if (peek_token(parser, 1)->type == TOKEN_COLON) {
                return parse_labeled_statement(parser);
            }
            // 否则是表达式语句
            return parse_expression_statement(parser);
            
        case TOKEN_SEMICOLON:
            advance(parser);
            return NULL; // 空语句
            
        default:
            return parse_expression_statement(parser);
    }
}

// ====================================
// 声明解析
// ====================================

// 解析存储类说明符
static int parse_storage_class(Parser *parser) {
    int storage = 0;
    
    while (true) {
        Token *token = current_token(parser);
        switch (token->type) {
            case TOKEN_TYPEDEF:
                storage |= Q_TYPEDEF;
                advance(parser);
                break;
            case TOKEN_EXTERN:
                storage |= Q_EXTERN;
                advance(parser);
                break;
            case TOKEN_STATIC:
                storage |= Q_STATIC;
                advance(parser);
                break;
            case TOKEN_AUTO:
                storage |= Q_AUTO;
                advance(parser);
                break;
            case TOKEN_REGISTER:
                storage |= Q_REGISTER;
                advance(parser);
                break;
            default:
                return storage;
        }
    }
}

// 解析类型限定符
static int parse_type_qualifiers(Parser *parser) {
    int qualifiers = 0;
    
    while (true) {
        Token *token = current_token(parser);
        switch (token->type) {
            case TOKEN_CONST:
                qualifiers |= Q_CONST;
                advance(parser);
                break;
            case TOKEN_VOLATILE:
                qualifiers |= Q_VOLATILE;
                advance(parser);
                break;
            case TOKEN_RESTRICT:
                qualifiers |= Q_RESTRICT;
                advance(parser);
                break;
            default:
                return qualifiers;
        }
    }
}

// 解析基本类型说明符
static ASTNode* parse_type_specifier(Parser *parser) {
    Token *token = current_token(parser);
    ASTNode *type = create_ast_node(NODE_TYPE_SPECIFIER, token->line, 0);
    
    // 解析类型限定符
    int qualifiers = parse_type_qualifiers(parser);
    
    token = current_token(parser);
    
    switch (token->type) {
        case TOKEN_VOID:
            type->type = create_type(TYPE_VOID, qualifiers, 0, 0);
            advance(parser);
            break;
            
        case TOKEN_CHAR:
            type->type = create_type(TYPE_CHAR, qualifiers, 1, 1);
            advance(parser);
            break;
            
        case TOKEN_SHORT:
            advance(parser);
            if (match(parser, TOKEN_INT)) {
                // short int
            }
            type->type = create_type(TYPE_SHORT, qualifiers, 2, 2);
            break;
            
        case TOKEN_INT:
            type->type = create_type(TYPE_INT, qualifiers, 4, 4);
            advance(parser);
            break;
            
        case TOKEN_LONG:
            advance(parser);
            if (match(parser, TOKEN_LONG)) {
                // long long
                type->type = create_type(TYPE_LONG_LONG, qualifiers, 8, 8);
            } else if (match(parser, TOKEN_DOUBLE)) {
                // long double
                type->type = create_type(TYPE_LONG_DOUBLE, qualifiers, 16, 16);
            } else {
                // long 或 long int
                if (match(parser, TOKEN_INT)) {
                    // long int
                }
                type->type = create_type(TYPE_LONG, qualifiers, 8, 8);
            }
            break;
            
        case TOKEN_FLOAT:
            type->type = create_type(TYPE_FLOAT, qualifiers, 4, 4);
            advance(parser);
            break;
            
        case TOKEN_DOUBLE:
            type->type = create_type(TYPE_DOUBLE, qualifiers, 8, 8);
            advance(parser);
            break;
            
        case TOKEN_SIGNED:
            advance(parser);
            // signed可以单独使用或与其他类型组合
            if (match(parser, TOKEN_CHAR)) {
                type->type = create_type(TYPE_CHAR, qualifiers | Q_SIGNED, 1, 1);
            } else if (match(parser, TOKEN_SHORT)) {
                type->type = create_type(TYPE_SHORT, qualifiers | Q_SIGNED, 2, 2);
            } else if (match(parser, TOKEN_LONG)) {
                if (match(parser, TOKEN_LONG)) {
                    type->type = create_type(TYPE_LONG_LONG, qualifiers | Q_SIGNED, 8, 8);
                } else {
                    type->type = create_type(TYPE_LONG, qualifiers | Q_SIGNED, 8, 8);
                }
            } else {
                // 默认是signed int
                type->type = create_type(TYPE_INT, qualifiers | Q_SIGNED, 4, 4);
            }
            break;
            
        case TOKEN_UNSIGNED:
            advance(parser);
            qualifiers |= Q_UNSIGNED;
            // unsigned可以单独使用或与其他类型组合
            if (match(parser, TOKEN_CHAR)) {
                type->type = create_type(TYPE_CHAR, qualifiers, 1, 1);
            } else if (match(parser, TOKEN_SHORT)) {
                type->type = create_type(TYPE_SHORT, qualifiers, 2, 2);
            } else if (match(parser, TOKEN_LONG)) {
                if (match(parser, TOKEN_LONG)) {
                    type->type = create_type(TYPE_LONG_LONG, qualifiers, 8, 8);
                } else {
                    type->type = create_type(TYPE_LONG, qualifiers, 8, 8);
                }
            } else {
                // 默认是unsigned int
                type->type = create_type(TYPE_INT, qualifiers, 4, 4);
            }
            break;
            
        case TOKEN_STRUCT:
        case TOKEN_UNION: {
            bool is_union = (token->type == TOKEN_UNION);
            advance(parser);
            
            char *tag = NULL;
            if (check(parser, TOKEN_IDENTIFIER)) {
                tag = strdup(current_token(parser)->value);
                advance(parser);
            }
            
            ASTNode *fields = NULL;
            if (match(parser, TOKEN_LBRACE)) {
                // 解析结构体成员
                // 简化处理：暂时跳过
                while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
                    parse_declaration(parser);
                }
                expect(parser, TOKEN_RBRACE, "Expected '}' after struct/union body");
            }
            
            type->type = create_type(is_union ? TYPE_UNION : TYPE_STRUCT, qualifiers, 0, 0);
            if (tag) {
                type->type->data.record.tag = tag;
            }
            break;
        }
            
        case TOKEN_ENUM: {
            advance(parser);
            
            char *name = NULL;
            if (check(parser, TOKEN_IDENTIFIER)) {
                name = strdup(current_token(parser)->value);
                advance(parser);
            }
            
            if (match(parser, TOKEN_LBRACE)) {
                // 解析枚举值
                // 简化处理：暂时跳过
                while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
                    if (check(parser, TOKEN_IDENTIFIER)) {
                        advance(parser);
                        if (match(parser, TOKEN_ASSIGN)) {
                            parse_conditional_expression(parser);
                        }
                        if (!match(parser, TOKEN_COMMA)) {
                            break;
                        }
                    } else {
                        break;
                    }
                }
                expect(parser, TOKEN_RBRACE, "Expected '}' after enum body");
            }
            
            type->type = create_type(TYPE_ENUM, qualifiers, 4, 4);
            if (name) {
                type->type->data.enum_type.name = name;
            }
            break;
        }
            
        case TOKEN_IDENTIFIER:
            // 可能是typedef名称
            type->type = create_type(TYPE_TYPEDEF, qualifiers, 0, 0);
            type->data.type_node.name = create_identifier_node(token->value, token->line, 0);
            advance(parser);
            break;
            
        default:
            parser_error(parser, "Expected type specifier");
            return NULL;
    }
    
    return type;
}

// 解析指针声明符
static ASTNode* parse_pointer(Parser *parser) {
    ASTNode *ptr = NULL;
    
    while (match(parser, TOKEN_MULTIPLY)) {
        ASTNode *new_ptr = create_ast_node(NODE_TYPE_SPECIFIER, 
                                          peek_token(parser, -1)->line, 0);
        new_ptr->type = create_type(TYPE_POINTER, parse_type_qualifiers(parser), 8, 8);
        
        if (ptr) {
            new_ptr->type->data.ptr.pointee = ptr->type;
        }
        ptr = new_ptr;
    }
    
    return ptr;
}

// 解析直接声明符
static ASTNode* parse_direct_declarator(Parser *parser) {
    ASTNode *declarator = NULL;
    
    if (match(parser, TOKEN_LPAREN)) {
        // 可能是函数或带括号的声明符
        if (!check(parser, TOKEN_MULTIPLY) && !check(parser, TOKEN_IDENTIFIER)) {
            // 函数参数列表
            parser->current--; // 回退
            return NULL;
        }
        
        declarator = parse_declarator(parser, NULL);
        expect(parser, TOKEN_RPAREN, "Expected ')' in declarator");
    } else if (check(parser, TOKEN_IDENTIFIER)) {
        Token *id = current_token(parser);
        declarator = create_identifier_node(id->value, id->line, 0);
        advance(parser);
    }
    
    // 处理后缀
    while (true) {
        if (match(parser, TOKEN_LBRACKET)) {
            // 数组
            ASTNode *size = NULL;
            if (!check(parser, TOKEN_RBRACKET)) {
                size = parse_conditional_expression(parser);
            }
            expect(parser, TOKEN_RBRACKET, "Expected ']' after array size");
            
            ASTNode *array = create_ast_node(NODE_TYPE_SPECIFIER, 
                                           peek_token(parser, -1)->line, 0);
            array->type = create_type(TYPE_ARRAY, 0, 0, 0);
            if (size) {
                // 简化：假设是常量
                array->type->data.array.size = 10; // 默认大小
            }
            
            // 链接声明符
            if (!declarator) {
                declarator = array;
            } else {
                // 需要正确处理数组和指针的组合
            }
        } else if (match(parser, TOKEN_LPAREN)) {
            // 函数
            ASTNode **params = NULL;
            int param_count = 0;
            int param_capacity = 0;
            
            if (!check(parser, TOKEN_RPAREN)) {
                if (match(parser, TOKEN_VOID) && check(parser, TOKEN_RPAREN)) {
                    // void参数
                } else {
                    parser->current--; // 回退
                    do {
                        if (param_count >= param_capacity) {
                            param_capacity = param_capacity ? param_capacity * 2 : 4;
                            params = realloc(params, param_capacity * sizeof(ASTNode*));
                        }
                        
                        // 解析参数声明
                        int storage = parse_storage_class(parser);
                        ASTNode *param_type = parse_type_specifier(parser);
                        ASTNode *param_decl = parse_declarator(parser, param_type);
                        
                        ASTNode *param = create_ast_node(NODE_PARAMETER, 
                                                       param_type->line, 0);
                        param->data.decl.type = param_type;
                        param->data.decl.name = param_decl ? 
                            strdup(param_decl->data.decl.name) : NULL;
                        
                        params[param_count++] = param;
                        
                    } while (match(parser, TOKEN_COMMA));
                }
            }
            
            expect(parser, TOKEN_RPAREN, "Expected ')' after parameters");
            
            ASTNode *func = create_ast_node(NODE_TYPE_SPECIFIER, 
                                          peek_token(parser, -1)->line, 0);
            func->type = create_type(TYPE_FUNCTION, 0, 0, 0);
            func->type->data.func.params = params;
            func->type->data.func.num_params = param_count;
            
            // 链接声明符
            if (!declarator) {
                declarator = func;
            } else {
                // 需要正确处理函数和其他类型的组合
            }
        } else {
            break;
        }
    }
    
    return declarator;
}

// 解析声明符
static ASTNode* parse_declarator(Parser *parser, ASTNode *base_type) {
    ASTNode *ptr = parse_pointer(parser);
    ASTNode *declarator = parse_direct_declarator(parser);
    
    if (ptr && declarator) {
        // 组合指针和声明符
        ptr->type->data.ptr.pointee = base_type ? base_type->type : NULL;
        declarator->type = ptr->type;
    } else if (ptr) {
        ptr->type->data.ptr.pointee = base_type ? base_type->type : NULL;
        declarator = ptr;
    }
    
    return declarator;
}

// 解析初始化器
static ASTNode* parse_initializer(Parser *parser) {
    if (match(parser, TOKEN_LBRACE)) {
        // 初始化列表
        ASTNode *init_list = create_ast_node(NODE_INIT_LIST, 
                                           peek_token(parser, -1)->line, 0);
        ASTNode **elements = NULL;
        int count = 0;
        int capacity = 0;
        
        while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
            if (count >= capacity) {
                capacity = capacity ? capacity * 2 : 4;
                elements = realloc(elements, capacity * sizeof(ASTNode*));
            }
            
            elements[count++] = parse_initializer(parser);
            
            if (!match(parser, TOKEN_COMMA)) {
                break;
            }
        }
        
        expect(parser, TOKEN_RBRACE, "Expected '}' after initializer list");
        
        init_list->data.expr.args = elements;
        init_list->data.expr.num_args = count;
        return init_list;
    } else {
        return parse_assignment_expression(parser);
    }
}

// 解析声明
static ASTNode* parse_declaration(Parser *parser) {
    Token *start = current_token(parser);
    
    // 解析存储类说明符
    int storage_class = parse_storage_class(parser);
    
    // 解析类型说明符
    ASTNode *type_spec = parse_type_specifier(parser);
    if (!type_spec) {
        return NULL;
    }
    
    // 解析声明符列表
    ASTNode *first_decl = NULL;
    ASTNode *last_decl = NULL;
    
    do {
        ASTNode *declarator = parse_declarator(parser, type_spec);
        
        if (declarator && declarator->node_type == NODE_IDENTIFIER) {
            // 变量声明
            ASTNode *init = NULL;
            if (match(parser, TOKEN_ASSIGN)) {
                init = parse_initializer(parser);
            }
            
            ASTNode *var_decl = create_var_decl(
                declarator->data.decl.name,
                type_spec,
                init,
                start->line, 0
            );
            var_decl->data.decl.storage_class = storage_class;
            
            if (!first_decl) {
                first_decl = var_decl;
                last_decl = var_decl;
            } else {
                last_decl->data.decl.semantic = var_decl; // 链接多个声明
                last_decl = var_decl;
            }
        } else if (declarator && declarator->type && 
                   declarator->type->kind == TYPE_FUNCTION) {
            // 函数声明或定义
            ASTNode *body = NULL;
            if (check(parser, TOKEN_LBRACE)) {
                body = parse_compound_statement(parser);
            } else {
                expect(parser, TOKEN_SEMICOLON, "Expected ';' after function declaration");
            }
            
            ASTNode *func_decl = create_function_decl(
                declarator->data.decl.name,
                type_spec,
                NULL, // 参数已在declarator中
                body,
                start->line, 0
            );
            func_decl->data.decl.storage_class = storage_class;
            
            return func_decl;
        }
        
    } while (match(parser, TOKEN_COMMA));
    
    expect(parser, TOKEN_SEMICOLON, "Expected ';' after declaration");
    
    return first_decl;
}

// ====================================
// 顶层解析函数
// ====================================

// 解析翻译单元（整个源文件）
static ASTNode* parse_translation_unit(Parser *parser) {
    ASTNode *unit = create_ast_node(NODE_TRANSLATION_UNIT, 1, 0);
    ASTNode **decls = NULL;
    int count = 0;
    int capacity = 0;
    
    while (!check(parser, TOKEN_EOF)) {
        ASTNode *decl = parse_declaration(parser);
        
        if (decl) {
            if (count >= capacity) {
                capacity = capacity ? capacity * 2 : 8;
                decls = realloc(decls, capacity * sizeof(ASTNode*));
            }
            decls[count++] = decl;
        } else if (parser->error_count > 10) {
            // 错误太多，停止解析
            break;
        } else {
            // 尝试恢复
            synchronize(parser);
        }
    }
    
    // 将声明列表存储在翻译单元中
    if (count > 0) {
        unit->data.stmt.body = (ASTNode*)decls; // 临时存储
        unit->data.stmt.has_else = count; // 临时存储计数
    }
    
    return unit;
}

// ====================================
// 主解析函数
// ====================================

ASTNode* parse_c_code(Token *tokens, int token_count) {
    Parser parser = {
        .tokens = tokens,
        .token_count = token_count,
        .current = 0,
        .error_count = 0,
        .error_msg = {0},
        .symbols = {.count = 0}
    };
    
    ASTNode *ast = parse_translation_unit(&parser);
    
    if (parser.error_count > 0) {
        fprintf(stderr, "Parsing completed with %d errors\n", parser.error_count);
    }
    
    return ast;
}

// ====================================
// AST打印函数（用于调试）
// ====================================

static void print_ast_node(ASTNode *node, int indent);

static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

static void print_type(Type *type) {
    if (!type) {
        printf("(null type)");
        return;
    }
    
    switch (type->kind) {
        case TYPE_VOID: printf("void"); break;
        case TYPE_CHAR: 
            if (type->qualifiers & Q_UNSIGNED) printf("unsigned ");
            printf("char"); 
            break;
        case TYPE_SHORT:
            if (type->qualifiers & Q_UNSIGNED) printf("unsigned ");
            printf("short");
            break;
        case TYPE_INT:
            if (type->qualifiers & Q_UNSIGNED) printf("unsigned ");
            printf("int");
            break;
        case TYPE_LONG:
            if (type->qualifiers & Q_UNSIGNED) printf("unsigned ");
            printf("long");
            break;
        case TYPE_LONG_LONG:
            if (type->qualifiers & Q_UNSIGNED) printf("unsigned ");
            printf("long long");
            break;
        case TYPE_FLOAT: printf("float"); break;
        case TYPE_DOUBLE: printf("double"); break;
        case TYPE_LONG_DOUBLE: printf("long double"); break;
        case TYPE_POINTER:
            print_type(type->data.ptr.pointee);
            printf("*");
            break;
        case TYPE_ARRAY:
            print_type(type->data.array.element);
            printf("[%d]", type->data.array.size);
            break;
        case TYPE_FUNCTION:
            print_type(type->data.func.return_type);
            printf("(");
            for (int i = 0; i < type->data.func.num_params; i++) {
                if (i > 0) printf(", ");
                printf("param%d", i);
            }
            printf(")");
            break;
        case TYPE_STRUCT:
            printf("struct");
            if (type->data.record.tag) printf(" %s", type->data.record.tag);
            break;
        case TYPE_UNION:
            printf("union");
            if (type->data.record.tag) printf(" %s", type->data.record.tag);
            break;
        case TYPE_ENUM:
            printf("enum");
            if (type->data.enum_type.name) printf(" %s", type->data.enum_type.name);
            break;
        default:
            printf("(unknown type %d)", type->kind);
    }
    
    if (type->qualifiers & Q_CONST) printf(" const");
    if (type->qualifiers & Q_VOLATILE) printf(" volatile");
    if (type->qualifiers & Q_RESTRICT) printf(" restrict");
}

static void print_ast_node(ASTNode *node, int indent) {
    if (!node) return;
    
    print_indent(indent);
    
    switch (node->node_type) {
        case NODE_TRANSLATION_UNIT:
            printf("TranslationUnit\n");
            if (node->data.stmt.body) {
                ASTNode **decls = (ASTNode**)node->data.stmt.body;
                int count = node->data.stmt.has_else;
                for (int i = 0; i < count; i++) {
                    print_ast_node(decls[i], indent + 1);
                }
            }
            break;
            
        case NODE_FUNCTION_DEF:
        case NODE_FUNCTION_DECL:
            printf("Function: %s\n", node->data.decl.name);
            print_indent(indent + 1);
            printf("Return type: ");
            if (node->data.decl.type) {
                print_type(node->data.decl.type->type);
            }
            printf("\n");
            if (node->data.decl.body) {
                print_ast_node(node->data.decl.body, indent + 1);
            }
            break;
            
        case NODE_DECLARATION:
            printf("Declaration: %s\n", node->data.decl.name);
            if (node->data.decl.type) {
                print_indent(indent + 1);
                printf("Type: ");
                print_type(node->data.decl.type->type);
                printf("\n");
            }
            if (node->data.decl.init) {
                print_indent(indent + 1);
                printf("Initializer:\n");
                print_ast_node(node->data.decl.init, indent + 2);
            }
            break;
            
        case NODE_COMPOUND_STMT:
            printf("CompoundStatement\n");
            if (node->data.stmt.body) {
                ASTNode **stmts = (ASTNode**)node->data.stmt.body;
                int count = node->data.stmt.has_else;
                for (int i = 0; i < count; i++) {
                    print_ast_node(stmts[i], indent + 1);
                }
            }
            break;
            
        case NODE_IF_STMT:
            printf("IfStatement\n");
            print_indent(indent + 1);
            printf("Condition:\n");
            print_ast_node(node->data.stmt.cond, indent + 2);
            print_indent(indent + 1);
            printf("Then:\n");
            print_ast_node(node->data.stmt.then, indent + 2);
            if (node->data.stmt.else_) {
                print_indent(indent + 1);
                printf("Else:\n");
                print_ast_node(node->data.stmt.else_, indent + 2);
            }
            break;
            
        case NODE_WHILE_STMT:
            printf("WhileStatement\n");
            print_indent(indent + 1);
            printf("Condition:\n");
            print_ast_node(node->data.stmt.cond, indent + 2);
            print_indent(indent + 1);
            printf("Body:\n");
            print_ast_node(node->data.stmt.body, indent + 2);
            break;
            
        case NODE_FOR_STMT:
            printf("ForStatement\n");
            if (node->data.stmt.init) {
                print_indent(indent + 1);
                printf("Init:\n");
                print_ast_node(node->data.stmt.init, indent + 2);
            }
            if (node->data.stmt.cond) {
                print_indent(indent + 1);
                printf("Condition:\n");
                print_ast_node(node->data.stmt.cond, indent + 2);
            }
            if (node->data.stmt.inc) {
                print_indent(indent + 1);
                printf("Update:\n");
                print_ast_node(node->data.stmt.inc, indent + 2);
            }
            print_indent(indent + 1);
            printf("Body:\n");
            print_ast_node(node->data.stmt.body, indent + 2);
            break;
            
        case NODE_RETURN_STMT:
            printf("ReturnStatement\n");
            if (node->data.stmt.cond) {
                print_ast_node(node->data.stmt.cond, indent + 1);
            }
            break;
            
        case NODE_EXPRESSION_STMT:
            printf("ExpressionStatement\n");
            if (node->data.stmt.cond) {
                print_ast_node(node->data.stmt.cond, indent + 1);
            }
            break;
            
        case NODE_BINARY_OP:
            printf("BinaryOp: ");
            switch (node->data.expr.expr_type) {
                case EXPR_BINARY: printf("(binary op)"); break;
                default: printf("(op %d)", node->data.expr.expr_type);
            }
            printf("\n");
            print_ast_node(node->data.expr.lhs, indent + 1);
            print_ast_node(node->data.expr.rhs, indent + 1);
            break;
            
        case NODE_ASSIGNMENT:
            printf("Assignment\n");
            print_ast_node(node->data.expr.lhs, indent + 1);
            print_ast_node(node->data.expr.rhs, indent + 1);
            break;
            
        case NODE_FUNCTION_CALL:
            printf("FunctionCall\n");
            print_ast_node(node->data.expr.lhs, indent + 1);
            if (node->data.expr.args) {
                for (int i = 0; i < node->data.expr.num_args; i++) {
                    print_indent(indent + 1);
                    printf("Arg %d:\n", i);
                    print_ast_node(node->data.expr.args[i], indent + 2);
                }
            }
            break;
            
        case NODE_IDENTIFIER:
            printf("Identifier: %s\n", node->data.decl.name);
            break;
            
        case NODE_INTEGER_LITERAL:
            printf("IntegerLiteral: %lld\n", node->value.int_val);
            break;
            
        case NODE_FLOAT_LITERAL:
            printf("FloatLiteral: %f\n", node->value.float_val);
            break;
            
        case NODE_STRING_LITERAL:
            printf("StringLiteral: \"%s\"\n", node->value.str_val.str);
            break;
            
        default:
            printf("(Unknown node type %d)\n", node->node_type);
    }
}

void print_ast(ASTNode *ast) {
    printf("\n=== Abstract Syntax Tree ===\n");
    print_ast_node(ast, 0);
    printf("===========================\n\n");
}

// 辅助函数：创建类型
static Type* create_type(BasicType kind, unsigned int qualifiers, 
                        unsigned int size, unsigned int align) {
    Type *type = calloc(1, sizeof(Type));
    if (type) {
        type->kind = kind;
        type->qualifiers = qualifiers;
        type->size = size;
        type->align = align;
    }
    return type;
}