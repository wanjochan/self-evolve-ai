// test_evolver0_parser.c - 测试evolver0解析器
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 不定义TEST_PARSER，避免包含evolver0_parser.c中的测试代码
// #define TEST_PARSER

// 包含解析器实现
#include "evolver0_parser.c"

// 测试用的C源代码
const char *test_sources[] = {
    // 测试1：简单函数
    "int main() {\n"
    "    return 42;\n"
    "}\n",
    
    // 测试2：带参数的函数
    "int add(int a, int b) {\n"
    "    return a + b;\n"
    "}\n",
    
    // 测试3：变量声明和赋值
    "int main() {\n"
    "    int x = 10;\n"
    "    int y = 20;\n"
    "    int z = x + y;\n"
    "    return z;\n"
    "}\n",
    
    // 测试4：if语句
    "int max(int a, int b) {\n"
    "    if (a > b) {\n"
    "        return a;\n"
    "    } else {\n"
    "        return b;\n"
    "    }\n"
    "}\n",
    
    // 测试5：while循环
    "int factorial(int n) {\n"
    "    int result = 1;\n"
    "    while (n > 0) {\n"
    "        result = result * n;\n"
    "        n = n - 1;\n"
    "    }\n"
    "    return result;\n"
    "}\n",
    
    // 测试6：for循环
    "int sum_array(int arr[], int size) {\n"
    "    int sum = 0;\n"
    "    for (int i = 0; i < size; i++) {\n"
    "        sum = sum + arr[i];\n"
    "    }\n"
    "    return sum;\n"
    "}\n",
    
    // 测试7：复杂表达式
    "int complex_expr(int a, int b, int c) {\n"
    "    return a * b + c / 2 - (a + b) * (c - 1);\n"
    "}\n",
    
    // 测试8：指针
    "void swap(int *a, int *b) {\n"
    "    int temp = *a;\n"
    "    *a = *b;\n"
    "    *b = temp;\n"
    "}\n",
    
    // 测试9：多个函数
    "int square(int x) {\n"
    "    return x * x;\n"
    "}\n"
    "\n"
    "int cube(int x) {\n"
    "    return x * x * x;\n"
    "}\n"
    "\n"
    "int main() {\n"
    "    int a = 5;\n"
    "    int b = square(a);\n"
    "    int c = cube(a);\n"
    "    return b + c;\n"
    "}\n",
    
    NULL  // 结束标记
};

// 改进的词法分析器（比evolver0_parser.c中的simple_tokenize更完整）
static int tokenize_source(const char *source, Token **out_tokens) {
    Token *tokens = (Token *)calloc(10000, sizeof(Token));
    int count = 0;
    const char *p = source;
    int line = 1;
    int column = 1;
    
    while (*p) {
        // 跳过空白字符
        while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) {
            if (*p == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
            p++;
        }
        
        if (!*p) break;
        
        Token *token = &tokens[count];
        token->line = line;
        token->column = column;
        
        // 单行注释
        if (*p == '/' && *(p+1) == '/') {
            while (*p && *p != '\n') {
                p++;
                column++;
            }
            continue;
        }
        
        // 多行注释
        if (*p == '/' && *(p+1) == '*') {
            p += 2;
            column += 2;
            while (*p && !(*p == '*' && *(p+1) == '/')) {
                if (*p == '\n') {
                    line++;
                    column = 1;
                } else {
                    column++;
                }
                p++;
            }
            if (*p) {
                p += 2;
                column += 2;
            }
            continue;
        }
        
        // 标识符或关键字
        if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_') {
            const char *start = p;
            while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || 
                   (*p >= '0' && *p <= '9') || *p == '_') {
                p++;
                column++;
            }
            
            int len = p - start;
            char *value = (char *)malloc(len + 1);
            strncpy(value, start, len);
            value[len] = '\0';
            
            // 检查关键字
            if (strcmp(value, "int") == 0) token->type = TOKEN_INT;
            else if (strcmp(value, "char") == 0) token->type = TOKEN_CHAR;
            else if (strcmp(value, "float") == 0) token->type = TOKEN_FLOAT;
            else if (strcmp(value, "double") == 0) token->type = TOKEN_DOUBLE;
            else if (strcmp(value, "void") == 0) token->type = TOKEN_VOID;
            else if (strcmp(value, "if") == 0) token->type = TOKEN_IF;
            else if (strcmp(value, "else") == 0) token->type = TOKEN_ELSE;
            else if (strcmp(value, "while") == 0) token->type = TOKEN_WHILE;
            else if (strcmp(value, "do") == 0) token->type = TOKEN_DO;
            else if (strcmp(value, "for") == 0) token->type = TOKEN_FOR;
            else if (strcmp(value, "return") == 0) token->type = TOKEN_RETURN;
            else if (strcmp(value, "break") == 0) token->type = TOKEN_BREAK;
            else if (strcmp(value, "continue") == 0) token->type = TOKEN_CONTINUE;
            else if (strcmp(value, "struct") == 0) token->type = TOKEN_STRUCT;
            else if (strcmp(value, "union") == 0) token->type = TOKEN_UNION;
            else if (strcmp(value, "enum") == 0) token->type = TOKEN_ENUM;
            else if (strcmp(value, "typedef") == 0) token->type = TOKEN_TYPEDEF;
            else if (strcmp(value, "sizeof") == 0) token->type = TOKEN_SIZEOF;
            else if (strcmp(value, "static") == 0) token->type = TOKEN_STATIC;
            else if (strcmp(value, "extern") == 0) token->type = TOKEN_EXTERN;
            else if (strcmp(value, "const") == 0) token->type = TOKEN_CONST;
            else if (strcmp(value, "volatile") == 0) token->type = TOKEN_VOLATILE;
            else token->type = TOKEN_IDENTIFIER;
            
            token->value = value;
            count++;
        }
        // 数字
        else if (*p >= '0' && *p <= '9') {
            const char *start = p;
            while (*p >= '0' && *p <= '9') {
                p++;
                column++;
            }
            
            // 检查浮点数
            if (*p == '.') {
                p++;
                column++;
                while (*p >= '0' && *p <= '9') {
                    p++;
                    column++;
                }
                token->type = TOKEN_FLOAT_NUMBER;
            } else {
                token->type = TOKEN_NUMBER;
            }
            
            int len = p - start;
            char *value = (char *)malloc(len + 1);
            strncpy(value, start, len);
            value[len] = '\0';
            token->value = value;
            count++;
        }
        // 字符串
        else if (*p == '"') {
            p++;
            column++;
            const char *start = p;
            while (*p && *p != '"') {
                if (*p == '\\' && *(p+1)) {
                    p += 2;
                    column += 2;
                } else {
                    p++;
                    column++;
                }
            }
            
            int len = p - start;
            char *value = (char *)malloc(len + 3);
            value[0] = '"';
            strncpy(value + 1, start, len);
            value[len + 1] = '"';
            value[len + 2] = '\0';
            
            if (*p == '"') {
                p++;
                column++;
            }
            
            token->type = TOKEN_STRING;
            token->value = value;
            count++;
        }
        // 字符字面量
        else if (*p == '\'') {
            p++;
            column++;
            const char *start = p - 1;
            if (*p == '\\' && *(p+1)) {
                p += 2;
                column += 2;
            } else if (*p) {
                p++;
                column++;
            }
            
            if (*p == '\'') {
                p++;
                column++;
            }
            
            int len = p - start;
            char *value = (char *)malloc(len + 1);
            strncpy(value, start, len);
            value[len] = '\0';
            
            token->type = TOKEN_CHAR_LITERAL;
            token->value = value;
            count++;
        }
        // 操作符和标点符号
        else {
            switch (*p) {
                case '+':
                    if (*(p+1) == '+') {
                        token->type = TOKEN_INCREMENT;
                        token->value = strdup("++");
                        p += 2;
                        column += 2;
                    } else if (*(p+1) == '=') {
                        token->type = TOKEN_ADD_ASSIGN;
                        token->value = strdup("+=");
                        p += 2;
                        column += 2;
                    } else {
                        token->type = TOKEN_PLUS;
                        token->value = strdup("+");
                        p++;
                        column++;
                    }
                    break;
                    
                case '-':
                    if (*(p+1) == '-') {
                        token->type = TOKEN_DECREMENT;
                        token->value = strdup("--");
                        p += 2;
                        column += 2;
                    } else if (*(p+1) == '=') {
                        token->type = TOKEN_SUB_ASSIGN;
                        token->value = strdup("-=");
                        p += 2;
                        column += 2;
                    } else if (*(p+1) == '>') {
                        token->type = TOKEN_ARROW;
                        token->value = strdup("->");
                        p += 2;
                        column += 2;
                    } else {
                        token->type = TOKEN_MINUS;
                        token->value = strdup("-");
                        p++;
                        column++;
                    }
                    break;
                    
                case '*':
                    if (*(p+1) == '=') {
                        token->type = TOKEN_MUL_ASSIGN;
                        token->value = strdup("*=");
                        p += 2;
                        column += 2;
                    } else {
                        token->type = TOKEN_MULTIPLY;
                        token->value = strdup("*");
                        p++;
                        column++;
                    }
                    break;
                    
                case '/':
                    if (*(p+1) == '=') {
                        token->type = TOKEN_DIV_ASSIGN;
                        token->value = strdup("/=");
                        p += 2;
                        column += 2;
                    } else {
                        token->type = TOKEN_DIVIDE;
                        token->value = strdup("/");
                        p++;
                        column++;
                    }
                    break;
                    
                case '%':
                    if (*(p+1) == '=') {
                        token->type = TOKEN_MOD_ASSIGN;
                        token->value = strdup("%=");
                        p += 2;
                        column += 2;
                    } else {
                        token->type = TOKEN_MOD;
                        token->value = strdup("%");
                        p++;
                        column++;
                    }
                    break;
                    
                case '=':
                    if (*(p+1) == '=') {
                        token->type = TOKEN_EQUAL;
                        token->value = strdup("==");
                        p += 2;
                        column += 2;
                    } else {
                        token->type = TOKEN_ASSIGN;
                        token->value = strdup("=");
                        p++;
                        column++;
                    }
                    break;
                    
                case '!':
                    if (*(p+1) == '=') {
                        token->type = TOKEN_NOT_EQUAL;
                        token->value = strdup("!=");
                        p += 2;
                        column += 2;
                    } else {
                        token->type = TOKEN_LOGICAL_NOT;
                        token->value = strdup("!");
                        p++;
                        column++;
                    }
                    break;
                    
                case '<':
                    if (*(p+1) == '=') {
                        token->type = TOKEN_LESS_EQUAL;
                        token->value = strdup("<=");
                        p += 2;
                        column += 2;
                    } else if (*(p+1) == '<') {
                        if (*(p+2) == '=') {
                            token->type = TOKEN_LEFT_SHIFT_ASSIGN;
                            token->value = strdup("<<=");
                            p += 3;
                            column += 3;
                        } else {
                            token->type = TOKEN_LEFT_SHIFT;
                            token->value = strdup("<<");
                            p += 2;
                            column += 2;
                        }
                    } else {
                        token->type = TOKEN_LESS;
                        token->value = strdup("<");
                        p++;
                        column++;
                    }
                    break;
                    
                case '>':
                    if (*(p+1) == '=') {
                        token->type = TOKEN_GREATER_EQUAL;
                        token->value = strdup(">=");
                        p += 2;
                        column += 2;
                    } else if (*(p+1) == '>') {
                        if (*(p+2) == '=') {
                            token->type = TOKEN_RIGHT_SHIFT_ASSIGN;
                            token->value = strdup(">>=");
                            p += 3;
                            column += 3;
                        } else {
                            token->type = TOKEN_RIGHT_SHIFT;
                            token->value = strdup(">>");
                            p += 2;
                            column += 2;
                        }
                    } else {
                        token->type = TOKEN_GREATER;
                        token->value = strdup(">");
                        p++;
                        column++;
                    }
                    break;
                    
                case '&':
                    if (*(p+1) == '&') {
                        token->type = TOKEN_LOGICAL_AND;
                        token->value = strdup("&&");
                        p += 2;
                        column += 2;
                    } else if (*(p+1) == '=') {
                        token->type = TOKEN_BIT_AND_ASSIGN;
                        token->value = strdup("&=");
                        p += 2;
                        column += 2;
                    } else {
                        token->type = TOKEN_BIT_AND;
                        token->value = strdup("&");
                        p++;
                        column++;
                    }
                    break;
                    
                case '|':
                    if (*(p+1) == '|') {
                        token->type = TOKEN_LOGICAL_OR;
                        token->value = strdup("||");
                        p += 2;
                        column += 2;
                    } else if (*(p+1) == '=') {
                        token->type = TOKEN_BIT_OR_ASSIGN;
                        token->value = strdup("|=");
                        p += 2;
                        column += 2;
                    } else {
                        token->type = TOKEN_BIT_OR;
                        token->value = strdup("|");
                        p++;
                        column++;
                    }
                    break;
                    
                case '^':
                    if (*(p+1) == '=') {
                        token->type = TOKEN_BIT_XOR_ASSIGN;
                        token->value = strdup("^=");
                        p += 2;
                        column += 2;
                    } else {
                        token->type = TOKEN_BIT_XOR;
                        token->value = strdup("^");
                        p++;
                        column++;
                    }
                    break;
                    
                case '~':
                    token->type = TOKEN_BIT_NOT;
                    token->value = strdup("~");
                    p++;
                    column++;
                    break;
                    
                case '(':
                    token->type = TOKEN_LPAREN;
                    token->value = strdup("(");
                    p++;
                    column++;
                    break;
                    
                case ')':
                    token->type = TOKEN_RPAREN;
                    token->value = strdup(")");
                    p++;
                    column++;
                    break;
                    
                case '[':
                    token->type = TOKEN_LBRACKET;
                    token->value = strdup("[");
                    p++;
                    column++;
                    break;
                    
                case ']':
                    token->type = TOKEN_RBRACKET;
                    token->value = strdup("]");
                    p++;
                    column++;
                    break;
                    
                case '{':
                    token->type = TOKEN_LBRACE;
                    token->value = strdup("{");
                    p++;
                    column++;
                    break;
                    
                case '}':
                    token->type = TOKEN_RBRACE;
                    token->value = strdup("}");
                    p++;
                    column++;
                    break;
                    
                case ';':
                    token->type = TOKEN_SEMICOLON;
                    token->value = strdup(";");
                    p++;
                    column++;
                    break;
                    
                case ':':
                    token->type = TOKEN_COLON;
                    token->value = strdup(":");
                    p++;
                    column++;
                    break;
                    
                case ',':
                    token->type = TOKEN_COMMA;
                    token->value = strdup(",");
                    p++;
                    column++;
                    break;
                    
                case '.':
                    if (*(p+1) == '.' && *(p+2) == '.') {
                        token->type = TOKEN_ELLIPSIS;
                        token->value = strdup("...");
                        p += 3;
                        column += 3;
                    } else {
                        token->type = TOKEN_DOT;
                        token->value = strdup(".");
                        p++;
                        column++;
                    }
                    break;
                    
                case '?':
                    token->type = TOKEN_QUESTION;
                    token->value = strdup("?");
                    p++;
                    column++;
                    break;
                    
                default:
                    // 未知字符，跳过
                    p++;
                    column++;
                    continue;
            }
            count++;
        }
    }
    
    // 添加EOF token
    tokens[count].type = TOKEN_EOF;
    tokens[count].value = strdup("");
    tokens[count].line = line;
    tokens[count].column = column;
    count++;
    
    *out_tokens = tokens;
    return count;
}

// 释放tokens
static void free_tokens(Token *tokens, int count) {
    for (int i = 0; i < count; i++) {
        free(tokens[i].value);
    }
    free(tokens);
}

// 主测试函数
int main() {
    printf("=== evolver0 Parser Test ===\n\n");
    
    int test_num = 0;
    const char **test = test_sources;
    
    while (*test) {
        test_num++;
        printf("测试 %d:\n", test_num);
        printf("源代码:\n%s\n", *test);
        
        // 词法分析
        Token *tokens;
        int token_count = tokenize_source(*test, &tokens);
        
        printf("词法分析结果 (%d 个token):\n", token_count - 1); // 不计EOF
        for (int i = 0; i < token_count && i < 20; i++) { // 只显示前20个
            printf("  [%d] type=%d, value='%s', line=%d, col=%d\n",
                   i, tokens[i].type, tokens[i].value, 
                   tokens[i].line, tokens[i].column);
        }
        if (token_count > 20) {
            printf("  ... (还有 %d 个token)\n", token_count - 20);
        }
        printf("\n");
        
        // 语法分析
        Parser *parser = create_parser(tokens, token_count);
        if (!parser) {
            fprintf(stderr, "创建解析器失败\n");
            free_tokens(tokens, token_count);
            test++;
            continue;
        }
        
        ASTNode *ast = parse(parser);
        if (!ast) {
            fprintf(stderr, "语法分析错误: %s (行 %d, 列 %d)\n",
                    parser->error_msg ? parser->error_msg : "未知错误",
                    parser->error_line,
                    parser->error_column);
        } else {
            printf("语法分析成功！\n");
            printf("AST结构:\n");
            print_ast(ast);
            free_ast_node(ast);
        }
        
        // 清理
        free_parser(parser);
        free_tokens(tokens, token_count);
        
        printf("\n" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "\n\n");
        test++;
    }
    
    return 0;
}