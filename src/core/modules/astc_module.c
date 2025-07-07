/**
 * astc_module.c - ASTC Module
 * 
 * Provides ASTC (Abstract Syntax Tree Compiler) functionality as a module.
 * Depends on the memory module.
 */

#include "../module.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

// Module name
#define MODULE_NAME "astc"

// Dependency on memory module
MODULE_DEPENDS_ON(memory);

// Function type definitions for memory module functions
typedef void* (*memory_alloc_t)(size_t size, int pool);
typedef void (*memory_free_t)(void* ptr);
typedef void* (*memory_calloc_t)(size_t count, size_t size, int pool);
typedef char* (*memory_strdup_t)(const char* str, int pool);

// Cached memory functions
static memory_alloc_t mem_alloc;
static memory_free_t mem_free;
static memory_calloc_t mem_calloc;
static memory_strdup_t mem_strdup;

// ===============================================
// Memory Pool Types (from memory.h)
// ===============================================

typedef enum {
    MEMORY_POOL_GENERAL,    // General purpose allocations
    MEMORY_POOL_BYTECODE,   // ASTC bytecode storage
    MEMORY_POOL_JIT,        // JIT compiled code
    MEMORY_POOL_MODULES,    // Native modules
    MEMORY_POOL_TEMP,       // Temporary allocations
    MEMORY_POOL_C99_AST,    // C99 AST nodes
    MEMORY_POOL_C99_SYMBOLS,// C99 symbol table
    MEMORY_POOL_C99_STRINGS,// C99 string literals
    MEMORY_POOL_COUNT
} MemoryPoolType;

// ===============================================
// AST Node Types
// ===============================================

typedef enum {
    // Core node types (simplified for this example)
    ASTC_UNKNOWN,
    ASTC_TRANSLATION_UNIT,
    ASTC_FUNC_DECL,
    ASTC_VAR_DECL,
    ASTC_PARAM_DECL,
    ASTC_TYPE_SPECIFIER,
    ASTC_COMPOUND_STMT,
    ASTC_IF_STMT,
    ASTC_WHILE_STMT,
    ASTC_FOR_STMT,
    ASTC_RETURN_STMT,
    ASTC_EXPR_STMT,
    ASTC_BINARY_OP,
    ASTC_UNARY_OP,
    ASTC_EXPR_CONSTANT,
    ASTC_EXPR_IDENTIFIER
} ASTNodeType;

// ===============================================
// AST Node Structure
// ===============================================

typedef struct ASTNode {
    ASTNodeType type;
    int line;
    int column;
    
    // Node data (simplified union for this example)
    union {
        // Binary operation
        struct {
            ASTNodeType op;
            struct ASTNode *left;
            struct ASTNode *right;
        } binary_op;
        
        // Unary operation
        struct {
            ASTNodeType op;
            struct ASTNode *operand;
        } unary_op;
        
        // Constant
        struct {
            ASTNodeType type;
            union {
                int64_t int_val;
                double float_val;
            };
        } constant;
        
        // Identifier
        struct {
            char *name;
        } identifier;
        
        // Function declaration
        struct {
            char *name;
            struct ASTNode *return_type;
            struct ASTNode **params;
            int param_count;
            int has_body;
            struct ASTNode *body;
        } func_decl;
        
        // Variable declaration
        struct {
            char *name;
            struct ASTNode *type;
            struct ASTNode *initializer;
        } var_decl;
        
        // Compound statement
        struct {
            struct ASTNode **statements;
            int statement_count;
        } compound_stmt;
        
        // If statement
        struct {
            struct ASTNode *condition;
            struct ASTNode *then_branch;
            struct ASTNode *else_branch;
        } if_stmt;
        
        // While statement
        struct {
            struct ASTNode *condition;
            struct ASTNode *body;
        } while_stmt;
        
        // For statement
        struct {
            struct ASTNode *init;
            struct ASTNode *condition;
            struct ASTNode *increment;
            struct ASTNode *body;
        } for_stmt;
        
        // Return statement
        struct {
            struct ASTNode *value;
        } return_stmt;
        
        // Expression statement
        struct {
            struct ASTNode *expr;
        } expr_stmt;
    } data;
} ASTNode;

// ===============================================
// ASTC Program Structure
// ===============================================

typedef struct {
    char program_name[256];
    uint32_t version;
    uint32_t flags;
    uint32_t entry_point;
    uint32_t source_size;
    char* source_code;
    uint32_t bytecode_size;
    uint8_t* bytecode;
} ASTCProgram;

// ===============================================
// AST Node Management
// ===============================================

/**
 * Create a new AST node
 */
static ASTNode* ast_create_node(ASTNodeType type, int line, int column) {
    ASTNode* node = (ASTNode*)mem_calloc(1, sizeof(ASTNode), MEMORY_POOL_C99_AST);
    if (!node) {
        return NULL;
    }
    
    // Initialize the node
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
            node->data.binary_op.op = ASTC_UNKNOWN;
            node->data.binary_op.left = NULL;
            node->data.binary_op.right = NULL;
            break;

        case ASTC_UNARY_OP:
            node->data.unary_op.op = ASTC_UNKNOWN;
            node->data.unary_op.operand = NULL;
            break;
            
        case ASTC_EXPR_CONSTANT:
            node->data.constant.type = ASTC_UNKNOWN;
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
static void ast_free(ASTNode* node) {
    if (!node) {
        return;
    }

    // Free type-specific data
    switch (node->type) {
        case ASTC_FUNC_DECL:
            if (node->data.func_decl.name) {
                mem_free(node->data.func_decl.name);
            }
            ast_free(node->data.func_decl.return_type);
            if (node->data.func_decl.params) {
                for (int i = 0; i < node->data.func_decl.param_count; i++) {
                    ast_free(node->data.func_decl.params[i]);
                }
                mem_free(node->data.func_decl.params);
            }
            ast_free(node->data.func_decl.body);
            break;
            
        case ASTC_VAR_DECL:
            if (node->data.var_decl.name) {
                mem_free(node->data.var_decl.name);
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
                mem_free(node->data.var_decl.name);
            }
            ast_free(node->data.var_decl.type);
            break;
            
        default:
            // For other node types, no special cleanup needed
            break;
    }
    
    mem_free(node);
}

/**
 * Print an AST node for debugging
 */
static void ast_print_node(const ASTNode* node, int indent) {
    if (!node) {
        return;
    }
    
    // Print indentation
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
    
    // Print node type and information
    printf("Node Type: %d, Line: %d, Column: %d\n", node->type, node->line, node->column);
    
    // Print node-specific data
    switch (node->type) {
        case ASTC_FUNC_DECL:
            printf("Function: %s\n", node->data.func_decl.name ? node->data.func_decl.name : "<unnamed>");
            break;
            
        case ASTC_VAR_DECL:
            printf("Variable: %s\n", node->data.var_decl.name ? node->data.var_decl.name : "<unnamed>");
            break;
            
        case ASTC_EXPR_CONSTANT:
            if (node->data.constant.type == ASTC_EXPR_CONSTANT) {
                printf("Constant: %lld\n", (long long)node->data.constant.int_val);
            } else {
                printf("Constant: %f\n", node->data.constant.float_val);
            }
            break;
            
        default:
            break;
    }
    
    // Recursively print children
    switch (node->type) {
        case ASTC_FUNC_DECL:
            if (node->data.func_decl.return_type) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Return Type:\n");
                ast_print_node(node->data.func_decl.return_type, indent + 2);
            }
            
            if (node->data.func_decl.params) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Parameters:\n");
                for (int i = 0; i < node->data.func_decl.param_count; i++) {
                    ast_print_node(node->data.func_decl.params[i], indent + 2);
                }
            }
            
            if (node->data.func_decl.body) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Body:\n");
                ast_print_node(node->data.func_decl.body, indent + 2);
            }
            break;
            
        case ASTC_BINARY_OP:
            if (node->data.binary_op.left) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Left:\n");
                ast_print_node(node->data.binary_op.left, indent + 2);
            }
            
            if (node->data.binary_op.right) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Right:\n");
                ast_print_node(node->data.binary_op.right, indent + 2);
            }
            break;
            
        // Add other node types as needed
        
        default:
            break;
    }
}

// ===============================================
// ASTC Program Management
// ===============================================

/**
 * Create a new ASTC program
 */
static ASTCProgram* astc_create_program(const char* name) {
    ASTCProgram* program = (ASTCProgram*)mem_alloc(sizeof(ASTCProgram), MEMORY_POOL_GENERAL);
    if (!program) {
        return NULL;
    }
    
    // Initialize program
    memset(program, 0, sizeof(ASTCProgram));
    
    if (name) {
        strncpy(program->program_name, name, sizeof(program->program_name) - 1);
    } else {
        strcpy(program->program_name, "unnamed_program");
    }
    
    program->version = 1;
    
    return program;
}

/**
 * Free an ASTC program
 */
static void astc_free_program(ASTCProgram* program) {
    if (!program) {
        return;
    }
    
    // Free source code if present
    if (program->source_code) {
        mem_free(program->source_code);
    }
    
    // Free bytecode if present
    if (program->bytecode) {
        mem_free(program->bytecode);
    }
    
    // Free program structure
    mem_free(program);
}

/**
 * Load ASTC program from file
 */
static ASTCProgram* astc_load_program(const char* astc_file) {
    if (!astc_file) {
        return NULL;
    }

    FILE* file = fopen(astc_file, "rb");
    if (!file) {
        return NULL;
    }

    // Read and verify ASTC magic
    char magic[4];
    if (fread(magic, 1, 4, file) != 4 || memcmp(magic, "ASTC", 4) != 0) {
        fclose(file);
        return NULL;
    }

    // Create program structure
    ASTCProgram* program = mem_calloc(1, sizeof(ASTCProgram), MEMORY_POOL_GENERAL);
    if (!program) {
        fclose(file);
        return NULL;
    }

    // Read header
    if (fread(&program->version, sizeof(uint32_t), 1, file) != 1 ||
        fread(&program->flags, sizeof(uint32_t), 1, file) != 1 ||
        fread(&program->entry_point, sizeof(uint32_t), 1, file) != 1 ||
        fread(&program->source_size, sizeof(uint32_t), 1, file) != 1) {
        mem_free(program);
        fclose(file);
        return NULL;
    }

    // Read source code if present
    if (program->source_size > 0) {
        program->source_code = mem_alloc(program->source_size + 1, MEMORY_POOL_C99_STRINGS);
        if (!program->source_code) {
            mem_free(program);
            fclose(file);
            return NULL;
        }

        if (fread(program->source_code, 1, program->source_size, file) != program->source_size) {
            mem_free(program->source_code);
            mem_free(program);
            fclose(file);
            return NULL;
        }
        program->source_code[program->source_size] = '\0';
    }

    // Read bytecode size
    if (fread(&program->bytecode_size, sizeof(uint32_t), 1, file) != 1) {
        if (program->source_code) mem_free(program->source_code);
        mem_free(program);
        fclose(file);
        return NULL;
    }

    // Read bytecode
    if (program->bytecode_size > 0) {
        program->bytecode = mem_alloc(program->bytecode_size, MEMORY_POOL_BYTECODE);
        if (!program->bytecode) {
            if (program->source_code) mem_free(program->source_code);
            mem_free(program);
            fclose(file);
            return NULL;
        }

        if (fread(program->bytecode, 1, program->bytecode_size, file) != program->bytecode_size) {
            mem_free(program->bytecode);
            if (program->source_code) mem_free(program->source_code);
            mem_free(program);
            fclose(file);
            return NULL;
        }
    }

    // Get program name from file path
    const char* filename = strrchr(astc_file, '/');
    if (!filename) filename = strrchr(astc_file, '\\');
    if (filename) {
        filename++; // Skip the slash
    } else {
        filename = astc_file;
    }
    
    strncpy(program->program_name, filename, sizeof(program->program_name) - 1);
    program->program_name[sizeof(program->program_name) - 1] = '\0';
    
    // Remove extension if present
    char* ext = strrchr(program->program_name, '.');
    if (ext) *ext = '\0';

    fclose(file);
    return program;
}

/**
 * Save ASTC program to file
 */
static int astc_save_program(const ASTCProgram* program, const char* astc_file) {
    if (!program || !astc_file) {
        return -1;
    }

    FILE* file = fopen(astc_file, "wb");
    if (!file) {
        return -1;
    }

    // Write ASTC magic
    const char magic[] = "ASTC";
    fwrite(magic, 1, 4, file);

    // Write header
    fwrite(&program->version, sizeof(uint32_t), 1, file);
    fwrite(&program->flags, sizeof(uint32_t), 1, file);
    fwrite(&program->entry_point, sizeof(uint32_t), 1, file);
    fwrite(&program->source_size, sizeof(uint32_t), 1, file);

    // Write source code if present
    if (program->source_size > 0 && program->source_code) {
        fwrite(program->source_code, 1, program->source_size, file);
    }

    // Write bytecode size and bytecode
    fwrite(&program->bytecode_size, sizeof(uint32_t), 1, file);
    if (program->bytecode_size > 0 && program->bytecode) {
        fwrite(program->bytecode, 1, program->bytecode_size, file);
    }

    fclose(file);
    return 0;
}

/**
 * Validate ASTC program
 */
static bool astc_validate_program(const ASTCProgram* program) {
    if (!program) {
        return false;
    }
    
    // Check for valid bytecode
    if (program->bytecode_size > 0 && !program->bytecode) {
        return false;
    }
    
    // Check for valid source code
    if (program->source_size > 0 && !program->source_code) {
        return false;
    }
    
    // Check for valid entry point
    if (program->bytecode_size > 0 && program->entry_point >= program->bytecode_size) {
        return false;
    }
    
    return true;
}

// ===============================================
// Symbol Table
// ===============================================

// Symbol table
static struct {
    const char* name;
    void* symbol;
} astc_symbols[] = {
    // AST node management
    {"create_node", ast_create_node},
    {"free_node", ast_free},
    {"print_node", ast_print_node},
    
    // ASTC program management
    {"create_program", astc_create_program},
    {"free_program", astc_free_program},
    {"load_program", astc_load_program},
    {"save_program", astc_save_program},
    {"validate_program", astc_validate_program},
    
    {NULL, NULL}  // Sentinel
};

// ===============================================
// Module Interface
// ===============================================

// Module init function (renamed from astc_load)
static int astc_init(void) {
    // Resolve required memory functions
    Module* memory = module_get("memory");
    if (!memory) {
        return -1;
    }
    
    mem_alloc = module_resolve(memory, "alloc_pool");
    mem_free = module_resolve(memory, "free");
    mem_calloc = module_resolve(memory, "calloc");
    mem_strdup = module_resolve(memory, "strdup");
    
    if (!mem_alloc || !mem_free || !mem_calloc || !mem_strdup) {
        return -1;
    }
    
    return 0;
}

// Module cleanup function (renamed from astc_unload)
static void astc_cleanup(void) {
    // Nothing to clean up
}

// Symbol resolution function
static void* astc_resolve(const char* symbol) {
    if (!symbol) {
        return NULL;
    }
    
    for (int i = 0; astc_symbols[i].name; i++) {
        if (strcmp(astc_symbols[i].name, symbol) == 0) {
            return astc_symbols[i].symbol;
        }
    }
    
    return NULL;
}

// Module definition - updated to match new module.h structure
Module module_astc = {
    .name = MODULE_NAME,
    .state = MODULE_UNLOADED,
    .error = NULL,
    .init = astc_init,
    .cleanup = astc_cleanup,
    .resolve = astc_resolve
};

// 注意：不再需要REGISTER_MODULE，使用动态加载机制
