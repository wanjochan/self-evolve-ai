/**
 * c99_parser.h - C99 Syntax Parser
 * 
 * Recursive descent parser for C99 language according to ISO/IEC 9899:1999.
 * Generates Abstract Syntax Tree (AST) from token stream.
 */

#ifndef C99_PARSER_H
#define C99_PARSER_H

#include "c99_lexer.h"
#include "../../core/astc.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// Parser Context
// ===============================================

typedef struct {
    LexerContext* lexer;        // Lexer for token stream
    Token* current_token;       // Current token
    Token* lookahead_token;     // Lookahead token
    
    // Error handling
    char error_message[512];
    bool has_error;
    int error_count;
    int warning_count;
    
    // Parse state
    int scope_depth;            // Current scope depth
    bool in_function;           // Currently parsing function
    bool in_loop;               // Currently in loop (for break/continue)
    bool in_switch;             // Currently in switch (for case/default)
    
    // Memory management
    struct ASTNode** ast_nodes; // All allocated AST nodes
    size_t ast_node_count;      // Number of allocated nodes
    size_t ast_node_capacity;   // Capacity of ast_nodes array
} ParserContext;

// ===============================================
// Parser Functions
// ===============================================

/**
 * Create parser context
 */
ParserContext* parser_create(LexerContext* lexer);

/**
 * Destroy parser context
 */
void parser_destroy(ParserContext* parser);

/**
 * Parse complete translation unit (main entry point)
 */
struct ASTNode* parser_parse_translation_unit(ParserContext* parser);

/**
 * Parse external declaration
 */
struct ASTNode* parser_parse_external_declaration(ParserContext* parser);

/**
 * Parse function definition
 */
struct ASTNode* parser_parse_function_definition(ParserContext* parser);

/**
 * Parse declaration
 */
struct ASTNode* parser_parse_declaration(ParserContext* parser);

/**
 * Parse statement
 */
struct ASTNode* parser_parse_statement(ParserContext* parser);

/**
 * Parse expression
 */
struct ASTNode* parser_parse_expression(ParserContext* parser);

/**
 * Parse compound statement
 */
struct ASTNode* parser_parse_compound_statement(ParserContext* parser);

/**
 * Parse expression statement
 */
struct ASTNode* parser_parse_expression_statement(ParserContext* parser);

/**
 * Parse jump statement (return/break/continue/goto)
 */
struct ASTNode* parser_parse_jump_statement(ParserContext* parser);

// ===============================================
// Parser Utility Functions
// ===============================================

/**
 * Advance to next token
 */
bool parser_advance(ParserContext* parser);

/**
 * Check if current token matches expected type
 */
bool parser_match(ParserContext* parser, TokenType expected);

/**
 * Consume token if it matches expected type
 */
bool parser_consume(ParserContext* parser, TokenType expected);

/**
 * Expect specific token type (error if not found)
 */
bool parser_expect(ParserContext* parser, TokenType expected);

/**
 * Check if parser has error
 */
bool parser_has_error(ParserContext* parser);

/**
 * Get error message
 */
const char* parser_get_error(ParserContext* parser);

/**
 * Report parser error
 */
void parser_error(ParserContext* parser, const char* message);

/**
 * Create AST node with automatic registration
 */
struct ASTNode* parser_create_ast_node(ParserContext* parser, ASTNodeType type);

/**
 * Print parser statistics
 */
void parser_print_stats(ParserContext* parser);

#ifdef __cplusplus
}
#endif

#endif // C99_PARSER_H
