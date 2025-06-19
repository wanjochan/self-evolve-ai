/**
 * evolver0_new.c - 第零代自举编译器
 * 目标：能够编译自身的最小C编译器
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <elf.h>

#define MAX_TOKENS 100000
#define MAX_CODE_SIZE 1048576  // 1MB

// 包含头文件
#include "evolver0_token.h"
#include "evolver0_ast.h"

// ====================================
// 全局变量
// ====================================

typedef struct {
    const char *input_file;
    const char *output_file;
    bool verbose;
    bool dump_ast;
    bool dump_asm;
} CompilerOptions;

// ====================================
// 词法分析器
// ====================================

typedef struct {
    const char *source;
    size_t pos;
    size_t length;
    int line;
    int column;
    const char *filename;
} Lexer;

static void lexer_init(Lexer *lexer, const char *source, const char *filename) {
    lexer->source = source;
    lexer->pos = 0;
    lexer->length = strlen(source);
    lexer->line = 1;
    lexer->column = 1;
    lexer->filename = filename;
}

static void lexer_skip_whitespace(Lexer *lexer) {
    while (lexer->pos < lexer->length) {
        char c = lexer->source[lexer->pos];
        if (c == ' ' || c == '\t' || c == '\r') {
            lexer->pos++;
            lexer->column++;
        } else if (c == '\n') {
            lexer->pos++;
            lexer->line++;
            lexer->column = 1;
        } else if (c == '/' && lexer->pos + 1 < lexer->length) {
            if (lexer->source[lexer->pos + 1] == '/') {
                // 单行注释
                lexer->pos += 2;
                while (lexer->pos < lexer->length && lexer->source[lexer->pos] != '\n') {
                    lexer->pos++;
                }
            } else if (lexer->source[lexer->pos + 1] == '*') {
                // 多行注释
                lexer->pos += 2;
                while (lexer->pos + 1 < lexer->length) {
                    if (lexer->source[lexer->pos] == '*' && lexer->source[lexer->pos + 1] == '/') {
                        lexer->pos += 2;
                        break;
                    }
                    if (lexer->source[lexer->pos] == '\n') {
                        lexer->line++;
                        lexer->column = 1;
                    } else {
                        lexer->column++;
                    }
                    lexer->pos++;
                }
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

static Token* lexer_tokenize(const char *source, const char *filename, int *token_count);

// ====================================
// 解析器
// ====================================

typedef struct {
    Token *tokens;
    int token_count;
    int current;
    char error_msg[256];
    
    // 符号表
    struct {
        char *names[1024];
        char *types[1024];
        int is_function[1024];
        int count;
    } symbols;
} Parser;

static ASTNode* parse_program(Parser *parser);

// ====================================
// 代码生成器
// ====================================

typedef struct {
    unsigned char *code;
    size_t size;
    size_t capacity;
    
    // 标签管理
    struct {
        char *name;
        size_t offset;
    } *labels;
    int label_count;
    int label_capacity;
    
    // 局部变量
    struct {
        char *name;
        int offset;  // 相对于RBP的偏移
    } *locals;
    int local_count;
    int local_capacity;
    int stack_offset;
    
    // 当前函数信息
    char *current_function;
    bool in_main;
} CodeGen;

static bool codegen_program(ASTNode *ast, CodeGen *gen);

// ====================================
// ELF生成器
// ====================================

static int write_elf_file(const char *filename, unsigned char *code, size_t code_size);

// ====================================
// AST实现
// ====================================

ASTNode* ast_create_node(ASTNodeType type, int line, int column) {
    ASTNode *node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (node) {
        node->type = type;
        node->line = line;
        node->column = column;
    }
    return node;
}

void ast_free(ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_IDENTIFIER:
            free(node->data.str_value);
            break;
            
        case AST_BINARY_OP:
            ast_free(node->data.binary.left);
            ast_free(node->data.binary.right);
            break;
            
        case AST_UNARY_OP:
            ast_free(node->data.unary.operand);
            break;
            
        case AST_FUNCTION:
            free(node->data.function.name);
            free(node->data.function.return_type);
            for (int i = 0; i < node->data.function.param_count; i++) {
                ast_free(node->data.function.params[i]);
            }
            free(node->data.function.params);
            ast_free(node->data.function.body);
            break;
            
        case AST_PARAMETER:
            free(node->data.param.type);
            free(node->data.param.name);
            break;
            
        case AST_RETURN:
            ast_free(node->data.ret.value);
            break;
            
        case AST_COMPOUND:
            for (int i = 0; i < node->data.compound.count; i++) {
                ast_free(node->data.compound.statements[i]);
            }
            free(node->data.compound.statements);
            break;
            
        case AST_DECLARATION:
            free(node->data.decl.type);
            free(node->data.decl.name);
            ast_free(node->data.decl.init);
            break;
            
        case AST_ASSIGNMENT:
            ast_free(node->data.assign.target);
            ast_free(node->data.assign.value);
            break;
            
        case AST_IF:
            ast_free(node->data.if_stmt.cond);
            ast_free(node->data.if_stmt.then_stmt);
            ast_free(node->data.if_stmt.else_stmt);
            break;
            
        case AST_WHILE:
            ast_free(node->data.while_stmt.cond);
            ast_free(node->data.while_stmt.body);
            break;
            
        case AST_FOR:
            ast_free(node->data.for_stmt.init);
            ast_free(node->data.for_stmt.cond);
            ast_free(node->data.for_stmt.inc);
            ast_free(node->data.for_stmt.body);
            break;
            
        case AST_CALL:
            free(node->data.call.name);
            for (int i = 0; i < node->data.call.arg_count; i++) {
                ast_free(node->data.call.args[i]);
            }
            free(node->data.call.args);
            break;
            
        case AST_ARRAY_ACCESS:
            ast_free(node->data.array_access.array);
            ast_free(node->data.array_access.index);
            break;
            
        case AST_CAST:
            free(node->data.cast.target_type);
            ast_free(node->data.cast.expr);
            break;
            
        case AST_SIZEOF:
            free(node->data.sizeof_expr.type_name);
            ast_free(node->data.sizeof_expr.expr);
            break;
            
        case AST_TYPE:
            free(node->data.type.base_type);
            break;
            
        default:
            break;
    }
    
    free(node);
}

static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

void ast_print(ASTNode *node, int indent) {
    if (!node) {
        print_indent(indent);
        printf("(null)\n");
        return;
    }
    
    print_indent(indent);
    
    switch (node->type) {
        case AST_PROGRAM:
            printf("Program\n");
            for (int i = 0; i < node->data.compound.count; i++) {
                ast_print(node->data.compound.statements[i], indent + 1);
            }
            break;
            
        case AST_FUNCTION:
            printf("Function: %s %s\n", node->data.function.return_type, node->data.function.name);
            print_indent(indent + 1);
            printf("Parameters:\n");
            for (int i = 0; i < node->data.function.param_count; i++) {
                ast_print(node->data.function.params[i], indent + 2);
            }
            print_indent(indent + 1);
            printf("Body:\n");
            ast_print(node->data.function.body, indent + 2);
            break;
            
        case AST_PARAMETER:
            printf("Parameter: %s %s\n", node->data.param.type, node->data.param.name);
            break;
            
        case AST_RETURN:
            printf("Return\n");
            if (node->data.ret.value) {
                ast_print(node->data.ret.value, indent + 1);
            }
            break;
            
        case AST_INTEGER:
            printf("Integer: %lld\n", node->data.int_value);
            break;
            
        case AST_IDENTIFIER:
            printf("Identifier: %s\n", node->data.str_value);
            break;
            
        case AST_BINARY_OP:
            printf("BinaryOp: '%c'\n", node->data.binary.op);
            ast_print(node->data.binary.left, indent + 1);
            ast_print(node->data.binary.right, indent + 1);
            break;
            
        case AST_UNARY_OP:
            printf("UnaryOp: '%c'\n", node->data.unary.op);
            ast_print(node->data.unary.operand, indent + 1);
            break;
            
        case AST_COMPOUND:
            printf("Compound (%d statements)\n", node->data.compound.count);
            for (int i = 0; i < node->data.compound.count; i++) {
                ast_print(node->data.compound.statements[i], indent + 1);
            }
            break;
            
        case AST_DECLARATION:
            printf("Declaration: %s %s", node->data.decl.type, node->data.decl.name);
            if (node->data.decl.is_array) {
                printf("[%d]", node->data.decl.array_size);
            }
            printf("\n");
            if (node->data.decl.init) {
                print_indent(indent + 1);
                printf("Init:\n");
                ast_print(node->data.decl.init, indent + 2);
            }
            break;
            
        case AST_ASSIGNMENT:
            printf("Assignment\n");
            print_indent(indent + 1);
            printf("Target:\n");
            ast_print(node->data.assign.target, indent + 2);
            print_indent(indent + 1);
            printf("Value:\n");
            ast_print(node->data.assign.value, indent + 2);
            break;
            
        case AST_IF:
            printf("If\n");
            print_indent(indent + 1);
            printf("Condition:\n");
            ast_print(node->data.if_stmt.cond, indent + 2);
            print_indent(indent + 1);
            printf("Then:\n");
            ast_print(node->data.if_stmt.then_stmt, indent + 2);
            if (node->data.if_stmt.else_stmt) {
                print_indent(indent + 1);
                printf("Else:\n");
                ast_print(node->data.if_stmt.else_stmt, indent + 2);
            }
            break;
            
        case AST_WHILE:
            printf("While\n");
            print_indent(indent + 1);
            printf("Condition:\n");
            ast_print(node->data.while_stmt.cond, indent + 2);
            print_indent(indent + 1);
            printf("Body:\n");
            ast_print(node->data.while_stmt.body, indent + 2);
            break;
            
        case AST_FOR:
            printf("For\n");
            if (node->data.for_stmt.init) {
                print_indent(indent + 1);
                printf("Init:\n");
                ast_print(node->data.for_stmt.init, indent + 2);
            }
            if (node->data.for_stmt.cond) {
                print_indent(indent + 1);
                printf("Condition:\n");
                ast_print(node->data.for_stmt.cond, indent + 2);
            }
            if (node->data.for_stmt.inc) {
                print_indent(indent + 1);
                printf("Increment:\n");
                ast_print(node->data.for_stmt.inc, indent + 2);
            }
            print_indent(indent + 1);
            printf("Body:\n");
            ast_print(node->data.for_stmt.body, indent + 2);
            break;
            
        case AST_EXPRESSION_STMT:
            printf("ExpressionStatement\n");
            break;
            
        case AST_CALL:
            printf("Call: %s\n", node->data.call.name);
            for (int i = 0; i < node->data.call.arg_count; i++) {
                print_indent(indent + 1);
                printf("Arg %d:\n", i);
                ast_print(node->data.call.args[i], indent + 2);
            }
            break;
            
        case AST_ARRAY_ACCESS:
            printf("ArrayAccess\n");
            print_indent(indent + 1);
            printf("Array:\n");
            ast_print(node->data.array_access.array, indent + 2);
            print_indent(indent + 1);
            printf("Index:\n");
            ast_print(node->data.array_access.index, indent + 2);
            break;
            
        case AST_BREAK:
            printf("Break\n");
            break;
            
        case AST_CONTINUE:
            printf("Continue\n");
            break;
            
        default:
            printf("Unknown AST node type: %d\n", node->type);
            break;
    }
}

// ====================================
// Token实现
// ====================================

const char* token_type_to_string(TokenType type) {
    switch (type) {
        case TOK_EOF: return "EOF";
        case TOK_IDENTIFIER: return "IDENTIFIER";
        case TOK_NUMBER: return "NUMBER";
        case TOK_STRING: return "STRING";
        case TOK_INT: return "int";
        case TOK_RETURN: return "return";
        case TOK_IF: return "if";
        case TOK_ELSE: return "else";
        case TOK_WHILE: return "while";
        case TOK_FOR: return "for";
        case TOK_PLUS: return "+";
        case TOK_MINUS: return "-";
        case TOK_MULTIPLY: return "*";
        case TOK_DIVIDE: return "/";
        case TOK_ASSIGN: return "=";
        case TOK_EQUAL: return "==";
        case TOK_NOT_EQUAL: return "!=";
        case TOK_LESS: return "<";
        case TOK_GREATER: return ">";
        case TOK_LESS_EQUAL: return "<=";
        case TOK_GREATER_EQUAL: return ">=";
        case TOK_LPAREN: return "(";
        case TOK_RPAREN: return ")";
        case TOK_LBRACE: return "{";
        case TOK_RBRACE: return "}";
        case TOK_SEMICOLON: return ";";
        case TOK_COMMA: return ",";
        default: return "UNKNOWN";
    }
}

void token_free(Token *tokens, int count) {
    for (int i = 0; i < count; i++) {
        free(tokens[i].value);
    }
    free(tokens);
}

// ====================================
// 包含词法分析器实现
// ====================================

#include "evolver0_lexer.inc.c"

// ====================================
// 包含解析器实现
// ====================================

#include "evolver0_parser.inc.c"

// ====================================
// 包含代码生成器实现
// ====================================

#include "evolver0_codegen.inc.c"

// ====================================
// 主函数
// ====================================

static char* read_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        return NULL;
    }
    
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *content = malloc(size + 1);
    if (!content) {
        fclose(f);
        return NULL;
    }
    
    size_t read_size = fread(content, 1, size, f);
    content[read_size] = '\0';
    
    fclose(f);
    return content;
}

static void print_usage(const char *program) {
    fprintf(stderr, "Usage: %s [options] <input.c> -o <output>\n", program);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -v, --verbose     Enable verbose output\n");
    fprintf(stderr, "  --dump-ast        Dump AST\n");
    fprintf(stderr, "  --dump-asm        Dump generated assembly\n");
    fprintf(stderr, "  -h, --help        Show this help\n");
}

int main(int argc, char *argv[]) {
    CompilerOptions options = {0};
    
    // 解析命令行参数
    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            options.output_file = argv[++i];
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            options.verbose = true;
        } else if (strcmp(argv[i], "--dump-ast") == 0) {
            options.dump_ast = true;
        } else if (strcmp(argv[i], "--dump-asm") == 0) {
            options.dump_asm = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (argv[i][0] != '-' && !options.input_file) {
            options.input_file = argv[i];
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
        i++;
    }
    
    if (!options.input_file || !options.output_file) {
        fprintf(stderr, "Error: Input and output files are required\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // 读取源文件
    char *source = read_file(options.input_file);
    if (!source) {
        fprintf(stderr, "Error: Cannot read file %s\n", options.input_file);
        return 1;
    }
    
    if (options.verbose) {
        printf("Compiling %s -> %s\n", options.input_file, options.output_file);
    }
    
    // 词法分析
    int token_count;
    Token *tokens = lexer_tokenize(source, options.input_file, &token_count);
    if (!tokens) {
        fprintf(stderr, "Lexical analysis failed\n");
        free(source);
        return 1;
    }
    
    if (options.verbose) {
        printf("Lexical analysis complete: %d tokens\n", token_count);
    }
    
    // 语法分析
    Parser parser = {
        .tokens = tokens,
        .token_count = token_count,
        .current = 0
    };
    
    ASTNode *ast = parse_program(&parser);
    if (!ast) {
        fprintf(stderr, "Syntax analysis failed: %s\n", parser.error_msg);
        token_free(tokens, token_count);
        free(source);
        return 1;
    }
    
    if (options.verbose) {
        printf("Syntax analysis complete\n");
    }
    
    if (options.dump_ast) {
        printf("\n=== AST ===\n");
        ast_print(ast, 0);
        printf("\n");
    }
    
    // 代码生成
    CodeGen gen = {0};
    gen.capacity = MAX_CODE_SIZE;
    gen.code = malloc(gen.capacity);
    
    if (!codegen_program(ast, &gen)) {
        fprintf(stderr, "Code generation failed\n");
        free(gen.code);
        ast_free(ast);
        token_free(tokens, token_count);
        free(source);
        return 1;
    }
    
    if (options.verbose) {
        printf("Code generation complete: %zu bytes\n", gen.size);
    }
    
    if (options.dump_asm) {
        printf("\n=== Generated Code ===\n");
        for (size_t i = 0; i < gen.size; i++) {
            printf("%02X ", gen.code[i]);
            if ((i + 1) % 16 == 0) printf("\n");
        }
        if (gen.size % 16 != 0) printf("\n");
        printf("\n");
    }
    
    // 生成ELF文件
    if (write_elf_file(options.output_file, gen.code, gen.size) != 0) {
        fprintf(stderr, "Failed to write output file\n");
        free(gen.code);
        ast_free(ast);
        token_free(tokens, token_count);
        free(source);
        return 1;
    }
    
    if (options.verbose) {
        printf("Successfully generated executable: %s\n", options.output_file);
    }
    
    // 清理
    free(gen.code);
    ast_free(ast);
    token_free(tokens, token_count);
    free(source);
    
    return 0;
}