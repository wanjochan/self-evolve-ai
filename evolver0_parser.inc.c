/**
 * evolver0_parser.inc.c - 递归下降解析器模块
 * 被 evolver0.c 包含使用
 */

#ifndef EVOLVER0_PARSER_INC_C
#define EVOLVER0_PARSER_INC_C

// 需要先包含 evolver0_lexer.inc.c 和 evolver0_ast.inc.c

// ====================================
// 全局状态变量（替代Parser结构中缺失的字段）
// ====================================

static int g_in_loop = 0;
static int g_in_switch = 0;
static int g_error_count = 0;
static int g_max_errors = 10;
static ASTNode* g_current_function = NULL;

// ====================================
// 解析器结构
// ====================================

// Parser结构已在主文件中定义
#if 0
typedef struct {
    Token *tokens;
    int token_count;
    int current;
    
    // 错误处理
    char error_msg[256];
    int error_count;
    int max_errors;
    
    // 符号表（简化版）
    struct {
        char *names[1024];
        ASTNode *nodes[1024];
        TypeInfo *types[1024];
        int count;
    } symbols;
    
    // 当前上下文
    int in_loop;
    int in_switch;
    ASTNode *current_function;
} Parser;
#endif

// ====================================
// 前向声明
// ====================================

static ASTNode* parse_expression(Parser *parser);
static ASTNode* parse_statement(Parser *parser);
static ASTNode* parse_compound_statement(Parser *parser);
static ASTNode* parse_declaration(Parser *parser);
static ASTNode* parse_type_specifier(Parser *parser);

// ====================================
// 辅助函数
// ====================================

static void init_parser(Parser *parser, Token *tokens, int token_count) {
    parser->tokens = tokens;
    parser->token_count = token_count;
    parser->current = 0;
    parser->error_msg[0] = '\0';
    parser->symbols.count = 0;
    // 新的Parser结构没有这些字段，注释掉
    // parser->error_count = 0;
    // parser->max_errors = 10;
    // parser->in_loop = 0;
    // parser->in_switch = 0;
    // parser->current_function = NULL;
}

static int is_at_end(Parser *parser) {
    return parser->current >= parser->token_count;
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

static Token* previous_token(Parser *parser) {
    if (parser->current == 0) return NULL;
    return &parser->tokens[parser->current - 1];
}

static Token* advance(Parser *parser) {
    if (!is_at_end(parser)) parser->current++;
    return previous_token(parser);
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

static int consume(Parser *parser, TokenType type, const char *message) {
    if (check(parser, type)) {
        advance(parser);
        return 1;
    }
    
    Token *token = current_token(parser);
    if (token) {
        snprintf(parser->error_msg, sizeof(parser->error_msg),
                 "%s:%d:%d: %s", token->filename, token->line, token->column, message);
    } else {
        snprintf(parser->error_msg, sizeof(parser->error_msg), "%s", message);
    }
    return 0;
}

static void report_error(Parser *parser, const char *format, ...) {
    // 简化错误报告，使用全局错误计数
    if (g_error_count >= g_max_errors) return;
    
    Token *token = current_token(parser);
    if (!token) token = &parser->tokens[parser->token_count - 1];
    
    va_list args;
    va_start(args, format);
    vsnprintf(parser->error_msg, sizeof(parser->error_msg), format, args);
    va_end(args);
    
    fprintf(stderr, "错误 %s:%d:%d: %s\n",
            token->filename ? token->filename : "<unknown>",
            token->line, token->column, parser->error_msg);
    
    g_error_count++;
}

static void synchronize(Parser *parser) {
    advance(parser);
    
    while (!is_at_end(parser)) {
        if (previous_token(parser)->type == TOKEN_SEMICOLON) return;
        
        switch (current_token(parser)->type) {
            case TOKEN_IF:
            case TOKEN_FOR:
            case TOKEN_WHILE:
            case TOKEN_RETURN:
            case TOKEN_INT:
            case TOKEN_CHAR:
            case TOKEN_VOID:
                return;
            default:
                advance(parser);
        }
    }
}

// ====================================
// 符号表操作
// ====================================

static void add_symbol(Parser *parser, const char *name, ASTNode *node, TypeInfo *type) {
    if (parser->symbols.count >= 1024) {
        report_error(parser, "符号表已满");
        return;
    }
    
    parser->symbols.names[parser->symbols.count] = strdup(name);
    // 简化类型存储
    if (type) {
        switch (type->kind) {
            case TYPE_INT: parser->symbols.types[parser->symbols.count] = "int"; break;
            case TYPE_CHAR: parser->symbols.types[parser->symbols.count] = "char"; break;
            case TYPE_VOID: parser->symbols.types[parser->symbols.count] = "void"; break;
            default: parser->symbols.types[parser->symbols.count] = "unknown"; break;
        }
    } else {
        parser->symbols.types[parser->symbols.count] = "unknown";
    }
    parser->symbols.is_function[parser->symbols.count] = (node && (node->type == AST_FUNCTION || node->type == AST_FUNCTION_DEF));
    parser->symbols.count++;
}

static ASTNode* find_symbol(Parser *parser, const char *name) {
    // 简化实现，因为新的Parser结构没有nodes数组
    // 只返回NULL，让调用者处理
    return NULL;
}

// ====================================
// 类型解析
// ====================================

static TypeInfo* create_type_info(TypeKind kind) {
    TypeInfo *type = (TypeInfo*)calloc(1, sizeof(TypeInfo));
    if (!type) return NULL;
    
    type->kind = kind;
    
    // 设置默认大小和对齐
    switch (kind) {
        case TYPE_VOID:
            type->size = 0;
            type->alignment = 1;
            break;
        case TYPE_CHAR:
            type->size = 1;
            type->alignment = 1;
            type->is_signed = 1;
            break;
        case TYPE_SHORT:
            type->size = 2;
            type->alignment = 2;
            type->is_signed = 1;
            break;
        case TYPE_INT:
            type->size = 4;
            type->alignment = 4;
            type->is_signed = 1;
            break;
        case TYPE_LONG:
            type->size = 8;
            type->alignment = 8;
            type->is_signed = 1;
            break;
        case TYPE_FLOAT:
            type->size = 4;
            type->alignment = 4;
            break;
        case TYPE_DOUBLE:
            type->size = 8;
            type->alignment = 8;
            break;
        case TYPE_POINTER:
            type->size = 8;
            type->alignment = 8;
            break;
        default:
            type->size = 0;
            type->alignment = 1;
    }
    
    return type;
}

static ASTNode* parse_type_specifier(Parser *parser) {
    Token *token = current_token(parser);
    if (!token) return NULL;
    
    TypeInfo *type = NULL;
    
    switch (token->type) {
        case TOKEN_VOID:
            advance(parser);
            type = create_type_info(TYPE_VOID);
            break;
        case TOKEN_CHAR:
            advance(parser);
            type = create_type_info(TYPE_CHAR);
            break;
        case TOKEN_INT:
            advance(parser);
            type = create_type_info(TYPE_INT);
            break;
        case TOKEN_STRUCT:
            // TODO: 实现结构体解析
            advance(parser);
            report_error(parser, "结构体类型暂未实现");
            return NULL;
        default:
            report_error(parser, "期望类型说明符");
            return NULL;
    }
    
    // 创建类型节点
    ASTNode *node = create_ast_node(AST_TYPE_NAME, token->line, token->column, token->filename);
    node->type_info = type;
    
    return node;
}

// ====================================
// 表达式解析
// ====================================

static OperatorType token_to_binary_op(TokenType type) {
    switch (type) {
        case TOKEN_PLUS: return OP_ADD;
        case TOKEN_MINUS: return OP_SUB;
        case TOKEN_MULTIPLY: return OP_MUL;
        case TOKEN_DIVIDE: return OP_DIV;
        case TOKEN_MOD: return OP_MOD;
        case TOKEN_LESS: return OP_LT;
        case TOKEN_GREATER: return OP_GT;
        case TOKEN_LESS_EQUAL: return OP_LE;
        case TOKEN_GREATER_EQUAL: return OP_GE;
        case TOKEN_EQUAL: return OP_EQ;
        case TOKEN_NOT_EQUAL: return OP_NE;
        case TOKEN_LOGICAL_AND: return OP_AND;
        case TOKEN_LOGICAL_OR: return OP_OR;
        case TOKEN_BIT_AND: return OP_BIT_AND;
        case TOKEN_BIT_OR: return OP_BIT_OR;
        case TOKEN_BIT_XOR: return OP_BIT_XOR;
        case TOKEN_LEFT_SHIFT: return OP_LEFT_SHIFT;
        case TOKEN_RIGHT_SHIFT: return OP_RIGHT_SHIFT;
        default: return -1;
    }
}

static OperatorType token_to_unary_op(TokenType type) {
    switch (type) {
        case TOKEN_PLUS: return OP_PLUS;
        case TOKEN_MINUS: return OP_MINUS;
        case TOKEN_LOGICAL_NOT: return OP_NOT;
        case TOKEN_BIT_NOT: return OP_BIT_NOT;
        case TOKEN_INCREMENT: return OP_PRE_INC;
        case TOKEN_DECREMENT: return OP_PRE_DEC;
        case TOKEN_BIT_AND: return OP_ADDR;
        case TOKEN_MULTIPLY: return OP_DEREF;
        default: return -1;
    }
}

static OperatorType token_to_assign_op(TokenType type) {
    switch (type) {
        case TOKEN_ASSIGN: return OP_ASSIGN;
        case TOKEN_ADD_ASSIGN: return OP_ADD_ASSIGN;
        case TOKEN_SUB_ASSIGN: return OP_SUB_ASSIGN;
        case TOKEN_MUL_ASSIGN: return OP_MUL_ASSIGN;
        case TOKEN_DIV_ASSIGN: return OP_DIV_ASSIGN;
        case TOKEN_MOD_ASSIGN: return OP_MOD_ASSIGN;
        default: return -1;
    }
}

// 解析主表达式
static ASTNode* parse_primary(Parser *parser) {
    Token *token = current_token(parser);
    if (!token) return NULL;
    
    // 整数字面量
    if (token->type == TOKEN_NUMBER) {
        advance(parser);
        ASTNode *node = create_ast_node(AST_INTEGER_LITERAL, token->line, token->column, token->filename);
        node->value.int_val = strtoll(token->value, NULL, 0);
        node->type_info = create_type_info(TYPE_INT);
        return node;
    }
    
    // 字符串字面量
    if (token->type == TOKEN_STRING) {
        advance(parser);
        ASTNode *node = create_ast_node(AST_STRING_LITERAL, token->line, token->column, token->filename);
        // 去掉引号
        int len = strlen(token->value);
        node->value.str_val = (char*)malloc(len - 1);
        strncpy(node->value.str_val, token->value + 1, len - 2);
        node->value.str_val[len - 2] = '\0';
        
        // 字符串类型是 char*
        node->type_info = create_type_info(TYPE_POINTER);
        node->type_info->data.pointer.pointee = create_type_info(TYPE_CHAR);
        return node;
    }
    
    // 字符字面量
    if (token->type == TOKEN_CHAR_LITERAL) {
        advance(parser);
        ASTNode *node = create_ast_node(AST_CHAR_LITERAL, token->line, token->column, token->filename);
        // 简化：只处理单字符
        if (strlen(token->value) >= 3) {
            node->value.int_val = token->value[1];
        }
        node->type_info = create_type_info(TYPE_CHAR);
        return node;
    }
    
    // 标识符
    if (token->type == TOKEN_IDENTIFIER) {
        advance(parser);
        ASTNode *node = create_ast_node(AST_IDENTIFIER, token->line, token->column, token->filename);
        node->data.identifier.name = strdup(token->value);
        
        // 查找符号
        ASTNode *symbol = find_symbol(parser, token->value);
        if (symbol) {
            node->data.identifier.symbol = symbol;
            if (symbol->type_info) {
                node->type_info = symbol->type_info;
            }
        }
        
        return node;
    }
    
    // 括号表达式
    if (match(parser, TOKEN_LPAREN)) {
        ASTNode *expr = parse_expression(parser);
        if (!match(parser, TOKEN_RPAREN)) {
            report_error(parser, "期望 ')'");
            free_ast_node(expr);
            return NULL;
        }
        return expr;
    }
    
    report_error(parser, "期望表达式");
    return NULL;
}

// 解析后缀表达式
static ASTNode* parse_postfix(Parser *parser) {
    ASTNode *left = parse_primary(parser);
    if (!left) return NULL;
    
    while (1) {
        Token *token = current_token(parser);
        if (!token) break;
        
        // 函数调用
        if (match(parser, TOKEN_LPAREN)) {
            ASTNode *call = create_ast_node(AST_CALL_EXPR, token->line, token->column, token->filename);
            call->data.call.function = left;
            
            // 解析参数
            ASTNode **args = NULL;
            int arg_count = 0;
            int arg_capacity = 0;
            
            while (!check(parser, TOKEN_RPAREN) && !is_at_end(parser)) {
                if (arg_count >= arg_capacity) {
                    arg_capacity = arg_capacity ? arg_capacity * 2 : 4;
                    args = (ASTNode**)realloc(args, sizeof(ASTNode*) * arg_capacity);
                }
                
                ASTNode *arg = parse_expression(parser);
                if (!arg) break;
                
                args[arg_count++] = arg;
                
                if (!match(parser, TOKEN_COMMA)) break;
            }
            
            if (!match(parser, TOKEN_RPAREN)) {
                report_error(parser, "期望 ')'");
                // 清理
                for (int i = 0; i < arg_count; i++) {
                    free_ast_node(args[i]);
                }
                free(args);
                free_ast_node(call);
                return left;
            }
            
            call->data.call.args = args;
            call->data.call.arg_count = arg_count;
            
            // 设置返回类型
            if (left->type_info && left->type_info->kind == TYPE_FUNCTION) {
                call->type_info = left->type_info->data.function.return_type;
            } else {
                // 默认返回int
                call->type_info = create_type_info(TYPE_INT);
            }
            
            left = call;
        }
        // 数组下标
        else if (match(parser, TOKEN_LBRACKET)) {
            ASTNode *index = parse_expression(parser);
            if (!index) {
                report_error(parser, "期望数组下标");
                return left;
            }
            
            if (!match(parser, TOKEN_RBRACKET)) {
                report_error(parser, "期望 ']'");
                free_ast_node(index);
                return left;
            }
            
            ASTNode *array_access = create_ast_node(AST_ARRAY_SUBSCRIPT_EXPR, 
                                                   token->line, token->column, token->filename);
            array_access->data.array_sub.array = left;
            array_access->data.array_sub.index = index;
            
            // 设置类型
            if (left->type_info) {
                if (left->type_info->kind == TYPE_ARRAY) {
                    array_access->type_info = left->type_info->data.array.element;
                } else if (left->type_info->kind == TYPE_POINTER) {
                    array_access->type_info = left->type_info->data.pointer.pointee;
                }
            }
            
            left = array_access;
        }
        // 成员访问
        else if (match(parser, TOKEN_DOT) || match(parser, TOKEN_ARROW)) {
            int is_arrow = (token->type == TOKEN_ARROW);
            
            Token *member_token = current_token(parser);
            if (!member_token || member_token->type != TOKEN_IDENTIFIER) {
                report_error(parser, "期望成员名");
                return left;
            }
            advance(parser);
            
            ASTNode *member = create_ast_node(AST_MEMBER_EXPR, 
                                            token->line, token->column, token->filename);
            member->data.member.object = left;
            member->data.member.member = strdup(member_token->value);
            member->data.member.is_arrow = is_arrow;
            
            left = member;
        }
        // 后缀++/--
        else if (match(parser, TOKEN_INCREMENT)) {
            ASTNode *post_inc = create_ast_node(AST_POST_INCREMENT_EXPR,
                                              token->line, token->column, token->filename);
            post_inc->data.unary.operand = left;
            post_inc->data.unary.op = OP_POST_INC;
            post_inc->type_info = left->type_info;
            left = post_inc;
        }
        else if (match(parser, TOKEN_DECREMENT)) {
            ASTNode *post_dec = create_ast_node(AST_POST_DECREMENT_EXPR,
                                              token->line, token->column, token->filename);
            post_dec->data.unary.operand = left;
            post_dec->data.unary.op = OP_POST_DEC;
            post_dec->type_info = left->type_info;
            left = post_dec;
        }
        else {
            break;
        }
    }
    
    return left;
}

// 解析一元表达式
static ASTNode* parse_unary(Parser *parser) {
    Token *token = current_token(parser);
    if (!token) return NULL;
    
    // sizeof
    if (match(parser, TOKEN_SIZEOF)) {
        ASTNode *node = create_ast_node(AST_SIZEOF_EXPR, token->line, token->column, token->filename);
        
        // sizeof(type) 或 sizeof expr
        if (match(parser, TOKEN_LPAREN)) {
            // 尝试解析类型
            int saved_pos = parser->current;
            ASTNode *type = parse_type_specifier(parser);
            
            if (type && match(parser, TOKEN_RPAREN)) {
                // 修复：正确处理类型名
                if (type->type_info) {
                    switch (type->type_info->kind) {
                        case TYPE_INT: node->data.sizeof_expr.type_name = strdup("int"); break;
                        case TYPE_CHAR: node->data.sizeof_expr.type_name = strdup("char"); break;
                        case TYPE_VOID: node->data.sizeof_expr.type_name = strdup("void"); break;
                        default: node->data.sizeof_expr.type_name = strdup("unknown"); break;
                    }
                } else {
                    node->data.sizeof_expr.type_name = strdup("unknown");
                }
                free_ast_node(type);
            } else {
                // 回退，解析表达式
                parser->current = saved_pos;
                ASTNode *expr = parse_unary(parser);
                if (!expr) {
                    free_ast_node(node);
                    return NULL;
                }
                node->data.sizeof_expr.expr = expr;
            }
        } else {
            // sizeof expr
            ASTNode *expr = parse_unary(parser);
            if (!expr) {
                free_ast_node(node);
                return NULL;
            }
            node->data.sizeof_expr.expr = expr;
        }
        
        // sizeof 的结果类型是 size_t (这里简化为 unsigned long)
        node->type_info = create_type_info(TYPE_LONG);
        node->type_info->is_signed = 0;
        
        return node;
    }
    
    // 一元操作符
    OperatorType op = token_to_unary_op(token->type);
    if (op != -1) {
        advance(parser);
        
        ASTNode *operand = parse_unary(parser);
        if (!operand) return NULL;
        
        ASTNode *node = create_ast_node(AST_UNARY_EXPR, token->line, token->column, token->filename);
        node->data.unary.op = op;
        node->data.unary.operand = operand;
        
        // 设置类型
        switch (op) {
            case OP_ADDR:
                node->type_info = create_type_info(TYPE_POINTER);
                node->type_info->data.pointer.pointee = operand->type_info;
                break;
            case OP_DEREF:
                if (operand->type_info && operand->type_info->kind == TYPE_POINTER) {
                    node->type_info = operand->type_info->data.pointer.pointee;
                } else {
                    report_error(parser, "解引用需要指针类型");
                    node->type_info = create_type_info(TYPE_INT);
                }
                break;
            default:
                node->type_info = operand->type_info;
        }
        
        return node;
    }
    
    // 类型转换
    if (match(parser, TOKEN_LPAREN)) {
        // 尝试解析类型
        int saved_pos = parser->current;
        ASTNode *type = parse_type_specifier(parser);
        
        if (type && match(parser, TOKEN_RPAREN)) {
            // 这是类型转换
            ASTNode *expr = parse_unary(parser);
            if (!expr) {
                free_ast_node(type);
                return NULL;
            }
            
            ASTNode *cast = create_ast_node(AST_CAST_EXPR, token->line, token->column, token->filename);
            // 修复类型转换
            TypeInfo *cast_type = type->type_info;
            cast->data.cast.target_type = strdup("int"); // 简化处理
            cast->data.cast.expr = expr;
            // 设置类型信息
            cast->type_info = cast_type;
            
            free_ast_node(type);
            return cast;
        } else {
            // 回退，这是括号表达式
            parser->current = saved_pos - 1;
        }
    }
    
    return parse_postfix(parser);
}

// 解析乘法表达式
static ASTNode* parse_multiplicative(Parser *parser) {
    ASTNode *left = parse_unary(parser);
    if (!left) return NULL;
    
    while (match(parser, TOKEN_MULTIPLY) || match(parser, TOKEN_DIVIDE) || match(parser, TOKEN_MOD)) {
        Token *op_token = &parser->tokens[parser->current - 1];
        OperatorType op = token_to_binary_op(op_token->type);
        
        ASTNode *right = parse_unary(parser);
        if (!right) {
            free_ast_node(left);
            return NULL;
        }
        
        ASTNode *node = create_ast_node(AST_BINARY_EXPR, op_token->line, op_token->column, op_token->filename);
        node->data.binary.left = left;
        node->data.binary.right = right;
        node->data.binary.op = op;
        node->type_info = left->type_info;
        
        left = node;
    }
    
    return left;
}

// 解析加法表达式
static ASTNode* parse_additive(Parser *parser) {
    ASTNode *left = parse_multiplicative(parser);
    if (!left) return NULL;
    
    while (match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS)) {
        Token *op_token = &parser->tokens[parser->current - 1];
        OperatorType op = token_to_binary_op(op_token->type);
        
        ASTNode *right = parse_multiplicative(parser);
        if (!right) {
            free_ast_node(left);
            return NULL;
        }
        
        ASTNode *node = create_ast_node(AST_BINARY_EXPR, op_token->line, op_token->column, op_token->filename);
        node->data.binary.left = left;
        node->data.binary.right = right;
        node->data.binary.op = op;
        node->type_info = left->type_info;
        
        left = node;
    }
    
    return left;
}

// 解析移位表达式
static ASTNode* parse_shift(Parser *parser) {
    ASTNode *left = parse_additive(parser);
    if (!left) return NULL;
    
    while (match(parser, TOKEN_LEFT_SHIFT) || match(parser, TOKEN_RIGHT_SHIFT)) {
        Token *op_token = &parser->tokens[parser->current - 1];
        OperatorType op = token_to_binary_op(op_token->type);
        
        ASTNode *right = parse_additive(parser);
        if (!right) {
            free_ast_node(left);
            return NULL;
        }
        
        ASTNode *node = create_ast_node(AST_BINARY_EXPR, op_token->line, op_token->column, op_token->filename);
        node->data.binary.left = left;
        node->data.binary.right = right;
        node->data.binary.op = op;
        node->type_info = left->type_info;
        
        left = node;
    }
    
    return left;
}

// 解析关系表达式
static ASTNode* parse_relational(Parser *parser) {
    ASTNode *left = parse_shift(parser);
    if (!left) return NULL;
    
    while (match(parser, TOKEN_LESS) || match(parser, TOKEN_GREATER) ||
           match(parser, TOKEN_LESS_EQUAL) || match(parser, TOKEN_GREATER_EQUAL)) {
        Token *op_token = &parser->tokens[parser->current - 1];
        OperatorType op = token_to_binary_op(op_token->type);
        
        ASTNode *right = parse_shift(parser);
        if (!right) {
            free_ast_node(left);
            return NULL;
        }
        
        ASTNode *node = create_ast_node(AST_BINARY_EXPR, op_token->line, op_token->column, op_token->filename);
        node->data.binary.left = left;
        node->data.binary.right = right;
        node->data.binary.op = op;
        
        // 关系运算结果是int类型
        node->type_info = create_type_info(TYPE_INT);
        
        left = node;
    }
    
    return left;
}

// 解析相等表达式
static ASTNode* parse_equality(Parser *parser) {
    ASTNode *left = parse_relational(parser);
    if (!left) return NULL;
    
    while (match(parser, TOKEN_EQUAL) || match(parser, TOKEN_NOT_EQUAL)) {
        Token *op_token = &parser->tokens[parser->current - 1];
        OperatorType op = token_to_binary_op(op_token->type);
        
        ASTNode *right = parse_relational(parser);
        if (!right) {
            free_ast_node(left);
            return NULL;
        }
        
        ASTNode *node = create_ast_node(AST_BINARY_EXPR, op_token->line, op_token->column, op_token->filename);
        node->data.binary.left = left;
        node->data.binary.right = right;
        node->data.binary.op = op;
        node->type_info = create_type_info(TYPE_INT);
        
        left = node;
    }
    
    return left;
}

// 解析位与表达式
static ASTNode* parse_and(Parser *parser) {
    ASTNode *left = parse_equality(parser);
    if (!left) return NULL;
    
    while (match(parser, TOKEN_BIT_AND)) {
        Token *op_token = &parser->tokens[parser->current - 1];
        
        ASTNode *right = parse_equality(parser);
        if (!right) {
            free_ast_node(left);
            return NULL;
        }
        
        ASTNode *node = create_ast_node(AST_BINARY_EXPR, op_token->line, op_token->column, op_token->filename);
        node->data.binary.left = left;
        node->data.binary.right = right;
        node->data.binary.op = OP_BIT_AND;
        node->type_info = left->type_info;
        
        left = node;
    }
    
    return left;
}

// 解析位异或表达式
static ASTNode* parse_xor(Parser *parser) {
    ASTNode *left = parse_and(parser);
    if (!left) return NULL;
    
    while (match(parser, TOKEN_BIT_XOR)) {
        Token *op_token = &parser->tokens[parser->current - 1];
        
        ASTNode *right = parse_and(parser);
        if (!right) {
            free_ast_node(left);
            return NULL;
        }
        
        ASTNode *node = create_ast_node(AST_BINARY_EXPR, op_token->line, op_token->column, op_token->filename);
        node->data.binary.left = left;
        node->data.binary.right = right;
        node->data.binary.op = OP_BIT_XOR;
        node->type_info = left->type_info;
        
        left = node;
    }
    
    return left;
}

// 解析位或表达式
static ASTNode* parse_or(Parser *parser) {
    ASTNode *left = parse_xor(parser);
    if (!left) return NULL;
    
    while (match(parser, TOKEN_BIT_OR)) {
        Token *op_token = &parser->tokens[parser->current - 1];
        
        ASTNode *right = parse_xor(parser);
        if (!right) {
            free_ast_node(left);
            return NULL;
        }
        
        ASTNode *node = create_ast_node(AST_BINARY_EXPR, op_token->line, op_token->column, op_token->filename);
        node->data.binary.left = left;
        node->data.binary.right = right;
        node->data.binary.op = OP_BIT_OR;
        node->type_info = left->type_info;
        
        left = node;
    }
    
    return left;
}

// 解析逻辑与表达式
static ASTNode* parse_logical_and(Parser *parser) {
    ASTNode *left = parse_or(parser);
    if (!left) return NULL;
    
    while (match(parser, TOKEN_LOGICAL_AND)) {
        Token *op_token = &parser->tokens[parser->current - 1];
        
        ASTNode *right = parse_or(parser);
        if (!right) {
            free_ast_node(left);
            return NULL;
        }
        
        ASTNode *node = create_ast_node(AST_BINARY_EXPR, op_token->line, op_token->column, op_token->filename);
        node->data.binary.left = left;
        node->data.binary.right = right;
        node->data.binary.op = OP_AND;
        node->type_info = create_type_info(TYPE_INT);
        
        left = node;
    }
    
    return left;
}

// 解析逻辑或表达式
static ASTNode* parse_logical_or(Parser *parser) {
    ASTNode *left = parse_logical_and(parser);
    if (!left) return NULL;
    
    while (match(parser, TOKEN_LOGICAL_OR)) {
        Token *op_token = &parser->tokens[parser->current - 1];
        
        ASTNode *right = parse_logical_and(parser);
        if (!right) {
            free_ast_node(left);
            return NULL;
        }
        
        ASTNode *node = create_ast_node(AST_BINARY_EXPR, op_token->line, op_token->column, op_token->filename);
        node->data.binary.left = left;
        node->data.binary.right = right;
        node->data.binary.op = OP_OR;
        node->type_info = create_type_info(TYPE_INT);
        
        left = node;
    }
    
    return left;
}

// 解析条件表达式
static ASTNode* parse_conditional(Parser *parser) {
    ASTNode *condition = parse_logical_or(parser);
    
    if (match(parser, TOKEN_QUESTION)) {
        ASTNode *true_expr = parse_expression(parser);
        if (!match(parser, TOKEN_COLON)) {
            report_error(parser, "期望 ':'");
            free_ast_node(condition);
            free_ast_node(true_expr);
            return NULL;
        }
        ASTNode *false_expr = parse_conditional(parser);
        
        ASTNode *node = create_ast_node(AST_CONDITIONAL_EXPR, 
                                       current_token(parser)->line, 
                                       current_token(parser)->column,
                                       current_token(parser)->filename);
        
        // 使用通用的三元表达式结构
        node->data.generic.child_count = 3;
        node->data.generic.child_capacity = 3;
        node->data.generic.children = malloc(sizeof(ASTNode*) * 3);
        node->data.generic.children[0] = condition;
        node->data.generic.children[1] = true_expr;
        node->data.generic.children[2] = false_expr;
        
        return node;
    }
    
    return condition;
}

// 解析赋值表达式
static ASTNode* parse_assignment(Parser *parser) {
    ASTNode *left = parse_conditional(parser);
    if (!left) return NULL;
    
    Token *token = current_token(parser);
    if (!token) return left;
    
    OperatorType op = token_to_assign_op(token->type);
    if (op != -1) {
        advance(parser);
        
        ASTNode *right = parse_assignment(parser);
        if (!right) {
            free_ast_node(left);
            return NULL;
        }
        
        ASTNode *node = create_ast_node(AST_ASSIGNMENT_EXPR, token->line, token->column, token->filename);
        node->data.assignment.left = left;
        node->data.assignment.right = right;
        node->data.assignment.op = op;
        node->type_info = left->type_info;
        
        return node;
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
    Token *token = current_token(parser);
    
    // 空语句
    if (match(parser, TOKEN_SEMICOLON)) {
        // 使用AST_COMPOUND_STMT表示空语句
        ASTNode *node = create_ast_node(AST_COMPOUND_STMT, token->line, token->column, token->filename);
        node->data.generic.child_count = 0;
        node->data.generic.children = NULL;
        return node;
    }
    
    // 表达式语句
    ASTNode *expr = parse_expression(parser);
    if (!expr) return NULL;
    
    if (!match(parser, TOKEN_SEMICOLON)) {
        report_error(parser, "期望 ';'");
        free_ast_node(expr);
        return NULL;
    }
    
    ASTNode *node = create_ast_node(AST_EXPRESSION_STMT, token->line, token->column, token->filename);
    add_child(node, expr);
    
    return node;
}

// 解析if语句
static ASTNode* parse_if_statement(Parser *parser) {
    Token *if_token = advance(parser); // 消费 'if'
    
    if (!match(parser, TOKEN_LPAREN)) {
        report_error(parser, "期望 '('");
        return NULL;
    }
    
    ASTNode *condition = parse_expression(parser);
    if (!condition) return NULL;
    
    if (!match(parser, TOKEN_RPAREN)) {
        report_error(parser, "期望 ')'");
        free_ast_node(condition);
        return NULL;
    }
    
    ASTNode *then_stmt = parse_statement(parser);
    if (!then_stmt) {
        free_ast_node(condition);
        return NULL;
    }
    
    ASTNode *else_stmt = NULL;
    if (match(parser, TOKEN_ELSE)) {
        else_stmt = parse_statement(parser);
        if (!else_stmt) {
            free_ast_node(condition);
            free_ast_node(then_stmt);
            return NULL;
        }
    }
    
    ASTNode *node = create_ast_node(AST_IF_STMT, if_token->line, if_token->column, if_token->filename);
    node->data.if_stmt.condition = condition;
    node->data.if_stmt.then_stmt = then_stmt;
    node->data.if_stmt.else_stmt = else_stmt;
    
    return node;
}

// 解析while语句
static ASTNode* parse_while_statement(Parser *parser) {
    Token *while_token = advance(parser); // 消费 'while'
    
    if (!match(parser, TOKEN_LPAREN)) {
        report_error(parser, "期望 '('");
        return NULL;
    }
    
    ASTNode *condition = parse_expression(parser);
    if (!condition) return NULL;
    
    if (!match(parser, TOKEN_RPAREN)) {
        report_error(parser, "期望 ')'");
        free_ast_node(condition);
        return NULL;
    }
    
    g_in_loop++;
    ASTNode *body = parse_statement(parser);
    g_in_loop--;
    
    if (!body) {
        free_ast_node(condition);
        return NULL;
    }
    
    ASTNode *node = create_ast_node(AST_WHILE_STMT, while_token->line, while_token->column, while_token->filename);
    node->data.while_stmt.condition = condition;
    node->data.while_stmt.body = body;
    node->data.while_stmt.is_do_while = 0;
    
    return node;
}

// 解析do-while语句
static ASTNode* parse_do_while_statement(Parser *parser) {
    Token *do_token = advance(parser); // 消费 'do'
    
    g_in_loop++;
    ASTNode *body = parse_statement(parser);
    g_in_loop--;
    
    if (!body) return NULL;
    
    if (!match(parser, TOKEN_WHILE)) {
        report_error(parser, "期望 'while'");
        free_ast_node(body);
        return NULL;
    }
    
    if (!match(parser, TOKEN_LPAREN)) {
        report_error(parser, "期望 '('");
        free_ast_node(body);
        return NULL;
    }
    
    ASTNode *condition = parse_expression(parser);
    if (!condition) {
        free_ast_node(body);
        return NULL;
    }
    
    if (!match(parser, TOKEN_RPAREN)) {
        report_error(parser, "期望 ')'");
        free_ast_node(body);
        free_ast_node(condition);
        return NULL;
    }
    
    if (!match(parser, TOKEN_SEMICOLON)) {
        report_error(parser, "期望 ';'");
        free_ast_node(body);
        free_ast_node(condition);
        return NULL;
    }
    
    ASTNode *node = create_ast_node(AST_DO_WHILE_STMT, do_token->line, do_token->column, do_token->filename);
    node->data.while_stmt.condition = condition;
    node->data.while_stmt.body = body;
    node->data.while_stmt.is_do_while = 1;
    
    return node;
}

// 解析for语句
static ASTNode* parse_for_statement(Parser *parser) {
    Token *for_token = advance(parser); // 消费 'for'
    
    if (!match(parser, TOKEN_LPAREN)) {
        report_error(parser, "期望 '('");
        return NULL;
    }
    
    // 初始化部分
    ASTNode *init = NULL;
    if (!check(parser, TOKEN_SEMICOLON)) {
        // 可能是声明或表达式
        if (check(parser, TOKEN_INT) || check(parser, TOKEN_CHAR) || 
            check(parser, TOKEN_VOID) || check(parser, TOKEN_STRUCT)) {
            init = parse_declaration(parser);
        } else {
            init = parse_expression(parser);
            if (init && !match(parser, TOKEN_SEMICOLON)) {
                report_error(parser, "期望 ';'");
                free_ast_node(init);
                return NULL;
            }
        }
    } else {
        advance(parser); // 消费 ';'
    }
    
    // 条件部分
    ASTNode *condition = NULL;
    if (!check(parser, TOKEN_SEMICOLON)) {
        condition = parse_expression(parser);
    }
    
    if (!match(parser, TOKEN_SEMICOLON)) {
        report_error(parser, "期望 ';'");
        free_ast_node(init);
        free_ast_node(condition);
        return NULL;
    }
    
    // 增量部分
    ASTNode *increment = NULL;
    if (!check(parser, TOKEN_RPAREN)) {
        increment = parse_expression(parser);
    }
    
    if (!match(parser, TOKEN_RPAREN)) {
        report_error(parser, "期望 ')'");
        free_ast_node(init);
        free_ast_node(condition);
        free_ast_node(increment);
        return NULL;
    }
    
    g_in_loop++;
    ASTNode *body = parse_statement(parser);
    g_in_loop--;
    
    if (!body) {
        free_ast_node(init);
        free_ast_node(condition);
        free_ast_node(increment);
        return NULL;
    }
    
    ASTNode *node = create_ast_node(AST_FOR_STMT, for_token->line, for_token->column, for_token->filename);
    node->data.for_stmt.init = init;
    node->data.for_stmt.condition = condition;
    node->data.for_stmt.increment = increment;
    node->data.for_stmt.body = body;
    
    return node;
}

// 解析return语句
static ASTNode* parse_return_statement(Parser *parser) {
    Token *return_token = advance(parser); // 消费 'return'
    
    ASTNode *value = NULL;
    if (!check(parser, TOKEN_SEMICOLON)) {
        value = parse_expression(parser);
    }
    
    if (!match(parser, TOKEN_SEMICOLON)) {
        report_error(parser, "期望 ';'");
        free_ast_node(value);
        return NULL;
    }
    
    ASTNode *node = create_ast_node(AST_RETURN_STMT, return_token->line, return_token->column, return_token->filename);
    node->data.return_stmt.value = value;
    
    return node;
}

// 解析break语句
static ASTNode* parse_break_statement(Parser *parser) {
    Token *break_token = advance(parser); // 消费 'break'
    
    if (!g_in_loop && !g_in_switch) {
        report_error(parser, "break语句只能在循环或switch中使用");
    }
    
    if (!match(parser, TOKEN_SEMICOLON)) {
        report_error(parser, "期望 ';'");
        return NULL;
    }
    
    return create_ast_node(AST_BREAK_STMT, break_token->line, break_token->column, break_token->filename);
}

// 解析continue语句
static ASTNode* parse_continue_statement(Parser *parser) {
    Token *continue_token = advance(parser); // 消费 'continue'
    
    if (!g_in_loop) {
        report_error(parser, "continue语句只能在循环中使用");
    }
    
    if (!match(parser, TOKEN_SEMICOLON)) {
        report_error(parser, "期望 ';'");
        return NULL;
    }
    
    return create_ast_node(AST_CONTINUE_STMT, continue_token->line, continue_token->column, continue_token->filename);
}

// 解析goto语句
static ASTNode* parse_goto_statement(Parser *parser) {
    Token *goto_token = advance(parser); // 消费 'goto'
    
    Token *label_token = current_token(parser);
    if (!label_token || label_token->type != TOKEN_IDENTIFIER) {
        report_error(parser, "期望标签名");
        return NULL;
    }
    advance(parser);
    
    if (!match(parser, TOKEN_SEMICOLON)) {
        report_error(parser, "期望 ';'");
        return NULL;
    }
    
    ASTNode *node = create_ast_node(AST_GOTO_STMT, goto_token->line, goto_token->column, goto_token->filename);
    node->data.goto_label.label = strdup(label_token->value);
    
    return node;
}

// 解析语句
static ASTNode* parse_statement(Parser *parser) {
    Token *token = current_token(parser);
    if (!token) return NULL;
    
    // 复合语句
    if (check(parser, TOKEN_LBRACE)) {
        return parse_compound_statement(parser);
    }
    
    // 控制流语句
    switch (token->type) {
        case TOKEN_IF:
            return parse_if_statement(parser);
        case TOKEN_WHILE:
            return parse_while_statement(parser);
        case TOKEN_DO:
            return parse_do_while_statement(parser);
        case TOKEN_FOR:
            return parse_for_statement(parser);
        case TOKEN_RETURN:
            return parse_return_statement(parser);
        case TOKEN_BREAK:
            return parse_break_statement(parser);
        case TOKEN_CONTINUE:
            return parse_continue_statement(parser);
        case TOKEN_GOTO:
            return parse_goto_statement(parser);
        case TOKEN_SWITCH:
            // TODO: 实现switch语句
            report_error(parser, "switch语句暂未实现");
            return NULL;
        default:
            break;
    }
    
    // 标签语句
    if (token->type == TOKEN_IDENTIFIER && peek_token(parser, 1) && peek_token(parser, 1)->type == TOKEN_COLON) {
        Token *label_token = advance(parser);
        advance(parser); // 消费 ':'
        
        ASTNode *stmt = parse_statement(parser);
        if (!stmt) return NULL;
        
        ASTNode *node = create_ast_node(AST_LABEL_STMT, label_token->line, label_token->column, label_token->filename);
        node->data.goto_label.label = strdup(label_token->value);
        node->data.goto_label.stmt = stmt;
        
        return node;
    }
    
    // 声明或表达式语句
    if (check(parser, TOKEN_INT) || check(parser, TOKEN_CHAR) || 
        check(parser, TOKEN_VOID) || check(parser, TOKEN_STRUCT) ||
        check(parser, TOKEN_TYPEDEF) || check(parser, TOKEN_STATIC) ||
        check(parser, TOKEN_EXTERN) || check(parser, TOKEN_CONST)) {
        return parse_declaration(parser);
    }
    
    return parse_expression_statement(parser);
}

// 解析复合语句
static ASTNode* parse_compound_statement(Parser *parser) {
    Token *lbrace = advance(parser); // 消费 '{'
    
    ASTNode *node = create_ast_node(AST_COMPOUND_STMT, lbrace->line, lbrace->column, lbrace->filename);
    
    while (!check(parser, TOKEN_RBRACE) && !is_at_end(parser)) {
        ASTNode *stmt = parse_statement(parser);
        if (stmt) {
            add_child(node, stmt);
        } else {
            // 错误恢复
            if (g_error_count >= g_max_errors) {
                report_error(parser, "错误过多，停止解析");
                break;
            }
            synchronize(parser);
        }
    }
    
    if (!match(parser, TOKEN_RBRACE)) {
        report_error(parser, "期望 '}'");
    }
    
    return node;
}

// ====================================
// 声明解析
// ====================================

// 解析声明符
static ASTNode* parse_declarator(Parser *parser, TypeInfo *base_type) {
    TypeInfo *type = base_type;
    
    // 处理指针
    while (match(parser, TOKEN_MULTIPLY)) {
        TypeInfo *ptr_type = create_type_info(TYPE_POINTER);
        ptr_type->data.pointer.pointee = type;
        type = ptr_type;
    }
    
    // 必须有标识符
    Token *name_token = current_token(parser);
    if (!name_token || name_token->type != TOKEN_IDENTIFIER) {
        report_error(parser, "期望标识符");
        return NULL;
    }
    advance(parser);
    
    // 创建声明节点
    ASTNode *decl = create_ast_node(AST_VAR_DECL, name_token->line, name_token->column, name_token->filename);
    decl->data.var_decl.name = strdup(name_token->value);
    decl->type_info = type;
    
    // 处理数组
    while (match(parser, TOKEN_LBRACKET)) {
        ASTNode *size_expr = NULL;
        
        if (!check(parser, TOKEN_RBRACKET)) {
            size_expr = parse_expression(parser);
        }
        
        if (!match(parser, TOKEN_RBRACKET)) {
            report_error(parser, "期望 ']'");
            free_ast_node(decl);
            return NULL;
        }
        
        TypeInfo *array_type = create_type_info(TYPE_ARRAY);
        array_type->data.array.element = type;
        
        // 简化：假设大小是常量
        if (size_expr && size_expr->type == AST_INTEGER_LITERAL) {
            array_type->data.array.size = size_expr->value.int_val;
            array_type->size = array_type->data.array.size * type->size;
        } else {
            array_type->data.array.size = -1; // 不完整类型
            array_type->data.array.is_vla = (size_expr != NULL);
        }
        
        free_ast_node(size_expr);
        type = array_type;
        decl->type_info = type;
    }
    
    // 处理函数
    if (match(parser, TOKEN_LPAREN)) {
        TypeInfo *func_type = create_type_info(TYPE_FUNCTION);
        func_type->data.function.return_type = type;
        
        // 参数列表
        ASTNode **params = NULL;
        int param_count = 0;
        int param_capacity = 0;
        
        while (!check(parser, TOKEN_RPAREN) && !is_at_end(parser)) {
            ASTNode *param_type_node = parse_type_specifier(parser);
            if (!param_type_node) break;
            
            ASTNode *param = parse_declarator(parser, param_type_node->type_info);
            free_ast_node(param_type_node);
            
            if (!param) break;
            
            // 更改为参数声明
            param->type = AST_PARAM_DECL;
            
            if (param_count >= param_capacity) {
                param_capacity = param_capacity ? param_capacity * 2 : 4;
                params = (ASTNode**)realloc(params, sizeof(ASTNode*) * param_capacity);
            }
            
            params[param_count++] = param;
            
            if (!match(parser, TOKEN_COMMA)) break;
        }
        
        if (!match(parser, TOKEN_RPAREN)) {
            report_error(parser, "期望 ')'");
            // 清理
            for (int i = 0; i < param_count; i++) {
                free_ast_node(params[i]);
            }
            free(params);
            free_ast_node(decl);
            return NULL;
        }
        
        // 转换为函数声明
        decl->type = AST_FUNCTION_DECL;
        decl->data.function.name = decl->data.var_decl.name;
        decl->data.function.type = func_type;
        decl->data.function.params = params;
        decl->data.function.param_count = param_count;
        decl->data.function.body = NULL;
        decl->data.function.is_definition = 0;
        decl->type_info = func_type;
    }
    
    return decl;
}

// 解析声明
static ASTNode* parse_declaration(Parser *parser) {
    // 解析类型说明符
    ASTNode *type_node = parse_type_specifier(parser);
    if (!type_node) return NULL;
    
    TypeInfo *base_type = type_node->type_info;
    
    // 解析声明符列表
    ASTNode *first_decl = NULL;
    ASTNode *last_decl = NULL;
    
    do {
        ASTNode *decl = parse_declarator(parser, base_type);
        if (!decl) {
            free_ast_node(type_node);
            free_ast_node(first_decl);
            return NULL;
        }
        
        // 如果是函数声明，检查是否有函数体
        if (decl->type == AST_FUNCTION_DECL) {
            if (check(parser, TOKEN_LBRACE)) {
                // 这是函数定义
                decl->type = AST_FUNCTION_DEF;
                decl->data.function.is_definition = 1;
                
                ASTNode *old_func = g_current_function;
                g_current_function = decl;
                
                decl->data.function.body = parse_compound_statement(parser);
                
                g_current_function = old_func;
                
                if (!decl->data.function.body) {
                    free_ast_node(type_node);
                    free_ast_node(decl);
                    return NULL;
                }
                
                // 添加到符号表
                add_symbol(parser, decl->data.function.name, decl, decl->type_info);
                
                free_ast_node(type_node);
                return decl;
            }
        }
        
        // 处理初始化
        if (match(parser, TOKEN_ASSIGN)) {
            decl->data.var_decl.init = parse_expression(parser);
            if (!decl->data.var_decl.init) {
                free_ast_node(type_node);
                free_ast_node(decl);
                free_ast_node(first_decl);
                return NULL;
            }
        }
        
        // 添加到符号表
        if (decl->type == AST_VAR_DECL) {
            add_symbol(parser, decl->data.var_decl.name, decl, decl->type_info);
        } else if (decl->type == AST_FUNCTION_DECL) {
            add_symbol(parser, decl->data.function.name, decl, decl->type_info);
        }
        
        // 链接声明
        if (!first_decl) {
            first_decl = decl;
            last_decl = decl;
        } else {
            last_decl->next = decl;
            last_decl = decl;
        }
        
    } while (match(parser, TOKEN_COMMA));
    
    if (!match(parser, TOKEN_SEMICOLON)) {
        report_error(parser, "期望 ';'");
        free_ast_node(type_node);
        free_ast_node(first_decl);
        return NULL;
    }
    
    free_ast_node(type_node);
    return first_decl;
}

// ====================================
// 顶层解析
// ====================================

// 解析翻译单元
static ASTNode* parse_translation_unit(Parser *parser) {
    ASTNode *unit = create_ast_node(AST_TRANSLATION_UNIT, 1, 1, parser->tokens[0].filename);
    
    while (!is_at_end(parser)) {
        ASTNode *decl = parse_declaration(parser);
        if (decl) {
            add_child(unit, decl);
        } else {
            // 错误恢复
            if (g_error_count >= g_max_errors) {
                report_error(parser, "错误过多，停止解析");
                break;
            }
            synchronize(parser);
        }
    }
    
    return unit;
}

// ====================================
// 公共接口
// ====================================

static ASTNode* parse_tokens(Token *tokens, int token_count) {
    Parser parser;
    init_parser(&parser, tokens, token_count);
    
    // 重置全局状态
    g_in_loop = 0;
    g_in_switch = 0;
    g_error_count = 0;
    g_current_function = NULL;
    
    ASTNode *ast = parse_translation_unit(&parser);
    
    if (g_error_count > 0) {
        fprintf(stderr, "解析完成，共 %d 个错误\n", g_error_count);
    }
    
    // 清理符号表
    for (int i = 0; i < parser.symbols.count; i++) {
        free(parser.symbols.names[i]);
    }
    
    return ast;
}

#endif // EVOLVER0_PARSER_INC_C