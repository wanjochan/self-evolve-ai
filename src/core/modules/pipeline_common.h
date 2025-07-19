/**
 * pipeline_common.h - Pipeline Common Definitions
 * 
 * 为pipeline子模块提供共享的类型定义、接口和工具函数
 */

#ifndef PIPELINE_COMMON_H
#define PIPELINE_COMMON_H

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
// 编译选项和配置
// ===============================================

typedef struct {
    int optimize_level;
    bool enable_debug;
    bool enable_warnings;
    char output_file[256];
} CompileOptions;

// ===============================================
// Token系统
// ===============================================

typedef enum {
    TOKEN_EOF = 0,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_CHAR_LITERAL,

    // 运算符
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,
    TOKEN_ASSIGN,
    TOKEN_PLUS_ASSIGN,
    TOKEN_MINUS_ASSIGN,
    TOKEN_STAR_ASSIGN,
    TOKEN_SLASH_ASSIGN,
    TOKEN_PERCENT_ASSIGN,

    // 比较运算符
    TOKEN_EQ,
    TOKEN_NE,
    TOKEN_LT,
    TOKEN_LE,
    TOKEN_GT,
    TOKEN_GE,

    // 逻辑运算符
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,

    // 位运算符
    TOKEN_BITWISE_AND,
    TOKEN_BITWISE_OR,
    TOKEN_BITWISE_XOR,
    TOKEN_BITWISE_NOT,
    TOKEN_LSHIFT,
    TOKEN_RSHIFT,

    // 分隔符
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_ARROW,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_QUESTION,
    TOKEN_COLON,

    // 关键字
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_DO,
    TOKEN_SWITCH,
    TOKEN_CASE,
    TOKEN_DEFAULT,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_RETURN,
    TOKEN_GOTO,

    // 类型关键字
    TOKEN_VOID,
    TOKEN_CHAR,
    TOKEN_SHORT,
    TOKEN_INT,
    TOKEN_LONG,
    TOKEN_FLOAT,
    TOKEN_DOUBLE,
    TOKEN_SIGNED,
    TOKEN_UNSIGNED,
    TOKEN_STRUCT,
    TOKEN_UNION,
    TOKEN_ENUM,
    TOKEN_TYPEDEF,

    // 存储类说明符
    TOKEN_AUTO,
    TOKEN_REGISTER,
    TOKEN_STATIC,
    TOKEN_EXTERN,

    // 类型限定符
    TOKEN_CONST,
    TOKEN_VOLATILE,

    // C99关键字
    TOKEN_INLINE,
    TOKEN_RESTRICT,
    TOKEN_BOOL,
    TOKEN_COMPLEX,
    TOKEN_IMAGINARY,
    
    // setjmp/longjmp支持 (T1.1.1增强)
    TOKEN_SETJMP,
    TOKEN_LONGJMP,
    TOKEN_JMP_BUF
} TokenType;

typedef struct {
    TokenType type;
    char* value;
    int line;
    int column;
} Token;

// ===============================================
// 代码生成器
// ===============================================

typedef struct {
    char* buffer;
    size_t buffer_size;
    size_t buffer_offset;
    int label_count;
} CodeGenerator;

typedef enum {
    TARGET_X64,
    TARGET_X86,
    TARGET_ARM64,
    TARGET_ARM32,
    TARGET_RISCV64,
    TARGET_RISCV32
} TargetArch;

typedef struct {
    TargetArch target_arch;
    int optimization_level;
    bool generate_debug_info;
    bool enable_vectorization;
    bool enable_simd;
} CodegenOptions;

typedef struct {
    TargetArch target_arch;
    CodeGenerator* cg;
    CodegenOptions* options;
    char* register_names[32];
    int register_count;
    char* instruction_prefix;
    int word_size;
} MultiTargetCodegen;

// ===============================================
// 优化器
// ===============================================

typedef enum {
    OPT_LEVEL_NONE = 0,
    OPT_LEVEL_BASIC = 1,
    OPT_LEVEL_STANDARD = 2,
    OPT_LEVEL_AGGRESSIVE = 3
} OptimizationLevel;

typedef struct {
    OptimizationLevel level;
    bool enable_constant_folding;
    bool enable_dead_code_elimination;
    bool enable_register_allocation;
    bool enable_basic_block_optimization;
    int optimization_passes;
    char* optimization_log;
    size_t log_size;
} OptimizerContext;

// ===============================================
// 虚拟机
// ===============================================

typedef enum {
    VM_STATE_READY,
    VM_STATE_RUNNING,
    VM_STATE_STOPPED,
    VM_STATE_ERROR
} VMState;

typedef struct {
    VMState state;
    ASTCBytecodeProgram* astc_program;
    uint8_t* bytecode;
    size_t bytecode_size;
    size_t program_counter;
    uint64_t* stack;
    size_t stack_size;
    size_t stack_pointer;
    uint64_t registers[16];
    char error_message[256];
} VMContext;

// ===============================================
// JIT编译器
// ===============================================

typedef enum {
    JIT_STATE_UNINITIALIZED,
    JIT_STATE_READY,
    JIT_STATE_COMPILING,
    JIT_STATE_EXECUTING,
    JIT_STATE_ERROR
} JITState;

typedef struct JITCacheEntry {
    uint64_t hash;
    void* machine_code;
    size_t code_size;
    uint64_t access_count;
    uint64_t last_access_time;
    struct JITCacheEntry* next;
} JITCacheEntry;

typedef struct {
    JITState state;
    TargetArch target_arch;
    void* code_buffer;
    size_t buffer_size;
    size_t buffer_used;
    JITCacheEntry* cache_head;
    uint32_t cache_size;
    uint32_t max_cache_size;
    bool enable_optimization;
    bool enable_profiling;
    uint64_t total_compilations;
    uint64_t cache_hits;
    uint64_t cache_misses;
} JITContext;

// ===============================================
// 错误处理
// ===============================================

typedef enum {
    PIPELINE_SUCCESS = 0,
    PIPELINE_ERROR_INVALID_INPUT = -1,
    PIPELINE_ERROR_PARSE_FAILED = -2,
    PIPELINE_ERROR_CODEGEN_FAILED = -3,
    PIPELINE_ERROR_OPTIMIZATION_FAILED = -4,
    PIPELINE_ERROR_VM_FAILED = -5,
    PIPELINE_ERROR_JIT_FAILED = -6,
    PIPELINE_ERROR_MEMORY_ALLOC = -7
} PipelineResult;

// ===============================================
// 工具函数声明
// ===============================================

// Token管理
Token* create_token(TokenType type, const char* value, int line, int column);
void free_token(Token* token);
void free_token_array(Token** tokens, int count);

// 代码生成器工具
void init_codegen(CodeGenerator* cg);
void free_codegen(CodeGenerator* cg);
void codegen_append(CodeGenerator* cg, const char* code);
void codegen_append_format(CodeGenerator* cg, const char* format, ...);

// 错误处理工具
void set_pipeline_error(char* error_buffer, size_t buffer_size, const char* format, ...);

// AST工具函数
bool is_constant_expression(const ASTNode* expr);
bool has_side_effects(const ASTNode* node);
ASTNode* copy_ast_node(const ASTNode* node);

// ===============================================
// 模块接口声明
// ===============================================

// 前端接口 (词法分析和语法分析)
bool frontend_tokenize(const char* source, Token*** tokens, int* token_count);
ASTNode* frontend_parse(Token** tokens, int token_count);
void frontend_free_tokens(Token** tokens, int token_count);
ASTNode* frontend_compile(const char* source);

// 后端接口 (代码生成)
bool backend_generate_assembly(ASTNode* ast, char** assembly_code);
bool backend_generate_bytecode(ASTNode* ast, uint8_t** bytecode, size_t* size);
bool backend_generate_multi_target(ASTNode* ast, TargetArch target, char** assembly_code);

// 优化器接口
OptimizerContext* optimizer_create_context(OptimizationLevel level);
void optimizer_free_context(OptimizerContext* ctx);
bool optimizer_optimize_ast(ASTNode* ast, OptimizerContext* ctx);

// 虚拟机接口
VMContext* vm_create_context(void);
void vm_free_context(VMContext* ctx);
bool vm_load_program(VMContext* ctx, ASTCBytecodeProgram* program);
bool vm_execute(VMContext* ctx);

// JIT编译器接口
JITContext* jit_create_context(TargetArch target_arch);
void jit_free_context(JITContext* ctx);
bool jit_compile_and_execute(JITContext* ctx, ASTCBytecodeProgram* program);

// ===============================================
// 工具函数实现将在各自模块中提供
// ===============================================

// ===============================================
// 常用宏定义
// ===============================================

#define PIPELINE_MAX_ERROR_MSG 512
#define PIPELINE_MAX_TOKENS 10000
#define PIPELINE_MAX_AST_DEPTH 100

// 错误检查宏
#define PIPELINE_CHECK_NULL(ptr, error_msg) \
    do { \
        if (!(ptr)) { \
            fprintf(stderr, "Pipeline error: %s\n", error_msg); \
            return NULL; \
        } \
    } while(0)

#define PIPELINE_CHECK_RESULT(result, error_msg) \
    do { \
        if (!(result)) { \
            fprintf(stderr, "Pipeline error: %s\n", error_msg); \
            return false; \
        } \
    } while(0)

// 内存分配检查宏
#define PIPELINE_MALLOC_CHECK(ptr, size) \
    do { \
        (ptr) = malloc(size); \
        if (!(ptr)) { \
            fprintf(stderr, "Pipeline error: malloc failed for %zu bytes\n", (size_t)(size)); \
            return NULL; \
        } \
    } while(0)

#endif // PIPELINE_COMMON_H