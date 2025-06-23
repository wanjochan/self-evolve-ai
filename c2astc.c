/**
 * c2astc.c - C语言到ASTC的转换库
 * 整合了evolver0_lexer.inc.c、evolver0_parser.inc.c和evolver0_ast.inc.c的有用代码
 * 使用ASTC作为核心数据结构
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>

// 包含AST定义
#include "astc.h"
#include "c2astc.h"
#include "evolver0_token.h"

// ===============================================
// 错误处理
// ===============================================

static char last_error[256] = {0};

static void set_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(last_error, sizeof(last_error), format, args);
    va_end(args);
}

const char* c2astc_get_error(void) {
    return last_error[0] ? last_error : NULL;
}

// ===============================================
// 内存管理
// ===============================================

void c2astc_free(void *ptr) {
    free(ptr);
}

// ===============================================
// 词法分析器
// ===============================================

// 词法分析器上下文
typedef struct {
    const char *source;
    size_t pos;
    size_t length;
    int line;
    int column;
    const char *filename;
    
    // 错误处理
    char error_msg[256];
    int error_count;
    
    // 预处理器状态
    int in_preprocessor;
    int in_include;
} Lexer;

// 初始化词法分析器
static void init_lexer(Lexer *lexer, const char *source, const char *filename) {
    lexer->source = source;
    lexer->pos = 0;
    lexer->length = strlen(source);
    lexer->line = 1;
    lexer->column = 1;
    lexer->filename = filename ? filename : "<input>";
    lexer->error_msg[0] = '\0';
    lexer->error_count = 0;
    lexer->in_preprocessor = 0;
    lexer->in_include = 0;
}

static int lexer_is_at_end(Lexer *lexer) {
    return lexer->pos >= lexer->length;
}

static char lexer_peek(Lexer *lexer) {
    if (lexer_is_at_end(lexer)) return '\0';
    return lexer->source[lexer->pos];
}

static char lexer_peek_next(Lexer *lexer) {
    if (lexer->pos + 1 >= lexer->length) return '\0';
    return lexer->source[lexer->pos + 1];
}

static char lexer_advance(Lexer *lexer) {
    if (lexer_is_at_end(lexer)) return '\0';
    
    char c = lexer->source[lexer->pos++];
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    return c;
}

static int lexer_match(Lexer *lexer, char expected) {
    if (lexer_is_at_end(lexer)) return 0;
    if (lexer->source[lexer->pos] != expected) return 0;
    lexer_advance(lexer);
    return 1;
}

static int lexer_is_digit(char c) {
    return c >= '0' && c <= '9';
}

static int lexer_is_hex_digit(char c) {
    return lexer_is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static int lexer_is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int lexer_is_alnum(char c) {
    return lexer_is_alpha(c) || lexer_is_digit(c);
}

// 跳过空白和注释
static void skip_whitespace(Lexer *lexer) {
    while (!lexer_is_at_end(lexer)) {
        char c = lexer_peek(lexer);
        
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                lexer_advance(lexer);
                break;
                
            case '\n':
                if (lexer->in_preprocessor) {
                    return; // 预处理指令在换行处结束
                }
                lexer_advance(lexer);
                break;
                
            case '/':
                if (lexer_peek_next(lexer) == '/') {
                    // 单行注释
                    lexer_advance(lexer); // /
                    lexer_advance(lexer); // /
                    while (!lexer_is_at_end(lexer) && lexer_peek(lexer) != '\n') {
                        lexer_advance(lexer);
                    }
                } else if (lexer_peek_next(lexer) == '*') {
                    // 多行注释
                    lexer_advance(lexer); // /
                    lexer_advance(lexer); // *
                    while (!lexer_is_at_end(lexer)) {
                        if (lexer_peek(lexer) == '*' && lexer_peek_next(lexer) == '/') {
                            lexer_advance(lexer); // *
                            lexer_advance(lexer); // /
                            break;
                        }
                        lexer_advance(lexer);
                    }
                } else {
                    return;
                }
                break;
                
            default:
                return;
        }
    }
}

// ===============================================
// ASTC节点创建和管理
// ===============================================

// 创建AST节点
struct ASTNode* ast_create_node(ASTNodeType type, int line, int column) {
    // 现在ASTNode已经完整定义，可以使用sizeof
    struct ASTNode *node = (struct ASTNode*)malloc(sizeof(struct ASTNode));
    if (node) {
        memset(node, 0, sizeof(struct ASTNode));
        node->type = type;
        node->line = line;
        node->column = column;
    }
    return node;
}

// 释放AST节点及其子节点
void ast_free(struct ASTNode *node) {
    if (!node) return;
    
    // 根据节点类型释放资源
    switch (node->type) {
        case ASTC_EXPR_IDENTIFIER:
            if (node->data.identifier.name) free(node->data.identifier.name);
            break;
        case ASTC_EXPR_STRING_LITERAL:
            if (node->data.string_literal.value) free(node->data.string_literal.value);
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
            // 释放翻译单元的子节点
            // 这里需要根据实际ASTC结构实现
            break;
        default:
            // 其他节点类型的释放逻辑
            break;
    }
    
    free(node);
}

// ===============================================
// 直接从Token创建ASTC节点
// ===============================================

// 从标识符Token创建ASTC标识符节点
static struct ASTNode* create_identifier_node(const char *name, int line, int column) {
    struct ASTNode *node = ast_create_node(ASTC_EXPR_IDENTIFIER, line, column);
    if (node) {
        node->data.identifier.name = strdup(name);
    }
    return node;
}

// 从数字Token创建ASTC常量节点
static struct ASTNode* create_number_node(const char *value, int line, int column) {
    struct ASTNode *node = ast_create_node(ASTC_EXPR_CONSTANT, line, column);
    if (node) {
        // 检查是否为浮点数
        if (strchr(value, '.') || strchr(value, 'e') || strchr(value, 'E')) {
            node->data.constant.type = ASTC_TYPE_FLOAT;
            node->data.constant.float_val = atof(value);
        } else {
            node->data.constant.type = ASTC_TYPE_INT;
            node->data.constant.int_val = atoll(value);
        }
    }
    return node;
}

// 从字符串Token创建ASTC字符串字面量节点
static struct ASTNode* create_string_node(const char *value, int line, int column) {
    struct ASTNode *node = ast_create_node(ASTC_EXPR_STRING_LITERAL, line, column);
    if (node) {
        // 去除引号
        size_t len = strlen(value);
        if (len >= 2 && value[0] == '"' && value[len-1] == '"') {
            char *str = (char*)malloc(len - 1);
            if (str) {
                strncpy(str, value + 1, len - 2);
                str[len - 2] = '\0';
                node->data.string_literal.value = str;
            }
        } else {
            node->data.string_literal.value = strdup(value);
        }
    }
    return node;
}

// 创建二元操作表达式节点
static struct ASTNode* create_binary_expr(ASTNodeType op, struct ASTNode *left, struct ASTNode *right, int line, int column) {
    struct ASTNode *node = ast_create_node(ASTC_BINARY_OP, line, column);
    if (node) {
        node->data.binary_op.op = op;
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
    }
    return node;
}

// 创建一元操作表达式节点
static struct ASTNode* create_unary_expr(ASTNodeType op, struct ASTNode *operand, int line, int column) {
    struct ASTNode *node = ast_create_node(ASTC_UNARY_OP, line, column);
    if (node) {
        node->data.unary_op.op = op;
        node->data.unary_op.operand = operand;
    }
    return node;
}

// 创建函数调用表达式节点
static struct ASTNode* create_call_expr(struct ASTNode *callee, struct ASTNode **args, int arg_count, int line, int column) {
    struct ASTNode *node = ast_create_node(ASTC_CALL_EXPR, line, column);
    if (node) {
        node->data.call_expr.callee = callee;
        node->data.call_expr.args = args;
        node->data.call_expr.arg_count = arg_count;
    }
    return node;
}

// ===============================================
// 解析器
// ===============================================

// 解析器上下文
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

// 初始化解析器
static void init_parser(Parser *parser, Token **tokens, int token_count) {
    parser->tokens = tokens;
    parser->token_count = token_count;
    parser->current = 0;
    parser->error_msg[0] = '\0';
    parser->error_count = 0;
    parser->symbols.count = 0;
}

// 检查当前Token类型
static int check(Parser *parser, TokenType type) {
    if (parser->current >= parser->token_count) return 0;
    return parser->tokens[parser->current]->type == type;
}

// 前进到下一个Token
static Token* advance(Parser *parser) {
    if (parser->current < parser->token_count) {
        return parser->tokens[parser->current++];
    }
    return NULL;
}

// 获取当前Token
static Token* peek(Parser *parser) {
    if (parser->current >= parser->token_count) return NULL;
    return parser->tokens[parser->current];
}

// 匹配并消耗一个Token
static int match(Parser *parser, TokenType type) {
    if (check(parser, type)) {
        advance(parser);
        return 1;
    }
    return 0;
}

// 报告解析错误
static void parser_error(Parser *parser, const char *message) {
    Token *token = peek(parser);
    if (token) {
        snprintf(parser->error_msg, sizeof(parser->error_msg), 
                 "%s:%d:%d: %s", token->filename, token->line, token->column, message);
    } else {
        snprintf(parser->error_msg, sizeof(parser->error_msg), "%s", message);
    }
    parser->error_count++;
}

// 前向声明解析函数
static struct ASTNode* parse_expression(Parser *parser);
static struct ASTNode* parse_statement(Parser *parser);
static struct ASTNode* parse_declaration(Parser *parser);

// 解析表达式
static struct ASTNode* parse_expression(Parser *parser) {
    // 简单实现：仅支持标识符、常量和字符串字面量
    Token *token = peek(parser);
    if (!token) return NULL;
    
    if (token->type == TOKEN_IDENTIFIER) {
        advance(parser);
        return create_identifier_node(token->value, token->line, token->column);
    } else if (token->type == TOKEN_NUMBER) {
        advance(parser);
        return create_number_node(token->value, token->line, token->column);
    } else if (token->type == TOKEN_STRING_LITERAL) {
        advance(parser);
        return create_string_node(token->value, token->line, token->column);
    }
    
    parser_error(parser, "预期表达式");
    return NULL;
}

// 解析语句
static struct ASTNode* parse_statement(Parser *parser) {
    // 简单实现：仅支持表达式语句
    struct ASTNode *expr = parse_expression(parser);
    if (!expr) return NULL;
    
    if (!match(parser, TOKEN_SEMICOLON)) {
        parser_error(parser, "预期分号");
    }
    
    return expr;
}

// 解析翻译单元
static struct ASTNode* parse_translation_unit(Parser *parser) {
    struct ASTNode *root = ast_create_node(ASTC_TRANSLATION_UNIT, 0, 0);
    if (!root) return NULL;
    
    // 简单实现：仅支持顶层声明
    while (parser->current < parser->token_count) {
        struct ASTNode *decl = parse_declaration(parser);
        if (decl) {
            // 添加到翻译单元
            // 这里需要根据实际ASTC结构实现
        } else {
            break;
        }
    }
    
    return root;
}

// 解析声明
static struct ASTNode* parse_declaration(Parser *parser) {
    // 简单实现：仅支持变量声明
    // 实际实现需要处理函数声明、类型声明等
    
    // 暂时返回NULL，表示不支持声明解析
    return NULL;
}

// ===============================================
// C2ASTC API实现
// ===============================================

// 默认选项
C2AstcOptions c2astc_default_options(void) {
    C2AstcOptions options;
    options.optimize_level = false;
    options.enable_extensions = true;
    options.emit_debug_info = true;
    return options;
}

// 打印版本信息
void c2astc_print_version(void) {
    printf("C to ASTC Converter v0.1\n");
    printf("Part of Self-Evolve AI System\n");
}

// 从文件加载C源代码并转换为ASTC
struct ASTNode* c2astc_convert_file(const char *filename, const C2AstcOptions *options) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        set_error("无法打开文件: %s", filename);
        return NULL;
    }
    
    // 获取文件大小
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(fp);
        set_error("文件为空或无法获取文件大小: %s", filename);
        return NULL;
    }
    
    // 分配内存并读取文件内容
    char *source = (char *)malloc(file_size + 1);
    if (!source) {
        fclose(fp);
        set_error("内存分配失败");
        return NULL;
    }
    
    size_t bytes_read = fread(source, 1, file_size, fp);
    fclose(fp);
    
    if (bytes_read != (size_t)file_size) {
        free(source);
        set_error("读取文件失败: %s", filename);
        return NULL;
    }
    
    source[file_size] = '\0';
    
    // 转换代码
    struct ASTNode *root = c2astc_convert(source, options);
    
    free(source);
    return root;
}

// 将C源代码转换为ASTC
struct ASTNode* c2astc_convert(const char *source, const C2AstcOptions *options) {
    if (!source) {
        set_error("源代码为NULL");
        return NULL;
    }
    
    // 使用默认选项
    C2AstcOptions default_options = c2astc_default_options();
    if (!options) options = &default_options;
    
    // 创建一个简单的翻译单元节点
    struct ASTNode *root = ast_create_node(ASTC_TRANSLATION_UNIT, 1, 1);
    if (!root) {
        set_error("内存分配失败");
        return NULL;
    }
    
    // TODO: 实现完整的C到ASTC转换
    // 1. 词法分析
    // 2. 语法分析
    // 3. 语义分析
    // 4. ASTC生成
    
    return root;
}

// 序列化ASTC为二进制格式
unsigned char* c2astc_serialize(struct ASTNode *node, size_t *out_size) {
    // 简单实现：暂时只序列化节点类型
    if (!node || !out_size) {
        set_error("无效的参数");
        return NULL;
    }
    
    *out_size = sizeof(int);
    unsigned char *binary = (unsigned char*)malloc(*out_size);
    if (!binary) {
        set_error("内存分配失败");
        return NULL;
    }
    
    *(int*)binary = node->type;
    return binary;
}

// 反序列化二进制格式为ASTC
struct ASTNode* c2astc_deserialize(const unsigned char *binary, size_t size) {
    // 简单实现：暂时只反序列化节点类型
    if (!binary || size < sizeof(int)) {
        set_error("无效的二进制数据");
        return NULL;
    }
    
    ASTNodeType type = *(int*)binary;
    return ast_create_node(type, 0, 0);
}

unsigned char* c2astc(struct ASTNode *node, const C2AstcOptions *options, size_t *out_size) {
    if (!node || !out_size) {
        set_error("无效的参数");
        return NULL;
    }
    
    // 使用默认选项
    C2AstcOptions default_options = c2astc_default_options();
    if (!options) options = &default_options;
    
    // 创建一个最小的模块
    // 格式: 
    // - 魔数: \0asm (4字节)
    // - 版本: 01 00 00 00 (4字节，小端序表示版本1)
    *out_size = 8;
    unsigned char *rt = (unsigned char*)malloc(*out_size);
    if (!rt) {
        set_error("内存分配失败");
        return NULL;
    }
    
    rt[0] = 0x00;
    rt[1] = 0x61;
    rt[2] = 0x73;
    rt[3] = 0x6D;
    rt[4] = 0x01;
    rt[5] = 0x00;
    rt[6] = 0x00;
    rt[7] = 0x00;
    
    return rt;
}

// 调试函数：将节点打印为文本
void ast_print(struct ASTNode *node, int indent) {
    if (!node) return;
    
    for (int i = 0; i < indent; i++) printf("  ");
    
    // 根据节点类型打印信息
    switch (node->type) {
        case ASTC_TRANSLATION_UNIT:
            printf("TranslationUnit\n");
            break;
        case ASTC_EXPR_IDENTIFIER:
            printf("Identifier: %s\n", node->data.identifier.name);
            break;
        case ASTC_EXPR_CONSTANT:
            if (node->data.constant.type == ASTC_TYPE_INT) {
                printf("Constant: %lld\n", (long long)node->data.constant.int_val);
            } else {
                printf("Constant: %f\n", node->data.constant.float_val);
            }
            break;
        case ASTC_EXPR_STRING_LITERAL:
            printf("String: \"%s\"\n", node->data.string_literal.value);
            break;
        default:
            printf("Node(type=%d)\n", node->type);
            break;
    }
    
    // 递归打印子节点
    switch (node->type) {
        case ASTC_BINARY_OP:
            ast_print(node->data.binary_op.left, indent + 1);
            ast_print(node->data.binary_op.right, indent + 1);
            break;
        case ASTC_UNARY_OP:
            ast_print(node->data.unary_op.operand, indent + 1);
            break;
        case ASTC_CALL_EXPR:
            ast_print(node->data.call_expr.callee, indent + 1);
            for (int i = 0; i < node->data.call_expr.arg_count; i++) {
                ast_print(node->data.call_expr.args[i], indent + 1);
            }
            break;
        default:
            // 其他节点类型的子节点打印
            break;
    }
} 
