/**
 * c99_parser.c - C99 Syntax Parser Implementation
 */

#include "c99_parser.h"
#include "../../core/astc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Forward declarations
static struct ASTNode* parser_parse_variable_declaration(ParserContext* parser);
// Forward declarations removed - now in header file
static struct ASTNode* parser_parse_struct_declaration(ParserContext* parser);
static struct ASTNode* parser_parse_union_declaration(ParserContext* parser);
static struct ASTNode* parser_parse_pointer_declarator(ParserContext* parser);
static struct ASTNode* parser_parse_array_declarator(ParserContext* parser);
static struct ASTNode* parser_parse_type_specifier(ParserContext* parser);

// ===============================================
// Parser Context Management
// ===============================================

ParserContext* parser_create(LexerContext* lexer) {
    if (!lexer) return NULL;
    
    ParserContext* parser = malloc(sizeof(ParserContext));
    if (!parser) return NULL;
    
    memset(parser, 0, sizeof(ParserContext));
    parser->lexer = lexer;
    
    // Get first token
    parser->current_token = lexer_next_token(lexer);
    parser->lookahead_token = lexer_next_token(lexer);
    
    return parser;
}

void parser_destroy(ParserContext* parser) {
    if (!parser) return;
    
    // Note: AST nodes are owned by the caller and should not be freed here
    // The caller is responsible for calling ast_free() on the root node
    // which will recursively free all child nodes
    
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

    // Use the proper ast_create_node function
    struct ASTNode* node = ast_create_node(type, line, column);

    // Note: We don't track individual nodes anymore since they form a tree
    // The caller is responsible for managing the root node

    return node;
}

// ===============================================
// Core Parsing Functions
// ===============================================

struct ASTNode* parser_parse_translation_unit(ParserContext* parser) {
    if (!parser) return NULL;

    struct ASTNode* translation_unit = parser_create_ast_node(parser, ASTC_TRANSLATION_UNIT);
    if (!translation_unit) return NULL;

    printf("Parser: Parsing translation unit\n");

    // Initialize translation unit data
    translation_unit->data.translation_unit.declarations = NULL;
    translation_unit->data.translation_unit.declaration_count = 0;

    // Parse external declarations
    while (parser->current_token && parser->current_token->type != TOKEN_EOF) {
        struct ASTNode* external_decl = parser_parse_external_declaration(parser);
        if (!external_decl) {
            if (parser->has_error) {
                break;
            }
            continue;
        }

        // Add to translation unit
        translation_unit->data.translation_unit.declaration_count++;
        translation_unit->data.translation_unit.declarations = realloc(
            translation_unit->data.translation_unit.declarations,
            sizeof(struct ASTNode*) * translation_unit->data.translation_unit.declaration_count
        );
        translation_unit->data.translation_unit.declarations[
            translation_unit->data.translation_unit.declaration_count - 1
        ] = external_decl;
    }

    return translation_unit;
}

struct ASTNode* parser_parse_external_declaration(ParserContext* parser) {
    if (!parser || !parser->current_token) return NULL;

    // Check for struct/union declarations first
    if (parser->current_token->type == TOKEN_STRUCT) {
        return parser_parse_struct_declaration(parser);
    }
    if (parser->current_token->type == TOKEN_UNION) {
        return parser_parse_union_declaration(parser);
    }

    // Improved lookahead to distinguish between function and variable declarations
    // We need to look past type specifiers to find the pattern:
    // - "type identifier (" = function
    // - "type identifier ;" or "type identifier =" = variable

    // Count how many type/storage tokens we need to skip
    int skip_count = 0;
    Token* current = parser->current_token;

    // Count type specifiers and storage class specifiers
    while (current &&
           (current->type == TOKEN_INT ||
            current->type == TOKEN_VOID ||
            current->type == TOKEN_CHAR ||
            current->type == TOKEN_FLOAT ||
            current->type == TOKEN_DOUBLE ||
            current->type == TOKEN_STRUCT ||
            current->type == TOKEN_UNION ||
            current->type == TOKEN_STATIC ||
            current->type == TOKEN_EXTERN)) {
        skip_count++;
        if (skip_count == 1) {
            current = parser->lookahead_token;
        } else {
            // We can only look ahead one token easily
            break;
        }
    }

    // Determine if this is a function or variable declaration
    bool is_function = false;

    if (skip_count == 1 && current && current->type == TOKEN_IDENTIFIER) {
        // We have "type identifier", now we need to check what comes after
        // Since we can't easily peek further, we'll use a different approach:
        // Look at the current pattern and make a reasonable guess

        // If we have exactly one type token followed by identifier,
        // we need to peek at what comes after the identifier
        // For now, let's try a simpler approach: parse as function if we see certain patterns

        // Check if the identifier looks like a function name (heuristic)
        if (current && current->value) {
            // Common function names or patterns
            if (strcmp(current->value, "main") == 0 ||
                strstr(current->value, "func") != NULL ||
                strstr(current->value, "test") != NULL) {
                is_function = true;
            }
        }
    }

    // If we still can't determine, try parsing as variable first
    // If that fails, we'll fall back to function parsing
    if (!is_function) {
        // Try variable declaration first
        struct ASTNode* var_result = parser_parse_variable_declaration(parser);
        if (var_result || !parser->has_error) {
            return var_result;
        }

        // If variable parsing failed, reset error state and try function
        parser->has_error = false;
        parser->error_count = 0;
        memset(parser->error_message, 0, sizeof(parser->error_message));
    }

    // Parse as function
    return parser_parse_function_definition(parser);
}

struct ASTNode* parser_parse_function_definition(ParserContext* parser) {
    if (!parser) return NULL;

    struct ASTNode* func_decl = parser_create_ast_node(parser, ASTC_FUNC_DECL);
    if (!func_decl) return NULL;

    printf("Parser: Parsing function definition\n");

    // Initialize function declaration data
    func_decl->data.func_decl.name = NULL;
    func_decl->data.func_decl.return_type = NULL;
    func_decl->data.func_decl.params = NULL;
    func_decl->data.func_decl.param_count = 0;
    func_decl->data.func_decl.has_body = 0;
    func_decl->data.func_decl.body = NULL;

    // Skip type specifiers for now (TODO: parse return type properly)
    while (parser->current_token &&
           (parser->current_token->type == TOKEN_INT ||
            parser->current_token->type == TOKEN_VOID ||
            parser->current_token->type == TOKEN_CHAR)) {
        printf("Parser: Skipping type specifier '%s'\n", parser->current_token->value);
        parser_advance(parser);
    }

    printf("Parser: Current token after type specifiers: %s '%s'\n",
           token_type_name(parser->current_token->type),
           parser->current_token->value);

    // Parse function name
    if (parser->current_token && parser->current_token->type == TOKEN_IDENTIFIER) {
        func_decl->data.func_decl.name = strdup(parser->current_token->value);
        printf("Parser: Found function '%s'\n", func_decl->data.func_decl.name);
        parser_advance(parser);
    } else {
        parser_error(parser, "Expected function name");
        ast_free(func_decl);  // Free the allocated node before returning NULL
        return NULL;
    }

    // Parse parameter list
    if (parser_expect(parser, TOKEN_LPAREN)) {
        // Skip to closing paren for now (TODO: parse parameters properly)
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

    // Check if this is a function declaration or definition
    if (parser_match(parser, TOKEN_LBRACE)) {
        // Function definition with body
        func_decl->data.func_decl.body = parser_parse_compound_statement(parser);
        func_decl->data.func_decl.has_body = 1;
        return func_decl;
    } else if (parser_match(parser, TOKEN_SEMICOLON)) {
        // Function declaration without body
        func_decl->data.func_decl.has_body = 0;
        func_decl->data.func_decl.body = NULL;
        return func_decl;
    } else {
        parser_error(parser, "Expected function body or semicolon");
        ast_free(func_decl);  // Free the allocated node before returning NULL
        return NULL;
    }
}

struct ASTNode* parser_parse_compound_statement(ParserContext* parser) {
    if (!parser) return NULL;
    
    if (!parser_expect(parser, TOKEN_LBRACE)) {
        return NULL;
    }
    
    struct ASTNode* compound = parser_create_ast_node(parser, ASTC_COMPOUND_STMT);
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
        case TOKEN_INT:
        case TOKEN_VOID:
        case TOKEN_CHAR:
        case TOKEN_FLOAT:
        case TOKEN_DOUBLE:
        case TOKEN_STRUCT:
        case TOKEN_UNION:
            // These are type specifiers, so this is a variable declaration
            return parser_parse_variable_declaration(parser);
        default:
            return parser_parse_expression_statement(parser);
    }
}

struct ASTNode* parser_parse_expression_statement(ParserContext* parser) {
    if (!parser) return NULL;
    
    struct ASTNode* expr_stmt = parser_create_ast_node(parser, ASTC_EXPR_STMT);
    if (!expr_stmt) return NULL;
    
    // Parse expression (simplified)
    if (parser->current_token->type != TOKEN_SEMICOLON) {
        parser_parse_expression(parser);
    }
    
    parser_expect(parser, TOKEN_SEMICOLON);
    
    return expr_stmt;
}

struct ASTNode* parser_parse_variable_declaration(ParserContext* parser) {
    if (!parser) return NULL;

    struct ASTNode* var_decl = parser_create_ast_node(parser, ASTC_VAR_DECL);
    if (!var_decl) return NULL;

    // Initialize variable declaration data
    var_decl->data.var_decl.name = NULL;
    var_decl->data.var_decl.type = NULL;
    var_decl->data.var_decl.initializer = NULL;

    // Parse storage class specifiers (skip for now, as they're not in the AST structure)
    while (parser->current_token &&
           (parser->current_token->type == TOKEN_STATIC ||
            parser->current_token->type == TOKEN_EXTERN)) {
        // TODO: Store storage class information somewhere
        parser_advance(parser);
    }

    // Parse type specifiers (simplified)
    while (parser->current_token &&
           (parser->current_token->type == TOKEN_INT ||
            parser->current_token->type == TOKEN_VOID ||
            parser->current_token->type == TOKEN_CHAR ||
            parser->current_token->type == TOKEN_FLOAT ||
            parser->current_token->type == TOKEN_DOUBLE ||
            parser->current_token->type == TOKEN_STRUCT ||
            parser->current_token->type == TOKEN_UNION)) {

        // Handle struct/union types
        if (parser->current_token->type == TOKEN_STRUCT ||
            parser->current_token->type == TOKEN_UNION) {
            parser_advance(parser); // consume 'struct' or 'union'

            // Expect struct/union name
            if (parser->current_token && parser->current_token->type == TOKEN_IDENTIFIER) {
                parser_advance(parser); // consume struct/union name
            }
        } else {
            parser_advance(parser);
        }
    }

    // Handle pointer declarators
    while (parser->current_token && parser->current_token->type == TOKEN_MULTIPLY) {
        parser_advance(parser); // consume '*'
    }

    // Parse variable name
    if (parser_match(parser, TOKEN_IDENTIFIER)) {
        var_decl->data.var_decl.name = strdup(parser->current_token->value);
        parser_advance(parser);
    } else {
        parser_error(parser, "Expected variable name");
        ast_free(var_decl);  // Free the allocated node before returning NULL
        return NULL;
    }

    // Handle array declarators
    while (parser->current_token && parser->current_token->type == TOKEN_LBRACKET) {
        parser_advance(parser); // consume '['

        // Parse array size (optional)
        if (parser->current_token && parser->current_token->type != TOKEN_RBRACKET) {
            // For now, just skip the array size expression
            // In a full implementation, we would parse and store it
            while (parser->current_token &&
                   parser->current_token->type != TOKEN_RBRACKET &&
                   parser->current_token->type != TOKEN_EOF) {
                parser_advance(parser);
            }
        }

        if (!parser_expect(parser, TOKEN_RBRACKET)) {
            ast_free(var_decl);
            return NULL;
        }
    }

    // Check for initializer
    if (parser_match(parser, TOKEN_ASSIGN)) {
        parser_advance(parser);
        var_decl->data.var_decl.initializer = parser_parse_expression(parser);
    }

    // Expect semicolon
    if (!parser_expect(parser, TOKEN_SEMICOLON)) {
        parser_error(parser, "Expected semicolon after variable declaration");
        ast_free(var_decl);  // Free the allocated node before returning NULL
        return NULL;
    }

    return var_decl;
}

struct ASTNode* parser_parse_jump_statement(ParserContext* parser) {
    if (!parser || !parser->current_token) return NULL;
    
    if (parser->current_token->type == TOKEN_RETURN) {
        struct ASTNode* return_stmt = parser_create_ast_node(parser, ASTC_RETURN_STMT);
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
    if (!parser) return NULL;

    return parser_parse_assignment_expression(parser);
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
    printf("  Errors: %d\n", parser->error_count);
    printf("  Warnings: %d\n", parser->warning_count);
    printf("  Scope Depth: %d\n", parser->scope_depth);
}

// ===============================================
// Enhanced Expression Parsing Functions
// ===============================================

struct ASTNode* parser_parse_assignment_expression(ParserContext* parser) {
    if (!parser) return NULL;

    struct ASTNode* left = parser_parse_unary_expression(parser);
    if (!left) return NULL;

    // Check for assignment operators
    if (parser->current_token &&
        (parser->current_token->type == TOKEN_ASSIGN ||
         parser->current_token->type == TOKEN_PLUS_ASSIGN ||
         parser->current_token->type == TOKEN_MINUS_ASSIGN ||
         parser->current_token->type == TOKEN_MUL_ASSIGN ||
         parser->current_token->type == TOKEN_DIV_ASSIGN ||
         parser->current_token->type == TOKEN_MOD_ASSIGN ||
         parser->current_token->type == TOKEN_AND_ASSIGN ||
         parser->current_token->type == TOKEN_OR_ASSIGN ||
         parser->current_token->type == TOKEN_XOR_ASSIGN ||
         parser->current_token->type == TOKEN_LSHIFT_ASSIGN ||
         parser->current_token->type == TOKEN_RSHIFT_ASSIGN)) {

        struct ASTNode* assign_expr = parser_create_ast_node(parser, ASTC_BINARY_OP);
        if (!assign_expr) return NULL;

        assign_expr->data.binary_op.left = left;
        assign_expr->data.binary_op.op = parser->current_token->type;
        parser_advance(parser);

        assign_expr->data.binary_op.right = parser_parse_assignment_expression(parser);
        if (!assign_expr->data.binary_op.right) return NULL;

        return assign_expr;
    }

    return left;
}

struct ASTNode* parser_parse_unary_expression(ParserContext* parser) {
    if (!parser || !parser->current_token) return NULL;

    // Check for unary operators
    switch (parser->current_token->type) {
        case TOKEN_BITWISE_AND:     // &var (address-of)
        case TOKEN_MULTIPLY:        // *ptr (dereference)
        case TOKEN_PLUS:           // +expr
        case TOKEN_MINUS:          // -expr
        case TOKEN_LOGICAL_NOT:    // !expr
        case TOKEN_BITWISE_NOT:    // ~expr
        case TOKEN_INCREMENT:      // ++var
        case TOKEN_DECREMENT: {    // --var
            struct ASTNode* unary_expr = parser_create_ast_node(parser, ASTC_UNARY_OP);
            if (!unary_expr) return NULL;

            unary_expr->data.unary_op.op = parser->current_token->type;
            printf("Parser: Found unary operator %d\n", parser->current_token->type);
            parser_advance(parser);

            unary_expr->data.unary_op.operand = parser_parse_unary_expression(parser);
            if (!unary_expr->data.unary_op.operand) {
                ast_free(unary_expr);
                return NULL;
            }

            return unary_expr;
        }

        default:
            return parser_parse_postfix_expression(parser);
    }
}

struct ASTNode* parser_parse_postfix_expression(ParserContext* parser) {
    if (!parser) return NULL;

    struct ASTNode* expr = parser_parse_primary_expression(parser);
    if (!expr) return NULL;

    // Handle postfix operators
    while (parser->current_token) {
        switch (parser->current_token->type) {
            case TOKEN_LBRACKET: {
                // Array subscript: expr[index]
                struct ASTNode* subscript = parser_create_ast_node(parser, ASTC_EXPR_ARRAY_SUBSCRIPT);
                if (!subscript) {
                    ast_free(expr);
                    return NULL;
                }

                subscript->data.array_subscript.array = expr;
                parser_advance(parser); // consume '['

                subscript->data.array_subscript.index = parser_parse_expression(parser);
                if (!subscript->data.array_subscript.index) {
                    ast_free(subscript);
                    return NULL;
                }

                if (!parser_expect(parser, TOKEN_RBRACKET)) {
                    ast_free(subscript);
                    return NULL;
                }

                expr = subscript;
                break;
            }

            case TOKEN_DOT: {
                // Member access: expr.member
                struct ASTNode* member_access = parser_create_ast_node(parser, ASTC_EXPR_MEMBER_ACCESS);
                if (!member_access) {
                    ast_free(expr);
                    return NULL;
                }

                member_access->data.member_access.object = expr;
                parser_advance(parser); // consume '.'

                if (parser->current_token->type != TOKEN_IDENTIFIER) {
                    parser_error(parser, "Expected member name after '.'");
                    ast_free(member_access);
                    return NULL;
                }

                member_access->data.member_access.member = strdup(parser->current_token->value);
                parser_advance(parser);

                expr = member_access;
                break;
            }

            case TOKEN_ARROW: {
                // Pointer member access: expr->member
                struct ASTNode* ptr_access = parser_create_ast_node(parser, ASTC_EXPR_PTR_MEMBER_ACCESS);
                if (!ptr_access) {
                    ast_free(expr);
                    return NULL;
                }

                ptr_access->data.member_access.object = expr;
                parser_advance(parser); // consume '->'

                if (parser->current_token->type != TOKEN_IDENTIFIER) {
                    parser_error(parser, "Expected member name after '->'");
                    ast_free(ptr_access);
                    return NULL;
                }

                ptr_access->data.member_access.member = strdup(parser->current_token->value);
                parser_advance(parser);

                expr = ptr_access;
                break;
            }

            case TOKEN_LPAREN: {
                // Function call: expr(args...)
                struct ASTNode* call_expr = parser_create_ast_node(parser, ASTC_CALL_EXPR);
                if (!call_expr) {
                    ast_free(expr);
                    return NULL;
                }

                call_expr->data.call_expr.callee = expr;
                call_expr->data.call_expr.is_libc_call = false;
                call_expr->data.call_expr.libc_func_id = 0;
                parser_advance(parser); // consume '('

                // Parse arguments
                struct ASTNode** args = NULL;
                int arg_count = 0;
                int arg_capacity = 0;

                if (parser->current_token && parser->current_token->type != TOKEN_RPAREN) {
                    do {
                        // Expand args array if needed
                        if (arg_count >= arg_capacity) {
                            arg_capacity = arg_capacity == 0 ? 4 : arg_capacity * 2;
                            args = realloc(args, arg_capacity * sizeof(struct ASTNode*));
                            if (!args) {
                                ast_free(call_expr);
                                return NULL;
                            }
                        }

                        // Parse argument expression
                        args[arg_count] = parser_parse_assignment_expression(parser);
                        if (!args[arg_count]) {
                            for (int i = 0; i < arg_count; i++) {
                                ast_free(args[i]);
                            }
                            free(args);
                            ast_free(call_expr);
                            return NULL;
                        }
                        arg_count++;

                        // Check for comma
                        if (parser->current_token && parser->current_token->type == TOKEN_COMMA) {
                            parser_advance(parser);
                        } else {
                            break;
                        }
                    } while (parser->current_token && parser->current_token->type != TOKEN_RPAREN);
                }

                call_expr->data.call_expr.args = args;
                call_expr->data.call_expr.arg_count = arg_count;

                if (!parser_expect(parser, TOKEN_RPAREN)) {
                    for (int i = 0; i < arg_count; i++) {
                        ast_free(args[i]);
                    }
                    free(args);
                    ast_free(call_expr);
                    return NULL;
                }

                printf("Parser: Found function call with %d arguments\n", arg_count);
                expr = call_expr;
                break;
            }

            default:
                return expr;
        }
    }

    return expr;
}

struct ASTNode* parser_parse_primary_expression(ParserContext* parser) {
    if (!parser || !parser->current_token) return NULL;

    switch (parser->current_token->type) {
        case TOKEN_IDENTIFIER: {
            struct ASTNode* id_expr = parser_create_ast_node(parser, ASTC_EXPR_IDENTIFIER);
            if (!id_expr) return NULL;

            id_expr->data.identifier.name = strdup(parser->current_token->value);
            printf("Parser: Found identifier '%s'\n", parser->current_token->value);
            parser_advance(parser);
            return id_expr;
        }

        case TOKEN_INTEGER_LITERAL: {
            struct ASTNode* const_expr = parser_create_ast_node(parser, ASTC_EXPR_CONSTANT);
            if (!const_expr) return NULL;

            const_expr->data.constant.type = ASTC_EXPR_CONSTANT;
            const_expr->data.constant.int_val = atoi(parser->current_token->value);
            printf("Parser: Found integer '%s'\n", parser->current_token->value);
            parser_advance(parser);
            return const_expr;
        }

        case TOKEN_FLOAT_LITERAL: {
            struct ASTNode* const_expr = parser_create_ast_node(parser, ASTC_EXPR_CONSTANT);
            if (!const_expr) return NULL;

            const_expr->data.constant.type = ASTC_EXPR_CONSTANT;
            const_expr->data.constant.float_val = atof(parser->current_token->value);
            printf("Parser: Found float '%s'\n", parser->current_token->value);
            parser_advance(parser);
            return const_expr;
        }

        case TOKEN_STRING_LITERAL: {
            struct ASTNode* string_expr = parser_create_ast_node(parser, ASTC_EXPR_STRING_LITERAL);
            if (!string_expr) return NULL;

            string_expr->data.string_literal.value = strdup(parser->current_token->value);
            printf("Parser: Found string literal '%s'\n", parser->current_token->value);
            parser_advance(parser);
            return string_expr;
        }

        case TOKEN_CHAR_LITERAL: {
            struct ASTNode* const_expr = parser_create_ast_node(parser, ASTC_EXPR_CONSTANT);
            if (!const_expr) return NULL;

            const_expr->data.constant.type = ASTC_EXPR_CONSTANT;
            // 简化处理：只取第一个字符（忽略转义）
            const_expr->data.constant.int_val = (int)parser->current_token->value[1]; // skip opening quote
            printf("Parser: Found char literal '%s'\n", parser->current_token->value);
            parser_advance(parser);
            return const_expr;
        }

        case TOKEN_LPAREN: {
            // 这可能是括号表达式或类型转换
            // 先尝试解析为类型转换
            struct ASTNode* cast_expr = parser_try_parse_cast_expression(parser);
            if (cast_expr) {
                return cast_expr;
            }

            // 如果不是类型转换，解析为括号表达式
            parser_advance(parser); // consume '('
            struct ASTNode* expr = parser_parse_expression(parser);
            if (!expr) return NULL;

            if (!parser_expect(parser, TOKEN_RPAREN)) {
                parser_error(parser, "Expected ')' after expression");
                ast_free(expr);  // Free the parsed expression before returning NULL
                return NULL;
            }
            return expr;
        }

        default:
            parser_error(parser, "Expected primary expression");
            return NULL;
    }
}

// ===============================================
// Struct and Union Parsing Functions
// ===============================================

struct ASTNode* parser_parse_struct_declaration(ParserContext* parser) {
    if (!parser || !parser->current_token) return NULL;

    if (!parser_expect(parser, TOKEN_STRUCT)) {
        return NULL;
    }

    struct ASTNode* struct_decl = parser_create_ast_node(parser, ASTC_STRUCT_DECL);
    if (!struct_decl) return NULL;

    printf("Parser: Parsing struct declaration\n");

    // Initialize struct declaration data
    struct_decl->data.struct_decl.name = NULL;
    struct_decl->data.struct_decl.members = NULL;
    struct_decl->data.struct_decl.member_count = 0;

    // Parse optional struct name
    if (parser->current_token && parser->current_token->type == TOKEN_IDENTIFIER) {
        struct_decl->data.struct_decl.name = strdup(parser->current_token->value);
        printf("Parser: Found struct name '%s'\n", parser->current_token->value);
        parser_advance(parser);
    }

    // Parse struct body if present
    if (parser->current_token && parser->current_token->type == TOKEN_LBRACE) {
        parser_advance(parser); // consume '{'

        // Parse struct fields
        while (parser->current_token &&
               parser->current_token->type != TOKEN_RBRACE &&
               parser->current_token->type != TOKEN_EOF) {

            // Parse field declaration (simplified)
            struct ASTNode* field = parser_parse_variable_declaration(parser);
            if (field) {
                // Add field to struct
                struct_decl->data.struct_decl.member_count++;
                struct_decl->data.struct_decl.members = realloc(
                    struct_decl->data.struct_decl.members,
                    sizeof(struct ASTNode*) * struct_decl->data.struct_decl.member_count
                );
                struct_decl->data.struct_decl.members[
                    struct_decl->data.struct_decl.member_count - 1
                ] = field;
            }
        }

        if (!parser_expect(parser, TOKEN_RBRACE)) {
            ast_free(struct_decl);
            return NULL;
        }
    }

    // Expect semicolon after struct declaration
    if (!parser_expect(parser, TOKEN_SEMICOLON)) {
        ast_free(struct_decl);
        return NULL;
    }

    return struct_decl;
}

struct ASTNode* parser_parse_union_declaration(ParserContext* parser) {
    if (!parser || !parser->current_token) return NULL;

    if (!parser_expect(parser, TOKEN_UNION)) {
        return NULL;
    }

    struct ASTNode* union_decl = parser_create_ast_node(parser, ASTC_UNION_DECL);
    if (!union_decl) return NULL;

    printf("Parser: Parsing union declaration\n");

    // Initialize union declaration data (similar to struct)
    union_decl->data.union_decl.name = NULL;
    union_decl->data.union_decl.members = NULL;
    union_decl->data.union_decl.member_count = 0;

    // Parse optional union name
    if (parser->current_token && parser->current_token->type == TOKEN_IDENTIFIER) {
        union_decl->data.union_decl.name = strdup(parser->current_token->value);
        printf("Parser: Found union name '%s'\n", parser->current_token->value);
        parser_advance(parser);
    }

    // Parse union body if present
    if (parser->current_token && parser->current_token->type == TOKEN_LBRACE) {
        parser_advance(parser); // consume '{'

        // Parse union fields
        while (parser->current_token &&
               parser->current_token->type != TOKEN_RBRACE &&
               parser->current_token->type != TOKEN_EOF) {

            // Parse field declaration (simplified)
            struct ASTNode* field = parser_parse_variable_declaration(parser);
            if (field) {
                // Add field to union
                union_decl->data.union_decl.member_count++;
                union_decl->data.union_decl.members = realloc(
                    union_decl->data.union_decl.members,
                    sizeof(struct ASTNode*) * union_decl->data.union_decl.member_count
                );
                union_decl->data.union_decl.members[
                    union_decl->data.union_decl.member_count - 1
                ] = field;
            }
        }

        if (!parser_expect(parser, TOKEN_RBRACE)) {
            ast_free(union_decl);
            return NULL;
        }
    }

    // Expect semicolon after union declaration
    if (!parser_expect(parser, TOKEN_SEMICOLON)) {
        ast_free(union_decl);
        return NULL;
    }

    return union_decl;
}

// ===============================================
// Pointer and Array Parsing Functions
// ===============================================

struct ASTNode* parser_parse_pointer_declarator(ParserContext* parser) {
    if (!parser || !parser->current_token) return NULL;

    if (!parser_match(parser, TOKEN_MULTIPLY)) {
        return NULL;
    }

    struct ASTNode* pointer_decl = parser_create_ast_node(parser, ASTC_POINTER_TYPE);
    if (!pointer_decl) return NULL;

    printf("Parser: Parsing pointer declarator\n");

    // Initialize pointer declaration data
    pointer_decl->data.pointer_type.base_type = NULL;
    pointer_decl->data.pointer_type.pointer_level = 0;

    // Count indirection levels (multiple *)
    while (parser->current_token && parser->current_token->type == TOKEN_MULTIPLY) {
        pointer_decl->data.pointer_type.pointer_level++;
        parser_advance(parser);
    }

    printf("Parser: Found pointer with %d indirection levels\n",
           pointer_decl->data.pointer_type.pointer_level);

    return pointer_decl;
}

struct ASTNode* parser_parse_array_declarator(ParserContext* parser) {
    if (!parser || !parser->current_token) return NULL;

    if (!parser_match(parser, TOKEN_LBRACKET)) {
        return NULL;
    }

    struct ASTNode* array_decl = parser_create_ast_node(parser, ASTC_ARRAY_TYPE);
    if (!array_decl) return NULL;

    printf("Parser: Parsing array declarator\n");

    // Initialize array declaration data
    array_decl->data.array_type.element_type = NULL;
    array_decl->data.array_type.size_expr = NULL;
    array_decl->data.array_type.dimensions = 1;

    parser_advance(parser); // consume '['

    // Parse array size expression (optional)
    if (parser->current_token && parser->current_token->type != TOKEN_RBRACKET) {
        array_decl->data.array_type.size_expr = parser_parse_expression(parser);

        // If it's a constant integer, we can note it but the size_expr holds the info
        if (array_decl->data.array_type.size_expr &&
            array_decl->data.array_type.size_expr->type == ASTC_EXPR_CONSTANT) {
            int size = array_decl->data.array_type.size_expr->data.constant.int_val;
            printf("Parser: Found array size %d\n", size);
        }
    }

    if (!parser_expect(parser, TOKEN_RBRACKET)) {
        ast_free(array_decl);
        return NULL;
    }

    return array_decl;
}

struct ASTNode* parser_parse_type_specifier(ParserContext* parser) {
    if (!parser || !parser->current_token) return NULL;

    struct ASTNode* type_spec = parser_create_ast_node(parser, ASTC_TYPE_SPECIFIER);
    if (!type_spec) return NULL;

    // Initialize type specifier data
    type_spec->data.type_specifier.type = parser->current_token->type;

    // Parse base type
    switch (parser->current_token->type) {
        case TOKEN_INT:
        case TOKEN_VOID:
        case TOKEN_CHAR:
        case TOKEN_FLOAT:
        case TOKEN_DOUBLE:
            printf("Parser: Found base type %s\n",
                   token_type_name(parser->current_token->type));
            parser_advance(parser);
            break;

        case TOKEN_STRUCT:
        case TOKEN_UNION:
            // Handle struct/union types - just record the type for now
            parser_advance(parser);
            if (parser->current_token && parser->current_token->type == TOKEN_IDENTIFIER) {
                // Skip the struct/union name for now
                parser_advance(parser);
            }
            break;

        default:
            parser_error(parser, "Expected type specifier");
            ast_free(type_spec);
            return NULL;
    }

    return type_spec;
}

struct ASTNode* parser_try_parse_cast_expression(ParserContext* parser) {
    if (!parser || !parser->current_token || parser->current_token->type != TOKEN_LPAREN) {
        return NULL;
    }

    // 简化实现：只在明确是类型转换时解析
    // 检查 (type) 模式
    if (parser->lookahead_token &&
        (parser->lookahead_token->type == TOKEN_INT ||
         parser->lookahead_token->type == TOKEN_CHAR ||
         parser->lookahead_token->type == TOKEN_FLOAT ||
         parser->lookahead_token->type == TOKEN_DOUBLE ||
         parser->lookahead_token->type == TOKEN_VOID ||
         parser->lookahead_token->type == TOKEN_STRUCT ||
         parser->lookahead_token->type == TOKEN_UNION)) {

        parser_advance(parser); // consume '('

        // 解析类型说明符
        struct ASTNode* type_node = parser_try_parse_type_specifier(parser);
        if (!type_node) {
            return NULL;
        }

        // 检查右括号
        if (!parser->current_token || parser->current_token->type != TOKEN_RPAREN) {
            ast_free(type_node);
            return NULL;
        }

        parser_advance(parser); // consume ')'

        // 解析被转换的表达式
        struct ASTNode* expr = parser_parse_unary_expression(parser);
        if (!expr) {
            ast_free(type_node);
            return NULL;
        }

        // 创建类型转换节点
        struct ASTNode* cast_node = parser_create_ast_node(parser, ASTC_EXPR_CAST_EXPR);
        if (!cast_node) {
            ast_free(type_node);
            ast_free(expr);
            return NULL;
        }

        cast_node->data.cast_expr.target_type = type_node;
        cast_node->data.cast_expr.expression = expr;

        printf("Parser: Found cast expression\n");
        return cast_node;
    }

    return NULL;
}

struct ASTNode* parser_try_parse_type_specifier(ParserContext* parser) {
    if (!parser || !parser->current_token) return NULL;

    switch (parser->current_token->type) {
        case TOKEN_INT:
        case TOKEN_CHAR:
        case TOKEN_FLOAT:
        case TOKEN_DOUBLE:
        case TOKEN_VOID:
        case TOKEN_SHORT:
        case TOKEN_LONG:
        case TOKEN_SIGNED:
        case TOKEN_UNSIGNED: {
            struct ASTNode* type_node = parser_create_ast_node(parser, ASTC_TYPE_SPECIFIER);
            if (!type_node) return NULL;

            type_node->data.type_specifier.type = parser->current_token->type;
            parser_advance(parser);
            return type_node;
        }

        case TOKEN_STRUCT:
        case TOKEN_UNION: {
            // 简化处理：只支持已定义的结构体/联合体名称
            TokenType struct_or_union = parser->current_token->type;
            parser_advance(parser);

            if (parser->current_token && parser->current_token->type == TOKEN_IDENTIFIER) {
                struct ASTNode* type_node = parser_create_ast_node(parser, ASTC_TYPE_SPECIFIER);
                if (!type_node) return NULL;

                type_node->data.type_specifier.type = struct_or_union;
                parser_advance(parser);
                return type_node;
            }
            return NULL;
        }

        default:
            return NULL;
    }
}
