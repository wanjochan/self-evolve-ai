/**
 * astc2rt.c - ASTC到Runtime转换库实现
 *
 * 正确的设计：将ASTC格式的Runtime虚拟机转换为可执行的.rt文件
 * 流程: runtime.astc (ASTC虚拟机) → (JIT编译/解释器生成) → runtime{arch}{bits}.rt
 *
 * 架构设计：
 * 1. 解析ASTC格式的Runtime虚拟机代码
 * 2. 生成包含ASTC解释器的机器码
 * 3. 嵌入libc转发表和ASTC指令处理
 * 4. 输出完整的Runtime.rt文件
 */

// TODO: [Module] 实现延迟链接和符号解析机制
// TODO: [Module] 支持增量编译和代码缓存策略
// TODO: [Module] 添加跨模块优化支持
// TODO: [Module] JIT编译中实现动态符号查找

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "compiler_astc2rt.h"
#include "compiler_c2astc.h"
#include "compiler_codegen.h"
#include "compiler_codegen_x64.h"

// ===============================================
// 代码生成器实现
// ===============================================

CodeGen* old_codegen_init(void) {
    CodeGen* gen = malloc(sizeof(CodeGen));
    if (!gen) return NULL;

    gen->code_capacity = 4096;
    gen->code = malloc(gen->code_capacity);
    if (!gen->code) {
        free(gen);
        return NULL;
    }
    gen->code_size = 0;
    return gen;
}

// 新的ASTC代码生成器实现
CodeGen* astc_codegen_init(void) {
    CodeGen* gen = malloc(sizeof(CodeGen));
    if (!gen) return NULL;

    gen->code_capacity = 4096;
    gen->code = malloc(gen->code_capacity);
    if (!gen->code) {
        free(gen);
        return NULL;
    }

    gen->code_size = 0;
    return gen;
}

void astc_codegen_free(CodeGen* gen) {
    if (!gen) return;
    if (gen->code) {
        free(gen->code);
    }
    free(gen);
}

void old_codegen_free(CodeGen* gen) {
    if (gen) {
        if (gen->code) {
            free(gen->code);
        }
        free(gen);
    }
}

void emit_byte(CodeGen* gen, uint8_t byte) {
    if (!gen) return;

    if (gen->code_size >= gen->code_capacity) {
        gen->code_capacity *= 2;
        uint8_t* new_code = realloc(gen->code, gen->code_capacity);
        if (!new_code) return; // 处理内存分配失败
        gen->code = new_code;
    }
    gen->code[gen->code_size++] = byte;
}

void emit_int32(CodeGen* gen, int32_t value) {
    emit_byte(gen, value & 0xFF);
    emit_byte(gen, (value >> 8) & 0xFF);
    emit_byte(gen, (value >> 16) & 0xFF);
    emit_byte(gen, (value >> 24) & 0xFF);
}

void emit_int64(CodeGen* gen, int64_t value) {
    emit_int32(gen, (int32_t)(value & 0xFFFFFFFF));
    emit_int32(gen, (int32_t)((value >> 32) & 0xFFFFFFFF));
}

// ===============================================
// 代码生成辅助函数
// ===============================================

// 编译常量表达式（借鉴TinyCC的立即数处理）
static void compile_constant(CodeGen* gen, struct ASTNode* node) {
    if (node->type == ASTC_EXPR_CONSTANT && node->data.constant.type == ASTC_TYPE_INT) {
        // mov eax, immediate
        emit_byte(gen, 0xb8);
        emit_int32(gen, node->data.constant.int_val);
    }
}

// 编译表达式（借鉴TinyCC的表达式编译）
static void compile_expression(CodeGen* gen, struct ASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case ASTC_EXPR_CONSTANT:
            compile_constant(gen, node);
            break;
        default:
            // 默认返回0
            emit_byte(gen, 0xb8);  // mov eax, 0
            emit_int32(gen, 0);
            break;
    }
}

// 编译return语句（借鉴TinyCC的函数返回处理）
static void compile_return(CodeGen* gen, struct ASTNode* node) {
    if (!node) return;

    if (node->data.return_stmt.value) {
        // 编译返回值表达式
        compile_expression(gen, node->data.return_stmt.value);
    }
    // ret指令
    emit_byte(gen, 0xc3);
}

// 编译语句（借鉴TinyCC的语句编译）
static void compile_statement(CodeGen* gen, struct ASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case ASTC_RETURN_STMT:
            compile_return(gen, node);
            break;
        case ASTC_COMPOUND_STMT:
            // 编译复合语句中的所有子语句
            for (int i = 0; i < node->data.compound_stmt.statement_count; i++) {
                compile_statement(gen, node->data.compound_stmt.statements[i]);
            }
            break;
        default:
            break;
    }
}

// 编译函数（借鉴TinyCC的函数编译）
static void compile_function(CodeGen* gen, struct ASTNode* node) {
    if (!node) return;

    // 函数序言
    emit_byte(gen, 0x55);        // push rbp
    emit_byte(gen, 0x48);        // mov rbp, rsp
    emit_byte(gen, 0x89);
    emit_byte(gen, 0xe5);

    // 编译函数体
    if (node->data.func_decl.body) {
        compile_statement(gen, node->data.func_decl.body);
    }

    // 如果没有显式return，添加默认返回
    emit_byte(gen, 0xb8);        // mov eax, 0
    emit_int32(gen, 0);
    emit_byte(gen, 0x5d);        // pop rbp
    emit_byte(gen, 0xc3);        // ret
}

// 编译整个翻译单元（包含多个函数）
static void compile_runtime_from_translation_unit(CodeGen* gen, struct ASTNode* node) {
    if (!gen || !node) return;
    
    printf("Compiling runtime from translation unit...\n");

    // 遍历翻译单元中的所有声明
    if (node->type == ASTC_TRANSLATION_UNIT && node->data.translation_unit.declarations) {
        int func_count = 0;

        // 遍历声明数组
        for (int i = 0; i < node->data.translation_unit.declaration_count; i++) {
            struct ASTNode* decl = node->data.translation_unit.declarations[i];

            if (decl && decl->type == ASTC_FUNC_DECL) {
                printf("  Compiling function: %s\n", decl->data.func_decl.name);

                // 为每个函数生成标签和代码
                if (strcmp(decl->data.func_decl.name, "evolver0_runtime_main") == 0) {
                    // 这是主入口函数，放在开头
                    compile_function(gen, decl);
                    func_count++;
                } else {
                    // 其他函数
                    compile_function(gen, decl);
                    func_count++;
                }
            }
        }

        printf("  Compiled %d functions from translation unit\n", func_count);
    } else {
        printf("  Warning: Not a valid translation unit\n");
    }
}

// ===============================================
// 公开API实现
// ===============================================

// ASTC JIT编译器 - 将ASTC字节码指令翻译成二进制机器码
// 使用架构特定的codegen函数，支持跨平台
void compile_astc_instruction_to_machine_code(CodeGen* gen, uint8_t opcode, uint8_t* operands, size_t operand_len) {
    switch (opcode) {
        case 0x00: // NOP
            x64_emit_nop(gen);
            break;

        case 0x01: // HALT
            x64_emit_halt_with_return_value(gen);
            break;

        case 0x10: // CONST_I32
            if (operand_len >= 4) {
                uint32_t value = *(uint32_t*)operands;
                x64_emit_const_i32(gen, value);
            }
            break;

        case 0x20: // ADD
            x64_emit_binary_op_add(gen);
            break;

        case 0x21: // SUB
            x64_emit_binary_op_sub(gen);
            break;

        case 0x22: // MUL
            x64_emit_binary_op_mul(gen);
            break;

        case 0xF0: // LIBC_CALL
            if (operand_len >= 4) {
                uint16_t func_id = *(uint16_t*)operands;
                uint16_t arg_count = *(uint16_t*)(operands + 2);
                x64_emit_libc_call(gen, func_id, arg_count);
            }
            break;

        default:
            // 未知指令，生成nop
            x64_emit_nop(gen);
            break;
    }
}

// ASTC JIT编译器 - 将ASTC字节码指令翻译成汇编代码
// 使用符合命名规范的proper codegen架构
void compile_astc_instruction_to_asm(CodeGenerator* cg, uint8_t opcode, uint8_t* operands, size_t operand_count) {
    char temp_buffer[256];

    switch (opcode) {
        case 0x00: // NOP
            codegen_append_public(cg, "    nop\n");
            break;

        case 0x01: // HALT
            codegen_append_public(cg, "    mov rsp, rbp\n");
            codegen_append_public(cg, "    pop rbp\n");
            codegen_append_public(cg, "    ret\n");
            break;

        case 0x10: // CONST_I32
            if (operand_count >= 4) {
                uint32_t value = *(uint32_t*)operands;
                sprintf(temp_buffer, "    mov eax, %u\n", value);
                codegen_append_public(cg, temp_buffer);
                codegen_append_public(cg, "    push rax\n");
            }
            break;

        case 0x20: // ADD
            codegen_append_public(cg, "    pop rbx\n");
            codegen_append_public(cg, "    pop rax\n");
            codegen_append_public(cg, "    add rax, rbx\n");
            codegen_append_public(cg, "    push rax\n");
            break;

        case 0x21: // SUB
            codegen_append_public(cg, "    pop rbx\n");
            codegen_append_public(cg, "    pop rax\n");
            codegen_append_public(cg, "    sub rax, rbx\n");
            codegen_append_public(cg, "    push rax\n");
            break;

        case 0x22: // MUL
            codegen_append_public(cg, "    pop rbx\n");
            codegen_append_public(cg, "    pop rax\n");
            codegen_append_public(cg, "    imul rax, rbx\n");
            codegen_append_public(cg, "    push rax\n");
            break;

        case 0xF0: // LIBC_CALL
            // 生成libc调用的机器码
            // 这里需要调用libc转发函数
            // 简化版本：调用printf
            if (operand_count >= 4) {
                uint16_t func_id = *(uint16_t*)operands;
                uint16_t arg_count = *(uint16_t*)(operands + 2);

                // 根据func_id生成对应的libc调用
                if (func_id == 0x0030) { // LIBC_PRINTF
                    sprintf(temp_buffer, "    ; LIBC_PRINTF call (func_id=%u, args=%u)\n", func_id, arg_count);
                    codegen_append_public(cg, temp_buffer);
                    codegen_append_public(cg, "    call printf\n");
                }
            }
            break;

        default:
            // 未知指令，生成NOP
            codegen_append_public(cg, "    nop\n");
            break;
    }
}

// ASTC JIT编译主函数 - 类似TinyCC的代码生成
int compile_astc_to_machine_code(uint8_t* astc_data, size_t astc_size, CodeGen* gen) {
    printf("JIT compiling ASTC bytecode to x64 machine code...\n");

    // 跳过ASTC头部
    if (astc_size < 16 || memcmp(astc_data, "ASTC", 4) != 0) {
        printf("Error: Invalid ASTC format\n");
        return 1;
    }

    uint32_t* header = (uint32_t*)astc_data;
    uint32_t version = header[1];
    uint32_t data_size = header[2];
    uint32_t entry_point = header[3];

    printf("ASTC version: %u, data_size: %u, entry_point: %u\n", version, data_size, entry_point);

    // 生成函数序言
    x64_emit_function_prologue(gen);

    // 编译ASTC指令序列
    uint8_t* code = astc_data + 16;
    size_t code_size = astc_size - 16;
    size_t pc = 0;

    while (pc < code_size) {
        uint8_t opcode = code[pc++];

        // 根据指令类型确定操作数长度
        size_t operand_len = 0;
        switch (opcode) {
            case 0x10: operand_len = 4; break; // CONST_I32
            case 0xF0: operand_len = 4; break; // LIBC_CALL
            default: operand_len = 0; break;
        }

        uint8_t* operands = (pc + operand_len <= code_size) ? &code[pc] : NULL;

        // JIT编译这条指令到机器码
        compile_astc_instruction_to_machine_code(gen, opcode, operands, operand_len);

        pc += operand_len;
    }

    // 如果没有显式的HALT，添加默认返回
    x64_emit_function_epilogue(gen);

    printf("JIT compilation completed: %zu ASTC bytes → %zu machine code bytes\n",
           code_size, gen->code_size);

    return 0;
}

int generate_runtime_file(uint8_t* code, size_t code_size, const char* output_file) {
    FILE* fp = fopen(output_file, "wb");
    if (!fp) {
        printf("Error: Cannot open output file %s\n", output_file);
        return 1;
    }

    // 创建运行时文件头
    RuntimeHeader header;
    strncpy(header.magic, "RTME", 4);
    header.version = 1;
    header.size = (uint32_t)code_size;
    header.entry_point = sizeof(RuntimeHeader); // 入口点在header之后

    // 写入文件头
    fwrite(&header, sizeof(header), 1, fp);

    // 写入代码
    fwrite(code, 1, code_size, fp);

    fclose(fp);
    printf("Generated runtime file: %s (%zu bytes + header)\n", output_file, code_size);
    return 0;
}

int compile_astc_to_runtime_bin(const char* astc_file, const char* output_file) {
    FILE* fp = fopen(astc_file, "rb");
    if (!fp) {
        printf("Error: Cannot open ASTC file %s\n", astc_file);
        return 1;
    }

    // 获取文件大小
    fseek(fp, 0, SEEK_END);
    size_t astc_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // 读取ASTC文件内容
    unsigned char* astc_data = (unsigned char*)malloc(astc_size);
    if (!astc_data) {
        printf("Error: Memory allocation failed\n");
        fclose(fp);
        return 1;
    }

    size_t read_size = fread(astc_data, 1, astc_size, fp);
    fclose(fp);

    if (read_size != astc_size) {
        printf("Error: Failed to read ASTC file\n");
        free(astc_data);
        return 1;
    }

    printf("ASTC file size: %zu bytes\n", astc_size);

    // 创建代码生成器
    CodeGen* gen = old_codegen_init();
    if (!gen) {
        printf("Error: Failed to initialize code generator\n");
        free(astc_data);
        return 1;
    }

    // 使用新的JIT编译器：ASTC字节码 → x64机器码
    if (compile_astc_to_machine_code(astc_data, astc_size, gen) != 0) {
        printf("Error: JIT compilation failed\n");
        free(astc_data);
        old_codegen_free(gen);
        return 1;
    }

    free(astc_data);

    // 生成运行时文件
    int result = generate_runtime_file(gen->code, gen->code_size, output_file);

    // 释放资源
    old_codegen_free(gen);

    return result;
}

int compile_c_to_runtime_bin(const char* c_file, const char* output_file) {
    // 首先将C文件编译成ASTC
    C2AstcOptions options = c2astc_default_options();
    struct ASTNode* ast = c2astc_convert_file(c_file, &options);

    if (!ast) {
        printf("Error: Failed to convert C file to ASTC\n");
        return 1;
    }

    // 创建代码生成器
    CodeGen* gen = old_codegen_init();
    if (!gen) {
        printf("Error: Failed to initialize code generator\n");
        ast_free(ast);
        return 1;
    }

    // 使用JIT编译器处理ASTC
    // 这里应该将C源码先转换为ASTC，然后JIT编译
    printf("Warning: C to runtime conversion should use C→ASTC→JIT pipeline\n");
    printf("Generating minimal runtime stub for compatibility\n");

    // 生成最小的Runtime机器码
    emit_byte(gen, 0x55);        // push rbp
    emit_byte(gen, 0x48);        // mov rbp, rsp
    emit_byte(gen, 0x89);
    emit_byte(gen, 0xe5);
    emit_byte(gen, 0xb8);        // mov eax, 42
    emit_int32(gen, 42);
    emit_byte(gen, 0x5d);        // pop rbp
    emit_byte(gen, 0xc3);        // ret

    // 生成运行时文件
    int result = generate_runtime_file(gen->code, gen->code_size, output_file);

    // 释放资源
    old_codegen_free(gen);
    ast_free(ast);

    return result;
} 