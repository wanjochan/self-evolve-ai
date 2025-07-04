/**
 * vm_astc_parser.h - ASTC Bytecode Parser Header
 * 
 * Complete ASTC bytecode parsing and validation interface for VM module.
 */

#ifndef VM_ASTC_PARSER_H
#define VM_ASTC_PARSER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ===============================================
// ASTC Parser Types
// ===============================================

/**
 * Opaque ASTC parser context
 */
typedef struct ASTCParserContext ASTCParserContext;

// ===============================================
// ASTC Parser Functions
// ===============================================

/**
 * Create ASTC parser context from bytecode data
 * @param bytecode Raw bytecode data
 * @param size Bytecode size in bytes
 * @return Parser context or NULL on error
 */
ASTCParserContext* astc_parser_create(uint8_t* bytecode, size_t size);

/**
 * Free ASTC parser context
 * @param ctx Parser context to free
 */
void astc_parser_free(ASTCParserContext* ctx);

/**
 * Get last parser error message
 * @param ctx Parser context
 * @return Error message string
 */
const char* astc_parser_get_error(ASTCParserContext* ctx);

/**
 * Parse ASTC header section
 * @param ctx Parser context
 * @return true on success, false on error
 */
bool astc_parser_parse_header(ASTCParserContext* ctx);

/**
 * Parse ASTC bytecode section
 * @param ctx Parser context
 * @param bytecode_out Output bytecode buffer (caller must free)
 * @param bytecode_size_out Output bytecode size
 * @return true on success, false on error
 */
bool astc_parser_parse_bytecode(ASTCParserContext* ctx, uint8_t** bytecode_out, size_t* bytecode_size_out);

/**
 * Validate ASTC bytecode structure
 * @param bytecode Bytecode to validate
 * @param size Bytecode size
 * @return true if valid, false if invalid
 */
bool astc_parser_validate_bytecode(uint8_t* bytecode, size_t size);

/**
 * Parse complete ASTC file
 * @param filename ASTC file path
 * @param bytecode_out Output bytecode buffer (caller must free)
 * @param bytecode_size_out Output bytecode size
 * @return true on success, false on error
 */
bool astc_parser_parse_file(const char* filename, uint8_t** bytecode_out, size_t* bytecode_size_out);

// ===============================================
// ASTC Instruction Opcodes (from astc.h)
// ===============================================

#define ASTC_HALT           0x01    // Halt execution
#define ASTC_LOAD_IMM32     0x10    // Load 32-bit immediate
#define ASTC_ADD            0x20    // Add two registers
#define ASTC_CALL           0x30    // Call function
#define ASTC_EXIT           0x40    // Exit with code

// ===============================================
// ASTC Parser Utilities
// ===============================================

/**
 * Get ASTC instruction name from opcode
 * @param opcode Instruction opcode
 * @return Instruction name string
 */
const char* astc_parser_get_instruction_name(uint8_t opcode);

/**
 * Get ASTC instruction parameter count
 * @param opcode Instruction opcode
 * @return Number of bytes for instruction parameters
 */
int astc_parser_get_instruction_param_count(uint8_t opcode);

/**
 * Disassemble ASTC bytecode to human-readable format
 * @param bytecode Bytecode to disassemble
 * @param size Bytecode size
 * @param output Output buffer
 * @param output_size Output buffer size
 * @return true on success, false on error
 */
bool astc_parser_disassemble(uint8_t* bytecode, size_t size, char* output, size_t output_size);

#endif // VM_ASTC_PARSER_H
