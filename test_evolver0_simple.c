/**
 * test_evolver0_simple.c - 测试简化的evolver0编译器
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

// 包含简化的解析器和代码生成器
#include "evolver0_simple_parser.c"
#include "evolver0_simple_codegen.c"

// 简单的词法分析器
typedef struct {
    const char *source;
    size_t pos;
    size_t length;
    int line;
    int column;
} Lexer;

static void init_lexer(Lexer *lexer, const char *source) {
    lexer->source = source;
    lexer->pos = 0;
    lexer->length = strlen(source);
    lexer->line = 1;
    lexer->column = 1;
}

static void skip_whitespace(Lexer *lexer) {
    while (lexer->pos < lexer->length) {
        char c = lexer->source[lexer->pos];
        if (c == ' ' || c == '\t' || c == '\r') {
            lexer->pos++;
            lexer->column++;
        } else if (c == '\n') {
            lexer->pos++;
            lexer->line++;
            lexer->column = 1;
        } else if (c == '/' && lexer->pos + 1 < lexer->length && lexer->source[lexer->pos + 1] == '/') {
            // 单行注释
            lexer->pos += 2;
            while (lexer->pos < lexer->length && lexer->source[lexer->pos] != '\n') {
                lexer->pos++;
            }
        } else if (c == '/' && lexer->pos + 1 < lexer->length && lexer->source[lexer->pos + 1] == '*') {
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
    }
}

static Token* tokenize_simple(const char *source, int *token_count) {
    Lexer lexer;
    init_lexer(&lexer, source);
    
    Token *tokens = malloc(sizeof(Token) * 1000);
    int count = 0;
    
    while (lexer.pos < lexer.length) {
        skip_whitespace(&lexer);
        if (lexer.pos >= lexer.length) break;
        
        Token *token = &tokens[count];
        token->line = lexer.line;
        token->column = lexer.column;
        
        char c = lexer.source[lexer.pos];
        
        // 标识符或关键字
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
            size_t start = lexer.pos;
            while (lexer.pos < lexer.length) {
                c = lexer.source[lexer.pos];
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
                    (c >= '0' && c <= '9') || c == '_') {
                    lexer.pos++;
                    lexer.column++;
                } else {
                    break;
                }
            }
            
            size_t len = lexer.pos - start;
            char *value = malloc(len + 1);
            memcpy(value, lexer.source + start, len);
            value[len] = '\0';
            
            // 检查关键字
            if (strcmp(value, "int") == 0) token->type = TOKEN_INT;
            else if (strcmp(value, "return") == 0) token->type = TOKEN_RETURN;
            else if (strcmp(value, "if") == 0) token->type = TOKEN_IF;
            else if (strcmp(value, "else") == 0) token->type = TOKEN_ELSE;
            else if (strcmp(value, "while") == 0) token->type = TOKEN_WHILE;
            else if (strcmp(value, "for") == 0) token->type = TOKEN_FOR;
            else token->type = TOKEN_IDENTIFIER;
            
            token->value = value;
            count++;
        }
        // 数字
        else if (c >= '0' && c <= '9') {
            size_t start = lexer.pos;
            while (lexer.pos < lexer.length && lexer.source[lexer.pos] >= '0' && lexer.source[lexer.pos] <= '9') {
                lexer.pos++;
                lexer.column++;
            }
            
            size_t len = lexer.pos - start;
            char *value = malloc(len + 1);
            memcpy(value, lexer.source + start, len);
            value[len] = '\0';
            
            token->type = TOKEN_NUMBER;
            token->value = value;
            count++;
        }
        // 运算符和标点
        else {
            switch (c) {
                case '+':
                    if (lexer.pos + 1 < lexer.length && lexer.source[lexer.pos + 1] == '+') {
                        token->type = TOKEN_INCREMENT;
                        token->value = strdup("++");
                        lexer.pos += 2;
                        lexer.column += 2;
                    } else if (lexer.pos + 1 < lexer.length && lexer.source[lexer.pos + 1] == '=') {
                        token->type = TOKEN_ADD_ASSIGN;
                        token->value = strdup("+=");
                        lexer.pos += 2;
                        lexer.column += 2;
                    } else {
                        token->type = TOKEN_PLUS;
                        token->value = strdup("+");
                        lexer.pos++;
                        lexer.column++;
                    }
                    count++;
                    break;
                    
                case '-':
                    if (lexer.pos + 1 < lexer.length && lexer.source[lexer.pos + 1] == '-') {
                        token->type = TOKEN_DECREMENT;
                        token->value = strdup("--");
                        lexer.pos += 2;
                        lexer.column += 2;
                    } else if (lexer.pos + 1 < lexer.length && lexer.source[lexer.pos + 1] == '=') {
                        token->type = TOKEN_SUB_ASSIGN;
                        token->value = strdup("-=");
                        lexer.pos += 2;
                        lexer.column += 2;
                    } else {
                        token->type = TOKEN_MINUS;
                        token->value = strdup("-");
                        lexer.pos++;
                        lexer.column++;
                    }
                    count++;
                    break;
                    
                case '*':
                    token->type = TOKEN_MULTIPLY;
                    token->value = strdup("*");
                    lexer.pos++;
                    lexer.column++;
                    count++;
                    break;
                    
                case '/':
                    token->type = TOKEN_DIVIDE;
                    token->value = strdup("/");
                    lexer.pos++;
                    lexer.column++;
                    count++;
                    break;
                    
                case '%':
                    token->type = TOKEN_MOD;
                    token->value = strdup("%");
                    lexer.pos++;
                    lexer.column++;
                    count++;
                    break;
                    
                case '=':
                    if (lexer.pos + 1 < lexer.length && lexer.source[lexer.pos + 1] == '=') {
                        token->type = TOKEN_EQUAL;
                        token->value = strdup("==");
                        lexer.pos += 2;
                        lexer.column += 2;
                    } else {
                        token->type = TOKEN_ASSIGN;
                        token->value = strdup("=");
                        lexer.pos++;
                        lexer.column++;
                    }
                    count++;
                    break;
                    
                case '!':
                    if (lexer.pos + 1 < lexer.length && lexer.source[lexer.pos + 1] == '=') {
                        token->type = TOKEN_NOT_EQUAL;
                        token->value = strdup("!=");
                        lexer.pos += 2;
                        lexer.column += 2;
                    } else {
                        token->type = TOKEN_LOGICAL_NOT;
                        token->value = strdup("!");
                        lexer.pos++;
                        lexer.column++;
                    }
                    count++;
                    break;
                    
                case '<':
                    if (lexer.pos + 1 < lexer.length && lexer.source[lexer.pos + 1] == '=') {
                        token->type = TOKEN_LESS_EQUAL;
                        token->value = strdup("<=");
                        lexer.pos += 2;
                        lexer.column += 2;
                    } else {
                        token->type = TOKEN_LESS;
                        token->value = strdup("<");
                        lexer.pos++;
                        lexer.column++;
                    }
                    count++;
                    break;
                    
                case '>':
                    if (lexer.pos + 1 < lexer.length && lexer.source[lexer.pos + 1] == '=') {
                        token->type = TOKEN_GREATER_EQUAL;
                        token->value = strdup(">=");
                        lexer.pos += 2;
                        lexer.column += 2;
                    } else {
                        token->type = TOKEN_GREATER;
                        token->value = strdup(">");
                        lexer.pos++;
                        lexer.column++;
                    }
                    count++;
                    break;
                    
                case '~':
                    token->type = TOKEN_BIT_NOT;
                    token->value = strdup("~");
                    lexer.pos++;
                    lexer.column++;
                    count++;
                    break;
                    
                case '(':
                    token->type = TOKEN_LPAREN;
                    token->value = strdup("(");
                    lexer.pos++;
                    lexer.column++;
                    count++;
                    break;
                    
                case ')':
                    token->type = TOKEN_RPAREN;
                    token->value = strdup(")");
                    lexer.pos++;
                    lexer.column++;
                    count++;
                    break;
                    
                case '{':
                    token->type = TOKEN_LBRACE;
                    token->value = strdup("{");
                    lexer.pos++;
                    lexer.column++;
                    count++;
                    break;
                    
                case '}':
                    token->type = TOKEN_RBRACE;
                    token->value = strdup("}");
                    lexer.pos++;
                    lexer.column++;
                    count++;
                    break;
                    
                case ';':
                    token->type = TOKEN_SEMICOLON;
                    token->value = strdup(";");
                    lexer.pos++;
                    lexer.column++;
                    count++;
                    break;
                    
                case ',':
                    token->type = TOKEN_COMMA;
                    token->value = strdup(",");
                    lexer.pos++;
                    lexer.column++;
                    count++;
                    break;
                    
                default:
                    fprintf(stderr, "未知字符: %c\n", c);
                    lexer.pos++;
                    lexer.column++;
                    break;
            }
        }
    }
    
    // 添加EOF token
    tokens[count].type = TOKEN_EOF;
    tokens[count].value = strdup("");
    tokens[count].line = lexer.line;
    tokens[count].column = lexer.column;
    count++;
    
    *token_count = count;
    return tokens;
}

// 释放tokens
static void free_tokens(Token *tokens, int count) {
    for (int i = 0; i < count; i++) {
        free(tokens[i].value);
    }
    free(tokens);
}

// 测试用例
static void test_case(const char *name, const char *source) {
    printf("\n=== 测试: %s ===\n", name);
    printf("源代码:\n%s\n", source);
    
    // 词法分析
    int token_count;
    Token *tokens = tokenize_simple(source, &token_count);
    
    printf("\n词法分析结果 (%d tokens):\n", token_count);
    for (int i = 0; i < token_count && i < 20; i++) {
        printf("  Token %d: type=%d, value='%s', line=%d, col=%d\n", 
               i, tokens[i].type, tokens[i].value, tokens[i].line, tokens[i].column);
    }
    
    // 语法分析
    SimpleASTNode *ast = parse_simple_c(tokens, token_count);
    if (!ast) {
        printf("解析失败!\n");
        free_tokens(tokens, token_count);
        return;
    }
    
    printf("\nAST:\n");
    print_simple_ast(ast, 0);
    
    // 代码生成
    size_t code_size;
    int entry_offset;
    unsigned char *code = generate_simple_code(ast, &code_size, &entry_offset);
    
    if (!code) {
        printf("代码生成失败!\n");
        free_simple_ast(ast);
        free_tokens(tokens, token_count);
        return;
    }
    
    printf("\n生成的机器码 (%zu 字节):\n", code_size);
    for (size_t i = 0; i < code_size && i < 64; i++) {
        printf("%02X ", code[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    if (code_size > 64) printf("\n...\n");
    else printf("\n");
    
    // 执行代码（仅在Linux x86-64上）
    #ifdef __linux__
    #ifdef __x86_64__
    // 分配可执行内存
    void *exec_mem = mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (exec_mem != MAP_FAILED) {
        // 复制代码
        memcpy(exec_mem, code, code_size);
        
        // 执行
        typedef int (*func_t)(void);
        func_t func = (func_t)exec_mem;
        int result = func();
        
        printf("\n执行结果: %d\n", result);
        
        munmap(exec_mem, 4096);
    }
    #endif
    #endif
    
    // 清理
    free(code);
    free_simple_ast(ast);
    free_tokens(tokens, token_count);
}

int main() {
    printf("=== evolver0 简化编译器测试 ===\n");
    
    // 测试1：简单的返回
    test_case("简单返回", 
        "int main() {\n"
        "    return 42;\n"
        "}\n");
    
    // 测试2：算术表达式
    test_case("算术表达式",
        "int main() {\n"
        "    return 10 + 20 * 3 - 5;\n"
        "}\n");
    
    // 测试3：变量声明和使用
    test_case("变量",
        "int main() {\n"
        "    int x = 10;\n"
        "    int y = 20;\n"
        "    return x + y;\n"
        "}\n");
    
    // 测试4：条件语句
    test_case("条件语句",
        "int main() {\n"
        "    int x = 10;\n"
        "    if (x > 5) {\n"
        "        return 1;\n"
        "    } else {\n"
        "        return 0;\n"
        "    }\n"
        "}\n");
    
    // 测试5：循环
    test_case("while循环",
        "int main() {\n"
        "    int sum = 0;\n"
        "    int i = 1;\n"
        "    while (i <= 5) {\n"
        "        sum = sum + i;\n"
        "        i = i + 1;\n"
        "    }\n"
        "    return sum;\n"
        "}\n");
    
    // 测试6：for循环
    test_case("for循环",
        "int main() {\n"
        "    int sum = 0;\n"
        "    for (int i = 1; i <= 5; i = i + 1) {\n"
        "        sum = sum + i;\n"
        "    }\n"
        "    return sum;\n"
        "}\n");
    
    return 0;
}