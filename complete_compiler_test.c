#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdint.h>

// ================================
// 复用语法分析器代码
// ================================

// Token类型
typedef enum {
    TOKEN_EOF = 0,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_INT,
    TOKEN_RETURN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_SEMICOLON,
    TOKEN_ASSIGN,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
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

// 编译器结构
typedef struct {
    Token *tokens;
    int token_count;
    int current_token;
    const char *source;
} Compiler;

#define MAX_TOKENS 1000

// ================================
// 词法分析器
// ================================

static bool is_keyword(const char *str) {
    return strcmp(str, "int") == 0 || strcmp(str, "return") == 0;
}

static TokenType get_keyword_type(const char *str) {
    if (strcmp(str, "int") == 0) return TOKEN_INT;
    if (strcmp(str, "return") == 0) return TOKEN_RETURN;
    return TOKEN_IDENTIFIER;
}

static int tokenize(Compiler *compiler) {
    const char *p = compiler->source;
    int token_count = 0;
    int line = 1;
    
    compiler->tokens = malloc(MAX_TOKENS * sizeof(Token));
    
    while (*p && token_count < MAX_TOKENS - 1) {
        // 跳过空白字符
        while (isspace(*p)) {
            if (*p == '\n') line++;
            p++;
        }
        
        if (*p == '\0') break;
        
        Token *token = &compiler->tokens[token_count++];
        token->line = line;
        
        // 标识符和关键字
        if (isalpha(*p) || *p == '_') {
            const char *start = p;
            while (isalnum(*p) || *p == '_') p++;
            
            int len = p - start;
            token->value = malloc(len + 1);
            strncpy(token->value, start, len);
            token->value[len] = '\0';
            
            if (is_keyword(token->value)) {
                token->type = get_keyword_type(token->value);
            } else {
                token->type = TOKEN_IDENTIFIER;
            }
        }
        // 数字
        else if (isdigit(*p)) {
            const char *start = p;
            while (isdigit(*p)) p++;
            
            int len = p - start;
            token->value = malloc(len + 1);
            strncpy(token->value, start, len);
            token->value[len] = '\0';
            token->type = TOKEN_NUMBER;
        }
        // 单字符操作符
        else {
            token->value = malloc(2);
            token->value[0] = *p;
            token->value[1] = '\0';
            
            switch (*p) {
                case '{': token->type = TOKEN_LBRACE; break;
                case '}': token->type = TOKEN_RBRACE; break;
                case '(': token->type = TOKEN_LPAREN; break;
                case ')': token->type = TOKEN_RPAREN; break;
                case ';': token->type = TOKEN_SEMICOLON; break;
                case '=': token->type = TOKEN_ASSIGN; break;
                case '+': token->type = TOKEN_PLUS; break;
                case '-': token->type = TOKEN_MINUS; break;
                case '*': token->type = TOKEN_MULTIPLY; break;
                case '/': token->type = TOKEN_DIVIDE; break;
                default: token->type = TOKEN_ERROR; break;
            }
            p++;
        }
    }
    
    // EOF token
    compiler->tokens[token_count].type = TOKEN_EOF;
    compiler->tokens[token_count].value = NULL;
    
    compiler->token_count = token_count;
    return token_count;
}

// ================================
// 语法分析器
// ================================

static Token* current_token(Compiler *compiler) {
    if (compiler->current_token < compiler->token_count) {
        return &compiler->tokens[compiler->current_token];
    }
    return &compiler->tokens[compiler->token_count]; // EOF token
}

static void advance_token(Compiler *compiler) {
    if (compiler->current_token < compiler->token_count) {
        compiler->current_token++;
    }
}

static bool match_token(Compiler *compiler, TokenType type) {
    Token *token = current_token(compiler);
    return token->type == type;
}

static bool consume_token(Compiler *compiler, TokenType type) {
    if (match_token(compiler, type)) {
        advance_token(compiler);
        return true;
    }
    return false;
}

static ASTNode* create_ast_node(ASTNodeType type, const char *value) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = type;
    if (value) {
        node->value = strdup(value);
    }
    return node;
}

static ASTNode* parse_expression(Compiler *compiler);

static ASTNode* parse_primary(Compiler *compiler) {
    Token *token = current_token(compiler);
    
    if (token->type == TOKEN_NUMBER) {
        ASTNode *node = create_ast_node(AST_NUMBER, token->value);
        advance_token(compiler);
        return node;
    }
    
    if (token->type == TOKEN_IDENTIFIER) {
        ASTNode *node = create_ast_node(AST_IDENTIFIER, token->value);
        advance_token(compiler);
        return node;
    }
    
    if (token->type == TOKEN_LPAREN) {
        advance_token(compiler);
        ASTNode *node = parse_expression(compiler);
        consume_token(compiler, TOKEN_RPAREN);
        return node;
    }
    
    printf("Error: Unexpected token in primary expression: %s\n", token->value);
    return NULL;
}

static ASTNode* parse_expression(Compiler *compiler) {
    ASTNode *left = parse_primary(compiler);
    
    Token *op_token = current_token(compiler);
    if (op_token->type == TOKEN_PLUS || op_token->type == TOKEN_MINUS ||
        op_token->type == TOKEN_MULTIPLY || op_token->type == TOKEN_DIVIDE) {
        
        advance_token(compiler);
        ASTNode *right = parse_expression(compiler);
        
        ASTNode *binary_op = create_ast_node(AST_BINARY_OP, op_token->value);
        binary_op->left = left;
        binary_op->right = right;
        return binary_op;
    }
    
    return left;
}

static ASTNode* parse_statement(Compiler *compiler) {
    if (match_token(compiler, TOKEN_RETURN)) {
        advance_token(compiler);
        ASTNode *expr = parse_expression(compiler);
        consume_token(compiler, TOKEN_SEMICOLON);
        
        ASTNode *return_stmt = create_ast_node(AST_RETURN_STMT, "return");
        return_stmt->left = expr;
        return return_stmt;
    }
    
    // Variable declaration: int x = expr;
    if (match_token(compiler, TOKEN_INT)) {
        advance_token(compiler);
        Token *name_token = current_token(compiler);
        if (!consume_token(compiler, TOKEN_IDENTIFIER)) {
            printf("Error: Expected identifier after int\n");
            return NULL;
        }
        
        ASTNode *var_decl = create_ast_node(AST_VAR_DECL, name_token->value);
        
        if (consume_token(compiler, TOKEN_ASSIGN)) {
            var_decl->right = parse_expression(compiler);
        }
        
        consume_token(compiler, TOKEN_SEMICOLON);
        return var_decl;
    }
    
    printf("Error: Unknown statement\n");
    return NULL;
}

static ASTNode* parse_function(Compiler *compiler) {
    if (!consume_token(compiler, TOKEN_INT)) {
        printf("Error: Expected return type\n");
        return NULL;
    }
    
    Token *name_token = current_token(compiler);
    if (!consume_token(compiler, TOKEN_IDENTIFIER)) {
        printf("Error: Expected function name\n");
        return NULL;
    }
    
    if (!consume_token(compiler, TOKEN_LPAREN)) {
        printf("Error: Expected '(' after function name\n");
        return NULL;
    }
    
    if (!consume_token(compiler, TOKEN_RPAREN)) {
        printf("Error: Expected ')' - parameters not supported yet\n");
        return NULL;
    }
    
    if (!consume_token(compiler, TOKEN_LBRACE)) {
        printf("Error: Expected '{' to start function body\n");
        return NULL;
    }
    
    ASTNode *function = create_ast_node(AST_FUNCTION, name_token->value);
    ASTNode *statements = NULL;
    ASTNode *last_stmt = NULL;
    
    while (!match_token(compiler, TOKEN_RBRACE) && !match_token(compiler, TOKEN_EOF)) {
        ASTNode *stmt = parse_statement(compiler);
        if (stmt) {
            if (!statements) {
                statements = stmt;
                last_stmt = stmt;
            } else {
                last_stmt->next = stmt;
                last_stmt = stmt;
            }
        }
    }
    
    if (!consume_token(compiler, TOKEN_RBRACE)) {
        printf("Error: Expected '}' to end function body\n");
        return NULL;
    }
    
    function->left = statements;
    return function;
}

static ASTNode* parse_program(Compiler *compiler) {
    ASTNode *program = create_ast_node(AST_PROGRAM, "program");
    ASTNode *functions = NULL;
    ASTNode *last_func = NULL;
    
    compiler->current_token = 0;
    
    while (!match_token(compiler, TOKEN_EOF)) {
        ASTNode *func = parse_function(compiler);
        if (func) {
            if (!functions) {
                functions = func;
                last_func = func;
            } else {
                last_func->next = func;
                last_func = func;
            }
        } else {
            break;
        }
    }
    
    program->left = functions;
    return program;
}

// ================================
// WASM生成器
// ================================

// WASM字节码缓冲区
typedef struct {
    uint8_t *data;
    size_t size;
    size_t capacity;
} WasmBuffer;

// WASM操作码
#define WASM_OP_I32_CONST   0x41
#define WASM_OP_I32_ADD     0x6A
#define WASM_OP_I32_SUB     0x6B
#define WASM_OP_I32_MUL     0x6C
#define WASM_OP_I32_DIV_S   0x6D
#define WASM_OP_RETURN      0x0F
#define WASM_OP_LOCAL_GET   0x20
#define WASM_OP_LOCAL_SET   0x21
#define WASM_OP_END         0x0B

// 变量表
typedef struct {
    char *name;
    int index;
} Variable;

typedef struct {
    Variable vars[16];
    int count;
} VariableTable;

// 缓冲区操作
static void wasm_buffer_init(WasmBuffer *buffer) {
    buffer->data = malloc(1024);
    buffer->size = 0;
    buffer->capacity = 1024;
}

static void wasm_write_byte(WasmBuffer *buffer, uint8_t byte) {
    if (buffer->size >= buffer->capacity) {
        buffer->capacity *= 2;
        buffer->data = realloc(buffer->data, buffer->capacity);
    }
    buffer->data[buffer->size++] = byte;
}

static void wasm_write_leb128_u32(WasmBuffer *buffer, uint32_t value) {
    do {
        uint8_t byte = value & 0x7F;
        value >>= 7;
        if (value != 0) {
            byte |= 0x80;
        }
        wasm_write_byte(buffer, byte);
    } while (value != 0);
}

static void wasm_write_leb128_i32(WasmBuffer *buffer, int32_t value) {
    int more = 1;
    while (more) {
        uint8_t byte = value & 0x7F;
        value >>= 7;
        if ((value == 0 && (byte & 0x40) == 0) || 
            (value == -1 && (byte & 0x40) != 0)) {
            more = 0;
        } else {
            byte |= 0x80;
        }
        wasm_write_byte(buffer, byte);
    }
}

static int find_or_add_variable(VariableTable *table, const char *name) {
    // 查找现有变量
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->vars[i].name, name) == 0) {
            return table->vars[i].index;
        }
    }
    
    // 添加新变量
    if (table->count < 16) {
        table->vars[table->count].name = strdup(name);
        table->vars[table->count].index = table->count;
        return table->count++;
    }
    
    printf("Error: Too many variables\n");
    return -1;
}

// AST到WASM转换器
static void generate_wasm_expression(ASTNode *node, WasmBuffer *buffer, VariableTable *vars) {
    if (!node) return;
    
    switch (node->type) {
        case AST_NUMBER:
            wasm_write_byte(buffer, WASM_OP_I32_CONST);
            wasm_write_leb128_i32(buffer, atoi(node->value));
            break;
            
        case AST_IDENTIFIER: {
            int var_index = find_or_add_variable(vars, node->value);
            if (var_index >= 0) {
                wasm_write_byte(buffer, WASM_OP_LOCAL_GET);
                wasm_write_leb128_u32(buffer, var_index);
            }
            break;
        }
            
        case AST_BINARY_OP:
            generate_wasm_expression(node->left, buffer, vars);
            generate_wasm_expression(node->right, buffer, vars);
            
            if (strcmp(node->value, "+") == 0) {
                wasm_write_byte(buffer, WASM_OP_I32_ADD);
            } else if (strcmp(node->value, "-") == 0) {
                wasm_write_byte(buffer, WASM_OP_I32_SUB);
            } else if (strcmp(node->value, "*") == 0) {
                wasm_write_byte(buffer, WASM_OP_I32_MUL);
            } else if (strcmp(node->value, "/") == 0) {
                wasm_write_byte(buffer, WASM_OP_I32_DIV_S);
            }
            break;
            
        default:
            break;
    }
}

static void generate_wasm_statement(ASTNode *node, WasmBuffer *buffer, VariableTable *vars) {
    if (!node) return;
    
    switch (node->type) {
        case AST_VAR_DECL: {
            int var_index = find_or_add_variable(vars, node->value);
            
            if (node->right) {
                generate_wasm_expression(node->right, buffer, vars);
                wasm_write_byte(buffer, WASM_OP_LOCAL_SET);
                wasm_write_leb128_u32(buffer, var_index);
            }
            break;
        }
            
        case AST_RETURN_STMT:
            if (node->left) {
                generate_wasm_expression(node->left, buffer, vars);
            }
            wasm_write_byte(buffer, WASM_OP_RETURN);
            break;
            
        default:
            break;
    }
}

static void generate_wasm_function(ASTNode *node, WasmBuffer *buffer) {
    if (!node || node->type != AST_FUNCTION) return;
    
    VariableTable vars = {0};
    
    // 遍历函数中的所有语句
    ASTNode *stmt = node->left;
    while (stmt) {
        generate_wasm_statement(stmt, buffer, &vars);
        stmt = stmt->next;
    }
    
    // 函数结束标记
    wasm_write_byte(buffer, WASM_OP_END);
    
    // 清理变量表
    for (int i = 0; i < vars.count; i++) {
        free(vars.vars[i].name);
    }
}

static void generate_wasm_module(ASTNode *ast, const char *output_file) {
    if (!ast || ast->type != AST_PROGRAM) return;
    
    WasmBuffer buffer;
    wasm_buffer_init(&buffer);
    
    // WASM魔数和版本
    uint8_t header[] = {0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00};
    for (size_t i = 0; i < sizeof(header); i++) {
        wasm_write_byte(&buffer, header[i]);
    }
    
    // 类型段 (section 1)
    wasm_write_byte(&buffer, 0x01); // type section
    wasm_write_leb128_u32(&buffer, 0x05); // section size
    wasm_write_leb128_u32(&buffer, 0x01); // 1 type
    wasm_write_byte(&buffer, 0x60);       // func type
    wasm_write_leb128_u32(&buffer, 0x00); // 0 params
    wasm_write_leb128_u32(&buffer, 0x01); // 1 result
    wasm_write_byte(&buffer, 0x7F);       // i32
    
    // 函数段 (section 3)
    wasm_write_byte(&buffer, 0x03); // function section
    wasm_write_leb128_u32(&buffer, 0x02); // section size
    wasm_write_leb128_u32(&buffer, 0x01); // 1 function
    wasm_write_leb128_u32(&buffer, 0x00); // type index 0
    
    // 导出段 (section 7)
    wasm_write_byte(&buffer, 0x07); // export section
    wasm_write_leb128_u32(&buffer, 0x08); // section size
    wasm_write_leb128_u32(&buffer, 0x01); // 1 export
    wasm_write_leb128_u32(&buffer, 0x04); // name length
    wasm_write_byte(&buffer, 'm');
    wasm_write_byte(&buffer, 'a');
    wasm_write_byte(&buffer, 'i');
    wasm_write_byte(&buffer, 'n');
    wasm_write_byte(&buffer, 0x00);       // export kind (function)
    wasm_write_leb128_u32(&buffer, 0x00); // function index 0
    
    // 代码段 (section 10)
    wasm_write_byte(&buffer, 0x0A); // code section
    
    // 临时缓冲区用于函数体
    WasmBuffer func_buffer;
    wasm_buffer_init(&func_buffer);
    
    // 生成函数体
    ASTNode *func = ast->left;
    if (func && func->type == AST_FUNCTION) {
        // 局部变量声明（简化为2个i32变量）
        wasm_write_leb128_u32(&func_buffer, 0x01); // 1 local group
        wasm_write_leb128_u32(&func_buffer, 0x02); // 2 locals
        wasm_write_byte(&func_buffer, 0x7F);       // i32 type
        
        generate_wasm_function(func, &func_buffer);
    }
    
    // 写入代码段大小和内容
    wasm_write_leb128_u32(&buffer, func_buffer.size + 2); // section size
    wasm_write_leb128_u32(&buffer, 0x01);                  // 1 function
    wasm_write_leb128_u32(&buffer, func_buffer.size);     // function size
    
    // 复制函数体
    for (size_t i = 0; i < func_buffer.size; i++) {
        wasm_write_byte(&buffer, func_buffer.data[i]);
    }
    
    // 写入文件
    FILE *file = fopen(output_file, "wb");
    if (file) {
        fwrite(buffer.data, 1, buffer.size, file);
        fclose(file);
        printf("✓ WASM模块已生成: %s (%zu bytes)\n", output_file, buffer.size);
    } else {
        printf("✗ 无法写入文件: %s\n", output_file);
    }
    
    // 清理
    free(buffer.data);
    free(func_buffer.data);
}

// ================================
// 主程序
// ================================

int main() {
    printf("完整的C到WASM编译器测试\n");
    printf("=======================\n\n");
    
    const char *source = 
        "int main() {\n"
        "    int x = 42;\n"
        "    int y = x + 10;\n"
        "    return y * 2;\n"
        "}\n";
    
    printf("源代码:\n%s", source);
    printf("===================\n");
    
    Compiler compiler = {0};
    compiler.source = source;
    
    // 步骤1: 词法分析
    printf("步骤1: 词法分析...\n");
    int token_count = tokenize(&compiler);
    printf("✓ 生成 %d 个token\n\n", token_count);
    
    // 步骤2: 语法分析
    printf("步骤2: 语法分析...\n");
    ASTNode *ast = parse_program(&compiler);
    
    if (!ast) {
        printf("✗ 语法分析失败\n");
        return 1;
    }
    printf("✓ AST构建完成\n\n");
    
    // 步骤3: WASM代码生成
    printf("步骤3: WASM代码生成...\n");
    generate_wasm_module(ast, "compiled.wasm");
    printf("\n");
    
    printf("✅ 编译完成！生成的文件: compiled.wasm\n");
    printf("可以用以下命令验证:\n");
    printf("  node -e \"const fs=require('fs'); WebAssembly.instantiate(fs.readFileSync('compiled.wasm')).then(r=>console.log('Result:', r.instance.exports.main()))\"\n");
    
    return 0;
}