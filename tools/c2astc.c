/**
 * c2astc.c - C Source to ASTC Bytecode Compiler Tool
 * 
 * This tool uses the pipeline module to convert C source code to ASTC bytecode format.
 * It serves as a command-line interface to the c2astc functionality in the pipeline module.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// Include core components
#include "../src/core/astc.h"
#include "../src/core/module.h"

/**
 * Print usage information
 */
void print_usage(const char* program_name) {
    printf("C to ASTC Bytecode Compiler v2.0\n");
    printf("Usage: %s <input.c> <output.astc>\n", program_name);
    printf("\n");
    printf("Description:\n");
    printf("  Converts C source code to ASTC bytecode format using the pipeline module\n");
    printf("  Output follows the updated ASTC specification with proper serialization\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s tests/test_program.c tests/test_program.astc\n", program_name);
    printf("  %s examples/hello_world.c examples/hello_world.astc\n", program_name);
}

/**
 * Load and read C source file
 */
char* read_source_file(const char* filename, size_t* size) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("c2astc: Error: Cannot open input file %s\n", filename);
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        printf("c2astc: Error: Input file is empty or invalid\n");
        fclose(file);
        return NULL;
    }
    
    // Read source code
    char* source_code = malloc(file_size + 1);
    if (!source_code) {
        printf("c2astc: Error: Memory allocation failed\n");
        fclose(file);
        return NULL;
    }
    
    size_t bytes_read = fread(source_code, 1, file_size, file);
    source_code[bytes_read] = '\0';
    fclose(file);
    
    if (size) *size = bytes_read;
    return source_code;
}

/**
 * Write ASTC bytecode to file
 */
int write_astc_file(const char* filename, ASTCBytecodeProgram* program) {
    if (!filename || !program) return -1;
    
    FILE* file = fopen(filename, "wb");
    if (!file) {
        printf("c2astc: Error: Cannot create output file %s\n", filename);
        return -1;
    }
    
    // Write magic number
    fwrite(program->magic, 1, 4, file);
    
    // Write header fields
    fwrite(&program->version, sizeof(uint32_t), 1, file);
    fwrite(&program->flags, sizeof(uint32_t), 1, file);
    fwrite(&program->entry_point, sizeof(uint32_t), 1, file);
    
    // Write instruction count and instructions
    fwrite(&program->instruction_count, sizeof(uint32_t), 1, file);
    
    for (uint32_t i = 0; i < program->instruction_count; i++) {
        ASTCInstruction* instr = &program->instructions[i];
        fwrite(&instr->opcode, sizeof(ASTNodeType), 1, file);
        fwrite(&instr->operand, sizeof(instr->operand), 1, file);
    }
    
    // Write data section if present
    fwrite(&program->data_size, sizeof(uint32_t), 1, file);
    if (program->data_size > 0 && program->data) {
        fwrite(program->data, 1, program->data_size, file);
    }
    
    // Write symbol table if present
    fwrite(&program->symbol_count, sizeof(uint32_t), 1, file);
    if (program->symbol_count > 0) {
        // Write symbol names
        for (uint32_t i = 0; i < program->symbol_count; i++) {
            if (program->symbol_names && program->symbol_names[i]) {
                uint32_t name_len = strlen(program->symbol_names[i]);
                fwrite(&name_len, sizeof(uint32_t), 1, file);
                fwrite(program->symbol_names[i], 1, name_len, file);
            } else {
                uint32_t name_len = 0;
                fwrite(&name_len, sizeof(uint32_t), 1, file);
            }
        }
        
        // Write symbol addresses
        if (program->symbol_addresses) {
            fwrite(program->symbol_addresses, sizeof(uint32_t), program->symbol_count, file);
        }
    }
    
    fclose(file);
    return 0;
}

/**
 * Compile C source to ASTC bytecode using pipeline module
 */
int compile_c_to_astc(const char* c_file, const char* astc_file) {
    printf("c2astc: Compiling %s to ASTC bytecode...\n", c_file);
    
    // Read C source file
    size_t source_size;
    char* source_code = read_source_file(c_file, &source_size);
    if (!source_code) {
        return -1;
    }
    
    printf("c2astc: Read %zu bytes of C source code\n", source_size);
    
    // Load pipeline module
    printf("c2astc: Loading pipeline module...\n");
    Module* pipeline = load_module("./bin/pipeline");
    if (!pipeline) {
        printf("c2astc: Error: Failed to load pipeline module\n");
        printf("c2astc: Make sure pipeline_*.native module is available in bin/\n");
        free(source_code);
        return -1;
    }
    
    // Get compilation function - fix signature
    typedef struct {
        int optimize_level;
        bool enable_debug;
        bool enable_warnings;
        char output_file[256];
    } CompileOptions;
    
    bool (*pipeline_compile)(const char*, CompileOptions*) = pipeline->resolve("pipeline_compile");
    ASTCBytecodeProgram* (*pipeline_get_astc_program)(void) = pipeline->resolve("pipeline_get_astc_program");
    const char* (*pipeline_get_error)(void) = pipeline->resolve("pipeline_get_error");
    
    if (!pipeline_compile || !pipeline_get_astc_program || !pipeline_get_error) {
        printf("c2astc: Error: Pipeline module missing required functions\n");
        free(source_code);
        return -1;
    }
    
    // Create compile options
    CompileOptions options = {0};
    options.optimize_level = 0;  // No optimization
    options.enable_debug = false;
    options.enable_warnings = true;
    strncpy(options.output_file, astc_file, sizeof(options.output_file) - 1);
    
    // Compile source code
    printf("c2astc: Compiling C source to ASTC...\n");
    if (!pipeline_compile(source_code, &options)) {
        printf("c2astc: Error: Compilation failed: %s\n", pipeline_get_error());
        free(source_code);
        return -1;
    }
    
    // Get ASTC bytecode program
    ASTCBytecodeProgram* astc_program = pipeline_get_astc_program();
    if (!astc_program) {
        printf("c2astc: Error: Failed to get ASTC bytecode program\n");
        free(source_code);
        return -1;
    }
    
    printf("c2astc: Generated %u ASTC instructions\n", astc_program->instruction_count);
    
    // Write ASTC file
    printf("c2astc: Writing ASTC bytecode to %s...\n", astc_file);
    if (write_astc_file(astc_file, astc_program) != 0) {
        printf("c2astc: Error: Failed to write ASTC file\n");
        free(source_code);
        return -1;
    }
    
    printf("c2astc: Success! Created %s\n", astc_file);
    printf("c2astc: ASTC file contains %u instructions\n", astc_program->instruction_count);
    
    free(source_code);
    return 0;
}

/**
 * Main entry point
 */
int main(int argc, char* argv[]) {
    printf("c2astc: C Source to ASTC Bytecode Compiler v2.0 (Module-based)\n");
    
    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char* input_file = argv[1];
    const char* output_file = argv[2];
    
    printf("c2astc: Input:  %s\n", input_file);
    printf("c2astc: Output: %s\n", output_file);
    
    // Check if input file exists
    FILE* test_file = fopen(input_file, "r");
    if (!test_file) {
        printf("c2astc: Error: Input file %s does not exist or cannot be read\n", input_file);
        return 1;
    }
    fclose(test_file);
    
    // Compile
    int result = compile_c_to_astc(input_file, output_file);
    
    if (result == 0) {
        printf("c2astc: Compilation completed successfully\n");
    } else {
        printf("c2astc: Compilation failed\n");
    }
    
    return result;
}
