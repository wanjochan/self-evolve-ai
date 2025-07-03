/**
 * module_parser.c - Module Attribute Parser
 * 
 * Parses __attribute__((annotate(...))) annotations for module system
 * and generates corresponding AST nodes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../core/include/core_astc.h"
#include "../core/include/module_attributes.h"

// ===============================================
// Attribute Parsing Structures
// ===============================================

typedef struct {
    char* name;
    char* value;
} AttributePair;

typedef struct {
    AttributePair* pairs;
    int count;
    int capacity;
} AttributeList;

// ===============================================
// Utility Functions
// ===============================================

static char* duplicate_string(const char* str) {
    if (!str) return NULL;
    
    size_t len = strlen(str);
    char* dup = malloc(len + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

static void trim_whitespace(char* str) {
    if (!str) return;
    
    // Trim leading whitespace
    char* start = str;
    while (isspace(*start)) start++;
    
    // Trim trailing whitespace
    char* end = start + strlen(start) - 1;
    while (end > start && isspace(*end)) end--;
    
    // Move trimmed string to beginning
    size_t len = end - start + 1;
    memmove(str, start, len);
    str[len] = '\0';
}

static void remove_quotes(char* str) {
    if (!str) return;
    
    size_t len = strlen(str);
    if (len >= 2 && str[0] == '"' && str[len-1] == '"') {
        memmove(str, str + 1, len - 2);
        str[len - 2] = '\0';
    }
}

// ===============================================
// Attribute List Management
// ===============================================

static AttributeList* create_attribute_list(void) {
    AttributeList* list = malloc(sizeof(AttributeList));
    if (!list) return NULL;
    
    list->pairs = NULL;
    list->count = 0;
    list->capacity = 0;
    return list;
}

static void free_attribute_list(AttributeList* list) {
    if (!list) return;
    
    for (int i = 0; i < list->count; i++) {
        free(list->pairs[i].name);
        free(list->pairs[i].value);
    }
    free(list->pairs);
    free(list);
}

static int add_attribute_pair(AttributeList* list, const char* name, const char* value) {
    if (!list || !name) return -1;
    
    // Expand capacity if needed
    if (list->count >= list->capacity) {
        int new_capacity = list->capacity == 0 ? 4 : list->capacity * 2;
        AttributePair* new_pairs = realloc(list->pairs, new_capacity * sizeof(AttributePair));
        if (!new_pairs) return -1;
        
        list->pairs = new_pairs;
        list->capacity = new_capacity;
    }
    
    // Add new pair
    list->pairs[list->count].name = duplicate_string(name);
    list->pairs[list->count].value = duplicate_string(value);
    
    if (!list->pairs[list->count].name) {
        free(list->pairs[list->count].value);
        return -1;
    }
    
    list->count++;
    return 0;
}

// ===============================================
// Attribute Parsing Functions
// ===============================================

/**
 * Parse annotate attribute string
 * Format: "module:math" or "export" or "import:libc" or "version:1.0.0"
 */
static AttributeList* parse_annotate_string(const char* annotate_str) {
    if (!annotate_str) return NULL;
    
    AttributeList* list = create_attribute_list();
    if (!list) return NULL;
    
    char* str_copy = duplicate_string(annotate_str);
    if (!str_copy) {
        free_attribute_list(list);
        return NULL;
    }
    
    trim_whitespace(str_copy);
    remove_quotes(str_copy);
    
    // Split by colon
    char* colon = strchr(str_copy, ':');
    if (colon) {
        *colon = '\0';
        char* name = str_copy;
        char* value = colon + 1;
        
        trim_whitespace(name);
        trim_whitespace(value);
        
        if (add_attribute_pair(list, name, value) != 0) {
            free_attribute_list(list);
            free(str_copy);
            return NULL;
        }
    } else {
        // No colon, treat as simple attribute
        if (add_attribute_pair(list, str_copy, NULL) != 0) {
            free_attribute_list(list);
            free(str_copy);
            return NULL;
        }
    }
    
    free(str_copy);
    return list;
}

/**
 * Check if a declaration has module attributes
 */
int has_module_attributes(struct ASTNode* node) {
    // This would be implemented by checking for __attribute__((annotate(...)))
    // in the actual parser. For now, we'll simulate it.
    return 0; // Placeholder
}

/**
 * Extract module attributes from a declaration
 */
AttributeList* extract_module_attributes(struct ASTNode* node) {
    // This would extract actual __attribute__((annotate(...))) from the AST
    // For now, we'll return NULL as a placeholder
    return NULL;
}

// ===============================================
// AST Node Creation Functions
// ===============================================

struct ASTNode* create_module_from_attributes(AttributeList* attrs, struct ASTNode* target) {
    if (!attrs || attrs->count == 0) return NULL;
    
    // Look for module attribute
    for (int i = 0; i < attrs->count; i++) {
        if (strcmp(attrs->pairs[i].name, "module") == 0) {
            const char* module_name = attrs->pairs[i].value;
            if (!module_name) continue;
            
            struct ASTNode* module = ast_create_node(ASTC_MODULE_DECL, 0, 0);
            if (!module) return NULL;
            
            module->data.module_decl.name = duplicate_string(module_name);
            module->data.module_decl.version = NULL;
            module->data.module_decl.author = NULL;
            module->data.module_decl.description = NULL;
            module->data.module_decl.license = NULL;
            module->data.module_decl.declarations = NULL;
            module->data.module_decl.declaration_count = 0;
            module->data.module_decl.exports = NULL;
            module->data.module_decl.export_count = 0;
            module->data.module_decl.imports = NULL;
            module->data.module_decl.import_count = 0;
            module->data.module_decl.init_func = target;
            module->data.module_decl.cleanup_func = NULL;
            
            // Look for additional attributes
            for (int j = 0; j < attrs->count; j++) {
                if (strcmp(attrs->pairs[j].name, "version") == 0) {
                    module->data.module_decl.version = duplicate_string(attrs->pairs[j].value);
                } else if (strcmp(attrs->pairs[j].name, "author") == 0) {
                    module->data.module_decl.author = duplicate_string(attrs->pairs[j].value);
                } else if (strcmp(attrs->pairs[j].name, "description") == 0) {
                    module->data.module_decl.description = duplicate_string(attrs->pairs[j].value);
                } else if (strcmp(attrs->pairs[j].name, "license") == 0) {
                    module->data.module_decl.license = duplicate_string(attrs->pairs[j].value);
                }
            }
            
            return module;
        }
    }
    
    return NULL;
}

struct ASTNode* create_export_from_attributes(AttributeList* attrs, struct ASTNode* target) {
    if (!attrs || attrs->count == 0 || !target) return NULL;
    
    // Look for export attribute
    for (int i = 0; i < attrs->count; i++) {
        if (strcmp(attrs->pairs[i].name, "export") == 0) {
            struct ASTNode* export_node = ast_create_node(ASTC_EXPORT_DECL, 0, 0);
            if (!export_node) return NULL;
            
            // Determine export name from target declaration
            const char* export_name = NULL;
            if (target->type == ASTC_FUNC_DECL) {
                export_name = target->data.func_decl.name;
            } else if (target->type == ASTC_VAR_DECL) {
                export_name = target->data.var_decl.name;
            }
            
            if (!export_name) {
                ast_free(export_node);
                return NULL;
            }
            
            export_node->data.export_decl.name = duplicate_string(export_name);
            export_node->data.export_decl.alias = NULL;
            export_node->data.export_decl.export_type = target->type;
            export_node->data.export_decl.declaration = target;
            export_node->data.export_decl.is_default = false;
            export_node->data.export_decl.flags = 0;
            
            // Check for export type specification
            const char* export_type = attrs->pairs[i].value;
            if (export_type) {
                if (strcmp(export_type, "function") == 0) {
                    export_node->data.export_decl.export_type = ASTC_FUNC_DECL;
                } else if (strcmp(export_type, "variable") == 0) {
                    export_node->data.export_decl.export_type = ASTC_VAR_DECL;
                } else if (strcmp(export_type, "constant") == 0) {
                    export_node->data.export_decl.export_type = ASTC_VAR_DECL; // Constants are variables
                }
            }
            
            return export_node;
        }
    }
    
    return NULL;
}

struct ASTNode* create_import_from_attributes(AttributeList* attrs, struct ASTNode* target) {
    if (!attrs || attrs->count == 0 || !target) return NULL;
    
    // Look for import attribute
    for (int i = 0; i < attrs->count; i++) {
        if (strcmp(attrs->pairs[i].name, "import") == 0) {
            const char* module_name = attrs->pairs[i].value;
            if (!module_name) continue;
            
            struct ASTNode* import_node = ast_create_node(ASTC_IMPORT_DECL, 0, 0);
            if (!import_node) return NULL;
            
            // Determine import name from target declaration
            const char* import_name = NULL;
            if (target->type == ASTC_FUNC_DECL) {
                import_name = target->data.func_decl.name;
            } else if (target->type == ASTC_VAR_DECL) {
                import_name = target->data.var_decl.name;
            }
            
            if (!import_name) {
                ast_free(import_node);
                return NULL;
            }
            
            import_node->data.import_decl.module_name = duplicate_string(module_name);
            import_node->data.import_decl.import_name = duplicate_string(import_name);
            import_node->data.import_decl.local_name = NULL;
            import_node->data.import_decl.version_requirement = NULL;
            import_node->data.import_decl.import_type = target->type;
            import_node->data.import_decl.is_weak = false;
            import_node->data.import_decl.is_lazy = false;
            import_node->data.import_decl.declaration = target;
            
            // Check for weak or lazy import
            for (int j = 0; j < attrs->count; j++) {
                if (strcmp(attrs->pairs[j].name, "import") == 0 && 
                    attrs->pairs[j].value && strstr(attrs->pairs[j].value, "weak:") == attrs->pairs[j].value) {
                    import_node->data.import_decl.is_weak = true;
                } else if (strcmp(attrs->pairs[j].name, "import") == 0 && 
                          attrs->pairs[j].value && strstr(attrs->pairs[j].value, "lazy:") == attrs->pairs[j].value) {
                    import_node->data.import_decl.is_lazy = true;
                }
            }
            
            return import_node;
        }
    }
    
    return NULL;
}

// ===============================================
// Main Parsing Interface
// ===============================================

/**
 * Process module attributes on a declaration and create appropriate AST nodes
 */
int process_module_attributes(struct ASTNode* declaration, struct ASTNode** module_nodes, int* node_count) {
    if (!declaration || !module_nodes || !node_count) return -1;
    
    *node_count = 0;
    
    // Extract attributes from declaration
    AttributeList* attrs = extract_module_attributes(declaration);
    if (!attrs) return 0; // No attributes found
    
    // Create module nodes based on attributes
    struct ASTNode* module = create_module_from_attributes(attrs, declaration);
    if (module) {
        module_nodes[(*node_count)++] = module;
    }
    
    struct ASTNode* export_node = create_export_from_attributes(attrs, declaration);
    if (export_node) {
        module_nodes[(*node_count)++] = export_node;
    }
    
    struct ASTNode* import_node = create_import_from_attributes(attrs, declaration);
    if (import_node) {
        module_nodes[(*node_count)++] = import_node;
    }
    
    free_attribute_list(attrs);
    return 0;
}
