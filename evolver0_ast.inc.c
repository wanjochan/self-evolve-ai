/**
 * evolver0_ast.inc.c - AST定义和操作模块
 * 被 evolver0.c 包含使用
 */

#ifndef EVOLVER0_AST_INC_C
#define EVOLVER0_AST_INC_C

// ====================================
// AST 节点类型定义
// ====================================

typedef enum {
    // 顶层节点
    AST_TRANSLATION_UNIT,
    
    // 声明
    AST_FUNCTION_DECL,
    AST_FUNCTION_DEF,
    AST_VAR_DECL,
    AST_PARAM_DECL,
    AST_STRUCT_DECL,
    AST_UNION_DECL,
    AST_ENUM_DECL,
    AST_TYPEDEF_DECL,
    
    // 语句
    AST_COMPOUND_STMT,
    AST_EXPRESSION_STMT,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_FOR_STMT,
    AST_DO_WHILE_STMT,
    AST_SWITCH_STMT,
    AST_CASE_STMT,
    AST_DEFAULT_STMT,
    AST_BREAK_STMT,
    AST_CONTINUE_STMT,
    AST_RETURN_STMT,
    AST_GOTO_STMT,
    AST_LABEL_STMT,
    AST_NULL_STMT,
    
    // 表达式
    AST_BINARY_EXPR,
    AST_UNARY_EXPR,
    AST_ASSIGNMENT_EXPR,
    AST_CONDITIONAL_EXPR,
    AST_CALL_EXPR,
    AST_CAST_EXPR,
    AST_SIZEOF_EXPR,
    AST_ARRAY_SUBSCRIPT_EXPR,
    AST_MEMBER_EXPR,
    AST_POST_INCREMENT_EXPR,
    AST_POST_DECREMENT_EXPR,
    AST_COMMA_EXPR,
    
    // 字面量和标识符
    AST_IDENTIFIER,
    AST_INTEGER_LITERAL,
    AST_FLOAT_LITERAL,
    AST_STRING_LITERAL,
    AST_CHAR_LITERAL,
    
    // 类型
    AST_TYPE_NAME,
    AST_POINTER_TYPE,
    AST_ARRAY_TYPE,
    AST_FUNCTION_TYPE,
    AST_STRUCT_TYPE,
    AST_UNION_TYPE,
    AST_ENUM_TYPE,
    
    // 其他
    AST_INIT_LIST,
    AST_FIELD,
    AST_ENUMERATOR,
    AST_ERROR
} ASTNodeType;

// ====================================
// 运算符定义
// ====================================

typedef enum {
    // 算术运算符
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    
    // 关系运算符
    OP_LT,
    OP_GT,
    OP_LE,
    OP_GE,
    OP_EQ,
    OP_NE,
    
    // 逻辑运算符
    OP_AND,
    OP_OR,
    OP_NOT,
    
    // 位运算符
    OP_BIT_AND,
    OP_BIT_OR,
    OP_BIT_XOR,
    OP_BIT_NOT,
    OP_LEFT_SHIFT,
    OP_RIGHT_SHIFT,
    
    // 赋值运算符
    OP_ASSIGN,
    OP_ADD_ASSIGN,
    OP_SUB_ASSIGN,
    OP_MUL_ASSIGN,
    OP_DIV_ASSIGN,
    OP_MOD_ASSIGN,
    OP_BIT_AND_ASSIGN,
    OP_BIT_OR_ASSIGN,
    OP_BIT_XOR_ASSIGN,
    OP_LEFT_SHIFT_ASSIGN,
    OP_RIGHT_SHIFT_ASSIGN,
    
    // 一元运算符
    OP_PLUS,
    OP_MINUS,
    OP_ADDR,
    OP_DEREF,
    OP_PRE_INC,
    OP_PRE_DEC,
    OP_POST_INC,
    OP_POST_DEC,
    
    // 其他
    OP_ARROW,
    OP_DOT,
    OP_COMMA
} OperatorType;

// ====================================
// 类型信息
// ====================================

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
    TYPE_ENUM,
    TYPE_TYPEDEF
} TypeKind;

typedef struct TypeInfo {
    TypeKind kind;
    int size;
    int alignment;
    int is_signed;
    int is_const;
    int is_volatile;
    int is_static;
    int is_extern;
    int is_typedef;
    
    union {
        // 指针类型
        struct {
            struct TypeInfo *pointee;
        } pointer;
        
        // 数组类型
        struct {
            struct TypeInfo *element;
            int size;
            int is_vla; // 变长数组
        } array;
        
        // 函数类型
        struct {
            struct TypeInfo *return_type;
            struct TypeInfo **param_types;
            int param_count;
            int is_variadic;
        } function;
        
        // 结构体/联合体
        struct {
            char *tag;
            struct ASTNode *fields;
            int is_complete;
        } record;
        
        // 枚举
        struct {
            char *tag;
            struct ASTNode *enumerators;
        } enum_type;
        
        // typedef
        struct {
            char *name;
            struct TypeInfo *base_type;
        } typedef_type;
    } data;
} TypeInfo;

// ====================================
// AST 节点结构
// ====================================

typedef struct ASTNode {
    ASTNodeType type;
    int line;
    int column;
    const char *filename;
    
    // 类型信息
    TypeInfo *type_info;
    
    // 值（对于常量表达式）
    union {
        long long int_val;
        unsigned long long uint_val;
        double float_val;
        char *str_val;
    } value;
    
    // 子节点
    union {
        // 通用子节点
        struct {
            struct ASTNode **children;
            int child_count;
            int child_capacity;
        } generic;
        
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
        
        // 条件表达式
        struct {
            struct ASTNode *condition;
            struct ASTNode *true_expr;
            struct ASTNode *false_expr;
        } conditional;
        
        // 函数调用
        struct {
            struct ASTNode *function;
            struct ASTNode **args;
            int arg_count;
        } call;
        
        // 函数定义/声明
        struct {
            char *name;
            struct TypeInfo *type;
            struct ASTNode **params;
            int param_count;
            struct ASTNode *body;
            int is_definition;
        } function;
        
        // 变量声明
        struct {
            char *name;
            struct TypeInfo *type;
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
        
        // switch语句
        struct {
            struct ASTNode *expression;
            struct ASTNode **cases;
            int case_count;
        } switch_stmt;
        
        // case语句
        struct {
            struct ASTNode *value; // NULL for default
            struct ASTNode **stmts;
            int stmt_count;
        } case_stmt;
        
        // return语句
        struct {
            struct ASTNode *value;
        } return_stmt;
        
        // goto/label
        struct {
            char *label;
            struct ASTNode *stmt; // for label
        } goto_label;
        
        // 标识符
        struct {
            char *name;
            struct ASTNode *symbol; // 符号表链接
        } identifier;
        
        // 成员访问
        struct {
            struct ASTNode *object;
            char *member;
            int is_arrow;
        } member;
        
        // 数组下标
        struct {
            struct ASTNode *array;
            struct ASTNode *index;
        } array_sub;
        
        // 类型转换
        struct {
            struct TypeInfo *target_type;
            struct ASTNode *expr;
        } cast;
        
        // sizeof
        struct {
            struct ASTNode *expr;
            struct TypeInfo *type;
        } sizeof_expr;
        
        // 结构体/联合体/枚举定义
        struct {
            char *tag;
            struct ASTNode **members;
            int member_count;
            int is_union;
        } record;
        
        // 字段/枚举值
        struct {
            char *name;
            struct TypeInfo *type;
            struct ASTNode *value;
            int bit_width;
        } field;
    } data;
    
    // 链表（用于语句列表等）
    struct ASTNode *next;
} ASTNode;

// ====================================
// AST 创建函数
// ====================================

static ASTNode* create_ast_node(ASTNodeType type, int line, int column, const char *filename) {
    ASTNode *node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = type;
    node->line = line;
    node->column = column;
    node->filename = filename;
    
    return node;
}

static void free_ast_node(ASTNode *node) {
    if (!node) return;
    
    // 释放类型信息
    if (node->type_info) {
        // TODO: 实现类型信息释放
        free(node->type_info);
    }
    
    // 根据节点类型释放数据
    switch (node->type) {
        case AST_IDENTIFIER:
            free(node->data.identifier.name);
            break;
            
        case AST_STRING_LITERAL:
            free(node->value.str_val);
            break;
            
        case AST_BINARY_EXPR:
            free_ast_node(node->data.binary.left);
            free_ast_node(node->data.binary.right);
            break;
            
        case AST_UNARY_EXPR:
            free_ast_node(node->data.unary.operand);
            break;
            
        case AST_CALL_EXPR:
            free_ast_node(node->data.call.function);
            for (int i = 0; i < node->data.call.arg_count; i++) {
                free_ast_node(node->data.call.args[i]);
            }
            free(node->data.call.args);
            break;
            
        case AST_FUNCTION_DEF:
        case AST_FUNCTION_DECL:
            free(node->data.function.name);
            for (int i = 0; i < node->data.function.param_count; i++) {
                free_ast_node(node->data.function.params[i]);
            }
            free(node->data.function.params);
            free_ast_node(node->data.function.body);
            break;
            
        case AST_VAR_DECL:
            free(node->data.var_decl.name);
            free_ast_node(node->data.var_decl.init);
            break;
            
        case AST_COMPOUND_STMT:
            if (node->data.generic.children) {
                for (int i = 0; i < node->data.generic.child_count; i++) {
                    free_ast_node(node->data.generic.children[i]);
                }
                free(node->data.generic.children);
            }
            break;
            
        case AST_IF_STMT:
            free_ast_node(node->data.if_stmt.condition);
            free_ast_node(node->data.if_stmt.then_stmt);
            free_ast_node(node->data.if_stmt.else_stmt);
            break;
            
        case AST_WHILE_STMT:
        case AST_DO_WHILE_STMT:
            free_ast_node(node->data.while_stmt.condition);
            free_ast_node(node->data.while_stmt.body);
            break;
            
        case AST_FOR_STMT:
            free_ast_node(node->data.for_stmt.init);
            free_ast_node(node->data.for_stmt.condition);
            free_ast_node(node->data.for_stmt.increment);
            free_ast_node(node->data.for_stmt.body);
            break;
            
        case AST_RETURN_STMT:
            free_ast_node(node->data.return_stmt.value);
            break;
            
        default:
            // 其他类型的清理
            break;
    }
    
    // 释放next链
    if (node->next) {
        free_ast_node(node->next);
    }
    
    free(node);
}

// ====================================
// AST 辅助函数
// ====================================

static void add_child(ASTNode *parent, ASTNode *child) {
    if (!parent || !child) return;
    
    // 确保有足够的容量
    if (parent->data.generic.child_count >= parent->data.generic.child_capacity) {
        int new_capacity = parent->data.generic.child_capacity ? 
                          parent->data.generic.child_capacity * 2 : 4;
        ASTNode **new_children = (ASTNode**)realloc(parent->data.generic.children,
                                                    sizeof(ASTNode*) * new_capacity);
        if (!new_children) return;
        
        parent->data.generic.children = new_children;
        parent->data.generic.child_capacity = new_capacity;
    }
    
    parent->data.generic.children[parent->data.generic.child_count++] = child;
}

// ====================================
// AST 打印函数（用于调试）
// ====================================

static void print_indent(int level) {
    for (int i = 0; i < level; i++) {
        printf("  ");
    }
}

static const char* get_node_type_name(ASTNodeType type) {
    switch (type) {
        case AST_TRANSLATION_UNIT: return "TranslationUnit";
        case AST_FUNCTION_DEF: return "FunctionDef";
        case AST_FUNCTION_DECL: return "FunctionDecl";
        case AST_VAR_DECL: return "VarDecl";
        case AST_PARAM_DECL: return "ParamDecl";
        case AST_COMPOUND_STMT: return "CompoundStmt";
        case AST_EXPRESSION_STMT: return "ExpressionStmt";
        case AST_IF_STMT: return "IfStmt";
        case AST_WHILE_STMT: return "WhileStmt";
        case AST_FOR_STMT: return "ForStmt";
        case AST_RETURN_STMT: return "ReturnStmt";
        case AST_BINARY_EXPR: return "BinaryExpr";
        case AST_UNARY_EXPR: return "UnaryExpr";
        case AST_CALL_EXPR: return "CallExpr";
        case AST_IDENTIFIER: return "Identifier";
        case AST_INTEGER_LITERAL: return "IntegerLiteral";
        case AST_STRING_LITERAL: return "StringLiteral";
        default: return "Unknown";
    }
}

static const char* get_operator_name(OperatorType op) {
    switch (op) {
        case OP_ADD: return "+";
        case OP_SUB: return "-";
        case OP_MUL: return "*";
        case OP_DIV: return "/";
        case OP_MOD: return "%";
        case OP_LT: return "<";
        case OP_GT: return ">";
        case OP_LE: return "<=";
        case OP_GE: return ">=";
        case OP_EQ: return "==";
        case OP_NE: return "!=";
        case OP_AND: return "&&";
        case OP_OR: return "||";
        case OP_NOT: return "!";
        case OP_ASSIGN: return "=";
        default: return "?";
    }
}

static void print_ast_node(ASTNode *node, int level) {
    if (!node) return;
    
    print_indent(level);
    printf("%s", get_node_type_name(node->type));
    
    // 打印额外信息
    switch (node->type) {
        case AST_IDENTIFIER:
            printf(" '%s'", node->data.identifier.name);
            break;
            
        case AST_INTEGER_LITERAL:
            printf(" %lld", node->value.int_val);
            break;
            
        case AST_STRING_LITERAL:
            printf(" \"%s\"", node->value.str_val);
            break;
            
        case AST_BINARY_EXPR:
            printf(" '%s'", get_operator_name(node->data.binary.op));
            break;
            
        case AST_UNARY_EXPR:
            printf(" '%s'", get_operator_name(node->data.unary.op));
            break;
            
        case AST_FUNCTION_DEF:
        case AST_FUNCTION_DECL:
            printf(" '%s'", node->data.function.name);
            break;
            
        case AST_VAR_DECL:
            printf(" '%s'", node->data.var_decl.name);
            break;
            
        default:
            break;
    }
    
    printf(" <%d:%d>\n", node->line, node->column);
    
    // 打印子节点
    switch (node->type) {
        case AST_BINARY_EXPR:
            print_ast_node(node->data.binary.left, level + 1);
            print_ast_node(node->data.binary.right, level + 1);
            break;
            
        case AST_UNARY_EXPR:
            print_ast_node(node->data.unary.operand, level + 1);
            break;
            
        case AST_CALL_EXPR:
            print_ast_node(node->data.call.function, level + 1);
            for (int i = 0; i < node->data.call.arg_count; i++) {
                print_ast_node(node->data.call.args[i], level + 1);
            }
            break;
            
        case AST_FUNCTION_DEF:
            for (int i = 0; i < node->data.function.param_count; i++) {
                print_ast_node(node->data.function.params[i], level + 1);
            }
            print_ast_node(node->data.function.body, level + 1);
            break;
            
        case AST_VAR_DECL:
            if (node->data.var_decl.init) {
                print_ast_node(node->data.var_decl.init, level + 1);
            }
            break;
            
        case AST_COMPOUND_STMT:
            for (int i = 0; i < node->data.generic.child_count; i++) {
                print_ast_node(node->data.generic.children[i], level + 1);
            }
            break;
            
        case AST_IF_STMT:
            print_indent(level + 1);
            printf("Condition:\n");
            print_ast_node(node->data.if_stmt.condition, level + 2);
            print_indent(level + 1);
            printf("Then:\n");
            print_ast_node(node->data.if_stmt.then_stmt, level + 2);
            if (node->data.if_stmt.else_stmt) {
                print_indent(level + 1);
                printf("Else:\n");
                print_ast_node(node->data.if_stmt.else_stmt, level + 2);
            }
            break;
            
        case AST_WHILE_STMT:
            print_indent(level + 1);
            printf("Condition:\n");
            print_ast_node(node->data.while_stmt.condition, level + 2);
            print_indent(level + 1);
            printf("Body:\n");
            print_ast_node(node->data.while_stmt.body, level + 2);
            break;
            
        case AST_FOR_STMT:
            if (node->data.for_stmt.init) {
                print_indent(level + 1);
                printf("Init:\n");
                print_ast_node(node->data.for_stmt.init, level + 2);
            }
            if (node->data.for_stmt.condition) {
                print_indent(level + 1);
                printf("Condition:\n");
                print_ast_node(node->data.for_stmt.condition, level + 2);
            }
            if (node->data.for_stmt.increment) {
                print_indent(level + 1);
                printf("Increment:\n");
                print_ast_node(node->data.for_stmt.increment, level + 2);
            }
            print_indent(level + 1);
            printf("Body:\n");
            print_ast_node(node->data.for_stmt.body, level + 2);
            break;
            
        case AST_RETURN_STMT:
            if (node->data.return_stmt.value) {
                print_ast_node(node->data.return_stmt.value, level + 1);
            }
            break;
            
        case AST_EXPRESSION_STMT:
            if (node->data.generic.child_count > 0) {
                print_ast_node(node->data.generic.children[0], level + 1);
            }
            break;
            
        default:
            break;
    }
}

static void print_ast(ASTNode *root) {
    printf("=== Abstract Syntax Tree ===\n");
    print_ast_node(root, 0);
    printf("===========================\n");
}

#endif // EVOLVER0_AST_INC_C