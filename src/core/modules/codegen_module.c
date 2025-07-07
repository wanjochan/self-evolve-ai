/**
 * codegen_module.c - 代码生成器模块
 * 
 * 提供将AST转换为汇编代码的功能。
 * 依赖于memory、astc和utils模块。
 */

#include "../module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Module name
static const char* MODULE_NAME = "codegen";

// Dependencies
MODULE_DEPENDS_ON(memory);
MODULE_DEPENDS_ON(astc);
MODULE_DEPENDS_ON(utils);

// Function type definitions for memory module functions
typedef void* (*memory_alloc_t)(size_t size, int pool);
typedef void (*memory_free_t)(void* ptr);
typedef void* (*memory_realloc_t)(void* ptr, size_t size);

// Function type definitions for astc module functions
typedef int (*astc_get_node_type_t)(void* node);

// Function type definitions for utils module functions
typedef void (*utils_print_error_t)(const char* format, ...);

// Cached module functions
static memory_alloc_t mem_alloc;
static memory_free_t mem_free;
static memory_realloc_t mem_realloc;
static astc_get_node_type_t astc_get_node_type;
static utils_print_error_t utils_print_error;

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
// AST Node Types (from astc.h)
// ===============================================

// Simplified subset of ASTNodeType
typedef enum {
    AST_UNKNOWN = 0,
    AST_MODULE,
    AST_IMPORT,
    AST_EXPORT,
    ASTC_TRANSLATION_UNIT,
    ASTC_TYPE_SPECIFIER,
    ASTC_FUNC_DECL,
    ASTC_VAR_DECL,
    ASTC_PARAM_DECL,
    ASTC_COMPOUND_STMT,
    ASTC_EXPR_IDENTIFIER,
    ASTC_EXPR_CONSTANT,
    ASTC_EXPR_STRING_LITERAL,
    ASTC_TYPE_STRUCT,
    ASTC_TYPE_ENUM,
    ASTC_TYPE_POINTER
} ASTNodeType;

// ===============================================
// AST Node Structure (simplified)
// ===============================================

typedef struct ASTNode {
    ASTNodeType type;
    int line;
    int column;
    union {
        struct {
            struct ASTNode** declarations;
            int declaration_count;
        } translation_unit;
        
        struct {
            char* name;
            struct ASTNode* return_type;
            struct ASTNode** parameters;
            int parameter_count;
            struct ASTNode* body;
            bool has_body;
        } func_decl;
        
        struct {
            struct ASTNode** statements;
            int statement_count;
        } compound_stmt;
        
        // Other node type data would be defined here
    } data;
} ASTNode;

// ===============================================
// 代码生成器结构
// ===============================================

typedef struct {
    char* buffer;           // 汇编代码缓冲区
    size_t buffer_size;     // 缓冲区大小
    size_t buffer_offset;   // 当前写入位置
    int label_count;        // 标签计数器，用于生成唯一的标签
} CodeGenerator;

// ===============================================
// 内部函数声明
// ===============================================

static void codegen_append(CodeGenerator* cg, const char* str);
static int codegen_generate_statement(CodeGenerator* cg, ASTNode* stmt_node);

// ===============================================
// 代码生成器实现
// ===============================================

/**
 * 初始化代码生成器
 */
static void codegen_init(CodeGenerator* cg) {
    cg->buffer_size = 16384; // 初始缓冲区大小 16KB
    cg->buffer = (char*)mem_alloc(cg->buffer_size, MEMORY_POOL_GENERAL);
    if (!cg->buffer) {
        utils_print_error("Failed to allocate buffer for code generator");
        return;
    }
    cg->buffer[0] = '\0';
    cg->buffer_offset = 0;
    cg->label_count = 0;
}

/**
 * 释放代码生成器资源
 */
static void codegen_free(CodeGenerator* cg) {
    if (cg->buffer) {
        mem_free(cg->buffer);
        cg->buffer = NULL;
    }
}

/**
 * 生成整个翻译单元的汇编代码
 */
static int codegen_generate_translation_unit(CodeGenerator* cg, ASTNode* unit_node) {
    if (!unit_node || unit_node->type != ASTC_TRANSLATION_UNIT) {
        return -1;
    }

    for (int i = 0; i < unit_node->data.translation_unit.declaration_count; i++) {
        ASTNode* decl = unit_node->data.translation_unit.declarations[i];
        if (decl->type == ASTC_FUNC_DECL) {
            if (codegen_generate_function(cg, decl) != 0) {
                return -1;
            }
        }
    }

    return 0;
}

/**
 * 生成单个函数的汇编代码
 */
static int codegen_generate_function(CodeGenerator* cg, ASTNode* func_node) {
    if (!func_node || func_node->type != ASTC_FUNC_DECL) {
        return -1;
    }

    char temp_buffer[256];

    // 函数标签
    sprintf(temp_buffer, "\n%s:\n", func_node->data.func_decl.name);
    codegen_append(cg, temp_buffer);

    // 函数序言
    codegen_append(cg, "    push rbp\n");
    codegen_append(cg, "    mov rbp, rsp\n");

    // 为局部变量分配栈空间 (TODO: 计算实际需要的空间)
    codegen_append(cg, "    sub rsp, 16\n"); 

    // 生成函数体代码
    if (func_node->data.func_decl.has_body) {
        if (codegen_generate_statement(cg, func_node->data.func_decl.body) != 0) {
            return -1;
        }
    }

    // 函数尾声
    codegen_append(cg, "    mov rsp, rbp\n");
    codegen_append(cg, "    pop rbp\n");
    codegen_append(cg, "    ret\n");

    return 0;
}

/**
 * 生成语句的汇编代码
 */
static int codegen_generate_statement(CodeGenerator* cg, ASTNode* stmt_node) {
    if (!stmt_node) return -1;

    switch (stmt_node->type) {
        case ASTC_COMPOUND_STMT:
            for (int i = 0; i < stmt_node->data.compound_stmt.statement_count; i++) {
                if (codegen_generate_statement(cg, stmt_node->data.compound_stmt.statements[i]) != 0) {
                    return -1;
                }
            }
            break;
        
        // TODO: 实现其他语句类型的代码生成

        default:
            // 其他语句暂不处理
            break;
    }
    return 0;
}

/**
 * 向缓冲区追加字符串
 */
static void codegen_append(CodeGenerator* cg, const char* str) {
    size_t str_len = strlen(str);
    if (cg->buffer_offset + str_len + 1 > cg->buffer_size) {
        cg->buffer_size = (cg->buffer_offset + str_len + 1) * 2;
        cg->buffer = (char*)mem_realloc(cg->buffer, cg->buffer_size);
        if (!cg->buffer) {
            utils_print_error("Failed to reallocate buffer for code generator");
            return;
        }
    }
    strcat(cg->buffer, str);
    cg->buffer_offset += str_len;
}

/**
 * 公开版本的codegen_append
 */
static void codegen_append_public(CodeGenerator* cg, const char* str) {
    codegen_append(cg, str);
}

/**
 * 创建新的代码生成器
 */
static CodeGenerator* codegen_create(void) {
    CodeGenerator* cg = (CodeGenerator*)mem_alloc(sizeof(CodeGenerator), MEMORY_POOL_GENERAL);
    if (cg) {
        codegen_init(cg);
    }
    return cg;
}

/**
 * 销毁代码生成器
 */
static void codegen_destroy(CodeGenerator* cg) {
    if (cg) {
        codegen_free(cg);
        mem_free(cg);
    }
}

/**
 * 获取生成的汇编代码
 */
static const char* codegen_get_assembly(CodeGenerator* cg) {
    return cg ? cg->buffer : NULL;
}

// ===============================================
// Module Symbols
// ===============================================

static struct {
    const char* name;
    void* symbol;
} codegen_symbols[] = {
    {"create", codegen_create},
    {"destroy", codegen_destroy},
    {"generate_translation_unit", codegen_generate_translation_unit},
    {"generate_function", codegen_generate_function},
    {"append", codegen_append_public},
    {"get_assembly", codegen_get_assembly},
    {NULL, NULL}
};

// ===============================================
// Module Interface
// ===============================================

/**
 * Resolve a symbol from this module
 */
static void* codegen_resolve(const char* symbol) {
    for (int i = 0; codegen_symbols[i].name != NULL; i++) {
        if (strcmp(codegen_symbols[i].name, symbol) == 0) {
            return codegen_symbols[i].symbol;
        }
    }
    return NULL;
}

/**
 * Initialize the module
 */
static int codegen_init(void) {
    // Resolve dependencies
    Module* memory_module = module_load("memory");
    if (!memory_module) {
        return -1;
    }
    
    Module* astc_module = module_load("astc");
    if (!astc_module) {
        return -1;
    }
    
    Module* utils_module = module_load("utils");
    if (!utils_module) {
        return -1;
    }
    
    // Resolve memory functions
    mem_alloc = module_resolve(memory_module, "alloc");
    mem_free = module_resolve(memory_module, "free");
    mem_realloc = module_resolve(memory_module, "realloc");
    
    // Resolve astc functions
    astc_get_node_type = module_resolve(astc_module, "get_node_type");
    
    // Resolve utils functions
    utils_print_error = module_resolve(utils_module, "print_error");
    
    if (!mem_alloc || !mem_free || !mem_realloc ||
        !astc_get_node_type || !utils_print_error) {
        return -1;
    }
    
    return 0;
}

/**
 * Clean up the module
 */
static void codegen_cleanup(void) {
    // Nothing to clean up
}

// Module definition - updated to match new module.h structure
Module module_codegen = {
    .name = MODULE_NAME,
    .state = MODULE_UNLOADED,
    .error = NULL,
    .init = codegen_init,
    .cleanup = codegen_cleanup,
    .resolve = codegen_resolve
};

// 注意：不再需要REGISTER_MODULE，使用动态加载机制