/**
 * c99_codegen.c - C99 Code Generator Implementation
 */

#include "c99_codegen.h"
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
    
    // Emit bytecode header
    codegen_emit_byte(codegen, 'A');
    codegen_emit_byte(codegen, 'S');
    codegen_emit_byte(codegen, 'T');
    codegen_emit_byte(codegen, 'C');
    codegen_emit_i32(codegen, 1); // Version
    
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
    
    // TODO: Process all external declarations
    // For now, just emit a simple program structure
    
    // Emit main function
    codegen_emit_instruction(codegen, 0x01); // FUNC_START
    codegen_emit_i32(codegen, 0); // Function ID for main
    
    // Emit return 0
    codegen_emit_instruction(codegen, 0x10); // LOAD_CONST
    codegen_emit_i32(codegen, 0); // Value 0
    
    codegen_emit_instruction(codegen, 0x20); // RETURN
    
    codegen_emit_instruction(codegen, 0x02); // FUNC_END
    
    return true;
}

bool codegen_function_definition(CodegenContext* codegen, struct ASTNode* func) {
    if (!codegen || !func) return false;
    
    printf("Codegen: Generating function definition\n");
    
    // Register function
    FunctionInfo* func_info = codegen_register_function(codegen, "function");
    if (!func_info) return false;
    
    func_info->bytecode_offset = codegen->bytecode_size;
    
    // Emit function start
    codegen_emit_instruction(codegen, 0x01); // FUNC_START
    codegen_emit_i32(codegen, func_info->function_id);
    
    // TODO: Generate function body
    
    // Emit function end
    codegen_emit_instruction(codegen, 0x02); // FUNC_END
    
    return true;
}

bool codegen_statement(CodegenContext* codegen, struct ASTNode* stmt) {
    if (!codegen || !stmt) return false;
    
    printf("Codegen: Generating statement\n");
    
    // TODO: Generate statement based on type
    
    return true;
}

bool codegen_expression(CodegenContext* codegen, struct ASTNode* expr) {
    if (!codegen || !expr) return false;
    
    printf("Codegen: Generating expression\n");
    
    // TODO: Generate expression based on type
    
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
    
    size_t written = fwrite(codegen->bytecode, 1, codegen->bytecode_size, file);
    fclose(file);
    
    if (written != codegen->bytecode_size) {
        codegen_error(codegen, NULL, "Failed to write complete bytecode");
        return false;
    }
    
    printf("Codegen: Wrote %zu bytes to %s\n", codegen->bytecode_size, filename);
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
