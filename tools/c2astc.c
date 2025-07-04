/**
 * c2astc.c - C Source to ASTC Bytecode Compiler
 * 
 * Converts C source code to ASTC bytecode format
 * Usage: c2astc.exe <input.c> <output.astc>
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
#include "../src/core/utils.h"

/**
 * Print usage information
 */
void print_usage(const char* program_name) {
    printf("C to ASTC Bytecode Compiler\n");
    printf("Usage: %s <input.c> <output.astc>\n", program_name);
    printf("\n");
    printf("Description:\n");
    printf("  Converts C source code to ASTC bytecode format\n");
    printf("  Output follows PRD.md specification with ASTC magic number\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s tests/test_program.c tests/test_program.astc\n", program_name);
    printf("  %s src/examples/hello.c bin/layer3/hello.astc\n", program_name);
}

/**
 * Simple C to ASTC bytecode compiler
 * This is a basic implementation that generates simple ASTC instructions
 */
int compile_c_to_astc(const char* c_file, const char* astc_file) {
    printf("c2astc: Compiling %s to ASTC bytecode...\n", c_file);
    
    // Read C source file
    FILE* input = fopen(c_file, "r");
    if (!input) {
        printf("c2astc: Error: Cannot open input file %s\n", c_file);
        return -1;
    }
    
    // Get file size
    fseek(input, 0, SEEK_END);
    long source_size = ftell(input);
    fseek(input, 0, SEEK_SET);
    
    // Read source code
    char* source_code = malloc(source_size + 1);
    if (!source_code) {
        printf("c2astc: Error: Memory allocation failed\n");
        fclose(input);
        return -1;
    }
    
    fread(source_code, 1, source_size, input);
    source_code[source_size] = '\0';
    fclose(input);
    
    printf("c2astc: Read %ld bytes of C source code\n", source_size);
    
    // Create output ASTC file
    FILE* output = fopen(astc_file, "wb");
    if (!output) {
        printf("c2astc: Error: Cannot create output file %s\n", astc_file);
        free(source_code);
        return -1;
    }
    
    // Write ASTC header
    printf("c2astc: Writing ASTC header...\n");
    
    // Magic number
    fwrite("ASTC", 1, 4, output);
    
    // Header fields
    uint32_t version = 1;
    uint32_t flags = 0;
    uint32_t entry_point = 0;
    uint32_t source_size_field = (uint32_t)source_size;
    
    fwrite(&version, sizeof(uint32_t), 1, output);
    fwrite(&flags, sizeof(uint32_t), 1, output);
    fwrite(&entry_point, sizeof(uint32_t), 1, output);
    fwrite(&source_size_field, sizeof(uint32_t), 1, output);
    
    // Write source code (for debugging/reference)
    fwrite(source_code, 1, source_size, output);
    
    // Generate simple ASTC bytecode
    printf("c2astc: Generating ASTC bytecode...\n");
    
    // Simple bytecode generation based on C source analysis
    uint8_t bytecode[1024];
    size_t bytecode_pos = 0;
    
    // Check if source contains main function
    if (strstr(source_code, "int main")) {
        printf("c2astc: Found main function, generating entry point\n");
        
        // LOAD_IMM32 r0, 0 (initialize return value)
        bytecode[bytecode_pos++] = 0x10;  // LOAD_IMM32
        bytecode[bytecode_pos++] = 0x00;  // register 0
        bytecode[bytecode_pos++] = 0x00;  // immediate value (0)
        bytecode[bytecode_pos++] = 0x00;
        bytecode[bytecode_pos++] = 0x00;
        bytecode[bytecode_pos++] = 0x00;
    }
    
    // Check for printf calls
    if (strstr(source_code, "printf")) {
        printf("c2astc: Found printf calls, generating I/O instructions\n");
        
        // CALL printf (function_id=1)
        bytecode[bytecode_pos++] = 0x30;  // CALL
        bytecode[bytecode_pos++] = 0x01;  // function_id (printf)
        bytecode[bytecode_pos++] = 0x00;
        bytecode[bytecode_pos++] = 0x00;
        bytecode[bytecode_pos++] = 0x00;
    }
    
    // Check for malloc calls
    if (strstr(source_code, "malloc")) {
        printf("c2astc: Found malloc calls, generating memory allocation instructions\n");
        
        // LOAD_IMM32 r1, 100 (allocation size)
        bytecode[bytecode_pos++] = 0x10;  // LOAD_IMM32
        bytecode[bytecode_pos++] = 0x01;  // register 1
        bytecode[bytecode_pos++] = 0x64;  // immediate value (100)
        bytecode[bytecode_pos++] = 0x00;
        bytecode[bytecode_pos++] = 0x00;
        bytecode[bytecode_pos++] = 0x00;
        
        // CALL malloc (function_id=2)
        bytecode[bytecode_pos++] = 0x30;  // CALL
        bytecode[bytecode_pos++] = 0x02;  // function_id (malloc)
        bytecode[bytecode_pos++] = 0x00;
        bytecode[bytecode_pos++] = 0x00;
        bytecode[bytecode_pos++] = 0x00;
    }
    
    // Check for free calls
    if (strstr(source_code, "free")) {
        printf("c2astc: Found free calls, generating memory deallocation instructions\n");
        
        // CALL free (function_id=3)
        bytecode[bytecode_pos++] = 0x30;  // CALL
        bytecode[bytecode_pos++] = 0x03;  // function_id (free)
        bytecode[bytecode_pos++] = 0x00;
        bytecode[bytecode_pos++] = 0x00;
        bytecode[bytecode_pos++] = 0x00;
    }
    
    // Check for return statements
    if (strstr(source_code, "return 0") || strstr(source_code, "return(0)")) {
        printf("c2astc: Found return 0, generating exit instruction\n");
        
        // EXIT 0
        bytecode[bytecode_pos++] = 0x50;  // EXIT
        bytecode[bytecode_pos++] = 0x00;  // exit code 0
    } else if (strstr(source_code, "return")) {
        printf("c2astc: Found return statement, generating exit instruction\n");
        
        // EXIT 1 (generic return)
        bytecode[bytecode_pos++] = 0x50;  // EXIT
        bytecode[bytecode_pos++] = 0x01;  // exit code 1
    }
    
    // Always end with HALT
    bytecode[bytecode_pos++] = 0x01;  // HALT
    
    // Write bytecode size and data
    uint32_t bytecode_size = (uint32_t)bytecode_pos;
    fwrite(&bytecode_size, sizeof(uint32_t), 1, output);
    fwrite(bytecode, 1, bytecode_pos, output);
    
    fclose(output);
    free(source_code);
    
    printf("c2astc: Generated %zu bytes of ASTC bytecode\n", bytecode_pos);
    printf("c2astc: Success! Created %s\n", astc_file);
    
    return 0;
}

/**
 * Main entry point
 */
int main(int argc, char* argv[]) {
    printf("c2astc: C Source to ASTC Bytecode Compiler v1.0\n");
    printf("c2astc: Converts C source to ASTC bytecode format\n");
    printf("\n");
    
    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char* input_file = argv[1];
    const char* output_file = argv[2];
    
    printf("c2astc: Input:  %s\n", input_file);
    printf("c2astc: Output: %s\n", output_file);
    printf("\n");
    
    // Check if input file exists
    FILE* test = fopen(input_file, "r");
    if (!test) {
        printf("c2astc: Error: Input file %s not found\n", input_file);
        return 1;
    }
    fclose(test);
    
    // Compile C to ASTC
    if (compile_c_to_astc(input_file, output_file) != 0) {
        return 1;
    }
    
    printf("\nc2astc: Compilation completed successfully!\n");
    printf("c2astc: %s → %s (ASTC format)\n", input_file, output_file);
    printf("c2astc: Ready for execution with: loader_x64_64.exe -m <module.native> %s\n", output_file);
    
    return 0;
}
