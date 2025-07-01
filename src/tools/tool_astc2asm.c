/**
 * tool_astc2asm.c - ASTC to Assembly-like Source Converter
 * 
 * Converts ASTC bytecode to human-readable assembly-like source code
 * for debugging, analysis, and understanding ASTC structure.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../core/include/core_astc.h"
#include "../core/include/logger.h"

// Version information
#define ASTC2ASM_VERSION "1.0.0"

// Assembly output format types
typedef enum {
    ASM_FORMAT_PSEUDO = 0,   // Pseudo-assembly (readable)
    ASM_FORMAT_INTEL = 1,    // Intel x86/x64 syntax
    ASM_FORMAT_AT_T = 2,     // AT&T syntax
    ASM_FORMAT_ARM = 3,      // ARM assembly syntax
    ASM_FORMAT_WASM = 4      // WebAssembly text format
} AsmFormat;

// Output formatting options
typedef struct {
    AsmFormat format;       // Assembly format to use
    int show_addresses;     // Show bytecode addresses
    int show_hex_dump;      // Show hex dump alongside assembly
    int show_comments;      // Show explanatory comments
    int show_types;         // Show data types
    int show_stack_effects; // Show stack effects
    int optimize_output;    // Optimize for readability
    int indent_level;       // Current indentation level
    FILE* output;           // Output file handle
} AsmOutputOptions;

// Enhanced ASTC instruction mapping
typedef struct {
    const char* pseudo_name;    // Pseudo-assembly name
    const char* intel_name;     // Intel syntax equivalent
    const char* att_name;       // AT&T syntax equivalent
    const char* arm_name;       // ARM syntax equivalent
    const char* description;    // Human-readable description
    int stack_effect;           // Net stack effect (-1 = pop, +1 = push, 0 = neutral)
} ASTCInstructionInfo;

// Enhanced instruction mapping table
static const ASTCInstructionInfo astc_instructions[] = {
    [AST_NOP] = {"nop", "nop", "nop", "nop", "No operation", 0},
    [AST_UNREACHABLE] = {"unreachable", "ud2", "ud2", "udf", "Unreachable code", 0},

    // Constants
    [AST_I32_CONST] = {"i32.const", "mov", "movl", "mov", "32-bit integer constant", +1},
    [AST_I64_CONST] = {"i64.const", "mov", "movq", "mov", "64-bit integer constant", +1},
    [AST_F32_CONST] = {"f32.const", "movss", "movss", "vmov.f32", "32-bit float constant", +1},
    [AST_F64_CONST] = {"f64.const", "movsd", "movsd", "vmov.f64", "64-bit float constant", +1},

    // Memory operations
    [AST_I32_LOAD] = {"i32.load", "mov", "movl", "ldr", "Load 32-bit integer", 0},
    [AST_I64_LOAD] = {"i64.load", "mov", "movq", "ldr", "Load 64-bit integer", 0},
    [AST_I32_STORE] = {"i32.store", "mov", "movl", "str", "Store 32-bit integer", -2},
    [AST_I64_STORE] = {"i64.store", "mov", "movq", "str", "Store 64-bit integer", -2},

    // Arithmetic operations
    [AST_I32_ADD] = {"i32.add", "add", "addl", "add", "Add 32-bit integers", -1},
    [AST_I32_SUB] = {"i32.sub", "sub", "subl", "sub", "Subtract 32-bit integers", -1},
    [AST_I32_MUL] = {"i32.mul", "imul", "imull", "mul", "Multiply 32-bit integers", -1},
    [AST_I32_DIV_S] = {"i32.div_s", "idiv", "idivl", "sdiv", "Signed divide 32-bit", -1},
    [AST_I32_DIV_U] = {"i32.div_u", "div", "divl", "udiv", "Unsigned divide 32-bit", -1},

    // Comparison operations
    [AST_I32_EQ] = {"i32.eq", "cmp", "cmpl", "cmp", "Compare equal", -1},
    [AST_I32_NE] = {"i32.ne", "cmp", "cmpl", "cmp", "Compare not equal", -1},
    [AST_I32_LT_S] = {"i32.lt_s", "cmp", "cmpl", "cmp", "Compare less than (signed)", -1},
    [AST_I32_GT_S] = {"i32.gt_s", "cmp", "cmpl", "cmp", "Compare greater than (signed)", -1},

    // Bitwise operations
    [AST_I32_AND] = {"i32.and", "and", "andl", "and", "Bitwise AND", -1},
    [AST_I32_OR] = {"i32.or", "or", "orl", "orr", "Bitwise OR", -1},
    [AST_I32_XOR] = {"i32.xor", "xor", "xorl", "eor", "Bitwise XOR", -1},
    [AST_I32_SHL] = {"i32.shl", "shl", "shll", "lsl", "Shift left", -1},
    [AST_I32_SHR_S] = {"i32.shr_s", "sar", "sarl", "asr", "Arithmetic shift right", -1},
    [AST_I32_SHR_U] = {"i32.shr_u", "shr", "shrl", "lsr", "Logical shift right", -1},

    // Control flow
    [AST_BR] = {"br", "jmp", "jmp", "b", "Unconditional branch", 0},
    [AST_BR_IF] = {"br_if", "jnz", "jnz", "bne", "Conditional branch", -1},
    [AST_RETURN] = {"return", "ret", "ret", "bx lr", "Return from function", 0},
    [AST_CALL] = {"call", "call", "call", "bl", "Function call", 0},
    [AST_CALL_INDIRECT] = {"call_indirect", "call", "call", "blx", "Indirect function call", -1},

    // Stack operations
    [AST_DROP] = {"drop", "pop", "pop", "pop", "Drop top stack value", -1},
    [AST_SELECT] = {"select", "cmov", "cmov", "csel", "Select value based on condition", -1},

    // Extended ASTC instructions
    [ASTC_FUNC_DECL] = {"func", ".func", ".func", ".func", "Function declaration", 0},
    [ASTC_VAR_DECL] = {"local", ".local", ".local", ".local", "Local variable declaration", 0},
    [ASTC_RETURN_STMT] = {"return", "ret", "ret", "bx lr", "Return statement", 0},
};

// Get instruction name based on format
const char* get_instruction_name(ASTNodeType type, AsmFormat format) {
    if (type >= sizeof(astc_instructions) / sizeof(astc_instructions[0])) {
        return "unknown";
    }

    const ASTCInstructionInfo* info = &astc_instructions[type];
    switch (format) {
        case ASM_FORMAT_PSEUDO: return info->pseudo_name;
        case ASM_FORMAT_INTEL: return info->intel_name;
        case ASM_FORMAT_AT_T: return info->att_name;
        case ASM_FORMAT_ARM: return info->arm_name;
        case ASM_FORMAT_WASM: return info->pseudo_name; // Use pseudo for WASM
        default: return info->pseudo_name;
    }
}

// Get instruction description
const char* get_instruction_description(ASTNodeType type) {
    if (type >= sizeof(astc_instructions) / sizeof(astc_instructions[0])) {
        return "Unknown instruction";
    }
    return astc_instructions[type].description;
}

// Get stack effect
int get_stack_effect(ASTNodeType type) {
    if (type >= sizeof(astc_instructions) / sizeof(astc_instructions[0])) {
        return 0;
    }
    return astc_instructions[type].stack_effect;
}

// Function prototypes
void print_usage(const char* program_name);
void print_version(void);
int parse_arguments(int argc, char* argv[], char** input_file, char** output_file, AsmOutputOptions* options);
int disassemble_astc_file(const char* input_file, const char* output_file, AsmOutputOptions* options);
void disassemble_bytecode(const uint8_t* bytecode, size_t size, AsmOutputOptions* options);
void print_instruction(uint8_t opcode, const uint8_t* data, size_t* offset, AsmOutputOptions* options);
void print_address(size_t address, AsmOutputOptions* options);
void print_hex_bytes(const uint8_t* data, size_t count, AsmOutputOptions* options);
void print_indent(AsmOutputOptions* options);
const char* get_opcode_name(uint8_t opcode);

int main(int argc, char* argv[]) {
    char* input_file = NULL;
    char* output_file = NULL;
    AsmOutputOptions options = {
        .show_addresses = 1,
        .show_hex_dump = 0,
        .show_comments = 1,
        .indent_level = 0,
        .output = stdout
    };

    // Parse command line arguments
    int result = parse_arguments(argc, argv, &input_file, &output_file, &options);
    if (result != 0) {
        return result;
    }

    // Disassemble ASTC file
    result = disassemble_astc_file(input_file, output_file, &options);
    
    // Cleanup
    if (options.output != stdout) {
        fclose(options.output);
    }

    return result;
}

void print_usage(const char* program_name) {
    printf("ASTC to Assembly Disassembler v%s\n", ASTC2ASM_VERSION);
    printf("Usage: %s [options] <input.astc> [output.asm]\n", program_name);
    printf("\nOptions:\n");
    printf("  -h, --help         Show this help message\n");
    printf("  -v, --version      Show version information\n");
    printf("  -a, --addresses    Show bytecode addresses (default)\n");
    printf("  -x, --hex          Show hex dump alongside assembly\n");
    printf("  -c, --comments     Show explanatory comments (default)\n");
    printf("  -n, --no-addresses Hide bytecode addresses\n");
    printf("  -q, --quiet        Minimal output (no comments)\n");
    printf("\nExamples:\n");
    printf("  %s program.astc                    # Disassemble to stdout\n", program_name);
    printf("  %s program.astc program.asm        # Disassemble to file\n", program_name);
    printf("  %s -x program.astc                 # Show hex dump\n", program_name);
    printf("  %s -q program.astc                 # Minimal output\n", program_name);
}

void print_version(void) {
    printf("ASTC to Assembly Disassembler v%s\n", ASTC2ASM_VERSION);
    printf("Part of Self-Evolve AI Compiler Toolchain\n");
    printf("Converts ASTC bytecode to human-readable assembly-like source\n");
}

int parse_arguments(int argc, char* argv[], char** input_file, char** output_file, AsmOutputOptions* options) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            return 1;
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--addresses") == 0) {
            options->show_addresses = 1;
        } else if (strcmp(argv[i], "-x") == 0 || strcmp(argv[i], "--hex") == 0) {
            options->show_hex_dump = 1;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--comments") == 0) {
            options->show_comments = 1;
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--no-addresses") == 0) {
            options->show_addresses = 0;
        } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
            options->show_comments = 0;
            options->show_addresses = 0;
        } else if (argv[i][0] != '-') {
            if (*input_file == NULL) {
                *input_file = argv[i];
            } else if (*output_file == NULL) {
                *output_file = argv[i];
            } else {
                fprintf(stderr, "Error: Too many file arguments\n");
                return 1;
            }
        } else {
            fprintf(stderr, "Error: Unknown option: %s\n", argv[i]);
            return 1;
        }
    }

    if (*input_file == NULL) {
        fprintf(stderr, "Error: No input file specified\n");
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}

int disassemble_astc_file(const char* input_file, const char* output_file, AsmOutputOptions* options) {
    // Open input file
    FILE* input = fopen(input_file, "rb");
    if (!input) {
        fprintf(stderr, "Error: Cannot open input file: %s\n", input_file);
        return 1;
    }

    // Open output file if specified
    if (output_file) {
        options->output = fopen(output_file, "w");
        if (!options->output) {
            fprintf(stderr, "Error: Cannot create output file: %s\n", output_file);
            fclose(input);
            return 1;
        }
    }

    // Get file size
    fseek(input, 0, SEEK_END);
    size_t file_size = ftell(input);
    fseek(input, 0, SEEK_SET);

    // Read entire file
    uint8_t* bytecode = malloc(file_size);
    if (!bytecode) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(input);
        if (options->output != stdout) fclose(options->output);
        return 1;
    }

    size_t bytes_read = fread(bytecode, 1, file_size, input);
    fclose(input);

    if (bytes_read != file_size) {
        fprintf(stderr, "Error: Failed to read complete file\n");
        free(bytecode);
        if (options->output != stdout) fclose(options->output);
        return 1;
    }

    // Print header
    fprintf(options->output, "; ASTC Disassembly\n");
    fprintf(options->output, "; Input file: %s\n", input_file);
    fprintf(options->output, "; File size: %zu bytes\n", file_size);
    fprintf(options->output, "; Generated by astc2asm v%s\n", ASTC2ASM_VERSION);
    fprintf(options->output, "\n");

    // Disassemble bytecode
    disassemble_bytecode(bytecode, file_size, options);

    // Cleanup
    free(bytecode);
    return 0;
}

void disassemble_bytecode(const uint8_t* bytecode, size_t size, AsmOutputOptions* options) {
    size_t offset = 0;

    // Check for ASTC header
    if (size >= 16) {
        uint32_t magic = *(uint32_t*)bytecode;
        if (magic == 0x43545341) { // "ASTC" magic
            fprintf(options->output, "; ASTC Header detected\n");
            uint32_t version = *(uint32_t*)(bytecode + 4);
            uint32_t data_size = *(uint32_t*)(bytecode + 8);
            uint32_t entry_point = *(uint32_t*)(bytecode + 12);

            fprintf(options->output, "; Version: %u\n", version);
            fprintf(options->output, "; Data size: %u bytes\n", data_size);
            fprintf(options->output, "; Entry point: 0x%04X\n", entry_point);
            fprintf(options->output, "\n");

            offset = 16; // Skip header
        }
    }

    fprintf(options->output, ".section .text\n");
    fprintf(options->output, ".global _start\n");
    fprintf(options->output, "\n_start:\n");

    // Disassemble instructions
    while (offset < size) {
        uint8_t opcode = bytecode[offset];

        if (opcode == 0xFF) { // END instruction
            print_address(offset, options);
            fprintf(options->output, "    END");
            if (options->show_comments) {
                fprintf(options->output, "                ; Program end");
            }
            fprintf(options->output, "\n");
            break;
        }

        print_instruction(opcode, bytecode, &offset, options);
    }

    fprintf(options->output, "\n; End of disassembly\n");
}

void print_instruction(uint8_t opcode, const uint8_t* data, size_t* offset, AsmOutputOptions* options) {
    size_t start_offset = *offset;

    print_address(start_offset, options);

    // Print hex dump if requested
    if (options->show_hex_dump) {
        fprintf(options->output, "%02X ", opcode);
    }

    fprintf(options->output, "    %-12s", get_opcode_name(opcode));

    (*offset)++; // Move past opcode

    // Handle instruction-specific operands
    switch (opcode) {
        case 0x01: // LOAD_CONST
            if (*offset + 8 <= 1000) { // Safety check
                uint64_t value = *(uint64_t*)(data + *offset);
                fprintf(options->output, " 0x%016llX", (unsigned long long)value);
                *offset += 8;
            }
            break;

        case 0x20: // LIBC_CALL
            if (*offset + 2 <= 1000) {
                uint16_t func_id = *(uint16_t*)(data + *offset);
                uint8_t arg_count = data[*offset + 2];
                fprintf(options->output, " 0x%04X, %u", func_id, arg_count);
                *offset += 3;
                if (options->show_comments) {
                    fprintf(options->output, "        ; libc function call");
                }
            }
            break;

        case 0x21: // USER_CALL
            if (*offset + 1 <= 1000) {
                uint8_t arg_count = data[*offset];
                fprintf(options->output, " %u", arg_count);
                *offset += 1;
                if (options->show_comments) {
                    fprintf(options->output, "             ; user function call");
                }
            }
            break;

        case 0x30: // STRING_LITERAL
            if (*offset + 2 <= 1000) {
                uint16_t str_len = *(uint16_t*)(data + *offset);
                *offset += 2;
                fprintf(options->output, " \"");

                // Print string content (safely)
                for (int i = 0; i < str_len && *offset < 1000; i++, (*offset)++) {
                    char c = data[*offset];
                    if (c == '\n') fprintf(options->output, "\\n");
                    else if (c == '\t') fprintf(options->output, "\\t");
                    else if (c == '\r') fprintf(options->output, "\\r");
                    else if (c == '\"') fprintf(options->output, "\\\"");
                    else if (c >= 32 && c <= 126) fprintf(options->output, "%c", c);
                    else fprintf(options->output, "\\x%02X", (unsigned char)c);
                }
                fprintf(options->output, "\"");
            }
            break;

        case 0x31: // INT_LITERAL
            if (*offset + 8 <= 1000) {
                int64_t value = *(int64_t*)(data + *offset);
                fprintf(options->output, " %lld", (long long)value);
                *offset += 8;
            }
            break;

        case 0x90: // MODULE_DECL
        case 0x91: // IMPORT
        case 0x92: // EXPORT
            if (*offset + 2 <= 1000) {
                uint16_t name_len = *(uint16_t*)(data + *offset);
                *offset += 2;
                fprintf(options->output, " \"");

                for (int i = 0; i < name_len && *offset < 1000; i++, (*offset)++) {
                    char c = data[*offset];
                    if (c >= 32 && c <= 126) fprintf(options->output, "%c", c);
                    else fprintf(options->output, "\\x%02X", (unsigned char)c);
                }
                fprintf(options->output, "\"");

                if (options->show_comments) {
                    if (opcode == 0x90) fprintf(options->output, "        ; module declaration");
                    else if (opcode == 0x91) fprintf(options->output, "        ; import statement");
                    else if (opcode == 0x92) fprintf(options->output, "        ; export statement");
                }
            }
            break;

        default:
            if (options->show_comments) {
                fprintf(options->output, "                ; unknown/simple instruction");
            }
            break;
    }

    fprintf(options->output, "\n");
}

void print_address(size_t address, AsmOutputOptions* options) {
    if (options->show_addresses) {
        fprintf(options->output, "%04X: ", (unsigned int)address);
    }
}

void print_hex_bytes(const uint8_t* data, size_t count, AsmOutputOptions* options) {
    if (options->show_hex_dump) {
        for (size_t i = 0; i < count; i++) {
            fprintf(options->output, "%02X ", data[i]);
        }
        // Pad to fixed width
        for (size_t i = count; i < 8; i++) {
            fprintf(options->output, "   ");
        }
    }
}

void print_indent(AsmOutputOptions* options) {
    for (int i = 0; i < options->indent_level; i++) {
        fprintf(options->output, "    ");
    }
}

const char* get_opcode_name(uint8_t opcode) {
    if (opcode < sizeof(astc_opcode_names) / sizeof(astc_opcode_names[0]) &&
        astc_opcode_names[opcode] != NULL) {
        return astc_opcode_names[opcode];
    }

    static char unknown_buf[16];
    snprintf(unknown_buf, sizeof(unknown_buf), "UNK_%02X", opcode);
    return unknown_buf;
}
