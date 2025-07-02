/**
 * attribute_combination_rules.c - Module Attribute Combination Rules
 * 
 * Implements validation rules for module attribute combinations and
 * ensures proper usage of MODULE, EXPORT, IMPORT attributes.
 */

#include "../include/module_attributes.h"
#include "../include/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// ===============================================
// Attribute Combination Rules
// ===============================================

typedef enum {
    ATTR_MODULE,
    ATTR_EXPORT,
    ATTR_IMPORT,
    ATTR_PRIVATE,
    ATTR_INIT,
    ATTR_CLEANUP,
    ATTR_VERSION,
    ATTR_REQUIRES,
    ATTR_EXPORT_FUNC,
    ATTR_EXPORT_VAR,
    ATTR_EXPORT_CONST,
    ATTR_EXPORT_TYPE,
    ATTR_IMPORT_WEAK,
    ATTR_IMPORT_LAZY,
    ATTR_AUTHOR,
    ATTR_DESCRIPTION,
    ATTR_LICENSE,
    ATTR_COUNT
} AttributeType;

typedef struct {
    AttributeType attr1;
    AttributeType attr2;
    bool is_compatible;
    const char* error_message;
} AttributeCompatibilityRule;

// Compatibility rules matrix
static const AttributeCompatibilityRule compatibility_rules[] = {
    // EXPORT and IMPORT are mutually exclusive
    {ATTR_EXPORT, ATTR_IMPORT, false, "EXPORT and IMPORT cannot be used together"},
    {ATTR_EXPORT_FUNC, ATTR_IMPORT, false, "EXPORT_FUNC and IMPORT cannot be used together"},
    {ATTR_EXPORT_VAR, ATTR_IMPORT, false, "EXPORT_VAR and IMPORT cannot be used together"},
    {ATTR_EXPORT_CONST, ATTR_IMPORT, false, "EXPORT_CONST and IMPORT cannot be used together"},
    {ATTR_EXPORT_TYPE, ATTR_IMPORT, false, "EXPORT_TYPE and IMPORT cannot be used together"},
    
    // EXPORT and PRIVATE are mutually exclusive
    {ATTR_EXPORT, ATTR_PRIVATE, false, "EXPORT and PRIVATE cannot be used together"},
    {ATTR_EXPORT_FUNC, ATTR_PRIVATE, false, "EXPORT_FUNC and PRIVATE cannot be used together"},
    {ATTR_EXPORT_VAR, ATTR_PRIVATE, false, "EXPORT_VAR and PRIVATE cannot be used together"},
    {ATTR_EXPORT_CONST, ATTR_PRIVATE, false, "EXPORT_CONST and PRIVATE cannot be used together"},
    {ATTR_EXPORT_TYPE, ATTR_PRIVATE, false, "EXPORT_TYPE and PRIVATE cannot be used together"},
    
    // Multiple export types are mutually exclusive
    {ATTR_EXPORT_FUNC, ATTR_EXPORT_VAR, false, "Multiple export types cannot be used together"},
    {ATTR_EXPORT_FUNC, ATTR_EXPORT_CONST, false, "Multiple export types cannot be used together"},
    {ATTR_EXPORT_FUNC, ATTR_EXPORT_TYPE, false, "Multiple export types cannot be used together"},
    {ATTR_EXPORT_VAR, ATTR_EXPORT_CONST, false, "Multiple export types cannot be used together"},
    {ATTR_EXPORT_VAR, ATTR_EXPORT_TYPE, false, "Multiple export types cannot be used together"},
    {ATTR_EXPORT_CONST, ATTR_EXPORT_TYPE, false, "Multiple export types cannot be used together"},
    
    // IMPORT variants are mutually exclusive
    {ATTR_IMPORT, ATTR_IMPORT_WEAK, false, "IMPORT and IMPORT_WEAK cannot be used together"},
    {ATTR_IMPORT, ATTR_IMPORT_LAZY, false, "IMPORT and IMPORT_LAZY cannot be used together"},
    {ATTR_IMPORT_WEAK, ATTR_IMPORT_LAZY, false, "IMPORT_WEAK and IMPORT_LAZY cannot be used together"},
    
    // Compatible combinations
    {ATTR_MODULE, ATTR_VERSION, true, NULL},
    {ATTR_MODULE, ATTR_AUTHOR, true, NULL},
    {ATTR_MODULE, ATTR_DESCRIPTION, true, NULL},
    {ATTR_MODULE, ATTR_LICENSE, true, NULL},
    {ATTR_VERSION, ATTR_AUTHOR, true, NULL},
    {ATTR_VERSION, ATTR_DESCRIPTION, true, NULL},
    {ATTR_VERSION, ATTR_LICENSE, true, NULL},
    {ATTR_AUTHOR, ATTR_DESCRIPTION, true, NULL},
    {ATTR_AUTHOR, ATTR_LICENSE, true, NULL},
    {ATTR_DESCRIPTION, ATTR_LICENSE, true, NULL},
    {ATTR_REQUIRES, ATTR_IMPORT, true, NULL},
    {ATTR_REQUIRES, ATTR_IMPORT_WEAK, true, NULL},
    {ATTR_REQUIRES, ATTR_IMPORT_LAZY, true, NULL},
};

static const int num_compatibility_rules = sizeof(compatibility_rules) / sizeof(AttributeCompatibilityRule);

// ===============================================
// Attribute Validation Functions
// ===============================================

/**
 * Check if two attributes are compatible
 */
bool are_attributes_compatible(AttributeType attr1, AttributeType attr2) {
    if (attr1 == attr2) {
        return true; // Same attribute is always compatible with itself
    }
    
    for (int i = 0; i < num_compatibility_rules; i++) {
        const AttributeCompatibilityRule* rule = &compatibility_rules[i];
        if ((rule->attr1 == attr1 && rule->attr2 == attr2) ||
            (rule->attr1 == attr2 && rule->attr2 == attr1)) {
            return rule->is_compatible;
        }
    }
    
    // If no rule found, assume compatible
    return true;
}

/**
 * Get error message for incompatible attributes
 */
const char* get_compatibility_error(AttributeType attr1, AttributeType attr2) {
    for (int i = 0; i < num_compatibility_rules; i++) {
        const AttributeCompatibilityRule* rule = &compatibility_rules[i];
        if ((rule->attr1 == attr1 && rule->attr2 == attr2) ||
            (rule->attr1 == attr2 && rule->attr2 == attr1)) {
            return rule->error_message;
        }
    }
    return "Unknown compatibility error";
}

/**
 * Parse attribute string to type
 */
AttributeType parse_attribute_type(const char* attr_str) {
    if (!attr_str) return ATTR_COUNT;
    
    if (strstr(attr_str, "module:")) return ATTR_MODULE;
    if (strcmp(attr_str, "export") == 0) return ATTR_EXPORT;
    if (strstr(attr_str, "import:")) return ATTR_IMPORT;
    if (strcmp(attr_str, "private") == 0) return ATTR_PRIVATE;
    if (strcmp(attr_str, "init") == 0) return ATTR_INIT;
    if (strcmp(attr_str, "cleanup") == 0) return ATTR_CLEANUP;
    if (strstr(attr_str, "version:")) return ATTR_VERSION;
    if (strstr(attr_str, "requires:")) return ATTR_REQUIRES;
    if (strcmp(attr_str, "export:function") == 0) return ATTR_EXPORT_FUNC;
    if (strcmp(attr_str, "export:variable") == 0) return ATTR_EXPORT_VAR;
    if (strcmp(attr_str, "export:constant") == 0) return ATTR_EXPORT_CONST;
    if (strcmp(attr_str, "export:type") == 0) return ATTR_EXPORT_TYPE;
    if (strstr(attr_str, "import:weak:")) return ATTR_IMPORT_WEAK;
    if (strstr(attr_str, "import:lazy:")) return ATTR_IMPORT_LAZY;
    if (strstr(attr_str, "author:")) return ATTR_AUTHOR;
    if (strstr(attr_str, "description:")) return ATTR_DESCRIPTION;
    if (strstr(attr_str, "license:")) return ATTR_LICENSE;
    
    return ATTR_COUNT; // Unknown attribute
}

/**
 * Validate attribute combination on a declaration
 */
int validate_attribute_combination(const char** attributes, int attr_count, char* error_buffer, size_t buffer_size) {
    if (!attributes || attr_count <= 0) {
        return 0; // No attributes to validate
    }
    
    AttributeType attr_types[32]; // Support up to 32 attributes
    int valid_attr_count = 0;
    
    // Parse all attributes
    for (int i = 0; i < attr_count && i < 32; i++) {
        AttributeType type = parse_attribute_type(attributes[i]);
        if (type != ATTR_COUNT) {
            attr_types[valid_attr_count++] = type;
        }
    }
    
    // Check all pairs for compatibility
    for (int i = 0; i < valid_attr_count; i++) {
        for (int j = i + 1; j < valid_attr_count; j++) {
            if (!are_attributes_compatible(attr_types[i], attr_types[j])) {
                const char* error_msg = get_compatibility_error(attr_types[i], attr_types[j]);
                if (error_buffer && buffer_size > 0) {
                    snprintf(error_buffer, buffer_size, "%s", error_msg);
                }
                return -1; // Incompatible combination found
            }
        }
    }
    
    return 0; // All combinations are valid
}

/**
 * Validate module-specific rules
 */
int validate_module_specific_rules(const char** attributes, int attr_count, char* error_buffer, size_t buffer_size) {
    bool has_module = false;
    bool has_export = false;
    bool has_import = false;
    bool has_version = false;
    
    // Check for presence of key attributes
    for (int i = 0; i < attr_count; i++) {
        AttributeType type = parse_attribute_type(attributes[i]);
        switch (type) {
            case ATTR_MODULE:
                has_module = true;
                break;
            case ATTR_EXPORT:
            case ATTR_EXPORT_FUNC:
            case ATTR_EXPORT_VAR:
            case ATTR_EXPORT_CONST:
            case ATTR_EXPORT_TYPE:
                has_export = true;
                break;
            case ATTR_IMPORT:
            case ATTR_IMPORT_WEAK:
            case ATTR_IMPORT_LAZY:
                has_import = true;
                break;
            case ATTR_VERSION:
                has_version = true;
                break;
            default:
                break;
        }
    }
    
    // Rule: MODULE declarations should have VERSION
    if (has_module && !has_version) {
        if (error_buffer && buffer_size > 0) {
            snprintf(error_buffer, buffer_size, "MODULE declarations should include VERSION attribute");
        }
        // This is a warning, not an error
        LOG_MODULE_WARN("MODULE declaration without VERSION attribute");
    }
    
    // Rule: EXPORT and IMPORT cannot coexist (already checked in compatibility rules)
    if (has_export && has_import) {
        if (error_buffer && buffer_size > 0) {
            snprintf(error_buffer, buffer_size, "Declaration cannot have both EXPORT and IMPORT attributes");
        }
        return -1;
    }
    
    return 0;
}

/**
 * Complete attribute validation (combination + module-specific rules)
 */
int validate_complete_attribute_set(const char** attributes, int attr_count, char* error_buffer, size_t buffer_size) {
    // First check basic compatibility
    if (validate_attribute_combination(attributes, attr_count, error_buffer, buffer_size) != 0) {
        return -1;
    }
    
    // Then check module-specific rules
    if (validate_module_specific_rules(attributes, attr_count, error_buffer, buffer_size) != 0) {
        return -1;
    }
    
    return 0;
}

/**
 * Test function for attribute combination rules
 */
int test_attribute_combination_rules(void) {
    printf("=== Testing Attribute Combination Rules ===\n");
    
    // Test 1: Valid combination
    const char* valid_attrs[] = {"module:math", "version:1.0.0", "author:Test"};
    char error_buf[256];
    int result = validate_complete_attribute_set(valid_attrs, 3, error_buf, sizeof(error_buf));
    printf("Test 1 (Valid combination): %s\n", result == 0 ? "PASS" : "FAIL");
    
    // Test 2: Invalid combination (EXPORT + IMPORT)
    const char* invalid_attrs[] = {"export", "import:libc"};
    result = validate_complete_attribute_set(invalid_attrs, 2, error_buf, sizeof(error_buf));
    printf("Test 2 (EXPORT + IMPORT): %s\n", result != 0 ? "PASS" : "FAIL");
    if (result != 0) {
        printf("  Error: %s\n", error_buf);
    }
    
    // Test 3: Multiple export types
    const char* multi_export[] = {"export:function", "export:variable"};
    result = validate_complete_attribute_set(multi_export, 2, error_buf, sizeof(error_buf));
    printf("Test 3 (Multiple export types): %s\n", result != 0 ? "PASS" : "FAIL");
    if (result != 0) {
        printf("  Error: %s\n", error_buf);
    }
    
    // Test 4: Compatible metadata
    const char* metadata_attrs[] = {"module:test", "version:1.0.0", "author:Dev", "license:MIT"};
    result = validate_complete_attribute_set(metadata_attrs, 4, error_buf, sizeof(error_buf));
    printf("Test 4 (Compatible metadata): %s\n", result == 0 ? "PASS" : "FAIL");
    
    printf("=== Attribute Combination Rules Test Complete ===\n");
    return 0;
}
