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
    
    // TODO: Implement based on AST structure
    printf("Semantic: Analyzing translation unit\n");
    
    return true;
}

bool semantic_analyze_function(SemanticContext* semantic, struct ASTNode* func) {
    if (!semantic || !func) return false;
    
    printf("Semantic: Analyzing function\n");
    
    semantic->in_function = true;
    semantic_enter_scope(semantic);
    
    // TODO: Analyze function parameters and body
    
    semantic_exit_scope(semantic);
    semantic->in_function = false;
    
    return true;
}

bool semantic_analyze_statement(SemanticContext* semantic, struct ASTNode* stmt) {
    if (!semantic || !stmt) return false;
    
    printf("Semantic: Analyzing statement\n");
    
    // TODO: Implement statement analysis
    
    return true;
}

struct Type* semantic_analyze_expression(SemanticContext* semantic, struct ASTNode* expr) {
    if (!semantic || !expr) return NULL;
    
    printf("Semantic: Analyzing expression\n");
    
    // TODO: Implement expression type checking
    
    return NULL;
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
