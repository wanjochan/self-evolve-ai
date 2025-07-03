/**
 * c2astc.c - C语言到ASTC（WebAssembly扩展AST）的转换库实现
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "../core/include/core_astc.h"
#include "../core/include/token.h"
#include "../core/include/logger.h"
#include "c2astc.h"

// 全局错误消息缓冲区
static char g_error_message[1024] = {0};

// Lexer structure
typedef struct {
    const char *source;
    int current;
    int line;
    int column;
} Lexer;

// Forward declarations for lexer functions
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

// Forward declarations
static void set_error(const char *format, ...);
static bool tokenize(const char *source, Token ***tokens, int *token_count);
static Token* peek(Parser* parser);
static Token* advance(Parser* parser);
static bool match(Parser* parser, TokenType type);
static void parser_error(Parser* parser, const char* message);

// AST parsing function declarations
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

// Bytecode generation declarations
bool generate_bytecode(struct ASTNode* ast, BytecodeContext* ctx, 
                      unsigned char** bytecode, size_t* size, size_t* capacity);

// Parser utility functions
static Token* peek(Parser* parser) {
    if (parser->current >= parser->token_count) {
        return NULL;
    }
    return parser->tokens[parser->current];
}

static Token* advance(Parser* parser) {
    if (parser->current < parser->token_count) {
        return parser->tokens[parser->current++];
    }
    return NULL;
}

static bool match(Parser* parser, TokenType type) {
    Token* token = peek(parser);
    if (token && token->type == type) {
        advance(parser);
        return true;
    }
    return false;
}

static void parser_error(Parser* parser, const char* message) {
    Token* token = peek(parser);
    if (token) {
        snprintf(parser->error_msg, sizeof(parser->error_msg), 
                "Error at line %d, column %d: %s", 
                token->line, token->column, message);
    } else {
        snprintf(parser->error_msg, sizeof(parser->error_msg), 
                "Error at end of file: %s", message);
    }
    parser->error_count++;
}

// ===============================================
// 模块系统解析函数
// ===============================================

// 解析模块声明: module name;
static struct ASTNode* parse_module_statement(Parser *parser) {
    if (!match(parser, TOKEN_MODULE)) {
        parser_error(parser, "预期module关键字");
        return NULL;
    }

    Token *token = peek(parser);
    if (!token || token->type != TOKEN_IDENTIFIER) {
        parser_error(parser, "预期模块名");
        return NULL;
    }

    struct ASTNode *module_node = ast_create_node(AST_MODULE, token->line, token->column);
    if (!module_node) return NULL;

    module_node->data.module_decl.name = strdup(token->value);
    module_node->data.module_decl.version = NULL;
    module_node->data.module_decl.author = NULL;
    module_node->data.module_decl.description = NULL;
    module_node->data.module_decl.license = NULL;
    module_node->data.module_decl.declarations = NULL;
    module_node->data.module_decl.declaration_count = 0;
    module_node->data.module_decl.exports = NULL;
    module_node->data.module_decl.export_count = 0;
    module_node->data.module_decl.imports = NULL;
    module_node->data.module_decl.import_count = 0;
    module_node->data.module_decl.init_func = NULL;
    module_node->data.module_decl.cleanup_func = NULL;

    advance(parser);

    if (!match(parser, TOKEN_SEMICOLON)) {
        parser_error(parser, "预期分号");
        ast_free(module_node);
        return NULL;
    }

    return module_node;
}

// 解析导入语句: import name from "path";
static struct ASTNode* parse_import_statement(Parser *parser) {
    if (!match(parser, TOKEN_IMPORT)) {
        parser_error(parser, "预期import关键字");
        return NULL;
    }

    Token *token = peek(parser);
    if (!token || token->type != TOKEN_IDENTIFIER) {
        parser_error(parser, "预期导入名称");
        return NULL;
    }

    struct ASTNode *import_node = ast_create_node(AST_IMPORT, token->line, token->column);
    if (!import_node) return NULL;

    import_node->data.import_decl.module_name = strdup(token->value);
    import_node->data.import_decl.import_name = NULL;
    import_node->data.import_decl.local_name = NULL;
    import_node->data.import_decl.version_requirement = NULL;
    import_node->data.import_decl.import_type = 0;
    import_node->data.import_decl.is_weak = false;
    import_node->data.import_decl.is_lazy = false;
    import_node->data.import_decl.declaration = NULL;

    advance(parser);

    if (match(parser, TOKEN_FROM)) {
        token = peek(parser);
        if (!token || token->type != TOKEN_STRING_LITERAL) {
            parser_error(parser, "预期字符串路径");
            ast_free(import_node);
            return NULL;
        }

        import_node->data.import_decl.local_name = strdup(token->value);
        advance(parser);
    }

    if (!match(parser, TOKEN_SEMICOLON)) {
        parser_error(parser, "预期分号");
        ast_free(import_node);
        return NULL;
    }

    return import_node;
}

// 解析导出语句: export name;
static struct ASTNode* parse_export_statement(Parser *parser) {
    if (!match(parser, TOKEN_EXPORT)) {
        parser_error(parser, "预期export关键字");
        return NULL;
    }

    Token *token = peek(parser);
    if (!token || token->type != TOKEN_IDENTIFIER) {
        parser_error(parser, "预期导出名称");
        return NULL;
    }

    struct ASTNode *export_node = ast_create_node(AST_EXPORT, token->line, token->column);
    if (!export_node) return NULL;

    export_node->data.export_decl.name = strdup(token->value);
    export_node->data.export_decl.alias = NULL;
    export_node->data.export_decl.export_type = 0;
    export_node->data.export_decl.declaration = NULL;
    export_node->data.export_decl.is_default = false;
    export_node->data.export_decl.flags = 0;

    advance(parser);

    if (!match(parser, TOKEN_SEMICOLON)) {
        parser_error(parser, "预期分号");
        ast_free(export_node);
        return NULL;
    }

    return export_node;
}

// 获取默认的转换选项
C2AstcOptions c2astc_default_options(void) {
    C2AstcOptions options = {
        .optimize_level = 0,
        .enable_extensions = false,
        .emit_debug_info = false,
        .enable_warnings = true,
        .warnings_as_errors = false,
        .compile_only = false,
        .generate_assembly = false,
        .preprocess_only = false,
        .c_standard = C_STD_C99,
        .include_dir_count = 0,
        .macro_count = 0
    };
    return options;
}

// 从文件加载C源代码并转换为ASTC
struct ASTNode* c2astc_convert_file(const char *filename, const C2AstcOptions *options) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        set_error("无法打开文件: %s", filename);
        return NULL;
    }

    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 分配内存并读取文件内容
    char *source = (char *)malloc(size + 1);
    if (!source) {
        fclose(file);
        set_error("内存分配失败");
        return NULL;
    }

    size_t read_size = fread(source, 1, size, file);
    fclose(file);

    if (read_size == 0 && size > 0) {
        free(source);
        set_error("读取文件失败: %s", filename);
        return NULL;
    }

    source[size] = '\0';

    // 转换源代码
    struct ASTNode *ast = c2astc_convert(source, options);
    free(source);

    return ast;
}

// 获取最后一次错误消息
const char* c2astc_get_error(void) {
    return g_error_message[0] ? g_error_message : NULL;
}

// 设置错误消息
static void set_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(g_error_message, sizeof(g_error_message), format, args);
    va_end(args);
}

// 确保字节码缓冲区有足够空间
static bool ensure_capacity(unsigned char** bytecode, size_t* size, size_t* capacity, size_t needed) {
    if (*size + needed > *capacity) {
        size_t new_capacity = *capacity * 2;
        while (new_capacity < *size + needed) {
            new_capacity *= 2;
        }
        
        unsigned char* new_buffer = (unsigned char*)realloc(*bytecode, new_capacity);
        if (!new_buffer) {
            set_error("内存分配失败");
            return false;
        }
        
        *bytecode = new_buffer;
        *capacity = new_capacity;
    }
    return true;
}

// 写入字节码
static bool write_bytecode(unsigned char** bytecode, size_t* size, size_t* capacity, 
                          const void* data, size_t data_size) {
    if (!ensure_capacity(bytecode, size, capacity, data_size)) {
        return false;
    }
    
    memcpy(*bytecode + *size, data, data_size);
    *size += data_size;
    return true;
}

// 生成表达式字节码
static bool generate_expression_bytecode(struct ASTNode* expr, BytecodeContext* ctx,
                                       unsigned char** bytecode, size_t* size, size_t* capacity) {
    if (!expr) return true;

    // 写入节点类型
    if (!write_bytecode(bytecode, size, capacity, &expr->type, sizeof(expr->type))) {
        return false;
    }

    // 根据节点类型生成相应的字节码
    switch (expr->type) {
        case ASTC_EXPR_IDENTIFIER: {
            size_t name_len = strlen(expr->data.identifier.name);
            if (!write_bytecode(bytecode, size, capacity, &name_len, sizeof(name_len)) ||
                !write_bytecode(bytecode, size, capacity, expr->data.identifier.name, name_len)) {
                return false;
            }
            break;
        }
        
        case ASTC_EXPR_CONSTANT: {
            if (!write_bytecode(bytecode, size, capacity, &expr->data.constant.type, sizeof(expr->data.constant.type)) ||
                !write_bytecode(bytecode, size, capacity, &expr->data.constant.int_val, sizeof(expr->data.constant.int_val))) {
                return false;
            }
            break;
        }
        
        case ASTC_EXPR_STRING_LITERAL: {
            size_t str_len = strlen(expr->data.string_literal.value);
            if (!write_bytecode(bytecode, size, capacity, &str_len, sizeof(str_len)) ||
                !write_bytecode(bytecode, size, capacity, expr->data.string_literal.value, str_len)) {
                return false;
            }
            break;
        }
        
        case ASTC_BINARY_OP: {
            if (!write_bytecode(bytecode, size, capacity, &expr->data.binary_op.op, sizeof(expr->data.binary_op.op)) ||
                !generate_expression_bytecode(expr->data.binary_op.left, ctx, bytecode, size, capacity) ||
                !generate_expression_bytecode(expr->data.binary_op.right, ctx, bytecode, size, capacity)) {
                return false;
            }
            break;
        }
        
        case ASTC_UNARY_OP: {
            if (!write_bytecode(bytecode, size, capacity, &expr->data.unary_op.op, sizeof(expr->data.unary_op.op)) ||
                !generate_expression_bytecode(expr->data.unary_op.operand, ctx, bytecode, size, capacity)) {
                return false;
            }
            break;
        }
        
        case ASTC_CALL_EXPR: {
            if (!generate_expression_bytecode(expr->data.call_expr.callee, ctx, bytecode, size, capacity)) {
                return false;
            }
            
            if (!write_bytecode(bytecode, size, capacity, &expr->data.call_expr.arg_count, sizeof(expr->data.call_expr.arg_count))) {
                return false;
            }
            
            for (int i = 0; i < expr->data.call_expr.arg_count; i++) {
                if (!generate_expression_bytecode(expr->data.call_expr.args[i], ctx, bytecode, size, capacity)) {
                    return false;
                }
            }
            break;
        }
        
        default:
            set_error("不支持的表达式类型: %d", expr->type);
            return false;
    }
    
    return true;
}

// 生成语句字节码
static bool generate_statement_bytecode(struct ASTNode* stmt, BytecodeContext* ctx,
                                      unsigned char** bytecode, size_t* size, size_t* capacity) {
    if (!stmt) return true;

    // 写入节点类型
    if (!write_bytecode(bytecode, size, capacity, &stmt->type, sizeof(stmt->type))) {
        return false;
    }

    // 根据节点类型生成相应的字节码
    switch (stmt->type) {
        case ASTC_COMPOUND_STMT: {
            if (!write_bytecode(bytecode, size, capacity, &stmt->data.compound_stmt.statement_count, 
                              sizeof(stmt->data.compound_stmt.statement_count))) {
                return false;
            }
            
            for (int i = 0; i < stmt->data.compound_stmt.statement_count; i++) {
                if (!generate_statement_bytecode(stmt->data.compound_stmt.statements[i], ctx, bytecode, size, capacity)) {
                    return false;
                }
            }
            break;
        }
        
        case ASTC_IF_STMT: {
            if (!generate_expression_bytecode(stmt->data.if_stmt.condition, ctx, bytecode, size, capacity) ||
                !generate_statement_bytecode(stmt->data.if_stmt.then_branch, ctx, bytecode, size, capacity) ||
                !generate_statement_bytecode(stmt->data.if_stmt.else_branch, ctx, bytecode, size, capacity)) {
                return false;
            }
            break;
        }
        
        case ASTC_WHILE_STMT: {
            if (!generate_expression_bytecode(stmt->data.while_stmt.condition, ctx, bytecode, size, capacity) ||
                !generate_statement_bytecode(stmt->data.while_stmt.body, ctx, bytecode, size, capacity)) {
                return false;
            }
            break;
        }
        
        case ASTC_FOR_STMT: {
            if (!generate_statement_bytecode(stmt->data.for_stmt.init, ctx, bytecode, size, capacity) ||
                !generate_expression_bytecode(stmt->data.for_stmt.condition, ctx, bytecode, size, capacity) ||
                !generate_expression_bytecode(stmt->data.for_stmt.increment, ctx, bytecode, size, capacity) ||
                !generate_statement_bytecode(stmt->data.for_stmt.body, ctx, bytecode, size, capacity)) {
                return false;
            }
            break;
        }
        
        case ASTC_RETURN_STMT: {
            if (!generate_expression_bytecode(stmt->data.return_stmt.value, ctx, bytecode, size, capacity)) {
                return false;
            }
            break;
        }
        
        case ASTC_EXPR_STMT: {
            if (!generate_expression_bytecode(stmt->data.expr_stmt.expr, ctx, bytecode, size, capacity)) {
                return false;
            }
            break;
        }
        
        default:
            set_error("不支持的语句类型: %d", stmt->type);
            return false;
    }
    
    return true;
}

// 生成字节码
bool generate_bytecode(struct ASTNode* ast, BytecodeContext* ctx, 
                      unsigned char** bytecode, size_t* size, size_t* capacity) {
    if (!ast || !ctx || !bytecode || !size || !capacity) {
        set_error("无效的参数");
        return false;
    }

    // 写入节点类型
    if (!write_bytecode(bytecode, size, capacity, &ast->type, sizeof(ast->type))) {
        return false;
    }

    // 根据节点类型生成相应的字节码
    switch (ast->type) {
        case ASTC_TRANSLATION_UNIT: {
            if (!write_bytecode(bytecode, size, capacity, &ast->data.translation_unit.declaration_count, 
                              sizeof(ast->data.translation_unit.declaration_count))) {
                return false;
            }
            
            for (int i = 0; i < ast->data.translation_unit.declaration_count; i++) {
                if (!generate_bytecode(ast->data.translation_unit.declarations[i], ctx, bytecode, size, capacity)) {
                    return false;
                }
            }
            break;
        }
        
        case ASTC_FUNC_DECL: {
            size_t name_len = strlen(ast->data.func_decl.name);
            if (!write_bytecode(bytecode, size, capacity, &name_len, sizeof(name_len)) ||
                !write_bytecode(bytecode, size, capacity, ast->data.func_decl.name, name_len)) {
                return false;
            }
            
            if (!write_bytecode(bytecode, size, capacity, &ast->data.func_decl.param_count, 
                              sizeof(ast->data.func_decl.param_count))) {
                return false;
            }
            
            for (int i = 0; i < ast->data.func_decl.param_count; i++) {
                if (!generate_bytecode(ast->data.func_decl.params[i], ctx, bytecode, size, capacity)) {
                    return false;
                }
            }
            
            if (!write_bytecode(bytecode, size, capacity, &ast->data.func_decl.has_body, 
                              sizeof(ast->data.func_decl.has_body))) {
                return false;
            }
            
            if (ast->data.func_decl.has_body) {
                if (!generate_statement_bytecode(ast->data.func_decl.body, ctx, bytecode, size, capacity)) {
                    return false;
                }
            }
            break;
        }
        
        case ASTC_VAR_DECL: {
            size_t name_len = strlen(ast->data.var_decl.name);
            if (!write_bytecode(bytecode, size, capacity, &name_len, sizeof(name_len)) ||
                !write_bytecode(bytecode, size, capacity, ast->data.var_decl.name, name_len)) {
                return false;
            }
            
            if (!generate_bytecode(ast->data.var_decl.type, ctx, bytecode, size, capacity) ||
                !generate_expression_bytecode(ast->data.var_decl.initializer, ctx, bytecode, size, capacity)) {
                return false;
            }
            break;
        }
        
        case ASTC_TYPE_SPECIFIER: {
            if (!write_bytecode(bytecode, size, capacity, &ast->data.type_specifier.type, 
                              sizeof(ast->data.type_specifier.type))) {
                return false;
            }
            break;
        }
        
        default:
            if (ast->type >= ASTC_EXPR_IDENTIFIER && ast->type <= ASTC_EXPR_CAST_EXPR) {
                return generate_expression_bytecode(ast, ctx, bytecode, size, capacity);
            } else if (ast->type >= ASTC_STMT_NONE && ast->type <= ASTC_STMT_ASM) {
                return generate_statement_bytecode(ast, ctx, bytecode, size, capacity);
            } else {
                set_error("不支持的节点类型: %d", ast->type);
                return false;
            }
    }
    
    return true;
}

// 将AST转换为ASTC字节码（带优化选项）
unsigned char* ast_to_astc_bytecode_with_options(struct ASTNode* ast, const C2AstcOptions* options, size_t* out_size) {
    if (!ast || !options || !out_size) {
        set_error("无效的参数");
        return NULL;
    }

    // 创建字节码生成器上下文
    BytecodeContext ctx = {
        .optimize_level = options->optimize_level,
        .enable_extensions = options->enable_extensions,
        .emit_debug_info = options->emit_debug_info,
        .symbols = {0},
        .bytecode = {0},
        .debug_info = {0}
    };

    // 初始化字节码缓冲区
    unsigned char *bytecode = NULL;
    size_t capacity = 1024;
    size_t size = 0;

    bytecode = (unsigned char *)malloc(capacity);
    if (!bytecode) {
        set_error("内存分配失败");
        return NULL;
    }

    // 写入ASTC魔数和版本号
    const char *magic = "ASTC";
    memcpy(bytecode, magic, 4);
    size += 4;

    uint32_t version = 1;
    memcpy(bytecode + size, &version, sizeof(version));
    size += sizeof(version);

    // 遍历AST并生成字节码
    if (!generate_bytecode(ast, &ctx, &bytecode, &size, &capacity)) {
        free(bytecode);
        return NULL;
    }

    // 更新输出大小
    *out_size = size;
    return bytecode;
}

// Lexer implementation
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
    while (1) {
        char c = peek_char(lexer);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance_char(lexer);
        } else {
            break;
        }
    }
}

static void skip_comment(Lexer *lexer) {
    if (peek_char(lexer) == '/') {
        advance_char(lexer);
        if (peek_char(lexer) == '/') {
            // 单行注释
            advance_char(lexer);
            while (peek_char(lexer) != '\0' && peek_char(lexer) != '\n') {
                advance_char(lexer);
            }
        } else if (peek_char(lexer) == '*') {
            // 多行注释
            advance_char(lexer);
            while (peek_char(lexer) != '\0') {
                if (peek_char(lexer) == '*') {
                    advance_char(lexer);
                    if (peek_char(lexer) == '/') {
                        advance_char(lexer);
                        break;
                    }
                } else {
                    advance_char(lexer);
                }
            }
        }
    }
}

static bool is_identifier_char(char c) {
    return isalnum(c) || c == '_';
}

static Token* create_token(TokenType type, const char *value, int line, int column) {
    Token *token = (Token *)malloc(sizeof(Token));
    if (!token) {
        set_error("内存分配失败");
        return NULL;
    }
    
    token->type = type;
    token->value = strdup(value);
    if (!token->value) {
        free(token);
        set_error("内存分配失败");
        return NULL;
    }
    
    token->line = line;
    token->column = column;
    
    return token;
}

// 获取标识符
static Token* get_identifier(Lexer *lexer) {
    char buffer[256];
    int length = 0;
    
    while (isalnum(peek_char(lexer)) || peek_char(lexer) == '_') {
        if (length >= 255) {
            set_error("标识符过长");
            return NULL;
        }
        buffer[length++] = advance_char(lexer);
    }
    
    buffer[length] = '\0';
    
    // 检查是否是关键字
    TokenType type = TOKEN_IDENTIFIER;
    if (strcmp(buffer, "if") == 0) type = TOKEN_IF;
    else if (strcmp(buffer, "else") == 0) type = TOKEN_ELSE;
    else if (strcmp(buffer, "while") == 0) type = TOKEN_WHILE;
    else if (strcmp(buffer, "for") == 0) type = TOKEN_FOR;
    else if (strcmp(buffer, "return") == 0) type = TOKEN_RETURN;
    else if (strcmp(buffer, "break") == 0) type = TOKEN_BREAK;
    else if (strcmp(buffer, "continue") == 0) type = TOKEN_CONTINUE;
    else if (strcmp(buffer, "struct") == 0) type = TOKEN_STRUCT;
    else if (strcmp(buffer, "union") == 0) type = TOKEN_UNION;
    else if (strcmp(buffer, "enum") == 0) type = TOKEN_ENUM;
    else if (strcmp(buffer, "typedef") == 0) type = TOKEN_TYPEDEF;
    else if (strcmp(buffer, "static") == 0) type = TOKEN_STATIC;
    else if (strcmp(buffer, "extern") == 0) type = TOKEN_EXTERN;
    else if (strcmp(buffer, "const") == 0) type = TOKEN_CONST;
    else if (strcmp(buffer, "void") == 0) type = TOKEN_VOID;
    else if (strcmp(buffer, "char") == 0) type = TOKEN_CHAR;
    else if (strcmp(buffer, "int") == 0) type = TOKEN_INT;
    else if (strcmp(buffer, "float") == 0) type = TOKEN_FLOAT;
    else if (strcmp(buffer, "double") == 0) type = TOKEN_DOUBLE;
    else if (strcmp(buffer, "module") == 0) type = TOKEN_MODULE;
    else if (strcmp(buffer, "import") == 0) type = TOKEN_IMPORT;
    else if (strcmp(buffer, "export") == 0) type = TOKEN_EXPORT;
    
    return create_token(type, buffer, lexer->line, lexer->column - length);
}

// 获取数字
static Token* get_number(Lexer *lexer) {
    char buffer[64];
    int length = 0;
    bool is_float = false;
    
    while (isdigit(peek_char(lexer)) || peek_char(lexer) == '.') {
        if (length >= 63) {
            set_error("数字过长");
            return NULL;
        }
        
        char c = peek_char(lexer);
        if (c == '.') {
            if (is_float) {
                set_error("数字中有多个小数点");
                return NULL;
            }
            is_float = true;
        }
        
        buffer[length++] = advance_char(lexer);
    }
    
    buffer[length] = '\0';
    Token *token = create_token(TOKEN_NUMBER, buffer, lexer->line, lexer->column - length);
    if (!token) return NULL;
    
    return token;
}

// 获取字符串字面量
static Token* get_string(Lexer *lexer) {
    char buffer[1024];
    int length = 0;
    
    while (peek_char(lexer) != '"' && peek_char(lexer) != '\0') {
        if (length >= 1023) {
            set_error("字符串过长");
            return NULL;
        }
        buffer[length++] = advance_char(lexer);
    }
    
    if (peek_char(lexer) == '\0') {
        set_error("未终止的字符串");
        return NULL;
    }
    
    advance_char(lexer); // 跳过结束的引号
    buffer[length] = '\0';
    
    return create_token(TOKEN_STRING_LITERAL, buffer, lexer->line, lexer->column - length - 2);
}

// 词法分析
static bool tokenize(const char *source, Token ***tokens, int *token_count) {
    *token_count = 0;
    
    // 分配初始token数组
    int capacity = 16;
    *tokens = (Token **)malloc(sizeof(Token *) * capacity);
    if (!*tokens) {
        set_error("内存分配失败");
        return false;
    }
    
    Lexer *lexer = (Lexer *)malloc(sizeof(Lexer));
    if (!lexer) {
        free(*tokens);
        set_error("内存分配失败");
        return false;
    }
    init_lexer(lexer, source);
    
    Token *token = NULL;
    char c;
    while ((c = peek_char(lexer)) != '\0') {
        skip_whitespace(lexer);
        c = peek_char(lexer);
        if (c == '\0') break;

        // Debug output (commented out for production)
        // printf("Processing character: '%c' (ASCII: %d) at line %d, column %d\n",
        //        c, (int)c, lexer->line, lexer->column);

        switch (c) {
            case '(':
                advance_char(lexer);
                token = create_token(TOKEN_LPAREN, "(", lexer->line, lexer->column - 1);
                break;
            case ')':
                advance_char(lexer);
                token = create_token(TOKEN_RPAREN, ")", lexer->line, lexer->column - 1);
                break;
            case '{':
                advance_char(lexer);
                token = create_token(TOKEN_LBRACE, "{", lexer->line, lexer->column - 1);
                break;
            case '}':
                advance_char(lexer);
                token = create_token(TOKEN_RBRACE, "}", lexer->line, lexer->column - 1);
                break;
            case '[':
                advance_char(lexer);
                token = create_token(TOKEN_LBRACKET, "[", lexer->line, lexer->column - 1);
                break;
            case ']':
                advance_char(lexer);
                token = create_token(TOKEN_RBRACKET, "]", lexer->line, lexer->column - 1);
                break;
            case ';':
                advance_char(lexer);
                token = create_token(TOKEN_SEMICOLON, ";", lexer->line, lexer->column - 1);
                break;
            case ',':
                advance_char(lexer);
                token = create_token(TOKEN_COMMA, ",", lexer->line, lexer->column - 1);
                break;
            case '.':
                advance_char(lexer);
                token = create_token(TOKEN_DOT, ".", lexer->line, lexer->column - 1);
                break;
            case '+':
                advance_char(lexer);
                token = create_token(TOKEN_PLUS, "+", lexer->line, lexer->column - 1);
                if (!token) {
                    free(lexer);
                    return false;
                }
                if (peek_char(lexer) == '+') {
                    advance_char(lexer);
                    free(token->value);
                    token->value = strdup("++");
                    token->type = TOKEN_INC;
                } else if (peek_char(lexer) == '=') {
                    advance_char(lexer);
                    free(token->value);
                    token->value = strdup("+=");
                    token->type = TOKEN_ADD_ASSIGN;
                }
                break;
            case '-':
                advance_char(lexer);
                token = create_token(TOKEN_MINUS, "-", lexer->line, lexer->column - 1);
                if (!token) {
                    free(lexer);
                    return false;
                }
                if (peek_char(lexer) == '-') {
                    advance_char(lexer);
                    free(token->value);
                    token->value = strdup("--");
                    token->type = TOKEN_DEC;
                } else if (peek_char(lexer) == '=') {
                    advance_char(lexer);
                    free(token->value);
                    token->value = strdup("-=");
                    token->type = TOKEN_SUB_ASSIGN;
                } else if (peek_char(lexer) == '>') {
                    advance_char(lexer);
                    free(token->value);
                    token->value = strdup("->");
                    token->type = TOKEN_ARROW;
                }
                break;
            case '*':
                advance_char(lexer);
                token = create_token(TOKEN_STAR, "*", lexer->line, lexer->column - 1);
                if (!token) {
                    free(lexer);
                    return false;
                }
                if (peek_char(lexer) == '=') {
                    advance_char(lexer);
                    free(token->value);
                    token->value = strdup("*=");
                    token->type = TOKEN_MUL_ASSIGN;
                }
                break;
            case '/':
                advance_char(lexer);
                token = create_token(TOKEN_SLASH, "/", lexer->line, lexer->column - 1);
                if (!token) {
                    free(lexer);
                    return false;
                }
                if (peek_char(lexer) == '=') {
                    advance_char(lexer);
                    free(token->value);
                    token->value = strdup("/=");
                    token->type = TOKEN_DIV_ASSIGN;
                } else if (peek_char(lexer) == '/') {
                    // Skip single-line comment
                    while (peek_char(lexer) != '\n' && peek_char(lexer) != '\0') {
                        advance_char(lexer);
                    }
                    free(token);
                    continue;
                } else if (peek_char(lexer) == '*') {
                    // Skip multi-line comment
                    advance_char(lexer);
                    while (peek_char(lexer) != '\0') {
                        if (peek_char(lexer) == '*') {
                            advance_char(lexer);
                            if (peek_char(lexer) == '/') {
                                advance_char(lexer);
                                break;
                            }
                        } else {
                            advance_char(lexer);
                        }
                    }
                    free(token);
                    continue;
                }
                break;
            case '%':
                advance_char(lexer);
                token = create_token(TOKEN_PERCENT, "%", lexer->line, lexer->column - 1);
                if (!token) {
                    free(lexer);
                    return false;
                }
                if (peek_char(lexer) == '=') {
                    advance_char(lexer);
                    free(token->value);
                    token->value = strdup("%=");
                    token->type = TOKEN_MOD_ASSIGN;
                }
                break;
            case '=':
                advance_char(lexer);
                token = create_token(TOKEN_ASSIGN, "=", lexer->line, lexer->column - 1);
                if (!token) {
                    free(lexer);
                    return false;
                }
                if (peek_char(lexer) == '=') {
                    advance_char(lexer);
                    free(token->value);
                    token->value = strdup("==");
                    token->type = TOKEN_EQ;
                }
                break;
            case '<':
                advance_char(lexer);
                token = create_token(TOKEN_LT, "<", lexer->line, lexer->column - 1);
                if (!token) {
                    free(lexer);
                    return false;
                }
                if (peek_char(lexer) == '=') {
                    advance_char(lexer);
                    free(token->value);
                    token->value = strdup("<=");
                    token->type = TOKEN_LE;
                } else if (peek_char(lexer) == '<') {
                    advance_char(lexer);
                    free(token->value);
                    token->value = strdup("<<");
                    token->type = TOKEN_SHL;
                    if (peek_char(lexer) == '=') {
                        advance_char(lexer);
                        free(token->value);
                        token->value = strdup("<<=");
                        token->type = TOKEN_SHL_ASSIGN;
                    }
                }
                break;
            case '>':
                advance_char(lexer);
                token = create_token(TOKEN_GT, ">", lexer->line, lexer->column - 1);
                if (!token) {
                    free(lexer);
                    return false;
                }
                if (peek_char(lexer) == '=') {
                    advance_char(lexer);
                    free(token->value);
                    token->value = strdup(">=");
                    token->type = TOKEN_GE;
                } else if (peek_char(lexer) == '>') {
                    advance_char(lexer);
                    free(token->value);
                    token->value = strdup(">>");
                    token->type = TOKEN_SHR;
                    if (peek_char(lexer) == '=') {
                        advance_char(lexer);
                        free(token->value);
                        token->value = strdup(">>=");
                        token->type = TOKEN_SHR_ASSIGN;
                    }
                }
                break;
            case '!':
                advance_char(lexer);
                token = create_token(TOKEN_BANG, "!", lexer->line, lexer->column - 1);
                if (!token) {
                    free(lexer);
                    return false;
                }
                if (peek_char(lexer) == '=') {
                    advance_char(lexer);
                    free(token->value);
                    token->value = strdup("!=");
                    token->type = TOKEN_NE;
                }
                break;
            case '&':
                advance_char(lexer);
                token = create_token(TOKEN_AMPERSAND, "&", lexer->line, lexer->column - 1);
                if (!token) {
                    free(lexer);
                    return false;
                }
                if (peek_char(lexer) == '&') {
                    advance_char(lexer);
                    free(token->value);
                    token->value = strdup("&&");
                    token->type = TOKEN_LOGICAL_AND;
                } else if (peek_char(lexer) == '=') {
                    advance_char(lexer);
                    free(token->value);
                    token->value = strdup("&=");
                    token->type = TOKEN_AND_ASSIGN;
                }
                break;
            case '|':
                advance_char(lexer);
                token = create_token(TOKEN_PIPE, "|", lexer->line, lexer->column - 1);
                if (!token) {
                    free(lexer);
                    return false;
                }
                if (peek_char(lexer) == '|') {
                    advance_char(lexer);
                    free(token->value);
                    token->value = strdup("||");
                    token->type = TOKEN_LOGICAL_OR;
                } else if (peek_char(lexer) == '=') {
                    advance_char(lexer);
                    free(token->value);
                    token->value = strdup("|=");
                    token->type = TOKEN_OR_ASSIGN;
                }
                break;
            case '^':
                advance_char(lexer);
                token = create_token(TOKEN_CARET, "^", lexer->line, lexer->column - 1);
                if (!token) {
                    free(lexer);
                    return false;
                }
                if (peek_char(lexer) == '=') {
                    advance_char(lexer);
                    free(token->value);
                    token->value = strdup("^=");
                    token->type = TOKEN_XOR_ASSIGN;
                }
                break;
            case '~':
                advance_char(lexer);
                token = create_token(TOKEN_TILDE, "~", lexer->line, lexer->column - 1);
                break;
            case '?':
                advance_char(lexer);
                token = create_token(TOKEN_QUESTION, "?", lexer->line, lexer->column - 1);
                break;
            case ':':
                advance_char(lexer);
                token = create_token(TOKEN_COLON, ":", lexer->line, lexer->column - 1);
                break;
            
            case '"':
                token = get_string(lexer);
                break;
                
            default:
                if (isdigit(c)) {
                    token = get_number(lexer);
                } else if (isalpha(c) || c == '_') {
                    token = get_identifier(lexer);
                } else {
                    free(lexer);
                    set_error("无效的字符: %c (ASCII: %d) at line %d, column %d", c, (int)c, lexer->line, lexer->column);
                    return false;
                }
                break;
        }
        
        if (!token) {
            free(lexer);
            return false;
        }
        
        // 检查是否需要扩展数组
        if (*token_count >= capacity) {
            capacity *= 2;
            Token **new_tokens = (Token **)realloc(*tokens, sizeof(Token *) * capacity);
            if (!new_tokens) {
                free(lexer);
                set_error("内存分配失败");
                return false;
            }
            *tokens = new_tokens;
        }
        
        (*tokens)[(*token_count)++] = token;
    }
    
    // 添加文件结束标记
    Token *eof_token = create_token(TOKEN_EOF, "", lexer->line, lexer->column);
    if (!eof_token) {
        free(lexer);
        return false;
    }
    
    if (*token_count >= capacity) {
        capacity *= 2;
        Token **new_tokens = (Token **)realloc(*tokens, sizeof(Token *) * capacity);
        if (!new_tokens) {
            free(lexer);
            free(eof_token);
            set_error("内存分配失败");
            return false;
        }
        *tokens = new_tokens;
    }
    
    (*tokens)[(*token_count)++] = eof_token;
    
    free(lexer);
    return true;
}

// 解析翻译单元
static struct ASTNode* parse_translation_unit(Parser *parser) {
    struct ASTNode *node = ast_create_node(ASTC_TRANSLATION_UNIT, 1, 1);
    if (!node) return NULL;
    
    // 分配初始声明数组
    int capacity = 16;
    node->data.translation_unit.declarations = (struct ASTNode **)malloc(sizeof(struct ASTNode *) * capacity);
    if (!node->data.translation_unit.declarations) {
        ast_free(node);
        set_error("内存分配失败");
        return NULL;
    }
    
    node->data.translation_unit.declaration_count = 0;
    
    // 解析所有顶层声明
    while (peek(parser) && peek(parser)->type != TOKEN_EOF) {
        struct ASTNode *decl = parse_declaration(parser);
        if (!decl) {
            ast_free(node);
            return NULL;
        }
        
        // 检查是否需要扩展数组
        if (node->data.translation_unit.declaration_count >= capacity) {
            capacity *= 2;
            struct ASTNode **new_decls = (struct ASTNode **)realloc(
                node->data.translation_unit.declarations,
                sizeof(struct ASTNode *) * capacity
            );
            if (!new_decls) {
                ast_free(node);
                ast_free(decl);
                set_error("内存分配失败");
                return NULL;
            }
            node->data.translation_unit.declarations = new_decls;
        }
        
        node->data.translation_unit.declarations[node->data.translation_unit.declaration_count++] = decl;
    }
    
    return node;
}

// 解析声明
static struct ASTNode* parse_declaration(Parser *parser) {
    Token *token = peek(parser);
    if (!token) {
        parser_error(parser, "预期声明");
        return NULL;
    }
    
    switch (token->type) {
        case TOKEN_MODULE:
            return parse_module_statement(parser);
            
        case TOKEN_IMPORT:
            return parse_import_statement(parser);
            
        case TOKEN_EXPORT:
            return parse_export_statement(parser);
            
        default: {
            // 解析类型说明符
            struct ASTNode *type = parse_type_specifier(parser);
            if (!type) return NULL;
            
            // 解析声明符
            token = peek(parser);
            if (!token || token->type != TOKEN_IDENTIFIER) {
                ast_free(type);
                parser_error(parser, "预期标识符");
                return NULL;
            }
            
            char *name = strdup(token->value);
            if (!name) {
                ast_free(type);
                set_error("内存分配失败");
                return NULL;
            }
            
            advance(parser);
            
            // 检查是否是函数声明
            if (match(parser, TOKEN_LPAREN)) {
                struct ASTNode *func = parse_function_declaration(parser, type, name);
                free(name);
                return func;
            }
            
            // 变量声明
            struct ASTNode *var = ast_create_node(ASTC_VAR_DECL, token->line, token->column);
            if (!var) {
                ast_free(type);
                free(name);
                return NULL;
            }
            
            var->data.var_decl.name = name;
            var->data.var_decl.type = type;
            
            // 检查是否有初始化器
            if (match(parser, TOKEN_ASSIGN)) {
                var->data.var_decl.initializer = parse_expression(parser);
                if (!var->data.var_decl.initializer) {
                    ast_free(var);
                    return NULL;
                }
            } else {
                var->data.var_decl.initializer = NULL;
            }
            
            // 检查分号
            if (!match(parser, TOKEN_SEMICOLON)) {
                ast_free(var);
                parser_error(parser, "预期分号");
                return NULL;
            }
            
            return var;
        }
    }
}

// 解析类型说明符
static struct ASTNode* parse_type_specifier(Parser *parser) {
    Token *token = peek(parser);
    if (!token) {
        parser_error(parser, "预期类型说明符");
        return NULL;
    }
    
    struct ASTNode *type = ast_create_node(ASTC_TYPE_SPECIFIER, token->line, token->column);
    if (!type) return NULL;
    
    switch (token->type) {
        case TOKEN_VOID:
            type->data.type_specifier.type = ASTC_TYPE_VOID;
            break;
            
        case TOKEN_INT:
            type->data.type_specifier.type = ASTC_TYPE_INT;
            break;
            
        case TOKEN_CHAR:
            type->data.type_specifier.type = ASTC_TYPE_CHAR;
            break;
            
        case TOKEN_SHORT:
            type->data.type_specifier.type = ASTC_TYPE_SHORT;
            break;
            
        case TOKEN_LONG:
            type->data.type_specifier.type = ASTC_TYPE_LONG;
            break;
            
        case TOKEN_FLOAT:
            type->data.type_specifier.type = ASTC_TYPE_FLOAT;
            break;
            
        case TOKEN_DOUBLE:
            type->data.type_specifier.type = ASTC_TYPE_DOUBLE;
            break;
            
        case TOKEN_SIGNED:
            type->data.type_specifier.type = ASTC_TYPE_SIGNED;
            break;
            
        case TOKEN_UNSIGNED:
            type->data.type_specifier.type = ASTC_TYPE_UNSIGNED;
            break;
            
        case TOKEN_STRUCT:
            ast_free(type);
            return parse_struct_or_union(parser);
            
        case TOKEN_UNION:
            ast_free(type);
            return parse_struct_or_union(parser);
            
        case TOKEN_ENUM:
            ast_free(type);
            return parse_enum(parser);
            
        default:
            ast_free(type);
            parser_error(parser, "无效的类型说明符");
            return NULL;
    }
    
    advance(parser);
    
    // 检查是否是指针类型
    while (match(parser, TOKEN_STAR)) {
        type = parse_pointer_type(parser, type);
        if (!type) return NULL;
    }
    
    return type;
}

// 解析函数声明
static struct ASTNode* parse_function_declaration(Parser *parser, struct ASTNode *return_type, char *name) {
    struct ASTNode *func = ast_create_node(ASTC_FUNC_DECL, parser->tokens[parser->current - 1]->line,
                                         parser->tokens[parser->current - 1]->column);
    if (!func) {
        ast_free(return_type);
        return NULL;
    }
    
    func->data.func_decl.name = name;
    func->data.func_decl.return_type = return_type;
    func->data.func_decl.params = NULL;
    func->data.func_decl.param_count = 0;
    func->data.func_decl.has_body = 0;
    func->data.func_decl.body = NULL;
    
    // 解析参数列表
    int capacity = 8;
    func->data.func_decl.params = (struct ASTNode **)malloc(sizeof(struct ASTNode *) * capacity);
    if (!func->data.func_decl.params) {
        ast_free(func);
        set_error("内存分配失败");
        return NULL;
    }
    
    while (!match(parser, TOKEN_RPAREN)) {
        if (func->data.func_decl.param_count > 0) {
            if (!match(parser, TOKEN_COMMA)) {
                ast_free(func);
                parser_error(parser, "预期逗号");
                return NULL;
            }
        }
        
        struct ASTNode *param = parse_parameter_declaration(parser);
        if (!param) {
            ast_free(func);
            return NULL;
        }
        
        // 检查是否需要扩展数组
        if (func->data.func_decl.param_count >= capacity) {
            capacity *= 2;
            struct ASTNode **new_params = (struct ASTNode **)realloc(
                func->data.func_decl.params,
                sizeof(struct ASTNode *) * capacity
            );
            if (!new_params) {
                ast_free(func);
                ast_free(param);
                set_error("内存分配失败");
                return NULL;
            }
            func->data.func_decl.params = new_params;
        }
        
        func->data.func_decl.params[func->data.func_decl.param_count++] = param;
    }
    
    // 检查是否有函数体
    if (match(parser, TOKEN_LBRACE)) {
        func->data.func_decl.has_body = 1;
        func->data.func_decl.body = parse_compound_statement(parser);
        if (!func->data.func_decl.body) {
            ast_free(func);
            return NULL;
        }
    } else {
        // 函数声明必须以分号结束
        if (!match(parser, TOKEN_SEMICOLON)) {
            ast_free(func);
            parser_error(parser, "预期分号");
            return NULL;
        }
    }
    
    return func;
}

// 解析参数声明
static struct ASTNode* parse_parameter_declaration(Parser *parser) {
    struct ASTNode *type = parse_type_specifier(parser);
    if (!type) return NULL;
    
    Token *token = peek(parser);
    if (!token || token->type != TOKEN_IDENTIFIER) {
        ast_free(type);
        parser_error(parser, "预期参数名");
        return NULL;
    }
    
    struct ASTNode *param = ast_create_node(ASTC_PARAM_DECL, token->line, token->column);
    if (!param) {
        ast_free(type);
        return NULL;
    }
    
    param->data.var_decl.name = strdup(token->value);
    if (!param->data.var_decl.name) {
        ast_free(type);
        ast_free(param);
        set_error("内存分配失败");
        return NULL;
    }
    
    param->data.var_decl.type = type;
    param->data.var_decl.initializer = NULL;
    
    advance(parser);
    return param;
}

// 将C源代码转换为ASTC表示
struct ASTNode* c2astc_convert(const char *source, const C2AstcOptions *options) {
    if (!source || !options) {
        set_error("无效的参数");
        return NULL;
    }

    // 初始化词法分析器
    Token **tokens = NULL;
    int token_count = 0;
    printf("Starting tokenization...\n");
    if (!tokenize(source, &tokens, &token_count)) {
        printf("Tokenization failed. Error: %s\n", c2astc_get_error());
        return NULL;
    }
    printf("Tokenization successful. Token count: %d\n", token_count);

    // Debug: Print all tokens (commented out for production)
    // for (int i = 0; i < token_count; i++) {
    //     printf("Token %d: type=%d, value='%s', line=%d, col=%d\n",
    //            i, tokens[i]->type, tokens[i]->value ? tokens[i]->value : "(null)",
    //            tokens[i]->line, tokens[i]->column);
    // }

    // 初始化解析器
    Parser parser = {
        .tokens = tokens,
        .token_count = token_count,
        .current = 0,
        .error_count = 0,
        .symbols = {0}
    };

    printf("Starting parsing...\n");
    // 解析源代码
    struct ASTNode *ast = parse_translation_unit(&parser);

    if (!ast) {
        printf("Parsing failed. Error: %s\n", c2astc_get_error());
    } else {
        printf("Parsing successful.\n");
    }

    // 释放词法分析结果
    for (int i = 0; i < token_count; i++) {
        free(tokens[i]);
    }
    free(tokens);

    // 检查是否有错误
    if (parser.error_count > 0) {
        if (ast) ast_free(ast);
        return NULL;
    }

    return ast;
}

// ===============================================
// AST节点管理函数实现
// ===============================================

struct ASTNode* ast_create_node(ASTNodeType type, int line, int column) {
    struct ASTNode* node = (struct ASTNode*)calloc(1, sizeof(struct ASTNode));
    if (!node) {
        set_error("内存分配失败");
        return NULL;
    }

    node->type = type;
    node->line = line;
    node->column = column;

    return node;
}

void ast_free(struct ASTNode *node) {
    if (!node) return;

    // 根据节点类型释放特定的数据
    switch (node->type) {
        case ASTC_EXPR_IDENTIFIER:
            free(node->data.identifier.name);
            break;

        case ASTC_EXPR_STRING_LITERAL:
            free(node->data.string_literal.value);
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
            for (int i = 0; i < node->data.call_expr.arg_count; i++) {
                ast_free(node->data.call_expr.args[i]);
            }
            free(node->data.call_expr.args);
            break;

        case ASTC_TRANSLATION_UNIT:
            for (int i = 0; i < node->data.translation_unit.declaration_count; i++) {
                ast_free(node->data.translation_unit.declarations[i]);
            }
            free(node->data.translation_unit.declarations);
            break;

        case ASTC_FUNC_DECL:
            free(node->data.func_decl.name);
            ast_free(node->data.func_decl.return_type);
            for (int i = 0; i < node->data.func_decl.param_count; i++) {
                ast_free(node->data.func_decl.params[i]);
            }
            free(node->data.func_decl.params);
            ast_free(node->data.func_decl.body);
            break;

        case ASTC_VAR_DECL:
        case ASTC_PARAM_DECL:
            free(node->data.var_decl.name);
            ast_free(node->data.var_decl.type);
            ast_free(node->data.var_decl.initializer);
            break;

        case AST_MODULE:
            free(node->data.module_decl.name);
            free(node->data.module_decl.version);
            free(node->data.module_decl.author);
            break;

        case AST_IMPORT:
            free(node->data.import_decl.module_name);
            free(node->data.import_decl.import_name);
            free(node->data.import_decl.local_name);
            break;

        case AST_EXPORT:
            free(node->data.export_decl.name);
            free(node->data.export_decl.alias);
            break;

        // 对于其他节点类型，使用默认处理
        default:
            // 简单节点类型不需要特殊处理
            break;
    }

    free(node);
}

// ===============================================
// 缺失的解析函数实现
// ===============================================

// 解析表达式 (简化实现)
static struct ASTNode* parse_expression(Parser* parser) {
    // 简化实现：只解析基本表达式
    return parse_assignment(parser);
}

// 解析赋值表达式
static struct ASTNode* parse_assignment(Parser* parser) {
    // 简化实现：直接解析逻辑或表达式
    return parse_logical_or(parser);
}

// 解析逻辑或表达式
static struct ASTNode* parse_logical_or(Parser* parser) {
    return parse_logical_and(parser);
}

// 解析逻辑与表达式
static struct ASTNode* parse_logical_and(Parser* parser) {
    return parse_equality(parser);
}

// 解析相等性表达式
static struct ASTNode* parse_equality(Parser* parser) {
    return parse_relational(parser);
}

// 解析关系表达式
static struct ASTNode* parse_relational(Parser* parser) {
    return parse_additive(parser);
}

// 解析加法表达式
static struct ASTNode* parse_additive(Parser* parser) {
    return parse_multiplicative(parser);
}

// 解析乘法表达式
static struct ASTNode* parse_multiplicative(Parser* parser) {
    return parse_unary(parser);
}

// 解析一元表达式
static struct ASTNode* parse_unary(Parser* parser) {
    return parse_primary(parser);
}

// 解析基本表达式
static struct ASTNode* parse_primary(Parser* parser) {
    Token* token = peek(parser);
    if (!token) {
        parser_error(parser, "预期表达式");
        return NULL;
    }

    switch (token->type) {
        case TOKEN_IDENTIFIER: {
            struct ASTNode* node = ast_create_node(ASTC_EXPR_IDENTIFIER, token->line, token->column);
            if (node) {
                node->data.identifier.name = strdup(token->value);
            }
            advance(parser);
            return node;
        }

        case TOKEN_NUMBER: {
            struct ASTNode* node = ast_create_node(ASTC_EXPR_CONSTANT, token->line, token->column);
            if (node) {
                node->data.constant.type = ASTC_TYPE_INT;
                node->data.constant.int_val = atoll(token->value);
            }
            advance(parser);
            return node;
        }

        case TOKEN_STRING_LITERAL: {
            struct ASTNode* node = ast_create_node(ASTC_EXPR_STRING_LITERAL, token->line, token->column);
            if (node) {
                node->data.string_literal.value = strdup(token->value);
            }
            advance(parser);
            return node;
        }

        default:
            parser_error(parser, "无效的表达式");
            return NULL;
    }
}

// 解析结构体或联合体 (简化实现)
static struct ASTNode* parse_struct_or_union(Parser* parser) {
    Token* token = peek(parser);
    if (!token) {
        parser_error(parser, "预期struct或union");
        return NULL;
    }

    struct ASTNode* node = ast_create_node(ASTC_TYPE_STRUCT, token->line, token->column);
    if (!node) return NULL;

    advance(parser); // 跳过struct/union关键字

    // 简化实现：跳过结构体定义
    if (match(parser, TOKEN_LBRACE)) {
        // 跳过到匹配的右大括号
        int brace_count = 1;
        while (brace_count > 0 && peek(parser)) {
            Token* t = advance(parser);
            if (t->type == TOKEN_LBRACE) brace_count++;
            else if (t->type == TOKEN_RBRACE) brace_count--;
        }
    }

    return node;
}

// 解析枚举 (简化实现)
static struct ASTNode* parse_enum(Parser* parser) {
    Token* token = peek(parser);
    if (!token) {
        parser_error(parser, "预期enum");
        return NULL;
    }

    struct ASTNode* node = ast_create_node(ASTC_TYPE_ENUM, token->line, token->column);
    if (!node) return NULL;

    advance(parser); // 跳过enum关键字

    // 简化实现：跳过枚举定义
    if (match(parser, TOKEN_LBRACE)) {
        int brace_count = 1;
        while (brace_count > 0 && peek(parser)) {
            Token* t = advance(parser);
            if (t->type == TOKEN_LBRACE) brace_count++;
            else if (t->type == TOKEN_RBRACE) brace_count--;
        }
    }

    return node;
}

// 解析指针类型 (简化实现)
static struct ASTNode* parse_pointer_type(Parser* parser, struct ASTNode* base_type) {
    struct ASTNode* node = ast_create_node(ASTC_TYPE_POINTER, 0, 0);
    if (!node) {
        ast_free(base_type);
        return NULL;
    }

    // 简化实现：不处理复杂的指针类型
    return node;
}

// 解析复合语句 (简化实现)
static struct ASTNode* parse_compound_statement(Parser* parser) {
    if (!match(parser, TOKEN_LBRACE)) {
        parser_error(parser, "预期'{'");
        return NULL;
    }

    struct ASTNode* node = ast_create_node(ASTC_COMPOUND_STMT, 0, 0);
    if (!node) return NULL;

    // 简化实现：跳过到匹配的右大括号
    int brace_count = 1;
    while (brace_count > 0 && peek(parser)) {
        Token* t = advance(parser);
        if (t->type == TOKEN_LBRACE) brace_count++;
        else if (t->type == TOKEN_RBRACE) brace_count--;
    }

    return node;
}

// 其他缺失的解析函数 (简化实现)
static struct ASTNode* parse_statement(Parser* parser) {
    // 简化实现：返回空语句
    return ast_create_node(ASTC_STMT_NONE, 0, 0);
}

static struct ASTNode* parse_expression_statement(Parser* parser) {
    return parse_expression(parser);
}

static struct ASTNode* parse_if_statement(Parser* parser) {
    return ast_create_node(ASTC_IF_STMT, 0, 0);
}

static struct ASTNode* parse_while_statement(Parser* parser) {
    return ast_create_node(ASTC_WHILE_STMT, 0, 0);
}

static struct ASTNode* parse_for_statement(Parser* parser) {
    return ast_create_node(ASTC_FOR_STMT, 0, 0);
}

static struct ASTNode* parse_return_statement(Parser* parser) {
    return ast_create_node(ASTC_RETURN_STMT, 0, 0);
}

static struct ASTNode* parse_array_type(Parser* parser, struct ASTNode* element_type) {
    return ast_create_node(ASTC_TYPE_ARRAY, 0, 0);
}

static struct ASTNode* parse_function_type(Parser* parser, struct ASTNode* return_type) {
    return ast_create_node(ASTC_TYPE_FUNCTION, 0, 0);
}
