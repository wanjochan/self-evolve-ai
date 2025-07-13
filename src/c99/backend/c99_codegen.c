/**
 * c99_codegen.c - C99 Code Generator Implementation
 */

#include "c99_codegen.h"
#include "../../core/astc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ===============================================
// Code Generation Context Management
// ===============================================

CodegenContext* codegen_create(void) {
    CodegenContext* codegen = malloc(sizeof(CodegenContext));
    if (!codegen) return NULL;
    
    memset(codegen, 0, sizeof(CodegenContext));
    
    // Initialize bytecode buffer
    codegen->bytecode_capacity = 4096;
    codegen->bytecode = malloc(codegen->bytecode_capacity);
    if (!codegen->bytecode) {
        free(codegen);
        return NULL;
    }
    
    // Initialize function table
    codegen->function_capacity = 64;
    codegen->functions = malloc(sizeof(FunctionInfo) * codegen->function_capacity);
    if (!codegen->functions) {
        free(codegen->bytecode);
        free(codegen);
        return NULL;
    }
    
    // Initialize string table
    codegen->string_capacity = 256;
    codegen->string_literals = malloc(sizeof(char*) * codegen->string_capacity);
    if (!codegen->string_literals) {
        free(codegen->functions);
        free(codegen->bytecode);
        free(codegen);
        return NULL;
    }
    
    codegen->optimization_level = 1;
    codegen->debug_info = true;
    
    printf("Codegen: Created code generation context\n");
    
    return codegen;
}

void codegen_destroy(CodegenContext* codegen) {
    if (!codegen) return;
    
    // Free bytecode buffer
    if (codegen->bytecode) {
        free(codegen->bytecode);
    }
    
    // Free function table
    if (codegen->functions) {
        for (size_t i = 0; i < codegen->function_count; i++) {
            if (codegen->functions[i].name) {
                free(codegen->functions[i].name);
            }
        }
        free(codegen->functions);
    }
    
    // Free string literals
    if (codegen->string_literals) {
        for (size_t i = 0; i < codegen->string_count; i++) {
            if (codegen->string_literals[i]) {
                free(codegen->string_literals[i]);
            }
        }
        free(codegen->string_literals);
    }
    
    free(codegen);
}

// ===============================================
// Bytecode Emission Functions
// ===============================================

static void codegen_ensure_capacity(CodegenContext* codegen, size_t additional) {
    if (codegen->bytecode_size + additional > codegen->bytecode_capacity) {
        while (codegen->bytecode_size + additional > codegen->bytecode_capacity) {
            codegen->bytecode_capacity *= 2;
        }
        codegen->bytecode = realloc(codegen->bytecode, codegen->bytecode_capacity);
    }
}

void codegen_emit_byte(CodegenContext* codegen, uint8_t byte) {
    if (!codegen) return;
    
    codegen_ensure_capacity(codegen, 1);
    codegen->bytecode[codegen->bytecode_size++] = byte;
}

void codegen_emit_i32(CodegenContext* codegen, int32_t value) {
    if (!codegen) return;
    
    codegen_ensure_capacity(codegen, 4);
    
    // Little-endian encoding
    codegen->bytecode[codegen->bytecode_size++] = (uint8_t)(value & 0xFF);
    codegen->bytecode[codegen->bytecode_size++] = (uint8_t)((value >> 8) & 0xFF);
    codegen->bytecode[codegen->bytecode_size++] = (uint8_t)((value >> 16) & 0xFF);
    codegen->bytecode[codegen->bytecode_size++] = (uint8_t)((value >> 24) & 0xFF);
}

void codegen_emit_instruction(CodegenContext* codegen, int instruction) {
    codegen_emit_byte(codegen, (uint8_t)instruction);
}

void codegen_emit_instruction_i32(CodegenContext* codegen, int instruction, int32_t operand) {
    codegen_emit_instruction(codegen, instruction);
    codegen_emit_i32(codegen, operand);
}

// ===============================================
// Code Generation Functions
// ===============================================

bool codegen_generate(CodegenContext* codegen, struct ASTNode* ast) {
    if (!codegen || !ast) return false;
    
    printf("Codegen: Starting code generation\n");
    
    // Emit ASTC header compatible with simple_loader VM
    codegen_emit_byte(codegen, 'A');
    codegen_emit_byte(codegen, 'S');
    codegen_emit_byte(codegen, 'T');
    codegen_emit_byte(codegen, 'C');
    codegen_emit_i32(codegen, 1);  // version
    codegen_emit_i32(codegen, 0);  // flags
    codegen_emit_i32(codegen, 0);  // entry_point
    codegen_emit_i32(codegen, 0);  // source_size (no source included)
    
    // Generate code for translation unit
    bool result = codegen_translation_unit(codegen, ast);
    
    if (result) {
        printf("Codegen: Generated %zu bytes of bytecode\n", codegen->bytecode_size);
    } else {
        printf("Codegen: Code generation failed\n");
    }
    
    return result;
}

bool codegen_translation_unit(CodegenContext* codegen, struct ASTNode* ast) {
    if (!codegen || !ast) return false;

    printf("Codegen: Generating translation unit\n");

    // Check if this is actually a translation unit
    if (ast->type != ASTC_TRANSLATION_UNIT) {
        codegen_error(codegen, ast, "Expected translation unit");
        return false;
    }

    // Process all declarations in the translation unit
    for (int i = 0; i < ast->data.translation_unit.declaration_count; i++) {
        struct ASTNode* decl = ast->data.translation_unit.declarations[i];
        if (!decl) continue;

        switch (decl->type) {
            case ASTC_FUNC_DECL:
                if (!codegen_function_definition(codegen, decl)) {
                    return false;
                }
                break;
            case ASTC_VAR_DECL:
                // TODO: Handle global variable declarations
                printf("Codegen: Skipping global variable declaration\n");
                break;
            default:
                printf("Codegen: Skipping unknown declaration type %d\n", decl->type);
                break;
        }
    }

    return true;
}

bool codegen_function_definition(CodegenContext* codegen, struct ASTNode* func) {
    if (!codegen || !func) return false;

    if (func->type != ASTC_FUNC_DECL) {
        codegen_error(codegen, func, "Expected function declaration");
        return false;
    }

    const char* func_name = func->data.func_decl.name ? func->data.func_decl.name : "anonymous";
    printf("Codegen: Generating function '%s'\n", func_name);

    // Register function
    FunctionInfo* func_info = codegen_register_function(codegen, func_name);
    if (!func_info) return false;

    func_info->bytecode_offset = codegen->bytecode_size;

    // Emit function start
    codegen_emit_instruction(codegen, 0x01); // FUNC_START
    codegen_emit_i32(codegen, func_info->function_id);

    // Generate function body if present
    if (func->data.func_decl.has_body && func->data.func_decl.body) {
        if (!codegen_statement(codegen, func->data.func_decl.body)) {
            return false;
        }
    } else {
        // Empty function body - just return
        codegen_emit_instruction(codegen, 0x10); // LOAD_CONST
        codegen_emit_i32(codegen, 0); // Value 0
        codegen_emit_instruction(codegen, 0x20); // RETURN
    }

    // Emit function end
    codegen_emit_instruction(codegen, 0x02); // FUNC_END

    return true;
}

bool codegen_statement(CodegenContext* codegen, struct ASTNode* stmt) {
    if (!codegen || !stmt) return false;

    printf("Codegen: Generating statement type %d\n", stmt->type);

    switch (stmt->type) {
        case ASTC_COMPOUND_STMT:
            return codegen_compound_statement(codegen, stmt);
        case ASTC_RETURN_STMT:
            return codegen_return_statement(codegen, stmt);
        case ASTC_EXPR_STMT:
            return codegen_expression_statement(codegen, stmt);
        case ASTC_IF_STMT:
            return codegen_if_statement(codegen, stmt);
        case ASTC_WHILE_STMT:
            return codegen_while_statement(codegen, stmt);
        case ASTC_FOR_STMT:
            return codegen_for_statement(codegen, stmt);
        default:
            printf("Codegen: Unsupported statement type %d\n", stmt->type);
            return true; // Skip unsupported statements for now
    }
}

bool codegen_expression(CodegenContext* codegen, struct ASTNode* expr) {
    if (!codegen || !expr) return false;

    printf("Codegen: Generating expression type %d\n", expr->type);

    switch (expr->type) {
        case ASTC_EXPR_CONSTANT:
            return codegen_constant_expression(codegen, expr);
        case ASTC_EXPR_IDENTIFIER:
            return codegen_identifier_expression(codegen, expr);
        case ASTC_BINARY_OP:
            return codegen_binary_operation(codegen, expr);
        case ASTC_UNARY_OP:
            return codegen_unary_operation(codegen, expr);
        case ASTC_CALL_EXPR:
            return codegen_call_expression(codegen, expr);
        default:
            printf("Codegen: Unsupported expression type %d\n", expr->type);
            return true; // Skip unsupported expressions for now
    }
}

// ===============================================
// Statement Generation Functions
// ===============================================

bool codegen_compound_statement(CodegenContext* codegen, struct ASTNode* stmt) {
    if (!codegen || !stmt || stmt->type != ASTC_COMPOUND_STMT) return false;

    printf("Codegen: Generating compound statement\n");

    // Generate all statements in the compound
    for (int i = 0; i < stmt->data.compound_stmt.statement_count; i++) {
        struct ASTNode* sub_stmt = stmt->data.compound_stmt.statements[i];
        if (sub_stmt && !codegen_statement(codegen, sub_stmt)) {
            return false;
        }
    }

    return true;
}

bool codegen_return_statement(CodegenContext* codegen, struct ASTNode* stmt) {
    if (!codegen || !stmt || stmt->type != ASTC_RETURN_STMT) return false;

    printf("Codegen: Generating return statement\n");

    // Generate return value if present
    if (stmt->data.return_stmt.value) {
        if (!codegen_expression(codegen, stmt->data.return_stmt.value)) {
            return false;
        }
    } else {
        // Return void/0
        codegen_emit_instruction(codegen, 0x10); // LOAD_CONST
        codegen_emit_i32(codegen, 0);
    }

    codegen_emit_instruction(codegen, 0x20); // RETURN
    return true;
}

bool codegen_expression_statement(CodegenContext* codegen, struct ASTNode* stmt) {
    if (!codegen || !stmt || stmt->type != ASTC_EXPR_STMT) return false;

    printf("Codegen: Generating expression statement\n");

    // Generate the expression
    if (stmt->data.expr_stmt.expr) {
        if (!codegen_expression(codegen, stmt->data.expr_stmt.expr)) {
            return false;
        }
        // Pop the result since it's not used
        codegen_emit_instruction(codegen, 0x1A); // DROP
    }

    return true;
}

bool codegen_if_statement(CodegenContext* codegen, struct ASTNode* stmt) {
    if (!codegen || !stmt || stmt->type != ASTC_IF_STMT) return false;

    printf("Codegen: Generating if statement\n");

    // Generate condition
    if (!codegen_expression(codegen, stmt->data.if_stmt.condition)) {
        return false;
    }

    // Emit conditional branch
    codegen_emit_instruction(codegen, 0x04); // BR_IF
    size_t else_label = codegen->bytecode_size;
    codegen_emit_i32(codegen, 0); // Placeholder for else branch offset

    // Generate then branch
    if (!codegen_statement(codegen, stmt->data.if_stmt.then_branch)) {
        return false;
    }

    // Generate else branch if present
    if (stmt->data.if_stmt.else_branch) {
        codegen_emit_instruction(codegen, 0x0C); // BR (unconditional jump to end)
        size_t end_label = codegen->bytecode_size;
        codegen_emit_i32(codegen, 0); // Placeholder for end offset

        // Update else branch offset
        int32_t else_offset = (int32_t)(codegen->bytecode_size - else_label - 4);
        memcpy(&codegen->bytecode[else_label], &else_offset, 4);

        if (!codegen_statement(codegen, stmt->data.if_stmt.else_branch)) {
            return false;
        }

        // Update end offset
        int32_t end_offset = (int32_t)(codegen->bytecode_size - end_label - 4);
        memcpy(&codegen->bytecode[end_label], &end_offset, 4);
    } else {
        // Update else branch offset to point to end
        int32_t else_offset = (int32_t)(codegen->bytecode_size - else_label - 4);
        memcpy(&codegen->bytecode[else_label], &else_offset, 4);
    }

    return true;
}

bool codegen_while_statement(CodegenContext* codegen, struct ASTNode* stmt) {
    if (!codegen || !stmt || stmt->type != ASTC_WHILE_STMT) return false;

    printf("Codegen: Generating while statement\n");

    size_t loop_start = codegen->bytecode_size;

    // Generate condition
    if (!codegen_expression(codegen, stmt->data.while_stmt.condition)) {
        return false;
    }

    // Emit conditional branch to exit
    codegen_emit_instruction(codegen, 0x04); // BR_IF (branch if false)
    size_t exit_label = codegen->bytecode_size;
    codegen_emit_i32(codegen, 0); // Placeholder for exit offset

    // Generate loop body
    if (!codegen_statement(codegen, stmt->data.while_stmt.body)) {
        return false;
    }

    // Jump back to condition
    codegen_emit_instruction(codegen, 0x0C); // BR (unconditional jump)
    int32_t back_offset = (int32_t)(loop_start - codegen->bytecode_size - 4);
    codegen_emit_i32(codegen, back_offset);

    // Update exit offset
    int32_t exit_offset = (int32_t)(codegen->bytecode_size - exit_label - 4);
    memcpy(&codegen->bytecode[exit_label], &exit_offset, 4);

    return true;
}

bool codegen_for_statement(CodegenContext* codegen, struct ASTNode* stmt) {
    if (!codegen || !stmt || stmt->type != ASTC_FOR_STMT) return false;

    printf("Codegen: Generating for statement\n");

    // Generate initialization
    if (stmt->data.for_stmt.init) {
        if (!codegen_statement(codegen, stmt->data.for_stmt.init)) {
            return false;
        }
    }

    size_t loop_start = codegen->bytecode_size;

    // Generate condition
    if (stmt->data.for_stmt.condition) {
        if (!codegen_expression(codegen, stmt->data.for_stmt.condition)) {
            return false;
        }

        // Emit conditional branch to exit
        codegen_emit_instruction(codegen, 0x04); // BR_IF (branch if false)
        size_t exit_label = codegen->bytecode_size;
        codegen_emit_i32(codegen, 0); // Placeholder for exit offset

        // Generate loop body
        if (!codegen_statement(codegen, stmt->data.for_stmt.body)) {
            return false;
        }

        // Generate increment
        if (stmt->data.for_stmt.increment) {
            if (!codegen_expression(codegen, stmt->data.for_stmt.increment)) {
                return false;
            }
            codegen_emit_instruction(codegen, 0x1A); // DROP (discard increment result)
        }

        // Jump back to condition
        codegen_emit_instruction(codegen, 0x0C); // BR (unconditional jump)
        int32_t back_offset = (int32_t)(loop_start - codegen->bytecode_size - 4);
        codegen_emit_i32(codegen, back_offset);

        // Update exit offset
        int32_t exit_offset = (int32_t)(codegen->bytecode_size - exit_label - 4);
        memcpy(&codegen->bytecode[exit_label], &exit_offset, 4);
    } else {
        // Infinite loop (no condition)
        if (!codegen_statement(codegen, stmt->data.for_stmt.body)) {
            return false;
        }

        if (stmt->data.for_stmt.increment) {
            if (!codegen_expression(codegen, stmt->data.for_stmt.increment)) {
                return false;
            }
            codegen_emit_instruction(codegen, 0x1A); // DROP
        }

        codegen_emit_instruction(codegen, 0x0C); // BR (unconditional jump back)
        int32_t back_offset = (int32_t)(loop_start - codegen->bytecode_size - 4);
        codegen_emit_i32(codegen, back_offset);
    }

    return true;
}

// ===============================================
// Function Management
// ===============================================

FunctionInfo* codegen_register_function(CodegenContext* codegen, const char* name) {
    if (!codegen || !name) return NULL;
    
    // Expand function table if needed
    if (codegen->function_count >= codegen->function_capacity) {
        codegen->function_capacity *= 2;
        codegen->functions = realloc(codegen->functions, 
                                   sizeof(FunctionInfo) * codegen->function_capacity);
    }
    
    FunctionInfo* func = &codegen->functions[codegen->function_count];
    memset(func, 0, sizeof(FunctionInfo));
    
    func->name = strdup(name);
    func->function_id = (int)codegen->function_count;
    func->is_main = (strcmp(name, "main") == 0);
    
    codegen->function_count++;
    
    printf("Codegen: Registered function '%s' with ID %d\n", name, func->function_id);
    
    return func;
}

FunctionInfo* codegen_find_function(CodegenContext* codegen, const char* name) {
    if (!codegen || !name) return NULL;
    
    for (size_t i = 0; i < codegen->function_count; i++) {
        if (strcmp(codegen->functions[i].name, name) == 0) {
            return &codegen->functions[i];
        }
    }
    
    return NULL;
}

// ===============================================
// Stack Management
// ===============================================

void codegen_push_stack(CodegenContext* codegen) {
    if (!codegen) return;
    
    codegen->stack_depth++;
    if (codegen->stack_depth > codegen->max_stack_depth) {
        codegen->max_stack_depth = codegen->stack_depth;
    }
}

void codegen_pop_stack(CodegenContext* codegen) {
    if (!codegen || codegen->stack_depth <= 0) return;
    
    codegen->stack_depth--;
}

// ===============================================
// Error Handling
// ===============================================

void codegen_error(CodegenContext* codegen, struct ASTNode* node, const char* message) {
    if (!codegen) return;
    
    codegen->has_error = true;
    codegen->error_count++;
    
    int line = node ? 0 : 0; // TODO: Get line from node
    int column = node ? 0 : 0; // TODO: Get column from node
    
    snprintf(codegen->error_message, sizeof(codegen->error_message),
             "Codegen error at line %d, column %d: %s", line, column, message);
    
    printf("Codegen Error: %s\n", codegen->error_message);
}

bool codegen_has_error(CodegenContext* codegen) {
    return codegen && codegen->has_error;
}

const char* codegen_get_error(CodegenContext* codegen) {
    return codegen ? codegen->error_message : "Invalid codegen context";
}

// ===============================================
// Output Functions
// ===============================================

uint8_t* codegen_get_bytecode(CodegenContext* codegen, size_t* size) {
    if (!codegen || !size) return NULL;
    
    *size = codegen->bytecode_size;
    return codegen->bytecode;
}

bool codegen_write_to_file(CodegenContext* codegen, const char* filename) {
    if (!codegen || !filename) return false;
    
    FILE* file = fopen(filename, "wb");
    if (!file) {
        codegen_error(codegen, NULL, "Failed to open output file");
        return false;
    }
    
    // The bytecode buffer already contains the ASTC header (20 bytes) + actual bytecode
    // We need to add the bytecode_size field after the header and before the actual bytecode
    
    // Calculate actual bytecode size (total - header size)
    uint32_t header_size = 20;  // ASTC header size
    uint32_t actual_bytecode_size = codegen->bytecode_size - header_size;
    
    // Write ASTC header (first 20 bytes of bytecode buffer)
    size_t written = fwrite(codegen->bytecode, 1, header_size, file);
    if (written != header_size) {
        codegen_error(codegen, NULL, "Failed to write ASTC header");
        fclose(file);
        return false;
    }
    
    // Write bytecode size
    written = fwrite(&actual_bytecode_size, sizeof(uint32_t), 1, file);
    if (written != 1) {
        codegen_error(codegen, NULL, "Failed to write bytecode size");
        fclose(file);
        return false;
    }
    
    // Write actual bytecode data (everything after the header)
    written = fwrite(codegen->bytecode + header_size, 1, actual_bytecode_size, file);
    if (written != actual_bytecode_size) {
        codegen_error(codegen, NULL, "Failed to write bytecode data");
        fclose(file);
        return false;
    }
    
    fclose(file);
    
    printf("Codegen: Wrote %zu bytes to %s\n", codegen->bytecode_size + sizeof(uint32_t), filename);
    return true;
}

void codegen_print_stats(CodegenContext* codegen) {
    if (!codegen) return;

    printf("Code Generation Statistics:\n");
    printf("  Bytecode size: %zu bytes\n", codegen->bytecode_size);
    printf("  Functions: %zu\n", codegen->function_count);
    printf("  String literals: %zu\n", codegen->string_count);
    printf("  Max stack depth: %d\n", codegen->max_stack_depth);
    printf("  Optimization level: %d\n", codegen->optimization_level);
    printf("  Errors: %d\n", codegen->error_count);
}

// ===============================================
// Enhanced Expression Generation Functions
// ===============================================

bool codegen_constant_expression(CodegenContext* codegen, struct ASTNode* expr) {
    if (!codegen || !expr || expr->type != ASTC_EXPR_CONSTANT) return false;

    printf("Codegen: Generating constant expression\n");

    // Load constant value
    codegen_emit_instruction(codegen, 0x10); // LOAD_CONST

    switch (expr->data.constant.type) {
        case ASTC_TYPE_INT:
            codegen_emit_i32(codegen, (int32_t)expr->data.constant.int_val);
            break;
        case ASTC_TYPE_FLOAT:
            // For simplicity, convert float to int for now
            codegen_emit_i32(codegen, (int32_t)expr->data.constant.float_val);
            break;
        default:
            codegen_emit_i32(codegen, 0);
            break;
    }

    codegen_push_stack(codegen);
    return true;
}

bool codegen_identifier_expression(CodegenContext* codegen, struct ASTNode* expr) {
    if (!codegen || !expr || expr->type != ASTC_EXPR_IDENTIFIER) return false;

    printf("Codegen: Generating identifier expression '%s'\n",
           expr->data.identifier.name ? expr->data.identifier.name : "unknown");

    // For now, just load 0 (TODO: implement variable lookup)
    codegen_emit_instruction(codegen, 0x10); // LOAD_CONST
    codegen_emit_i32(codegen, 0);

    codegen_push_stack(codegen);
    return true;
}

bool codegen_binary_operation(CodegenContext* codegen, struct ASTNode* expr) {
    if (!codegen || !expr || expr->type != ASTC_BINARY_OP) return false;

    printf("Codegen: Generating binary operation\n");

    // Generate left operand
    if (!codegen_expression(codegen, expr->data.binary_op.left)) {
        return false;
    }

    // Generate right operand
    if (!codegen_expression(codegen, expr->data.binary_op.right)) {
        return false;
    }

    // Generate operation
    switch (expr->data.binary_op.op) {
        case ASTC_OP_ADD:
            codegen_emit_instruction(codegen, 0x6A); // i32.add
            break;
        case ASTC_OP_SUB:
            codegen_emit_instruction(codegen, 0x6B); // i32.sub
            break;
        case ASTC_OP_MUL:
            codegen_emit_instruction(codegen, 0x6C); // i32.mul
            break;
        case ASTC_OP_DIV:
            codegen_emit_instruction(codegen, 0x6D); // i32.div_s
            break;
        case ASTC_OP_MOD:
            codegen_emit_instruction(codegen, 0x6F); // i32.rem_s
            break;
        case ASTC_OP_EQ:
            codegen_emit_instruction(codegen, 0x46); // i32.eq
            break;
        case ASTC_OP_NE:
            codegen_emit_instruction(codegen, 0x47); // i32.ne
            break;
        case ASTC_OP_LT:
            codegen_emit_instruction(codegen, 0x48); // i32.lt_s
            break;
        case ASTC_OP_LE:
            codegen_emit_instruction(codegen, 0x4C); // i32.le_s
            break;
        case ASTC_OP_GT:
            codegen_emit_instruction(codegen, 0x4A); // i32.gt_s
            break;
        case ASTC_OP_GE:
            codegen_emit_instruction(codegen, 0x4E); // i32.ge_s
            break;
        default:
            printf("Codegen: Unsupported binary operation %d\n", expr->data.binary_op.op);
            return false;
    }

    codegen_pop_stack(codegen); // Two operands consumed, one result produced
    return true;
}

bool codegen_unary_operation(CodegenContext* codegen, struct ASTNode* expr) {
    if (!codegen || !expr || expr->type != ASTC_UNARY_OP) return false;

    printf("Codegen: Generating unary operation\n");

    // Generate operand
    if (!codegen_expression(codegen, expr->data.unary_op.operand)) {
        return false;
    }

    // Generate operation
    switch (expr->data.unary_op.op) {
        case ASTC_OP_NEG:
            // Negate: 0 - operand
            codegen_emit_instruction(codegen, 0x10); // LOAD_CONST
            codegen_emit_i32(codegen, 0);
            codegen_push_stack(codegen);
            // Swap operands
            codegen_emit_instruction(codegen, 0x6B); // i32.sub
            codegen_pop_stack(codegen);
            break;
        case ASTC_OP_NOT:
            // Logical not: operand == 0
            codegen_emit_instruction(codegen, 0x10); // LOAD_CONST
            codegen_emit_i32(codegen, 0);
            codegen_push_stack(codegen);
            codegen_emit_instruction(codegen, 0x46); // i32.eq
            codegen_pop_stack(codegen);
            break;
        default:
            printf("Codegen: Unsupported unary operation %d\n", expr->data.unary_op.op);
            return false;
    }

    return true;
}

bool codegen_call_expression(CodegenContext* codegen, struct ASTNode* expr) {
    if (!codegen || !expr || expr->type != ASTC_CALL_EXPR) return false;

    printf("Codegen: Generating function call\n");

    // Generate arguments
    for (int i = 0; i < expr->data.call_expr.arg_count; i++) {
        if (!codegen_expression(codegen, expr->data.call_expr.args[i])) {
            return false;
        }
    }

    // Generate function call
    if (expr->data.call_expr.is_libc_call) {
        // Call libc function
        codegen_emit_instruction(codegen, 0x11); // CALL_INDIRECT
        codegen_emit_i32(codegen, expr->data.call_expr.libc_func_id);
    } else {
        // Call user function (TODO: implement function lookup)
        codegen_emit_instruction(codegen, 0x10); // CALL
        codegen_emit_i32(codegen, 0); // Function index
    }

    // Adjust stack (args consumed, result produced)
    for (int i = 0; i < expr->data.call_expr.arg_count; i++) {
        codegen_pop_stack(codegen);
    }
    codegen_push_stack(codegen); // Function result

    return true;
}
