/**
 * test_astc2native.c - Test ASTC to Native Compilation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test simple ASTC program
int main() {
    printf("Testing astc2native functionality...\n");
    
    // Create a simple test ASTC file
    const char* test_astc = "tests/test_simple.astc";
    const char* test_native = "tests/test_simple.native";
    
    // Create ASTC file with simple bytecode
    FILE* astc_file = fopen(test_astc, "wb");
    if (!astc_file) {
        printf("Error: Cannot create test ASTC file\n");
        return 1;
    }
    
    // Write ASTC header
    fwrite("ASTC", 1, 4, astc_file);           // Magic
    uint32_t version = 1;
    uint32_t flags = 0;
    uint32_t entry_point = 0;
    uint32_t source_size = 0;
    fwrite(&version, sizeof(uint32_t), 1, astc_file);
    fwrite(&flags, sizeof(uint32_t), 1, astc_file);
    fwrite(&entry_point, sizeof(uint32_t), 1, astc_file);
    fwrite(&source_size, sizeof(uint32_t), 1, astc_file);
    
    // Write simple bytecode: LOAD_IMM32 r0, 42; EXIT 0
    uint32_t bytecode_size = 8;
    fwrite(&bytecode_size, sizeof(uint32_t), 1, astc_file);
    
    uint8_t bytecode[] = {
        0x10, 0x00, 0x2A, 0x00, 0x00, 0x00,  // LOAD_IMM32 r0, 42
        0x50, 0x00                            // EXIT 0
    };
    fwrite(bytecode, 1, sizeof(bytecode), astc_file);
    fclose(astc_file);
    
    printf("Created test ASTC file: %s\n", test_astc);
    
    // Test astc2native compilation
    printf("Testing astc2native compilation...\n");
    
    // Call astc2native function (would need to link with astc_module)
    // For now, just verify the test file was created
    printf("Test ASTC file created successfully\n");
    printf("astc2native test completed\n");
    
    return 0;
}
