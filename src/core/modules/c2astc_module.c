/**
 * c2astc_module.c - C to ASTC Converter Module
 * 
 * Provides C to ASTC conversion functionality as a module.
 * Depends on the memory, astc, and utils modules.
 */

#include "../module.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

// Module name
#define MODULE_NAME "c2astc"

// Dependencies
MODULE_DEPENDS_ON(memory);
MODULE_DEPENDS_ON(astc);
MODULE_DEPENDS_ON(utils);

// Function type definitions for memory module functions
typedef void* (*memory_alloc_t)(size_t size, int pool);
typedef void (*memory_free_t)(void* ptr);
typedef void* (*memory_calloc_t)(size_t count, size_t size, int pool);
typedef char* (*memory_strdup_t)(const char* str);

// Function type definitions for astc module functions
typedef struct ASTNode* (*astc_create_node_t)(int type, int line, int column);
typedef void (*astc_free_node_t)(void* node);
typedef struct ASTNode* (*astc_create_translation_unit_t)(void);

// Function type definitions for utils module functions
typedef bool (*utils_read_file_t)(const char* filename, char** content, size_t* size);
typedef void (*utils_print_error_t)(const char* format, ...);

// Cached module functions
static memory_alloc_t mem_alloc;
static memory_free_t mem_free;
static memory_calloc_t mem_calloc;
static memory_strdup_t mem_strdup;
static astc_create_node_t astc_create_node;
static astc_free_node_t astc_free_node;
static astc_create_translation_unit_t astc_create_translation_unit;
static utils_read_file_t utils_read_file;
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
// Token Types
// ===============================================

typedef enum {
    TOKEN_EOF = 0,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_BANG,
    TOKEN_BANG_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_AND,
    TOKEN_AND_AND,
    TOKEN_OR,
    TOKEN_OR_OR,
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_RETURN,
    TOKEN_STRUCT,
    TOKEN_UNION,
    TOKEN_ENUM,
    TOKEN_TYPEDEF,
    TOKEN_EXTERN,
    TOKEN_STATIC,
    TOKEN_CONST,
    TOKEN_VOID,
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_DOUBLE,
    TOKEN_CHAR,
    TOKEN_LONG,
    TOKEN_SHORT,
    TOKEN_UNSIGNED,
    TOKEN_SIGNED,
    TOKEN_MODULE,
    TOKEN_IMPORT,
    TOKEN_EXPORT,
    TOKEN_FROM
} TokenType;

// ===============================================
// Public Structures
// ===============================================

/**
 * C标准版本枚举
 */
typedef enum {
    C_STD_C99 = 0,
    C_STD_C11 = 1,
    C_STD_C17 = 2
} CStandard;

/**
 * 转换配置选项
 */
typedef struct {
    int optimize_level;         // 优化级别 (0=无优化, 1=基础, 2=高级, 3=激进)
    bool enable_extensions;     // 启用WASX扩展
    bool emit_debug_info;       // 生成调试信息
    bool enable_warnings;       // 启用警告
    bool warnings_as_errors;    // 将警告视为错误
    bool compile_only;          // 仅编译，不链接
    bool generate_assembly;     // 生成汇编输出
    bool preprocess_only;       // 仅预处理
    CStandard c_standard;       // C标准版本
    char include_dirs[16][256]; // 包含目录列表 (最多16个)
    int include_dir_count;      // 包含目录数量
    char macros[32][256];       // 宏定义列表 (最多32个)
    int macro_count;            // 宏定义数量
} C2AstcOptions;

/**
 * 字节码生成器上下文
 */
typedef struct {
    int optimize_level;         // 优化级别
    bool enable_extensions;     // 是否启用扩展
    bool emit_debug_info;       // 是否生成调试信息
    
    // 符号表
    struct {
        char *names[1024];      // 符号名称
        void *addresses[1024];  // 符号地址
        int count;              // 符号数量
    } symbols;
    
    // 字节码生成状态
    struct {
        unsigned char *buffer;  // 字节码缓冲区
        size_t size;           // 当前大小
        size_t capacity;       // 缓冲区容量
    } bytecode;
    
    // 调试信息
    struct {
        int *line_numbers;     // 行号表
        int *columns;          // 列号表
        int count;             // 调试信息条目数
    } debug_info;
} BytecodeContext;

// ===============================================
// Internal Structures
// ===============================================

// Token structure
typedef struct {
    TokenType type;
    char* value;
    int line;
    int column;
} Token;

// Lexer structure
typedef struct {
    const char *source;
    int current;
    int line;
    int column;
} Lexer;

// Parser structure
typedef struct {
    Token **tokens;
    int token_count;
    int current;
    
    // 错误处理
    char error_msg[256];
    int error_count;
    
    // 符号表
    struct {
        char *names[1024];
        struct ASTNode *nodes[1024];
        int count;
    } symbols;
} Parser;

// 全局错误消息缓冲区
static char g_error_message[1024] = {0};

// ===============================================
// Forward Declarations
// ===============================================

// Lexer functions
static void init_lexer(Lexer *lexer, const char *source);
static char peek_char(Lexer *lexer);
static char advance_char(Lexer *lexer);
static void skip_whitespace(Lexer *lexer);
static void skip_comment(Lexer *lexer);
static bool is_identifier_char(char c);
static Token* get_string(Lexer *lexer);
static Token* get_number(Lexer *lexer);
static Token* get_identifier(Lexer *lexer);
static Token* create_token(TokenType type, const char *value, int line, int column);
static bool tokenize(const char *source, Token ***tokens, int *token_count);

// Parser functions
static void set_error(const char *format, ...);
static Token* peek(Parser* parser);
static Token* advance(Parser* parser);
static bool match(Parser* parser, TokenType type);
static void parser_error(Parser* parser, const char* message);

// AST parsing functions
static struct ASTNode* parse_translation_unit(Parser* parser);
static struct ASTNode* parse_declaration(Parser* parser);
static struct ASTNode* parse_type_specifier(Parser* parser);
static struct ASTNode* parse_function_declaration(Parser* parser, struct ASTNode* return_type, char* name);
static struct ASTNode* parse_parameter_declaration(Parser* parser);
static struct ASTNode* parse_compound_statement(Parser* parser);
static struct ASTNode* parse_statement(Parser* parser);
static struct ASTNode* parse_expression_statement(Parser* parser);
static struct ASTNode* parse_if_statement(Parser* parser);
static struct ASTNode* parse_while_statement(Parser* parser);
static struct ASTNode* parse_for_statement(Parser* parser);
static struct ASTNode* parse_return_statement(Parser* parser);
static struct ASTNode* parse_expression(Parser* parser);
static struct ASTNode* parse_assignment(Parser* parser);
static struct ASTNode* parse_logical_or(Parser* parser);
static struct ASTNode* parse_logical_and(Parser* parser);
static struct ASTNode* parse_equality(Parser* parser);
static struct ASTNode* parse_relational(Parser* parser);
static struct ASTNode* parse_additive(Parser* parser);
static struct ASTNode* parse_multiplicative(Parser* parser);
static struct ASTNode* parse_unary(Parser* parser);
static struct ASTNode* parse_primary(Parser* parser);
static struct ASTNode* parse_struct_or_union(Parser* parser);
static struct ASTNode* parse_enum(Parser* parser);
static struct ASTNode* parse_pointer_type(Parser* parser, struct ASTNode* base_type);
static struct ASTNode* parse_array_type(Parser* parser, struct ASTNode* element_type);
static struct ASTNode* parse_function_type(Parser* parser, struct ASTNode* return_type);
static struct ASTNode* parse_module_statement(Parser* parser);
static struct ASTNode* parse_import_statement(Parser* parser);
static struct ASTNode* parse_export_statement(Parser* parser);

// Bytecode generation functions
static bool generate_bytecode(struct ASTNode* ast, BytecodeContext* ctx, 
                      unsigned char** bytecode, size_t* size, size_t* capacity);

// ===============================================
// API Implementation
// ===============================================

/**
 * 获取默认的转换选项
 */
static C2AstcOptions c2astc_default_options(void) {
    C2AstcOptions options;
    memset(&options, 0, sizeof(C2AstcOptions));
    options.optimize_level = 1;
    options.enable_extensions = true;
    options.emit_debug_info = true;
    options.enable_warnings = true;
    options.warnings_as_errors = false;
    options.compile_only = false;
    options.generate_assembly = false;
    options.preprocess_only = false;
    options.c_standard = C_STD_C99;
    options.include_dir_count = 0;
    options.macro_count = 0;
    return options;
}

/**
 * 打印C到ASTC转换库版本信息
 */
static void c2astc_print_version(void) {
    printf("C to ASTC Converter Module v1.0\n");
}

/**
 * 将C源代码转换为ASTC表示
 */
static struct ASTNode* c2astc_convert(const char *source, const C2AstcOptions *options) {
    if (!source) {
        set_error("Source code is NULL");
        return NULL;
    }
    
    // 使用默认选项（如果未提供）
    C2AstcOptions default_options;
    if (!options) {
        default_options = c2astc_default_options();
        options = &default_options;
    }
    
    // 词法分析
    Token **tokens = NULL;
    int token_count = 0;
    if (!tokenize(source, &tokens, &token_count)) {
        return NULL;
    }
    
    // 语法分析
    Parser parser;
    memset(&parser, 0, sizeof(Parser));
    parser.tokens = tokens;
    parser.token_count = token_count;
    parser.current = 0;
    
    struct ASTNode *ast = parse_translation_unit(&parser);
    
    // 释放tokens
    for (int i = 0; i < token_count; i++) {
        if (tokens[i]->value) {
            mem_free(tokens[i]->value);
        }
        mem_free(tokens[i]);
    }
    mem_free(tokens);
    
    if (parser.error_count > 0) {
        set_error("%s", parser.error_msg);
        if (ast) {
            astc_free_node(ast);
            ast = NULL;
        }
    }
    
    return ast;
}

/**
 * 从文件加载C源代码并转换为ASTC
 */
static struct ASTNode* c2astc_convert_file(const char *filename, const C2AstcOptions *options) {
    if (!filename) {
        set_error("Filename is NULL");
        return NULL;
    }
    
    // 读取文件内容
    char *source = NULL;
    size_t size = 0;
    if (!utils_read_file(filename, &source, &size)) {
        set_error("Failed to read file: %s", filename);
        return NULL;
    }
    
    // 转换源代码
    struct ASTNode *ast = c2astc_convert(source, options);
    
    // 释放源代码内存
    mem_free(source);
    
    return ast;
}

/**
 * 获取最后一次错误消息
 */
static const char* c2astc_get_error(void) {
    return g_error_message[0] ? g_error_message : NULL;
}

/**
 * 设置错误消息
 */
static void set_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(g_error_message, sizeof(g_error_message), format, args);
    va_end(args);
}

// ===============================================
// Lexer Implementation
// ===============================================

static void init_lexer(Lexer *lexer, const char *source) {
    lexer->source = source;
    lexer->current = 0;
    lexer->line = 1;
    lexer->column = 1;
}

static char peek_char(Lexer *lexer) {
    return lexer->source[lexer->current];
}

static char advance_char(Lexer *lexer) {
    char c = lexer->source[lexer->current++];
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    return c;
}

static void skip_whitespace(Lexer *lexer) {
    while (isspace(peek_char(lexer))) {
        advance_char(lexer);
    }
}

static void skip_comment(Lexer *lexer) {
    // Skip the first '/'
    advance_char(lexer);
    
    if (peek_char(lexer) == '/') {
        // Line comment
        advance_char(lexer);
        while (peek_char(lexer) != '\0' && peek_char(lexer) != '\n') {
            advance_char(lexer);
        }
    } else if (peek_char(lexer) == '*') {
        // Block comment
        advance_char(lexer);
        while (!(peek_char(lexer) == '*' && lexer->source[lexer->current + 1] == '/')) {
            if (peek_char(lexer) == '\0') {
                set_error("Unterminated block comment");
                return;
            }
            advance_char(lexer);
        }
        // Skip '*/'
        advance_char(lexer);
        advance_char(lexer);
    }
}

static bool is_identifier_char(char c) {
    return isalnum(c) || c == '_';
}

static Token* create_token(TokenType type, const char *value, int line, int column) {
    Token *token = (Token *)mem_alloc(sizeof(Token), MEMORY_POOL_C99_SYMBOLS);
    if (!token) return NULL;
    
    token->type = type;
    token->line = line;
    token->column = column;
    
    if (value) {
        token->value = mem_strdup(value);
    } else {
        token->value = NULL;
    }
    
    return token;
}

// ===============================================
// Module Symbols
// ===============================================

static struct {
    const char* name;
    void* symbol;
} c2astc_symbols[] = {
    {"default_options", c2astc_default_options},
    {"print_version", c2astc_print_version},
    {"convert", c2astc_convert},
    {"convert_file", c2astc_convert_file},
    {"get_error", c2astc_get_error},
    {NULL, NULL}
};

// ===============================================
// Module Interface
// ===============================================

/**
 * Resolve a symbol from this module
 */
static void* c2astc_resolve(const char* symbol) {
    for (int i = 0; c2astc_symbols[i].name != NULL; i++) {
        if (strcmp(c2astc_symbols[i].name, symbol) == 0) {
            return c2astc_symbols[i].symbol;
        }
    }
    return NULL;
}

/**
 * Initialize the module
 */
static int c2astc_init(void) {
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
    mem_calloc = module_resolve(memory_module, "calloc");
    mem_strdup = module_resolve(memory_module, "strdup");
    
    // Resolve astc functions
    astc_create_node = module_resolve(astc_module, "create_node");
    astc_free_node = module_resolve(astc_module, "free_node");
    astc_create_translation_unit = module_resolve(astc_module, "create_translation_unit");
    
    // Resolve utils functions
    utils_read_file = module_resolve(utils_module, "read_file_to_buffer");
    utils_print_error = module_resolve(utils_module, "print_error");
    
    if (!mem_alloc || !mem_free || !mem_calloc || !mem_strdup ||
        !astc_create_node || !astc_free_node || !astc_create_translation_unit ||
        !utils_read_file || !utils_print_error) {
        return -1;
    }
    
    return 0;
}

/**
 * Clean up the module
 */
static void c2astc_cleanup(void) {
    // Nothing to clean up
}

// Module definition - updated to match new module.h structure
Module module_c2astc = {
    .name = MODULE_NAME,
    .state = MODULE_UNLOADED,
    .error = NULL,
    .init = c2astc_init,
    .cleanup = c2astc_cleanup,
    .resolve = c2astc_resolve
};

// 注意：不再需要REGISTER_MODULE，使用动态加载机制