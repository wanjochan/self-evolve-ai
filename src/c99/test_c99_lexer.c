/**
 * test_c99_lexer.c - 测试C99词法分析器
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c99_lexer.h"

void test_basic_tokens() {
    printf("Testing basic tokens...\n");
    
    const char* source = "int main() { return 42; }";
    C99Lexer lexer;
    c99_lexer_init(&lexer, source, strlen(source));
    
    Token* token;
    int token_count = 0;
    
    while ((token = c99_lexer_next_token(&lexer)) && token->type != TOKEN_EOF) {
        printf("Token %d: %s = '%s' (line %d, col %d)\n", 
               token_count++, 
               c99_token_type_name(token->type),
               token->value ? token->value : "(null)",
               token->line,
               token->column);
        c99_token_free(token);
    }
    
    if (token) {
        printf("Token %d: %s (EOF)\n", token_count, c99_token_type_name(token->type));
        c99_token_free(token);
    }
    
    if (c99_lexer_has_error(&lexer)) {
        printf("Lexer error: %s\n", c99_lexer_get_error(&lexer));
    }
    
    printf("Basic tokens test completed.\n\n");
}

void test_keywords() {
    printf("Testing keywords...\n");
    
    const char* source = "if else while for return int void char float double";
    C99Lexer lexer;
    c99_lexer_init(&lexer, source, strlen(source));
    
    Token* token;
    int token_count = 0;
    
    while ((token = c99_lexer_next_token(&lexer)) && token->type != TOKEN_EOF) {
        printf("Token %d: %s = '%s'\n", 
               token_count++, 
               c99_token_type_name(token->type),
               token->value ? token->value : "(null)");
        c99_token_free(token);
    }
    
    if (token) {
        c99_token_free(token);
    }
    
    printf("Keywords test completed.\n\n");
}

void test_operators() {
    printf("Testing operators...\n");
    
    const char* source = "+ - * / % ++ -- += -= *= /= == != <= >= && || << >> <<=";
    C99Lexer lexer;
    c99_lexer_init(&lexer, source, strlen(source));
    
    Token* token;
    int token_count = 0;
    
    while ((token = c99_lexer_next_token(&lexer)) && token->type != TOKEN_EOF) {
        printf("Token %d: %s = '%s'\n", 
               token_count++, 
               c99_token_type_name(token->type),
               token->value ? token->value : "(null)");
        c99_token_free(token);
    }
    
    if (token) {
        c99_token_free(token);
    }
    
    printf("Operators test completed.\n\n");
}

void test_numbers() {
    printf("Testing numbers...\n");
    
    const char* source = "42 0x1A 077 3.14 2.5e10 123L 456UL";
    C99Lexer lexer;
    c99_lexer_init(&lexer, source, strlen(source));
    
    Token* token;
    int token_count = 0;
    
    while ((token = c99_lexer_next_token(&lexer)) && token->type != TOKEN_EOF) {
        printf("Token %d: %s = '%s'", 
               token_count++, 
               c99_token_type_name(token->type),
               token->value ? token->value : "(null)");
        
        if (token->type == TOKEN_INTEGER_CONSTANT) {
            printf(" (base=%d, unsigned=%s, long=%s, long_long=%s)",
                   token->data.integer.base,
                   token->data.integer.is_unsigned ? "yes" : "no",
                   token->data.integer.is_long ? "yes" : "no",
                   token->data.integer.is_long_long ? "yes" : "no");
        } else if (token->type == TOKEN_FLOATING_CONSTANT) {
            printf(" (float=%s, long_double=%s)",
                   token->data.floating.is_float ? "yes" : "no",
                   token->data.floating.is_long_double ? "yes" : "no");
        }
        
        printf("\n");
        c99_token_free(token);
    }
    
    if (token) {
        c99_token_free(token);
    }
    
    printf("Numbers test completed.\n\n");
}

void test_comments() {
    printf("Testing comments...\n");
    
    const char* source = "int x; // single line comment\n/* multi\nline\ncomment */ int y;";
    C99Lexer lexer;
    c99_lexer_init(&lexer, source, strlen(source));
    
    // 设置不跳过注释
    lexer.skip_comments = false;
    
    Token* token;
    int token_count = 0;
    
    while ((token = c99_lexer_next_token(&lexer)) && token->type != TOKEN_EOF) {
        printf("Token %d: %s = '%s' (line %d)\n", 
               token_count++, 
               c99_token_type_name(token->type),
               token->value ? token->value : "(null)",
               token->line);
        c99_token_free(token);
    }
    
    if (token) {
        c99_token_free(token);
    }
    
    printf("Comments test completed.\n\n");
}

int main() {
    printf("C99 Lexer Test Suite\n");
    printf("====================\n\n");
    
    test_basic_tokens();
    test_keywords();
    test_operators();
    test_numbers();
    test_comments();
    
    printf("All tests completed!\n");
    return 0;
}
