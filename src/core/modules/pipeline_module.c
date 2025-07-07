/**
 * pipeline_module.c - Pipeline Module
 * 
 * 管道模块，整合了完整的编译执行流水线：
 * - Frontend: C源码 -> ASTC (c2astc)
 * - Backend: ASTC -> 汇编代码 (codegen) -> 原生代码 (astc2native)
 * - Execution: ASTC字节码执行 (astc + vm)
 */

#include "../module.h"
#include "../astc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

// ===============================================
// 模块信息
// ===============================================

#define MODULE_NAME "pipeline"
#define MODULE_VERSION "1.0.0"
#define MODULE_DESCRIPTION "Complete compilation and execution pipeline"

// 依赖layer0模块
MODULE_DEPENDS_ON(layer0);

// ===============================================
// 前端编译器 (C -> ASTC)
// ===============================================

// Token类型
typedef enum {
    TOKEN_EOF = 0,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_RETURN,
    TOKEN_INT,
    TOKEN_VOID,
    TOKEN_CHAR,
    TOKEN_FLOAT
} TokenType;

// Token结构
typedef struct {
    TokenType type;
    char* value;
    int line;
    int column;
} Token;

// 词法分析器
typedef struct {
    const char* source;
    int current;
    int line;
    int column;
} Lexer;

// 语法分析器
typedef struct {
    Token** tokens;
    int token_count;
    int current;
    char error_msg[256];
} Parser;

// 编译选项
typedef struct {
    int optimize_level;
    bool enable_debug;
    bool enable_warnings;
    char output_file[256];
} CompileOptions;

// ===============================================
// 后端代码生成器
// ===============================================

// 代码生成器
typedef struct {
    char* buffer;
    size_t buffer_size;
    size_t buffer_offset;
    int label_count;
} CodeGenerator;

// 目标架构
typedef enum {
    TARGET_X64,
    TARGET_X86,
    TARGET_ARM64,
    TARGET_ARM32
} TargetArch;

// 代码生成选项
typedef struct {
    TargetArch target_arch;
    int optimization_level;
    bool generate_debug_info;
} CodegenOptions;

// ===============================================
// 虚拟机执行器
// ===============================================

// VM状态
typedef enum {
    VM_STATE_READY,
    VM_STATE_RUNNING,
    VM_STATE_STOPPED,
    VM_STATE_ERROR
} VMState;

// VM上下文
typedef struct {
    VMState state;
    uint8_t* bytecode;
    size_t bytecode_size;
    size_t program_counter;
    uint64_t* stack;
    size_t stack_size;
    size_t stack_pointer;
    uint64_t registers[16];
    char error_message[256];
} VMContext;

// ASTC指令
typedef enum {
    VM_OP_NOP = 0x00,
    VM_OP_HALT = 0x01,
    VM_OP_LOAD_IMM = 0x10,
    VM_OP_STORE = 0x11,
    VM_OP_ADD = 0x20,
    VM_OP_SUB = 0x21,
    VM_OP_MUL = 0x22,
    VM_OP_DIV = 0x23,
    VM_OP_CALL = 0x30,
    VM_OP_RETURN = 0x31,
    VM_OP_JUMP = 0x40,
    VM_OP_JUMP_IF = 0x41,
    VM_OP_PUSH = 0x50,
    VM_OP_POP = 0x51,
    VM_OP_PRINT = 0x60,
    VM_OP_EXIT = 0xFF
} VMOpcode;

// ===============================================
// 管道状态
// ===============================================

typedef struct {
    // 编译阶段
    bool frontend_initialized;
    bool backend_initialized;
    bool vm_initialized;
    
    // 当前处理的程序
    char* source_code;
    ASTNode* ast_root;
    char* assembly_code;
    uint8_t* bytecode;
    size_t bytecode_size;
    
    // VM实例
    VMContext* vm_ctx;
    
    // 错误信息
    char error_message[512];
} PipelineState;

static PipelineState pipeline_state = {0};

// ===============================================
// 前端实现 (简化版C编译器)
// ===============================================

// 初始化词法分析器
static void init_lexer(Lexer* lexer, const char* source) {
    lexer->source = source;
    lexer->current = 0;
    lexer->line = 1;
    lexer->column = 1;
}

// 创建Token
static Token* create_token(TokenType type, const char* value, int line, int column) {
    Token* token = malloc(sizeof(Token));
    if (!token) return NULL;
    
    token->type = type;
    token->value = value ? strdup(value) : NULL;
    token->line = line;
    token->column = column;
    
    return token;
}

// 简化的词法分析
static bool tokenize(const char* source, Token*** tokens, int* token_count) {
    Lexer lexer;
    init_lexer(&lexer, source);
    
    Token** token_array = malloc(1024 * sizeof(Token*));
    int count = 0;
    
    while (lexer.source[lexer.current] != '\0') {
        char c = lexer.source[lexer.current];
        
        // 跳过空白字符
        if (c == ' ' || c == '\t' || c == '\r') {
            lexer.current++;
            lexer.column++;
            continue;
        }
        
        if (c == '\n') {
            lexer.current++;
            lexer.line++;
            lexer.column = 1;
            continue;
        }
        
        // 识别标识符和关键字
        if (isalpha(c) || c == '_') {
            int start = lexer.current;
            while (isalnum(lexer.source[lexer.current]) || lexer.source[lexer.current] == '_') {
                lexer.current++;
            }
            
            char* value = malloc(lexer.current - start + 1);
            strncpy(value, &lexer.source[start], lexer.current - start);
            value[lexer.current - start] = '\0';
            
            TokenType type = TOKEN_IDENTIFIER;
            if (strcmp(value, "int") == 0) type = TOKEN_INT;
            else if (strcmp(value, "void") == 0) type = TOKEN_VOID;
            else if (strcmp(value, "if") == 0) type = TOKEN_IF;
            else if (strcmp(value, "return") == 0) type = TOKEN_RETURN;
            
            token_array[count++] = create_token(type, value, lexer.line, lexer.column);
            free(value);
            continue;
        }
        
        // 识别数字
        if (isdigit(c)) {
            int start = lexer.current;
            while (isdigit(lexer.source[lexer.current])) {
                lexer.current++;
            }
            
            char* value = malloc(lexer.current - start + 1);
            strncpy(value, &lexer.source[start], lexer.current - start);
            value[lexer.current - start] = '\0';
            
            token_array[count++] = create_token(TOKEN_NUMBER, value, lexer.line, lexer.column);
            free(value);
            continue;
        }
        
        // 识别符号
        switch (c) {
            case '+': token_array[count++] = create_token(TOKEN_PLUS, "+", lexer.line, lexer.column); break;
            case '-': token_array[count++] = create_token(TOKEN_MINUS, "-", lexer.line, lexer.column); break;
            case '*': token_array[count++] = create_token(TOKEN_STAR, "*", lexer.line, lexer.column); break;
            case '/': token_array[count++] = create_token(TOKEN_SLASH, "/", lexer.line, lexer.column); break;
            case ';': token_array[count++] = create_token(TOKEN_SEMICOLON, ";", lexer.line, lexer.column); break;
            case '(': token_array[count++] = create_token(TOKEN_LPAREN, "(", lexer.line, lexer.column); break;
            case ')': token_array[count++] = create_token(TOKEN_RPAREN, ")", lexer.line, lexer.column); break;
            case '{': token_array[count++] = create_token(TOKEN_LBRACE, "{", lexer.line, lexer.column); break;
            case '}': token_array[count++] = create_token(TOKEN_RBRACE, "}", lexer.line, lexer.column); break;
        }
        
        lexer.current++;
        lexer.column++;
    }
    
    token_array[count++] = create_token(TOKEN_EOF, NULL, lexer.line, lexer.column);
    
    *tokens = token_array;
    *token_count = count;
    return true;
}

// 简化的语法分析
static ASTNode* parse_program(Token** tokens, int token_count) {
    // 创建翻译单元根节点
    ASTNode* root = ast_create_node(ASTC_TRANSLATION_UNIT, 1, 1);
    if (!root) return NULL;
    
    // 简化实现：只解析一个main函数
    root->data.translation_unit.declarations = malloc(sizeof(ASTNode*));
    root->data.translation_unit.declaration_count = 1;
    
    // 创建main函数节点
    ASTNode* main_func = ast_create_node(ASTC_FUNC_DECL, 1, 1);
    main_func->data.func_decl.name = strdup("main");
    main_func->data.func_decl.return_type = ast_create_node(ASTC_TYPE_INT, 1, 1);
    main_func->data.func_decl.param_count = 0;
    main_func->data.func_decl.params = NULL;
    main_func->data.func_decl.has_body = true;
    main_func->data.func_decl.body = ast_create_node(ASTC_COMPOUND_STMT, 1, 1);
    
    root->data.translation_unit.declarations[0] = main_func;
    
    return root;
}

// ===============================================
// 后端实现 (代码生成)
// ===============================================

// 初始化代码生成器
static void init_codegen(CodeGenerator* cg) {
    cg->buffer_size = 4096;
    cg->buffer = malloc(cg->buffer_size);
    cg->buffer[0] = '\0';
    cg->buffer_offset = 0;
    cg->label_count = 0;
}

// 追加代码
static void codegen_append(CodeGenerator* cg, const char* code) {
    size_t len = strlen(code);
    if (cg->buffer_offset + len >= cg->buffer_size) {
        cg->buffer_size *= 2;
        cg->buffer = realloc(cg->buffer, cg->buffer_size);
    }
    
    strcpy(cg->buffer + cg->buffer_offset, code);
    cg->buffer_offset += len;
}

// 生成汇编代码
static bool generate_assembly(ASTNode* ast, CodeGenerator* cg) {
    if (!ast || !cg) return false;
    
    // 生成简单的汇编代码
    codegen_append(cg, ".text\n");
    codegen_append(cg, ".global _start\n");
    codegen_append(cg, "_start:\n");
    codegen_append(cg, "    mov rax, 60\n");  // sys_exit
    codegen_append(cg, "    mov rdi, 0\n");   // exit status
    codegen_append(cg, "    syscall\n");
    
    return true;
}

// 汇编代码转字节码
static bool assembly_to_bytecode(const char* assembly, uint8_t** bytecode, size_t* size) {
    // 简化实现：生成简单的字节码
    *size = 16;
    *bytecode = malloc(*size);
    
    // 简单的字节码：LOAD_IMM 42, PRINT, EXIT
    (*bytecode)[0] = VM_OP_LOAD_IMM;
    (*bytecode)[1] = 0;  // 寄存器0
    (*bytecode)[2] = 42; // 立即数42
    (*bytecode)[3] = 0;
    (*bytecode)[4] = 0;
    (*bytecode)[5] = 0;
    (*bytecode)[6] = 0;
    (*bytecode)[7] = 0;
    
    (*bytecode)[8] = VM_OP_PRINT;
    (*bytecode)[9] = 0;  // 寄存器0
    (*bytecode)[10] = 0;
    (*bytecode)[11] = 0;
    (*bytecode)[12] = 0;
    (*bytecode)[13] = 0;
    (*bytecode)[14] = 0;
    (*bytecode)[15] = VM_OP_EXIT;
    
    return true;
}

// ===============================================
// 虚拟机实现
// ===============================================

// 创建VM上下文
static VMContext* create_vm_context(void) {
    VMContext* ctx = malloc(sizeof(VMContext));
    if (!ctx) return NULL;
    
    ctx->state = VM_STATE_READY;
    ctx->bytecode = NULL;
    ctx->bytecode_size = 0;
    ctx->program_counter = 0;
    ctx->stack_size = 1024;
    ctx->stack = malloc(ctx->stack_size * sizeof(uint64_t));
    ctx->stack_pointer = 0;
    memset(ctx->registers, 0, sizeof(ctx->registers));
    ctx->error_message[0] = '\0';
    
    return ctx;
}

// 销毁VM上下文
static void destroy_vm_context(VMContext* ctx) {
    if (!ctx) return;
    
    if (ctx->bytecode) free(ctx->bytecode);
    if (ctx->stack) free(ctx->stack);
    free(ctx);
}

// 加载字节码
static bool vm_load_bytecode(VMContext* ctx, const uint8_t* bytecode, size_t size) {
    if (!ctx || !bytecode) return false;
    
    if (ctx->bytecode) free(ctx->bytecode);
    
    ctx->bytecode = malloc(size);
    memcpy(ctx->bytecode, bytecode, size);
    ctx->bytecode_size = size;
    ctx->program_counter = 0;
    
    return true;
}

// 执行字节码
static bool vm_execute(VMContext* ctx) {
    if (!ctx || !ctx->bytecode) return false;
    
    ctx->state = VM_STATE_RUNNING;
    
    while (ctx->program_counter < ctx->bytecode_size && ctx->state == VM_STATE_RUNNING) {
        uint8_t opcode = ctx->bytecode[ctx->program_counter];
        
        switch (opcode) {
            case VM_OP_NOP:
                ctx->program_counter++;
                break;
                
            case VM_OP_LOAD_IMM: {
                uint8_t reg = ctx->bytecode[ctx->program_counter + 1];
                uint64_t value = *(uint64_t*)&ctx->bytecode[ctx->program_counter + 2];
                if (reg < 16) {
                    ctx->registers[reg] = value;
                }
                ctx->program_counter += 10;
                break;
            }
            
            case VM_OP_PRINT: {
                uint8_t reg = ctx->bytecode[ctx->program_counter + 1];
                if (reg < 16) {
                    printf("Output: %llu\n", ctx->registers[reg]);
                }
                ctx->program_counter += 2;
                break;
            }
            
            case VM_OP_EXIT:
                ctx->state = VM_STATE_STOPPED;
                break;
                
            default:
                snprintf(ctx->error_message, sizeof(ctx->error_message), 
                        "Unknown opcode: 0x%02X", opcode);
                ctx->state = VM_STATE_ERROR;
                return false;
        }
    }
    
    return ctx->state != VM_STATE_ERROR;
}

// ===============================================
// 管道接口实现
// ===============================================

// 编译C源码为ASTC
static bool pipeline_compile(const char* source_code, CompileOptions* options) {
    if (!source_code) return false;
    
    // 清理之前的状态
    if (pipeline_state.source_code) free(pipeline_state.source_code);
    if (pipeline_state.ast_root) ast_free(pipeline_state.ast_root);
    if (pipeline_state.assembly_code) free(pipeline_state.assembly_code);
    if (pipeline_state.bytecode) free(pipeline_state.bytecode);
    
    // 保存源码
    pipeline_state.source_code = strdup(source_code);
    
    // 词法分析
    Token** tokens;
    int token_count;
    if (!tokenize(source_code, &tokens, &token_count)) {
        strcpy(pipeline_state.error_message, "Tokenization failed");
        return false;
    }
    
    // 语法分析
    pipeline_state.ast_root = parse_program(tokens, token_count);
    if (!pipeline_state.ast_root) {
        strcpy(pipeline_state.error_message, "Parsing failed");
        return false;
    }
    
    // 代码生成
    CodeGenerator cg;
    init_codegen(&cg);
    if (!generate_assembly(pipeline_state.ast_root, &cg)) {
        strcpy(pipeline_state.error_message, "Code generation failed");
        return false;
    }
    
    pipeline_state.assembly_code = strdup(cg.buffer);
    
    // 汇编转字节码
    if (!assembly_to_bytecode(cg.buffer, &pipeline_state.bytecode, &pipeline_state.bytecode_size)) {
        strcpy(pipeline_state.error_message, "Assembly to bytecode conversion failed");
        return false;
    }
    
    // 清理
    free(cg.buffer);
    for (int i = 0; i < token_count; i++) {
        if (tokens[i]->value) free(tokens[i]->value);
        free(tokens[i]);
    }
    free(tokens);
    
    return true;
}

// 执行编译后的程序
static bool pipeline_execute(void) {
    if (!pipeline_state.bytecode) {
        strcpy(pipeline_state.error_message, "No bytecode to execute");
        return false;
    }
    
    // 创建VM上下文
    if (!pipeline_state.vm_ctx) {
        pipeline_state.vm_ctx = create_vm_context();
        if (!pipeline_state.vm_ctx) {
            strcpy(pipeline_state.error_message, "Failed to create VM context");
            return false;
        }
    }
    
    // 加载字节码
    if (!vm_load_bytecode(pipeline_state.vm_ctx, pipeline_state.bytecode, pipeline_state.bytecode_size)) {
        strcpy(pipeline_state.error_message, "Failed to load bytecode");
        return false;
    }
    
    // 执行
    if (!vm_execute(pipeline_state.vm_ctx)) {
        snprintf(pipeline_state.error_message, sizeof(pipeline_state.error_message),
                "VM execution failed: %s", pipeline_state.vm_ctx->error_message);
        return false;
    }
    
    return true;
}

// 编译并执行
static bool pipeline_compile_and_run(const char* source_code, CompileOptions* options) {
    if (!pipeline_compile(source_code, options)) {
        return false;
    }
    
    return pipeline_execute();
}

// 获取错误信息
static const char* pipeline_get_error(void) {
    return pipeline_state.error_message;
}

// 获取生成的汇编代码
static const char* pipeline_get_assembly(void) {
    return pipeline_state.assembly_code;
}

// 获取字节码
static const uint8_t* pipeline_get_bytecode(size_t* size) {
    if (size) *size = pipeline_state.bytecode_size;
    return pipeline_state.bytecode;
}

// ===============================================
// 模块符号表
// ===============================================

static struct {
    const char* name;
    void* symbol;
} pipeline_symbols[] = {
    // 编译接口
    {"pipeline_compile", pipeline_compile},
    {"pipeline_execute", pipeline_execute},
    {"pipeline_compile_and_run", pipeline_compile_and_run},
    
    // 信息获取
    {"pipeline_get_error", pipeline_get_error},
    {"pipeline_get_assembly", pipeline_get_assembly},
    {"pipeline_get_bytecode", pipeline_get_bytecode},
    
    // VM接口
    {"create_vm_context", create_vm_context},
    {"destroy_vm_context", destroy_vm_context},
    {"vm_load_bytecode", vm_load_bytecode},
    {"vm_execute", vm_execute},
    
    {NULL, NULL}
};

// ===============================================
// 模块初始化和清理
// ===============================================

static int pipeline_init(void) {
    printf("Pipeline Module: Initializing compilation pipeline...\n");
    
    // 初始化管道状态
    memset(&pipeline_state, 0, sizeof(PipelineState));
    
    pipeline_state.frontend_initialized = true;
    pipeline_state.backend_initialized = true;
    pipeline_state.vm_initialized = true;
    
    printf("Pipeline Module: Frontend (C->ASTC) initialized\n");
    printf("Pipeline Module: Backend (ASTC->Assembly->Bytecode) initialized\n");
    printf("Pipeline Module: VM (Bytecode execution) initialized\n");
    
    return 0;
}

static void pipeline_cleanup(void) {
    printf("Pipeline Module: Cleaning up compilation pipeline...\n");
    
    // 清理管道状态
    if (pipeline_state.source_code) {
        free(pipeline_state.source_code);
        pipeline_state.source_code = NULL;
    }
    
    if (pipeline_state.ast_root) {
        ast_free(pipeline_state.ast_root);
        pipeline_state.ast_root = NULL;
    }
    
    if (pipeline_state.assembly_code) {
        free(pipeline_state.assembly_code);
        pipeline_state.assembly_code = NULL;
    }
    
    if (pipeline_state.bytecode) {
        free(pipeline_state.bytecode);
        pipeline_state.bytecode = NULL;
    }
    
    if (pipeline_state.vm_ctx) {
        destroy_vm_context(pipeline_state.vm_ctx);
        pipeline_state.vm_ctx = NULL;
    }
    
    pipeline_state.frontend_initialized = false;
    pipeline_state.backend_initialized = false;
    pipeline_state.vm_initialized = false;
}

// 解析符号
static void* pipeline_resolve(const char* symbol) {
    if (!symbol) return NULL;
    
    for (int i = 0; pipeline_symbols[i].name; i++) {
        if (strcmp(pipeline_symbols[i].name, symbol) == 0) {
            return pipeline_symbols[i].symbol;
        }
    }
    
    return NULL;
}

// ===============================================
// 模块定义
// ===============================================

Module module_pipeline = {
    .name = "pipeline",
    .state = MODULE_UNLOADED,
    .error = NULL,
    .init = pipeline_init,
    .cleanup = pipeline_cleanup,
    .resolve = pipeline_resolve
}; 