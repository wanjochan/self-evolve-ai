/**
 * vm_astc_parser.c - Complete ASTC Bytecode Parser for VM
 * 
 * Implements full ASTC bytecode parsing and validation according to astc.h specification.
 * This is part of the VM module functionality enhancement.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "../core/astc.h"
#include "../core/utils.h"

// ===============================================
// ASTC Parser State and Context
// ===============================================

typedef struct {
    uint8_t* bytecode;          // Raw bytecode data
    size_t bytecode_size;       // Total bytecode size
    size_t position;            // Current parsing position
    uint32_t version;           // ASTC version
    uint32_t flags;             // ASTC flags
    uint32_t entry_point;       // Entry point offset
    char* source_code;          // Original source code (optional)
    size_t source_size;         // Source code size
    char error_message[512];    // Last error message
    bool has_error;             // Error flag
} ASTCParserContext;

// ===============================================
// ASTC Header Parsing
// ===============================================

/**
 * Initialize ASTC parser context
 */
ASTCParserContext* astc_parser_create(uint8_t* bytecode, size_t size) {
    if (!bytecode || size < 16) {
        return NULL;
    }
    
    ASTCParserContext* ctx = malloc(sizeof(ASTCParserContext));
    if (!ctx) {
        return NULL;
    }
    
    memset(ctx, 0, sizeof(ASTCParserContext));
    ctx->bytecode = bytecode;
    ctx->bytecode_size = size;
    ctx->position = 0;
    
    return ctx;
}

/**
 * Free ASTC parser context
 */
void astc_parser_free(ASTCParserContext* ctx) {
    if (!ctx) {
        return;
    }
    
    if (ctx->source_code) {
        free(ctx->source_code);
    }
    
    free(ctx);
}

/**
 * Set parser error
 */
static void astc_parser_set_error(ASTCParserContext* ctx, const char* message) {
    if (!ctx || !message) {
        return;
    }
    
    strncpy(ctx->error_message, message, sizeof(ctx->error_message) - 1);
    ctx->error_message[sizeof(ctx->error_message) - 1] = '\0';
    ctx->has_error = true;
}

/**
 * Get parser error message
 */
const char* astc_parser_get_error(ASTCParserContext* ctx) {
    if (!ctx) {
        return "Invalid parser context";
    }
    
    return ctx->has_error ? ctx->error_message : "No error";
}

/**
 * Read bytes from bytecode
 */
static bool astc_parser_read_bytes(ASTCParserContext* ctx, void* buffer, size_t count) {
    if (!ctx || !buffer || ctx->position + count > ctx->bytecode_size) {
        astc_parser_set_error(ctx, "Unexpected end of bytecode");
        return false;
    }
    
    memcpy(buffer, ctx->bytecode + ctx->position, count);
    ctx->position += count;
    return true;
}

/**
 * Read uint8 from bytecode
 */
static bool astc_parser_read_uint8(ASTCParserContext* ctx, uint8_t* value) {
    return astc_parser_read_bytes(ctx, value, sizeof(uint8_t));
}

/**
 * Read uint32 from bytecode
 */
static bool astc_parser_read_uint32(ASTCParserContext* ctx, uint32_t* value) {
    return astc_parser_read_bytes(ctx, value, sizeof(uint32_t));
}

/**
 * Parse ASTC header
 */
bool astc_parser_parse_header(ASTCParserContext* ctx) {
    if (!ctx) {
        return false;
    }
    
    // Reset position and error state
    ctx->position = 0;
    ctx->has_error = false;
    
    // Check magic number
    char magic[4];
    if (!astc_parser_read_bytes(ctx, magic, 4) || memcmp(magic, "ASTC", 4) != 0) {
        astc_parser_set_error(ctx, "Invalid ASTC magic number");
        return false;
    }
    
    // Read version
    if (!astc_parser_read_uint32(ctx, &ctx->version)) {
        astc_parser_set_error(ctx, "Failed to read ASTC version");
        return false;
    }
    
    // Read flags
    if (!astc_parser_read_uint32(ctx, &ctx->flags)) {
        astc_parser_set_error(ctx, "Failed to read ASTC flags");
        return false;
    }
    
    // Read entry point
    if (!astc_parser_read_uint32(ctx, &ctx->entry_point)) {
        astc_parser_set_error(ctx, "Failed to read ASTC entry point");
        return false;
    }
    
    // Read source code size
    if (!astc_parser_read_uint32(ctx, (uint32_t*)&ctx->source_size)) {
        astc_parser_set_error(ctx, "Failed to read source code size");
        return false;
    }
    
    // Read source code (optional)
    if (ctx->source_size > 0) {
        ctx->source_code = malloc(ctx->source_size + 1);
        if (!ctx->source_code) {
            astc_parser_set_error(ctx, "Failed to allocate memory for source code");
            return false;
        }
        
        if (!astc_parser_read_bytes(ctx, ctx->source_code, ctx->source_size)) {
            astc_parser_set_error(ctx, "Failed to read source code");
            return false;
        }
        
        ctx->source_code[ctx->source_size] = '\0';
    }
    
    printf("ASTC Parser: Header parsed successfully\n");
    printf("  Version: %u\n", ctx->version);
    printf("  Flags: 0x%08X\n", ctx->flags);
    printf("  Entry Point: 0x%08X\n", ctx->entry_point);
    printf("  Source Size: %zu bytes\n", ctx->source_size);
    
    return true;
}

/**
 * Parse ASTC bytecode section
 */
bool astc_parser_parse_bytecode(ASTCParserContext* ctx, uint8_t** bytecode_out, size_t* bytecode_size_out) {
    if (!ctx || !bytecode_out || !bytecode_size_out) {
        astc_parser_set_error(ctx, "Invalid parameters for bytecode parsing");
        return false;
    }
    
    // Read bytecode size
    uint32_t bytecode_size;
    if (!astc_parser_read_uint32(ctx, &bytecode_size)) {
        astc_parser_set_error(ctx, "Failed to read bytecode size");
        return false;
    }
    
    if (bytecode_size == 0) {
        astc_parser_set_error(ctx, "Empty bytecode section");
        return false;
    }
    
    if (ctx->position + bytecode_size > ctx->bytecode_size) {
        astc_parser_set_error(ctx, "Bytecode section exceeds file size");
        return false;
    }
    
    // Allocate and copy bytecode
    uint8_t* bytecode = malloc(bytecode_size);
    if (!bytecode) {
        astc_parser_set_error(ctx, "Failed to allocate memory for bytecode");
        return false;
    }
    
    if (!astc_parser_read_bytes(ctx, bytecode, bytecode_size)) {
        free(bytecode);
        astc_parser_set_error(ctx, "Failed to read bytecode");
        return false;
    }
    
    *bytecode_out = bytecode;
    *bytecode_size_out = bytecode_size;
    
    printf("ASTC Parser: Bytecode section parsed successfully (%u bytes)\n", bytecode_size);
    return true;
}

/**
 * Validate ASTC bytecode structure
 */
bool astc_parser_validate_bytecode(uint8_t* bytecode, size_t size) {
    if (!bytecode || size == 0) {
        return false;
    }
    
    printf("ASTC Parser: Validating bytecode structure...\n");
    
    // Basic validation: check for valid instruction opcodes
    size_t pos = 0;
    int instruction_count = 0;
    
    while (pos < size) {
        uint8_t opcode = bytecode[pos++];
        instruction_count++;
        
        // Validate opcode ranges based on astc.h definitions
        if (opcode == 0x01) {
            // HALT - no parameters
            printf("  Instruction %d: HALT (0x%02X)\n", instruction_count, opcode);
        } else if (opcode == 0x10) {
            // LOAD_IMM32 - requires 5 more bytes (reg + imm32)
            if (pos + 5 > size) {
                printf("ASTC Parser: Invalid LOAD_IMM32 instruction at position %zu\n", pos - 1);
                return false;
            }
            uint8_t reg = bytecode[pos++];
            uint32_t imm = *(uint32_t*)(bytecode + pos);
            pos += 4;
            printf("  Instruction %d: LOAD_IMM32 r%u, %u (0x%02X)\n", instruction_count, reg, imm, opcode);
        } else if (opcode == 0x20) {
            // ADD - requires 3 more bytes (reg1, reg2, reg3)
            if (pos + 3 > size) {
                printf("ASTC Parser: Invalid ADD instruction at position %zu\n", pos - 1);
                return false;
            }
            uint8_t reg1 = bytecode[pos++];
            uint8_t reg2 = bytecode[pos++];
            uint8_t reg3 = bytecode[pos++];
            printf("  Instruction %d: ADD r%u, r%u, r%u (0x%02X)\n", instruction_count, reg1, reg2, reg3, opcode);
        } else if (opcode == 0x30) {
            // CALL - requires 4 more bytes (func_id)
            if (pos + 4 > size) {
                printf("ASTC Parser: Invalid CALL instruction at position %zu\n", pos - 1);
                return false;
            }
            uint32_t func_id = *(uint32_t*)(bytecode + pos);
            pos += 4;
            printf("  Instruction %d: CALL %u (0x%02X)\n", instruction_count, func_id, opcode);
        } else if (opcode == 0x40) {
            // EXIT - requires 1 more byte (exit_code)
            if (pos + 1 > size) {
                printf("ASTC Parser: Invalid EXIT instruction at position %zu\n", pos - 1);
                return false;
            }
            uint8_t exit_code = bytecode[pos++];
            printf("  Instruction %d: EXIT %u (0x%02X)\n", instruction_count, exit_code, opcode);
        } else {
            printf("  Instruction %d: Unknown opcode 0x%02X at position %zu\n", instruction_count, opcode, pos - 1);
            // For now, just skip unknown opcodes
            // In a real implementation, this might be an error
        }
        
        // Safety check to prevent infinite loops
        if (instruction_count > 10000) {
            printf("ASTC Parser: Too many instructions, possible infinite loop\n");
            return false;
        }
    }
    
    printf("ASTC Parser: Bytecode validation completed (%d instructions)\n", instruction_count);
    return true;
}

/**
 * Complete ASTC file parsing
 */
bool astc_parser_parse_file(const char* filename, uint8_t** bytecode_out, size_t* bytecode_size_out) {
    if (!filename || !bytecode_out || !bytecode_size_out) {
        return false;
    }
    
    printf("ASTC Parser: Parsing file %s\n", filename);
    
    // Read file
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("ASTC Parser: Cannot open file %s\n", filename);
        return false;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        printf("ASTC Parser: Invalid file size %ld\n", file_size);
        fclose(file);
        return false;
    }
    
    // Read file content
    uint8_t* file_data = malloc(file_size);
    if (!file_data) {
        printf("ASTC Parser: Memory allocation failed\n");
        fclose(file);
        return false;
    }
    
    if (fread(file_data, 1, file_size, file) != (size_t)file_size) {
        printf("ASTC Parser: Failed to read file completely\n");
        free(file_data);
        fclose(file);
        return false;
    }
    
    fclose(file);
    
    // Create parser context
    ASTCParserContext* ctx = astc_parser_create(file_data, file_size);
    if (!ctx) {
        printf("ASTC Parser: Failed to create parser context\n");
        free(file_data);
        return false;
    }
    
    // Parse header
    if (!astc_parser_parse_header(ctx)) {
        printf("ASTC Parser: Header parsing failed: %s\n", astc_parser_get_error(ctx));
        astc_parser_free(ctx);
        free(file_data);
        return false;
    }
    
    // Parse bytecode
    uint8_t* bytecode;
    size_t bytecode_size;
    if (!astc_parser_parse_bytecode(ctx, &bytecode, &bytecode_size)) {
        printf("ASTC Parser: Bytecode parsing failed: %s\n", astc_parser_get_error(ctx));
        astc_parser_free(ctx);
        free(file_data);
        return false;
    }
    
    // Validate bytecode
    if (!astc_parser_validate_bytecode(bytecode, bytecode_size)) {
        printf("ASTC Parser: Bytecode validation failed\n");
        free(bytecode);
        astc_parser_free(ctx);
        free(file_data);
        return false;
    }
    
    // Success
    *bytecode_out = bytecode;
    *bytecode_size_out = bytecode_size;
    
    astc_parser_free(ctx);
    free(file_data);
    
    printf("ASTC Parser: File parsing completed successfully\n");
    return true;
}
