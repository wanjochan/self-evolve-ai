/**
 * c99_parser.c - C99 Syntax Parser Implementation
 */

#include "c99_parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ===============================================
// Parser Context Management
// ===============================================

ParserContext* parser_create(LexerContext* lexer) {
    if (!lexer) return NULL;
    
    ParserContext* parser = malloc(sizeof(ParserContext));
    if (!parser) return NULL;
    
    memset(parser, 0, sizeof(ParserContext));
    parser->lexer = lexer;
    parser->ast_node_capacity = 1024;
    parser->ast_nodes = malloc(sizeof(struct ASTNode*) * parser->ast_node_capacity);
    
    if (!parser->ast_nodes) {
        free(parser);
        return NULL;
    }
    
    // Get first token
    parser->current_token = lexer_next_token(lexer);
    parser->lookahead_token = lexer_next_token(lexer);
    
    return parser;
}

void parser_destroy(ParserContext* parser) {
    if (!parser) return;
    
    // Free all AST nodes
    for (size_t i = 0; i < parser->ast_node_count; i++) {
        if (parser->ast_nodes[i]) {
            // ast_free(parser->ast_nodes[i]); // TODO: Implement ast_free
            free(parser->ast_nodes[i]);
        }
    }
    
    free(parser->ast_nodes);
    
    if (parser->current_token) {
        token_free(parser->current_token);
    }
    
    if (parser->lookahead_token) {
        token_free(parser->lookahead_token);
    }
    
    free(parser);
}

// ===============================================
// Parser Utility Functions
// ===============================================

bool parser_advance(ParserContext* parser) {
    if (!parser) return false;
    
    if (parser->current_token) {
        token_free(parser->current_token);
    }
    
    parser->current_token = parser->lookahead_token;
    parser->lookahead_token = lexer_next_token(parser->lexer);
    
    return parser->current_token != NULL;
}

bool parser_match(ParserContext* parser, TokenType expected) {
    return parser && parser->current_token && parser->current_token->type == expected;
}

bool parser_consume(ParserContext* parser, TokenType expected) {
    if (parser_match(parser, expected)) {
        parser_advance(parser);
        return true;
    }
    return false;
}

bool parser_expect(ParserContext* parser, TokenType expected) {
    if (parser_match(parser, expected)) {
        parser_advance(parser);
        return true;
    }
    
    char error_msg[256];
    snprintf(error_msg, sizeof(error_msg), "Expected %s, got %s",
             token_type_name(expected),
             parser->current_token ? token_type_name(parser->current_token->type) : "EOF");
    parser_error(parser, error_msg);
    return false;
}

void parser_error(ParserContext* parser, const char* message) {
    if (!parser) return;
    
    parser->has_error = true;
    parser->error_count++;
    
    int line = parser->current_token ? parser->current_token->line : 0;
    int column = parser->current_token ? parser->current_token->column : 0;
    
    snprintf(parser->error_message, sizeof(parser->error_message),
             "Parse error at line %d, column %d: %s", line, column, message);
    
    printf("Parser Error: %s\n", parser->error_message);
}

struct ASTNode* parser_create_ast_node(ParserContext* parser, ASTNodeType type) {
    if (!parser) return NULL;
    
    int line = parser->current_token ? parser->current_token->line : 0;
    int column = parser->current_token ? parser->current_token->column : 0;
    
    // TODO: Implement ast_create_node
    struct ASTNode* node = malloc(sizeof(struct ASTNode));
    if (node) {
        memset(node, 0, sizeof(struct ASTNode));
        // node->type = type;
        // node->line = line;
        // node->column = column;
        
        // Register node for cleanup
        if (parser->ast_node_count >= parser->ast_node_capacity) {
            parser->ast_node_capacity *= 2;
            parser->ast_nodes = realloc(parser->ast_nodes, 
                                       sizeof(struct ASTNode*) * parser->ast_node_capacity);
        }
        
        parser->ast_nodes[parser->ast_node_count++] = node;
    }
    
    return node;
}

// ===============================================
// Core Parsing Functions
// ===============================================

struct ASTNode* parser_parse_translation_unit(ParserContext* parser) {
    if (!parser) return NULL;
    
    struct ASTNode* translation_unit = parser_create_ast_node(parser, 0); // ASTC_TRANSLATION_UNIT
    if (!translation_unit) return NULL;
    
    printf("Parser: Parsing translation unit\n");
    
    // Parse external declarations
    while (parser->current_token && parser->current_token->type != TOKEN_EOF) {
        struct ASTNode* external_decl = parser_parse_external_declaration(parser);
        if (!external_decl) {
            if (parser->has_error) {
                break;
            }
            continue;
        }
        
        // TODO: Add to translation unit
    }
    
    return translation_unit;
}

struct ASTNode* parser_parse_external_declaration(ParserContext* parser) {
    if (!parser || !parser->current_token) return NULL;
    
    // For now, assume all external declarations are function definitions
    return parser_parse_function_definition(parser);
}

struct ASTNode* parser_parse_function_definition(ParserContext* parser) {
    if (!parser) return NULL;
    
    struct ASTNode* func_decl = parser_create_ast_node(parser, 1); // ASTC_FUNC_DECL
    if (!func_decl) return NULL;
    
    printf("Parser: Parsing function definition\n");
    
    // Skip type specifiers for now
    while (parser->current_token && 
           (parser->current_token->type == TOKEN_INT || 
            parser->current_token->type == TOKEN_VOID ||
            parser->current_token->type == TOKEN_CHAR)) {
        parser_advance(parser);
    }
    
    // Parse function name
    if (parser_match(parser, TOKEN_IDENTIFIER)) {
        printf("Parser: Found function '%s'\n", parser->current_token->value);
        parser_advance(parser);
    } else {
        parser_error(parser, "Expected function name");
        return NULL;
    }
    
    // Parse parameter list
    if (parser_expect(parser, TOKEN_LPAREN)) {
        // Skip to closing paren for now
        int paren_count = 1;
        while (paren_count > 0 && parser->current_token && parser->current_token->type != TOKEN_EOF) {
            if (parser->current_token->type == TOKEN_LPAREN) {
                paren_count++;
            } else if (parser->current_token->type == TOKEN_RPAREN) {
                paren_count--;
            }
            parser_advance(parser);
        }
    }
    
    // Parse function body
    if (parser_match(parser, TOKEN_LBRACE)) {
        return parser_parse_compound_statement(parser);
    } else {
        parser_error(parser, "Expected function body");
        return NULL;
    }
}

struct ASTNode* parser_parse_compound_statement(ParserContext* parser) {
    if (!parser) return NULL;
    
    if (!parser_expect(parser, TOKEN_LBRACE)) {
        return NULL;
    }
    
    struct ASTNode* compound = parser_create_ast_node(parser, 2); // ASTC_COMPOUND_STMT
    if (!compound) return NULL;
    
    parser->scope_depth++;
    
    // Parse statements until closing brace
    while (parser->current_token && 
           parser->current_token->type != TOKEN_RBRACE && 
           parser->current_token->type != TOKEN_EOF) {
        
        struct ASTNode* stmt = parser_parse_statement(parser);
        if (stmt) {
            // TODO: Add statement to compound
        }
    }
    
    parser->scope_depth--;
    
    if (!parser_expect(parser, TOKEN_RBRACE)) {
        return NULL;
    }
    
    return compound;
}

struct ASTNode* parser_parse_statement(ParserContext* parser) {
    if (!parser || !parser->current_token) return NULL;
    
    switch (parser->current_token->type) {
        case TOKEN_RETURN:
            return parser_parse_jump_statement(parser);
        case TOKEN_LBRACE:
            return parser_parse_compound_statement(parser);
        default:
            return parser_parse_expression_statement(parser);
    }
}

struct ASTNode* parser_parse_expression_statement(ParserContext* parser) {
    if (!parser) return NULL;
    
    struct ASTNode* expr_stmt = parser_create_ast_node(parser, 3); // ASTC_EXPR_STMT
    if (!expr_stmt) return NULL;
    
    // Parse expression (simplified)
    if (parser->current_token->type != TOKEN_SEMICOLON) {
        parser_parse_expression(parser);
    }
    
    parser_expect(parser, TOKEN_SEMICOLON);
    
    return expr_stmt;
}

struct ASTNode* parser_parse_jump_statement(ParserContext* parser) {
    if (!parser || !parser->current_token) return NULL;
    
    if (parser->current_token->type == TOKEN_RETURN) {
        struct ASTNode* return_stmt = parser_create_ast_node(parser, 4); // ASTC_RETURN_STMT
        parser_advance(parser); // consume 'return'
        
        // Parse optional return value
        if (parser->current_token->type != TOKEN_SEMICOLON) {
            parser_parse_expression(parser);
        }
        
        parser_expect(parser, TOKEN_SEMICOLON);
        return return_stmt;
    }
    
    parser_error(parser, "Unsupported jump statement");
    return NULL;
}

struct ASTNode* parser_parse_expression(ParserContext* parser) {
    if (!parser || !parser->current_token) return NULL;
    
    // Simplified expression parsing
    if (parser->current_token->type == TOKEN_IDENTIFIER) {
        printf("Parser: Found identifier '%s'\n", parser->current_token->value);
        parser_advance(parser);
    } else if (parser->current_token->type == TOKEN_INTEGER_LITERAL) {
        printf("Parser: Found integer '%s'\n", parser->current_token->value);
        parser_advance(parser);
    } else {
        // Skip unknown tokens
        parser_advance(parser);
    }
    
    return parser_create_ast_node(parser, 5); // ASTC_EXPR
}

// ===============================================
// Utility Functions
// ===============================================

bool parser_has_error(ParserContext* parser) {
    return parser && parser->has_error;
}

const char* parser_get_error(ParserContext* parser) {
    return parser ? parser->error_message : "Invalid parser";
}

void parser_print_stats(ParserContext* parser) {
    if (!parser) return;
    
    printf("Parser Statistics:\n");
    printf("  AST Nodes Created: %zu\n", parser->ast_node_count);
    printf("  Errors: %d\n", parser->error_count);
    printf("  Warnings: %d\n", parser->warning_count);
    printf("  Scope Depth: %d\n", parser->scope_depth);
}
