/**
 * c2native.c - C Source to Native Module Converter
 * 
 * Converts C source code to .native format (pure machine code without OS headers)
 * Usage: c2native.exe <input.c> <output.native>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

// Include core components
#include "../src/core/native.h"
#include "../src/core/utils.h"

/**
 * Print usage information
 */
void print_usage(const char* program_name) {
    printf("C to Native Module Converter\n");
    printf("Usage: %s <input.c> <output.native>\n", program_name);
    printf("\n");
    printf("Description:\n");
    printf("  Converts C source code to .native format (pure machine code)\n");
    printf("  Output follows PRD.md specification with NATV magic number\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s src/ext/std_module.c bin/layer2/std_x64_64.native\n", program_name);
    printf("  %s src/ext/vm_module.c bin/layer2/vm_x64_64.native\n", program_name);
}

/**
 * Compile C source to object file using TCC
 */
int compile_c_to_object(const char* c_file, const char* obj_file) {
    printf("c2native: Compiling %s to object file...\n", c_file);
    
    char command[2048];
    snprintf(command, sizeof(command), 
        "external\\tcc-win\\tcc\\tcc.exe -c -o \"%s\" \"%s\" "
        "-Isrc/core -Isrc/ext "
        "-DNDEBUG -O2",
        obj_file, c_file);
    
    printf("c2native: Running: %s\n", command);
    
    int result = system(command);
    if (result != 0) {
        printf("c2native: Error: TCC compilation failed with code %d\n", result);
        return -1;
    }
    
    printf("c2native: Successfully compiled to %s\n", obj_file);
    return 0;
}

/**
 * Extract machine code from object file (remove PE/ELF headers)
 */
int extract_machine_code(const char* obj_file, uint8_t** code_data, size_t* code_size) {
    printf("c2native: Extracting machine code from %s...\n", obj_file);

    FILE* file = fopen(obj_file, "rb");
    if (!file) {
        printf("c2native: Error: Cannot open object file %s\n", obj_file);
        return -1;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0) {
        printf("c2native: Error: Invalid object file size\n");
        fclose(file);
        return -1;
    }

    // Read entire file for now
    // TODO: Implement proper PE/COFF parsing to extract only .text section
    uint8_t* file_data = malloc(file_size);
    if (!file_data) {
        printf("c2native: Error: Memory allocation failed\n");
        fclose(file);
        return -1;
    }

    size_t read_size = fread(file_data, 1, file_size, file);
    if (read_size != file_size) {
        printf("c2native: Error: Failed to read object file\n");
        free(file_data);
        fclose(file);
        return -1;
    }
    fclose(file);

    // Simple heuristic: skip first 1024 bytes (headers) and take rest as code
    // This is a temporary solution until proper PE/COFF parsing is implemented
    size_t header_skip = 1024;
    if (file_size > header_skip) {
        *code_size = file_size - header_skip;
        *code_data = malloc(*code_size);
        if (*code_data) {
            memcpy(*code_data, file_data + header_skip, *code_size);
            printf("c2native: Extracted %zu bytes of machine code (skipped %zu header bytes)\n",
                   *code_size, header_skip);
        } else {
            printf("c2native: Error: Memory allocation failed for code data\n");
            free(file_data);
            return -1;
        }
    } else {
        // File too small, use entire file
        *code_size = file_size;
        *code_data = file_data;
        file_data = NULL; // Transfer ownership
        printf("c2native: Extracted %zu bytes (entire file as machine code)\n", *code_size);
    }

    if (file_data) free(file_data);
    return 0;
}

/**
 * Create .native format file
 */
int create_native_file(const char* output_file, const uint8_t* code_data, size_t code_size) {
    printf("c2native: Creating .native format file %s...\n", output_file);
    
    // Create native module
    NativeModule* module = native_module_create(NATIVE_ARCH_X86_64, NATIVE_TYPE_USER);
    if (!module) {
        printf("c2native: Error: Failed to create native module\n");
        return -1;
    }
    
    // Set machine code
    if (native_module_set_code(module, code_data, code_size, 0) != 0) {
        printf("c2native: Error: Failed to set code section\n");
        native_module_free(module);
        return -1;
    }
    
    // Add basic exports (these would normally be parsed from the C source)
    // For now, add common function exports
    native_module_add_export(module, "main", NATIVE_EXPORT_FUNCTION, 0, 0);
    native_module_add_export(module, "module_init", NATIVE_EXPORT_FUNCTION, 0, 0);
    native_module_add_export(module, "module_cleanup", NATIVE_EXPORT_FUNCTION, 0, 0);
    
    // Write to .native file
    if (native_module_write_file(module, output_file) != 0) {
        printf("c2native: Error: Failed to write .native file\n");
        native_module_free(module);
        return -1;
    }
    
    // Verify the file
    FILE* test_file = fopen(output_file, "rb");
    if (test_file) {
        fseek(test_file, 0, SEEK_END);
        long file_size = ftell(test_file);
        fclose(test_file);
        
        // Verify magic number
        test_file = fopen(output_file, "rb");
        uint32_t magic;
        if (fread(&magic, sizeof(uint32_t), 1, test_file) == 1) {
            if (magic == NATIVE_MAGIC) {
                printf("c2native: Success! Created %s (%ld bytes, NATV format)\n", 
                       output_file, file_size);
            } else {
                printf("c2native: Warning: Magic number mismatch (0x%08X)\n", magic);
            }
        }
        fclose(test_file);
    }
    
    native_module_free(module);
    return 0;
}

/**
 * Main entry point
 */
int main(int argc, char* argv[]) {
    printf("c2native: C Source to Native Module Converter v1.0\n");
    printf("c2native: Converts C source to .native format (pure machine code)\n");
    printf("\n");
    
    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char* input_file = argv[1];
    const char* output_file = argv[2];
    
    printf("c2native: Input:  %s\n", input_file);
    printf("c2native: Output: %s\n", output_file);
    printf("\n");
    
    // Check if input file exists
    FILE* test = fopen(input_file, "r");
    if (!test) {
        printf("c2native: Error: Input file %s not found\n", input_file);
        return 1;
    }
    fclose(test);
    
    // Step 1: Compile C source to object file
    char temp_obj[512];
    snprintf(temp_obj, sizeof(temp_obj), "%s.tmp.o", output_file);
    
    if (compile_c_to_object(input_file, temp_obj) != 0) {
        return 1;
    }
    
    // Step 2: Extract machine code from object file
    uint8_t* code_data = NULL;
    size_t code_size = 0;
    
    if (extract_machine_code(temp_obj, &code_data, &code_size) != 0) {
        remove(temp_obj);
        return 1;
    }
    
    // Step 3: Create .native format file
    if (create_native_file(output_file, code_data, code_size) != 0) {
        free(code_data);
        remove(temp_obj);
        return 1;
    }
    
    // Cleanup
    free(code_data);
    remove(temp_obj);
    
    printf("\nc2native: Conversion completed successfully!\n");
    printf("c2native: %s → %s (NATV format)\n", input_file, output_file);
    
    return 0;
}
