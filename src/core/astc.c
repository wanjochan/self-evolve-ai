/**
 * astc.c - ASTC Core Implementation
 * 
 * Implements the core ASTC (Abstract Syntax Tree Compiler) functionality
 * including AST node creation, management, and basic operations.
 */

#include "astc.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ===============================================
// AST Node Management
// ===============================================

/**
 * Create a new AST node
 */
ASTNode* ast_create_node(ASTNodeType type, int line, int column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        return NULL;
    }
    
    // Initialize the node
    memset(node, 0, sizeof(ASTNode));
    node->type = type;
    node->line = line;
    node->column = column;
    
    // Initialize type-specific data
    switch (type) {
        case ASTC_FUNC_DECL:
            node->data.func_decl.name = NULL;
            node->data.func_decl.return_type = NULL;
            node->data.func_decl.params = NULL;
            node->data.func_decl.param_count = 0;
            node->data.func_decl.has_body = 0;
            node->data.func_decl.body = NULL;
            break;
            
        case ASTC_VAR_DECL:
            node->data.var_decl.name = NULL;
            node->data.var_decl.type = NULL;
            node->data.var_decl.initializer = NULL;
            break;
            
        case ASTC_BINARY_OP:
            node->data.binary_op.op = ASTC_OP_UNKNOWN;
            node->data.binary_op.left = NULL;
            node->data.binary_op.right = NULL;
            break;

        case ASTC_UNARY_OP:
            node->data.unary_op.op = ASTC_OP_UNKNOWN;
            node->data.unary_op.operand = NULL;
            break;
            
        case ASTC_EXPR_CONSTANT:
            node->data.constant.type = ASTC_TYPE_INVALID;
            node->data.constant.int_val = 0;
            break;
            
        case ASTC_IF_STMT:
            node->data.if_stmt.condition = NULL;
            node->data.if_stmt.then_branch = NULL;
            node->data.if_stmt.else_branch = NULL;
            break;

        case ASTC_WHILE_STMT:
            node->data.while_stmt.condition = NULL;
            node->data.while_stmt.body = NULL;
            break;

        case ASTC_FOR_STMT:
            node->data.for_stmt.init = NULL;
            node->data.for_stmt.condition = NULL;
            node->data.for_stmt.increment = NULL;
            node->data.for_stmt.body = NULL;
            break;
            
        case ASTC_RETURN_STMT:
            node->data.return_stmt.value = NULL;
            break;
            
        case ASTC_PARAM_DECL:
            // PARAM_DECL uses var_decl structure
            node->data.var_decl.name = NULL;
            node->data.var_decl.type = NULL;
            node->data.var_decl.initializer = NULL;
            break;
            
        default:
            // For other node types, the union is already zeroed
            break;
    }
    
    return node;
}

/**
 * Free an AST node and its children
 */
void ast_free(ASTNode* node) {
    if (!node) {
        return;
    }
    
    // Free type-specific data
    switch (node->type) {
        case ASTC_FUNC_DECL:
            if (node->data.func_decl.name) {
                free(node->data.func_decl.name);
            }
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
            if (node->data.var_decl.name) {
                free(node->data.var_decl.name);
            }
            ast_free(node->data.var_decl.type);
            ast_free(node->data.var_decl.initializer);
            break;
            
        case ASTC_BINARY_OP:
            ast_free(node->data.binary_op.left);
            ast_free(node->data.binary_op.right);
            break;
            
        case ASTC_UNARY_OP:
            ast_free(node->data.unary_op.operand);
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
            
        case ASTC_PARAM_DECL:
            if (node->data.var_decl.name) {
                free(node->data.var_decl.name);
            }
            ast_free(node->data.var_decl.type);
            break;
            
        default:
            // For other node types, no special cleanup needed
            break;
    }
    
    // Free the node itself
    free(node);
}

/**
 * Clone an AST node (shallow copy)
 */
ASTNode* ast_clone_node(const ASTNode* node) {
    if (!node) {
        return NULL;
    }
    
    ASTNode* clone = ast_create_node(node->type, node->line, node->column);
    if (!clone) {
        return NULL;
    }
    
    // Copy the data union (shallow copy)
    clone->data = node->data;
    
    return clone;
}

/**
 * Get node type name as string
 */
const char* ast_get_node_type_name(ASTNodeType type) {
    switch (type) {
        case ASTC_TRANSLATION_UNIT: return "TRANSLATION_UNIT";
        case ASTC_FUNC_DECL: return "FUNC_DECL";
        case ASTC_VAR_DECL: return "VAR_DECL";
        case ASTC_PARAM_DECL: return "PARAM_DECL";
        case ASTC_IF_STMT: return "IF_STMT";
        case ASTC_WHILE_STMT: return "WHILE_STMT";
        case ASTC_FOR_STMT: return "FOR_STMT";
        case ASTC_RETURN_STMT: return "RETURN_STMT";
        case ASTC_EXPR_CONSTANT: return "EXPR_CONSTANT";
        case ASTC_BINARY_OP: return "BINARY_OP";
        case ASTC_UNARY_OP: return "UNARY_OP";
        case ASTC_MODULE_DECL: return "MODULE_DECL";
        case ASTC_EXPORT_DECL: return "EXPORT_DECL";
        case ASTC_IMPORT_DECL: return "IMPORT_DECL";
        case ASTC_REQUIRES_DECL: return "REQUIRES_DECL";
        case ASTC_MODULE_ATTRIBUTE: return "MODULE_ATTRIBUTE";
        case ASTC_SYMBOL_REF: return "SYMBOL_REF";
        default: return "UNKNOWN";
    }
}

/**
 * Print AST node information (for debugging)
 */
void ast_print_node(const ASTNode* node, int indent) {
    if (!node) {
        return;
    }
    
    // Print indentation
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
    
    printf("%s (%d:%d)", ast_get_node_type_name(node->type), node->line, node->column);
    
    // Print type-specific information
    switch (node->type) {
        case ASTC_FUNC_DECL:
            if (node->data.func_decl.name) {
                printf(" name='%s'", node->data.func_decl.name);
            }
            printf(" params=%d", node->data.func_decl.param_count);
            break;
            
        case ASTC_VAR_DECL:
            if (node->data.var_decl.name) {
                printf(" name='%s'", node->data.var_decl.name);
            }
            printf(" type=%p", node->data.var_decl.type);
            break;
            
        case ASTC_EXPR_CONSTANT:
            printf(" type=%d value=%lld", node->data.constant.type, (long long)node->data.constant.int_val);
            break;
            
        case ASTC_BINARY_OP:
            printf(" op=%d", node->data.binary_op.op);
            break;

        case ASTC_UNARY_OP:
            printf(" op=%d", node->data.unary_op.op);
            break;
            
        default:
            break;
    }
    
    printf("\n");
}

/**
 * Validate AST node structure
 */
int ast_validate_node(const ASTNode* node) {
    if (!node) {
        return 0; // NULL nodes are valid in some contexts
    }
    
    // Check basic node properties
    if (node->type < 0) {
        return -1; // Invalid node type
    }
    
    if (node->line < 0 || node->column < 0) {
        return -1; // Invalid position
    }
    
    // Type-specific validation
    switch (node->type) {
        case ASTC_FUNC_DECL:
            if (node->data.func_decl.param_count < 0) {
                return -1;
            }
            if (node->data.func_decl.param_count > 0 && !node->data.func_decl.params) {
                return -1;
            }
            break;
            
        case ASTC_BINARY_OP:
            if (!node->data.binary_op.left || !node->data.binary_op.right) {
                return -1; // Binary operations need both operands
            }
            break;
            
        case ASTC_UNARY_OP:
            if (!node->data.unary_op.operand) {
                return -1; // Unary operations need an operand
            }
            break;
            
        default:
            break;
    }
    
    return 0; // Valid
}
