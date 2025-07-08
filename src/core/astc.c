/**
 * astc.c - ASTC Core Implementation
 * 
 * Core implementation of ASTC (Abstract Syntax Tree Code) system
 * providing AST node creation, management, and basic operations.
 */

#include "astc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ===============================================
// AST Node Creation and Management
// ===============================================

/**
 * Create a new AST node
 */
struct ASTNode* ast_create_node(ASTNodeType type, int line, int column) {
    struct ASTNode* node = malloc(sizeof(struct ASTNode));
    if (!node) return NULL;

    memset(node, 0, sizeof(struct ASTNode));
    node->type = type;
    node->line = line;
    node->column = column;

    return node;
}

/**
 * Free an AST node and all its children
 */
void ast_free(struct ASTNode* node) {
    if (!node) return;

    // Free node-specific data based on type
    switch (node->type) {
        case ASTC_TRANSLATION_UNIT:
            if (node->data.translation_unit.declarations) {
                for (int i = 0; i < node->data.translation_unit.declaration_count; i++) {
                    ast_free(node->data.translation_unit.declarations[i]);
                }
                free(node->data.translation_unit.declarations);
            }
            break;

        case ASTC_FUNC_DECL:
            free(node->data.func_decl.name);
            ast_free(node->data.func_decl.return_type);
            if (node->data.func_decl.params) {
                for (int i = 0; i < node->data.func_decl.param_count; i++) {
                    ast_free(node->data.func_decl.params[i]);
                }
                free(node->data.func_decl.params);
            }
            ast_free(node->data.func_decl.body);
            break;

        case ASTC_VAR_DECL:
            free(node->data.var_decl.name);
            ast_free(node->data.var_decl.type);
            ast_free(node->data.var_decl.initializer);
            break;

        case ASTC_COMPOUND_STMT:
            if (node->data.compound_stmt.statements) {
                for (int i = 0; i < node->data.compound_stmt.statement_count; i++) {
                    ast_free(node->data.compound_stmt.statements[i]);
                }
                free(node->data.compound_stmt.statements);
            }
            break;

        case ASTC_IF_STMT:
            ast_free(node->data.if_stmt.condition);
            ast_free(node->data.if_stmt.then_branch);
            ast_free(node->data.if_stmt.else_branch);
            break;

        case ASTC_WHILE_STMT:
            ast_free(node->data.while_stmt.condition);
            ast_free(node->data.while_stmt.body);
            break;

        case ASTC_FOR_STMT:
            ast_free(node->data.for_stmt.init);
            ast_free(node->data.for_stmt.condition);
            ast_free(node->data.for_stmt.increment);
            ast_free(node->data.for_stmt.body);
            break;

        case ASTC_RETURN_STMT:
            ast_free(node->data.return_stmt.value);
            break;

        case ASTC_EXPR_STMT:
            ast_free(node->data.expr_stmt.expr);
            break;

        case ASTC_BINARY_OP:
            ast_free(node->data.binary_op.left);
            ast_free(node->data.binary_op.right);
            break;

        case ASTC_UNARY_OP:
            ast_free(node->data.unary_op.operand);
            break;

        case ASTC_CALL_EXPR:
            ast_free(node->data.call_expr.callee);
            if (node->data.call_expr.args) {
                for (int i = 0; i < node->data.call_expr.arg_count; i++) {
                    ast_free(node->data.call_expr.args[i]);
                }
                free(node->data.call_expr.args);
            }
            break;

        case ASTC_EXPR_IDENTIFIER:
            free(node->data.identifier.name);
            break;

        case ASTC_EXPR_STRING_LITERAL:
            free(node->data.string_literal.value);
            break;

        case ASTC_MODULE_DECL:
            free(node->data.module_decl.name);
            free(node->data.module_decl.version);
            free(node->data.module_decl.author);
            free(node->data.module_decl.description);
            free(node->data.module_decl.license);
            if (node->data.module_decl.declarations) {
                for (int i = 0; i < node->data.module_decl.declaration_count; i++) {
                    ast_free(node->data.module_decl.declarations[i]);
                }
                free(node->data.module_decl.declarations);
            }
            if (node->data.module_decl.exports) {
                for (int i = 0; i < node->data.module_decl.export_count; i++) {
                    ast_free(node->data.module_decl.exports[i]);
                }
                free(node->data.module_decl.exports);
            }
            if (node->data.module_decl.imports) {
                for (int i = 0; i < node->data.module_decl.import_count; i++) {
                    ast_free(node->data.module_decl.imports[i]);
                }
                free(node->data.module_decl.imports);
            }
            ast_free(node->data.module_decl.init_func);
            ast_free(node->data.module_decl.cleanup_func);
            break;

        case ASTC_EXPORT_DECL:
            free(node->data.export_decl.name);
            free(node->data.export_decl.alias);
            ast_free(node->data.export_decl.declaration);
            break;

        case ASTC_IMPORT_DECL:
            free(node->data.import_decl.module_name);
            free(node->data.import_decl.import_name);
            free(node->data.import_decl.local_name);
            free(node->data.import_decl.version_requirement);
            ast_free(node->data.import_decl.declaration);
            break;

        case ASTC_SYMBOL_REF:
            free(node->data.symbol_ref.module_name);
            free(node->data.symbol_ref.symbol_name);
            break;

        default:
            // For other node types, no special cleanup needed
            break;
    }

    free(node);
}

/**
 * Print AST node for debugging
 */
void ast_print(struct ASTNode* node, int indent) {
    if (!node) return;

    // Print indentation
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }

    // Print node type and basic info
    printf("Node: type=%d, line=%d, col=%d", node->type, node->line, node->column);

    // Print type-specific information
    switch (node->type) {
        case ASTC_FUNC_DECL:
            if (node->data.func_decl.name) {
                printf(", name=%s", node->data.func_decl.name);
            }
            break;

        case ASTC_VAR_DECL:
            if (node->data.var_decl.name) {
                printf(", name=%s", node->data.var_decl.name);
            }
            break;

        case ASTC_EXPR_IDENTIFIER:
            if (node->data.identifier.name) {
                printf(", name=%s", node->data.identifier.name);
            }
            break;

        case ASTC_EXPR_STRING_LITERAL:
            if (node->data.string_literal.value) {
                printf(", value=\"%s\"", node->data.string_literal.value);
            }
            break;

        case ASTC_EXPR_CONSTANT:
            printf(", type=%d", node->data.constant.type);
            if (node->data.constant.type == ASTC_TYPE_INT) {
                printf(", value=%lld", node->data.constant.int_val);
            } else if (node->data.constant.type == ASTC_TYPE_FLOAT) {
                printf(", value=%f", node->data.constant.float_val);
            }
            break;

        default:
            break;
    }

    printf("\n");

    // Print children based on node type
    switch (node->type) {
        case ASTC_TRANSLATION_UNIT:
            for (int i = 0; i < node->data.translation_unit.declaration_count; i++) {
                ast_print(node->data.translation_unit.declarations[i], indent + 1);
            }
            break;

        case ASTC_FUNC_DECL:
            if (node->data.func_decl.return_type) {
                ast_print(node->data.func_decl.return_type, indent + 1);
            }
            for (int i = 0; i < node->data.func_decl.param_count; i++) {
                ast_print(node->data.func_decl.params[i], indent + 1);
            }
            if (node->data.func_decl.body) {
                ast_print(node->data.func_decl.body, indent + 1);
            }
            break;

        case ASTC_COMPOUND_STMT:
            for (int i = 0; i < node->data.compound_stmt.statement_count; i++) {
                ast_print(node->data.compound_stmt.statements[i], indent + 1);
            }
            break;

        case ASTC_BINARY_OP:
            ast_print(node->data.binary_op.left, indent + 1);
            ast_print(node->data.binary_op.right, indent + 1);
            break;

        case ASTC_UNARY_OP:
            ast_print(node->data.unary_op.operand, indent + 1);
            break;

        default:
            // For other types, print any child nodes if they exist
            break;
    }
}

// ===============================================
// Token Management (for compatibility)
// ===============================================

/**
 * Free a token (compatibility function)
 */
void token_free(void* token) {
    if (token) {
        // Assume token has a value field that needs to be freed
        struct { int type; char* value; } *t = (struct { int type; char* value; } *)token;
        if (t->value) {
            free(t->value);
        }
        free(token);
    }
}
