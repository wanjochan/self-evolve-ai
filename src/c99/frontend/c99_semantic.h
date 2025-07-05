/**
 * c99_semantic.h - C99 Semantic Analyzer
 * 
 * Semantic analysis for C99 including type checking, scope management,
 * and symbol table management according to ISO/IEC 9899:1999.
 */

#ifndef C99_SEMANTIC_H
#define C99_SEMANTIC_H

#include "c99_parser.h"
#include "../../core/astc.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// Symbol Table
// ===============================================

typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_TYPE,
    SYMBOL_ENUM_CONSTANT,
    SYMBOL_LABEL
} SymbolKind;

typedef enum {
    STORAGE_AUTO,
    STORAGE_REGISTER,
    STORAGE_STATIC,
    STORAGE_EXTERN,
    STORAGE_TYPEDEF
} StorageClass;

typedef struct Symbol {
    char* name;                     // Symbol name
    SymbolKind kind;                // Symbol kind
    struct ASTNode* type;           // Type information
    StorageClass storage_class;     // Storage class
    int scope_level;                // Scope level
    bool is_defined;                // Is symbol defined (vs declared)
    bool is_used;                   // Is symbol used
    
    // Location information
    int line;
    int column;
    
    // Symbol-specific data
    union {
        struct {
            struct ASTNode* initializer;
            bool is_parameter;
        } variable;
        
        struct {
            struct ASTNode* parameters;
            struct ASTNode* body;
            bool is_inline;
            bool is_variadic;
        } function;
        
        struct {
            long long value;
        } enum_constant;
    } data;
    
    struct Symbol* next;            // Next symbol in hash bucket
} Symbol;

typedef struct SymbolTable {
    Symbol** buckets;               // Hash table buckets
    size_t bucket_count;            // Number of buckets
    size_t symbol_count;            // Total symbols
    int current_scope;              // Current scope level
    struct SymbolTable* parent;     // Parent scope
} SymbolTable;

// ===============================================
// Type System
// ===============================================

typedef enum {
    TYPE_VOID,
    TYPE_CHAR,
    TYPE_SHORT,
    TYPE_INT,
    TYPE_LONG,
    TYPE_LONG_LONG,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_LONG_DOUBLE,
    TYPE_BOOL,
    TYPE_COMPLEX,
    TYPE_IMAGINARY,
    TYPE_POINTER,
    TYPE_ARRAY,
    TYPE_FUNCTION,
    TYPE_STRUCT,
    TYPE_UNION,
    TYPE_ENUM
} TypeKind;

typedef struct Type {
    TypeKind kind;
    bool is_const;
    bool is_volatile;
    bool is_restrict;
    bool is_signed;
    bool is_unsigned;
    
    union {
        struct {
            struct Type* pointee;
        } pointer;
        
        struct {
            struct Type* element;
            size_t size;
            bool is_vla;                // Variable length array
        } array;
        
        struct {
            struct Type* return_type;
            struct Type** parameters;
            size_t parameter_count;
            bool is_variadic;
        } function;
        
        struct {
            char* tag;
            Symbol** members;
            size_t member_count;
            bool is_complete;
        } composite;
    } data;
} Type;

// ===============================================
// Semantic Analyzer Context
// ===============================================

typedef struct {
    SymbolTable* global_scope;      // Global symbol table
    SymbolTable* current_scope;     // Current scope
    
    // Type checking state
    struct Type* current_function_type;
    bool in_function;
    bool in_loop;
    bool in_switch;
    
    // Error handling
    char error_message[512];
    bool has_error;
    int error_count;
    int warning_count;
    
    // Options
    bool strict_mode;               // Strict C99 compliance
    bool allow_extensions;          // Allow compiler extensions
    bool warn_unused;               // Warn about unused symbols
} SemanticContext;

// ===============================================
// Semantic Analysis Functions
// ===============================================

/**
 * Create semantic analyzer context
 */
SemanticContext* semantic_create(void);

/**
 * Destroy semantic analyzer context
 */
void semantic_destroy(SemanticContext* semantic);

/**
 * Analyze AST for semantic correctness
 */
bool semantic_analyze(SemanticContext* semantic, struct ASTNode* ast);

/**
 * Analyze translation unit
 */
bool semantic_analyze_translation_unit(SemanticContext* semantic, struct ASTNode* ast);

/**
 * Analyze function definition
 */
bool semantic_analyze_function(SemanticContext* semantic, struct ASTNode* func);

/**
 * Analyze statement
 */
bool semantic_analyze_statement(SemanticContext* semantic, struct ASTNode* stmt);

/**
 * Analyze expression
 */
struct Type* semantic_analyze_expression(SemanticContext* semantic, struct ASTNode* expr);

// ===============================================
// Symbol Table Functions
// ===============================================

/**
 * Create symbol table
 */
SymbolTable* symbol_table_create(SymbolTable* parent);

/**
 * Destroy symbol table
 */
void symbol_table_destroy(SymbolTable* table);

/**
 * Enter new scope
 */
void semantic_enter_scope(SemanticContext* semantic);

/**
 * Exit current scope
 */
void semantic_exit_scope(SemanticContext* semantic);

/**
 * Declare symbol
 */
Symbol* semantic_declare_symbol(SemanticContext* semantic, const char* name, 
                               SymbolKind kind, struct Type* type);

/**
 * Lookup symbol
 */
Symbol* semantic_lookup_symbol(SemanticContext* semantic, const char* name);

// ===============================================
// Type System Functions
// ===============================================

/**
 * Create type
 */
struct Type* type_create(TypeKind kind);

/**
 * Destroy type
 */
void type_destroy(struct Type* type);

/**
 * Check type compatibility
 */
bool type_compatible(struct Type* type1, struct Type* type2);

/**
 * Get type size
 */
size_t type_get_size(struct Type* type);

/**
 * Check if type is arithmetic
 */
bool type_is_arithmetic(struct Type* type);

/**
 * Check if type is integral
 */
bool type_is_integral(struct Type* type);

// ===============================================
// Error Handling
// ===============================================

/**
 * Report semantic error
 */
void semantic_error(SemanticContext* semantic, struct ASTNode* node, const char* message);

/**
 * Report semantic warning
 */
void semantic_warning(SemanticContext* semantic, struct ASTNode* node, const char* message);

/**
 * Check if semantic analyzer has error
 */
bool semantic_has_error(SemanticContext* semantic);

/**
 * Get error message
 */
const char* semantic_get_error(SemanticContext* semantic);

/**
 * Print semantic analysis statistics
 */
void semantic_print_stats(SemanticContext* semantic);

#ifdef __cplusplus
}
#endif

#endif // C99_SEMANTIC_H
