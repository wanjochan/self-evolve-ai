/**
 * module_parser.h - Module Attribute Parser Header
 * 
 * Header file for module attribute parsing functionality
 */

#ifndef MODULE_PARSER_H
#define MODULE_PARSER_H

#include "../core/include/core_astc.h"

// ===============================================
// Function Declarations
// ===============================================

/**
 * Check if a declaration has module attributes
 * @param node The AST node to check
 * @return 1 if has module attributes, 0 otherwise
 */
int has_module_attributes(struct ASTNode* node);

/**
 * Process module attributes on a declaration and create appropriate AST nodes
 * @param declaration The declaration with attributes
 * @param module_nodes Array to store created module nodes (must be pre-allocated)
 * @param node_count Pointer to store the number of created nodes
 * @return 0 on success, -1 on error
 */
int process_module_attributes(struct ASTNode* declaration, struct ASTNode** module_nodes, int* node_count);

/**
 * Validate module attribute combinations
 * @param declaration The declaration to validate
 * @return 0 if valid, -1 if invalid combination
 */
int validate_module_attribute_combinations(struct ASTNode* declaration);

/**
 * Parse a single annotate attribute string
 * @param annotate_str The attribute string to parse
 * @param attr_name Output buffer for attribute name
 * @param attr_value Output buffer for attribute value
 * @param name_size Size of name buffer
 * @param value_size Size of value buffer
 * @return 0 on success, -1 on error
 */
int parse_single_attribute(const char* annotate_str, char* attr_name, char* attr_value, 
                          size_t name_size, size_t value_size);

#endif // MODULE_PARSER_H
