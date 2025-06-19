/**
 * evolver0_parser.inc.c - evolver0的解析器模块
 * 这个文件被evolver0.c包含
 */

// ====================================
// 解析器结构和前向声明
// ====================================

typedef struct Parser {
    Token *tokens;
    int token_count;
    int current;
    char error_msg[256];
    
    // 符号表
    struct {
        char *names[1024];
        struct {
            enum { SYM_VAR, SYM_FUNC, SYM_TYPE } kind;
            void *data;
        } symbols[1024];
        int count;
    } symtab;
} Parser;

// 前向声明
static ASTNode* parse_expression(Parser *parser);
static ASTNode* parse_statement(Parser *parser);
static ASTNode* parse_declaration(Parser *parser);
static ASTNode* parse_compound_statement(Parser *parser);
static ASTNode* parse_type_specifier(Parser *parser);
static ASTNode* parse_declarator(Parser *parser, ASTNode *base_type);

// ====================================
// 解析器辅助函数
// ====================================

static Parser* create_parser(Token *tokens, int token_count) {
    Parser *parser = (Parser*)calloc(1, sizeof(Parser));
    parser->tokens = tokens;
    parser->token_count = token_count;
    parser->current = 0;
    return parser;
}

static void free_parser(Parser *parser) {
    if (!parser) return;
    
    // 释放符号表
    for (int i = 0; i < parser->symtab.count; i++) {
        free(parser->symtab.names[i]);
    }
    
    free(parser);
}

static int is_at_end(Parser *parser) {
    return parser->current >= parser->token_count || 
           parser->tokens[parser->current].type == TOKEN_EOF;
}

static Token* current_token(Parser *parser) {
    if (is_at_end(parser)) return NULL;
    return &parser->tokens[parser->current];
}

static Token* peek_token(Parser *parser, int offset) {
    int pos = parser->current + offset;
    if (pos >= parser->token_count) return NULL;
    return &parser->tokens[pos];
}

static Token* advance(Parser *parser) {
    if (!is_at_end(parser)) parser->current++;
    return &parser->tokens[parser->current - 1];
}

static int check(Parser *parser, TokenType type) {
    if (is_at_end(parser)) return 0;
    return current_token(parser)->type == type;
}

static int match(Parser *parser, TokenType type) {
    if (check(parser, type)) {
        advance(parser);
        return 1;
    }
    return 0;
}

static void parser_error(Parser *parser, const char *msg) {
    Token *token = current_token(parser);
    if (token) {
        snprintf(parser->error_msg, sizeof(parser->error_msg),
                 "错误 (行%d,列%d): %s", token->line, 1, msg);
    } else {
        snprintf(parser->error_msg, sizeof(parser->error_msg),
                 "错误: %s", msg);
    }
}

// ====================================
// 表达式解析
// ====================================

static ASTNode* parse_primary(Parser *parser) {
    Token *token = current_token(parser);
    if (!token) return NULL;
    
    // 整数字面量
    if (token->type == TOKEN_NUMBER) {
        advance(parser);
        return create_integer_literal(strtoll(token->value, NULL, 0), token->line, 1);
    }
    
    // 标识符或函数调用
    if (token->type == TOKEN_IDENTIFIER) {
        char *name = token->value;
        int line = token->line;
        advance(parser);
        
        // 函数调用
        if (match(parser, TOKEN_LPAREN)) {
            ASTNode **args = NULL;
            int num_args = 0;
            
            if (!check(parser, TOKEN_RPAREN)) {
                // 最多支持32个参数
                args = malloc(sizeof(ASTNode*) * 32);
                
                do {
                    args[num_args++] = parse_expression(parser);
                } while (match(parser, TOKEN_COMMA) && num_args < 32);
            }
            
            if (!match(parser, TOKEN_RPAREN)) {
                parser_error(parser, "期望 ')'");
                if (args) free(args);
                return NULL;
            }
            
            return create_function_call(name, args, num_args, line, 1);
        }
        
        // 普通标识符
        return create_identifier_node(name, line, 1);
    }
    
    // 括号表达式
    if (match(parser, TOKEN_LPAREN)) {
        ASTNode *expr = parse_expression(parser);
        if (!match(parser, TOKEN_RPAREN)) {
            parser_error(parser, "期望 ')'");
            free_ast_node(expr);
            return NULL;
        }
        return expr;
    }
    
    parser_error(parser, "期望表达式");
    return NULL;
}

// 解析后缀表达式
static ASTNode* parse_postfix(Parser *parser) {
    ASTNode *expr = parse_primary(parser);
    
    while (expr) {
        if (match(parser, TOKEN_LBRACKET)) {
            // 数组访问
            ASTNode *index = parse_expression(parser);
            if (!match(parser, TOKEN_RBRACKET)) {
                parser_error(parser, "期望 ']'");
                free_ast_node(expr);
                free_ast_node(index);
                return NULL;
            }
            expr = create_array_access(expr, index, expr->line, expr->column);
        } else if (match(parser, TOKEN_INCREMENT) || match(parser, TOKEN_DECREMENT)) {
            // 后缀++/--
            Token *op = &parser->tokens[parser->current - 1];
            expr = create_ast_node(NODE_UNARY_OP, expr->line, expr->column);
            expr->data.expr.expr_type = (op->type == TOKEN_INCREMENT) ? EXPR_POST_INC : EXPR_POST_DEC;
            expr->data.expr.lhs = expr;
        } else {
            break;
        }
    }
    
    return expr;
}

// 解析一元表达式
static ASTNode* parse_unary(Parser *parser) {
    Token *token = current_token(parser);
    
    if (match(parser, TOKEN_MINUS) || match(parser, TOKEN_PLUS) ||
        match(parser, TOKEN_LOGICAL_NOT) || match(parser, TOKEN_BIT_NOT) ||
        match(parser, TOKEN_INCREMENT) || match(parser, TOKEN_DECREMENT)) {
        
        UnaryOp op;
        switch (token->type) {
            case TOKEN_MINUS: op = OP_NEG; break;
            case TOKEN_PLUS: op = OP_POS; break;
            case TOKEN_LOGICAL_NOT: op = OP_LNOT; break;
            case TOKEN_BIT_NOT: op = OP_BNOT; break;
            case TOKEN_INCREMENT: op = OP_PRE_INC; break;
            case TOKEN_DECREMENT: op = OP_PRE_DEC; break;
            default: op = OP_NEG;
        }
        
        ASTNode *operand = parse_unary(parser);
        ASTNode *node = create_ast_node(NODE_UNARY_OP, token->line, 1);
        node->data.expr.expr_type = EXPR_UNARY;
        node->data.expr.lhs = operand;
        // 存储操作符 - 这里需要修改AST结构来支持
        return node;
    }
    
    return parse_postfix(parser);
}

// 解析乘除表达式
static ASTNode* parse_multiplicative(Parser *parser) {
    ASTNode *left = parse_unary(parser);
    
    while (match(parser, TOKEN_MULTIPLY) || match(parser, TOKEN_DIVIDE) || match(parser, TOKEN_MOD)) {
        Token *op = &parser->tokens[parser->current - 1];
        ASTNode *right = parse_unary(parser);
        
        BinaryOp binop;
        switch (op->type) {
            case TOKEN_MULTIPLY: binop = OP_MUL; break;
            case TOKEN_DIVIDE: binop = OP_DIV; break;
            case TOKEN_MOD: binop = OP_MOD; break;
            default: binop = OP_MUL;
        }
        
        left = create_binary_op(binop, left, right, op->line, 1);
    }
    
    return left;
}

// 解析加减表达式
static ASTNode* parse_additive(Parser *parser) {
    ASTNode *left = parse_multiplicative(parser);
    
    while (match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS)) {
        Token *op = &parser->tokens[parser->current - 1];
        ASTNode *right = parse_multiplicative(parser);
        
        BinaryOp binop = (op->type == TOKEN_PLUS) ? OP_ADD : OP_SUB;
        left = create_binary_op(binop, left, right, op->line, 1);
    }
    
    return left;
}

// 解析关系表达式
static ASTNode* parse_relational(Parser *parser) {
    ASTNode *left = parse_additive(parser);
    
    while (match(parser, TOKEN_LESS) || match(parser, TOKEN_GREATER) ||
           match(parser, TOKEN_LESS_EQUAL) || match(parser, TOKEN_GREATER_EQUAL)) {
        Token *op = &parser->tokens[parser->current - 1];
        ASTNode *right = parse_additive(parser);
        
        BinaryOp binop;
        switch (op->type) {
            case TOKEN_LESS: binop = OP_LT; break;
            case TOKEN_GREATER: binop = OP_GT; break;
            case TOKEN_LESS_EQUAL: binop = OP_LE; break;
            case TOKEN_GREATER_EQUAL: binop = OP_GE; break;
            default: binop = OP_LT;
        }
        
        left = create_binary_op(binop, left, right, op->line, 1);
    }
    
    return left;
}

// 解析相等表达式
static ASTNode* parse_equality(Parser *parser) {
    ASTNode *left = parse_relational(parser);
    
    while (match(parser, TOKEN_EQUAL) || match(parser, TOKEN_NOT_EQUAL)) {
        Token *op = &parser->tokens[parser->current - 1];
        ASTNode *right = parse_relational(parser);
        
        BinaryOp binop = (op->type == TOKEN_EQUAL) ? OP_EQ : OP_NE;
        left = create_binary_op(binop, left, right, op->line, 1);
    }
    
    return left;
}

// 解析赋值表达式
static ASTNode* parse_assignment(Parser *parser) {
    ASTNode *left = parse_equality(parser);
    
    if (match(parser, TOKEN_ASSIGN)) {
        Token *op = &parser->tokens[parser->current - 1];
        ASTNode *right = parse_assignment(parser);
        
        return create_binary_op(OP_ASSIGN, left, right, op->line, 1);
    }
    
    return left;
}

// 解析表达式
static ASTNode* parse_expression(Parser *parser) {
    return parse_assignment(parser);
}

// ====================================
// 语句解析
// ====================================

// 解析表达式语句
static ASTNode* parse_expression_statement(Parser *parser) {
    ASTNode *expr = parse_expression(parser);
    if (!expr) return NULL;
    
    if (!match(parser, TOKEN_SEMICOLON)) {
        parser_error(parser, "期望 ';'");
        free_ast_node(expr);
        return NULL;
    }
    
    ASTNode *stmt = create_ast_node(NODE_EXPRESSION_STMT, expr->line, expr->column);
    stmt->data.stmt.cond = expr;
    return stmt;
}

// 解析return语句
static ASTNode* parse_return_statement(Parser *parser) {
    Token *ret_token = advance(parser); // 消费 'return'
    
    ASTNode *expr = NULL;
    if (!check(parser, TOKEN_SEMICOLON)) {
        expr = parse_expression(parser);
    }
    
    if (!match(parser, TOKEN_SEMICOLON)) {
        parser_error(parser, "期望 ';'");
        free_ast_node(expr);
        return NULL;
    }
    
    return create_return_stmt(expr, ret_token->line, 1);
}

// 解析if语句
static ASTNode* parse_if_statement(Parser *parser) {
    Token *if_token = advance(parser); // 消费 'if'
    
    if (!match(parser, TOKEN_LPAREN)) {
        parser_error(parser, "期望 '('");
        return NULL;
    }
    
    ASTNode *cond = parse_expression(parser);
    if (!cond) return NULL;
    
    if (!match(parser, TOKEN_RPAREN)) {
        parser_error(parser, "期望 ')'");
        free_ast_node(cond);
        return NULL;
    }
    
    ASTNode *then_stmt = parse_statement(parser);
    if (!then_stmt) {
        free_ast_node(cond);
        return NULL;
    }
    
    ASTNode *else_stmt = NULL;
    if (match(parser, TOKEN_ELSE)) {
        else_stmt = parse_statement(parser);
    }
    
    return create_if_stmt(cond, then_stmt, else_stmt, if_token->line, 1);
}

// 解析while语句
static ASTNode* parse_while_statement(Parser *parser) {
    Token *while_token = advance(parser); // 消费 'while'
    
    if (!match(parser, TOKEN_LPAREN)) {
        parser_error(parser, "期望 '('");
        return NULL;
    }
    
    ASTNode *cond = parse_expression(parser);
    if (!cond) return NULL;
    
    if (!match(parser, TOKEN_RPAREN)) {
        parser_error(parser, "期望 ')'");
        free_ast_node(cond);
        return NULL;
    }
    
    ASTNode *body = parse_statement(parser);
    if (!body) {
        free_ast_node(cond);
        return NULL;
    }
    
    return create_while_loop(cond, body, while_token->line, 1);
}

// 解析for语句
static ASTNode* parse_for_statement(Parser *parser) {
    Token *for_token = advance(parser); // 消费 'for'
    
    if (!match(parser, TOKEN_LPAREN)) {
        parser_error(parser, "期望 '('");
        return NULL;
    }
    
    // 初始化部分
    ASTNode *init = NULL;
    if (!check(parser, TOKEN_SEMICOLON)) {
        if (check(parser, TOKEN_INT) || check(parser, TOKEN_CHAR)) {
            init = parse_declaration(parser);
        } else {
            init = parse_expression(parser);
            match(parser, TOKEN_SEMICOLON);
        }
    } else {
        match(parser, TOKEN_SEMICOLON);
    }
    
    // 条件部分
    ASTNode *cond = NULL;
    if (!check(parser, TOKEN_SEMICOLON)) {
        cond = parse_expression(parser);
    }
    match(parser, TOKEN_SEMICOLON);
    
    // 增量部分
    ASTNode *inc = NULL;
    if (!check(parser, TOKEN_RPAREN)) {
        inc = parse_expression(parser);
    }
    
    if (!match(parser, TOKEN_RPAREN)) {
        parser_error(parser, "期望 ')'");
        free_ast_node(init);
        free_ast_node(cond);
        free_ast_node(inc);
        return NULL;
    }
    
    ASTNode *body = parse_statement(parser);
    if (!body) {
        free_ast_node(init);
        free_ast_node(cond);
        free_ast_node(inc);
        return NULL;
    }
    
    ASTNode *node = create_ast_node(NODE_FOR_STMT, for_token->line, 1);
    node->data.stmt.init = init;
    node->data.stmt.cond = cond;
    node->data.stmt.inc = inc;
    node->data.stmt.body = body;
    
    return node;
}

// 解析语句
static ASTNode* parse_statement(Parser *parser) {
    Token *token = current_token(parser);
    if (!token) return NULL;
    
    switch (token->type) {
        case TOKEN_RETURN:
            return parse_return_statement(parser);
            
        case TOKEN_IF:
            return parse_if_statement(parser);
            
        case TOKEN_WHILE:
            return parse_while_statement(parser);
            
        case TOKEN_FOR:
            return parse_for_statement(parser);
            
        case TOKEN_LBRACE:
            return parse_compound_statement(parser);
            
        case TOKEN_SEMICOLON:
            advance(parser);
            return create_ast_node(NODE_EXPRESSION_STMT, token->line, 1);
            
        default:
            // 尝试解析声明或表达式语句
            if (token->type == TOKEN_INT || token->type == TOKEN_CHAR ||
                token->type == TOKEN_VOID || token->type == TOKEN_STRUCT) {
                return parse_declaration(parser);
            }
            return parse_expression_statement(parser);
    }
}

// 解析复合语句
static ASTNode* parse_compound_statement(Parser *parser) {
    Token *lbrace = advance(parser); // 消费 '{'
    
    ASTNode *compound = create_ast_node(NODE_COMPOUND_STMT, lbrace->line, 1);
    
    // 预分配语句数组
    int capacity = 32;
    compound->data.expr.args = malloc(sizeof(ASTNode*) * capacity);
    compound->data.expr.num_args = 0;
    
    while (!check(parser, TOKEN_RBRACE) && !is_at_end(parser)) {
        ASTNode *stmt = parse_statement(parser);
        if (stmt) {
            if (compound->data.expr.num_args >= capacity) {
                capacity *= 2;
                compound->data.expr.args = realloc(compound->data.expr.args, 
                                                  sizeof(ASTNode*) * capacity);
            }
            compound->data.expr.args[compound->data.expr.num_args++] = stmt;
        } else {
            // 错误恢复：跳过到下一个语句
            while (!is_at_end(parser) && !check(parser, TOKEN_SEMICOLON) && 
                   !check(parser, TOKEN_RBRACE)) {
                advance(parser);
            }
            if (check(parser, TOKEN_SEMICOLON)) advance(parser);
        }
    }
    
    if (!match(parser, TOKEN_RBRACE)) {
        parser_error(parser, "期望 '}'");
        free_ast_node(compound);
        return NULL;
    }
    
    return compound;
}

// ====================================
// 声明解析
// ====================================

// 解析类型说明符
static ASTNode* parse_type_specifier(Parser *parser) {
    Token *token = current_token(parser);
    if (!token) return NULL;
    
    BasicType basic_type = BT_INT; // 默认
    
    switch (token->type) {
        case TOKEN_VOID:
            basic_type = BT_VOID;
            advance(parser);
            break;
        case TOKEN_CHAR:
            basic_type = BT_CHAR;
            advance(parser);
            break;
        case TOKEN_INT:
            basic_type = BT_INT;
            advance(parser);
            break;
        case TOKEN_FLOAT:
            basic_type = BT_FLOAT;
            advance(parser);
            break;
        case TOKEN_DOUBLE:
            basic_type = BT_DOUBLE;
            advance(parser);
            break;
        default:
            parser_error(parser, "期望类型说明符");
            return NULL;
    }
    
    ASTNode *type_node = create_ast_node(NODE_TYPE_SPECIFIER, token->line, 1);
    type_node->type_info.basic_type = basic_type;
    
    return type_node;
}

// 解析声明符
static ASTNode* parse_declarator(Parser *parser, ASTNode *base_type) {
    int pointer_count = 0;
    
    // 处理指针
    while (match(parser, TOKEN_MULTIPLY)) {
        pointer_count++;
    }
    
    Token *name_token = current_token(parser);
    if (!name_token || name_token->type != TOKEN_IDENTIFIER) {
        parser_error(parser, "期望标识符");
        return NULL;
    }
    advance(parser);
    
    ASTNode *declarator = create_ast_node(NODE_DECLARATOR, name_token->line, 1);
    declarator->id.name = strdup(name_token->value);
    
    // 处理数组
    while (match(parser, TOKEN_LBRACKET)) {
        if (check(parser, TOKEN_NUMBER)) {
            Token *size_token = advance(parser);
            // TODO: 处理数组大小
        }
        if (!match(parser, TOKEN_RBRACKET)) {
            parser_error(parser, "期望 ']'");
            free_ast_node(declarator);
            return NULL;
        }
    }
    
    // 处理函数
    if (match(parser, TOKEN_LPAREN)) {
        // TODO: 解析参数列表
        if (!match(parser, TOKEN_RPAREN)) {
            parser_error(parser, "期望 ')'");
            free_ast_node(declarator);
            return NULL;
        }
    }
    
    return declarator;
}

// 解析声明
static ASTNode* parse_declaration(Parser *parser) {
    ASTNode *type_spec = parse_type_specifier(parser);
    if (!type_spec) return NULL;
    
    ASTNode *declarator = parse_declarator(parser, type_spec);
    if (!declarator) {
        free_ast_node(type_spec);
        return NULL;
    }
    
    ASTNode *init = NULL;
    if (match(parser, TOKEN_ASSIGN)) {
        init = parse_expression(parser);
    }
    
    if (!match(parser, TOKEN_SEMICOLON)) {
        parser_error(parser, "期望 ';'");
        free_ast_node(type_spec);
        free_ast_node(declarator);
        free_ast_node(init);
        return NULL;
    }
    
    return create_var_decl(declarator->id.name, type_spec, init, declarator->line, declarator->column);
}

// ====================================
// 顶层解析
// ====================================

// 解析函数定义
static ASTNode* parse_function_definition(Parser *parser) {
    ASTNode *type_spec = parse_type_specifier(parser);
    if (!type_spec) return NULL;
    
    Token *name_token = current_token(parser);
    if (!name_token || name_token->type != TOKEN_IDENTIFIER) {
        parser_error(parser, "期望函数名");
        free_ast_node(type_spec);
        return NULL;
    }
    char *func_name = name_token->value;
    int line = name_token->line;
    advance(parser);
    
    if (!match(parser, TOKEN_LPAREN)) {
        parser_error(parser, "期望 '('");
        free_ast_node(type_spec);
        return NULL;
    }
    
    // TODO: 解析参数列表
    ASTNode *params = NULL;
    
    if (!match(parser, TOKEN_RPAREN)) {
        parser_error(parser, "期望 ')'");
        free_ast_node(type_spec);
        return NULL;
    }
    
    ASTNode *body = parse_compound_statement(parser);
    if (!body) {
        free_ast_node(type_spec);
        return NULL;
    }
    
    return create_function_decl(func_name, type_spec, params, body, line, 1);
}

// 解析翻译单元
static ASTNode* parse_translation_unit(Parser *parser) {
    ASTNode *unit = create_ast_node(NODE_TRANSLATION_UNIT, 1, 1);
    
    // 预分配数组
    int capacity = 16;
    unit->data.expr.args = malloc(sizeof(ASTNode*) * capacity);
    unit->data.expr.num_args = 0;
    
    while (!is_at_end(parser)) {
        ASTNode *decl = parse_function_definition(parser);
        if (decl) {
            if (unit->data.expr.num_args >= capacity) {
                capacity *= 2;
                unit->data.expr.args = realloc(unit->data.expr.args,
                                              sizeof(ASTNode*) * capacity);
            }
            unit->data.expr.args[unit->data.expr.num_args++] = decl;
        } else {
            // 错误恢复
            while (!is_at_end(parser) && !check(parser, TOKEN_INT) &&
                   !check(parser, TOKEN_VOID) && !check(parser, TOKEN_CHAR)) {
                advance(parser);
            }
        }
    }
    
    return unit;
}

// ====================================
// 主解析函数
// ====================================

static ASTNode* parse_c_code(Token *tokens, int token_count) {
    Parser *parser = create_parser(tokens, token_count);
    ASTNode *ast = parse_translation_unit(parser);
    
    if (parser->error_msg[0] != '\0') {
        fprintf(stderr, "%s\n", parser->error_msg);
    }
    
    free_parser(parser);
    return ast;
}

// ====================================
// AST打印函数（用于调试）
// ====================================

static void print_ast_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

static void print_ast(ASTNode *node) {
    print_ast_node(node, 0);
}

static void print_ast_node(ASTNode *node, int indent) {
    if (!node) return;
    
    print_ast_indent(indent);
    
    switch (node->node_type) {
        case NODE_TRANSLATION_UNIT:
            printf("TranslationUnit\n");
            for (int i = 0; i < node->data.expr.num_args; i++) {
                print_ast_node(node->data.expr.args[i], indent + 1);
            }
            break;
            
        case NODE_FUNCTION_DECL:
            printf("FunctionDecl: %s\n", node->decl.name);
            print_ast_indent(indent + 1);
            printf("ReturnType:\n");
            print_ast_node(node->decl.type, indent + 2);
            if (node->decl.body) {
                print_ast_indent(indent + 1);
                printf("Body:\n");
                print_ast_node(node->decl.body, indent + 2);
            }
            break;
            
        case NODE_VAR_DECL:
            printf("VarDecl: %s\n", node->decl.name);
            if (node->decl.init) {
                print_ast_indent(indent + 1);
                printf("Init:\n");
                print_ast_node(node->decl.init, indent + 2);
            }
            break;
            
        case NODE_COMPOUND_STMT:
            printf("CompoundStmt\n");
            for (int i = 0; i < node->data.expr.num_args; i++) {
                print_ast_node(node->data.expr.args[i], indent + 1);
            }
            break;
            
        case NODE_RETURN_STMT:
            printf("ReturnStmt\n");
            if (node->data.stmt.cond) {
                print_ast_node(node->data.stmt.cond, indent + 1);
            }
            break;
            
        case NODE_IF_STMT:
            printf("IfStmt\n");
            print_ast_indent(indent + 1);
            printf("Condition:\n");
            print_ast_node(node->data.stmt.cond, indent + 2);
            print_ast_indent(indent + 1);
            printf("Then:\n");
            print_ast_node(node->data.stmt.then, indent + 2);
            if (node->data.stmt.else_) {
                print_ast_indent(indent + 1);
                printf("Else:\n");
                print_ast_node(node->data.stmt.else_, indent + 2);
            }
            break;
            
        case NODE_WHILE_STMT:
            printf("WhileStmt\n");
            print_ast_indent(indent + 1);
            printf("Condition:\n");
            print_ast_node(node->data.stmt.cond, indent + 2);
            print_ast_indent(indent + 1);
            printf("Body:\n");
            print_ast_node(node->data.stmt.body, indent + 2);
            break;
            
        case NODE_FOR_STMT:
            printf("ForStmt\n");
            if (node->data.stmt.init) {
                print_ast_indent(indent + 1);
                printf("Init:\n");
                print_ast_node(node->data.stmt.init, indent + 2);
            }
            if (node->data.stmt.cond) {
                print_ast_indent(indent + 1);
                printf("Condition:\n");
                print_ast_node(node->data.stmt.cond, indent + 2);
            }
            if (node->data.stmt.inc) {
                print_ast_indent(indent + 1);
                printf("Increment:\n");
                print_ast_node(node->data.stmt.inc, indent + 2);
            }
            print_ast_indent(indent + 1);
            printf("Body:\n");
            print_ast_node(node->data.stmt.body, indent + 2);
            break;
            
        case NODE_BINARY_OP:
            printf("BinaryOp: ");
            switch (node->data.expr.lhs->data.expr.expr_type) {
                case OP_ADD: printf("+\n"); break;
                case OP_SUB: printf("-\n"); break;
                case OP_MUL: printf("*\n"); break;
                case OP_DIV: printf("/\n"); break;
                case OP_MOD: printf("%%\n"); break;
                case OP_LT: printf("<\n"); break;
                case OP_GT: printf(">\n"); break;
                case OP_LE: printf("<=\n"); break;
                case OP_GE: printf(">=\n"); break;
                case OP_EQ: printf("==\n"); break;
                case OP_NE: printf("!=\n"); break;
                case OP_ASSIGN: printf("=\n"); break;
                default: printf("?\n"); break;
            }
            print_ast_node(node->data.expr.lhs, indent + 1);
            print_ast_node(node->data.expr.rhs, indent + 1);
            break;
            
        case NODE_UNARY_OP:
            printf("UnaryOp\n");
            print_ast_node(node->data.expr.lhs, indent + 1);
            break;
            
        case NODE_FUNCTION_CALL:
            printf("FunctionCall: %s\n", node->id.name);
            for (int i = 0; i < node->data.expr.num_args; i++) {
                print_ast_node(node->data.expr.args[i], indent + 1);
            }
            break;
            
        case NODE_IDENTIFIER:
            printf("Identifier: %s\n", node->id.name);
            break;
            
        case NODE_INTEGER_LITERAL:
            printf("IntegerLiteral: %lld\n", node->value.int_val);
            break;
            
        case NODE_TYPE_SPECIFIER:
            printf("TypeSpecifier: ");
            switch (node->type_info.basic_type) {
                case BT_VOID: printf("void\n"); break;
                case BT_CHAR: printf("char\n"); break;
                case BT_INT: printf("int\n"); break;
                case BT_FLOAT: printf("float\n"); break;
                case BT_DOUBLE: printf("double\n"); break;
                default: printf("?\n"); break;
            }
            break;
            
        case NODE_EXPRESSION_STMT:
            printf("ExpressionStmt\n");
            if (node->data.stmt.cond) {
                print_ast_node(node->data.stmt.cond, indent + 1);
            }
            break;
            
        default:
            printf("Unknown node type: %d\n", node->node_type);
            break;
    }
}