/**
 * c99_semantic.c - C99 Semantic Analyzer Implementation
 */

#include "c99_semantic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Forward declarations
static void symbol_free(Symbol* symbol);
static Symbol* semantic_lookup_symbol_current_scope(SemanticContext* semantic, const char* name);
static bool semantic_analyze_declaration(SemanticContext* semantic, struct ASTNode* decl);
static void check_unused_symbols(SemanticContext* semantic, SymbolTable* scope);
static struct Type* analyze_type(SemanticContext* semantic, struct ASTNode* type_node);
static bool analyze_compound_statement(SemanticContext* semantic, struct ASTNode* stmt);
static bool analyze_if_statement(SemanticContext* semantic, struct ASTNode* stmt);
static bool analyze_while_statement(SemanticContext* semantic, struct ASTNode* stmt);
static bool analyze_for_statement(SemanticContext* semantic, struct ASTNode* stmt);
static bool analyze_break_statement(SemanticContext* semantic, struct ASTNode* stmt);
static bool analyze_continue_statement(SemanticContext* semantic, struct ASTNode* stmt);
static bool analyze_switch_statement(SemanticContext* semantic, struct ASTNode* stmt);
static bool analyze_return_statement(SemanticContext* semantic, struct ASTNode* stmt);
static bool analyze_goto_statement(SemanticContext* semantic, struct ASTNode* stmt);
static bool analyze_labeled_statement(SemanticContext* semantic, struct ASTNode* stmt);
static bool analyze_case_statement(SemanticContext* semantic, struct ASTNode* stmt);
static bool analyze_default_statement(SemanticContext* semantic, struct ASTNode* stmt);
static struct Type* check_binary_operation(SemanticContext* semantic, int operator, struct Type* left, struct Type* right, struct ASTNode* expr);
static struct Type* check_unary_operation(SemanticContext* semantic, int operator, struct Type* operand, struct ASTNode* expr);
static bool check_function_call(SemanticContext* semantic, Symbol* func, struct ASTNode* call);
static bool check_function_call_args(SemanticContext* semantic, struct Type* func_type, struct ASTNode* call);
static bool type_is_scalar(struct Type* type);
static struct Type* type_arithmetic_conversion(struct Type* left, struct Type* right);
static struct Type* type_implicit_conversion(struct Type* from, struct Type* to);
static bool type_can_cast(struct Type* from, struct Type* to);
static size_t type_get_alignment(struct Type* type);
static bool is_builtin_function(const char* name);
static struct Type* handle_builtin_function_call(SemanticContext* semantic, struct ASTNode* call, const char* func_name);
static uint16_t get_libc_function_id(const char* func_name);
static struct Type* type_create_pointer(struct Type* pointee);
static bool type_is_integer(struct Type* type);
static bool is_reserved_keyword(const char* name);
static bool validate_array_type(SemanticContext* semantic, struct Type* array_type, struct ASTNode* decl);
static bool validate_variable_initializer(SemanticContext* semantic, struct Type* var_type,
                                         struct ASTNode* initializer, struct ASTNode* decl);
static bool validate_array_initializer(SemanticContext* semantic, struct Type* array_type,
                                      struct ASTNode* initializer, struct ASTNode* decl);
static bool validate_struct_initializer(SemanticContext* semantic, struct Type* struct_type,
                                       struct ASTNode* initializer, struct ASTNode* decl);
static bool is_lvalue_expression(struct ASTNode* expr);
static int get_base_operator(int compound_op);
static struct Type* check_binary_operation_compatibility(SemanticContext* semantic, int operator,
                                                       struct Type* left, struct Type* right, struct ASTNode* expr);
static bool is_zero_literal(struct ASTNode* expr);

// ===============================================
// Hash Function for Symbol Table
// ===============================================

static unsigned int hash_string(const char* str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

// ===============================================
// Semantic Context Management
// ===============================================

SemanticContext* semantic_create(void) {
    SemanticContext* semantic = malloc(sizeof(SemanticContext));
    if (!semantic) return NULL;
    
    memset(semantic, 0, sizeof(SemanticContext));
    
    // Create global scope
    semantic->global_scope = symbol_table_create(NULL);
    semantic->current_scope = semantic->global_scope;
    
    if (!semantic->global_scope) {
        free(semantic);
        return NULL;
    }
    
    // Set default options
    semantic->strict_mode = true;
    semantic->allow_extensions = false;
    semantic->warn_unused = true;
    
    printf("Semantic: Created semantic analyzer\n");
    
    return semantic;
}

void semantic_destroy(SemanticContext* semantic) {
    if (!semantic) return;
    
    // Destroy symbol tables
    symbol_table_destroy(semantic->global_scope);
    
    free(semantic);
}

// ===============================================
// Symbol Table Functions
// ===============================================

SymbolTable* symbol_table_create(SymbolTable* parent) {
    SymbolTable* table = malloc(sizeof(SymbolTable));
    if (!table) return NULL;
    
    memset(table, 0, sizeof(SymbolTable));
    table->bucket_count = 256;
    table->buckets = calloc(table->bucket_count, sizeof(Symbol*));
    table->parent = parent;
    table->current_scope = parent ? parent->current_scope + 1 : 0;
    
    if (!table->buckets) {
        free(table);
        return NULL;
    }
    
    return table;
}

void symbol_table_destroy(SymbolTable* table) {
    if (!table) return;
    
    // Free all symbols
    for (size_t i = 0; i < table->bucket_count; i++) {
        Symbol* symbol = table->buckets[i];
        while (symbol) {
            Symbol* next = symbol->next;
            symbol_free(symbol);
            symbol = next;
        }
    }
    
    free(table->buckets);
    free(table);
}

static void symbol_free(Symbol* symbol) {
    if (!symbol) return;
    
    if (symbol->name) {
        free(symbol->name);
    }
    
    // Free type information
    if (symbol->type) {
        // 由于type是ASTNode*，应该在AST清理时统一释放
        // 这里只需要将指针置空即可
        symbol->type = NULL;
    }
    
    free(symbol);
}

void semantic_enter_scope(SemanticContext* semantic) {
    if (!semantic) return;
    
    SymbolTable* new_scope = symbol_table_create(semantic->current_scope);
    if (new_scope) {
        semantic->current_scope = new_scope;
        printf("Semantic: Entered scope level %d\n", new_scope->current_scope);
    }
}

void semantic_exit_scope(SemanticContext* semantic) {
    if (!semantic || !semantic->current_scope) return;
    
    SymbolTable* parent = semantic->current_scope->parent;
    if (parent) {
        printf("Semantic: Exiting scope level %d\n", semantic->current_scope->current_scope);
        symbol_table_destroy(semantic->current_scope);
        semantic->current_scope = parent;
    }
}

Symbol* semantic_declare_symbol(SemanticContext* semantic, const char* name, 
                               SymbolKind kind, struct Type* type) {
    if (!semantic || !name) return NULL;
    
    // Check if symbol already exists in current scope
    Symbol* existing = semantic_lookup_symbol_current_scope(semantic, name);
    if (existing) {
        semantic_error(semantic, NULL, "Symbol already declared in current scope");
        return NULL;
    }
    
    // Create new symbol
    Symbol* symbol = malloc(sizeof(Symbol));
    if (!symbol) return NULL;
    
    memset(symbol, 0, sizeof(Symbol));
    symbol->name = strdup(name);
    symbol->kind = kind;
    symbol->type = (struct ASTNode*)type; // 类型系统集成
    symbol->scope_level = semantic->current_scope->current_scope;
    
    // Add to symbol table
    unsigned int hash = hash_string(name) % semantic->current_scope->bucket_count;
    symbol->next = semantic->current_scope->buckets[hash];
    semantic->current_scope->buckets[hash] = symbol;
    semantic->current_scope->symbol_count++;
    
    printf("Semantic: Declared symbol '%s' in scope %d\n", name, symbol->scope_level);
    
    return symbol;
}

Symbol* semantic_lookup_symbol(SemanticContext* semantic, const char* name) {
    if (!semantic || !name) return NULL;
    
    SymbolTable* scope = semantic->current_scope;
    while (scope) {
        unsigned int hash = hash_string(name) % scope->bucket_count;
        Symbol* symbol = scope->buckets[hash];
        
        while (symbol) {
            if (strcmp(symbol->name, name) == 0) {
                return symbol;
            }
            symbol = symbol->next;
        }
        
        scope = scope->parent;
    }
    
    return NULL;
}

static Symbol* semantic_lookup_symbol_current_scope(SemanticContext* semantic, const char* name) {
    if (!semantic || !name || !semantic->current_scope) return NULL;
    
    unsigned int hash = hash_string(name) % semantic->current_scope->bucket_count;
    Symbol* symbol = semantic->current_scope->buckets[hash];
    
    while (symbol) {
        if (strcmp(symbol->name, name) == 0) {
            return symbol;
        }
        symbol = symbol->next;
    }
    
    return NULL;
}

// ===============================================
// Semantic Analysis Functions
// ===============================================

bool semantic_analyze(SemanticContext* semantic, struct ASTNode* ast) {
    if (!semantic || !ast) return false;
    
    printf("Semantic: Starting semantic analysis\n");
    
    bool result = semantic_analyze_translation_unit(semantic, ast);
    
    if (result) {
        printf("Semantic: Analysis completed successfully\n");
        printf("Semantic: %d errors, %d warnings\n", semantic->error_count, semantic->warning_count);
    } else {
        printf("Semantic: Analysis failed with %d errors\n", semantic->error_count);
    }
    
    return result;
}

bool semantic_analyze_translation_unit(SemanticContext* semantic, struct ASTNode* ast) {
    if (!semantic || !ast) return false;
    
    printf("Semantic: Analyzing translation unit\n");
    
    // 遍历AST的所有顶层声明
    for (int i = 0; i < ast->data.translation_unit.declaration_count; i++) {
        struct ASTNode* current = ast->data.translation_unit.declarations[i];
        if (!current) continue;

        if (current->type == ASTC_FUNC_DECL) {
            if (!semantic_analyze_function(semantic, current)) {
                return false;
            }
        } else if (current->type == ASTC_VAR_DECL) {
            if (!semantic_analyze_declaration(semantic, current)) {
                return false;
            }
        }
    }
    
    // 检查未使用的符号
    if (semantic->warn_unused) {
        check_unused_symbols(semantic, semantic->global_scope);
    }
    
    return !semantic->has_error;
}

bool semantic_analyze_function(SemanticContext* semantic, struct ASTNode* func) {
    if (!semantic || !func) return false;
    
    printf("Semantic: Analyzing function\n");
    
    semantic->in_function = true;
    semantic->has_default_case = false;

    // 创建函数级别的标签作用域
    semantic->label_scope = symbol_table_create(NULL);

    semantic_enter_scope(semantic);
    
    // 分析返回类型
    struct Type* return_type;
    if (func->data.func_decl.return_type) {
        return_type = analyze_type(semantic, func->data.func_decl.return_type);
        if (!return_type) {
            semantic_error(semantic, func, "Invalid return type");
            return false;
        }
    } else {
        // 如果没有显式返回类型，默认为int (C99标准)
        return_type = type_create(TYPE_INT);
    }

    // 分析参数
    for (int i = 0; i < func->data.func_decl.param_count; i++) {
        struct ASTNode* param = func->data.func_decl.params[i];
        if (param && !semantic_analyze_declaration(semantic, param)) {
            return false;
        }
    }

    // 分析函数体
    if (func->data.func_decl.has_body && func->data.func_decl.body) {
        semantic->current_function_type = return_type;
        if (!semantic_analyze_statement(semantic, func->data.func_decl.body)) {
            return false;
        }
        semantic->current_function_type = NULL;
    }
    
    semantic_exit_scope(semantic);
    semantic->in_function = false;

    // 清理标签作用域
    if (semantic->label_scope) {
        symbol_table_destroy(semantic->label_scope);
        semantic->label_scope = NULL;
    }
    
    return true;
}

bool semantic_analyze_statement(SemanticContext* semantic, struct ASTNode* stmt) {
    if (!semantic || !stmt) return false;
    
    switch (stmt->type) {
        case ASTC_COMPOUND_STMT:
            semantic_enter_scope(semantic);
            bool result = analyze_compound_statement(semantic, stmt);
            semantic_exit_scope(semantic);
            return result;
            
        case ASTC_IF_STMT:
            return analyze_if_statement(semantic, stmt);
            
        case ASTC_WHILE_STMT:
            semantic->in_loop = true;
            bool loop_result = analyze_while_statement(semantic, stmt);
            semantic->in_loop = false;
            return loop_result;
            
        case ASTC_FOR_STMT:
            semantic->in_loop = true;
            bool for_result = analyze_for_statement(semantic, stmt);
            semantic->in_loop = false;
            return for_result;

        case ASTC_BREAK_STMT:
            return analyze_break_statement(semantic, stmt);

        case ASTC_CONTINUE_STMT:
            return analyze_continue_statement(semantic, stmt);

        case ASTC_SWITCH_STMT:
            return analyze_switch_statement(semantic, stmt);

        case ASTC_RETURN_STMT:
            return analyze_return_statement(semantic, stmt);

        case ASTC_EXPR_STMT:
            return semantic_analyze_expression(semantic, stmt->data.expr_stmt.expr) != NULL;

        case ASTC_GOTO_STMT:
            return analyze_goto_statement(semantic, stmt);

        case ASTC_LABELED_STMT:
            return analyze_labeled_statement(semantic, stmt);

        case ASTC_CASE_STMT:
            return analyze_case_statement(semantic, stmt);

        case ASTC_DEFAULT_STMT:
            return analyze_default_statement(semantic, stmt);

        default:
            semantic_error(semantic, stmt, "Unsupported statement type");
            return false;
    }
}

struct Type* semantic_analyze_expression(SemanticContext* semantic, struct ASTNode* expr) {
    if (!semantic || !expr) return NULL;
    
    switch (expr->type) {
        case ASTC_BINARY_OP: {
            struct Type* left = semantic_analyze_expression(semantic, expr->data.binary_op.left);
            struct Type* right = semantic_analyze_expression(semantic, expr->data.binary_op.right);

            if (!left || !right) return NULL;

            // 类型检查和转换
            return check_binary_operation(semantic, expr->data.binary_op.op, left, right, expr);
        }

        case ASTC_UNARY_OP: {
            struct Type* operand = semantic_analyze_expression(semantic, expr->data.unary_op.operand);
            if (!operand) return NULL;

            return check_unary_operation(semantic, expr->data.unary_op.op, operand, expr);
        }

        case ASTC_EXPR_IDENTIFIER: {
            Symbol* symbol = semantic_lookup_symbol(semantic, expr->data.identifier.name);
            if (!symbol) {
                semantic_error(semantic, expr, "Undefined identifier");
                return NULL;
            }
            symbol->is_used = true;
            return (struct Type*)symbol->type;
        }
        
        case ASTC_EXPR_CONSTANT: {
            // 根据常量类型返回相应的类型
            switch (expr->data.constant.type) {
                case ASTC_EXPR_CONSTANT: // 整数常量
                    return type_create(TYPE_INT);
                default:
                    return type_create(TYPE_INT); // 默认为int
            }
        }
            
        case ASTC_CALL_EXPR: {
            printf("Semantic: Analyzing function call\n");

            // 分析被调用的表达式（通常是标识符）
            if (expr->data.call_expr.callee->type == ASTC_EXPR_IDENTIFIER) {
                const char* func_name = expr->data.call_expr.callee->data.identifier.name;
                printf("Semantic: Function call to '%s'\n", func_name);

                // 查找函数符号
                Symbol* func_symbol = semantic_lookup_symbol(semantic, func_name);
                if (!func_symbol) {
                    // 检查是否为内置函数
                    if (is_builtin_function(func_name)) {
                        printf("Semantic: Found builtin function '%s'\n", func_name);
                        return handle_builtin_function_call(semantic, expr, func_name);
                    }

                    semantic_error(semantic, expr, "Undeclared function");
                    return NULL;
                }

                if (func_symbol->kind != SYMBOL_FUNCTION) {
                    semantic_error(semantic, expr, "Called object is not a function");
                    return NULL;
                }

                // 检查函数调用参数
                if (!check_function_call(semantic, func_symbol, expr)) {
                    return NULL;
                }

                // 返回函数的返回类型
                struct Type* func_type = (struct Type*)func_symbol->type;
                return func_type->data.function.return_type;
            } else {
                // 函数指针调用等复杂情况
                struct Type* func_type = semantic_analyze_expression(semantic, expr->data.call_expr.callee);
                if (!func_type) return NULL;

                if (func_type->kind == TYPE_POINTER &&
                    func_type->data.pointer.pointee->kind == TYPE_FUNCTION) {
                    func_type = func_type->data.pointer.pointee;
                } else if (func_type->kind != TYPE_FUNCTION) {
                    semantic_error(semantic, expr, "Called object is not a function");
                    return NULL;
                }

                // 检查参数数量和类型
                if (!check_function_call_args(semantic, func_type, expr)) {
                    return NULL;
                }

                return func_type->data.function.return_type;
            }
        }

        case ASTC_EXPR_MEMBER_ACCESS: {
            // 结构体/联合体成员访问 (obj.member)
            struct Type* obj_type = semantic_analyze_expression(semantic, expr->data.member_access.object);
            if (!obj_type) return NULL;

            if (obj_type->kind != TYPE_STRUCT && obj_type->kind != TYPE_UNION) {
                semantic_error(semantic, expr, "Member access requires struct or union type");
                return NULL;
            }

            // 查找成员
            const char* member_name = expr->data.member_access.member;
            if (!member_name) {
                semantic_error(semantic, expr, "Invalid member name");
                return NULL;
            }

            if (obj_type->data.composite.is_complete && obj_type->data.composite.members) {
                for (size_t i = 0; i < obj_type->data.composite.member_count; i++) {
                    Symbol* member = obj_type->data.composite.members[i];
                    if (member && member->name && strcmp(member->name, member_name) == 0) {
                        return (struct Type*)member->type;
                    }
                }
            }

            semantic_error(semantic, expr, "No such member in struct/union");
            return NULL;
        }

        case ASTC_EXPR_PTR_MEMBER_ACCESS: {
            // 指针成员访问 (ptr->member)
            struct Type* ptr_type = semantic_analyze_expression(semantic, expr->data.member_access.object);
            if (!ptr_type) return NULL;

            if (ptr_type->kind != TYPE_POINTER) {
                semantic_error(semantic, expr, "Pointer access requires pointer type");
                return NULL;
            }

            struct Type* pointee_type = ptr_type->data.pointer.pointee;
            if (!pointee_type || (pointee_type->kind != TYPE_STRUCT && pointee_type->kind != TYPE_UNION)) {
                semantic_error(semantic, expr, "Pointer must point to struct or union");
                return NULL;
            }

            // 查找成员
            const char* member_name = expr->data.member_access.member;
            if (!member_name) {
                semantic_error(semantic, expr, "Invalid member name");
                return NULL;
            }

            if (pointee_type->data.composite.is_complete && pointee_type->data.composite.members) {
                for (size_t i = 0; i < pointee_type->data.composite.member_count; i++) {
                    Symbol* member = pointee_type->data.composite.members[i];
                    if (member && member->name && strcmp(member->name, member_name) == 0) {
                        return (struct Type*)member->type;
                    }
                }
            }

            semantic_error(semantic, expr, "No such member in struct/union");
            return NULL;
        }

        case ASTC_EXPR_ARRAY_SUBSCRIPT: {
            // 数组访问 (arr[index])
            struct Type* array_type = semantic_analyze_expression(semantic, expr->data.array_subscript.array);
            struct Type* index_type = semantic_analyze_expression(semantic, expr->data.array_subscript.index);

            if (!array_type || !index_type) return NULL;

            // 检查数组类型
            if (array_type->kind != TYPE_ARRAY && array_type->kind != TYPE_POINTER) {
                semantic_error(semantic, expr, "Array access requires array or pointer type");
                return NULL;
            }

            // 检查索引类型
            if (!type_is_integral(index_type)) {
                semantic_error(semantic, expr, "Array index must be integral type");
                return NULL;
            }

            // 返回元素类型
            if (array_type->kind == TYPE_ARRAY) {
                return array_type->data.array.element;
            } else {
                return array_type->data.pointer.pointee;
            }
        }

        case ASTC_EXPR_STRING_LITERAL: {
            printf("Semantic: Analyzing string literal\n");
            // 字符串字面量的类型是 char*
            return type_create_pointer(type_create(TYPE_CHAR));
        }

        case ASTC_EXPR_CAST_EXPR: {
            // 类型转换表达式
            struct Type* source_type = semantic_analyze_expression(semantic, expr->data.cast_expr.expression);
            if (!source_type) return NULL;

            struct Type* target_type = analyze_type(semantic, expr->data.cast_expr.target_type);
            if (!target_type) return NULL;

            // 检查是否可以进行类型转换
            if (!type_can_cast(source_type, target_type)) {
                semantic_error(semantic, expr, "Invalid type cast");
                return NULL;
            }

            printf("Semantic: Cast from type %d to type %d\n", source_type->kind, target_type->kind);
            return target_type;
        }

        case ASTC_EXPR_CONDITIONAL: {
            // 三元条件运算符 condition ? true_expr : false_expr
            struct Type* cond_type = semantic_analyze_expression(semantic, expr->data.conditional.condition);
            if (!cond_type || !type_is_scalar(cond_type)) {
                semantic_error(semantic, expr, "Conditional expression requires scalar condition");
                return NULL;
            }

            struct Type* true_type = semantic_analyze_expression(semantic, expr->data.conditional.true_expr);
            struct Type* false_type = semantic_analyze_expression(semantic, expr->data.conditional.false_expr);

            if (!true_type || !false_type) return NULL;

            // 确定结果类型：两个操作数的公共类型
            if (type_compatible(true_type, false_type)) {
                return true_type;
            }

            // 尝试算术转换
            if (type_is_arithmetic(true_type) && type_is_arithmetic(false_type)) {
                return type_arithmetic_conversion(true_type, false_type);
            }

            // 指针类型处理
            if (true_type->kind == TYPE_POINTER && false_type->kind == TYPE_POINTER) {
                if (type_compatible(true_type->data.pointer.pointee, false_type->data.pointer.pointee)) {
                    return true_type;
                }
            }

            semantic_error(semantic, expr, "Incompatible types in conditional expression");
            return NULL;
        }

        case ASTC_EXPR_COMMA: {
            // 逗号运算符：计算所有表达式，返回最后一个的类型
            struct Type* result_type = NULL;
            for (int i = 0; i < expr->data.comma_expr.expr_count; i++) {
                result_type = semantic_analyze_expression(semantic, expr->data.comma_expr.expressions[i]);
                if (!result_type) return NULL;
            }
            return result_type;
        }

        case ASTC_EXPR_POSTFIX_INC:
        case ASTC_EXPR_POSTFIX_DEC: {
            // 后缀递增递减运算符 var++ var--
            struct Type* operand_type = semantic_analyze_expression(semantic, expr->data.postfix.operand);
            if (!operand_type) return NULL;

            if (!type_is_scalar(operand_type)) {
                semantic_error(semantic, expr, "Postfix increment/decrement requires scalar type");
                return NULL;
            }

            if (!is_lvalue_expression(expr->data.postfix.operand)) {
                semantic_error(semantic, expr, "Postfix increment/decrement requires lvalue");
                return NULL;
            }

            return operand_type;
        }

        case ASTC_EXPR_SIZEOF: {
            // sizeof运算符
            if (expr->data.sizeof_expr.is_type) {
                // sizeof(type)
                struct Type* type = analyze_type(semantic, expr->data.sizeof_expr.type_node);
                if (!type) return NULL;
            } else {
                // sizeof(expression)
                struct Type* expr_type = semantic_analyze_expression(semantic, expr->data.sizeof_expr.expr);
                if (!expr_type) return NULL;
            }

            // sizeof返回size_t类型，这里简化为int
            return type_create(TYPE_INT);
        }

        default:
            semantic_error(semantic, expr, "Unsupported expression type");
            return NULL;
    }
}

// 辅助函数实现
static bool analyze_compound_statement(SemanticContext* semantic, struct ASTNode* stmt) {
    if (!semantic || !stmt) return false;

    for (int i = 0; i < stmt->data.compound_stmt.statement_count; i++) {
        if (!semantic_analyze_statement(semantic, stmt->data.compound_stmt.statements[i])) {
            return false;
        }
    }
    return true;
}

static bool analyze_if_statement(SemanticContext* semantic, struct ASTNode* stmt) {
    struct Type* cond_type = semantic_analyze_expression(semantic, stmt->data.if_stmt.condition);
    if (!cond_type || !type_is_scalar(cond_type)) {
        semantic_error(semantic, stmt->data.if_stmt.condition, "Condition must be a scalar type");
        return false;
    }

    return semantic_analyze_statement(semantic, stmt->data.if_stmt.then_branch) &&
           (!stmt->data.if_stmt.else_branch || semantic_analyze_statement(semantic, stmt->data.if_stmt.else_branch));
}

static bool analyze_while_statement(SemanticContext* semantic, struct ASTNode* stmt) {
    struct Type* cond_type = semantic_analyze_expression(semantic, stmt->data.while_stmt.condition);
    if (!cond_type || !type_is_scalar(cond_type)) {
        semantic_error(semantic, stmt->data.while_stmt.condition, "Condition must be a scalar type");
        return false;
    }

    return semantic_analyze_statement(semantic, stmt->data.while_stmt.body);
}

static bool analyze_for_statement(SemanticContext* semantic, struct ASTNode* stmt) {
    if (!semantic || !stmt) return false;

    printf("Semantic: Analyzing for statement\n");

    // 进入新的作用域（for循环的初始化变量作用域）
    semantic_enter_scope(semantic);

    // 分析初始化部分（可以是声明或表达式）
    if (stmt->data.for_stmt.init) {
        if (stmt->data.for_stmt.init->type == ASTC_VAR_DECL) {
            if (!semantic_analyze_declaration(semantic, stmt->data.for_stmt.init)) {
                semantic_exit_scope(semantic);
                return false;
            }
        } else {
            if (!semantic_analyze_expression(semantic, stmt->data.for_stmt.init)) {
                semantic_exit_scope(semantic);
                return false;
            }
        }
    }

    // 分析条件部分
    if (stmt->data.for_stmt.condition) {
        struct Type* cond_type = semantic_analyze_expression(semantic, stmt->data.for_stmt.condition);
        if (!cond_type || !type_is_scalar(cond_type)) {
            semantic_error(semantic, stmt->data.for_stmt.condition, "For loop condition must be a scalar type");
            semantic_exit_scope(semantic);
            return false;
        }
    }

    // 分析增量部分
    if (stmt->data.for_stmt.increment) {
        if (!semantic_analyze_expression(semantic, stmt->data.for_stmt.increment)) {
            semantic_exit_scope(semantic);
            return false;
        }
    }

    // 分析循环体
    bool body_result = semantic_analyze_statement(semantic, stmt->data.for_stmt.body);

    semantic_exit_scope(semantic);
    return body_result;
}

static bool analyze_break_statement(SemanticContext* semantic, struct ASTNode* stmt) {
    if (!semantic || !stmt) return false;

    printf("Semantic: Analyzing break statement\n");

    if (!semantic->in_loop && !semantic->in_switch) {
        semantic_error(semantic, stmt, "Break statement not within loop or switch");
        return false;
    }

    return true;
}

static bool analyze_continue_statement(SemanticContext* semantic, struct ASTNode* stmt) {
    if (!semantic || !stmt) return false;

    printf("Semantic: Analyzing continue statement\n");

    if (!semantic->in_loop) {
        semantic_error(semantic, stmt, "Continue statement not within loop");
        return false;
    }

    return true;
}

static bool analyze_switch_statement(SemanticContext* semantic, struct ASTNode* stmt) {
    if (!semantic || !stmt) return false;

    printf("Semantic: Analyzing switch statement\n");

    // 分析switch表达式
    struct Type* switch_type = semantic_analyze_expression(semantic, stmt->data.switch_stmt.expr);
    if (!switch_type || !type_is_integer(switch_type)) {
        semantic_error(semantic, stmt->data.switch_stmt.expr, "Switch expression must be an integer type");
        return false;
    }

    // 设置switch上下文
    bool prev_in_switch = semantic->in_switch;
    bool prev_has_default = semantic->has_default_case;
    struct Type* prev_switch_type = semantic->current_switch_type;

    semantic->in_switch = true;
    semantic->has_default_case = false;
    semantic->current_switch_type = switch_type;

    // 分析switch体
    bool result = semantic_analyze_statement(semantic, stmt->data.switch_stmt.body);

    // 恢复上下文
    semantic->in_switch = prev_in_switch;
    semantic->has_default_case = prev_has_default;
    semantic->current_switch_type = prev_switch_type;

    return result;
}

static bool analyze_goto_statement(SemanticContext* semantic, struct ASTNode* stmt) {
    if (!semantic || !stmt) return false;

    printf("Semantic: Analyzing goto statement\n");

    // 检查标签名是否有效
    if (!stmt->data.goto_stmt.label || strlen(stmt->data.goto_stmt.label) == 0) {
        semantic_error(semantic, stmt, "Goto statement missing label");
        return false;
    }

    // 在当前函数作用域中查找或声明标签
    // 简化实现：这里我们只检查标签名的有效性
    // 完整实现需要维护标签符号表
    printf("Semantic: Goto to label '%s'\n", stmt->data.goto_stmt.label);

    return true;
}

static bool analyze_labeled_statement(SemanticContext* semantic, struct ASTNode* stmt) {
    if (!semantic || !stmt) return false;

    printf("Semantic: Analyzing labeled statement\n");

    // 检查标签名是否有效
    if (!stmt->data.labeled_stmt.label || strlen(stmt->data.labeled_stmt.label) == 0) {
        semantic_error(semantic, stmt, "Labeled statement missing label");
        return false;
    }

    // 检查标签是否已经定义
    // 简化实现：这里我们只检查标签名的有效性
    // 完整实现需要维护标签符号表并检查重复定义
    printf("Semantic: Label '%s' defined\n", stmt->data.labeled_stmt.label);

    // 分析标签后的语句
    if (stmt->data.labeled_stmt.statement) {
        return semantic_analyze_statement(semantic, stmt->data.labeled_stmt.statement);
    }

    return true;
}

static bool analyze_case_statement(SemanticContext* semantic, struct ASTNode* stmt) {
    if (!semantic || !stmt) return false;

    printf("Semantic: Analyzing case statement\n");

    // 检查是否在switch语句内
    if (!semantic->in_switch) {
        semantic_error(semantic, stmt, "Case statement not within switch");
        return false;
    }

    // 分析case值表达式
    if (stmt->data.case_stmt.value) {
        struct Type* case_type = semantic_analyze_expression(semantic, stmt->data.case_stmt.value);
        if (!case_type) {
            semantic_error(semantic, stmt, "Invalid case value expression");
            return false;
        }

        // 检查case值是否为整数常量
        if (!type_is_integer(case_type)) {
            semantic_error(semantic, stmt, "Case value must be an integer constant");
            return false;
        }

        // 检查case值类型是否与switch表达式兼容
        if (semantic->current_switch_type &&
            !type_compatible(case_type, semantic->current_switch_type)) {
            semantic_warning(semantic, stmt, "Case value type differs from switch expression type");
        }
    }

    // 分析case后的语句
    if (stmt->data.case_stmt.statement) {
        return semantic_analyze_statement(semantic, stmt->data.case_stmt.statement);
    }

    return true;
}

static bool analyze_default_statement(SemanticContext* semantic, struct ASTNode* stmt) {
    if (!semantic || !stmt) return false;

    printf("Semantic: Analyzing default statement\n");

    // 检查是否在switch语句内
    if (!semantic->in_switch) {
        semantic_error(semantic, stmt, "Default statement not within switch");
        return false;
    }

    // 检查是否已有default语句
    if (semantic->has_default_case) {
        semantic_error(semantic, stmt, "Multiple default statements in switch");
        return false;
    }

    semantic->has_default_case = true;

    // 分析default后的语句
    if (stmt->data.default_stmt.statement) {
        return semantic_analyze_statement(semantic, stmt->data.default_stmt.statement);
    }

    return true;
}

static bool analyze_return_statement(SemanticContext* semantic, struct ASTNode* stmt) {
    if (!semantic->in_function) {
        semantic_error(semantic, stmt, "Return statement outside of function");
        return false;
    }

    if (!stmt->data.return_stmt.value) {
        if (semantic->current_function_type->kind != TYPE_VOID) {
            semantic_error(semantic, stmt, "Return value required");
            return false;
        }
        return true;
    }

    struct Type* expr_type = semantic_analyze_expression(semantic, stmt->data.return_stmt.value);
    if (!expr_type) return false;

    if (!type_compatible(semantic->current_function_type, expr_type)) {
        semantic_error(semantic, stmt, "Incompatible return type");
        return false;
    }

    return true;
}

static struct Type* check_binary_operation(SemanticContext* semantic, int operator,
                                         struct Type* left, struct Type* right,
                                         struct ASTNode* expr) {
    switch (operator) {
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            // 指针算术：pointer + integer 或 integer + pointer
            if (left->kind == TYPE_POINTER && type_is_integral(right)) {
                return left; // 返回指针类型
            }
            if (type_is_integral(left) && right->kind == TYPE_POINTER) {
                return right; // 返回指针类型
            }
            // 指针减法：pointer - pointer
            if (operator == TOKEN_MINUS && left->kind == TYPE_POINTER && right->kind == TYPE_POINTER) {
                if (!type_compatible(left->data.pointer.pointee, right->data.pointer.pointee)) {
                    semantic_error(semantic, expr, "Pointer subtraction requires compatible pointer types");
                    return NULL;
                }
                return type_create(TYPE_INT); // 指针差值为整数
            }
            // 普通算术运算
            if (type_is_arithmetic(left) && type_is_arithmetic(right)) {
                return type_arithmetic_conversion(left, right);
            }
            semantic_error(semantic, expr, "Invalid operands for addition/subtraction");
            return NULL;

        case TOKEN_MULTIPLY:
        case TOKEN_DIVIDE:
        case TOKEN_MODULO:
            // 乘除模运算只支持算术类型
            if (!type_is_arithmetic(left) || !type_is_arithmetic(right)) {
                semantic_error(semantic, expr, "Operands must be arithmetic types");
                return NULL;
            }

            // 模运算只支持整数类型
            if (operator == TOKEN_MODULO && (!type_is_integral(left) || !type_is_integral(right))) {
                semantic_error(semantic, expr, "Modulo operator requires integral operands");
                return NULL;
            }

            // 检查除零（简化检查：只检查字面量）
            if ((operator == TOKEN_DIVIDE || operator == TOKEN_MODULO) &&
                is_zero_literal(expr->data.binary_op.right)) {
                semantic_error(semantic, expr, "Division by zero");
                return NULL;
            }

            return type_arithmetic_conversion(left, right);

        case TOKEN_ASSIGN:
            // 赋值运算符：检查左值和类型转换
            if (!is_lvalue_expression(expr->data.binary_op.left)) {
                semantic_error(semantic, expr, "Assignment requires lvalue");
                return NULL;
            }
            if (!type_implicit_conversion(right, left)) {
                semantic_error(semantic, expr, "Cannot convert type in assignment");
                return NULL;
            }
            return left; // 赋值表达式的类型是左操作数的类型

        case TOKEN_PLUS_ASSIGN:
        case TOKEN_MINUS_ASSIGN:
        case TOKEN_MUL_ASSIGN:
        case TOKEN_DIV_ASSIGN:
        case TOKEN_MOD_ASSIGN:
        case TOKEN_AND_ASSIGN:
        case TOKEN_OR_ASSIGN:
        case TOKEN_XOR_ASSIGN:
        case TOKEN_LSHIFT_ASSIGN:
        case TOKEN_RSHIFT_ASSIGN:
            // 复合赋值运算符：op= 等价于 left = left op right
            if (!is_lvalue_expression(expr->data.binary_op.left)) {
                semantic_error(semantic, expr, "Compound assignment requires lvalue");
                return NULL;
            }

            // 检查对应的二元运算是否有效
            int base_op = get_base_operator(operator);
            struct Type* result_type = check_binary_operation_compatibility(semantic, base_op, left, right, expr);
            if (!result_type) return NULL;

            // 检查结果类型是否可以赋值给左操作数
            if (!type_implicit_conversion(result_type, left)) {
                semantic_error(semantic, expr, "Cannot convert result type in compound assignment");
                return NULL;
            }

            return left; // 复合赋值表达式的类型是左操作数的类型

        case TOKEN_EQUAL:
        case TOKEN_NOT_EQUAL:
            // 相等比较
            if (type_compatible(left, right) ||
                (left->kind == TYPE_POINTER && right->kind == TYPE_POINTER)) {
                return type_create(TYPE_INT);
            }
            semantic_error(semantic, expr, "Incompatible operand types for comparison");
            return NULL;

        case TOKEN_LESS:
        case TOKEN_LESS_EQUAL:
        case TOKEN_GREATER:
        case TOKEN_GREATER_EQUAL:
            // 关系比较
            if (type_compatible(left, right)) {
                return type_create(TYPE_INT);
            }
            // 指针比较
            if (left->kind == TYPE_POINTER && right->kind == TYPE_POINTER &&
                type_compatible(left->data.pointer.pointee, right->data.pointer.pointee)) {
                return type_create(TYPE_INT);
            }
            semantic_error(semantic, expr, "Incompatible operand types for comparison");
            return NULL;

        case TOKEN_LOGICAL_AND:
        case TOKEN_LOGICAL_OR:
            // 逻辑运算符
            if (!type_is_scalar(left) || !type_is_scalar(right)) {
                semantic_error(semantic, expr, "Operands must be scalar types");
                return NULL;
            }
            return type_create(TYPE_INT);

        case TOKEN_BITWISE_AND:
        case TOKEN_BITWISE_OR:
        case TOKEN_BITWISE_XOR:
        case TOKEN_LEFT_SHIFT:
        case TOKEN_RIGHT_SHIFT:
            // 位运算符
            if (!type_is_integral(left) || !type_is_integral(right)) {
                semantic_error(semantic, expr, "Operands must be integral types");
                return NULL;
            }
            return type_arithmetic_conversion(left, right);

        default:
            semantic_error(semantic, expr, "Unsupported binary operator");
            return NULL;
    }
}

static struct Type* check_unary_operation(SemanticContext* semantic, int operator,
                                        struct Type* operand, struct ASTNode* expr) {
    switch (operator) {
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            if (!type_is_arithmetic(operand)) {
                semantic_error(semantic, expr, "Operand must be arithmetic type");
                return NULL;
            }
            return operand;
            
        case TOKEN_LOGICAL_NOT:
            if (!type_is_scalar(operand)) {
                semantic_error(semantic, expr, "Operand must be scalar type");
                return NULL;
            }
            return type_create(TYPE_INT);
            
        case TOKEN_BITWISE_NOT:
            if (!type_is_integral(operand)) {
                semantic_error(semantic, expr, "Operand must be integral type");
                return NULL;
            }
            return operand;

        case TOKEN_MULTIPLY: // 指针解引用 *ptr
            if (operand->kind != TYPE_POINTER) {
                semantic_error(semantic, expr, "Dereference requires pointer type");
                return NULL;
            }
            return operand->data.pointer.pointee;

        case TOKEN_BITWISE_AND: // 取地址 &var
            // 创建指向操作数类型的指针类型
            struct Type* pointer_type = type_create(TYPE_POINTER);
            if (!pointer_type) return NULL;
            pointer_type->data.pointer.pointee = operand;
            return pointer_type;

        case TOKEN_INCREMENT: // 前缀递增 ++var
        case TOKEN_DECREMENT: // 前缀递减 --var
            if (!type_is_scalar(operand)) {
                semantic_error(semantic, expr, "Increment/decrement requires scalar type");
                return NULL;
            }
            // 检查是否为左值
            if (!is_lvalue_expression(expr->data.unary_op.operand)) {
                semantic_error(semantic, expr, "Increment/decrement requires lvalue");
                return NULL;
            }
            return operand;

        case TOKEN_SIZEOF: // sizeof运算符
            // sizeof返回size_t类型，这里简化为int
            return type_create(TYPE_INT);

        default:
            semantic_error(semantic, expr, "Unsupported unary operator");
            return NULL;
    }
}

static bool check_function_call(SemanticContext* semantic, Symbol* func,
                              struct ASTNode* call) {
    struct Type* func_type = (struct Type*)func->type;
    int arg_count = call->data.call_expr.arg_count;
    int param_count = 0;
    
    // 检查每个参数
    for (int i = 0; i < arg_count && i < func_type->data.function.parameter_count; i++) {
        struct Type* arg_type = semantic_analyze_expression(semantic, call->data.call_expr.args[i]);
        if (!arg_type) return false;

        struct Type* param_type = func_type->data.function.parameters[i];
        if (!type_compatible(param_type, arg_type)) {
            semantic_error(semantic, call->data.call_expr.args[i],
                         "Argument type incompatible with parameter type");
            return false;
        }
    }
    
    // 检查参数数量
    if (arg_count > func_type->data.function.parameter_count && !func_type->data.function.is_variadic) {
        semantic_error(semantic, call, "Too many arguments");
        return false;
    }

    if (arg_count < func_type->data.function.parameter_count) {
        semantic_error(semantic, call, "Too few arguments");
        return false;
    }
    
    return true;
}

static void check_unused_symbols(SemanticContext* semantic, SymbolTable* scope) {
    for (size_t i = 0; i < scope->bucket_count; i++) {
        Symbol* symbol = scope->buckets[i];
        while (symbol) {
            if (!symbol->is_used && symbol->kind != SYMBOL_TYPE) {
                char message[256];
                snprintf(message, sizeof(message),
                        "Unused %s '%s'",
                        symbol->kind == SYMBOL_FUNCTION ? "function" : "variable",
                        symbol->name);
                semantic_warning(semantic, NULL, message);
            }
            symbol = symbol->next;
        }
    }
}

// ===============================================
// Type System Functions
// ===============================================

struct Type* type_create(TypeKind kind) {
    struct Type* type = malloc(sizeof(struct Type));
    if (!type) return NULL;
    
    memset(type, 0, sizeof(struct Type));
    type->kind = kind;
    
    return type;
}

void type_destroy(struct Type* type) {
    if (!type) return;
    
    // Free type-specific data
    switch (type->kind) {
        case TYPE_POINTER:
            if (type->data.pointer.base_type) {
                type_destroy(type->data.pointer.base_type);
            }
            break;
        case TYPE_ARRAY:
            if (type->data.array.element_type) {
                type_destroy(type->data.array.element_type);
            }
            break;
        case TYPE_FUNCTION:
            if (type->data.function.return_type) {
                type_destroy(type->data.function.return_type);
            }
            for (int i = 0; i < type->data.function.param_count; i++) {
                if (type->data.function.param_types[i]) {
                    type_destroy(type->data.function.param_types[i]);
                }
            }
            break;
        default:
            // Basic types don't need special cleanup
            break;
    }
    
    free(type);
}

bool type_compatible(struct Type* type1, struct Type* type2) {
    if (!type1 || !type2) return false;

    // 基本类型兼容性检查
    if (type1->kind != type2->kind) return false;

    switch (type1->kind) {
        case TYPE_VOID:
        case TYPE_CHAR:
        case TYPE_SHORT:
        case TYPE_INT:
        case TYPE_LONG:
        case TYPE_LONG_LONG:
        case TYPE_FLOAT:
        case TYPE_DOUBLE:
        case TYPE_LONG_DOUBLE:
        case TYPE_BOOL:
            // 基本类型：相同类型即兼容
            return true;

        case TYPE_POINTER:
            // 指针类型：指向的类型必须兼容
            return type_compatible(type1->data.pointer.pointee, type2->data.pointer.pointee);

        case TYPE_ARRAY:
            // 数组类型：元素类型必须兼容，大小可以不同
            return type_compatible(type1->data.array.element, type2->data.array.element);

        case TYPE_STRUCT:
        case TYPE_UNION:
            // 结构体/联合体：标签名必须相同
            if (!type1->data.composite.tag || !type2->data.composite.tag) {
                return false; // 匿名结构体/联合体不兼容
            }
            return strcmp(type1->data.composite.tag, type2->data.composite.tag) == 0;

        case TYPE_FUNCTION:
            // 函数类型：返回类型和参数类型都必须兼容
            if (!type_compatible(type1->data.function.return_type, type2->data.function.return_type)) {
                return false;
            }
            if (type1->data.function.parameter_count != type2->data.function.parameter_count) {
                return false;
            }
            for (size_t i = 0; i < type1->data.function.parameter_count; i++) {
                if (!type_compatible(type1->data.function.parameters[i], type2->data.function.parameters[i])) {
                    return false;
                }
            }
            return true;

        default:
            return false;
    }
}

size_t type_get_size(struct Type* type) {
    if (!type) return 0;

    switch (type->kind) {
        case TYPE_VOID: return 0;
        case TYPE_CHAR: return 1;
        case TYPE_SHORT: return 2;
        case TYPE_INT: return 4;
        case TYPE_LONG: return 8;
        case TYPE_LONG_LONG: return 8;
        case TYPE_FLOAT: return 4;
        case TYPE_DOUBLE: return 8;
        case TYPE_LONG_DOUBLE: return 16;
        case TYPE_BOOL: return 1;

        case TYPE_POINTER:
            // 指针大小通常是8字节（64位系统）
            return 8;

        case TYPE_ARRAY:
            // 数组大小 = 元素大小 * 元素数量
            if (type->data.array.element && type->data.array.size > 0) {
                return type_get_size(type->data.array.element) * type->data.array.size;
            }
            return 0; // 不完整数组或VLA

        case TYPE_STRUCT: {
            // 结构体大小 = 所有成员大小之和（考虑对齐）
            size_t total_size = 0;
            size_t max_align = 1;

            if (type->data.composite.is_complete && type->data.composite.members) {
                for (size_t i = 0; i < type->data.composite.member_count; i++) {
                    Symbol* member = type->data.composite.members[i];
                    if (member && member->type) {
                        struct Type* member_type = (struct Type*)member->type;
                        size_t member_size = type_get_size(member_type);
                        size_t member_align = type_get_alignment(member_type);

                        // 对齐当前偏移
                        total_size = (total_size + member_align - 1) & ~(member_align - 1);
                        total_size += member_size;

                        if (member_align > max_align) {
                            max_align = member_align;
                        }
                    }
                }

                // 结构体总大小需要对齐到最大成员的对齐要求
                total_size = (total_size + max_align - 1) & ~(max_align - 1);
            }

            return total_size;
        }

        case TYPE_UNION: {
            // 联合体大小 = 最大成员的大小
            size_t max_size = 0;

            if (type->data.composite.is_complete && type->data.composite.members) {
                for (size_t i = 0; i < type->data.composite.member_count; i++) {
                    Symbol* member = type->data.composite.members[i];
                    if (member && member->type) {
                        struct Type* member_type = (struct Type*)member->type;
                        size_t member_size = type_get_size(member_type);
                        if (member_size > max_size) {
                            max_size = member_size;
                        }
                    }
                }
            }

            return max_size;
        }

        case TYPE_FUNCTION:
            // 函数类型没有大小
            return 0;

        default:
            return 0;
    }
}

bool type_is_arithmetic(struct Type* type) {
    if (!type) return false;

    return (type->kind >= TYPE_CHAR && type->kind <= TYPE_LONG_LONG) ||
           (type->kind >= TYPE_FLOAT && type->kind <= TYPE_LONG_DOUBLE) ||
           (type->kind == TYPE_BOOL);
}

bool type_is_integral(struct Type* type) {
    if (!type) return false;

    return type->kind >= TYPE_CHAR && type->kind <= TYPE_LONG_LONG;
}

static size_t type_get_alignment(struct Type* type) {
    if (!type) return 1;

    switch (type->kind) {
        case TYPE_CHAR: return 1;
        case TYPE_SHORT: return 2;
        case TYPE_INT: return 4;
        case TYPE_LONG: return 8;
        case TYPE_LONG_LONG: return 8;
        case TYPE_FLOAT: return 4;
        case TYPE_DOUBLE: return 8;
        case TYPE_LONG_DOUBLE: return 16;
        case TYPE_BOOL: return 1;
        case TYPE_POINTER: return 8;

        case TYPE_ARRAY:
            // 数组的对齐要求与元素类型相同
            return type->data.array.element ? type_get_alignment(type->data.array.element) : 1;

        case TYPE_STRUCT:
        case TYPE_UNION: {
            // 结构体/联合体的对齐要求是最大成员的对齐要求
            size_t max_align = 1;

            if (type->data.composite.is_complete && type->data.composite.members) {
                for (size_t i = 0; i < type->data.composite.member_count; i++) {
                    Symbol* member = type->data.composite.members[i];
                    if (member && member->type) {
                        struct Type* member_type = (struct Type*)member->type;
                        size_t member_align = type_get_alignment(member_type);
                        if (member_align > max_align) {
                            max_align = member_align;
                        }
                    }
                }
            }

            return max_align;
        }

        default:
            return 1;
    }
}

// ===============================================
// Error Handling
// ===============================================

void semantic_error(SemanticContext* semantic, struct ASTNode* node, const char* message) {
    if (!semantic) return;
    
    semantic->has_error = true;
    semantic->error_count++;
    
    // 获取位置信息
    int line = node && node->location.line > 0 ? node->location.line : 1;
    int column = node && node->location.column > 0 ? node->location.column : 1;
    
    snprintf(semantic->error_message, sizeof(semantic->error_message),
             "Semantic error at line %d, column %d: %s", line, column, message);
    
    printf("Semantic Error: %s\n", semantic->error_message);
}

void semantic_warning(SemanticContext* semantic, struct ASTNode* node, const char* message) {
    if (!semantic) return;
    
    semantic->warning_count++;
    
    // 获取位置信息
    int line = node && node->location.line > 0 ? node->location.line : 1;
    int column = node && node->location.column > 0 ? node->location.column : 1;
    
    printf("Semantic Warning at line %d, column %d: %s\n", line, column, message);
}

bool semantic_has_error(SemanticContext* semantic) {
    return semantic && semantic->has_error;
}

const char* semantic_get_error(SemanticContext* semantic) {
    return semantic ? semantic->error_message : "Invalid semantic context";
}

void semantic_print_stats(SemanticContext* semantic) {
    if (!semantic) return;

    printf("Semantic Analysis Statistics:\n");
    printf("  Symbols in global scope: %zu\n", semantic->global_scope->symbol_count);
    printf("  Current scope level: %d\n", semantic->current_scope->current_scope);
    printf("  Errors: %d\n", semantic->error_count);
    printf("  Warnings: %d\n", semantic->warning_count);
}

// ===============================================
// Function Call Argument Checking
// ===============================================

static bool check_function_call_args(SemanticContext* semantic, struct Type* func_type, struct ASTNode* call) {
    if (!semantic || !func_type || !call) return false;

    // 检查参数数量
    int expected_params = func_type->data.function.parameter_count;
    int actual_args = call->data.call_expr.arg_count;

    if (!func_type->data.function.is_variadic && actual_args != expected_params) {
        semantic_error(semantic, call, "Function call argument count mismatch");
        return false;
    }

    if (actual_args < expected_params) {
        semantic_error(semantic, call, "Too few arguments to function call");
        return false;
    }

    // 检查每个参数的类型
    for (int i = 0; i < expected_params && i < actual_args; i++) {
        struct Type* arg_type = semantic_analyze_expression(semantic, call->data.call_expr.args[i]);
        if (!arg_type) return false;

        struct Type* param_type = func_type->data.function.parameters[i];
        if (!type_compatible(arg_type, param_type)) {
            semantic_error(semantic, call, "Function argument type mismatch");
            return false;
        }
    }

    return true;
}

// ===============================================
// Missing Function Implementations
// ===============================================

static bool semantic_analyze_declaration(SemanticContext* semantic, struct ASTNode* decl) {
    if (!semantic || !decl) return false;

    switch (decl->type) {
        case ASTC_FUNC_DECL: {
            // 函数声明分析
            printf("Semantic: Analyzing function declaration\n");

            // 分析返回类型
            struct Type* return_type = NULL;
            if (decl->data.func_decl.return_type) {
                return_type = analyze_type(semantic, decl->data.func_decl.return_type);
            } else {
                return_type = type_create(TYPE_INT); // 默认返回int
            }

            // 创建函数符号
            if (decl->data.func_decl.name) {
                Symbol* func_symbol = semantic_declare_symbol(semantic,
                    decl->data.func_decl.name, SYMBOL_FUNCTION, return_type);
                if (func_symbol) {
                    // 注意：这里需要根据实际的Symbol结构来设置参数
                    // func_symbol->data.function.parameters = decl->data.func_decl.params;
                    func_symbol->data.function.body = decl->data.func_decl.body;
                }
            }

            return true;
        }

        case ASTC_VAR_DECL: {
            // 变量声明分析
            printf("Semantic: Analyzing variable declaration '%s'\n",
                   decl->data.var_decl.name ? decl->data.var_decl.name : "<unnamed>");

            // 检查变量名是否有效
            if (!decl->data.var_decl.name || strlen(decl->data.var_decl.name) == 0) {
                semantic_error(semantic, decl, "Variable declaration missing name");
                return false;
            }

            // 检查变量名是否为关键字
            if (is_reserved_keyword(decl->data.var_decl.name)) {
                semantic_error(semantic, decl, "Cannot use reserved keyword as variable name");
                return false;
            }

            // 分析变量类型
            struct Type* var_type = NULL;
            if (decl->data.var_decl.type) {
                var_type = analyze_type(semantic, decl->data.var_decl.type);
                if (!var_type) {
                    semantic_error(semantic, decl, "Invalid variable type");
                    return false;
                }
            } else {
                var_type = type_create(TYPE_INT); // 默认为int
            }

            // 检查类型是否完整（不能声明void类型的变量）
            if (var_type->kind == TYPE_VOID) {
                semantic_error(semantic, decl, "Cannot declare variable of void type");
                return false;
            }

            // 检查数组类型的有效性
            if (var_type->kind == TYPE_ARRAY) {
                if (!validate_array_type(semantic, var_type, decl)) {
                    return false;
                }
            }

            // 创建变量符号
            Symbol* var_symbol = semantic_declare_symbol(semantic,
                decl->data.var_decl.name, SYMBOL_VARIABLE, var_type);
            if (!var_symbol) {
                return false; // 错误已在semantic_declare_symbol中报告
            }

            var_symbol->data.variable.initializer = decl->data.var_decl.initializer;
            var_symbol->is_initialized = (decl->data.var_decl.initializer != NULL);

            // 如果有初始化器，检查类型兼容性
            if (decl->data.var_decl.initializer) {
                if (!validate_variable_initializer(semantic, var_type,
                                                 decl->data.var_decl.initializer, decl)) {
                    return false;
                }
            } else {
                // 检查是否需要初始化（const变量必须初始化）
                if (var_type->is_const) {
                    semantic_error(semantic, decl, "const variable must be initialized");
                    return false;
                }

                // 在函数内部的局部变量如果未初始化，给出警告
                if (semantic->in_function && semantic->warn_unused) {
                    semantic_warning(semantic, decl, "Variable declared but not initialized");
                }
            }

            printf("Semantic: Successfully declared variable '%s' of type %d\n",
                   decl->data.var_decl.name, var_type->kind);
            return true;
        }

        case ASTC_STRUCT_DECL: {
            // 结构体声明分析
            printf("Semantic: Analyzing struct declaration\n");

            struct Type* struct_type = analyze_type(semantic, decl);

            // 如果有标签名，将结构体类型添加到符号表
            if (decl->data.struct_decl.name) {
                Symbol* struct_symbol = semantic_declare_symbol(semantic,
                    decl->data.struct_decl.name, SYMBOL_TYPE, struct_type);
                if (struct_symbol) {
                    printf("Semantic: Declared struct type '%s'\n", decl->data.struct_decl.name);
                }
            }

            return true;
        }

        case ASTC_UNION_DECL: {
            // 联合体声明分析
            printf("Semantic: Analyzing union declaration\n");

            struct Type* union_type = analyze_type(semantic, decl);

            // 如果有标签名，将联合体类型添加到符号表
            if (decl->data.union_decl.name) {
                Symbol* union_symbol = semantic_declare_symbol(semantic,
                    decl->data.union_decl.name, SYMBOL_TYPE, union_type);
                if (union_symbol) {
                    printf("Semantic: Declared union type '%s'\n", decl->data.union_decl.name);
                }
            }

            return true;
        }

        default:
            printf("Semantic: Unknown declaration type %d\n", decl->type);
            return true;
    }
}

static struct Type* analyze_type(SemanticContext* semantic, struct ASTNode* type_node) {
    if (!semantic || !type_node) return NULL;

    switch (type_node->type) {
        case ASTC_TYPE_SPECIFIER: {
            // 基本类型
            switch (type_node->data.type_specifier.type) {
                case TOKEN_VOID: return type_create(TYPE_VOID);
                case TOKEN_CHAR: return type_create(TYPE_CHAR);
                case TOKEN_SHORT: return type_create(TYPE_SHORT);
                case TOKEN_INT: return type_create(TYPE_INT);
                case TOKEN_LONG: return type_create(TYPE_LONG);
                case TOKEN_FLOAT: return type_create(TYPE_FLOAT);
                case TOKEN_DOUBLE: return type_create(TYPE_DOUBLE);
                case TOKEN_BOOL: return type_create(TYPE_BOOL);
                default: return type_create(TYPE_INT);
            }
        }

        case ASTC_STRUCT_DECL: {
            // 结构体类型
            struct Type* struct_type = type_create(TYPE_STRUCT);
            if (!struct_type) return NULL;

            struct_type->data.composite.tag = type_node->data.struct_decl.name ?
                strdup(type_node->data.struct_decl.name) : NULL;
            struct_type->data.composite.member_count = type_node->data.struct_decl.member_count;
            struct_type->data.composite.is_complete = (type_node->data.struct_decl.members != NULL);

            // 分析成员类型
            if (struct_type->data.composite.is_complete) {
                struct_type->data.composite.members = malloc(
                    sizeof(Symbol*) * struct_type->data.composite.member_count);

                for (size_t i = 0; i < struct_type->data.composite.member_count; i++) {
                    struct ASTNode* member = type_node->data.struct_decl.members[i];
                    if (member && member->type == ASTC_VAR_DECL) {
                        Symbol* member_symbol = malloc(sizeof(Symbol));
                        if (member_symbol) {
                            memset(member_symbol, 0, sizeof(Symbol));
                            member_symbol->name = member->data.var_decl.name ?
                                strdup(member->data.var_decl.name) : NULL;
                            member_symbol->kind = SYMBOL_VARIABLE;
                            member_symbol->type = (struct ASTNode*)analyze_type(semantic, member->data.var_decl.type);
                            struct_type->data.composite.members[i] = member_symbol;
                        }
                    }
                }
            }

            printf("Semantic: Analyzed struct type '%s' with %zu members\n",
                   struct_type->data.composite.tag ? struct_type->data.composite.tag : "(anonymous)",
                   struct_type->data.composite.member_count);

            return struct_type;
        }

        case ASTC_UNION_DECL: {
            // 联合体类型
            struct Type* union_type = type_create(TYPE_UNION);
            if (!union_type) return NULL;

            union_type->data.composite.tag = type_node->data.union_decl.name ?
                strdup(type_node->data.union_decl.name) : NULL;
            union_type->data.composite.member_count = type_node->data.union_decl.member_count;
            union_type->data.composite.is_complete = (type_node->data.union_decl.members != NULL);

            // 分析成员类型
            if (union_type->data.composite.is_complete) {
                union_type->data.composite.members = malloc(
                    sizeof(Symbol*) * union_type->data.composite.member_count);

                for (size_t i = 0; i < union_type->data.composite.member_count; i++) {
                    struct ASTNode* member = type_node->data.union_decl.members[i];
                    if (member && member->type == ASTC_VAR_DECL) {
                        Symbol* member_symbol = malloc(sizeof(Symbol));
                        if (member_symbol) {
                            memset(member_symbol, 0, sizeof(Symbol));
                            member_symbol->name = member->data.var_decl.name ?
                                strdup(member->data.var_decl.name) : NULL;
                            member_symbol->kind = SYMBOL_VARIABLE;
                            member_symbol->type = (struct ASTNode*)analyze_type(semantic, member->data.var_decl.type);
                            union_type->data.composite.members[i] = member_symbol;
                        }
                    }
                }
            }

            printf("Semantic: Analyzed union type '%s' with %zu members\n",
                   union_type->data.composite.tag ? union_type->data.composite.tag : "(anonymous)",
                   union_type->data.composite.member_count);

            return union_type;
        }

        case ASTC_POINTER_TYPE: {
            // 指针类型
            struct Type* pointer_type = type_create(TYPE_POINTER);
            if (!pointer_type) return NULL;

            // 分析基础类型
            if (type_node->data.pointer_type.base_type) {
                pointer_type->data.pointer.pointee = analyze_type(semantic, type_node->data.pointer_type.base_type);
            } else {
                // 如果没有基础类型，默认为void*
                pointer_type->data.pointer.pointee = type_create(TYPE_VOID);
            }

            printf("Semantic: Analyzed pointer type with %d indirection levels\n",
                   type_node->data.pointer_type.pointer_level);

            return pointer_type;
        }

        case ASTC_ARRAY_TYPE: {
            // 数组类型
            struct Type* array_type = type_create(TYPE_ARRAY);
            if (!array_type) return NULL;

            // 分析元素类型
            if (type_node->data.array_type.element_type) {
                array_type->data.array.element = analyze_type(semantic, type_node->data.array_type.element_type);
            } else {
                // 默认为int数组
                array_type->data.array.element = type_create(TYPE_INT);
            }

            // 分析数组大小
            array_type->data.array.size = 0; // 默认大小
            array_type->data.array.is_vla = false;

            if (type_node->data.array_type.size_expr) {
                if (type_node->data.array_type.size_expr->type == ASTC_EXPR_CONSTANT) {
                    array_type->data.array.size = type_node->data.array_type.size_expr->data.constant.int_val;
                } else {
                    // 变长数组
                    array_type->data.array.is_vla = true;
                }
            }

            printf("Semantic: Analyzed array type with size %zu (VLA: %s)\n",
                   array_type->data.array.size,
                   array_type->data.array.is_vla ? "yes" : "no");

            return array_type;
        }

        default:
            printf("Semantic: Unknown type node type %d, defaulting to int\n", type_node->type);
            return type_create(TYPE_INT);
    }
}

static bool type_is_scalar(struct Type* type) {
    if (!type) return false;

    // 标量类型包括算术类型和指针类型
    return type_is_arithmetic(type) || type->kind == TYPE_POINTER;
}

static struct Type* type_arithmetic_conversion(struct Type* left, struct Type* right) {
    if (!left || !right) return NULL;

    // C99标准算术转换规则
    // 1. 如果任一操作数是long double，结果为long double
    if (left->kind == TYPE_LONG_DOUBLE || right->kind == TYPE_LONG_DOUBLE) {
        return type_create(TYPE_LONG_DOUBLE);
    }

    // 2. 如果任一操作数是double，结果为double
    if (left->kind == TYPE_DOUBLE || right->kind == TYPE_DOUBLE) {
        return type_create(TYPE_DOUBLE);
    }

    // 3. 如果任一操作数是float，结果为float
    if (left->kind == TYPE_FLOAT || right->kind == TYPE_FLOAT) {
        return type_create(TYPE_FLOAT);
    }

    // 4. 整数提升：char, short -> int
    TypeKind left_promoted = left->kind;
    TypeKind right_promoted = right->kind;

    if (left_promoted == TYPE_CHAR || left_promoted == TYPE_SHORT) {
        left_promoted = TYPE_INT;
    }
    if (right_promoted == TYPE_CHAR || right_promoted == TYPE_SHORT) {
        right_promoted = TYPE_INT;
    }

    // 5. 如果任一操作数是long long，结果为long long
    if (left_promoted == TYPE_LONG_LONG || right_promoted == TYPE_LONG_LONG) {
        return type_create(TYPE_LONG_LONG);
    }

    // 6. 如果任一操作数是long，结果为long
    if (left_promoted == TYPE_LONG || right_promoted == TYPE_LONG) {
        return type_create(TYPE_LONG);
    }

    // 7. 否则结果为int
    return type_create(TYPE_INT);
}

static struct Type* type_implicit_conversion(struct Type* from, struct Type* to) {
    if (!from || !to) return NULL;

    // 相同类型，无需转换
    if (type_compatible(from, to)) {
        return to;
    }

    // 算术类型之间的隐式转换
    if (type_is_arithmetic(from) && type_is_arithmetic(to)) {
        return to; // 允许算术类型之间的隐式转换
    }

    // 指针类型转换
    if (from->kind == TYPE_POINTER && to->kind == TYPE_POINTER) {
        // void* 可以转换为任何指针类型
        if (from->data.pointer.pointee->kind == TYPE_VOID ||
            to->data.pointer.pointee->kind == TYPE_VOID) {
            return to;
        }

        // 兼容的指针类型
        if (type_compatible(from->data.pointer.pointee, to->data.pointer.pointee)) {
            return to;
        }
    }

    // 数组到指针的隐式转换
    if (from->kind == TYPE_ARRAY && to->kind == TYPE_POINTER) {
        if (type_compatible(from->data.array.element, to->data.pointer.pointee)) {
            return to;
        }
    }

    // 函数到函数指针的隐式转换
    if (from->kind == TYPE_FUNCTION && to->kind == TYPE_POINTER &&
        to->data.pointer.pointee->kind == TYPE_FUNCTION) {
        if (type_compatible(from, to->data.pointer.pointee)) {
            return to;
        }
    }

    // 空指针常量到指针的转换
    if (from->kind == TYPE_INT && to->kind == TYPE_POINTER) {
        // 这里应该检查是否为0常量，简化处理
        return to;
    }

    return NULL; // 无法进行隐式转换
}

static bool type_can_cast(struct Type* from, struct Type* to) {
    if (!from || !to) return false;

    // 隐式转换总是允许的
    if (type_implicit_conversion(from, to)) {
        return true;
    }

    // 显式转换规则

    // 算术类型之间可以显式转换
    if (type_is_arithmetic(from) && type_is_arithmetic(to)) {
        return true;
    }

    // 指针类型之间可以显式转换
    if (from->kind == TYPE_POINTER && to->kind == TYPE_POINTER) {
        return true;
    }

    // 整数和指针之间可以显式转换
    if ((type_is_integral(from) && to->kind == TYPE_POINTER) ||
        (from->kind == TYPE_POINTER && type_is_integral(to))) {
        return true;
    }

    return false;
}

static bool is_builtin_function(const char* name) {
    if (!name) return false;

    // 常见的C标准库函数
    const char* builtin_functions[] = {
        // stdio.h
        "printf", "scanf", "fprintf", "fscanf", "sprintf", "sscanf",
        "fopen", "fclose", "fread", "fwrite", "fseek", "ftell",
        "fgetc", "fputc", "fgets", "fputs", "getchar", "putchar",
        "puts", "getc", "putc", "ungetc", "fflush", "feof", "ferror",

        // stdlib.h
        "malloc", "calloc", "realloc", "free", "exit", "abort",
        "atoi", "atof", "atol", "strtol", "strtod", "rand", "srand",

        // string.h
        "strlen", "strcpy", "strncpy", "strcat", "strncat", "strcmp",
        "strncmp", "strchr", "strrchr", "strstr", "strtok", "memcpy",
        "memmove", "memset", "memcmp", "memchr",

        // math.h
        "sin", "cos", "tan", "asin", "acos", "atan", "atan2",
        "sinh", "cosh", "tanh", "exp", "log", "log10", "pow",
        "sqrt", "ceil", "floor", "fabs", "fmod",

        NULL
    };

    for (int i = 0; builtin_functions[i]; i++) {
        if (strcmp(name, builtin_functions[i]) == 0) {
            return true;
        }
    }

    return false;
}

static struct Type* handle_builtin_function_call(SemanticContext* semantic, struct ASTNode* call, const char* func_name) {
    if (!semantic || !call || !func_name) return NULL;

    printf("Semantic: Handling builtin function '%s' with %d arguments\n",
           func_name, call->data.call_expr.arg_count);

    // 标记为libc调用
    call->data.call_expr.is_libc_call = true;
    call->data.call_expr.libc_func_id = get_libc_function_id(func_name);

    // 简化的内置函数类型检查
    // 在实际实现中，这里应该有更详细的参数类型检查

    // 分析所有参数表达式
    for (int i = 0; i < call->data.call_expr.arg_count; i++) {
        struct Type* arg_type = semantic_analyze_expression(semantic, call->data.call_expr.args[i]);
        if (!arg_type) {
            return NULL;
        }
    }

    // 返回适当的类型
    if (strcmp(func_name, "printf") == 0 || strcmp(func_name, "scanf") == 0 ||
        strcmp(func_name, "fprintf") == 0 || strcmp(func_name, "fscanf") == 0) {
        return type_create(TYPE_INT);
    } else if (strcmp(func_name, "malloc") == 0 || strcmp(func_name, "calloc") == 0 ||
               strcmp(func_name, "realloc") == 0) {
        return type_create_pointer(type_create(TYPE_VOID));
    } else if (strcmp(func_name, "strlen") == 0) {
        return type_create(TYPE_LONG); // size_t
    } else if (strcmp(func_name, "strcpy") == 0 || strcmp(func_name, "strcat") == 0) {
        return type_create_pointer(type_create(TYPE_CHAR));
    } else if (strcmp(func_name, "strcmp") == 0 || strcmp(func_name, "atoi") == 0) {
        return type_create(TYPE_INT);
    } else if (strcmp(func_name, "atof") == 0 || strcmp(func_name, "sin") == 0 ||
               strcmp(func_name, "cos") == 0 || strcmp(func_name, "sqrt") == 0) {
        return type_create(TYPE_DOUBLE);
    } else {
        // 默认返回int
        return type_create(TYPE_INT);
    }
}

static uint16_t get_libc_function_id(const char* func_name) {
    if (!func_name) return 0x0000;

    // 基于函数名返回libc函数ID
    // 这些ID应该与运行时系统中的定义保持一致

    // stdio.h functions
    if (strcmp(func_name, "printf") == 0) return 0x0001;
    if (strcmp(func_name, "scanf") == 0) return 0x0002;
    if (strcmp(func_name, "fprintf") == 0) return 0x0003;
    if (strcmp(func_name, "fscanf") == 0) return 0x0004;
    if (strcmp(func_name, "sprintf") == 0) return 0x0005;
    if (strcmp(func_name, "sscanf") == 0) return 0x0006;
    if (strcmp(func_name, "fopen") == 0) return 0x0010;
    if (strcmp(func_name, "fclose") == 0) return 0x0011;
    if (strcmp(func_name, "fread") == 0) return 0x0012;
    if (strcmp(func_name, "fwrite") == 0) return 0x0013;
    if (strcmp(func_name, "fseek") == 0) return 0x0014;
    if (strcmp(func_name, "ftell") == 0) return 0x0015;
    if (strcmp(func_name, "fgetc") == 0) return 0x0016;
    if (strcmp(func_name, "fputc") == 0) return 0x0017;
    if (strcmp(func_name, "fgets") == 0) return 0x0018;
    if (strcmp(func_name, "fputs") == 0) return 0x0019;
    if (strcmp(func_name, "getchar") == 0) return 0x001A;
    if (strcmp(func_name, "putchar") == 0) return 0x001B;
    if (strcmp(func_name, "puts") == 0) return 0x001C;

    // stdlib.h functions
    if (strcmp(func_name, "malloc") == 0) return 0x0020;
    if (strcmp(func_name, "calloc") == 0) return 0x0021;
    if (strcmp(func_name, "realloc") == 0) return 0x0022;
    if (strcmp(func_name, "free") == 0) return 0x0023;
    if (strcmp(func_name, "exit") == 0) return 0x0024;
    if (strcmp(func_name, "abort") == 0) return 0x0025;
    if (strcmp(func_name, "atoi") == 0) return 0x0026;
    if (strcmp(func_name, "atof") == 0) return 0x0027;
    if (strcmp(func_name, "atol") == 0) return 0x0028;
    if (strcmp(func_name, "rand") == 0) return 0x0029;
    if (strcmp(func_name, "srand") == 0) return 0x002A;

    // string.h functions
    if (strcmp(func_name, "strlen") == 0) return 0x0030;
    if (strcmp(func_name, "strcpy") == 0) return 0x0031;
    if (strcmp(func_name, "strncpy") == 0) return 0x0032;
    if (strcmp(func_name, "strcat") == 0) return 0x0033;
    if (strcmp(func_name, "strncat") == 0) return 0x0034;
    if (strcmp(func_name, "strcmp") == 0) return 0x0035;
    if (strcmp(func_name, "strncmp") == 0) return 0x0036;
    if (strcmp(func_name, "strchr") == 0) return 0x0037;
    if (strcmp(func_name, "strrchr") == 0) return 0x0038;
    if (strcmp(func_name, "strstr") == 0) return 0x0039;
    if (strcmp(func_name, "memcpy") == 0) return 0x003A;
    if (strcmp(func_name, "memmove") == 0) return 0x003B;
    if (strcmp(func_name, "memset") == 0) return 0x003C;
    if (strcmp(func_name, "memcmp") == 0) return 0x003D;

    // math.h functions
    if (strcmp(func_name, "sin") == 0) return 0x0040;
    if (strcmp(func_name, "cos") == 0) return 0x0041;
    if (strcmp(func_name, "tan") == 0) return 0x0042;
    if (strcmp(func_name, "sqrt") == 0) return 0x0043;
    if (strcmp(func_name, "pow") == 0) return 0x0044;
    if (strcmp(func_name, "exp") == 0) return 0x0045;
    if (strcmp(func_name, "log") == 0) return 0x0046;
    if (strcmp(func_name, "log10") == 0) return 0x0047;
    if (strcmp(func_name, "fabs") == 0) return 0x0048;
    if (strcmp(func_name, "ceil") == 0) return 0x0049;
    if (strcmp(func_name, "floor") == 0) return 0x004A;

    return 0x0000; // 未知函数
}

static struct Type* type_create_pointer(struct Type* pointee) {
    if (!pointee) return NULL;

    struct Type* ptr_type = type_create(TYPE_POINTER);
    if (!ptr_type) return NULL;

    ptr_type->data.pointer.pointee = pointee;
    return ptr_type;
}

static bool type_is_integer(struct Type* type) {
    if (!type) return false;

    switch (type->kind) {
        case TYPE_CHAR:
        case TYPE_SHORT:
        case TYPE_INT:
        case TYPE_LONG:
        case TYPE_LONG_LONG:
        case TYPE_BOOL:
            return true;
        default:
            return false;
    }
}

static bool is_reserved_keyword(const char* name) {
    if (!name) return false;

    // C99关键字列表
    const char* keywords[] = {
        "auto", "break", "case", "char", "const", "continue", "default", "do",
        "double", "else", "enum", "extern", "float", "for", "goto", "if",
        "inline", "int", "long", "register", "restrict", "return", "short",
        "signed", "sizeof", "static", "struct", "switch", "typedef", "union",
        "unsigned", "void", "volatile", "while", "_Bool", "_Complex", "_Imaginary",
        NULL
    };

    for (int i = 0; keywords[i]; i++) {
        if (strcmp(name, keywords[i]) == 0) {
            return true;
        }
    }

    return false;
}

static bool validate_array_type(SemanticContext* semantic, struct Type* array_type, struct ASTNode* decl) {
    if (!semantic || !array_type || !decl) return false;

    if (array_type->kind != TYPE_ARRAY) return true;

    // 检查数组元素类型
    struct Type* element_type = array_type->data.array.element_type;
    if (!element_type) {
        semantic_error(semantic, decl, "Array missing element type");
        return false;
    }

    // 数组元素不能是void类型
    if (element_type->kind == TYPE_VOID) {
        semantic_error(semantic, decl, "Array cannot have void element type");
        return false;
    }

    // 数组元素不能是函数类型
    if (element_type->kind == TYPE_FUNCTION) {
        semantic_error(semantic, decl, "Array cannot have function element type");
        return false;
    }

    // 递归检查多维数组
    if (element_type->kind == TYPE_ARRAY) {
        return validate_array_type(semantic, element_type, decl);
    }

    return true;
}

static bool validate_variable_initializer(SemanticContext* semantic, struct Type* var_type,
                                         struct ASTNode* initializer, struct ASTNode* decl) {
    if (!semantic || !var_type || !initializer || !decl) return false;

    printf("Semantic: Validating variable initializer\n");

    // 分析初始化器表达式的类型
    struct Type* init_type = semantic_analyze_expression(semantic, initializer);
    if (!init_type) {
        semantic_error(semantic, decl, "Invalid initializer expression");
        return false;
    }

    // 检查类型兼容性
    if (!type_compatible(var_type, init_type)) {
        // 尝试隐式类型转换
        if (type_implicit_conversion(init_type, var_type)) {
            printf("Semantic: Implicit conversion in initializer\n");
            return true;
        } else {
            semantic_error(semantic, decl, "Initializer type incompatible with variable type");
            return false;
        }
    }

    // 特殊检查：数组初始化
    if (var_type->kind == TYPE_ARRAY) {
        return validate_array_initializer(semantic, var_type, initializer, decl);
    }

    // 特殊检查：结构体初始化
    if (var_type->kind == TYPE_STRUCT) {
        return validate_struct_initializer(semantic, var_type, initializer, decl);
    }

    printf("Semantic: Variable initializer validation successful\n");
    return true;
}

static bool validate_array_initializer(SemanticContext* semantic, struct Type* array_type,
                                      struct ASTNode* initializer, struct ASTNode* decl) {
    if (!semantic || !array_type || !initializer || !decl) return false;

    printf("Semantic: Validating array initializer\n");

    // 简化实现：目前只支持单个表达式初始化
    // 完整实现需要支持初始化器列表 {1, 2, 3}

    struct Type* element_type = array_type->data.array.element_type;
    struct Type* init_type = semantic_analyze_expression(semantic, initializer);

    if (!init_type) {
        semantic_error(semantic, decl, "Invalid array initializer");
        return false;
    }

    // 检查初始化器类型是否与数组元素类型兼容
    if (!type_compatible(element_type, init_type) &&
        !type_implicit_conversion(init_type, element_type)) {
        semantic_error(semantic, decl, "Array initializer type incompatible with element type");
        return false;
    }

    return true;
}

static bool validate_struct_initializer(SemanticContext* semantic, struct Type* struct_type,
                                       struct ASTNode* initializer, struct ASTNode* decl) {
    if (!semantic || !struct_type || !initializer || !decl) return false;

    printf("Semantic: Validating struct initializer\n");

    // 简化实现：目前只支持单个表达式初始化
    // 完整实现需要支持结构体初始化器列表 {.field1 = value1, .field2 = value2}

    struct Type* init_type = semantic_analyze_expression(semantic, initializer);
    if (!init_type) {
        semantic_error(semantic, decl, "Invalid struct initializer");
        return false;
    }

    // 检查是否为相同的结构体类型
    if (!type_compatible(struct_type, init_type)) {
        semantic_error(semantic, decl, "Struct initializer type mismatch");
        return false;
    }

    return true;
}

static bool is_lvalue_expression(struct ASTNode* expr) {
    if (!expr) return false;

    switch (expr->type) {
        case ASTC_EXPR_IDENTIFIER:
            // 变量标识符是左值
            return true;

        case ASTC_UNARY_OP:
            // 解引用操作 *ptr 是左值
            if (expr->data.unary_op.op == TOKEN_MULTIPLY) {
                return true;
            }
            return false;

        case ASTC_EXPR_ARRAY_SUBSCRIPT:
            // 数组访问 arr[i] 是左值
            return true;

        case ASTC_EXPR_MEMBER_ACCESS:
        case ASTC_EXPR_PTR_MEMBER_ACCESS:
            // 成员访问 obj.member 和 ptr->member 是左值
            return true;

        default:
            return false;
    }
}

static int get_base_operator(int compound_op) {
    switch (compound_op) {
        case TOKEN_PLUS_ASSIGN: return TOKEN_PLUS;
        case TOKEN_MINUS_ASSIGN: return TOKEN_MINUS;
        case TOKEN_MUL_ASSIGN: return TOKEN_MULTIPLY;
        case TOKEN_DIV_ASSIGN: return TOKEN_DIVIDE;
        case TOKEN_MOD_ASSIGN: return TOKEN_MODULO;
        case TOKEN_AND_ASSIGN: return TOKEN_BITWISE_AND;
        case TOKEN_OR_ASSIGN: return TOKEN_BITWISE_OR;
        case TOKEN_XOR_ASSIGN: return TOKEN_BITWISE_XOR;
        case TOKEN_LSHIFT_ASSIGN: return TOKEN_LEFT_SHIFT;
        case TOKEN_RSHIFT_ASSIGN: return TOKEN_RIGHT_SHIFT;
        default: return compound_op;
    }
}

static struct Type* check_binary_operation_compatibility(SemanticContext* semantic, int operator,
                                                       struct Type* left, struct Type* right, struct ASTNode* expr) {
    if (!semantic || !left || !right || !expr) return NULL;

    switch (operator) {
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            // 指针算术和普通算术
            if (left->kind == TYPE_POINTER && type_is_integral(right)) {
                return left;
            }
            if (type_is_integral(left) && right->kind == TYPE_POINTER) {
                return right;
            }
            if (operator == TOKEN_MINUS && left->kind == TYPE_POINTER && right->kind == TYPE_POINTER) {
                if (type_compatible(left->data.pointer.pointee, right->data.pointer.pointee)) {
                    return type_create(TYPE_INT);
                }
            }
            if (type_is_arithmetic(left) && type_is_arithmetic(right)) {
                return type_arithmetic_conversion(left, right);
            }
            return NULL;

        case TOKEN_MULTIPLY:
        case TOKEN_DIVIDE:
            if (type_is_arithmetic(left) && type_is_arithmetic(right)) {
                return type_arithmetic_conversion(left, right);
            }
            return NULL;

        case TOKEN_MODULO:
            if (type_is_integral(left) && type_is_integral(right)) {
                return type_arithmetic_conversion(left, right);
            }
            return NULL;

        case TOKEN_BITWISE_AND:
        case TOKEN_BITWISE_OR:
        case TOKEN_BITWISE_XOR:
        case TOKEN_LEFT_SHIFT:
        case TOKEN_RIGHT_SHIFT:
            if (type_is_integral(left) && type_is_integral(right)) {
                return type_arithmetic_conversion(left, right);
            }
            return NULL;

        default:
            return NULL;
    }
}

static bool is_zero_literal(struct ASTNode* expr) {
    if (!expr) return false;

    switch (expr->type) {
        case ASTC_EXPR_INTEGER_LITERAL:
            return expr->data.integer_literal.value == 0;

        case ASTC_EXPR_FLOAT_LITERAL:
            return expr->data.float_literal.value == 0.0;

        case ASTC_EXPR_CHAR_LITERAL:
            return expr->data.char_literal.value == 0;

        default:
            return false;
    }
}
