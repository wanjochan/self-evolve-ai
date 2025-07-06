/**
 * c99_semantic.c - C99 Semantic Analyzer Implementation
 */

#include "c99_semantic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
    
    // TODO: Free type information
    
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
    symbol->type = (struct ASTNode*)type; // TODO: Fix type system
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
    struct ASTNode* current = ast->first_child;
    while (current) {
        if (current->type == ASTC_FUNC_DECL) {
            if (!semantic_analyze_function(semantic, current)) {
                return false;
            }
        } else if (current->type == ASTC_VAR_DECL) {
            if (!semantic_analyze_declaration(semantic, current)) {
                return false;
            }
        }
        current = current->next;
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
    semantic_enter_scope(semantic);
    
    // 分析返回类型
    struct Type* return_type = analyze_type(semantic, func->return_type);
    if (!return_type) {
        semantic_error(semantic, func, "Invalid return type");
        return false;
    }
    
    // 分析参数
    struct ASTNode* param = func->parameters;
    while (param) {
        if (!semantic_analyze_declaration(semantic, param)) {
            return false;
        }
        param = param->next;
    }
    
    // 分析函数体
    if (func->body) {
        semantic->current_function_type = return_type;
        if (!semantic_analyze_statement(semantic, func->body)) {
            return false;
        }
        semantic->current_function_type = NULL;
    }
    
    semantic_exit_scope(semantic);
    semantic->in_function = false;
    
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
            
        case ASTC_RETURN_STMT:
            return analyze_return_statement(semantic, stmt);
            
        case ASTC_EXPR_STMT:
            return semantic_analyze_expression(semantic, stmt->expression) != NULL;
            
        default:
            semantic_error(semantic, stmt, "Unsupported statement type");
            return false;
    }
}

struct Type* semantic_analyze_expression(SemanticContext* semantic, struct ASTNode* expr) {
    if (!semantic || !expr) return NULL;
    
    switch (expr->type) {
        case ASTC_BINARY_EXPR: {
            struct Type* left = semantic_analyze_expression(semantic, expr->left);
            struct Type* right = semantic_analyze_expression(semantic, expr->right);
            
            if (!left || !right) return NULL;
            
            // 类型检查和转换
            return check_binary_operation(semantic, expr->operator, left, right, expr);
        }
        
        case ASTC_UNARY_EXPR: {
            struct Type* operand = semantic_analyze_expression(semantic, expr->operand);
            if (!operand) return NULL;
            
            return check_unary_operation(semantic, expr->operator, operand, expr);
        }
        
        case ASTC_IDENTIFIER: {
            Symbol* symbol = semantic_lookup_symbol(semantic, expr->name);
            if (!symbol) {
                semantic_error(semantic, expr, "Undefined identifier");
                return NULL;
            }
            symbol->is_used = true;
            return symbol->type;
        }
        
        case ASTC_INTEGER_LITERAL:
            return type_create(TYPE_INT);
            
        case ASTC_FLOAT_LITERAL:
            return type_create(TYPE_FLOAT);
            
        case ASTC_CALL_EXPR: {
            Symbol* func = semantic_lookup_symbol(semantic, expr->name);
            if (!func || func->kind != SYMBOL_FUNCTION) {
                semantic_error(semantic, expr, "Called object is not a function");
                return NULL;
            }
            
            // 检查参数数量和类型
            if (!check_function_call(semantic, func, expr)) {
                return NULL;
            }
            
            return ((struct Type*)func->type)->data.function.return_type;
        }
        
        default:
            semantic_error(semantic, expr, "Unsupported expression type");
            return NULL;
    }
}

// 辅助函数实现
static bool analyze_compound_statement(SemanticContext* semantic, struct ASTNode* stmt) {
    struct ASTNode* current = stmt->first_child;
    while (current) {
        if (!semantic_analyze_statement(semantic, current)) {
            return false;
        }
        current = current->next;
    }
    return true;
}

static bool analyze_if_statement(SemanticContext* semantic, struct ASTNode* stmt) {
    struct Type* cond_type = semantic_analyze_expression(semantic, stmt->condition);
    if (!cond_type || !type_is_scalar(cond_type)) {
        semantic_error(semantic, stmt->condition, "Condition must be a scalar type");
        return false;
    }
    
    return semantic_analyze_statement(semantic, stmt->then_branch) &&
           (!stmt->else_branch || semantic_analyze_statement(semantic, stmt->else_branch));
}

static bool analyze_while_statement(SemanticContext* semantic, struct ASTNode* stmt) {
    struct Type* cond_type = semantic_analyze_expression(semantic, stmt->condition);
    if (!cond_type || !type_is_scalar(cond_type)) {
        semantic_error(semantic, stmt->condition, "Condition must be a scalar type");
        return false;
    }
    
    return semantic_analyze_statement(semantic, stmt->body);
}

static bool analyze_return_statement(SemanticContext* semantic, struct ASTNode* stmt) {
    if (!semantic->in_function) {
        semantic_error(semantic, stmt, "Return statement outside of function");
        return false;
    }
    
    if (!stmt->expression) {
        if (semantic->current_function_type->kind != TYPE_VOID) {
            semantic_error(semantic, stmt, "Return value required");
            return false;
        }
        return true;
    }
    
    struct Type* expr_type = semantic_analyze_expression(semantic, stmt->expression);
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
    // 算术运算符
    if (operator >= TOKEN_PLUS && operator <= TOKEN_MODULO) {
        if (!type_is_arithmetic(left) || !type_is_arithmetic(right)) {
            semantic_error(semantic, expr, "Operands must be arithmetic types");
            return NULL;
        }
        return type_arithmetic_conversion(left, right);
    }
    
    // 比较运算符
    if (operator >= TOKEN_EQUAL && operator <= TOKEN_GREATER_EQUAL) {
        if (!type_compatible(left, right)) {
            semantic_error(semantic, expr, "Incompatible operand types for comparison");
            return NULL;
        }
        return type_create(TYPE_INT); // 比较运算返回int
    }
    
    // 逻辑运算符
    if (operator == TOKEN_LOGICAL_AND || operator == TOKEN_LOGICAL_OR) {
        if (!type_is_scalar(left) || !type_is_scalar(right)) {
            semantic_error(semantic, expr, "Operands must be scalar types");
            return NULL;
        }
        return type_create(TYPE_INT); // 逻辑运算返回int
    }
    
    semantic_error(semantic, expr, "Unsupported binary operator");
    return NULL;
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
            
        default:
            semantic_error(semantic, expr, "Unsupported unary operator");
            return NULL;
    }
}

static bool check_function_call(SemanticContext* semantic, Symbol* func,
                              struct ASTNode* call) {
    struct Type* func_type = func->type;
    struct ASTNode* arg = call->arguments;
    int param_count = 0;
    
    // 检查每个参数
    while (arg && param_count < func_type->data.function.parameter_count) {
        struct Type* arg_type = semantic_analyze_expression(semantic, arg);
        if (!arg_type) return false;
        
        struct Type* param_type = func_type->data.function.parameters[param_count];
        if (!type_compatible(param_type, arg_type)) {
            semantic_error(semantic, arg,
                         "Argument type incompatible with parameter type");
            return false;
        }
        
        arg = arg->next;
        param_count++;
    }
    
    // 检查参数数量
    if (arg && !func_type->data.function.is_variadic) {
        semantic_error(semantic, call, "Too many arguments");
        return false;
    }
    
    if (param_count < func_type->data.function.parameter_count) {
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
    
    // TODO: Free type-specific data
    
    free(type);
}

bool type_compatible(struct Type* type1, struct Type* type2) {
    if (!type1 || !type2) return false;
    
    // Simplified compatibility check
    return type1->kind == type2->kind;
}

size_t type_get_size(struct Type* type) {
    if (!type) return 0;
    
    switch (type->kind) {
        case TYPE_CHAR: return 1;
        case TYPE_SHORT: return 2;
        case TYPE_INT: return 4;
        case TYPE_LONG: return 8;
        case TYPE_FLOAT: return 4;
        case TYPE_DOUBLE: return 8;
        default: return 0;
    }
}

bool type_is_arithmetic(struct Type* type) {
    if (!type) return false;
    
    return type->kind >= TYPE_CHAR && type->kind <= TYPE_IMAGINARY;
}

bool type_is_integral(struct Type* type) {
    if (!type) return false;
    
    return type->kind >= TYPE_CHAR && type->kind <= TYPE_LONG_LONG;
}

// ===============================================
// Error Handling
// ===============================================

void semantic_error(SemanticContext* semantic, struct ASTNode* node, const char* message) {
    if (!semantic) return;
    
    semantic->has_error = true;
    semantic->error_count++;
    
    int line = node ? 0 : 0; // TODO: Get line from node
    int column = node ? 0 : 0; // TODO: Get column from node
    
    snprintf(semantic->error_message, sizeof(semantic->error_message),
             "Semantic error at line %d, column %d: %s", line, column, message);
    
    printf("Semantic Error: %s\n", semantic->error_message);
}

void semantic_warning(SemanticContext* semantic, struct ASTNode* node, const char* message) {
    if (!semantic) return;
    
    semantic->warning_count++;
    
    int line = node ? 0 : 0; // TODO: Get line from node
    int column = node ? 0 : 0; // TODO: Get column from node
    
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
