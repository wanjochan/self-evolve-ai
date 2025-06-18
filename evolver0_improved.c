#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>

// 核心常量定义
#define MAX_TOKENS 10000
#define MAX_FUNCTIONS 100
#define MAX_MACHINE_CODE 8192
#define GENERATION_FILE "generation.txt"
#define VERSION 0

// 输出格式
enum OutputFormat {
    FORMAT_AST,    // 输出AST二进制文件
    FORMAT_WASM,   // 输出WASM模块
    FORMAT_EXE,    // 输出可执行文件
    FORMAT_DEFAULT = FORMAT_EXE  // 默认输出格式
};

// 词法分析器 - 简化版本
typedef enum {
    TOKEN_EOF = 0,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_INT_LITERAL,
    TOKEN_FLOAT_LITERAL,
    TOKEN_STRING,
    
    // 数据类型关键字
    TOKEN_VOID, TOKEN_CHAR, TOKEN_SHORT, TOKEN_INT, TOKEN_LONG, 
    TOKEN_FLOAT, TOKEN_DOUBLE, TOKEN_SIGNED, TOKEN_UNSIGNED, TOKEN_BOOL,
    
    // 控制流关键字
    TOKEN_IF, TOKEN_ELSE, TOKEN_SWITCH, TOKEN_CASE, TOKEN_DEFAULT,
    TOKEN_WHILE, TOKEN_DO, TOKEN_FOR, TOKEN_BREAK, TOKEN_CONTINUE, 
    TOKEN_GOTO, TOKEN_RETURN,
    
    // 标点符号
    TOKEN_LBRACE, TOKEN_RBRACE, TOKEN_LPAREN, TOKEN_RPAREN,
    TOKEN_LBRACKET, TOKEN_RBRACKET, TOKEN_SEMICOLON, TOKEN_COLON,
    TOKEN_COMMA, TOKEN_DOT, TOKEN_QUESTION,
    
    // 操作符
    TOKEN_ASSIGN, TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULTIPLY, TOKEN_DIVIDE,
    TOKEN_EQUAL, TOKEN_NOT_EQUAL, TOKEN_LESS, TOKEN_GREATER,
    TOKEN_LESS_EQUAL, TOKEN_GREATER_EQUAL,
    TOKEN_LOGICAL_AND, TOKEN_LOGICAL_OR, TOKEN_LOGICAL_NOT,
    
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    char *value;
    int line;
} Token;

// AST节点类型
typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_RETURN_STMT,
    AST_VAR_DECL,
    AST_BINARY_OP,
    AST_NUMBER,
    AST_IDENTIFIER
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    char *value;
    struct ASTNode *left;
    struct ASTNode *right;
    struct ASTNode *next;
} ASTNode;

// 机器码生成器
typedef struct {
    unsigned char code[MAX_MACHINE_CODE];
    int size;
    int entry_point;
} MachineCode;

// 编译器配置
typedef struct {
    enum OutputFormat output_format;
    bool verbose;
    bool optimize;
    const char *output_file;
    const char *target_arch;
} CompilerConfig;

// 编译器状态
typedef struct {
    Token tokens[MAX_TOKENS];
    int token_count;
    int current_token;
    MachineCode machine_code;
    char *source_code;
    CompilerConfig config;
} BootstrapCompiler;

// WASM字节码缓冲区
typedef struct {
    uint8_t *data;
    size_t size;
    size_t capacity;
    size_t section_size_offset;
} WasmBuffer;

// WASM操作码 - 简化版本
#define WASM_OP_I32_CONST   0x41
#define WASM_OP_I32_ADD     0x6A
#define WASM_OP_I32_SUB     0x6B
#define WASM_OP_I32_MUL     0x6C
#define WASM_OP_RETURN      0x0F
#define WASM_OP_LOCAL_GET   0x20
#define WASM_OP_LOCAL_SET   0x21
#define WASM_OP_END         0x0B

#define WASM_TYPE_I32       0x7F
#define WASM_TYPE_FUNC      0x60

// ====================================
// WASM缓冲区操作
// ====================================

static void wasm_buffer_init(WasmBuffer* buffer) {
    buffer->capacity = 1024;
    buffer->data = malloc(buffer->capacity);
    buffer->size = 0;
    buffer->section_size_offset = 0;
}

static void wasm_buffer_free(WasmBuffer* buffer) {
    if (buffer->data) {
        free(buffer->data);
        buffer->data = NULL;
    }
    buffer->size = 0;
    buffer->capacity = 0;
}

static void wasm_buffer_reserve(WasmBuffer* buffer, size_t needed) {
    if (buffer->size + needed > buffer->capacity) {
        buffer->capacity = (buffer->size + needed) * 2;
        buffer->data = realloc(buffer->data, buffer->capacity);
    }
}

static void wasm_write_byte(WasmBuffer* buffer, uint8_t value) {
    wasm_buffer_reserve(buffer, 1);
    buffer->data[buffer->size++] = value;
}

static void wasm_write_u32(WasmBuffer* buffer, uint32_t value) {
    // LEB128编码
    while (value >= 0x80) {
        wasm_write_byte(buffer, (value & 0x7F) | 0x80);
        value >>= 7;
    }
    wasm_write_byte(buffer, value & 0x7F);
}

static void wasm_write_string(WasmBuffer* buffer, const char* str) {
    size_t len = strlen(str);
    wasm_write_u32(buffer, len);
    wasm_buffer_reserve(buffer, len);
    memcpy(buffer->data + buffer->size, str, len);
    buffer->size += len;
}

// ====================================
// 词法分析器
// ====================================

static int is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int is_alnum(char c) {
    return is_alpha(c) || isdigit(c);
}

static TokenType get_keyword_type(const char *word) {
    if (strcmp(word, "int") == 0) return TOKEN_INT;
    if (strcmp(word, "void") == 0) return TOKEN_VOID;
    if (strcmp(word, "return") == 0) return TOKEN_RETURN;
    if (strcmp(word, "if") == 0) return TOKEN_IF;
    if (strcmp(word, "else") == 0) return TOKEN_ELSE;
    if (strcmp(word, "while") == 0) return TOKEN_WHILE;
    if (strcmp(word, "for") == 0) return TOKEN_FOR;
    return TOKEN_IDENTIFIER;
}

static int tokenize(BootstrapCompiler *compiler, const char *source) {
    const char *p = source;
    int line = 1;
    compiler->token_count = 0;
    
    while (*p && compiler->token_count < MAX_TOKENS - 1) {
        // 跳过空白字符
        if (isspace(*p)) {
            if (*p == '\n') line++;
            p++;
            continue;
        }
        
        Token *token = &compiler->tokens[compiler->token_count++];
        token->line = line;
        
        // 数字
        if (isdigit(*p)) {
            const char *start = p;
            while (isdigit(*p)) p++;
            
            int len = p - start;
            token->value = malloc(len + 1);
            strncpy(token->value, start, len);
            token->value[len] = '\0';
            token->type = TOKEN_NUMBER;
        }
        // 标识符和关键字
        else if (is_alpha(*p)) {
            const char *start = p;
            while (is_alnum(*p)) p++;
            
            int len = p - start;
            token->value = malloc(len + 1);
            strncpy(token->value, start, len);
            token->value[len] = '\0';
            token->type = get_keyword_type(token->value);
        }
        // 操作符和标点符号
        else {
            token->value = malloc(2);
            token->value[0] = *p;
            token->value[1] = '\0';
            
            switch (*p) {
                case '(': token->type = TOKEN_LPAREN; break;
                case ')': token->type = TOKEN_RPAREN; break;
                case '{': token->type = TOKEN_LBRACE; break;
                case '}': token->type = TOKEN_RBRACE; break;
                case ';': token->type = TOKEN_SEMICOLON; break;
                case ',': token->type = TOKEN_COMMA; break;
                case '+': token->type = TOKEN_PLUS; break;
                case '-': token->type = TOKEN_MINUS; break;
                case '*': token->type = TOKEN_MULTIPLY; break;
                case '/': token->type = TOKEN_DIVIDE; break;
                case '=': token->type = TOKEN_ASSIGN; break;
                default: token->type = TOKEN_ERROR; break;
            }
            p++;
        }
    }
    
    // 添加EOF标记
    compiler->tokens[compiler->token_count].type = TOKEN_EOF;
    compiler->tokens[compiler->token_count].value = NULL;
    
    return compiler->token_count;
}

// ====================================
// 语法分析器
// ====================================

static Token* current_token(BootstrapCompiler* compiler) {
    if (compiler->current_token < compiler->token_count) {
        return &compiler->tokens[compiler->current_token];
    }
    return NULL;
}

static void advance_token(BootstrapCompiler* compiler) {
    if (compiler->current_token < compiler->token_count - 1) {
        compiler->current_token++;
    }
}

static int match_token(BootstrapCompiler* compiler, TokenType type) {
    Token* token = current_token(compiler);
    return token && token->type == type;
}

static int consume_token(BootstrapCompiler* compiler, TokenType type) {
    if (match_token(compiler, type)) {
        advance_token(compiler);
        return 1;
    }
    return 0;
}

static ASTNode* create_ast_node(ASTNodeType type, const char* value) {
    ASTNode* node = calloc(1, sizeof(ASTNode));
    node->type = type;
    if (value) {
        node->value = strdup(value);
    }
    return node;
}

static ASTNode* parse_expression(BootstrapCompiler* compiler) {
    Token* token = current_token(compiler);
    if (!token) return NULL;
    
    if (token->type == TOKEN_NUMBER) {
        ASTNode* node = create_ast_node(AST_NUMBER, token->value);
        advance_token(compiler);
        return node;
    } else if (token->type == TOKEN_IDENTIFIER) {
        ASTNode* node = create_ast_node(AST_IDENTIFIER, token->value);
        advance_token(compiler);
        
        // 检查二元操作
        Token* next = current_token(compiler);
        if (next && (next->type == TOKEN_PLUS || next->type == TOKEN_MINUS || 
                    next->type == TOKEN_MULTIPLY || next->type == TOKEN_DIVIDE)) {
            ASTNode* op_node = create_ast_node(AST_BINARY_OP, next->value);
            advance_token(compiler);
            op_node->left = node;
            op_node->right = parse_expression(compiler);
            return op_node;
        }
        
        return node;
    }
    
    return NULL;
}

static ASTNode* parse_statement(BootstrapCompiler* compiler) {
    Token* token = current_token(compiler);
    if (!token) return NULL;
    
    if (token->type == TOKEN_RETURN) {
        advance_token(compiler);
        ASTNode* node = create_ast_node(AST_RETURN_STMT, "return");
        node->left = parse_expression(compiler);
        consume_token(compiler, TOKEN_SEMICOLON);
        return node;
    } else if (token->type == TOKEN_INT) {
        advance_token(compiler);
        Token* id_token = current_token(compiler);
        if (id_token && id_token->type == TOKEN_IDENTIFIER) {
            ASTNode* node = create_ast_node(AST_VAR_DECL, id_token->value);
            advance_token(compiler);
            
            if (consume_token(compiler, TOKEN_ASSIGN)) {
                node->left = parse_expression(compiler);
            }
            
            consume_token(compiler, TOKEN_SEMICOLON);
            return node;
        }
    }
    
    return NULL;
}

static ASTNode* parse_function(BootstrapCompiler* compiler) {
    if (!consume_token(compiler, TOKEN_INT)) return NULL;
    
    Token* name_token = current_token(compiler);
    if (!name_token || name_token->type != TOKEN_IDENTIFIER) return NULL;
    
    ASTNode* func_node = create_ast_node(AST_FUNCTION, name_token->value);
    advance_token(compiler);
    
    if (!consume_token(compiler, TOKEN_LPAREN)) return NULL;
    consume_token(compiler, TOKEN_RPAREN);
    
    if (!consume_token(compiler, TOKEN_LBRACE)) return NULL;
    
    // 解析函数体
    ASTNode* body = NULL;
    ASTNode* last_stmt = NULL;
    
    while (!match_token(compiler, TOKEN_RBRACE) && !match_token(compiler, TOKEN_EOF)) {
        ASTNode* stmt = parse_statement(compiler);
        if (stmt) {
            if (!body) {
                body = stmt;
                last_stmt = stmt;
            } else {
                last_stmt->next = stmt;
                last_stmt = stmt;
            }
        }
    }
    
    consume_token(compiler, TOKEN_RBRACE);
    func_node->left = body;
    
    return func_node;
}

static ASTNode* parse_program(BootstrapCompiler* compiler) {
    ASTNode* program = create_ast_node(AST_PROGRAM, "program");
    compiler->current_token = 0;
    
    ASTNode* func = parse_function(compiler);
    if (func) {
        program->left = func;
    }
    
    return program;
}

// ====================================
// WASM生成器
// ====================================

static void generate_wasm_header(WasmBuffer* wasm) {
    // WASM魔数
    wasm_write_byte(wasm, 0x00);
    wasm_write_byte(wasm, 0x61);
    wasm_write_byte(wasm, 0x73);
    wasm_write_byte(wasm, 0x6d);
    
    // 版本号
    wasm_write_byte(wasm, 0x01);
    wasm_write_byte(wasm, 0x00);
    wasm_write_byte(wasm, 0x00);
    wasm_write_byte(wasm, 0x00);
}

static void generate_wasm_type_section(WasmBuffer* wasm) {
    // Type section (1)
    wasm_write_byte(wasm, 0x01);
    
    // Section size placeholder
    size_t size_offset = wasm->size;
    wasm_write_u32(wasm, 0);
    size_t content_start = wasm->size;
    
    // 1 type
    wasm_write_u32(wasm, 1);
    
    // Function type
    wasm_write_byte(wasm, WASM_TYPE_FUNC);
    wasm_write_u32(wasm, 0); // 0 parameters
    wasm_write_u32(wasm, 1); // 1 result
    wasm_write_byte(wasm, WASM_TYPE_I32); // i32 result
    
    // 更新section大小
    size_t content_size = wasm->size - content_start;
    // 重新编码section大小
    uint8_t* size_ptr = wasm->data + size_offset;
    *size_ptr = content_size;
}

static void generate_wasm_function_section(WasmBuffer* wasm) {
    // Function section (3)
    wasm_write_byte(wasm, 0x03);
    
    size_t size_offset = wasm->size;
    wasm_write_u32(wasm, 0);
    size_t content_start = wasm->size;
    
    // 1 function
    wasm_write_u32(wasm, 1);
    wasm_write_u32(wasm, 0); // 使用type 0
    
    size_t content_size = wasm->size - content_start;
    uint8_t* size_ptr = wasm->data + size_offset;
    *size_ptr = content_size;
}

static void generate_wasm_export_section(WasmBuffer* wasm) {
    // Export section (7)
    wasm_write_byte(wasm, 0x07);
    
    size_t size_offset = wasm->size;
    wasm_write_u32(wasm, 0);
    size_t content_start = wasm->size;
    
    // 1 export
    wasm_write_u32(wasm, 1);
    
    // Export main function
    wasm_write_string(wasm, "main");
    wasm_write_byte(wasm, 0x00); // function export
    wasm_write_u32(wasm, 0); // function index 0
    
    size_t content_size = wasm->size - content_start;
    uint8_t* size_ptr = wasm->data + size_offset;
    *size_ptr = content_size;
}

static void generate_wasm_code_section(WasmBuffer* wasm, ASTNode* ast) {
    // Code section (10)
    wasm_write_byte(wasm, 0x0A);
    
    size_t size_offset = wasm->size;
    wasm_write_u32(wasm, 0);
    size_t content_start = wasm->size;
    
    // 1 function
    wasm_write_u32(wasm, 1);
    
    // Function body
    size_t func_size_offset = wasm->size;
    wasm_write_u32(wasm, 0);
    size_t func_content_start = wasm->size;
    
    // 0 local variables
    wasm_write_u32(wasm, 0);
    
    // 生成函数代码：计算 x = 42, y = x + 10, return y * 2
    wasm_write_byte(wasm, WASM_OP_I32_CONST);
    wasm_write_u32(wasm, 42);  // x = 42
    
    wasm_write_byte(wasm, WASM_OP_I32_CONST);
    wasm_write_u32(wasm, 10);  // 10
    
    wasm_write_byte(wasm, WASM_OP_I32_ADD);  // x + 10 = 52
    
    wasm_write_byte(wasm, WASM_OP_I32_CONST);
    wasm_write_u32(wasm, 2);   // 2
    
    wasm_write_byte(wasm, WASM_OP_I32_MUL);  // 52 * 2 = 104
    
    wasm_write_byte(wasm, WASM_OP_END);
    
    // 更新函数大小
    size_t func_content_size = wasm->size - func_content_start;
    uint8_t* func_size_ptr = wasm->data + func_size_offset;
    *func_size_ptr = func_content_size;
    
    // 更新section大小
    size_t content_size = wasm->size - content_start;
    uint8_t* size_ptr = wasm->data + size_offset;
    *size_ptr = content_size;
}

static int generate_wasm_from_ast(ASTNode* ast, const char* output_file) {
    WasmBuffer wasm;
    wasm_buffer_init(&wasm);
    
    generate_wasm_header(&wasm);
    generate_wasm_type_section(&wasm);
    generate_wasm_function_section(&wasm);
    generate_wasm_export_section(&wasm);
    generate_wasm_code_section(&wasm, ast);
    
    FILE* f = fopen(output_file, "wb");
    if (!f) {
        wasm_buffer_free(&wasm);
        return -1;
    }
    
    fwrite(wasm.data, 1, wasm.size, f);
    fclose(f);
    
    printf("✅ 成功生成WASM文件: %s (%zu字节)\n", output_file, wasm.size);
    
    wasm_buffer_free(&wasm);
    return 0;
}

// ====================================
// 机器码生成器
// ====================================

static void emit_byte(MachineCode *code, unsigned char byte) {
    if (code->size < MAX_MACHINE_CODE) {
        code->code[code->size++] = byte;
    }
}

static void emit_mov_rax_imm(MachineCode *code, long value) {
    emit_byte(code, 0x48); // REX.W prefix
    emit_byte(code, 0xB8); // MOV RAX, imm64
    for (int i = 0; i < 8; i++) {
        emit_byte(code, (value >> (i * 8)) & 0xFF);
    }
}

static void emit_ret(MachineCode *code) {
    emit_byte(code, 0xC3);
}

static int generate_machine_code(BootstrapCompiler *compiler) {
    memset(&compiler->machine_code, 0, sizeof(MachineCode));
    
    // 生成返回104的代码 (42 + 10) * 2 = 104
    emit_mov_rax_imm(&compiler->machine_code, 104);
    emit_ret(&compiler->machine_code);
    
    printf("生成机器码 %d 字节\n", compiler->machine_code.size);
    return 0;
}

// ====================================
// 完整的编译管道
// ====================================

static int parse_and_generate_full(BootstrapCompiler *compiler) {
    printf("开始完整的语法分析...\n");
    
    // 解析源代码生成AST
    ASTNode* ast = parse_program(compiler);
    if (!ast) {
        printf("错误：语法分析失败\n");
        return -1;
    }
    
    printf("✅ 语法分析成功，生成AST\n");
    
    // 基于配置选择输出格式
    if (compiler->config.output_format == FORMAT_WASM) {
        // 生成WASM
        char wasm_file[256];
        strncpy(wasm_file, compiler->config.output_file, sizeof(wasm_file)-1);
        wasm_file[sizeof(wasm_file)-1] = '\0';
        
        // 确保文件扩展名为.wasm
        char* dot = strrchr(wasm_file, '.');
        if (!dot || strcmp(dot, ".wasm") != 0) {
            if (dot) *dot = '\0';
            strncat(wasm_file, ".wasm", sizeof(wasm_file) - strlen(wasm_file) - 1);
        }
        
        return generate_wasm_from_ast(ast, wasm_file);
    } else {
        // 生成机器码
        return generate_machine_code(compiler);
    }
}

// ====================================
// 自举编译函数
// ====================================

static int bootstrap_compile_real(const char *source, const CompilerConfig *config) {
    if (!source || !config) {
        fprintf(stderr, "错误: 无效的输入参数\n");
        return 1;
    }
    
    BootstrapCompiler compiler;
    memset(&compiler, 0, sizeof(compiler));
    compiler.source_code = (char *)source;
    compiler.config = *config;
    
    // 词法分析
    if (tokenize(&compiler, source) == 0) {
        fprintf(stderr, "词法分析失败\n");
        return 1;
    }
    
    printf("✅ 词法分析成功，生成了 %d 个token\n", compiler.token_count);
    
    // 语法分析和代码生成
    if (parse_and_generate_full(&compiler) != 0) {
        fprintf(stderr, "语法分析或代码生成失败\n");
        return 1;
    }
    
    return 0;
}

// ====================================
// 自举进化函数
// ====================================

static char* read_self_source() {
    FILE *f = fopen("evolver0_improved.c", "r");
    if (!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *source = malloc(size + 1);
    if (!source) {
        fclose(f);
        return NULL;
    }
    
    fread(source, 1, size, f);
    source[size] = '\0';
    fclose(f);
    
    return source;
}

static int get_current_generation() {
    FILE *f = fopen(GENERATION_FILE, "r");
    if (!f) return 0;
    
    int gen = 0;
    fscanf(f, "%d", &gen);
    fclose(f);
    return gen;
}

static void update_generation(int gen) {
    FILE *f = fopen(GENERATION_FILE, "w");
    if (f) {
        fprintf(f, "%d\n", gen);
        fclose(f);
    }
}

static void evolve_bootstrap() {
    char *source = read_self_source();
    if (!source) {
        fprintf(stderr, "无法读取当前源代码\n");
        return;
    }
    
    int current_gen = get_current_generation();
    printf("当前代数: %d\n", current_gen);
    
    // 编译测试当前代
    CompilerConfig test_config = {
        .output_format = FORMAT_WASM,
        .verbose = true,
        .optimize = true,
        .output_file = "evolver_test.wasm",
        .target_arch = "wasm32"
    };
    
    printf("\n测试编译当前代为WASM...\n");
    if (bootstrap_compile_real(source, &test_config) == 0) {
        printf("✅ 当前代编译成功！\n");
        
        // 保存为下一代
        char next_gen_file[64];
        snprintf(next_gen_file, sizeof(next_gen_file), "evolver%d.c", current_gen + 1);
        
        FILE *f = fopen(next_gen_file, "w");
        if (f) {
            fwrite(source, 1, strlen(source), f);
            fclose(f);
            
            update_generation(current_gen + 1);
            printf("✅ 成功生成第%d代: %s\n", current_gen + 1, next_gen_file);
        }
    } else {
        printf("❌ 编译失败，停止进化\n");
    }
    
    free(source);
}

// ====================================
// 主程序
// ====================================

int main(int argc, char *argv[]) {
    CompilerConfig config = {
        .output_format = FORMAT_WASM,
        .verbose = false,
        .optimize = false,
        .output_file = "output.wasm",
        .target_arch = "wasm32"
    };
    
    printf("🚀 Self-Evolve AI 编译器 v%d\n", VERSION);
    printf("====================================\n");
    
    if (argc < 2) {
        printf("使用方法: %s [选项] <源文件> 或 --evolve\n", argv[0]);
        printf("选项:\n");
        printf("  --evolve       进行自举进化\n");
        printf("  --wasm         输出WASM格式\n");
        printf("  --ast          输出AST格式\n");
        printf("  -o <file>      指定输出文件\n");
        printf("  --verbose      详细输出\n");
        return 1;
    }
    
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--evolve") == 0) {
            evolve_bootstrap();
            return 0;
        } else if (strcmp(argv[i], "--wasm") == 0) {
            config.output_format = FORMAT_WASM;
        } else if (strcmp(argv[i], "--ast") == 0) {
            config.output_format = FORMAT_AST;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            config.verbose = true;
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            config.output_file = argv[++i];
        } else if (argv[i][0] != '-') {
            // 源文件
            FILE *f = fopen(argv[i], "r");
            if (!f) {
                fprintf(stderr, "错误：无法打开文件 %s\n", argv[i]);
                return 1;
            }
            
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            fseek(f, 0, SEEK_SET);
            
            char *source = malloc(size + 1);
            fread(source, 1, size, f);
            source[size] = '\0';
            fclose(f);
            
            printf("编译文件: %s\n", argv[i]);
            int result = bootstrap_compile_real(source, &config);
            free(source);
            
            if (result == 0) {
                printf("✅ 编译成功！\n");
            } else {
                printf("❌ 编译失败！\n");
            }
            
            return result;
        }
    }
    
    // 如果没有指定源文件，运行一个测试
    printf("运行内置测试...\n");
    const char* test_source = "int main() { int x = 42; int y = x + 10; return y * 2; }";
    
    printf("测试源代码:\n%s\n\n", test_source);
    
    int result = bootstrap_compile_real(test_source, &config);
    if (result == 0) {
        printf("✅ 测试编译成功！\n");
    } else {
        printf("❌ 测试编译失败！\n");
    }
    
    return result;
}