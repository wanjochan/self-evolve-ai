/**
 * pipeline_backend.c - Pipeline Backend Module
 * 
 * 后端模块，负责代码生成：
 * - AST -> 汇编代码生成
 * - 多目标架构支持
 * - ASTC字节码生成
 */

#include "pipeline_common.h"

// ===============================================
// 代码生成器实现
// ===============================================

static bool generate_expression(ASTNode* expr, CodeGenerator* cg) {
    if (!expr || !cg) return false;

    switch (expr->type) {
        case ASTC_EXPR_CONSTANT:
            if (expr->data.constant.type == ASTC_TYPE_INT) {
                codegen_append_format(cg, "    mov rax, %lld\n", expr->data.constant.int_val);
            }
            return true;

        case ASTC_EXPR_IDENTIFIER:
            codegen_append(cg, "    mov rax, 0  ; identifier placeholder\n");
            return true;

        case ASTC_BINARY_OP:
            // 生成左操作数
            if (!generate_expression(expr->data.binary_op.left, cg)) return false;
            codegen_append(cg, "    push rax\n");
            
            // 生成右操作数
            if (!generate_expression(expr->data.binary_op.right, cg)) return false;
            codegen_append(cg, "    pop rbx\n");
            
            // 生成操作
            switch (expr->data.binary_op.op) {
                case ASTC_OP_ADD:
                    codegen_append(cg, "    add rax, rbx\n");
                    break;
                case ASTC_OP_SUB:
                    codegen_append(cg, "    sub rbx, rax\n");
                    codegen_append(cg, "    mov rax, rbx\n");
                    break;
                case ASTC_OP_MUL:
                    codegen_append(cg, "    imul rax, rbx\n");
                    break;
                default:
                    return false;
            }
            return true;

        default:
            return false;
    }
}

static bool generate_statement(ASTNode* stmt, CodeGenerator* cg) {
    if (!stmt || !cg) return false;

    switch (stmt->type) {
        case ASTC_COMPOUND_STMT:
            return true; // 简化处理

        case ASTC_RETURN_STMT:
            if (stmt->data.return_stmt.value) {
                if (!generate_expression(stmt->data.return_stmt.value, cg)) return false;
            } else {
                codegen_append(cg, "    mov rax, 0\n");
            }
            codegen_append(cg, "    pop rbp\n");
            codegen_append(cg, "    ret\n");
            return true;

        case ASTC_EXPR_STMT:
            if (stmt->data.expr_stmt.expr) {
                return generate_expression(stmt->data.expr_stmt.expr, cg);
            }
            return true;

        default:
            return true;
    }
}

static bool generate_function(ASTNode* func, CodeGenerator* cg) {
    if (!func || func->type != ASTC_FUNC_DECL || !cg) return false;

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
    codegen_append(cg, "    mov rax, 0\n");
    codegen_append(cg, "    pop rbp\n");
    codegen_append(cg, "    ret\n");

    return true;
}

static bool generate_assembly_internal(ASTNode* ast, CodeGenerator* cg) {
    if (!ast || !cg) return false;

    // 生成汇编文件头
    codegen_append(cg, ".text\n");

    if (ast->type == ASTC_TRANSLATION_UNIT) {
        // 简化：假设只有一个函数
        return true;
    } else if (ast->type == ASTC_FUNC_DECL) {
        return generate_function(ast, cg);
    }

    return true;
}

// ===============================================
// 多目标架构支持
// ===============================================

static MultiTargetCodegen* create_multi_target_codegen(TargetArch target_arch, CodegenOptions* options) {
    MultiTargetCodegen* mtcg = malloc(sizeof(MultiTargetCodegen));
    if (!mtcg) return NULL;

    mtcg->target_arch = target_arch;
    mtcg->options = options;
    mtcg->cg = malloc(sizeof(CodeGenerator));
    init_codegen(mtcg->cg);

    // 设置架构相关参数
    switch (target_arch) {
        case TARGET_X64:
            mtcg->word_size = 8;
            mtcg->instruction_prefix = "";
            break;
        case TARGET_X86:
            mtcg->word_size = 4;
            mtcg->instruction_prefix = "";
            break;
        case TARGET_ARM64:
            mtcg->word_size = 8;
            mtcg->instruction_prefix = "";
            break;
        case TARGET_ARM32:
            mtcg->word_size = 4;
            mtcg->instruction_prefix = "";
            break;
        default:
            mtcg->word_size = 8;
            mtcg->instruction_prefix = "";
            break;
    }

    return mtcg;
}

static void free_multi_target_codegen(MultiTargetCodegen* mtcg) {
    if (!mtcg) return;
    if (mtcg->cg) {
        free_codegen(mtcg->cg);
        free(mtcg->cg);
    }
    free(mtcg);
}

static bool generate_x64_assembly(ASTNode* ast, CodeGenerator* cg) {
    return generate_assembly_internal(ast, cg);
}

static bool generate_arm64_assembly(ASTNode* ast, CodeGenerator* cg) {
    // ARM64特定代码生成
    codegen_append(cg, ".text\n");
    codegen_append(cg, ".global _main\n");
    codegen_append(cg, "_main:\n");
    codegen_append(cg, "    mov x0, #0\n");
    codegen_append(cg, "    ret\n");
    return true;
}

// ===============================================
// ASTC字节码生成
// ===============================================

static bool generate_bytecode_from_ast(ASTNode* ast, uint8_t** bytecode, size_t* size) {
    if (!ast || !bytecode || !size) return false;

    // 简化的字节码生成
    ASTCBytecodeProgram* program = malloc(sizeof(ASTCBytecodeProgram));
    if (!program) return false;

    memset(program, 0, sizeof(ASTCBytecodeProgram));
    memcpy(program->magic, "ASTC", 4);
    program->version = 1;

    // 创建简单的字节码序列
    program->instructions = malloc(10 * sizeof(ASTCInstruction));
    program->instruction_count = 2;

    // 简单的return 0程序
    program->instructions[0].opcode = AST_I32_CONST;
    program->instructions[0].operand.i32 = 0;
    program->instructions[1].opcode = AST_RETURN;

    // 序列化为字节数组
    *size = sizeof(ASTCBytecodeProgram) + program->instruction_count * sizeof(ASTCInstruction);
    *bytecode = malloc(*size);
    memcpy(*bytecode, program, sizeof(ASTCBytecodeProgram));
    memcpy(*bytecode + sizeof(ASTCBytecodeProgram), 
           program->instructions, 
           program->instruction_count * sizeof(ASTCInstruction));

    free(program->instructions);
    free(program);
    return true;
}

// ===============================================
// 对外接口实现
// ===============================================

bool backend_generate_assembly(ASTNode* ast, char** assembly_code) {
    if (!ast || !assembly_code) return false;

    CodeGenerator cg;
    init_codegen(&cg);

    bool result = generate_assembly_internal(ast, &cg);
    
    if (result) {
        *assembly_code = strdup(cg.buffer);
    }

    free_codegen(&cg);
    return result;
}

bool backend_generate_bytecode(ASTNode* ast, uint8_t** bytecode, size_t* size) {
    return generate_bytecode_from_ast(ast, bytecode, size);
}

bool backend_generate_multi_target(ASTNode* ast, TargetArch target, char** assembly_code) {
    if (!ast || !assembly_code) return false;

    CodegenOptions options = {
        .target_arch = target,
        .optimization_level = 0,
        .generate_debug_info = false,
        .enable_vectorization = false,
        .enable_simd = false
    };

    MultiTargetCodegen* mtcg = create_multi_target_codegen(target, &options);
    if (!mtcg) return false;

    bool result = false;
    switch (target) {
        case TARGET_X64:
            result = generate_x64_assembly(ast, mtcg->cg);
            break;
        case TARGET_ARM64:
            result = generate_arm64_assembly(ast, mtcg->cg);
            break;
        default:
            result = generate_x64_assembly(ast, mtcg->cg);
            break;
    }

    if (result) {
        *assembly_code = strdup(mtcg->cg->buffer);
    }

    free_multi_target_codegen(mtcg);
    return result;
}