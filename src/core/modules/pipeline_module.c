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
#include <sys/stat.h>

// ===============================================
// 模块信息
// ===============================================

#define MODULE_NAME "pipeline"
#define MODULE_VERSION "1.0.0"
#define MODULE_DESCRIPTION "Complete compilation and execution pipeline"

// 依赖layer0模块 (通过动态加载)

// ===============================================
// ASTC字节码生成器实现
// ===============================================

// ASTC字节码生成器状态
static struct {
    ASTCBytecodeProgram* current_program;
    bool initialized;
} astc_generator = {0};

// 创建ASTC字节码程序
ASTCBytecodeProgram* astc_bytecode_create(void) {
    ASTCBytecodeProgram* program = malloc(sizeof(ASTCBytecodeProgram));
    if (!program) return NULL;

    memset(program, 0, sizeof(ASTCBytecodeProgram));

    // 设置文件头
    memcpy(program->magic, "ASTC", 4);
    program->version = 1;
    program->flags = 0;

    // 初始化指令数组
    program->instructions = malloc(1024 * sizeof(ASTCInstruction));
    program->instruction_count = 0;
    program->code_size = 1024;

    return program;
}

// 释放ASTC字节码程序
void astc_bytecode_free(ASTCBytecodeProgram* program) {
    if (!program) return;

    if (program->instructions) free(program->instructions);
    if (program->data) free(program->data);
    if (program->symbol_names) {
        for (uint32_t i = 0; i < program->symbol_count; i++) {
            if (program->symbol_names[i]) free(program->symbol_names[i]);
        }
        free(program->symbol_names);
    }
    if (program->symbol_addresses) free(program->symbol_addresses);

    free(program);
}

// 添加ASTC指令
int astc_bytecode_add_instruction(ASTCBytecodeProgram* program, ASTNodeType opcode, int64_t operand) {
    if (!program || !program->instructions) return -1;

    // 检查是否需要扩展指令数组
    if (program->instruction_count >= program->code_size) {
        program->code_size *= 2;
        program->instructions = realloc(program->instructions,
                                      program->code_size * sizeof(ASTCInstruction));
        if (!program->instructions) return -1;
    }

    // 添加指令
    ASTCInstruction* instr = &program->instructions[program->instruction_count];
    instr->opcode = opcode;
    instr->operand.i64 = operand;

    program->instruction_count++;
    return 0;
}

// 从AST生成ASTC字节码
static int generate_astc_bytecode_from_ast(ASTNode* ast, ASTCBytecodeProgram* program) {
    if (!ast || !program) return -1;

    switch (ast->type) {
        case ASTC_TRANSLATION_UNIT:
            // 遍历所有声明
            for (int i = 0; i < ast->data.translation_unit.declaration_count; i++) {
                if (generate_astc_bytecode_from_ast(ast->data.translation_unit.declarations[i], program) != 0) {
                    return -1;
                }
            }
            break;

        case ASTC_FUNC_DECL:
            // 生成函数代码
            if (ast->data.func_decl.has_body && ast->data.func_decl.body) {
                // 函数开始
                astc_bytecode_add_instruction(program, AST_BLOCK, 0);

                // 生成函数体
                if (generate_astc_bytecode_from_ast(ast->data.func_decl.body, program) != 0) {
                    return -1;
                }

                // 函数结束
                astc_bytecode_add_instruction(program, AST_END, 0);
            }
            break;

        case ASTC_COMPOUND_STMT:
            // 生成复合语句
            for (int i = 0; i < ast->data.compound_stmt.statement_count; i++) {
                if (generate_astc_bytecode_from_ast(ast->data.compound_stmt.statements[i], program) != 0) {
                    return -1;
                }
            }
            break;

        case ASTC_RETURN_STMT:
            // 生成返回语句
            if (ast->data.return_stmt.value) {
                // 生成返回值表达式
                if (generate_astc_bytecode_from_ast(ast->data.return_stmt.value, program) != 0) {
                    return -1;
                }
            } else {
                // 返回0
                astc_bytecode_add_instruction(program, AST_I32_CONST, 0);
            }
            astc_bytecode_add_instruction(program, AST_RETURN, 0);
            break;

        case ASTC_EXPR_CONSTANT:
            // 生成常量
            if (ast->data.constant.type == ASTC_TYPE_INT) {
                astc_bytecode_add_instruction(program, AST_I32_CONST, ast->data.constant.int_val);
            }
            break;

        case ASTC_EXPR_IDENTIFIER:
            // 生成变量引用 (简化实现)
            astc_bytecode_add_instruction(program, AST_LOCAL_GET, 0);
            break;

        default:
            // 其他节点类型暂时跳过
            break;
    }

    return 0;
}

// ===============================================
// ASTC汇编生成器实现
// ===============================================

// 创建ASTC汇编程序
ASTCAssemblyProgram* astc_assembly_create(void) {
    ASTCAssemblyProgram* program = malloc(sizeof(ASTCAssemblyProgram));
    if (!program) return NULL;

    memset(program, 0, sizeof(ASTCAssemblyProgram));

    // 初始化汇编文本缓冲区
    program->text_capacity = 4096;
    program->assembly_text = malloc(program->text_capacity);
    if (!program->assembly_text) {
        free(program);
        return NULL;
    }

    program->assembly_text[0] = '\0';
    program->text_size = 0;

    return program;
}

// 释放ASTC汇编程序
void astc_assembly_free(ASTCAssemblyProgram* program) {
    if (!program) return;

    if (program->assembly_text) free(program->assembly_text);
    if (program->module_name) free(program->module_name);

    if (program->function_names) {
        for (uint32_t i = 0; i < program->function_count; i++) {
            if (program->function_names[i]) free(program->function_names[i]);
        }
        free(program->function_names);
    }

    if (program->function_addresses) free(program->function_addresses);

    free(program);
}

// 添加汇编行
int astc_assembly_add_line(ASTCAssemblyProgram* program, const char* line) {
    if (!program || !line) return -1;

    size_t line_len = strlen(line);
    size_t needed = program->text_size + line_len + 2; // +2 for \n and \0

    // 检查是否需要扩展缓冲区
    if (needed > program->text_capacity) {
        program->text_capacity = needed * 2;
        program->assembly_text = realloc(program->assembly_text, program->text_capacity);
        if (!program->assembly_text) return -1;
    }

    // 添加行
    strcat(program->assembly_text, line);
    strcat(program->assembly_text, "\n");
    program->text_size = strlen(program->assembly_text);

    return 0;
}

// 添加ASTC指令
int astc_assembly_add_instruction(ASTCAssemblyProgram* program, const char* mnemonic, const char* operands) {
    if (!program || !mnemonic) return -1;

    char instruction[256];
    if (operands && strlen(operands) > 0) {
        snprintf(instruction, sizeof(instruction), "    %s %s", mnemonic, operands);
    } else {
        snprintf(instruction, sizeof(instruction), "    %s", mnemonic);
    }

    return astc_assembly_add_line(program, instruction);
}

// 添加标签
int astc_assembly_add_label(ASTCAssemblyProgram* program, const char* label) {
    if (!program || !label) return -1;

    char label_line[256];
    snprintf(label_line, sizeof(label_line), "%s:", label);

    return astc_assembly_add_line(program, label_line);
}

// ASTC操作码到助记符的映射
static const char* astc_opcode_to_mnemonic(ASTNodeType opcode) {
    switch (opcode) {
        case AST_UNREACHABLE: return "unreachable";
        case AST_NOP: return "nop";
        case AST_BLOCK: return "block";
        case AST_LOOP: return "loop";
        case AST_IF: return "if";
        case AST_ELSE: return "else";
        case AST_END: return "end";
        case AST_BR: return "br";
        case AST_BR_IF: return "br_if";
        case AST_RETURN: return "return";
        case AST_CALL: return "call";

        case AST_LOCAL_GET: return "local.get";
        case AST_LOCAL_SET: return "local.set";
        case AST_LOCAL_TEE: return "local.tee";
        case AST_GLOBAL_GET: return "global.get";
        case AST_GLOBAL_SET: return "global.set";

        case AST_I32_LOAD: return "i32.load";
        case AST_I64_LOAD: return "i64.load";
        case AST_F32_LOAD: return "f32.load";
        case AST_F64_LOAD: return "f64.load";
        case AST_I32_STORE: return "i32.store";
        case AST_I64_STORE: return "i64.store";
        case AST_F32_STORE: return "f32.store";
        case AST_F64_STORE: return "f64.store";

        case AST_I32_CONST: return "i32.const";
        case AST_I64_CONST: return "i64.const";
        case AST_F32_CONST: return "f32.const";
        case AST_F64_CONST: return "f64.const";

        case AST_I32_EQZ: return "i32.eqz";
        case AST_I32_EQ: return "i32.eq";
        case AST_I32_NE: return "i32.ne";
        case AST_I32_LT_S: return "i32.lt_s";
        case AST_I32_LT_U: return "i32.lt_u";
        case AST_I32_GT_S: return "i32.gt_s";
        case AST_I32_GT_U: return "i32.gt_u";
        case AST_I32_LE_S: return "i32.le_s";
        case AST_I32_LE_U: return "i32.le_u";
        case AST_I32_GE_S: return "i32.ge_s";
        case AST_I32_GE_U: return "i32.ge_u";

        case AST_I32_ADD: return "i32.add";
        case AST_I32_SUB: return "i32.sub";
        case AST_I32_MUL: return "i32.mul";
        case AST_I32_DIV_S: return "i32.div_s";
        case AST_I32_DIV_U: return "i32.div_u";
        case AST_I32_REM_S: return "i32.rem_s";
        case AST_I32_REM_U: return "i32.rem_u";
        case AST_I32_AND: return "i32.and";
        case AST_I32_OR: return "i32.or";
        case AST_I32_XOR: return "i32.xor";
        case AST_I32_SHL: return "i32.shl";
        case AST_I32_SHR_S: return "i32.shr_s";
        case AST_I32_SHR_U: return "i32.shr_u";

        // C99扩展指令 - 使用ASTC特定的节点类型
        case ASTC_C99_COMPILE: return "c99.compile";
        case ASTC_C99_PARSE: return "c99.parse";
        case ASTC_C99_CODEGEN: return "c99.codegen";
        case ASTC_C99_OPTIMIZE: return "c99.optimize";
        case ASTC_C99_LINK: return "c99.link";

        // 调试指令 - 需要在ASTNodeType中添加对应项
        // case AST_DEBUG_PRINT: return "debug.print";
        // case AST_DEBUG_BREAK: return "debug.break";

        default: return "unknown";
    }
}

// ASTC字节码转换为汇编
ASTCAssemblyProgram* astc_bytecode_to_assembly(ASTCBytecodeProgram* bytecode_program) {
    if (!bytecode_program) return NULL;

    ASTCAssemblyProgram* assembly = astc_assembly_create();
    if (!assembly) return NULL;

    // 添加汇编头部
    astc_assembly_add_line(assembly, ";; ASTC Assembly Generated from Bytecode");
    astc_assembly_add_line(assembly, ";; Magic: ASTC");

    char version_line[64];
    snprintf(version_line, sizeof(version_line), ";; Version: %u", bytecode_program->version);
    astc_assembly_add_line(assembly, version_line);

    astc_assembly_add_line(assembly, "");
    astc_assembly_add_line(assembly, "(module");

    // 添加函数定义
    astc_assembly_add_line(assembly, "  (func $main (result i32)");

    // 转换指令
    for (uint32_t i = 0; i < bytecode_program->instruction_count; i++) {
        ASTCInstruction* instr = &bytecode_program->instructions[i];
        const char* mnemonic = astc_opcode_to_mnemonic(instr->opcode);

        char operand_str[64] = "";

        // 根据操作码类型格式化操作数
        switch (instr->opcode) {
            case AST_I32_CONST:
            case AST_I64_CONST:
                snprintf(operand_str, sizeof(operand_str), "%lld", instr->operand.i64);
                break;
            case AST_F32_CONST:
                snprintf(operand_str, sizeof(operand_str), "%f", instr->operand.f32);
                break;
            case AST_F64_CONST:
                snprintf(operand_str, sizeof(operand_str), "%f", instr->operand.f64);
                break;
            case AST_LOCAL_GET:
            case AST_LOCAL_SET:
            case AST_GLOBAL_GET:
            case AST_GLOBAL_SET:
            case AST_BR:
            case AST_BR_IF:
            case AST_CALL:
                snprintf(operand_str, sizeof(operand_str), "%u", instr->operand.index);
                break;
            default:
                // 无操作数的指令
                break;
        }

        astc_assembly_add_instruction(assembly, mnemonic, operand_str);
    }

    // 结束函数和模块
    astc_assembly_add_line(assembly, "  )");
    astc_assembly_add_line(assembly, ")");

    return assembly;
}

// ===============================================
// AST函数引用 (使用 src/core/astc.c 中的实现)
// ===============================================

// 注意：ast_create_node, ast_free, ast_print 函数在 src/core/astc.c 中定义
// 这里不再重复定义，直接使用 astc.h 中的声明

// ===============================================
// ASTC核心函数实现
// ===============================================

// 序列化模块到二进制缓冲区
int ast_serialize_module(ASTNode* module, uint8_t** buffer, size_t* size) {
    if (!module || !buffer || !size) return -1;

    // 简化实现：创建基本的序列化格式
    size_t estimated_size = 1024;
    uint8_t* buf = malloc(estimated_size);
    if (!buf) return -1;

    size_t offset = 0;

    // 写入魔数 "ASTC"
    memcpy(buf + offset, "ASTC", 4);
    offset += 4;

    // 写入版本号
    uint32_t version = 1;
    memcpy(buf + offset, &version, 4);
    offset += 4;

    // 写入节点类型
    uint32_t node_type = (uint32_t)module->type;
    memcpy(buf + offset, &node_type, 4);
    offset += 4;

    *buffer = buf;
    *size = offset;
    return 0;
}

// 从二进制缓冲区反序列化模块
ASTNode* ast_deserialize_module(const uint8_t* buffer, size_t size) {
    if (!buffer || size < 12) return NULL;

    size_t offset = 0;

    // 验证魔数
    if (memcmp(buffer + offset, "ASTC", 4) != 0) return NULL;
    offset += 4;

    // 读取版本号
    uint32_t version;
    memcpy(&version, buffer + offset, 4);
    offset += 4;
    if (version != 1) return NULL;

    // 读取节点类型
    uint32_t node_type;
    memcpy(&node_type, buffer + offset, 4);
    offset += 4;

    return ast_create_node((ASTNodeType)node_type, 1, 1);
}

// 验证模块结构
int ast_validate_module(ASTNode* module) {
    if (!module) return -1;
    if (module->type != ASTC_MODULE_DECL) return -1;
    return 0;
}

// 验证导出声明
int ast_validate_export_declaration(ASTNode* export_decl) {
    if (!export_decl) return -1;
    if (export_decl->type != ASTC_EXPORT_DECL) return -1;
    return 0;
}

// 验证导入声明
int ast_validate_import_declaration(ASTNode* import_decl) {
    if (!import_decl) return -1;
    if (import_decl->type != ASTC_IMPORT_DECL) return -1;
    return 0;
}

// 从文件加载ASTC程序
ASTCProgram* astc_load_program(const char* astc_file) {
    if (!astc_file) return NULL;

    FILE* file = fopen(astc_file, "rb");
    if (!file) return NULL;

    ASTCProgram* program = malloc(sizeof(ASTCProgram));
    if (!program) {
        fclose(file);
        return NULL;
    }

    memset(program, 0, sizeof(ASTCProgram));
    strncpy(program->program_name, astc_file, sizeof(program->program_name) - 1);
    program->version = 1;

    fclose(file);
    return program;
}

// 释放ASTC程序
void astc_free_program(ASTCProgram* program) {
    if (!program) return;

    if (program->source_code) free(program->source_code);
    if (program->bytecode) free(program->bytecode);
    if (program->compiler_context) free(program->compiler_context);

    free(program);
}

// 验证ASTC程序
int astc_validate_program(const ASTCProgram* program) {
    if (!program) return -1;
    if (strlen(program->program_name) == 0) return -1;
    if (program->bytecode_size > 0 && !program->bytecode) return -1;
    return 0;
}

// 向模块添加声明
int ast_module_add_declaration(ASTNode* module, ASTNode* declaration) {
    if (!module || !declaration) return -1;
    if (module->type != ASTC_MODULE_DECL) return -1;

    // 简化实现：暂时不维护声明列表
    return 0;
}

// 向模块添加导出
int ast_module_add_export(ASTNode* module, ASTNode* export_decl) {
    if (!module || !export_decl) return -1;
    if (module->type != ASTC_MODULE_DECL) return -1;
    if (export_decl->type != ASTC_EXPORT_DECL) return -1;

    // 简化实现：暂时不维护导出列表
    return 0;
}

// 向模块添加导入
int ast_module_add_import(ASTNode* module, ASTNode* import_decl) {
    if (!module || !import_decl) return -1;
    if (module->type != ASTC_MODULE_DECL) return -1;
    if (import_decl->type != ASTC_IMPORT_DECL) return -1;

    // 简化实现：暂时不维护导入列表
    return 0;
}

// 解析符号引用
int ast_resolve_symbol_references(ASTNode* module) {
    if (!module) return -1;
    if (module->type != ASTC_MODULE_DECL) return -1;

    // 简化实现：暂时返回成功
    return 0;
}

// 注意：ast_print 函数在 src/core/astc.c 中定义，这里不再重复定义

// ===============================================
// 前端编译器 (C -> ASTC)
// ===============================================

// Token类型
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
    TOKEN_IMAGINARY
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

// 前向声明
static ASTNode* parse_declaration(Parser* parser);
static ASTNode* parse_function_declaration(Parser* parser);
static ASTNode* parse_variable_declaration(Parser* parser);
static ASTNode* parse_statement(Parser* parser);
static ASTNode* parse_compound_statement(Parser* parser);
static ASTNode* parse_expression(Parser* parser);
static ASTNode* parse_primary_expression(Parser* parser);
static ASTNode* parse_type_specifier(Parser* parser);
static bool match_token(Parser* parser, TokenType type);
static Token* consume_token(Parser* parser, TokenType type, const char* error_msg);
static Token* peek_token(Parser* parser);
static Token* advance_token(Parser* parser);
static ASTNode* parse_function_declaration_with_type(Parser* parser, ASTNode* return_type, Token* name_token);
static ASTNode* parse_variable_declaration_with_type(Parser* parser, ASTNode* type, Token* name_token);

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
    ASTCBytecodeProgram* astc_program;  // ASTC字节码程序
    uint8_t* bytecode;                  // 兼容性字段
    size_t bytecode_size;               // 兼容性字段
    size_t program_counter;
    uint64_t* stack;
    size_t stack_size;
    size_t stack_pointer;
    uint64_t registers[16];
    char error_message[256];
} VMContext;

// ASTC VM执行器 - 直接执行ASTC字节码

// ===============================================
// AOT编译器实现 (Backend - astc2native)
// ===============================================

// 优化级别
typedef enum {
    OPT_NONE = 0,
    OPT_BASIC = 1,
    OPT_STANDARD = 2,
    OPT_AGGRESSIVE = 3
} OptLevel;

// 编译状态
typedef enum {
    COMPILE_SUCCESS = 0,
    COMPILE_ERROR_INVALID_INPUT = -1,
    COMPILE_ERROR_UNSUPPORTED_ARCH = -2,
    COMPILE_ERROR_MEMORY_ALLOC = -3,
    COMPILE_ERROR_CODEGEN_FAILED = -4,
    COMPILE_ERROR_LINK_FAILED = -5
} CompileResult;

// AOT编译器上下文
typedef struct {
    TargetArch target_arch;
    OptLevel opt_level;
    char output_file[256];
    char error_message[256];
} AOTCompiler;

// 创建AOT编译器
static AOTCompiler* aot_create_compiler(TargetArch arch, OptLevel opt_level) {
    AOTCompiler* aot = malloc(sizeof(AOTCompiler));
    if (!aot) return NULL;

    aot->target_arch = arch;
    aot->opt_level = opt_level;
    aot->output_file[0] = '\0';
    aot->error_message[0] = '\0';

    return aot;
}

// 销毁AOT编译器
static void aot_destroy_compiler(AOTCompiler* aot) {
    if (!aot) return;
    free(aot);
}

// 生成机器码的辅助函数
static size_t generate_machine_code_from_bytecode(const uint8_t* bytecode, size_t bytecode_size,
                                                 uint8_t** machine_code) {
    // 分配机器码缓冲区
    size_t capacity = bytecode_size * 8; // 估算机器码大小
    uint8_t* code = malloc(capacity);
    size_t offset = 0;

    // 生成函数序言
    uint8_t prologue[] = {
        0x55,                    // push rbp
        0x48, 0x89, 0xe5,        // mov rbp, rsp
        0x48, 0x83, 0xec, 0x20   // sub rsp, 32
    };
    memcpy(code + offset, prologue, sizeof(prologue));
    offset += sizeof(prologue);

    // 遍历字节码并生成对应的机器码
    for (size_t i = 0; i < bytecode_size; ) {
        uint8_t opcode = bytecode[i];

        switch (opcode) {
            case 0x10: { // 临时替换VM_OP_LOAD_IMM
                if (i + 9 < bytecode_size) {
                    uint8_t reg = bytecode[i + 1];
                    int64_t value = *(int64_t*)&bytecode[i + 2];

                    // 生成 mov rax, immediate (简化为只支持rax寄存器)
                    if (reg == 0) {
                        uint8_t mov_instr[] = {0x48, 0xb8}; // mov rax, imm64
                        memcpy(code + offset, mov_instr, sizeof(mov_instr));
                        offset += sizeof(mov_instr);
                        *(int64_t*)(code + offset) = value;
                        offset += 8;
                    }
                    i += 10;
                } else {
                    i++;
                }
                break;
            }

            case 0x31: { // 临时替换VM_OP_RETURN
                // 生成函数结尾
                uint8_t epilogue[] = {
                    0x48, 0x89, 0xec,    // mov rsp, rbp
                    0x5d,                // pop rbp
                    0xc3                 // ret
                };
                memcpy(code + offset, epilogue, sizeof(epilogue));
                offset += sizeof(epilogue);
                i++;
                break;
            }

            case 0x01: { // 临时替换VM_OP_HALT
                // 生成 exit 系统调用
                uint8_t exit_code[] = {
                    0x48, 0xc7, 0xc0, 0x3c, 0x00, 0x00, 0x00,  // mov rax, 60 (sys_exit)
                    0x48, 0xc7, 0xc7, 0x00, 0x00, 0x00, 0x00,  // mov rdi, 0 (exit status)
                    0x0f, 0x05                                  // syscall
                };
                memcpy(code + offset, exit_code, sizeof(exit_code));
                offset += sizeof(exit_code);
                i++;
                break;
            }

            default:
                i++;
                break;
        }
    }

    *machine_code = code;
    return offset;
}

// 生成ELF可执行文件
static CompileResult generate_elf_executable(const uint8_t* machine_code, size_t code_size,
                                           const char* output_file) {
    FILE* file = fopen(output_file, "wb");
    if (!file) {
        return COMPILE_ERROR_CODEGEN_FAILED;
    }

    // ELF头
    uint8_t elf_header[64] = {0};

    // ELF magic number
    elf_header[0] = 0x7f;
    elf_header[1] = 'E';
    elf_header[2] = 'L';
    elf_header[3] = 'F';

    // 64-bit
    elf_header[4] = 2;
    // Little endian
    elf_header[5] = 1;
    // ELF version
    elf_header[6] = 1;
    // System V ABI
    elf_header[7] = 0;

    // e_type: ET_EXEC (executable file)
    *(uint16_t*)(elf_header + 16) = 2;
    // e_machine: EM_X86_64
    *(uint16_t*)(elf_header + 18) = 0x3e;
    // e_version
    *(uint32_t*)(elf_header + 20) = 1;
    // e_entry: entry point address
    *(uint64_t*)(elf_header + 24) = 0x401000;
    // e_phoff: program header offset
    *(uint64_t*)(elf_header + 32) = 64;
    // e_ehsize: ELF header size
    *(uint16_t*)(elf_header + 52) = 64;
    // e_phentsize: program header entry size
    *(uint16_t*)(elf_header + 54) = 56;
    // e_phnum: number of program header entries
    *(uint16_t*)(elf_header + 56) = 1;

    fwrite(elf_header, 64, 1, file);

    // Program header
    uint8_t program_header[56] = {0};

    // p_type: PT_LOAD
    *(uint32_t*)(program_header + 0) = 1;
    // p_flags: PF_X | PF_R (executable + readable)
    *(uint32_t*)(program_header + 4) = 5;
    // p_offset: offset in file
    *(uint64_t*)(program_header + 8) = 0x1000;
    // p_vaddr: virtual address
    *(uint64_t*)(program_header + 16) = 0x401000;
    // p_paddr: physical address
    *(uint64_t*)(program_header + 24) = 0x401000;
    // p_filesz: size in file
    *(uint64_t*)(program_header + 32) = code_size;
    // p_memsz: size in memory
    *(uint64_t*)(program_header + 40) = code_size;
    // p_align: alignment
    *(uint64_t*)(program_header + 48) = 0x1000;

    fwrite(program_header, 56, 1, file);

    // 填充到0x1000偏移
    size_t current_pos = 64 + 56;
    size_t padding_size = 0x1000 - current_pos;
    uint8_t* padding = calloc(padding_size, 1);
    fwrite(padding, padding_size, 1, file);
    free(padding);

    // 写入机器码
    fwrite(machine_code, code_size, 1, file);

    fclose(file);

    // 设置可执行权限
    chmod(output_file, 0755);

    return COMPILE_SUCCESS;
}

// AOT编译ASTC字节码为可执行文件
static CompileResult aot_compile_to_executable(AOTCompiler* aot, const uint8_t* bytecode,
                                             size_t bytecode_size, const char* output_file) {
    if (!aot || !bytecode || !output_file) {
        return COMPILE_ERROR_INVALID_INPUT;
    }

    strcpy(aot->output_file, output_file);

    // 生成机器码
    uint8_t* machine_code = NULL;
    size_t code_size = generate_machine_code_from_bytecode(bytecode, bytecode_size, &machine_code);

    if (!machine_code || code_size == 0) {
        strcpy(aot->error_message, "Failed to generate machine code");
        return COMPILE_ERROR_CODEGEN_FAILED;
    }

    // 生成ELF可执行文件
    CompileResult result = generate_elf_executable(machine_code, code_size, output_file);

    free(machine_code);

    if (result != COMPILE_SUCCESS) {
        strcpy(aot->error_message, "Failed to generate ELF executable");
        return result;
    }

    return COMPILE_SUCCESS;
}

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
    void* astc_program;              // ASTC字节码程序
    void* astc_assembly;             // ASTC汇编程序
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

        // 识别字符串字面量
        if (c == '"') {
            int start = lexer.current + 1;
            lexer.current++; // 跳过开始的引号

            while (lexer.current < strlen(lexer.source) && lexer.source[lexer.current] != '"') {
                if (lexer.source[lexer.current] == '\\') {
                    lexer.current++; // 跳过转义字符
                }
                lexer.current++;
            }

            if (lexer.current < strlen(lexer.source)) {
                char* value = malloc(lexer.current - start + 1);
                strncpy(value, &lexer.source[start], lexer.current - start);
                value[lexer.current - start] = '\0';

                token_array[count++] = create_token(TOKEN_STRING, value, lexer.line, lexer.column);
                free(value);
                lexer.current++; // 跳过结束的引号
            }
            continue;
        }

        // 识别字符字面量
        if (c == '\'') {
            int start = lexer.current + 1;
            lexer.current++; // 跳过开始的单引号

            while (lexer.current < strlen(lexer.source) && lexer.source[lexer.current] != '\'') {
                if (lexer.source[lexer.current] == '\\') {
                    lexer.current++; // 跳过转义字符
                }
                lexer.current++;
            }

            if (lexer.current < strlen(lexer.source)) {
                char* value = malloc(lexer.current - start + 1);
                strncpy(value, &lexer.source[start], lexer.current - start);
                value[lexer.current - start] = '\0';

                token_array[count++] = create_token(TOKEN_CHAR_LITERAL, value, lexer.line, lexer.column);
                free(value);
                lexer.current++; // 跳过结束的单引号
            }
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
            // 关键字识别
            if (strcmp(value, "if") == 0) type = TOKEN_IF;
            else if (strcmp(value, "else") == 0) type = TOKEN_ELSE;
            else if (strcmp(value, "while") == 0) type = TOKEN_WHILE;
            else if (strcmp(value, "for") == 0) type = TOKEN_FOR;
            else if (strcmp(value, "do") == 0) type = TOKEN_DO;
            else if (strcmp(value, "switch") == 0) type = TOKEN_SWITCH;
            else if (strcmp(value, "case") == 0) type = TOKEN_CASE;
            else if (strcmp(value, "default") == 0) type = TOKEN_DEFAULT;
            else if (strcmp(value, "break") == 0) type = TOKEN_BREAK;
            else if (strcmp(value, "continue") == 0) type = TOKEN_CONTINUE;
            else if (strcmp(value, "return") == 0) type = TOKEN_RETURN;
            else if (strcmp(value, "goto") == 0) type = TOKEN_GOTO;
            // 类型关键字
            else if (strcmp(value, "void") == 0) type = TOKEN_VOID;
            else if (strcmp(value, "char") == 0) type = TOKEN_CHAR;
            else if (strcmp(value, "short") == 0) type = TOKEN_SHORT;
            else if (strcmp(value, "int") == 0) type = TOKEN_INT;
            else if (strcmp(value, "long") == 0) type = TOKEN_LONG;
            else if (strcmp(value, "float") == 0) type = TOKEN_FLOAT;
            else if (strcmp(value, "double") == 0) type = TOKEN_DOUBLE;
            else if (strcmp(value, "signed") == 0) type = TOKEN_SIGNED;
            else if (strcmp(value, "unsigned") == 0) type = TOKEN_UNSIGNED;
            else if (strcmp(value, "struct") == 0) type = TOKEN_STRUCT;
            else if (strcmp(value, "union") == 0) type = TOKEN_UNION;
            else if (strcmp(value, "enum") == 0) type = TOKEN_ENUM;
            else if (strcmp(value, "typedef") == 0) type = TOKEN_TYPEDEF;
            // 存储类说明符
            else if (strcmp(value, "auto") == 0) type = TOKEN_AUTO;
            else if (strcmp(value, "register") == 0) type = TOKEN_REGISTER;
            else if (strcmp(value, "static") == 0) type = TOKEN_STATIC;
            else if (strcmp(value, "extern") == 0) type = TOKEN_EXTERN;
            // 类型限定符
            else if (strcmp(value, "const") == 0) type = TOKEN_CONST;
            else if (strcmp(value, "volatile") == 0) type = TOKEN_VOLATILE;
            // C99关键字
            else if (strcmp(value, "inline") == 0) type = TOKEN_INLINE;
            else if (strcmp(value, "restrict") == 0) type = TOKEN_RESTRICT;
            else if (strcmp(value, "_Bool") == 0) type = TOKEN_BOOL;
            else if (strcmp(value, "_Complex") == 0) type = TOKEN_COMPLEX;
            else if (strcmp(value, "_Imaginary") == 0) type = TOKEN_IMAGINARY;
            
            token_array[count++] = create_token(type, value, lexer.line, lexer.column);
            free(value);
            continue;
        }
        
        // 识别数字（包括整数和浮点数）
        if (isdigit(c) || (c == '.' && lexer.current + 1 < strlen(lexer.source) && isdigit(lexer.source[lexer.current + 1]))) {
            int start = lexer.current;
            bool has_dot = false;
            bool has_exp = false;

            // 处理十六进制数字
            if (c == '0' && lexer.current + 1 < strlen(lexer.source) &&
                (lexer.source[lexer.current + 1] == 'x' || lexer.source[lexer.current + 1] == 'X')) {
                lexer.current += 2; // 跳过 "0x"
                while (lexer.current < strlen(lexer.source) &&
                       (isdigit(lexer.source[lexer.current]) ||
                        (lexer.source[lexer.current] >= 'a' && lexer.source[lexer.current] <= 'f') ||
                        (lexer.source[lexer.current] >= 'A' && lexer.source[lexer.current] <= 'F'))) {
                    lexer.current++;
                }
            } else {
                // 处理十进制数字
                while (lexer.current < strlen(lexer.source)) {
                    char ch = lexer.source[lexer.current];

                    if (isdigit(ch)) {
                        lexer.current++;
                    } else if (ch == '.' && !has_dot && !has_exp) {
                        has_dot = true;
                        lexer.current++;
                    } else if ((ch == 'e' || ch == 'E') && !has_exp) {
                        has_exp = true;
                        lexer.current++;
                        // 处理指数符号
                        if (lexer.current < strlen(lexer.source) &&
                            (lexer.source[lexer.current] == '+' || lexer.source[lexer.current] == '-')) {
                            lexer.current++;
                        }
                    } else {
                        break;
                    }
                }
            }

            // 处理数字后缀 (L, U, F等)
            while (lexer.current < strlen(lexer.source)) {
                char ch = lexer.source[lexer.current];
                if (ch == 'L' || ch == 'l' || ch == 'U' || ch == 'u' || ch == 'F' || ch == 'f') {
                    lexer.current++;
                } else {
                    break;
                }
            }

            char* value = malloc(lexer.current - start + 1);
            strncpy(value, &lexer.source[start], lexer.current - start);
            value[lexer.current - start] = '\0';

            token_array[count++] = create_token(TOKEN_NUMBER, value, lexer.line, lexer.column);
            free(value);
            continue;
        }
        
        // 识别符号和运算符
        switch (c) {
            case '+':
                if (lexer.current + 1 < strlen(lexer.source) && lexer.source[lexer.current + 1] == '=') {
                    token_array[count++] = create_token(TOKEN_PLUS_ASSIGN, "+=", lexer.line, lexer.column);
                    lexer.current++;
                } else {
                    token_array[count++] = create_token(TOKEN_PLUS, "+", lexer.line, lexer.column);
                }
                break;
            case '-':
                if (lexer.current + 1 < strlen(lexer.source) && lexer.source[lexer.current + 1] == '=') {
                    token_array[count++] = create_token(TOKEN_MINUS_ASSIGN, "-=", lexer.line, lexer.column);
                    lexer.current++;
                } else if (lexer.current + 1 < strlen(lexer.source) && lexer.source[lexer.current + 1] == '>') {
                    token_array[count++] = create_token(TOKEN_ARROW, "->", lexer.line, lexer.column);
                    lexer.current++;
                } else {
                    token_array[count++] = create_token(TOKEN_MINUS, "-", lexer.line, lexer.column);
                }
                break;
            case '*':
                if (lexer.current + 1 < strlen(lexer.source) && lexer.source[lexer.current + 1] == '=') {
                    token_array[count++] = create_token(TOKEN_STAR_ASSIGN, "*=", lexer.line, lexer.column);
                    lexer.current++;
                } else {
                    token_array[count++] = create_token(TOKEN_STAR, "*", lexer.line, lexer.column);
                }
                break;
            case '/':
                if (lexer.current + 1 < strlen(lexer.source) && lexer.source[lexer.current + 1] == '=') {
                    token_array[count++] = create_token(TOKEN_SLASH_ASSIGN, "/=", lexer.line, lexer.column);
                    lexer.current++;
                } else {
                    token_array[count++] = create_token(TOKEN_SLASH, "/", lexer.line, lexer.column);
                }
                break;
            case '%':
                if (lexer.current + 1 < strlen(lexer.source) && lexer.source[lexer.current + 1] == '=') {
                    token_array[count++] = create_token(TOKEN_PERCENT_ASSIGN, "%=", lexer.line, lexer.column);
                    lexer.current++;
                } else {
                    token_array[count++] = create_token(TOKEN_PERCENT, "%", lexer.line, lexer.column);
                }
                break;
            case '=':
                if (lexer.current + 1 < strlen(lexer.source) && lexer.source[lexer.current + 1] == '=') {
                    token_array[count++] = create_token(TOKEN_EQ, "==", lexer.line, lexer.column);
                    lexer.current++;
                } else {
                    token_array[count++] = create_token(TOKEN_ASSIGN, "=", lexer.line, lexer.column);
                }
                break;
            case '!':
                if (lexer.current + 1 < strlen(lexer.source) && lexer.source[lexer.current + 1] == '=') {
                    token_array[count++] = create_token(TOKEN_NE, "!=", lexer.line, lexer.column);
                    lexer.current++;
                } else {
                    token_array[count++] = create_token(TOKEN_NOT, "!", lexer.line, lexer.column);
                }
                break;
            case '<':
                if (lexer.current + 1 < strlen(lexer.source) && lexer.source[lexer.current + 1] == '=') {
                    token_array[count++] = create_token(TOKEN_LE, "<=", lexer.line, lexer.column);
                    lexer.current++;
                } else if (lexer.current + 1 < strlen(lexer.source) && lexer.source[lexer.current + 1] == '<') {
                    token_array[count++] = create_token(TOKEN_LSHIFT, "<<", lexer.line, lexer.column);
                    lexer.current++;
                } else {
                    token_array[count++] = create_token(TOKEN_LT, "<", lexer.line, lexer.column);
                }
                break;
            case '>':
                if (lexer.current + 1 < strlen(lexer.source) && lexer.source[lexer.current + 1] == '=') {
                    token_array[count++] = create_token(TOKEN_GE, ">=", lexer.line, lexer.column);
                    lexer.current++;
                } else if (lexer.current + 1 < strlen(lexer.source) && lexer.source[lexer.current + 1] == '>') {
                    token_array[count++] = create_token(TOKEN_RSHIFT, ">>", lexer.line, lexer.column);
                    lexer.current++;
                } else {
                    token_array[count++] = create_token(TOKEN_GT, ">", lexer.line, lexer.column);
                }
                break;
            case '&':
                if (lexer.current + 1 < strlen(lexer.source) && lexer.source[lexer.current + 1] == '&') {
                    token_array[count++] = create_token(TOKEN_AND, "&&", lexer.line, lexer.column);
                    lexer.current++;
                } else {
                    token_array[count++] = create_token(TOKEN_BITWISE_AND, "&", lexer.line, lexer.column);
                }
                break;
            case '|':
                if (lexer.current + 1 < strlen(lexer.source) && lexer.source[lexer.current + 1] == '|') {
                    token_array[count++] = create_token(TOKEN_OR, "||", lexer.line, lexer.column);
                    lexer.current++;
                } else {
                    token_array[count++] = create_token(TOKEN_BITWISE_OR, "|", lexer.line, lexer.column);
                }
                break;
            case '^': token_array[count++] = create_token(TOKEN_BITWISE_XOR, "^", lexer.line, lexer.column); break;
            case '~': token_array[count++] = create_token(TOKEN_BITWISE_NOT, "~", lexer.line, lexer.column); break;
            case ';': token_array[count++] = create_token(TOKEN_SEMICOLON, ";", lexer.line, lexer.column); break;
            case ',': token_array[count++] = create_token(TOKEN_COMMA, ",", lexer.line, lexer.column); break;
            case '.': token_array[count++] = create_token(TOKEN_DOT, ".", lexer.line, lexer.column); break;
            case '(': token_array[count++] = create_token(TOKEN_LPAREN, "(", lexer.line, lexer.column); break;
            case ')': token_array[count++] = create_token(TOKEN_RPAREN, ")", lexer.line, lexer.column); break;
            case '{': token_array[count++] = create_token(TOKEN_LBRACE, "{", lexer.line, lexer.column); break;
            case '}': token_array[count++] = create_token(TOKEN_RBRACE, "}", lexer.line, lexer.column); break;
            case '[': token_array[count++] = create_token(TOKEN_LBRACKET, "[", lexer.line, lexer.column); break;
            case ']': token_array[count++] = create_token(TOKEN_RBRACKET, "]", lexer.line, lexer.column); break;
            case '?': token_array[count++] = create_token(TOKEN_QUESTION, "?", lexer.line, lexer.column); break;
            case ':': token_array[count++] = create_token(TOKEN_COLON, ":", lexer.line, lexer.column); break;
        }
        
        lexer.current++;
        lexer.column++;
    }
    
    token_array[count++] = create_token(TOKEN_EOF, NULL, lexer.line, lexer.column);
    
    *tokens = token_array;
    *token_count = count;
    return true;
}

// 解析器辅助函数
static bool match_token(Parser* parser, TokenType type) {
    if (parser->current >= parser->token_count) return false;
    return parser->tokens[parser->current]->type == type;
}

static Token* peek_token(Parser* parser) {
    if (parser->current >= parser->token_count) return NULL;
    return parser->tokens[parser->current];
}

static Token* advance_token(Parser* parser) {
    if (parser->current >= parser->token_count) return NULL;
    return parser->tokens[parser->current++];
}

static Token* consume_token(Parser* parser, TokenType type, const char* error_msg) {
    if (match_token(parser, type)) {
        return advance_token(parser);
    }

    snprintf(parser->error_msg, sizeof(parser->error_msg), "%s", error_msg);
    return NULL;
}

// 解析类型说明符
static ASTNode* parse_type_specifier(Parser* parser) {
    Token* token = peek_token(parser);
    if (!token) return NULL;

    ASTNodeType type = ASTC_TYPE_INVALID;

    switch (token->type) {
        case TOKEN_VOID: type = ASTC_TYPE_VOID; break;
        case TOKEN_CHAR: type = ASTC_TYPE_CHAR; break;
        case TOKEN_SHORT: type = ASTC_TYPE_SHORT; break;
        case TOKEN_INT: type = ASTC_TYPE_INT; break;
        case TOKEN_LONG: type = ASTC_TYPE_LONG; break;
        case TOKEN_FLOAT: type = ASTC_TYPE_FLOAT; break;
        case TOKEN_DOUBLE: type = ASTC_TYPE_DOUBLE; break;
        case TOKEN_SIGNED: type = ASTC_TYPE_SIGNED; break;
        case TOKEN_UNSIGNED: type = ASTC_TYPE_UNSIGNED; break;
        default: return NULL;
    }

    advance_token(parser);
    ASTNode* node = ast_create_node(ASTC_TYPE_SPECIFIER, token->line, token->column);
    if (node) {
        node->data.type_specifier.type = type;
    }

    return node;
}

// 解析主表达式
static ASTNode* parse_primary_expression(Parser* parser) {
    Token* token = peek_token(parser);
    if (!token) return NULL;

    switch (token->type) {
        case TOKEN_IDENTIFIER: {
            advance_token(parser);
            ASTNode* node = ast_create_node(ASTC_EXPR_IDENTIFIER, token->line, token->column);
            if (node) {
                node->data.identifier.name = strdup(token->value);
            }
            return node;
        }

        case TOKEN_NUMBER: {
            advance_token(parser);
            ASTNode* node = ast_create_node(ASTC_EXPR_CONSTANT, token->line, token->column);
            if (node) {
                node->data.constant.type = ASTC_TYPE_INT;
                node->data.constant.int_val = atoi(token->value);
            }
            return node;
        }

        case TOKEN_STRING: {
            advance_token(parser);
            ASTNode* node = ast_create_node(ASTC_EXPR_STRING_LITERAL, token->line, token->column);
            if (node) {
                node->data.string_literal.value = strdup(token->value);
            }
            return node;
        }

        case TOKEN_LPAREN: {
            advance_token(parser); // consume '('
            ASTNode* expr = parse_expression(parser);
            consume_token(parser, TOKEN_RPAREN, "Expected ')' after expression");
            return expr;
        }

        default:
            return NULL;
    }
}

// 简化的表达式解析
static ASTNode* parse_expression(Parser* parser) {
    return parse_primary_expression(parser);
}

// 解析复合语句
static ASTNode* parse_compound_statement(Parser* parser) {
    if (!consume_token(parser, TOKEN_LBRACE, "Expected '{'")) {
        return NULL;
    }

    ASTNode* node = ast_create_node(ASTC_COMPOUND_STMT, 0, 0);
    if (!node) return NULL;

    // 简化实现：暂时不解析语句内容
    node->data.compound_stmt.statements = NULL;
    node->data.compound_stmt.statement_count = 0;

    consume_token(parser, TOKEN_RBRACE, "Expected '}'");
    return node;
}

// 解析语句
static ASTNode* parse_statement(Parser* parser) {
    Token* token = peek_token(parser);
    if (!token) return NULL;

    switch (token->type) {
        case TOKEN_LBRACE:
            return parse_compound_statement(parser);
        case TOKEN_RETURN: {
            advance_token(parser);
            ASTNode* node = ast_create_node(ASTC_RETURN_STMT, token->line, token->column);
            if (node) {
                if (!match_token(parser, TOKEN_SEMICOLON)) {
                    node->data.return_stmt.value = parse_expression(parser);
                } else {
                    node->data.return_stmt.value = NULL;
                }
                consume_token(parser, TOKEN_SEMICOLON, "Expected ';' after return statement");
            }
            return node;
        }
        default: {
            // 表达式语句
            ASTNode* expr = parse_expression(parser);
            if (expr) {
                consume_token(parser, TOKEN_SEMICOLON, "Expected ';' after expression");
                ASTNode* stmt = ast_create_node(ASTC_EXPR_STMT, 0, 0);
                if (stmt) {
                    stmt->data.expr_stmt.expr = expr;
                }
                return stmt;
            }
            return NULL;
        }
    }
}

// 改进的语法分析
static ASTNode* parse_program(Token** tokens, int token_count) {
    Parser parser = {
        .tokens = tokens,
        .token_count = token_count,
        .current = 0,
        .error_msg = {0}
    };
    // 创建翻译单元根节点
    ASTNode* root = ast_create_node(ASTC_TRANSLATION_UNIT, 1, 1);
    if (!root) return NULL;

    // 解析声明列表
    ASTNode** declarations = malloc(32 * sizeof(ASTNode*));
    int declaration_count = 0;

    while (parser.current < parser.token_count && parser.tokens[parser.current]->type != TOKEN_EOF) {
        ASTNode* decl = parse_declaration(&parser);
        if (decl) {
            declarations[declaration_count++] = decl;
        } else {
            // 跳过错误的token
            parser.current++;
        }

        if (declaration_count >= 32) break; // 防止溢出
    }

    root->data.translation_unit.declarations = declarations;
    root->data.translation_unit.declaration_count = declaration_count;

    return root;
}

// 解析声明
static ASTNode* parse_declaration(Parser* parser) {
    // 简化实现：尝试解析函数声明
    Token* token = peek_token(parser);
    if (!token) return NULL;

    // 检查是否是类型说明符开始
    if (token->type == TOKEN_INT || token->type == TOKEN_VOID || token->type == TOKEN_CHAR ||
        token->type == TOKEN_FLOAT || token->type == TOKEN_DOUBLE) {

        ASTNode* type = parse_type_specifier(parser);
        if (!type) return NULL;

        Token* name_token = consume_token(parser, TOKEN_IDENTIFIER, "Expected identifier");
        if (!name_token) {
            ast_free(type);
            return NULL;
        }

        // 检查是否是函数声明
        if (match_token(parser, TOKEN_LPAREN)) {
            return parse_function_declaration_with_type(parser, type, name_token);
        } else {
            return parse_variable_declaration_with_type(parser, type, name_token);
        }
    }

    return NULL;
}

// 解析带类型的函数声明
static ASTNode* parse_function_declaration_with_type(Parser* parser, ASTNode* return_type, Token* name_token) {
    ASTNode* func = ast_create_node(ASTC_FUNC_DECL, name_token->line, name_token->column);
    if (!func) {
        ast_free(return_type);
        return NULL;
    }

    func->data.func_decl.name = strdup(name_token->value);
    func->data.func_decl.return_type = return_type;
    func->data.func_decl.param_count = 0;
    func->data.func_decl.params = NULL;

    // 解析参数列表
    consume_token(parser, TOKEN_LPAREN, "Expected '('");

    // 简化实现：跳过参数解析
    while (!match_token(parser, TOKEN_RPAREN) && peek_token(parser)) {
        advance_token(parser);
    }

    consume_token(parser, TOKEN_RPAREN, "Expected ')'");

    // 检查是否有函数体
    if (match_token(parser, TOKEN_LBRACE)) {
        func->data.func_decl.has_body = true;
        func->data.func_decl.body = parse_compound_statement(parser);
    } else {
        func->data.func_decl.has_body = false;
        func->data.func_decl.body = NULL;
        consume_token(parser, TOKEN_SEMICOLON, "Expected ';' after function declaration");
    }

    return func;
}

// 解析带类型的变量声明
static ASTNode* parse_variable_declaration_with_type(Parser* parser, ASTNode* type, Token* name_token) {
    ASTNode* var = ast_create_node(ASTC_VAR_DECL, name_token->line, name_token->column);
    if (!var) {
        ast_free(type);
        return NULL;
    }

    var->data.var_decl.name = strdup(name_token->value);
    var->data.var_decl.type = type;

    // 检查是否有初始化器
    if (match_token(parser, TOKEN_ASSIGN)) {
        advance_token(parser);
        var->data.var_decl.initializer = parse_expression(parser);
    } else {
        var->data.var_decl.initializer = NULL;
    }

    consume_token(parser, TOKEN_SEMICOLON, "Expected ';' after variable declaration");
    return var;
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

// 前向声明
static bool generate_declaration(ASTNode* decl, CodeGenerator* cg);
static bool generate_function(ASTNode* func, CodeGenerator* cg);
static bool generate_statement(ASTNode* stmt, CodeGenerator* cg);
static bool generate_expression(ASTNode* expr, CodeGenerator* cg);

// 生成汇编代码
static bool generate_assembly(ASTNode* ast, CodeGenerator* cg) {
    if (!ast || !cg) return false;

    // 生成汇编文件头
    codegen_append(cg, ".text\n");

    if (ast->type == ASTC_TRANSLATION_UNIT) {
        // 遍历所有声明
        for (int i = 0; i < ast->data.translation_unit.declaration_count; i++) {
            ASTNode* decl = ast->data.translation_unit.declarations[i];
            if (!generate_declaration(decl, cg)) {
                return false;
            }
        }
    }

    return true;
}

// 生成声明的汇编代码
static bool generate_declaration(ASTNode* decl, CodeGenerator* cg) {
    if (!decl) return false;

    switch (decl->type) {
        case ASTC_FUNC_DECL:
            return generate_function(decl, cg);
        case ASTC_VAR_DECL:
            // 简化实现：暂时跳过全局变量
            return true;
        default:
            return true;
    }
}

// 生成函数的汇编代码
static bool generate_function(ASTNode* func, CodeGenerator* cg) {
    if (!func || func->type != ASTC_FUNC_DECL) return false;

    // 生成函数标签
    codegen_append(cg, ".global ");
    codegen_append(cg, func->data.func_decl.name);
    codegen_append(cg, "\n");
    codegen_append(cg, func->data.func_decl.name);
    codegen_append(cg, ":\n");

    // 生成函数序言
    codegen_append(cg, "    push rbp\n");
    codegen_append(cg, "    mov rbp, rsp\n");

    // 生成函数体
    if (func->data.func_decl.has_body && func->data.func_decl.body) {
        if (!generate_statement(func->data.func_decl.body, cg)) {
            return false;
        }
    }

    // 生成函数结尾（如果没有显式return）
    codegen_append(cg, "    mov rax, 0\n");  // 默认返回0
    codegen_append(cg, "    pop rbp\n");
    codegen_append(cg, "    ret\n");

    return true;
}

// 生成语句的汇编代码
static bool generate_statement(ASTNode* stmt, CodeGenerator* cg) {
    if (!stmt) return false;

    switch (stmt->type) {
        case ASTC_COMPOUND_STMT:
            // 生成复合语句中的所有语句
            for (int i = 0; i < stmt->data.compound_stmt.statement_count; i++) {
                if (!generate_statement(stmt->data.compound_stmt.statements[i], cg)) {
                    return false;
                }
            }
            return true;

        case ASTC_RETURN_STMT:
            // 生成return语句
            if (stmt->data.return_stmt.value) {
                // 生成返回值表达式
                if (!generate_expression(stmt->data.return_stmt.value, cg)) {
                    return false;
                }
                // 返回值已在rax中
            } else {
                codegen_append(cg, "    mov rax, 0\n");
            }
            codegen_append(cg, "    pop rbp\n");
            codegen_append(cg, "    ret\n");
            return true;

        case ASTC_EXPR_STMT:
            // 生成表达式语句
            if (stmt->data.expr_stmt.expr) {
                return generate_expression(stmt->data.expr_stmt.expr, cg);
            }
            return true;

        default:
            return true;
    }
}

// 生成表达式的汇编代码
static bool generate_expression(ASTNode* expr, CodeGenerator* cg) {
    if (!expr) return false;

    switch (expr->type) {
        case ASTC_EXPR_CONSTANT:
            // 生成常量
            if (expr->data.constant.type == ASTC_TYPE_INT) {
                char buffer[64];
                snprintf(buffer, sizeof(buffer), "    mov rax, %lld\n", expr->data.constant.int_val);
                codegen_append(cg, buffer);
            }
            return true;

        case ASTC_EXPR_IDENTIFIER:
            // 简化实现：暂时不处理变量
            codegen_append(cg, "    mov rax, 0\n");
            return true;

        case ASTC_EXPR_STRING_LITERAL:
            // 简化实现：暂时不处理字符串
            codegen_append(cg, "    mov rax, 0\n");
            return true;

        default:
            return true;
    }
}

// 汇编代码转字节码
static bool assembly_to_bytecode(const char* assembly, uint8_t** bytecode, size_t* size) {
    if (!assembly || !bytecode || !size) return false;

    // 分配字节码缓冲区
    size_t capacity = 1024;
    uint8_t* code = malloc(capacity);
    size_t offset = 0;

    // 简化的汇编解析器
    char* asm_copy = strdup(assembly);
    char* line = strtok(asm_copy, "\n");

    while (line) {
        // 跳过空行和注释
        while (*line == ' ' || *line == '\t') line++;
        if (*line == '\0' || *line == '#' || *line == ';') {
            line = strtok(NULL, "\n");
            continue;
        }

        // 解析汇编指令
        if (strstr(line, "mov rax,")) {
            // 解析 mov rax, immediate
            char* value_str = strstr(line, ",");
            if (value_str) {
                value_str++;
                while (*value_str == ' ') value_str++;

                int64_t value = atoll(value_str);

                // 生成 LOAD_IMM 指令
                if (offset + 10 > capacity) {
                    capacity *= 2;
                    code = realloc(code, capacity);
                }

                code[offset++] = 0x10; // 临时替换VM_OP_LOAD_IMM
                code[offset++] = 0; // 寄存器0 (rax)
                *(int64_t*)(code + offset) = value;
                offset += 8;
            }
        } else if (strstr(line, "ret")) {
            // 生成 RETURN 指令
            if (offset + 1 > capacity) {
                capacity *= 2;
                code = realloc(code, capacity);
            }
            code[offset++] = 0x31; // 临时替换VM_OP_RETURN
        } else if (strstr(line, "push")) {
            // 生成 PUSH 指令
            if (offset + 2 > capacity) {
                capacity *= 2;
                code = realloc(code, capacity);
            }
            code[offset++] = 0x50; // 临时替换VM_OP_PUSH
            code[offset++] = 0; // 寄存器0
        } else if (strstr(line, "pop")) {
            // 生成 POP 指令
            if (offset + 2 > capacity) {
                capacity *= 2;
                code = realloc(code, capacity);
            }
            code[offset++] = 0x51; // 临时替换VM_OP_POP
            code[offset++] = 0; // 寄存器0
        }

        line = strtok(NULL, "\n");
    }

    // 添加程序结束指令
    if (offset + 1 > capacity) {
        capacity *= 2;
        code = realloc(code, capacity);
    }
    code[offset++] = 0x01; // 临时替换VM_OP_HALT

    free(asm_copy);

    *bytecode = code;
    *size = offset;
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

// 加载ASTC字节码程序
static bool vm_load_astc_program(VMContext* ctx, ASTCBytecodeProgram* program) {
    if (!ctx || !program) return false;

    // 清理之前的字节码
    if (ctx->bytecode) free(ctx->bytecode);

    // 保存ASTC程序引用
    ctx->astc_program = program;
    ctx->program_counter = program->entry_point;

    return true;
}

// 执行ASTC字节码
static bool vm_execute(VMContext* ctx) {
    if (!ctx || !ctx->astc_program) return false;

    ctx->state = VM_STATE_RUNNING;
    ASTCBytecodeProgram* program = ctx->astc_program;

    while (ctx->program_counter < program->instruction_count && ctx->state == VM_STATE_RUNNING) {
        ASTCInstruction* instr = &program->instructions[ctx->program_counter];
        ctx->program_counter++;

        switch (instr->opcode) {
            case AST_NOP:
                // 空操作
                break;

            case AST_BLOCK:
                // 块开始 - 简化实现
                break;

            case AST_END:
                // 块结束 - 简化实现
                break;

            case AST_I32_CONST: {
                // 压入32位整数常量
                if (ctx->stack_pointer >= ctx->stack_size) {
                    strcpy(ctx->error_message, "Stack overflow");
                    ctx->state = VM_STATE_ERROR;
                    return false;
                }
                ctx->stack[ctx->stack_pointer++] = (uint64_t)instr->operand.i32;
                break;
            }

            case AST_I64_CONST: {
                // 压入64位整数常量
                if (ctx->stack_pointer >= ctx->stack_size) {
                    strcpy(ctx->error_message, "Stack overflow");
                    ctx->state = VM_STATE_ERROR;
                    return false;
                }
                ctx->stack[ctx->stack_pointer++] = (uint64_t)instr->operand.i64;
                break;
            }

            case AST_RETURN: {
                // 返回指令
                if (ctx->stack_pointer > 0) {
                    uint64_t return_value = ctx->stack[--ctx->stack_pointer];
                    printf("ASTC VM Return: %llu\n", return_value);
                } else {
                    printf("ASTC VM Return: void\n");
                }
                ctx->state = VM_STATE_STOPPED;
                break;
            }

            case AST_I32_ADD: {
                // 32位整数加法
                if (ctx->stack_pointer < 2) {
                    strcpy(ctx->error_message, "Stack underflow for i32.add");
                    ctx->state = VM_STATE_ERROR;
                    return false;
                }
                uint32_t b = (uint32_t)ctx->stack[--ctx->stack_pointer];
                uint32_t a = (uint32_t)ctx->stack[--ctx->stack_pointer];
                ctx->stack[ctx->stack_pointer++] = (uint64_t)(a + b);
                break;
            }

            case 0x21: { // 临时替换VM_OP_SUB
                if (ctx->program_counter + 3 >= ctx->bytecode_size) {
                    ctx->state = VM_STATE_ERROR;
                    strcpy(ctx->error_message, "SUB: Insufficient bytecode");
                    return false;
                }

                uint8_t reg1 = ctx->bytecode[ctx->program_counter + 1];
                uint8_t reg2 = ctx->bytecode[ctx->program_counter + 2];
                uint8_t dst_reg = ctx->bytecode[ctx->program_counter + 3];

                if (reg1 >= 16 || reg2 >= 16 || dst_reg >= 16) {
                    ctx->state = VM_STATE_ERROR;
                    strcpy(ctx->error_message, "SUB: Invalid register");
                    return false;
                }

                ctx->registers[dst_reg] = ctx->registers[reg1] - ctx->registers[reg2];
                ctx->program_counter += 4;
                break;
            }

            case 0x22: { // 临时替换VM_OP_MUL
                if (ctx->program_counter + 3 >= ctx->bytecode_size) {
                    ctx->state = VM_STATE_ERROR;
                    strcpy(ctx->error_message, "MUL: Insufficient bytecode");
                    return false;
                }

                uint8_t reg1 = ctx->bytecode[ctx->program_counter + 1];
                uint8_t reg2 = ctx->bytecode[ctx->program_counter + 2];
                uint8_t dst_reg = ctx->bytecode[ctx->program_counter + 3];

                if (reg1 >= 16 || reg2 >= 16 || dst_reg >= 16) {
                    ctx->state = VM_STATE_ERROR;
                    strcpy(ctx->error_message, "MUL: Invalid register");
                    return false;
                }

                ctx->registers[dst_reg] = ctx->registers[reg1] * ctx->registers[reg2];
                ctx->program_counter += 4;
                break;
            }

            case 0x23: { // 临时替换VM_OP_DIV
                if (ctx->program_counter + 3 >= ctx->bytecode_size) {
                    ctx->state = VM_STATE_ERROR;
                    strcpy(ctx->error_message, "DIV: Insufficient bytecode");
                    return false;
                }

                uint8_t reg1 = ctx->bytecode[ctx->program_counter + 1];
                uint8_t reg2 = ctx->bytecode[ctx->program_counter + 2];
                uint8_t dst_reg = ctx->bytecode[ctx->program_counter + 3];

                if (reg1 >= 16 || reg2 >= 16 || dst_reg >= 16) {
                    ctx->state = VM_STATE_ERROR;
                    strcpy(ctx->error_message, "DIV: Invalid register");
                    return false;
                }

                if (ctx->registers[reg2] == 0) {
                    ctx->state = VM_STATE_ERROR;
                    strcpy(ctx->error_message, "DIV: Division by zero");
                    return false;
                }

                ctx->registers[dst_reg] = ctx->registers[reg1] / ctx->registers[reg2];
                ctx->program_counter += 4;
                break;
            }

            case 0x50: { // 临时替换VM_OP_PUSH
                if (ctx->program_counter + 1 >= ctx->bytecode_size) {
                    ctx->state = VM_STATE_ERROR;
                    strcpy(ctx->error_message, "PUSH: Insufficient bytecode");
                    return false;
                }

                uint8_t reg = ctx->bytecode[ctx->program_counter + 1];

                if (reg >= 16) {
                    ctx->state = VM_STATE_ERROR;
                    strcpy(ctx->error_message, "PUSH: Invalid register");
                    return false;
                }

                if (ctx->stack_pointer >= ctx->stack_size) {
                    ctx->state = VM_STATE_ERROR;
                    strcpy(ctx->error_message, "PUSH: Stack overflow");
                    return false;
                }

                ctx->stack[ctx->stack_pointer++] = ctx->registers[reg];
                ctx->program_counter += 2;
                break;
            }

            case 0x51: { // 临时替换VM_OP_POP
                if (ctx->program_counter + 1 >= ctx->bytecode_size) {
                    ctx->state = VM_STATE_ERROR;
                    strcpy(ctx->error_message, "POP: Insufficient bytecode");
                    return false;
                }

                uint8_t reg = ctx->bytecode[ctx->program_counter + 1];

                if (reg >= 16) {
                    ctx->state = VM_STATE_ERROR;
                    strcpy(ctx->error_message, "POP: Invalid register");
                    return false;
                }

                if (ctx->stack_pointer == 0) {
                    ctx->state = VM_STATE_ERROR;
                    strcpy(ctx->error_message, "POP: Stack underflow");
                    return false;
                }

                ctx->registers[reg] = ctx->stack[--ctx->stack_pointer];
                ctx->program_counter += 2;
                break;
            }

            case 0x31: // 临时替换VM_OP_RETURN
                // 返回指令：将rax寄存器的值作为返回值
                ctx->state = VM_STATE_STOPPED;
                break;

            case 0x60: { // 临时替换VM_OP_PRINT
                if (ctx->program_counter + 1 >= ctx->bytecode_size) {
                    ctx->state = VM_STATE_ERROR;
                    strcpy(ctx->error_message, "PRINT: Insufficient bytecode");
                    return false;
                }

                uint8_t reg = ctx->bytecode[ctx->program_counter + 1];

                if (reg >= 16) {
                    ctx->state = VM_STATE_ERROR;
                    strcpy(ctx->error_message, "PRINT: Invalid register");
                    return false;
                }

                printf("Output: %lld\n", (long long)ctx->registers[reg]);
                ctx->program_counter += 2;
                break;
            }

            case 0xFF: // 临时替换VM_OP_EXIT
                ctx->state = VM_STATE_STOPPED;
                break;

            default:
                snprintf(ctx->error_message, sizeof(ctx->error_message),
                        "Unsupported ASTC opcode: 0x%02x", instr->opcode);
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
    
    // ASTC字节码生成 (新的c2astc流程)
    ASTCBytecodeProgram* astc_program = astc_bytecode_create();
    if (!astc_program) {
        strcpy(pipeline_state.error_message, "Failed to create ASTC bytecode program");
        return false;
    }

    // 从AST生成ASTC字节码
    if (generate_astc_bytecode_from_ast(pipeline_state.ast_root, astc_program) != 0) {
        strcpy(pipeline_state.error_message, "ASTC bytecode generation failed");
        astc_bytecode_free(astc_program);
        return false;
    }

    // 保存ASTC字节码程序
    if (pipeline_state.astc_program) {
        astc_bytecode_free((ASTCBytecodeProgram*)pipeline_state.astc_program);
    }
    pipeline_state.astc_program = astc_program;

    // 传统代码生成 (用于兼容性和调试)
    CodeGenerator cg;
    init_codegen(&cg);
    if (!generate_assembly(pipeline_state.ast_root, &cg)) {
        strcpy(pipeline_state.error_message, "Assembly generation failed");
        return false;
    }

    pipeline_state.assembly_code = strdup(cg.buffer);

    // 汇编转VM字节码 (用于执行)
    if (!assembly_to_bytecode(cg.buffer, &pipeline_state.bytecode, &pipeline_state.bytecode_size)) {
        strcpy(pipeline_state.error_message, "VM bytecode generation failed");
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
    if (!pipeline_state.astc_program) {
        strcpy(pipeline_state.error_message, "No ASTC program to execute");
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

    // 加载ASTC字节码程序
    if (!vm_load_astc_program(pipeline_state.vm_ctx, (ASTCBytecodeProgram*)pipeline_state.astc_program)) {
        strcpy(pipeline_state.error_message, "Failed to load ASTC program");
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

// AOT编译接口 - 将字节码编译为可执行文件
static bool pipeline_astc2native(const char* output_file) {
    if (!pipeline_state.bytecode || !output_file) {
        strcpy(pipeline_state.error_message, "No bytecode or invalid output file");
        return false;
    }

    // 创建AOT编译器
    AOTCompiler* aot = aot_create_compiler(TARGET_X64, OPT_STANDARD);
    if (!aot) {
        strcpy(pipeline_state.error_message, "Failed to create AOT compiler");
        return false;
    }

    // 编译为可执行文件
    CompileResult result = aot_compile_to_executable(aot, pipeline_state.bytecode,
                                                   pipeline_state.bytecode_size, output_file);

    if (result != COMPILE_SUCCESS) {
        snprintf(pipeline_state.error_message, sizeof(pipeline_state.error_message),
                "AOT compilation failed: %s", aot->error_message);
        aot_destroy_compiler(aot);
        return false;
    }

    aot_destroy_compiler(aot);
    return true;
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

// 获取ASTC字节码程序
static ASTCBytecodeProgram* pipeline_get_astc_program(void) {
    return (ASTCBytecodeProgram*)pipeline_state.astc_program;
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
    {"pipeline_astc2native", pipeline_astc2native},

    // 信息获取
    {"pipeline_get_error", pipeline_get_error},
    {"pipeline_get_assembly", pipeline_get_assembly},
    {"pipeline_get_bytecode", pipeline_get_bytecode},
    {"pipeline_get_astc_program", pipeline_get_astc_program},

    // VM接口
    {"create_vm_context", create_vm_context},
    {"destroy_vm_context", destroy_vm_context},
    {"vm_load_astc_program", vm_load_astc_program},
    {"vm_execute", vm_execute},

    // AOT编译器接口
    {"aot_create_compiler", aot_create_compiler},
    {"aot_destroy_compiler", aot_destroy_compiler},
    {"aot_compile_to_executable", aot_compile_to_executable},

    // ASTC核心函数接口
    {"ast_serialize_module", ast_serialize_module},
    {"ast_deserialize_module", ast_deserialize_module},
    {"ast_validate_module", ast_validate_module},
    {"ast_validate_export_declaration", ast_validate_export_declaration},
    {"ast_validate_import_declaration", ast_validate_import_declaration},
    {"astc_load_program", astc_load_program},
    {"astc_free_program", astc_free_program},
    {"astc_validate_program", astc_validate_program},
    {"ast_module_add_declaration", ast_module_add_declaration},
    {"ast_module_add_export", ast_module_add_export},
    {"ast_module_add_import", ast_module_add_import},
    {"ast_resolve_symbol_references", ast_resolve_symbol_references},

    // ASTC汇编接口
    {"astc_assembly_create", astc_assembly_create},
    {"astc_assembly_free", astc_assembly_free},
    {"astc_assembly_add_line", astc_assembly_add_line},
    {"astc_assembly_add_instruction", astc_assembly_add_instruction},
    {"astc_assembly_add_label", astc_assembly_add_label},
    {"astc_bytecode_to_assembly", astc_bytecode_to_assembly},

    {NULL, NULL}
};

// ===============================================
// 简单的导出函数 (供c2native工具的假导出表使用)
// ===============================================

/**
 * 简单的ASTC执行接口 - 供simple_loader调用
 * 这些函数将被c2native工具放在导出表的固定偏移位置
 */

// 这个函数将被放在偏移0处
int vm_execute_astc(const char* astc_file, int argc, char* argv[]) {
    printf("Pipeline Module: vm_execute_astc called with file: %s\n", astc_file);

    if (!astc_file) {
        printf("Pipeline Module: Error - astc_file is NULL\n");
        return 1;
    }

    printf("Pipeline Module: Successfully called vm_execute_astc\n");
    printf("Pipeline Module: Note - This is a placeholder implementation\n");
    printf("Pipeline Module: File: %s, argc: %d\n", astc_file, argc);

    // 简单的成功返回，表示函数被正确调用
    return 0;
}

// 这个函数将被放在偏移128处
int execute_astc(const char* astc_file, int argc, char* argv[]) {
    return vm_execute_astc(astc_file, argc, argv);
}

// 这个函数将被放在偏移256处
int native_main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Pipeline Module: Usage: native_main <astc_file> [args...]\n");
        return 1;
    }
    return vm_execute_astc(argv[1], argc - 1, argv + 1);
}

// 这个函数将被放在偏移384处
int test_export_function(void) {
    printf("Pipeline Module: test_export_function called\n");
    return 42;
}

// 这个函数将被放在偏移512处
int module_init(void) {
    printf("Pipeline Module: module_init called\n");
    return 0;
}

// 这个函数将被放在偏移640处
void module_cleanup(void) {
    printf("Pipeline Module: module_cleanup called\n");
}

// 这个函数将被放在偏移768处
void* pipeline_module_resolve(const char* symbol) {
    printf("Pipeline Module: pipeline_module_resolve called for symbol: %s\n", symbol ? symbol : "NULL");
    return NULL;
}

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

    if (pipeline_state.astc_program) {
        astc_bytecode_free((ASTCBytecodeProgram*)pipeline_state.astc_program);
        pipeline_state.astc_program = NULL;
    }

    if (pipeline_state.astc_assembly) {
        astc_assembly_free((ASTCAssemblyProgram*)pipeline_state.astc_assembly);
        pipeline_state.astc_assembly = NULL;
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