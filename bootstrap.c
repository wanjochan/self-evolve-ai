/**
 * bootstrap.c - 极简创世自举器
 * 自主进化AI系统第一步
 */

#include <locale.h>

#include "bootstrap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

/* 前置声明 */
// 词法分析器
static Lexer* lexer_create(char *source);
static void lexer_destroy(Lexer *lexer);
static Token lexer_next_token(Lexer *lexer);

// 语法分析器
static Parser* parser_create(Lexer *lexer);
static void parser_destroy(Parser *parser);
static IRProgram* parser_parse(Parser *parser);

// IR生成
static IRProgram* ir_program_create();
static void ir_program_destroy(IRProgram *program);
static void ir_program_add(IRProgram *program, OpCode op, u64 arg1, u64 arg2, u64 arg3);

// 代码生成
static CodeGen* codegen_create(IRProgram *program);
static void codegen_destroy(CodeGen *codegen);
static void codegen_generate_x86_64(CodeGen *codegen);
static int codegen_write_elf(CodeGen *codegen, const char *filename);
static int codegen_write_pe(CodeGen *codegen, const char *filename);

/* 指令表 */
static const char *instructions[] = {
    "mov", "add", "sub", "push", "pop", "call", "ret", 
    "jmp", "je", "jne", "jz", "jnz", "xor", "leave", NULL
};

/* 寄存器表 */
static const char *registers[] = {
    "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", NULL
};

/*=====================================================
 * 词法分析器实现
 *=====================================================*/

/* 创建词法分析器 */
static Lexer* lexer_create(char *source) {
    Lexer *lexer = (Lexer*)malloc(sizeof(Lexer));
    if (!lexer) return NULL;
    
    lexer->source = source;
    lexer->pos = 0;
    lexer->line = 1;
    lexer->column = 1;
    
    return lexer;
}

/* 销毁词法分析器 */
static void lexer_destroy(Lexer *lexer) {
    if (lexer) {
        free(lexer);
    }
}

/* 获取当前字符 */
static char lexer_current_char(Lexer *lexer) {
    return lexer->source[lexer->pos];
}

/* 前进一个字符 */
static void lexer_advance(Lexer *lexer) {
    char c = lexer_current_char(lexer);
    
    lexer->pos++;
    
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
}

/* 查看下一个字符 */
static char lexer_peek(Lexer *lexer) {
    return lexer->source[lexer->pos + 1];
}

/* 跳过空白字符 */
static void lexer_skip_whitespace(Lexer *lexer) {
    char c = lexer_current_char(lexer);
    
    while (c != '\0' && isspace(c)) {
        lexer_advance(lexer);
        c = lexer_current_char(lexer);
    }
}

/* 跳过注释 */
static void lexer_skip_comment(Lexer *lexer) {
    // 跳过 '#'
    lexer_advance(lexer);
    
    char c = lexer_current_char(lexer);
    while (c != '\0' && c != '\n') {
        lexer_advance(lexer);
        c = lexer_current_char(lexer);
    }
}

/* 检查关键字 */
static int is_instruction(const char *str) {
    for (int i = 0; instructions[i] != NULL; i++) {
        if (strcmp(str, instructions[i]) == 0) {
            return i;
        }
    }
    return -1;
}

static int is_register(const char *str) {
    for (int i = 0; registers[i] != NULL; i++) {
        if (strcmp(str, registers[i]) == 0) {
            return i;
        }
    }
    return -1;
}

/* 读取标识符 */
static Token lexer_read_identifier(Lexer *lexer) {
    u32 start_pos = lexer->pos;
    u32 length = 0;
    
    // 读取标识符字符
    while (isalnum(lexer_current_char(lexer)) || lexer_current_char(lexer) == '_' || 
           lexer_current_char(lexer) == '.' || lexer_current_char(lexer) == '$') {
        lexer_advance(lexer);
        length++;
    }
    
    // 分配内存并复制标识符
    char *value = (char*)malloc(length + 1);
    strncpy(value, &lexer->source[start_pos], length);
    value[length] = '\0';
    
    // 检查是否是关键字或特殊标识符
    Token token;
    token.line = lexer->line;
    token.column = lexer->column - length;
    token.value = value;
    
    // 检查TASM特有关键字
    if (strcmp(value, ".section") == 0) {
        token.type = TOK_SECTION;
    } else if (strcmp(value, "db") == 0) {
        token.type = TOK_DB;
    } else if (strcmp(value, "dw") == 0) {
        token.type = TOK_DW;
    } else if (strcmp(value, "dd") == 0) {
        token.type = TOK_DD;
    } else if (strcmp(value, "dq") == 0) {
        token.type = TOK_DQ;
    } else if (strcmp(value, "equ") == 0) {
        token.type = TOK_EQU;
    } else if (strcmp(value, "times") == 0) {
        token.type = TOK_TIMES;
    } else if (is_instruction(value) >= 0) {
        token.type = TOK_INSTRUCTION;
    } else if (is_register(value) >= 0) {
        token.type = TOK_REGISTER;
    } else if (strcmp(value, "if") == 0) {
        token.type = TOK_IF;
    } else if (strcmp(value, "else") == 0) {
        token.type = TOK_ELSE;
    } else if (strcmp(value, "while") == 0) {
        token.type = TOK_WHILE;
    } else if (strcmp(value, "return") == 0) {
        token.type = TOK_RETURN;
    } else if (strcmp(value, "function") == 0) {
        token.type = TOK_FUNCTION;
    } else if (strcmp(value, "var") == 0) {
        token.type = TOK_VAR;
    } else {
        // 检查是否是标签定义
        char next_char = lexer_current_char(lexer);
        if (next_char == ':') {
            token.type = TOK_LABEL;
            lexer_advance(lexer); // 跳过 ':'
            
            // 检查是否紧跟着equ (label: equ value格式)
            lexer_skip_whitespace(lexer);
            if (strncmp(&lexer->source[lexer->pos], "equ", 3) == 0 && 
                (isspace(lexer->source[lexer->pos + 3]) || lexer->source[lexer->pos + 3] == '\0')) {
                // 跳过equ关键字
                lexer->pos += 3;
                lexer->column += 3;
                // 标记为已经处理了equ
                token.type = TOK_LABEL;
            }
        } else {
            token.type = TOK_IDENT;
        }
    }
    
    return token;
}

/* 读取数字 */
static Token lexer_read_number(Lexer *lexer) {
    u32 start_pos = lexer->pos;
    u32 length = 0;
    int is_hex = 0;
    
    // 检查是否是十六进制
    if (lexer_current_char(lexer) == '0' && 
        (lexer_peek(lexer) == 'x' || lexer_peek(lexer) == 'X')) {
        lexer_advance(lexer); // 跳过 '0'
        lexer_advance(lexer); // 跳过 'x'
        is_hex = 1;
        length = 2;
    }
    
    // 读取数字字符
    while (isxdigit(lexer_current_char(lexer)) || 
           (is_hex && ((lexer_current_char(lexer) >= 'a' && lexer_current_char(lexer) <= 'f') ||
                      (lexer_current_char(lexer) >= 'A' && lexer_current_char(lexer) <= 'F')))) {
        lexer_advance(lexer);
        length++;
    }
    
    // 分配内存并复制数字
    char *value = (char*)malloc(length + 1);
    strncpy(value, &lexer->source[start_pos], length);
    value[length] = '\0';
    
    Token token;
    token.type = TOK_NUMBER;
    token.value = value;
    token.line = lexer->line;
    token.column = lexer->column - length;
    
    return token;
}

/* 读取字符串 */
static Token lexer_read_string(Lexer *lexer) {
    // 跳过开始的引号
    lexer_advance(lexer);
    
    u32 start_pos = lexer->pos;
    u32 length = 0;
    
    // 读取字符串内容
    while (lexer_current_char(lexer) != '"' && lexer_current_char(lexer) != '\0') {
        lexer_advance(lexer);
        length++;
    }
    
    // 分配内存并复制字符串
    char *value = (char*)malloc(length + 1);
    strncpy(value, &lexer->source[start_pos], length);
    value[length] = '\0';
    
    // 跳过结束的引号
    if (lexer_current_char(lexer) == '"') {
        lexer_advance(lexer);
    }
    
    Token token;
    token.type = TOK_STRING;
    token.value = value;
    token.line = lexer->line;
    token.column = lexer->column - length - 2; // 减去两个引号
    
    return token;
}

/* 获取下一个标记 */
static Token lexer_next_token(Lexer *lexer) {
    // 跳过空白字符
    lexer_skip_whitespace(lexer);
    
    Token token;
    char c = lexer_current_char(lexer);
    
    // 检查EOF
    if (c == '\0') {
        token.type = TOK_EOF;
        token.value = NULL;
        token.line = lexer->line;
        token.column = lexer->column;
        return token;
    }
    
    // 检查注释
    if (c == '#') {
        token.type = TOK_COMMENT;
        token.value = NULL;
        token.line = lexer->line;
        token.column = lexer->column;
        lexer_skip_comment(lexer);
        return lexer_next_token(lexer);
    }
    
    // 识别标记
    if (isalpha(c) || c == '_' || c == '.') {
        return lexer_read_identifier(lexer);
    } else if (isdigit(c) || (c == '-' && isdigit(lexer_peek(lexer)))) {
        return lexer_read_number(lexer);
    } else if (c == '"') {
        return lexer_read_string(lexer);
    } else {
        token.line = lexer->line;
        token.column = lexer->column;
        token.value = NULL;
        
        switch (c) {
            case '+':
                token.type = TOK_PLUS;
                break;
            case '-':
                token.type = TOK_MINUS;
                break;
            case '*':
                token.type = TOK_STAR;
                break;
            case '/':
                token.type = TOK_SLASH;
                break;
            case '(':
                token.type = TOK_LPAREN;
                break;
            case ')':
                token.type = TOK_RPAREN;
                break;
            case '{':
                token.type = TOK_LBRACE;
                break;
            case '}':
                token.type = TOK_RBRACE;
                break;
            case ';':
                token.type = TOK_SEMICOLON;
                break;
            case ',':
                token.type = TOK_COMMA;
                break;
            case '=':
                token.type = TOK_ASSIGN;
                break;
            case ':':
                token.type = TOK_COLON;
                break;
            default:
                fprintf(stderr, "词法错误: 未知字符 '%c' 在 %d:%d\n", 
                        c, lexer->line, lexer->column);
                exit(1);
        }
        
        lexer_advance(lexer);
        return token;
    }
}

/*=====================================================
 * 语法分析器实现
 *=====================================================*/

/* 创建语法分析器 */
static Parser* parser_create(Lexer *lexer) {
    Parser *parser = (Parser*)malloc(sizeof(Parser));
    if (!parser) return NULL;
    
    parser->lexer = lexer;
    parser->current = lexer_next_token(lexer);
    
    return parser;
}

/* 销毁语法分析器 */
static void parser_destroy(Parser *parser) {
    if (parser) {
        free(parser);
    }
}

/* 前进到下一个标记 */
static void parser_advance(Parser *parser) {
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
}

/* 检查当前标记类型 */
static int parser_check(Parser *parser, TokenType type) {
    return parser->current.type == type;
}

/* 匹配并消费当前标记 */
static int parser_match(Parser *parser, TokenType type) {
    if (parser_check(parser, type)) {
        parser_advance(parser);
        return 1;
    }
    return 0;
}

/* 期望当前标记类型 */
static void parser_expect(Parser *parser, TokenType type, const char *message) {
    if (parser->current.type == type) {
        parser_advance(parser);
    } else {
        fprintf(stderr, "语法错误: %s 在 %d:%d\n", 
                message, parser->current.line, parser->current.column);
        exit(1);
    }
}

/*=====================================================
 * IR生成器实现
 *=====================================================*/

/* 创建IR程序 */
static IRProgram* ir_program_create() {
    IRProgram *program = (IRProgram*)malloc(sizeof(IRProgram));
    if (!program) return NULL;
    
    program->capacity = 1024;
    program->count = 0;
    program->insts = (IRInst*)malloc(program->capacity * sizeof(IRInst));
    
    return program;
}

/* 销毁IR程序 */
static void ir_program_destroy(IRProgram *program) {
    if (program) {
        if (program->insts) {
            free(program->insts);
        }
        free(program);
    }
}

/* 添加IR指令 */
static void ir_program_add(IRProgram *program, OpCode op, u64 arg1, u64 arg2, u64 arg3) {
    if (program->count >= program->capacity) {
        program->capacity *= 2;
        program->insts = (IRInst*)realloc(program->insts, program->capacity * sizeof(IRInst));
    }
    
    program->insts[program->count].op = op;
    program->insts[program->count].arg1 = arg1;
    program->insts[program->count].arg2 = arg2;
    program->insts[program->count].arg3 = arg3;
    program->count++;
}

/* 前置声明 */
static void parse_tasm_statement(Parser *parser, IRProgram *program);
static void parse_tasm_section(Parser *parser, IRProgram *program);
static void parse_tasm_label(Parser *parser, IRProgram *program);
static void parse_tasm_instruction(Parser *parser, IRProgram *program);
static void parse_tasm_directive(Parser *parser, IRProgram *program);
static void parse_tasm_data_definition(Parser *parser, IRProgram *program, OpCode op);
static void parse_tasm_equ(Parser *parser, IRProgram *program);
static void parse_tasm_times(Parser *parser, IRProgram *program);

/* 解析TASM程序 */
static IRProgram* parser_parse(Parser *parser) {
    IRProgram *program = ir_program_create();
    
    // 解析语句列表
    while (!parser_check(parser, TOK_EOF)) {
        parse_tasm_statement(parser, program);
    }
    
    return program;
}

/* 解析TASM语句 */
static void parse_tasm_statement(Parser *parser, IRProgram *program) {
    // 跳过注释
    if (parser_match(parser, TOK_COMMENT)) {
        return;
    }
    
    // 解析节定义
    if (parser_match(parser, TOK_SECTION)) {
        parse_tasm_section(parser, program);
        return;
    }
    
    // 解析标签定义
    if (parser_match(parser, TOK_LABEL)) {
        char *name = parser->previous.value;
        
        // 检查是否是常量定义 (label: equ value)
        if (parser_match(parser, TOK_EQU)) {
            // 解析值
            if (parser_match(parser, TOK_NUMBER)) {
                u64 value = strtoul(parser->previous.value, NULL, 0);
                ir_program_add(program, OP_EQU, (u64)name, value, 0);
            } else {
                fprintf(stderr, "语法错误: EQU后应有数值 在 %d:%d\n", 
                        parser->current.line, parser->current.column);
                exit(1);
            }
            return;
        }
        
        // 普通标签定义
        ir_program_add(program, OP_LABEL, (u64)name, 0, 0);
        return;
    }
    
    // 解析指令
    if (parser_match(parser, TOK_INSTRUCTION)) {
        parse_tasm_instruction(parser, program);
        return;
    }
    
    // 解析数据定义指令
    if (parser_match(parser, TOK_DB)) {
        parse_tasm_data_definition(parser, program, OP_DB);
        return;
    }
    
    if (parser_match(parser, TOK_DW)) {
        parse_tasm_data_definition(parser, program, OP_DW);
        return;
    }
    
    if (parser_match(parser, TOK_DD)) {
        parse_tasm_data_definition(parser, program, OP_DD);
        return;
    }
    
    if (parser_match(parser, TOK_DQ)) {
        parse_tasm_data_definition(parser, program, OP_DQ);
        return;
    }
    
    // 解析EQU指令
    if (parser_match(parser, TOK_EQU)) {
        parse_tasm_equ(parser, program);
        return;
    }
    
    // 解析TIMES指令
    if (parser_match(parser, TOK_TIMES)) {
        parse_tasm_times(parser, program);
        return;
    }
    
    // 未知语句
    fprintf(stderr, "语法错误: 未知的TASM语句 在 %d:%d\n", 
            parser->current.line, parser->current.column);
    exit(1);
}

/* 解析节定义 */
static void parse_tasm_section(Parser *parser, IRProgram *program) {
    // 期望标识符作为节名
    parser_expect(parser, TOK_IDENT, "节定义后应有节名");
    char *section_name = parser->previous.value;
    
    // 添加节定义指令
    ir_program_add(program, OP_SECTION, (u64)section_name, 0, 0);
}

/* 解析标签定义 */
static void parse_tasm_label(Parser *parser, IRProgram *program) {
    // 标签名已在词法分析器中解析
    char *label_name = parser->previous.value;
    
    // 添加标签定义指令
    ir_program_add(program, OP_LABEL, (u64)label_name, 0, 0);
    
    // 检查是否是常量定义 (label: equ value)
    if (parser_match(parser, TOK_EQU)) {
        // 解析值
        if (parser_match(parser, TOK_NUMBER)) {
            u64 value = strtoul(parser->previous.value, NULL, 0);
            ir_program_add(program, OP_EQU, (u64)label_name, value, 0);
        } else {
            fprintf(stderr, "语法错误: EQU后应有数值 在 %d:%d\n", 
                    parser->current.line, parser->current.column);
            exit(1);
        }
    }
}

/* 解析指令 */
static void parse_tasm_instruction(Parser *parser, IRProgram *program) {
    // 指令名已在词法分析器中解析
    char *inst_name = parser->previous.value;
    int inst_index = is_instruction(inst_name);
    
    if (inst_index < 0) {
        fprintf(stderr, "语法错误: 未知指令 '%s' 在 %d:%d\n", 
                inst_name, parser->current.line, parser->current.column);
        exit(1);
    }
    
    // 解析操作数
    u64 arg1 = 0, arg2 = 0;
    
    // 特殊处理call指令
    if (inst_index == 5) { // call
        if (parser_match(parser, TOK_IDENT)) {
            arg1 = (u64)parser->previous.value; // 标签引用
        } else {
            fprintf(stderr, "语法错误: call指令后应有标签 在 %d:%d\n", 
                    parser->current.line, parser->current.column);
            exit(1);
        }
    } 
    // 特殊处理ret和leave指令
    else if (inst_index == 6 || inst_index == 13) { // ret或leave
        // 这些指令不需要操作数
    } 
    // 处理其他指令
    else if (!parser_check(parser, TOK_EOF) && !parser_check(parser, TOK_COMMENT)) {
        if (parser_match(parser, TOK_REGISTER)) {
            arg1 = is_register(parser->previous.value) | 0x100; // 标记为寄存器
        } else if (parser_match(parser, TOK_NUMBER)) {
            arg1 = strtoul(parser->previous.value, NULL, 0); // 立即数
        } else if (parser_match(parser, TOK_IDENT)) {
            arg1 = (u64)parser->previous.value; // 标识符/标签引用
        } else {
            fprintf(stderr, "语法错误: 预期操作数 在 %d:%d\n", 
                    parser->current.line, parser->current.column);
            exit(1);
        }
        
        // 可选的第二个操作数
        if (parser_match(parser, TOK_COMMA)) {
            if (parser_match(parser, TOK_REGISTER)) {
                arg2 = is_register(parser->previous.value) | 0x100; // 标记为寄存器
            } else if (parser_match(parser, TOK_NUMBER)) {
                arg2 = strtoul(parser->previous.value, NULL, 0); // 立即数
            } else if (parser_match(parser, TOK_IDENT)) {
                arg2 = (u64)parser->previous.value; // 标识符/标签引用
            } else {
                fprintf(stderr, "语法错误: 预期操作数 在 %d:%d\n", 
                        parser->current.line, parser->current.column);
                exit(1);
            }
        }
    }
    
    // 添加指令
    OpCode op;
    switch (inst_index) {
        case 0: op = OP_MOV; break;
        case 1: op = OP_ADD; break;
        case 2: op = OP_SUB; break;
        case 3: op = OP_PUSH; break;
        case 4: op = OP_POP; break;
        case 5: op = OP_CALL; break;
        case 6: op = OP_RET; break;
        case 7: op = OP_JMP; break;
        case 8: op = OP_JZ; break;  // je
        case 9: op = OP_JNZ; break; // jne
        case 10: op = OP_JZ; break; // jz
        case 11: op = OP_JNZ; break; // jnz
        case 12: op = OP_XOR; break;
        case 13: op = OP_LEAVE; break;
        default: op = OP_NOP; break;
    }
    
    ir_program_add(program, op, arg1, arg2, 0);
}

/* 解析数据定义 */
static void parse_tasm_data_definition(Parser *parser, IRProgram *program, OpCode op) {
    // 解析数据值
    if (parser_match(parser, TOK_NUMBER)) {
        u64 value = strtoul(parser->previous.value, NULL, 0);
        ir_program_add(program, op, value, 0, 0);
    } else if (parser_match(parser, TOK_STRING)) {
        char *str = parser->previous.value;
        size_t len = strlen(str);
        
        // 对于字符串，生成多个字节定义
        for (size_t i = 0; i < len; i++) {
            ir_program_add(program, OP_DB, (u64)str[i], 0, 0);
        }
    } else {
        fprintf(stderr, "语法错误: 预期数据值 在 %d:%d\n", 
                parser->current.line, parser->current.column);
        exit(1);
    }
}

/* 解析EQU指令 */
static void parse_tasm_equ(Parser *parser, IRProgram *program) {
    // 检查是否是"identifier equ value"或"identifier: equ value"格式
    if (parser->previous.type != TOK_IDENT && parser->previous.type != TOK_LABEL) {
        fprintf(stderr, "语法错误: EQU前应有标识符 在 %d:%d\n", 
                parser->current.line, parser->current.column);
        exit(1);
    }
    
    char *name = parser->previous.value;
    
    // 解析值
    if (parser_match(parser, TOK_NUMBER)) {
        u64 value = strtoul(parser->previous.value, NULL, 0);
        ir_program_add(program, OP_EQU, (u64)name, value, 0);
    } else {
        fprintf(stderr, "语法错误: EQU后应有数值 在 %d:%d\n", 
                parser->current.line, parser->current.column);
        exit(1);
    }
}

/* 解析TIMES指令 */
static void parse_tasm_times(Parser *parser, IRProgram *program) {
    // 解析重复次数
    u64 count = 0;
    if (parser_match(parser, TOK_NUMBER)) {
        count = strtoul(parser->previous.value, NULL, 0);
    } else {
        fprintf(stderr, "语法错误: TIMES后应有数值 在 %d:%d\n", 
                parser->current.line, parser->current.column);
        exit(1);
    }
    
    // 解析要重复的指令或数据定义
    OpCode repeat_op = OP_NOP;
    u64 repeat_value = 0;
    
    if (parser_match(parser, TOK_DB)) {
        repeat_op = OP_DB;
        if (parser_match(parser, TOK_NUMBER)) {
            repeat_value = strtoul(parser->previous.value, NULL, 0);
        } else {
            fprintf(stderr, "语法错误: 预期数据值 在 %d:%d\n", 
                    parser->current.line, parser->current.column);
            exit(1);
        }
    } else if (parser_match(parser, TOK_DW)) {
        repeat_op = OP_DW;
        if (parser_match(parser, TOK_NUMBER)) {
            repeat_value = strtoul(parser->previous.value, NULL, 0);
        } else {
            fprintf(stderr, "语法错误: 预期数据值 在 %d:%d\n", 
                    parser->current.line, parser->current.column);
            exit(1);
        }
    } else if (parser_match(parser, TOK_DD)) {
        repeat_op = OP_DD;
        if (parser_match(parser, TOK_NUMBER)) {
            repeat_value = strtoul(parser->previous.value, NULL, 0);
        } else {
            fprintf(stderr, "语法错误: 预期数据值 在 %d:%d\n", 
                    parser->current.line, parser->current.column);
            exit(1);
        }
    } else if (parser_match(parser, TOK_DQ)) {
        repeat_op = OP_DQ;
        if (parser_match(parser, TOK_NUMBER)) {
            repeat_value = strtoul(parser->previous.value, NULL, 0);
        } else {
            fprintf(stderr, "语法错误: 预期数据值 在 %d:%d\n", 
                    parser->current.line, parser->current.column);
            exit(1);
        }
    } else {
        fprintf(stderr, "语法错误: TIMES后应有数据定义 在 %d:%d\n", 
                parser->current.line, parser->current.column);
        exit(1);
    }
    
    // 添加TIMES指令
    ir_program_add(program, OP_TIMES, count, repeat_op, repeat_value);
}

/*=====================================================
 * 代码生成器实现
 *=====================================================*/

/* 创建代码生成器 */
static CodeGen* codegen_create(IRProgram *program) {
    CodeGen *codegen = (CodeGen*)malloc(sizeof(CodeGen));
    if (!codegen) return NULL;
    
    codegen->program = program;
    codegen->symbols = (SymbolTable*)malloc(sizeof(SymbolTable));
    codegen->symbols->capacity = 1024;
    codegen->symbols->count = 0;
    codegen->symbols->symbols = (Symbol*)malloc(codegen->symbols->capacity * sizeof(Symbol));
    
    codegen->constants = (ConstantTable*)malloc(sizeof(ConstantTable));
    codegen->constants->capacity = 64;
    codegen->constants->count = 0;
    codegen->constants->constants = (Constant*)malloc(codegen->constants->capacity * sizeof(Constant));
    
    codegen->code_capacity = 65536;
    codegen->code_size = 0;
    codegen->code = (u8*)malloc(codegen->code_capacity);
    
    return codegen;
}

/* 销毁代码生成器 */
static void codegen_destroy(CodeGen *codegen) {
    if (codegen) {
        if (codegen->symbols) {
            if (codegen->symbols->symbols) {
                free(codegen->symbols->symbols);
            }
            free(codegen->symbols);
        }
        if (codegen->code) {
            free(codegen->code);
        }
        free(codegen);
    }
}

/* 添加符号 */
static void codegen_add_symbol(CodeGen *codegen, const char *name, u32 addr, u32 size) {
    if (codegen->symbols->count >= codegen->symbols->capacity) {
        codegen->symbols->capacity *= 2;
        codegen->symbols->symbols = (Symbol*)realloc(
            codegen->symbols->symbols, 
            codegen->symbols->capacity * sizeof(Symbol)
        );
    }
    
    Symbol *symbol = &codegen->symbols->symbols[codegen->symbols->count];
    symbol->name = strdup(name);
    symbol->addr = addr;
    symbol->size = size;
    codegen->symbols->count++;
}

/* 查找符号 */
static Symbol* codegen_find_symbol(CodeGen *codegen, const char *name) {
    for (u32 i = 0; i < codegen->symbols->count; i++) {
        if (strcmp(codegen->symbols->symbols[i].name, name) == 0) {
            return &codegen->symbols->symbols[i];
        }
    }
    return NULL;
}

/* 添加代码 */
static void codegen_emit_byte(CodeGen *codegen, u8 byte) {
    if (codegen->code_size >= codegen->code_capacity) {
        codegen->code_capacity *= 2;
        codegen->code = (u8*)realloc(codegen->code, codegen->code_capacity);
    }
    
    codegen->code[codegen->code_size++] = byte;
}

static void codegen_emit_word(CodeGen *codegen, u16 word) {
    codegen_emit_byte(codegen, word & 0xFF);
    codegen_emit_byte(codegen, (word >> 8) & 0xFF);
}

static void codegen_emit_dword(CodeGen *codegen, u32 dword) {
    codegen_emit_byte(codegen, dword & 0xFF);
    codegen_emit_byte(codegen, (dword >> 8) & 0xFF);
    codegen_emit_byte(codegen, (dword >> 16) & 0xFF);
    codegen_emit_byte(codegen, (dword >> 24) & 0xFF);
}

static void codegen_emit_qword(CodeGen *codegen, u64 qword) {
    codegen_emit_dword(codegen, qword & 0xFFFFFFFF);
    codegen_emit_dword(codegen, (qword >> 32) & 0xFFFFFFFF);
}

static void codegen_emit_bytes(CodeGen *codegen, const u8 *bytes, u32 count) {
    for (u32 i = 0; i < count; i++) {
        codegen_emit_byte(codegen, bytes[i]);
    }
}

static void codegen_emit_string(CodeGen *codegen, const char *str) {
    while (*str) {
        codegen_emit_byte(codegen, *str++);
    }
    codegen_emit_byte(codegen, 0); // 结束符
}

/* 添加常量 */
static void codegen_add_constant(CodeGen *codegen, const char *name, u64 value) {
    // 检查是否已经存在
    for (u32 i = 0; i < codegen->constants->count; i++) {
        if (strcmp(codegen->constants->constants[i].name, name) == 0) {
            // 已存在，更新值
            codegen->constants->constants[i].value = value;
            return;
        }
    }
    
    // 扩容检查
    if (codegen->constants->count >= codegen->constants->capacity) {
        codegen->constants->capacity *= 2;
        codegen->constants->constants = (Constant*)realloc(
            codegen->constants->constants, 
            codegen->constants->capacity * sizeof(Constant)
        );
    }
    
    // 添加新常量
    Constant *constant = &codegen->constants->constants[codegen->constants->count];
    constant->name = strdup(name);
    constant->value = value;
    codegen->constants->count++;
}

/* 查找常量 */
static Constant* codegen_find_constant(CodeGen *codegen, const char *name) {
    for (u32 i = 0; i < codegen->constants->count; i++) {
        if (strcmp(codegen->constants->constants[i].name, name) == 0) {
            return &codegen->constants->constants[i];
        }
    }
    return NULL;
}

/* 生成x86-64指令 */
static void codegen_generate_x86_64(CodeGen *codegen) {
    // 当前代码偏移
    u32 code_offset = 0;
    
    // 第一遍：收集标签和符号
    for (u32 i = 0; i < codegen->program->count; i++) {
        IRInst *inst = &codegen->program->insts[i];
        
        switch (inst->op) {
            case OP_LABEL:
                codegen_add_symbol(codegen, (const char*)inst->arg1, code_offset, 0);
                break;
            case OP_DB:
                code_offset += 1;
                break;
            case OP_DW:
                code_offset += 2;
                break;
            case OP_DD:
                code_offset += 4;
                break;
            case OP_DQ:
                code_offset += 8;
                break;
            case OP_TIMES:
                switch (inst->arg2) {
                    case OP_DB: code_offset += inst->arg1 * 1; break;
                    case OP_DW: code_offset += inst->arg1 * 2; break;
                    case OP_DD: code_offset += inst->arg1 * 4; break;
                    case OP_DQ: code_offset += inst->arg1 * 8; break;
                    default: break;
                }
                break;
            case OP_MOV:
                code_offset += 3; // 简化处理，实际长度可能不同
                break;
            case OP_ADD:
            case OP_SUB:
                code_offset += 3;
                break;
            case OP_PUSH:
                code_offset += 1;
                break;
            case OP_POP:
                code_offset += 1;
                break;
            case OP_CALL:
                code_offset += 5;
                break;
            case OP_RET:
                code_offset += 1;
                break;
            case OP_JMP:
                code_offset += 5;
                break;
            case OP_JZ:
            case OP_JNZ:
                code_offset += 6;
                break;
            case OP_XOR:
                code_offset += 3;
                break;
            case OP_LEAVE:
                code_offset += 1;
                break;
            default:
                break;
        }
    }
    
    // 第二遍：生成代码
    for (u32 i = 0; i < codegen->program->count; i++) {
        IRInst *inst = &codegen->program->insts[i];
        
        switch (inst->op) {
            case OP_DB:
                codegen_emit_byte(codegen, inst->arg1);
                break;
            case OP_DW:
                codegen_emit_word(codegen, inst->arg1);
                break;
            case OP_DD:
                codegen_emit_dword(codegen, inst->arg1);
                break;
            case OP_DQ:
                codegen_emit_qword(codegen, inst->arg1);
                break;
            case OP_TIMES:
                for (u64 j = 0; j < inst->arg1; j++) {
                    switch (inst->arg2) {
                        case OP_DB: codegen_emit_byte(codegen, inst->arg3); break;
                        case OP_DW: codegen_emit_word(codegen, inst->arg3); break;
                        case OP_DD: codegen_emit_dword(codegen, inst->arg3); break;
                        case OP_DQ: codegen_emit_qword(codegen, inst->arg3); break;
                        default: break;
                    }
                }
                break;
            case OP_MOV:
                // 简化的MOV实现
                if ((inst->arg1 & 0x100) && (inst->arg2 & 0x100)) {
                    // MOV r64, r64
                    codegen_emit_byte(codegen, 0x48); // REX.W
                    codegen_emit_byte(codegen, 0x89); // MOV r/m64, r64
                    codegen_emit_byte(codegen, 0xC0 | ((inst->arg1 & 0xFF) << 3) | (inst->arg2 & 0xFF)); // ModR/M
                } else if (inst->arg1 & 0x100) {
                    // MOV r64, imm32
                    codegen_emit_byte(codegen, 0x48); // REX.W
                    codegen_emit_byte(codegen, 0xC7); // MOV r/m64, imm32
                    codegen_emit_byte(codegen, 0xC0 | (inst->arg1 & 0xFF)); // ModR/M
                    codegen_emit_dword(codegen, inst->arg2); // imm32
                }
                break;
            case OP_PUSH:
                // PUSH r64
                if (inst->arg1 & 0x100) {
                    codegen_emit_byte(codegen, 0x50 | (inst->arg1 & 0x7)); // PUSH r64
                } else {
                    // PUSH imm32
                    codegen_emit_byte(codegen, 0x68);
                    codegen_emit_dword(codegen, inst->arg1);
                }
                break;
            case OP_POP:
                // POP r64
                codegen_emit_byte(codegen, 0x58 | (inst->arg1 & 0x7)); // POP r64
                break;
            case OP_CALL:
                // CALL rel32
                codegen_emit_byte(codegen, 0xE8);
                {
                    Symbol *sym = codegen_find_symbol(codegen, (const char*)inst->arg1);
                    if (sym) {
                        i32 rel = sym->addr - (codegen->code_size + 4);
                        codegen_emit_dword(codegen, rel);
                    } else {
                        // 未解析的符号，先填0
                        codegen_emit_dword(codegen, 0);
                    }
                }
                break;
            case OP_RET:
                // RET
                codegen_emit_byte(codegen, 0xC3);
                break;
            case OP_JMP:
                // JMP rel32
                codegen_emit_byte(codegen, 0xE9);
                {
                    Symbol *sym = codegen_find_symbol(codegen, (const char*)inst->arg1);
                    if (sym) {
                        i32 rel = sym->addr - (codegen->code_size + 4);
                        codegen_emit_dword(codegen, rel);
                    } else {
                        // 未解析的符号，先填0
                        codegen_emit_dword(codegen, 0);
                    }
                }
                break;
            case OP_JZ:
                // JE/JZ rel32
                codegen_emit_byte(codegen, 0x0F);
                codegen_emit_byte(codegen, 0x84);
                {
                    Symbol *sym = codegen_find_symbol(codegen, (const char*)inst->arg1);
                    if (sym) {
                        i32 rel = sym->addr - (codegen->code_size + 4);
                        codegen_emit_dword(codegen, rel);
                    } else {
                        // 未解析的符号，先填0
                        codegen_emit_dword(codegen, 0);
                    }
                }
                break;
            case OP_JNZ:
                // JNE/JNZ rel32
                codegen_emit_byte(codegen, 0x0F);
                codegen_emit_byte(codegen, 0x85);
                {
                    Symbol *sym = codegen_find_symbol(codegen, (const char*)inst->arg1);
                    if (sym) {
                        i32 rel = sym->addr - (codegen->code_size + 4);
                        codegen_emit_dword(codegen, rel);
                    } else {
                        // 未解析的符号，先填0
                        codegen_emit_dword(codegen, 0);
                    }
                }
                break;
            case OP_XOR:
                // XOR r64, r64
                codegen_emit_byte(codegen, 0x48); // REX.W
                codegen_emit_byte(codegen, 0x31); // XOR r/m64, r64
                codegen_emit_byte(codegen, 0xC0 | ((inst->arg1 & 0xFF) << 3) | (inst->arg2 & 0xFF)); // ModR/M
                break;
            case OP_LEAVE:
                // LEAVE
                codegen_emit_byte(codegen, 0xC9);
                break;
            default:
                break;
        }
    }
}

/* 写入PE格式文件 */
static int codegen_write_pe(CodeGen *codegen, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "无法创建输出文件: %s\n", filename);
        return 1;
    }
    
    // PE文件常量
    const u16 IMAGE_DOS_SIGNATURE = 0x5A4D;      // MZ
    const u32 IMAGE_NT_SIGNATURE = 0x00004550;   // PE00
    const u16 IMAGE_FILE_MACHINE_AMD64 = 0x8664;
    const u16 IMAGE_FILE_EXECUTABLE_IMAGE = 0x0002;
    const u16 IMAGE_FILE_RELOCS_STRIPPED = 0x0001;
    const u16 IMAGE_SUBSYSTEM_WINDOWS_CUI = 0x0003;
    const u32 IMAGE_SCN_CNT_CODE = 0x00000020;
    const u32 IMAGE_SCN_MEM_EXECUTE = 0x20000000;
    const u32 IMAGE_SCN_MEM_READ = 0x40000000;
    
    // 计算各种大小
    const u32 dos_header_size = 64;
    const u32 file_header_size = 20;
    const u32 optional_header_size = 240;
    const u32 section_header_size = 40;
    const u32 headers_size = dos_header_size + 4 + file_header_size + optional_header_size + section_header_size;
    const u32 code_size = codegen->code_size;
    const u32 file_size = headers_size + code_size;
    
    // DOS头
    u8 dos_header[64] = {0};
    dos_header[0] = IMAGE_DOS_SIGNATURE & 0xFF;
    dos_header[1] = (IMAGE_DOS_SIGNATURE >> 8) & 0xFF;
    *(u32*)(dos_header + 60) = dos_header_size;
    
    // PE头
    u8 pe_header[4] = {
        IMAGE_NT_SIGNATURE & 0xFF,
        (IMAGE_NT_SIGNATURE >> 8) & 0xFF,
        (IMAGE_NT_SIGNATURE >> 16) & 0xFF,
        (IMAGE_NT_SIGNATURE >> 24) & 0xFF
    };
    
    // 文件头
    u8 file_header[20] = {0};
    *(u16*)(file_header) = IMAGE_FILE_MACHINE_AMD64;
    *(u16*)(file_header + 2) = 1; // NumberOfSections
    *(u16*)(file_header + 16) = optional_header_size; // SizeOfOptionalHeader
    *(u16*)(file_header + 18) = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_RELOCS_STRIPPED; // Characteristics
    
    // 可选头
    u8 optional_header[240] = {0};
    *(u16*)(optional_header) = 0x020B; // Magic (PE32+)
    *(u32*)(optional_header + 16) = code_size; // SizeOfCode
    *(u32*)(optional_header + 28) = headers_size; // AddressOfEntryPoint
    *(u32*)(optional_header + 32) = headers_size; // BaseOfCode
    *(u64*)(optional_header + 40) = 0x400000; // ImageBase
    *(u32*)(optional_header + 48) = 0x1000; // SectionAlignment
    *(u32*)(optional_header + 52) = 0x200; // FileAlignment
    *(u16*)(optional_header + 56) = 6; // MajorOperatingSystemVersion
    *(u16*)(optional_header + 62) = 6; // MajorSubsystemVersion
    *(u32*)(optional_header + 72) = file_size; // SizeOfImage
    *(u32*)(optional_header + 76) = headers_size; // SizeOfHeaders
    *(u16*)(optional_header + 84) = IMAGE_SUBSYSTEM_WINDOWS_CUI; // Subsystem
    *(u64*)(optional_header + 88) = 0x100000; // SizeOfStackReserve
    *(u64*)(optional_header + 96) = 0x1000; // SizeOfStackCommit
    *(u64*)(optional_header + 104) = 0x100000; // SizeOfHeapReserve
    *(u64*)(optional_header + 112) = 0x1000; // SizeOfHeapCommit
    *(u32*)(optional_header + 120) = 16; // NumberOfRvaAndSizes
    
    // 节头
    u8 section_header[40] = {0};
    memcpy(section_header, ".text", 5); // Name
    *(u32*)(section_header + 8) = code_size; // VirtualSize
    *(u32*)(section_header + 12) = headers_size; // VirtualAddress
    *(u32*)(section_header + 16) = code_size; // SizeOfRawData
    *(u32*)(section_header + 20) = headers_size; // PointerToRawData
    *(u32*)(section_header + 36) = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ; // Characteristics
    
    // 写入文件
    fwrite(dos_header, 1, dos_header_size, file);
    fwrite(pe_header, 1, 4, file);
    fwrite(file_header, 1, file_header_size, file);
    fwrite(optional_header, 1, optional_header_size, file);
    fwrite(section_header, 1, section_header_size, file);
    fwrite(codegen->code, 1, codegen->code_size, file);
    
    fclose(file);
    return 0;
}

/* 写入ELF格式文件 */
static int codegen_write_elf(CodeGen *codegen, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "无法创建输出文件: %s\n", filename);
        return 1;
    }
    
    // ELF文件常量
    const u32 EI_NIDENT = 16;
    const u32 EI_MAG0 = 0;
    const u32 EI_MAG1 = 1;
    const u32 EI_MAG2 = 2;
    const u32 EI_MAG3 = 3;
    const u32 EI_CLASS = 4;
    const u32 EI_DATA = 5;
    const u32 EI_VERSION = 6;
    const u32 EI_OSABI = 7;
    const u32 EI_ABIVERSION = 8;
    
    const u8 ELFMAG0 = 0x7F;
    const u8 ELFMAG1 = 'E';
    const u8 ELFMAG2 = 'L';
    const u8 ELFMAG3 = 'F';
    const u8 ELFCLASS64 = 2;
    const u8 ELFDATA2LSB = 1;
    const u8 EV_CURRENT = 1;
    const u8 ELFOSABI_SYSV = 0;
    
    const u16 ET_EXEC = 2;
    const u16 EM_X86_64 = 62;
    
    const u32 PF_X = 1;
    const u32 PF_W = 2;
    const u32 PF_R = 4;
    
    const u32 PT_LOAD = 1;
    
    // 计算各种大小
    const u64 elf_header_size = 64;
    const u64 program_header_size = 56;
    const u64 headers_size = elf_header_size + program_header_size;
    const u64 code_size = codegen->code_size;
    const u64 file_size = headers_size + code_size;
    
    // ELF头
    u8 elf_header[64] = {0};
    elf_header[EI_MAG0] = ELFMAG0;
    elf_header[EI_MAG1] = ELFMAG1;
    elf_header[EI_MAG2] = ELFMAG2;
    elf_header[EI_MAG3] = ELFMAG3;
    elf_header[EI_CLASS] = ELFCLASS64;
    elf_header[EI_DATA] = ELFDATA2LSB;
    elf_header[EI_VERSION] = EV_CURRENT;
    elf_header[EI_OSABI] = ELFOSABI_SYSV;
    elf_header[EI_ABIVERSION] = 0;
    
    *(u16*)(elf_header + 16) = ET_EXEC; // e_type
    *(u16*)(elf_header + 18) = EM_X86_64; // e_machine
    *(u32*)(elf_header + 20) = EV_CURRENT; // e_version
    *(u64*)(elf_header + 24) = headers_size; // e_entry
    *(u64*)(elf_header + 32) = elf_header_size; // e_phoff
    *(u64*)(elf_header + 40) = 0; // e_shoff
    *(u32*)(elf_header + 48) = 0; // e_flags
    *(u16*)(elf_header + 52) = elf_header_size; // e_ehsize
    *(u16*)(elf_header + 54) = program_header_size; // e_phentsize
    *(u16*)(elf_header + 56) = 1; // e_phnum
    *(u16*)(elf_header + 58) = 0; // e_shentsize
    *(u16*)(elf_header + 60) = 0; // e_shnum
    *(u16*)(elf_header + 62) = 0; // e_shstrndx
    
    // 程序头
    u8 program_header[56] = {0};
    *(u32*)(program_header + 0) = PT_LOAD; // p_type
    *(u32*)(program_header + 4) = PF_X | PF_R; // p_flags
    *(u64*)(program_header + 8) = 0; // p_offset
    *(u64*)(program_header + 16) = 0x400000; // p_vaddr
    *(u64*)(program_header + 24) = 0x400000; // p_paddr
    *(u64*)(program_header + 32) = file_size; // p_filesz
    *(u64*)(program_header + 40) = file_size; // p_memsz
    *(u64*)(program_header + 48) = 0x1000; // p_align
    
    // 写入文件
    fwrite(elf_header, 1, elf_header_size, file);
    fwrite(program_header, 1, program_header_size, file);
    fwrite(codegen->code, 1, codegen->code_size, file);
    
    fclose(file);
    return 0;
}

/**
 * 主函数
 */
int main(int argc, char *argv[]) {
    // 设置本地化
    setlocale(LC_ALL, "");
    
    if (argc != 3) {
        fprintf(stderr, "用法: %s <输入文件> <输出文件>\n", argv[0]);
        return 1;
    }
    
    const char *input_file = argv[1];
    const char *output_file = argv[2];
    
    // 读取输入文件
    FILE *file = fopen(input_file, "r");
    if (!file) {
        fprintf(stderr, "无法打开输入文件: %s\n", input_file);
        return 1;
    }
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // 读取文件内容
    char *source = (char*)malloc(file_size + 1);
    fread(source, 1, file_size, file);
    source[file_size] = '\0';
    fclose(file);
    
    // 编译过程
    Lexer *lexer = lexer_create(source);
    Parser *parser = parser_create(lexer);
    IRProgram *program = parser_parse(parser);
    CodeGen *codegen = codegen_create(program);
    
    // 生成代码
    codegen_generate_x86_64(codegen);
    
    // 写入输出文件
    int result;
    const char *ext = strrchr(output_file, '.');
    if (ext && strcmp(ext, ".exe") == 0) {
        // 输出PE格式
        result = codegen_write_pe(codegen, output_file);
    } else {
        // 输出ELF格式
        result = codegen_write_elf(codegen, output_file);
    }
    
    // 清理资源
    codegen_destroy(codegen);
    ir_program_destroy(program);
    parser_destroy(parser);
    lexer_destroy(lexer);
    free(source);
    
    printf("编译完成: %s -> %s\n", input_file, output_file);
    
    return result;
}

/* 
 * 接下来将实现:
 * 4. 代码生成
 */ 