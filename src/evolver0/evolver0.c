#ifdef _WIN32
#include "tcc-win/tcc/include/compat.h"
#endif

/**
 * evolver0.c - 第零代自举编译器
 * 目标：能够编译自身的最小C编译器
 * 基于evolver0_integrated.c改进
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#ifdef _WIN32
/* Windows环境 */
#include "tcc-win/tcc/include/unistd.h"
#include <windows.h>
#else
/* Unix/Linux环境 */
#include <unistd.h>
#include <sys/stat.h>
#endif
#include <stdarg.h>

// ====================================
// 基础定义
// ====================================

#define MAX_TOKENS 100000
#define MAX_CODE_SIZE 1048576  // 1MB

// 包含Token定义
#include "evolver0_token.h"

// ====================================
// 全局变量
// ====================================

typedef struct {
    const char *input_file;
    const char *output_file;
    bool verbose;
    bool dump_ast;
    bool dump_asm;
    const char *target;  // 目标平台: elf, pe, macho
} CompilerOptions;

// ====================================
// 包含AST定义和操作
// ====================================

#include "evolver0_ast.h"
#ifndef EVOLVER0_AST_INC_C
#define EVOLVER0_AST_INC_C
#include "evolver0_ast.inc.c"
#endif

// ====================================
// 包含词法分析器实现
// ====================================

#ifndef EVOLVER0_LEXER_INCLUDED
#define EVOLVER0_LEXER_INCLUDED
#include "evolver0_lexer.inc.c"
#endif

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

// ====================================
// 包含解析器实现
// ====================================

#ifndef EVOLVER0_PARSER_INCLUDED
#define EVOLVER0_PARSER_INCLUDED
#include "evolver0_parser.inc.c"
#endif

// ====================================
// 代码生成器
// ====================================

// ====================================
// 包含代码生成器实现
// ====================================

#ifndef EVOLVER0_CODEGEN_INCLUDED
#define EVOLVER0_CODEGEN_INCLUDED
// 整合evolver0_improved.c中的代码生成改进
// 特别是控制流处理和表达式求值

// 从evolver0_improved.c整合改进的代码生成函数
static void emit_syscall(CodeGen *gen) {
    emit_byte(gen, 0x0F);
    emit_byte(gen, 0x05);
}

// 整合改进的if语句代码生成
static void gen_if_stmt_improved(CodeGen *gen, ASTNode *node) {
    static int if_counter = 0;
    char else_label[32], end_label[32];
    
    snprintf(else_label, sizeof(else_label), ".L_else_%d", if_counter);
    snprintf(end_label, sizeof(end_label), ".L_endif_%d", if_counter);
    if_counter++;
    
    // 计算条件
    gen_expression(gen, node->data.if_stmt.condition);
    
    // TEST RAX, RAX
    emit_byte(gen, 0x48);
    emit_byte(gen, 0x85);
    emit_byte(gen, 0xC0);
    
    // JE else_label
    emit_jcc(gen, 0x04, else_label);
    
    // then分支
    gen_statement(gen, node->data.if_stmt.then_stmt);
    
    if (node->data.if_stmt.else_stmt) {
        emit_jmp(gen, end_label);
        add_label(gen, else_label);
        gen_statement(gen, node->data.if_stmt.else_stmt);
        add_label(gen, end_label);
    } else {
        add_label(gen, else_label);
    }
}

// 整合改进的while语句代码生成
static void gen_while_stmt_improved(CodeGen *gen, ASTNode *node) {
    static int while_counter = 0;
    char loop_label[32], end_label[32];
    
    snprintf(loop_label, sizeof(loop_label), ".L_while_%d", while_counter);
    snprintf(end_label, sizeof(end_label), ".L_endwhile_%d", while_counter);
    while_counter++;
    
    add_label(gen, loop_label);
    
    // 计算条件
    gen_expression(gen, node->data.while_stmt.condition);
    
    // TEST RAX, RAX
    emit_byte(gen, 0x48);
    emit_byte(gen, 0x85);
    emit_byte(gen, 0xC0);
    
    // JE end_label
    emit_jcc(gen, 0x04, end_label);
    
    // 循环体
    gen_statement(gen, node->data.while_stmt.body);
    
    // JMP loop_label
    emit_jmp(gen, loop_label);
    
    add_label(gen, end_label);
}

// 整合改进的return语句代码生成
static void gen_return_stmt_improved(CodeGen *gen, ASTNode *node) {
    if (node->data.return_stmt.value) {
        gen_expression(gen, node->data.return_stmt.value);
    } else {
        emit_mov_reg_imm(gen, RAX, 0);
    }
    
    // 如果是main函数，使用syscall直接退出
    if (gen->in_main) {
        // 将返回值移到RDI作为退出码
        emit_byte(gen, 0x48);  // REX.W
        emit_byte(gen, 0x89);  // MOV
        emit_byte(gen, 0xC7);  // ModRM: rdi = rax
        
        // 设置系统调用号
        emit_mov_reg_imm(gen, RAX, 60);  // sys_exit
        
        // 执行系统调用
        emit_syscall(gen);
    } else {
        // 普通函数返回
        // MOV RSP, RBP
        emit_byte(gen, 0x48);
        emit_byte(gen, 0x89);
        emit_byte(gen, 0xEC);
        
        emit_pop(gen, RBP);
        emit_ret(gen);
    }
}

// 修改原有的语句生成函数，整合改进的代码
static void gen_statement(CodeGen *gen, ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_COMPOUND_STMT:
            gen_compound_stmt(gen, node);
            break;
            
        case AST_EXPRESSION_STMT:
            gen_expression_stmt(gen, node);
            break;
            
        case AST_IF_STMT:
            gen_if_stmt_improved(gen, node);  // 使用改进版
            break;
            
        case AST_WHILE_STMT:
            gen_while_stmt_improved(gen, node);  // 使用改进版
            break;
            
        case AST_FOR_STMT:
            gen_for_stmt(gen, node);
            break;
            
        case AST_RETURN_STMT:
            gen_return_stmt_improved(gen, node);  // 使用改进版
            break;
            
        case AST_VAR_DECL:
            gen_var_decl(gen, node);
            break;
            
        default:
            fprintf(stderr, "未实现的语句类型: %d\n", node->type);
    }
    
    // 处理链表中的下一个声明
    if (node->next) {
        gen_statement(gen, node->next);
    }
}

// 修改函数生成，标记main函数
static void gen_function(CodeGen *gen, ASTNode *node) {
    // 添加函数标签
    add_label(gen, node->data.function.name);
    
    // 检查是否是main函数
    gen->in_main = (strcmp(node->data.function.name, "main") == 0);
    gen->current_function = strdup(node->data.function.name);
    
    // 重置局部变量
    gen->local_count = 0;
    gen->stack_offset = 0;
    gen->max_stack_size = 0;
    
    // 函数序言
    emit_push(gen, RBP);
    // MOV RBP, RSP
    emit_byte(gen, 0x48);
    emit_byte(gen, 0x89);
    emit_byte(gen, 0xE5);
    
    // 保存函数序言后的位置，用于后续调整栈空间
    size_t stack_adjustment_pos = gen->size;
    // SUB RSP, imm32 (占位)
    emit_byte(gen, 0x48);
    emit_byte(gen, 0x81);
    emit_byte(gen, 0xEC);
    emit_int32(gen, 0);
    
    // 处理参数
    const int arg_regs[] = {RDI, RSI, RDX, RCX, R8, R9};
    for (int i = 0; i < node->data.function.param_count && i < 6; i++) {
        ASTNode *param = node->data.function.params[i];
        if (param->type == AST_PARAM_DECL) {
            int offset = add_local(gen, param->data.var_decl.name, param->type_info);
            emit_mov_mem_reg(gen, offset, arg_regs[i]);
        }
    }
    
    // 生成函数体
    if (node->data.function.body) {
        gen_statement(gen, node->data.function.body);
    }
    
    // 如果函数体没有显式返回，添加默认返回
    // 检查最后生成的字节是否是RET (0xC3)
    if (gen->size == 0 || gen->code[gen->size - 1] != 0xC3) {
        // 函数结尾标签
        add_label(gen, ".L_func_epilogue");
        
        // 如果是main函数且没有显式返回，添加默认退出
        if (gen->in_main) {
            // 设置返回值为0
            emit_mov_reg_imm(gen, RAX, 0);
            
            // 将返回值移到RDI作为退出码
            emit_byte(gen, 0x48);  // REX.W
            emit_byte(gen, 0x89);  // MOV
            emit_byte(gen, 0xC7);  // ModRM: rdi = rax
            
            // 设置系统调用号
            emit_mov_reg_imm(gen, RAX, 60);  // sys_exit
            
            // 执行系统调用
            emit_syscall(gen);
        } else {
            // 普通函数的默认返回
            // 函数尾声
            // MOV RSP, RBP
            emit_byte(gen, 0x48);
            emit_byte(gen, 0x89);
            emit_byte(gen, 0xEC);
            
            emit_pop(gen, RBP);
            emit_ret(gen);
        }
    }
    
    // 回填栈空间大小
    int32_t stack_size = (gen->max_stack_size + 15) & ~15; // 16字节对齐
    memcpy(gen->code + stack_adjustment_pos + 3, &stack_size, 4);
    
    // 清理
    free(gen->current_function);
    gen->current_function = NULL;
    gen->in_main = false;
}

#include "evolver0_codegen.inc.c"
#endif

// ====================================
// 包含ELF生成器实现
// ====================================

#ifndef EVOLVER0_ELF_INCLUDED
#define EVOLVER0_ELF_INCLUDED
#include "evolver0_elf.inc.c"
#endif

// ====================================
// 包含PE生成器实现
// ====================================

#ifndef EVOLVER0_PE_INCLUDED
#define EVOLVER0_PE_INCLUDED
#include "evolver0_pe.inc.c"
#endif

// ====================================
// 包含Mach-O生成器实现
// ====================================

#ifndef EVOLVER0_MACHO_INCLUDED
#define EVOLVER0_MACHO_INCLUDED
#include "evolver0_macho.inc.c"
#endif

// ====================================
// 公共接口适配
// ====================================

// 适配旧接口到新接口
static ASTNode* parse_program(Parser *parser) {
    return parse_tokens(parser->tokens, parser->token_count);
}

// 适配代码生成接口
static bool codegen_program(ASTNode *ast, CodeGen *gen) {
    size_t code_size;
    uint8_t *code = generate_code(ast, &code_size);
    
    if (!code) return false;
    
    // 复制生成的代码到gen结构
    if (gen->capacity < code_size) {
        gen->capacity = code_size;
        gen->code = realloc(gen->code, gen->capacity);
    }
    
    memcpy(gen->code, code, code_size);
    gen->size = code_size;
    
    free(code);
    return true;
}

// 写入ELF文件
static int write_elf_file(const char *filename, unsigned char *code, size_t code_size) {
    // 对于64位程序，入口点应该是代码段的起始地址
    // 因为我们在开头生成了_start函数
    return create_elf_executable(filename, code, code_size, 64);  // 64位
}

// 写入PE文件
static int write_pe_file_wrapper(const char *filename, unsigned char *code, size_t code_size) {
    return write_pe_file(filename, code, code_size);
}

// 写入Mach-O文件
static int write_macho_file(const char *filename, unsigned char *code, size_t code_size) {
    return create_macho64_executable(filename, code, code_size);
}

// 根据目标平台选择输出格式
static int write_executable_file(const char *filename, unsigned char *code, size_t code_size, const char *target) {
    if (!target || strcmp(target, "elf") == 0) {
        return write_elf_file(filename, code, code_size);
    } else if (strcmp(target, "pe") == 0) {
        return write_pe_file_wrapper(filename, code, code_size);
    } else if (strcmp(target, "macho") == 0) {
        return write_macho_file(filename, code, code_size);
    } else {
        fprintf(stderr, "不支持的目标格式: %s\n", target);
        return -1;
    }
}

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
    fprintf(stderr, "Usage: %s [options] <input.c> -o <o>\n", program);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -v, --verbose     Enable verbose output\n");
    fprintf(stderr, "  --dump-ast        Dump AST\n");
    fprintf(stderr, "  --dump-asm        Dump generated assembly\n");
    fprintf(stderr, "  --target=<format> Target format (elf, pe, macho), default: elf\n");
    fprintf(stderr, "  -h, --help        Show this help\n");
}

int main(int argc, char *argv[]) {
    CompilerOptions options = {0};
    options.target = "elf";  // 默认目标格式为ELF
    
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
        } else if (strncmp(argv[i], "--target=", 9) == 0) {
            options.target = argv[i] + 9;
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
        fprintf(stderr, "错误：需要指定输入文件和输出文件\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // 读取源文件
    char *source = read_file(options.input_file);
    if (!source) {
        fprintf(stderr, "错误：无法读取文件 %s\n", options.input_file);
        return 1;
    }
    
    if (options.verbose) {
        printf("Compiling %s -> %s\n", options.input_file, options.output_file);
    }
    
    // 词法分析
    int token_count;
    Token *tokens = tokenize_source(source, options.input_file, &token_count);
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
    
    // 生成可执行文件
    if (write_executable_file(options.output_file, gen.code, gen.size, options.target) != 0) {
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