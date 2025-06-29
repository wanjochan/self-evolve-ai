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
#include "compiler_codegen_arm64.h"

// 前向声明
int compile_ast_node_to_machine_code(struct ASTNode* node, CodeGen* gen);

// ===============================================
// 架构检测实现
// ===============================================

/**
 * 检测当前运行时架构
 */
TargetArch detect_runtime_architecture(void) {
    // 使用编译时宏检测架构
    #if defined(_M_X64) || defined(__x86_64__) || defined(__x86_64) || defined(__amd64__) || defined(__amd64)
        return ARCH_X86_64;
    #elif defined(_M_IX86) || defined(__i386__) || defined(__i386) || defined(i386)
        return ARCH_X86_32;
    #elif defined(_M_ARM64) || defined(__aarch64__)
        return ARCH_ARM64;
    #elif defined(_M_ARM) || defined(__arm__) || defined(__arm)
        return ARCH_ARM32;
    #else
        return ARCH_UNKNOWN;
    #endif
}

/**
 * 获取架构名称字符串
 */
const char* get_architecture_name(TargetArch arch) {
    switch (arch) {
        case ARCH_X86_32: return "x86_32";
        case ARCH_X86_64: return "x86_64";
        case ARCH_ARM32:  return "arm32";
        case ARCH_ARM64:  return "arm64";
        default:          return "unknown";
    }
}

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
    gen->target_arch = detect_runtime_architecture();
    return gen;
}

// 新的ASTC代码生成器实现
CodeGen* astc_codegen_init(TargetArch target_arch) {
    CodeGen* gen = malloc(sizeof(CodeGen));
    if (!gen) return NULL;

    gen->code_capacity = 4096;
    gen->code = malloc(gen->code_capacity);
    if (!gen->code) {
        free(gen);
        return NULL;
    }

    gen->code_size = 0;

    // 设置目标架构
    if (target_arch == ARCH_UNKNOWN) {
        gen->target_arch = detect_runtime_architecture();
    } else {
        gen->target_arch = target_arch;
    }

    printf("Initialized code generator for architecture: %s\n",
           get_architecture_name(gen->target_arch));

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

// 架构特定的指令生成函数指针
typedef void (*emit_nop_func)(CodeGen* gen);
typedef void (*emit_halt_func)(CodeGen* gen);
typedef void (*emit_const_i32_func)(CodeGen* gen, uint32_t value);
typedef void (*emit_binary_op_func)(CodeGen* gen);
typedef void (*emit_libc_call_func)(CodeGen* gen, uint16_t func_id, uint16_t arg_count);
typedef void (*emit_function_prologue_func)(CodeGen* gen);
typedef void (*emit_function_epilogue_func)(CodeGen* gen);

// 架构特定的代码生成函数表
typedef struct {
    emit_nop_func emit_nop;
    emit_halt_func emit_halt;
    emit_const_i32_func emit_const_i32;
    emit_binary_op_func emit_add;
    emit_binary_op_func emit_sub;
    emit_binary_op_func emit_mul;
    emit_binary_op_func emit_div;
    emit_libc_call_func emit_libc_call;
    emit_function_prologue_func emit_function_prologue;
    emit_function_epilogue_func emit_function_epilogue;
} ArchCodegenTable;

// 获取架构特定的代码生成函数表
ArchCodegenTable* get_arch_codegen_table(TargetArch arch) {
    static ArchCodegenTable x64_table = {0};
    static ArchCodegenTable x86_table = {0};
    static ArchCodegenTable arm64_table = {0};
    static ArchCodegenTable arm32_table = {0};

    switch (arch) {
        case ARCH_X86_64:
            if (!x64_table.emit_nop) {
                // 初始化x64函数表
                x64_table.emit_nop = x64_emit_nop;
                x64_table.emit_halt = x64_emit_halt_with_return_value;
                x64_table.emit_const_i32 = x64_emit_const_i32;
                x64_table.emit_add = x64_emit_binary_op_add;
                x64_table.emit_sub = x64_emit_binary_op_sub;
                x64_table.emit_mul = x64_emit_binary_op_mul;
                x64_table.emit_div = x64_emit_div;
                x64_table.emit_libc_call = x64_emit_libc_call;
                x64_table.emit_function_prologue = x64_emit_function_prologue;
                x64_table.emit_function_epilogue = x64_emit_function_epilogue;
            }
            return &x64_table;

        case ARCH_X86_32:
            // TODO: 实现x86_32支持
            printf("Warning: x86_32 architecture not fully implemented, using x64 fallback\n");
            return get_arch_codegen_table(ARCH_X86_64);

        case ARCH_ARM64:
            if (!arm64_table.emit_nop) {
                // 初始化ARM64函数表
                arm64_table.emit_nop = arm64_emit_nop;
                arm64_table.emit_halt = arm64_emit_halt_with_return_value;
                arm64_table.emit_const_i32 = arm64_emit_const_i32;
                arm64_table.emit_add = arm64_emit_binary_op_add;
                arm64_table.emit_sub = arm64_emit_binary_op_sub;
                arm64_table.emit_mul = arm64_emit_binary_op_mul;
                arm64_table.emit_div = arm64_emit_div;
                arm64_table.emit_libc_call = arm64_emit_libc_call;
                arm64_table.emit_function_prologue = arm64_emit_function_prologue;
                arm64_table.emit_function_epilogue = arm64_emit_function_epilogue;
            }
            return &arm64_table;

        case ARCH_ARM32:
            // TODO: 实现ARM32支持
            printf("Warning: ARM32 architecture not implemented, using x64 fallback\n");
            return get_arch_codegen_table(ARCH_X86_64);

        default:
            printf("Warning: Unknown architecture, using x64 fallback\n");
            return get_arch_codegen_table(ARCH_X86_64);
    }
}

// ASTC JIT编译器 - 将ASTC字节码指令翻译成二进制机器码
// 使用架构特定的codegen函数，支持跨平台
void compile_astc_instruction_to_machine_code(CodeGen* gen, uint8_t opcode, uint8_t* operands, size_t operand_len) {
    ArchCodegenTable* table = get_arch_codegen_table(gen->target_arch);

    switch (opcode) {
        case 0x00: // NOP
            table->emit_nop(gen);
            break;

        case 0x01: // HALT
            table->emit_halt(gen);
            break;

        case 0x10: // CONST_I32
            if (operand_len >= 4) {
                uint32_t value = *(uint32_t*)operands;
                table->emit_const_i32(gen, value);
            }
            break;

        case 0x20: // ADD
            table->emit_add(gen);
            break;

        case 0x21: // SUB
            table->emit_sub(gen);
            break;

        case 0x22: // MUL
            table->emit_mul(gen);
            break;

        case 0x23: // DIV
            table->emit_div(gen);
            break;

        case 0x12: // CONST_STRING
            // 字符串常量指令 - 将字符串地址压入栈
            if (operand_len >= 4) {
                uint32_t str_len = *(uint32_t*)operands;
                // 简化实现：将字符串数据地址压入栈
                table->emit_const_i32(gen, (uint32_t)(uintptr_t)(operands + 4));
            }
            break;

        case 0x30: // STORE_LOCAL
            // 存储到局部变量
            table->emit_nop(gen); // 简化实现
            break;

        case 0x31: // LOAD_LOCAL
            // 加载局部变量
            table->emit_nop(gen); // 简化实现
            break;

        case 0x40: // JUMP
            // 无条件跳转
            table->emit_nop(gen); // 简化实现
            break;

        case 0x41: // JUMP_IF_FALSE
            // 条件跳转
            table->emit_nop(gen); // 简化实现
            break;

        case 0x50: // CALL_USER
            // 用户函数调用
            table->emit_nop(gen); // 简化实现
            break;

        case 0xF0: // LIBC_CALL
            if (operand_len >= 4) {
                uint16_t func_id = *(uint16_t*)operands;
                uint16_t arg_count = *(uint16_t*)(operands + 2);
                table->emit_libc_call(gen, func_id, arg_count);
            }
            break;

        default:
            // 未知指令，生成nop
            printf("Warning: Unknown ASTC opcode 0x%02X, generating NOP\n", opcode);
            table->emit_nop(gen);
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
    printf("JIT compiling ASTC bytecode to %s machine code...\n",
           get_architecture_name(gen->target_arch));

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
    ArchCodegenTable* table = get_arch_codegen_table(gen->target_arch);
    table->emit_function_prologue(gen);

    // 尝试反序列化AST
    uint8_t* ast_data = astc_data + 16;
    size_t ast_data_size = astc_size - 16;

    struct ASTNode* ast = c2astc_deserialize(ast_data, ast_data_size);
    if (ast) {
        printf("JIT compiling AST to x64 machine code...\n");
        // 编译AST节点到机器码
        compile_ast_node_to_machine_code(ast, gen);
        ast_free(ast);
    } else {
        printf("Warning: Failed to deserialize AST, trying bytecode mode...\n");
        // 回退到字节码模式
        uint8_t* code = astc_data + 16;
        size_t code_size = astc_size - 16;
        size_t pc = 0;

        while (pc < code_size) {
            uint8_t opcode = code[pc++];

            // 根据指令类型确定操作数长度
            size_t operand_len = 0;
            switch (opcode) {
                case 0x10: operand_len = 4; break; // CONST_I32
                case 0x12: // CONST_STRING - 需要读取长度字段
                    if (pc + 4 <= code_size) {
                        uint32_t str_len = *(uint32_t*)&code[pc];
                        operand_len = 4 + str_len; // 长度字段 + 字符串数据
                    }
                    break;
                case 0x30: operand_len = 4; break; // STORE_LOCAL
                case 0x31: operand_len = 4; break; // LOAD_LOCAL
                case 0x40: operand_len = 4; break; // JUMP
                case 0x41: operand_len = 4; break; // JUMP_IF_FALSE
                case 0x50: operand_len = 4; break; // CALL_USER
                case 0xF0: operand_len = 4; break; // LIBC_CALL
                default: operand_len = 0; break;
            }

            uint8_t* operands = (pc + operand_len <= code_size) ? &code[pc] : NULL;

            // JIT编译这条指令到机器码
            compile_astc_instruction_to_machine_code(gen, opcode, operands, operand_len);

            pc += operand_len;
        }
    }

    // 如果没有显式的HALT，添加默认返回
    table->emit_function_epilogue(gen);

    printf("JIT compilation completed: %zu ASTC bytes → %zu machine code bytes\n",
           ast_data_size, gen->code_size);

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

    // 创建代码生成器（自动检测架构）
    CodeGen* gen = astc_codegen_init(ARCH_UNKNOWN);
    if (!gen) {
        printf("Error: Failed to initialize code generator\n");
        free(astc_data);
        return 1;
    }

    // 使用新的JIT编译器：ASTC字节码 → 目标架构机器码
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

// 编译AST节点到机器码
int compile_ast_node_to_machine_code(struct ASTNode* node, CodeGen* gen) {
    if (!node || !gen) {
        return 1;
    }

    switch (node->type) {
        case ASTC_TRANSLATION_UNIT:
            // 编译翻译单元中的所有声明
            for (int i = 0; i < node->data.translation_unit.declaration_count; i++) {
                compile_ast_node_to_machine_code(node->data.translation_unit.declarations[i], gen);
            }
            break;

        case ASTC_FUNC_DECL:
            // 编译函数声明
            if (node->data.func_decl.has_body) {
                compile_ast_node_to_machine_code(node->data.func_decl.body, gen);
            }
            break;

        case ASTC_COMPOUND_STMT:
            // 编译复合语句中的所有语句
            for (int i = 0; i < node->data.compound_stmt.statement_count; i++) {
                compile_ast_node_to_machine_code(node->data.compound_stmt.statements[i], gen);
            }
            break;

        case ASTC_EXPR_STMT:
            // 编译表达式语句
            compile_ast_node_to_machine_code(node->data.expr_stmt.expr, gen);
            break;

        case ASTC_CALL_EXPR:
            // 编译函数调用表达式
            printf("Found function call expression\n");

            // 检查AST中的libc标记
            if (node->data.call_expr.is_libc_call) {
                printf("Generating LIBC_CALL with ID: 0x%04X, args: %d\n",
                       node->data.call_expr.libc_func_id, node->data.call_expr.arg_count);

                // 生成LIBC_CALL指令
                ArchCodegenTable* table = get_arch_codegen_table(gen->target_arch);
                table->emit_libc_call(gen, node->data.call_expr.libc_func_id, node->data.call_expr.arg_count);
            } else {
                // 普通函数调用
                if (node->data.call_expr.callee &&
                    node->data.call_expr.callee->type == ASTC_EXPR_IDENTIFIER) {
                    const char* func_name = node->data.call_expr.callee->data.identifier.name;
                    printf("Regular function call: %s\n", func_name);
                    // TODO: 处理普通函数调用
                }
            }
            break;

        case ASTC_RETURN_STMT:
            // 编译return语句
            {
                ArchCodegenTable* table = get_arch_codegen_table(gen->target_arch);
                if (node->data.return_stmt.value) {
                    compile_ast_node_to_machine_code(node->data.return_stmt.value, gen);
                    table->emit_halt(gen);
                } else {
                    // 返回0
                    table->emit_const_i32(gen, 0);
                    table->emit_halt(gen);
                }
            }
            break;

        case ASTC_EXPR_CONSTANT:
            // 编译常量表达式
            if (node->data.constant.type == ASTC_TYPE_INT) {
                ArchCodegenTable* table = get_arch_codegen_table(gen->target_arch);
                table->emit_const_i32(gen, (uint32_t)node->data.constant.int_val);
            }
            break;

        default:
            // 其他节点类型暂时忽略
            printf("Ignoring AST node type: %d\n", node->type);
            break;
    }

    return 0;
}